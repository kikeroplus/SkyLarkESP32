// 音の合成と、I2S (MAX98357A) へ流すところ。波形は Windows 版の Sound.cs と同じ作り方 (標本の速さも同じ 22050)
#include "sound.h"
#include "config.h"
#include <driver/i2s.h>
#include <math.h>
#include <LittleFS.h>

namespace
{
const int SR = 22050;
const int BUF = 256; // 1 回に混ぜる標本の数 (約 12ms)
const float PI2 = 6.28318531f;

struct Wave
{
  int16_t *w = nullptr;
  int n = 0;
  float vol = 0;
};
Wave waves[(int)Sfx::Count];

struct Voice
{
  const Wave *w;
  int pos;
  float vol;
  bool on;
};
const int VOICES = 16;
Voice voices[VOICES];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Sound *self = nullptr;

uint32_t rng = 0x12345678;
inline float noise() // -1〜1
{
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return (int32_t)rng / 2147483648.0f;
}
inline float rnd01() { return (rng = rng * 1664525u + 1013904223u) / 4294967296.0f; }
inline void breathe(int i) // コア 0 の見張り (IDLE) が止まらないように、ときどき休む
{
  if ((i & 4095) == 4095)
    vTaskDelay(1);
}
inline float square(float ph) { return ph - floorf(ph) < 0.5f ? 1.0f : -1.0f; }

// 作りかけの波形 (float)。できたら 16 ビットにして PSRAM へ写す
float *tmp = nullptr;
int tmpN = 0;
float *newWave(float sec)
{
  vTaskDelay(1); // コア 0 の見張り (IDLE) が止まらないように、波形ごとに休む
  tmpN = (int)(SR * sec);
  memset(tmp, 0, tmpN * sizeof(float));
  return tmp;
}
void set(Sfx s, float vol)
{
  Wave &w = waves[(int)s];
  float peak = 0;
  for (int i = 0; i < tmpN; i++)
    peak = max(peak, fabsf(tmp[i]));
  float k = peak > 1 ? 1 / peak : 1; // 16 ビットに入るように (音量は vol 側で戻す)
  int16_t *d = (int16_t *)ps_malloc(tmpN * 2);
  if (!d)
    return;
  for (int i = 0; i < tmpN; i++)
    d[i] = (int16_t)(tmp[i] * k * 32767);
  w.n = tmpN;
  w.vol = vol / k;
  __sync_synchronize();
  w.w = d; // できあがってから見せる (Play は w が nullptr の音を鳴らさない)
}

// 音符を並べる (矩形波に少し三角を混ぜた、やわらかい笛の音)。notes は半音 (A4 = 0)、len は秒
void melody(std::initializer_list<int> notes, std::initializer_list<float> lens, float duty)
{
  float total = 0;
  for (float l : lens)
    total += l;
  float *w = newWave(total + 0.1f);
  int p = 0;
  auto li = lens.begin();
  for (int note : notes)
  {
    int n = (int)(SR * *li++);
    float f = 440 * powf(2, note / 12.0f), ph = 0;
    for (int i = 0; i < n && p + i < tmpN; i++)
    {
      breathe(i);
      ph += f / SR;
      float fr = ph - floorf(ph);
      float sq = fr < duty ? 1.0f : -1.0f, tri = 1 - 4 * fabsf(fr - 0.5f);
      float env = min(1.0f, i / (SR * 0.005f)) * expf(-2.5f * i / n);
      w[p + i] += (sq * 0.35f + tri * 0.65f) * env;
    }
    p += n;
  }
}

// 爆発: 低いほうへこもっていく雑音と、ドンという低い音
void explosion(float sec, float lpStart, float thump)
{
  float *w = newWave(sec);
  float lp = 0, lp2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, k = t / sec;
    float cut = lpStart * 4 * (1 - k) + lpStart * 0.3f;
    lp += (noise() - lp) * min(0.9f, cut * 3);
    lp2 += (lp - lp2) * min(0.9f, cut * 3);
    float env = min(1.0f, t / 0.004f) * powf(1 - k, 2.2f);
    w[i] = (lp2 * 2.2f + sinf(t * PI2 * thump * (1 - 0.5f * k)) * 0.6f * expf(-t * 6)) * env;
  }
}

// 円盤を落とした: 「ちゅ」と高いところから一気に下がる音のあとに、「どーん」と腹に響く音。
// 小さなスピーカーでは低い音そのものは出ないので、どーんの低い音はわざと歪ませて倍音を作り、聞こえるようにする
void chudoon(float sec)
{
  float *w = newWave(sec);
  const float D = 0.07f; // ちゅ の長さ。どーん はここから
  float ph = 0, tph = 0, lp = 0, lp2 = 0, hp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, v = 0;
    if (t < D + 0.03f) // ちゅ: 2200Hz から 330Hz へ急に下がる、矩形波と雑音
    {
      ph += 2200 * powf(0.15f, min(1.0f, t / D)) / SR;
      float env = min(1.0f, t / 0.002f) * (1 - t / (D + 0.03f));
      v += (square(ph) * 0.5f + noise() * 0.25f) * env * 0.75f;
    }
    if (t >= D) // どーん: 120Hz から 45Hz へ沈む低い音 (歪ませる) と、こもっていく地鳴りの雑音
    {
      float u = t - D, k = u / (sec - D);
      tph += (45 + 75 * expf(-u * 7)) / SR;
      float body = tanhf(sinf(tph * PI2) * 2.5f) * expf(-u * 4.5f);
      float cut = 0.25f * expf(-u * 5) + 0.03f;
      lp += (noise() - lp) * cut;
      lp2 += (lp - lp2) * cut;
      hp += (noise() - hp) * 0.6f; // 当たった瞬間のバリッ (すぐ消える)
      float crack = hp * expf(-u * 40) * 0.5f;
      float env = min(1.0f, u / 0.004f);
      v += (body * 0.8f + lp2 * 3 * powf(1 - k, 1.8f) + crack) * env;
    }
    w[i] = v;
  }
}

// 連れ去られる鳴き声: 震えるメエ (モー) の声が、高さを下げながら遠ざかる (だんだん小さく、こもり、こだまが残る)
void bleat(float sec, float f0, float f1, float trem)
{
  float *w = newWave(sec + 0.4f);
  float ph = 0, lp = 0;
  for (int i = 0; i < (int)(SR * sec); i++)
  {
    float t = (float)i / SR, k = t / sec;
    float f = f0 + (f1 - f0) * k + 12 * sinf(t * PI2 * trem);
    ph += f / SR;
    float saw = (ph - floorf(ph)) * 2 - 1;
    float voice = saw * (0.6f + 0.4f * sinf(t * PI2 * trem));
    lp += (voice - lp) * (0.5f - 0.42f * k); // 遠ざかるほどこもる
    float env = min(1.0f, t / 0.04f) * powf(1 - k, 1.6f);
    w[i] += lp * env;
  }
  int echo = (int)(SR * 0.22f);
  for (int i = tmpN - 1; i >= echo; i--)
    w[i] += w[i - echo] * 0.3f;
}

void makeWaves()
{
  tmp = (float *)ps_malloc(SR * 4 * sizeof(float));
  float *w;
  // 機銃: 短い雑音のパチッと、下がる低い音
  w = newWave(0.07f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (noise() * 0.6f + square(t * (520 - 2600 * t)) * 0.4f) * expf(-t * 55);
  }
  set(Sfx::Shot, 0.22f);
  // 弾が円盤に当たった
  w = newWave(0.05f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = square(t * 1700) * expf(-t * 80) * 0.6f + noise() * expf(-t * 120) * 0.4f;
  }
  set(Sfx::Hit, 0.18f);
  // 母船に当たった: 金属を打つ音
  w = newWave(0.09f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * 1230) + sinf(t * PI2 * 1847) * 0.7f) * expf(-t * 45) * 0.5f;
  }
  set(Sfx::MomHit, 0.14f);
  chudoon(0.9f);
  set(Sfx::Boom, 0.3f);
  explosion(1.1f, 0.06f, 50);
  set(Sfx::BigBoom, 0.42f);
  explosion(3.2f, 0.035f, 38);
  set(Sfx::Crash, 0.5f);
  // 自機がやられた: 爆発に下がっていく音を重ねる
  explosion(1.2f, 0.08f, 60);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    tmp[i] += square(t * (700 - 260 * t)) * expf(-t * 3) * 0.25f;
  }
  set(Sfx::Death, 0.42f);
  melody({3, 7, 10, 15, 19}, {0.05f, 0.05f, 0.05f, 0.05f, 0.16f}, 0.25f);
  set(Sfx::Power, 0.3f);
  // 羊を助けた: メエと鳴いて、鈴の音
  w = newWave(0.75f);
  for (int i = 0; i < (int)(SR * 0.38f); i++)
  {
    float t = (float)i / SR, f = 300 + 25 * sinf(t * PI2 * 22);
    float ph = t * f, saw = (ph - floorf(ph)) * 2 - 1;
    float env = min(1.0f, t / 0.03f) * min(1.0f, (0.38f - t) / 0.08f);
    w[i] = saw * (0.6f + 0.4f * sinf(t * PI2 * 22)) * env * 0.5f;
  }
  const float bells[2] = {1568, 2093};
  for (int b = 0; b < 2; b++)
  {
    int st = (int)(SR * (0.36f + b * 0.1f));
    for (int i = 0; st + i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      w[st + i] += (sinf(t * PI2 * bells[b]) + 0.3f * sinf(t * PI2 * bells[b] * 2.76f)) * expf(-t * 9) * 0.45f;
    }
  }
  set(Sfx::Saved, 0.4f);
  melody({3, 7, 10, 7, 10, 15}, {0.09f, 0.09f, 0.09f, 0.09f, 0.09f, 0.35f}, 0.5f);
  set(Sfx::Extend, 0.35f);
  // 警報: 高い・低いをくり返す
  w = newWave(2.6f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    float f = (int)(t * 4) % 2 == 0 ? 740 : 554;
    w[i] = (square(t * f) * 0.5f + sinf(t * f * PI2) * 0.5f) * min(1.0f, (2.6f - t) * 4) * 0.6f;
  }
  set(Sfx::Warning, 0.3f);
  // 汽笛: 3 つの音の和音に息の雑音。少し遅れて鳴り出し、ふっと消える
  w = newWave(1.3f);
  const float wf[3] = {466, 587, 698};
  float lp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, env = min(1.0f, t / 0.08f) * min(1.0f, (1.3f - t) / 0.3f), bend = 1 - 0.03f * expf(-t * 8);
    float v = 0;
    for (float f : wf)
      v += sinf(t * f * bend * PI2);
    lp += (noise() - lp) * 0.3f;
    w[i] = (v * 0.28f + lp * 0.25f) * env;
  }
  set(Sfx::Whistle, 0.3f);
  // 雷: 遠くでゴロゴロ。揺れながら弱まる低い雑音
  w = newWave(2.8f);
  float a = 0, b2 = 0, am = 1;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    a += (noise() - a) * 0.04f;
    b2 += (a - b2) * 0.1f;
    if (i % 1500 == 0)
      am = 0.4f + rnd01() * 0.6f;
    float crack = t < 0.15f ? noise() * (1 - t / 0.15f) * 0.6f : 0;
    w[i] = (b2 * 9 * am + crack) * min(1.0f, t / 0.05f) * expf(-t * 1.3f);
  }
  set(Sfx::Thunder, 0.45f);
  // 円盤の弾: ふわっと下がる小さな音
  w = newWave(0.12f);
  float eph = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    eph += (1400 - 7000 * t) / SR;
    w[i] = sinf(eph * PI2) * expf(-t * 25);
  }
  set(Sfx::EnemyShot, 0.09f);
  // 出発: のどかな短い節
  melody({-2, 2, 5, 10, 9, 5, 7, 10, 14}, {0.14f, 0.14f, 0.14f, 0.28f, 0.14f, 0.14f, 0.14f, 0.14f, 0.5f}, 0.5f);
  set(Sfx::Start, 0.35f);
  melody({10, 9, 5, 2, 0, -2}, {0.22f, 0.22f, 0.22f, 0.22f, 0.22f, 0.7f}, 0.5f);
  set(Sfx::GameOver, 0.35f);
  melody({15}, {0.08f}, 0.5f);
  set(Sfx::Tick, 0.25f);
  melody({22}, {0.04f}, 0.25f);
  set(Sfx::Select, 0.2f);
  melody({15, 10}, {0.06f, 0.1f}, 0.5f);
  set(Sfx::Pause, 0.25f);
  bleat(1.3f, 330, 210, 22);
  set(Sfx::BaaAway, 0.5f);
  bleat(1.6f, 150, 105, 6);
  set(Sfx::MooAway, 0.55f);
  // 牧羊犬: ワンワン
  w = newWave(0.42f);
  for (int k = 0; k < 2; k++)
  {
    int st = (int)(SR * k * 0.2f);
    float lpw = 0, bph = 0;
    for (int i = 0; st + i < tmpN && i < SR * 0.13f; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      bph += (480 - 1500 * t) / SR;
      float saw = (bph - floorf(bph)) * 2 - 1;
      lpw += (noise() - lpw) * 0.4f;
      float env = min(1.0f, t / 0.006f) * expf(-t * 22);
      w[st + i] += (saw * 0.7f + lpw * 0.6f) * env;
    }
  }
  set(Sfx::Woof, 0.35f);
  // カモ: クワッ クワッ (鼻にかかった音)
  w = newWave(0.36f);
  for (int k = 0; k < 2; k++)
  {
    int st = (int)(SR * k * 0.17f);
    float qph = 0;
    for (int i = 0; st + i < tmpN && i < SR * 0.11f; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      qph += (620 - 900 * t) / SR;
      float fr = qph - floorf(qph);
      float pulse = fr < 0.18f ? 1 : -0.25f;
      float env = min(1.0f, t / 0.008f) * expf(-t * 18);
      w[st + i] += pulse * env * (0.7f + 0.3f * sinf(t * PI2 * 60));
    }
  }
  set(Sfx::Quack, 0.25f);
  // 名所を見つけた: 竪琴をつま弾くような、少し不思議な和音の分散
  const int harp[7] = {-12, -5, 0, 3, 7, 12, 14};
  w = newWave(2.2f);
  for (int k = 0; k < 7; k++)
  {
    float f = 440 * powf(2, harp[k] / 12.0f);
    int st = (int)(SR * k * 0.09f);
    for (int i = 0; st + i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      w[st + i] += (sinf(t * f * PI2) + 0.35f * sinf(t * f * 2 * PI2) + 0.15f * sinf(t * f * 3 * PI2)) * expf(-t * 2.6f) *
                   min(1.0f, t / 0.003f) * 0.28f;
    }
  }
  set(Sfx::Discover, 0.4f);
  // ネッシーが水から出る: ザバッという水音と、低いあぶくの音
  w = newWave(1.2f);
  float slp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    slp += (noise() - slp) * 0.25f;
    float bubble = sinf(t * PI2 * (90 + 40 * sinf(t * 30))) * expf(-t * 3) * 0.4f;
    w[i] = slp * 1.6f * expf(-t * 4) * min(1.0f, t / 0.02f) + bubble;
  }
  set(Sfx::Splash, 0.35f);
  // ストーンヘンジから円盤が湧く: 低いところから上ずっていく、揺れる不思議な音
  w = newWave(1.4f);
  float eph2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    eph2 += (120 + 700 * t * t + 30 * sinf(t * PI2 * 9)) / SR;
    w[i] = (sinf(eph2 * PI2) + 0.4f * sinf(eph2 * 3.01f * PI2)) * min(1.0f, t / 0.3f) * min(1.0f, (1.4f - t) / 0.4f) * 0.5f;
  }
  set(Sfx::Emerge, 0.3f);
  // ボドミンの野獣が座ってうなる: 低くごろごろ震える声
  w = newWave(1.0f);
  float glp = 0, gph = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    glp += (noise() - glp) * 0.08f;
    gph += (75 - 15 * t) / SR;
    float saw = (gph - floorf(gph)) * 2 - 1;
    float trem = 0.55f + 0.45f * sinf(t * PI2 * 24);
    float env = min(1.0f, t / 0.15f) * min(1.0f, (1.0f - t) / 0.35f);
    w[i] = (saw * 0.5f + glp * 3.5f) * trem * env;
  }
  set(Sfx::Growl, 0.45f);
  // 爆弾を落とす: ヒューと下がっていく口笛のような音
  w = newWave(0.8f);
  float bph2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    bph2 += (1500 - 900 * t) / SR;
    w[i] = sinf(bph2 * PI2) * min(1.0f, t / 0.05f) * min(1.0f, (0.8f - t) / 0.2f) * 0.5f;
  }
  set(Sfx::Bomb, 0.16f);
  // 高射砲: ぽん、と軽くこもった音
  w = newWave(0.18f);
  float flp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    flp += (noise() - flp) * 0.15f;
    w[i] = (flp * 2.5f + sinf(t * PI2 * (160 - 300 * t)) * 0.6f) * expf(-t * 30);
  }
  set(Sfx::Flak, 0.22f);
  // 光線円盤が撃つ: ビーッと震える高い音
  w = newWave(1.1f);
  float zph = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    zph += (880 + 60 * sinf(t * PI2 * 30)) / SR;
    float fr = zph - floorf(zph);
    w[i] = ((fr < 0.5f ? 1 : -1) * 0.4f + sinf(zph * PI2 * 1.5f) * 0.4f) * min(1.0f, t / 0.02f) * min(1.0f, (1.1f - t) / 0.3f);
  }
  set(Sfx::BeamFire, 0.13f);
  // 戦車が着地: どすん
  w = newWave(0.4f);
  float dlp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    dlp += (noise() - dlp) * 0.05f;
    w[i] = (sinf(t * PI2 * (70 - 60 * t)) * 0.8f + dlp * 4) * expf(-t * 10);
  }
  set(Sfx::TankDrop, 0.4f);

  // ---- 2 本目 (FOUNDRY) ----
  // 戦車の青い弾: トゥン (少し上から落ちて止まる、丸い音程のある音。短い余韻)
  w = newWave(0.16f);
  float sph = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    sph += 560 * (1 + 0.7f * expf(-t * 70)) / SR;
    float v = sinf(sph * PI2) + 0.25f * sinf(sph * 2 * PI2) + 0.12f * square(sph);
    w[i] = v * min(1.0f, t / 0.003f) * expf(-t * 24);
  }
  set(Sfx::G2Shot, 0.12f);
  // 機械が壊れる: 爆発に、金属が打ち合うガシャンを重ねる
  explosion(0.75f, 0.07f, 55);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    tmp[i] += (sinf(t * PI2 * 310) + sinf(t * PI2 * 487) * 0.8f + sinf(t * PI2 * 733) * 0.6f) * expf(-t * 14) * 0.35f;
  }
  set(Sfx::G2Boom, 0.32f);
  // 核が飛び出す: 震えながら上ずる電子音
  w = newWave(0.35f);
  float cph = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, k = t / 0.35f;
    cph += (600 + 1400 * k + 150 * sinf(t * PI2 * 30)) / SR;
    w[i] = (square(cph) * 0.4f + sinf(cph * PI2) * 0.6f) * min(1.0f, t / 0.01f) * (1 - k);
  }
  set(Sfx::CoreFree, 0.1f);
  // 核を撃ち落とした: ガラスを打ったようなチン
  w = newWave(0.4f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * 2093) + sinf(t * PI2 * 3136) * 0.6f) * expf(-t * 11) * 0.6f + noise() * expf(-t * 90) * 0.3f;
  }
  set(Sfx::CoreKill, 0.2f);
  // 核が乗り移った: ぶるぶる震えながら下がる、こもったうなり
  w = newWave(0.6f);
  float pph = 0, plp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, k = t / 0.6f;
    pph += (900 - 600 * k) / SR;
    float saw = (pph - floorf(pph)) * 2 - 1;
    plp += (saw * (0.6f + 0.4f * sinf(t * PI2 * 40)) - plp) * 0.25f;
    w[i] = plp * min(1.0f, t / 0.02f) * powf(1 - k, 1.3f);
  }
  set(Sfx::Possess, 0.22f);
  // ウイルス: 音の高さがでたらめに跳ぶ、デジタルのジジジ
  w = newWave(0.8f);
  float vph = 0, vf = 800;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    if (i % (SR * 3 / 100) == 0)
      vf = 400 + rnd01() * 2000;
    vph += vf / SR;
    float v = square(vph) * 0.5f + ((i / 40) % 2 ? 0.25f : -0.25f) * (rnd01() > 0.7f ? 1 : 0);
    w[i] = v * min(1.0f, t / 0.01f) * powf(1 - t / 0.8f, 1.5f);
  }
  set(Sfx::Virus, 0.14f);
  // 弾をはじいた: キン
  w = newWave(0.14f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * (2600 - 1200 * t)) + sinf(t * PI2 * 3900) * 0.5f) * expf(-t * 30) * 0.6f;
  }
  set(Sfx::Ping, 0.1f);
  // 防犯カメラに見つかった: ピピッ
  w = newWave(0.32f);
  for (int b = 0; b < 2; b++)
  {
    int st = (int)(SR * b * 0.14f);
    for (int i = 0; st + i < tmpN && i < SR * 0.09f; i++)
    {
      float t = (float)i / SR;
      w[st + i] = (square(t * 1320) * 0.4f + sinf(t * PI2 * 1320) * 0.6f) * min(1.0f, t / 0.003f) * min(1.0f, (0.09f - t) / 0.01f);
    }
  }
  set(Sfx::Alarm, 0.13f);
  // 蒸気: シュー (高い雑音が吹き出して弱まる)
  w = newWave(0.5f);
  float slp2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, n = noise();
    slp2 += (n - slp2) * 0.5f;
    w[i] = (n - slp2) * min(1.0f, t / 0.02f) * powf(1 - t / 0.5f, 1.5f);
  }
  set(Sfx::Steam, 0.12f);
  // 戦車の足が着く: 小さなカシャ (サーボのカチッと、低いトン)
  w = newWave(0.08f);
  float flp2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    flp2 += (noise() - flp2) * 0.3f;
    w[i] = (flp2 * 1.2f * expf(-t * 70) + sinf(t * PI2 * 170) * expf(-t * 45) * 0.6f + sinf(t * PI2 * 3100) * expf(-t * 160) * 0.3f);
  }
  set(Sfx::Step, 0.07f);
  // フックやプレスが落ちる: ズシンと、鉄の響き
  w = newWave(0.7f);
  float mlp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    mlp += (noise() - mlp) * 0.06f;
    float body = tanhf(sinf(t * PI2 * (80 - 50 * t)) * 2.2f) * expf(-t * 7);
    float ring = (sinf(t * PI2 * 220) + sinf(t * PI2 * 347) * 0.7f) * expf(-t * 6) * 0.3f;
    w[i] = body * 0.8f + mlp * 4 * expf(-t * 9) + ring;
  }
  set(Sfx::Slam, 0.42f);
  // 出発: 低めの、少し暗い工場らしい節
  melody({-12, -5, 0, 3, 2, -2, 0}, {0.12f, 0.12f, 0.12f, 0.24f, 0.12f, 0.12f, 0.5f}, 0.25f);
  set(Sfx::G2Start, 0.3f);
  // 背景: 遠くのくい打ち。重い打撃 (歪ませた低い音) と鉄の響きをこもらせて、こだまを 2 つ
  w = newWave(1.1f);
  float klp = 0, klp2 = 0;
  for (int i = 0; i < (int)(SR * 0.6f); i++)
  {
    breathe(i);
    float t = (float)i / SR;
    float thud = tanhf(sinf(t * PI2 * (95 - 45 * min(1.0f, t * 8))) * 2.5f) * expf(-t * 9);
    float clang = (sinf(t * PI2 * 183) + sinf(t * PI2 * 412) * 0.8f + sinf(t * PI2 * 655) * 0.6f + sinf(t * PI2 * 1021) * 0.3f) * expf(-t * 7);
    float hit = noise() * expf(-t * 60);
    float v = thud * 0.9f + clang * 0.35f + hit * 0.5f;
    klp += (v - klp) * 0.35f; // 遠いのでこもらせる
    klp2 += (klp - klp2) * 0.5f;
    w[i] = klp2 * min(1.0f, t / 0.002f);
  }
  for (int i = tmpN - 1; i >= 0; i--)
  {
    int e1 = (int)(SR * 0.19f), e2 = (int)(SR * 0.41f);
    if (i >= e1)
      w[i] += w[i - e1] * 0.32f;
    if (i >= e2)
      w[i] += w[i - e2] * 0.15f;
  }
  set(Sfx::PileDrive, 0.3f);
  // 背景: 遠くの大きな機械。ガシャンと噛み合う音のあとに、油圧が抜けるシュー
  w = newWave(1.0f);
  float hlp = 0, hlp2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, v = 0;
    for (int k = 0; k < 2; k++) // ガシャ、ン (2 つに分かれて当たる)
    {
      float u = t - k * 0.07f;
      if (u >= 0)
        v += ((sinf(u * PI2 * 240) + sinf(u * PI2 * 377) * 0.8f + sinf(u * PI2 * 590) * 0.5f) * 0.4f + noise() * 0.5f) * expf(-u * 22) *
             (k ? 1.0f : 0.6f);
    }
    if (t > 0.18f) // シュー
    {
      float u = t - 0.18f, n = noise();
      hlp += (n - hlp) * 0.4f;
      v += (n - hlp) * 0.35f * min(1.0f, u / 0.05f) * powf(max(0.0f, 1 - u / 0.8f), 1.5f);
    }
    hlp2 += (v - hlp2) * 0.45f; // 遠いのでこもらせる
    w[i] = hlp2;
  }
  set(Sfx::MachClank, 0.2f);

  // ---- 3 本目 (LONGSHOT) ----
  // 狙撃銃: 鋭い破裂 (すぐ消える雑音)、腹に響くドン (歪ませた低い音)、こもっていく轟き。ビルに当たって返るこだまを 3 つ
  w = newWave(1.9f);
  float rlp = 0, rlp2 = 0, rhp = 0;
  const int RIFLE_N = (int)(SR * 0.7f);
  for (int i = 0; i < RIFLE_N; i++)
  {
    breathe(i);
    float t = (float)i / SR, n = noise();
    rhp += (n - rhp) * 0.7f;
    float crack = (n - rhp) * expf(-t * 140) * 1.6f;
    float thump = tanhf(sinf(t * PI2 * (120 - 70 * min(1.0f, t * 6))) * 3) * expf(-t * 16);
    float cut = 0.3f * expf(-t * 7) + 0.02f;
    rlp += (n - rlp) * cut;
    rlp2 += (rlp - rlp2) * cut;
    w[i] = (crack + thump * 0.9f + rlp2 * 3.2f * expf(-t * 4)) * min(1.0f, t / 0.0005f);
  }
  {
    const float ED[3] = {0.21f, 0.47f, 0.83f}, EG[3] = {0.34f, 0.2f, 0.11f};
    for (int e = 0; e < 3; e++) // こだま: 元の音 (最初の 0.7 秒) だけをこもらせて遅らせて足す
    {
      int d = (int)(SR * ED[e]);
      float elp = 0;
      for (int j = 0; j < RIFLE_N && j + d < tmpN; j++)
      {
        breathe(j);
        elp += (w[j] - elp) * 0.25f;
        w[j + d] += elp * EG[e];
      }
    }
  }
  set(Sfx::Rifle, 0.55f);
  // ボルトを引いて押しこむ: カチャ (0 秒) と カチッ (0.22 秒)、あいだに金属がこすれる音
  w = newWave(0.5f);
  float blp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, v = 0, n = noise();
    for (int k = 0; k < 2; k++)
    {
      float u = t - (k ? 0.22f : 0.0f);
      if (u >= 0)
        v += (n * expf(-u * 220) * 0.8f + sinf(u * PI2 * (k ? 2700 : 2100)) * expf(-u * 90) * 0.5f +
              sinf(u * PI2 * (k ? 1150 : 860)) * expf(-u * 60) * 0.4f) *
             (k ? 1.0f : 0.8f);
    }
    if (t > 0.04f && t < 0.2f) // こすれる
    {
      blp += (n - blp) * 0.5f;
      v += (n - blp) * 0.25f * sinf((t - 0.04f) / 0.16f * 3.1416f);
    }
    w[i] = v;
  }
  set(Sfx::Bolt, 0.16f);
  // 弾が遠くに当たる: こもったトッ
  w = newWave(0.25f);
  float ilp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    ilp += (noise() - ilp) * 0.15f;
    w[i] = (ilp * 3 * expf(-t * 30) + sinf(t * PI2 * 170) * expf(-t * 35) * 0.6f) * min(1.0f, t / 0.001f);
  }
  set(Sfx::Impact, 0.1f);
  // 窓ガラス: パリンと割れて、かけらがチリチリ落ちる
  w = newWave(1.0f);
  for (int i = 0; i < tmpN; i++)
    w[i] = noise() * expf(-(float)i / SR * 70) * 0.8f;
  for (int k = 0; k < 22; k++)
  {
    vTaskDelay(1);
    int st = (int)(SR * (k < 6 ? rnd01() * 0.05f : 0.05f + rnd01() * 0.7f));
    float f = 2800 + rnd01() * 4200, a = k < 6 ? 0.5f : 0.25f * (1 - st / (SR * 0.8f));
    for (int i = 0; st + i < tmpN && i < SR * 0.12f; i++)
    {
      float u = (float)i / SR;
      w[st + i] += sinf(u * PI2 * f) * expf(-u * 45) * a;
    }
  }
  set(Sfx::Glass, 0.14f);
  // 車に当たる: カン (鉄板の響き)
  w = newWave(0.45f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * 820) + sinf(t * PI2 * 1310) * 0.7f + sinf(t * PI2 * 2150) * 0.4f) * expf(-t * 12) * 0.4f + noise() * expf(-t * 150) * 0.6f;
  }
  set(Sfx::Clank, 0.12f);
  // 鳩が飛び立つ: 羽ばたきのバタバタ (だんだん速く、遠く)
  w = newWave(0.9f);
  float flp3 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, n = noise();
    float rate = 11 + t * 6, ph = t * rate;
    float env = powf(max(0.0f, sinf((ph - floorf(ph)) * 3.1416f)), 4.0f);
    flp3 += (n - flp3) * 0.35f;
    w[i] = flp3 * env * 1.6f * (1 - t / 0.9f);
  }
  set(Sfx::Flutter, 0.12f);
  // 心臓: ドクン (低い 2 つの打音。小さなスピーカーでも聞こえるように歪ませる)
  w = newWave(0.4f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, v = 0;
    for (int k = 0; k < 2; k++)
    {
      float u = t - k * 0.15f;
      if (u >= 0)
        v += tanhf(sinf(u * PI2 * (k ? 58 : 66)) * 3) * expf(-u * (k ? 22 : 18)) * (k ? 0.7f : 1.0f);
    }
    w[i] = v;
  }
  set(Sfx::Heart, 0.3f);
  // 息を吐く: はぁ (やわらかい雑音がふくらんで消える)
  w = newWave(0.7f);
  float elp1 = 0, elp2 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR, n = noise();
    elp1 += (n - elp1) * 0.3f;
    elp2 += (elp1 - elp2) * 0.08f;
    float env = sinf(min(1.0f, t / 0.7f) * 3.1416f);
    w[i] = (elp1 - elp2) * env * env;
  }
  set(Sfx::Exhale, 0.1f);
  // スコープ: 小さなカチ
  w = newWave(0.08f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = noise() * expf(-t * 300) * 0.6f + sinf(t * PI2 * 3200) * expf(-t * 120) * 0.4f;
  }
  set(Sfx::ScopeIn, 0.07f);
  // 指令: 低く張りつめた節。しとめた: 明るく上がる節。失敗: 下がる節
  melody({-17, -10, -9, -10, -14, -9}, {0.14f, 0.14f, 0.28f, 0.14f, 0.14f, 0.5f}, 0.2f);
  set(Sfx::Mission, 0.26f);
  melody({0, 4, 7, 12, 7, 12}, {0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.45f}, 0.3f);
  set(Sfx::Clear, 0.28f);
  melody({-5, -8, -11, -15}, {0.18f, 0.18f, 0.18f, 0.6f}, 0.35f);
  set(Sfx::Fail, 0.26f);
  // クラクション: 2 つの音の和音
  w = newWave(0.55f);
  float hlp3 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    float v = t < 0.5f ? square(t * 410) + square(t * 512) * 0.8f : 0;
    hlp3 += (v - hlp3) * 0.2f;
    w[i] = hlp3 * min(1.0f, t / 0.01f) * 0.5f;
  }
  set(Sfx::Horn, 0.08f);
  // ざわめき: 何人もの声 (高さが揺れるのこぎり波をこもらせる) が驚いて上ずり、遠のく
  w = newWave(1.6f);
  for (int k = 0; k < 7; k++)
  {
    float ph = 0, lp = 0, f0 = 200 + rnd01() * 260, st = rnd01() * 0.25f, len = 0.5f + rnd01() * 0.8f, vib = 4 + rnd01() * 4;
    for (int i = (int)(SR * st); i < tmpN && i < (int)(SR * (st + len)); i++)
    {
      breathe(i);
      float u = (float)i / SR - st, kk = u / len;
      float f = f0 * (1 + 0.35f * sinf(min(1.0f, kk * 2) * 3.1416f)) * (1 + 0.03f * sinf(u * PI2 * vib));
      ph += f / SR;
      float saw = (ph - floorf(ph)) * 2 - 1;
      lp += (saw - lp) * 0.18f;
      w[i] += lp * sinf(kk * 3.1416f) * 0.35f;
    }
  }
  {
    float clp = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      clp += (noise() - clp) * 0.1f;
      w[i] += clp * 0.5f * sinf(min(1.0f, (float)i / tmpN) * 3.1416f);
    }
  }
  set(Sfx::Crowd, 0.14f);

  // ---- 4 本目 (CORNERS) ----
  // キューで突く: コッ (木の短い響き)
  w = newWave(0.12f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * 950) * expf(-t * 70) + sinf(t * PI2 * 2100) * 0.45f * expf(-t * 120) + noise() * expf(-t * 500) * 0.5f) *
           min(1.0f, t / 0.0004f);
  }
  set(Sfx::CueHit, 0.3f);
  // 玉どうし: カチッ (硬く高い響き)
  w = newWave(0.12f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = sinf(t * PI2 * 3300) * expf(-t * 95) + sinf(t * PI2 * 5100) * 0.5f * expf(-t * 140) + sinf(t * PI2 * 2200) * 0.3f * expf(-t * 70) +
           noise() * expf(-t * 900) * 0.6f;
  }
  set(Sfx::BallClick, 0.3f);
  // クッション: トッ (こもった低い音)
  w = newWave(0.15f);
  float clp4 = 0;
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    clp4 += (noise() - clp4) * 0.12f;
    w[i] = clp4 * 3 * expf(-t * 45) + sinf(t * PI2 * 180) * expf(-t * 40) * 0.6f;
  }
  set(Sfx::Cushion, 0.22f);
  // ポケットに落ちる: ゴトン (歪ませた低い音と木の響き) のあと、台の中を転がるゴロゴロ
  w = newWave(1.0f);
  float pklp = 0;
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    float thump = tanhf(sinf(t * PI2 * (150 - 50 * min(1.0f, t * 8))) * 2.5f) * expf(-t * 22);
    float knock = sinf(t * PI2 * 620) * expf(-t * 60) * 0.5f;
    w[i] = (thump + knock) * min(1.0f, t / 0.001f);
  }
  {
    float st = 0.14f;
    for (int k = 0; st < 0.9f; k++) // 転がる: こもったコトコトが、だんだん間をあけて遠のく
    {
      vTaskDelay(1);
      int s0 = (int)(SR * st);
      float a = 0.35f * (1 - st / 0.95f), f = 500 + rnd01() * 300;
      for (int i = 0; s0 + i < tmpN && i < SR * 0.05f; i++)
      {
        float u = (float)i / SR;
        w[s0 + i] += (sinf(u * PI2 * f) * expf(-u * 80) + noise() * expf(-u * 300) * 0.3f) * a;
      }
      st += 0.035f + k * 0.006f + rnd01() * 0.02f;
    }
    for (int i = 0; i < tmpN; i++)
    {
      pklp += (w[i] - pklp) * 0.5f;
      w[i] = pklp;
    }
  }
  set(Sfx::Pocket, 0.3f);
  // 玉を並べる: ガラガラ (たくさんのカチ)
  w = newWave(0.7f);
  for (int k = 0; k < 26; k++)
  {
    vTaskDelay(1);
    int s0 = (int)(SR * rnd01() * 0.55f);
    float f = 2500 + rnd01() * 2500, a = 0.3f + rnd01() * 0.4f;
    for (int i = 0; s0 + i < tmpN && i < SR * 0.05f; i++)
    {
      float u = (float)i / SR;
      w[s0 + i] += (sinf(u * PI2 * f) * expf(-u * 120) + noise() * expf(-u * 800) * 0.4f) * a;
    }
  }
  set(Sfx::Rack, 0.18f);

  // ---- 5 本目 (ORBIT) ----
  // フリッパー: ソレノイドのカシャッ (短い雑音、低いドッ、金属のカチ)
  w = newWave(0.12f);
  {
    float flp = 0;
    for (int i = 0; i < tmpN; i++)
    {
      float t = (float)i / SR;
      flp += (noise() - flp) * 0.35f;
      w[i] = flp * 2.2f * expf(-t * 90) + tanhf(sinf(t * PI2 * 95) * 2) * expf(-t * 45) * 0.7f + sinf(t * PI2 * 2600) * expf(-t * 160) * 0.3f;
    }
  }
  set(Sfx::Flipper, 0.22f);
  // バンパー: ポン (下がる低い音を歪ませ、高い響きを足す)
  w = newWave(0.28f);
  {
    float ph = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      ph += (90 + 110 * expf(-t * 30)) / SR;
      w[i] = tanhf(sinf(ph * PI2) * 3) * expf(-t * 16) + sinf(t * PI2 * 880) * expf(-t * 25) * 0.35f + noise() * expf(-t * 400) * 0.5f;
    }
  }
  set(Sfx::Bumper, 0.26f);
  // スリングショット: パシッ
  w = newWave(0.12f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = noise() * expf(-t * 120) * 0.8f + sinf(t * PI2 * 420) * expf(-t * 50) * 0.6f;
  }
  set(Sfx::Sling, 0.2f);
  // レーン: ピン (鈴)
  w = newWave(0.35f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (sinf(t * PI2 * 1760) + sinf(t * PI2 * 2640) * 0.4f + sinf(t * PI2 * 4400) * 0.15f) * expf(-t * 12) * min(1.0f, t / 0.002f);
  }
  set(Sfx::Rollover, 0.14f);
  // ターゲット: コトッ
  w = newWave(0.12f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = sinf(t * PI2 * 320) * expf(-t * 55) + sinf(t * PI2 * 1150) * expf(-t * 90) * 0.4f + noise() * expf(-t * 600) * 0.5f;
  }
  set(Sfx::DropT, 0.26f);
  // ブラックホール: ズゥン (沈んでいく音と、こもった雑音)
  w = newWave(0.8f);
  {
    float ph = 0, lp = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR, k = t / 0.8f;
      ph += (380 * powf(0.15f, k) + 50) / SR;
      lp += (noise() - lp) * 0.05f;
      w[i] = (tanhf(sinf(ph * PI2) * 2) * 0.8f + lp * 2) * min(1.0f, t / 0.01f) * powf(1 - k, 1.5f);
    }
  }
  set(Sfx::Saucer, 0.26f);
  // 打ち出す: バネのシュッとビョン
  w = newWave(0.3f);
  {
    float lp = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      lp += (noise() - lp) * 0.25f;
      w[i] = lp * 1.6f * expf(-t * 30) + sinf(t * PI2 * (160 + 40 * sinf(t * PI2 * 30))) * expf(-t * 14) * 0.5f;
    }
  }
  set(Sfx::Launch, 0.24f);
  // ボールが落ちた: 高いところから下がる音
  w = newWave(0.8f);
  {
    float ph = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR, k = t / 0.8f;
      ph += 880 * powf(0.125f, k) / SR;
      float fr = ph - floorf(ph);
      w[i] = ((fr < 0.5f ? 1.0f : -1.0f) * 0.3f + (1 - 4 * fabsf(fr - 0.5f)) * 0.7f) * (1 - k) * min(1.0f, t / 0.01f);
    }
  }
  set(Sfx::Drain, 0.2f);
  // TILT: ブー
  w = newWave(0.7f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = (square(t * 110) * 0.6f + square(t * 113) * 0.4f) * min(1.0f, t / 0.01f) * (t < 0.6f ? 1 : (0.7f - t) * 10);
  }
  set(Sfx::Tilt, 0.16f);
  melody({3, 7, 10, 15, 19, 22, 27}, {0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.35f}, 0.25f);
  set(Sfx::Jackpot, 0.24f);
  melody({-2, 3, 7, 10, 7, 15}, {0.1f, 0.1f, 0.1f, 0.15f, 0.1f, 0.45f}, 0.3f);
  set(Sfx::PinStart, 0.24f);

  // ---- 6 本目 (JUKEBOX) ----
  // チャイム: 古い台の、金属の棒を打つ音 (基音と、ずれた倍音)
  {
    const float CF[3] = {1046.5f, 784.0f, 523.3f};
    const Sfx CS[3] = {Sfx::Chime10, Sfx::Chime100, Sfx::Chime1000};
    for (int k = 0; k < 3; k++)
    {
      w = newWave(0.9f);
      for (int i = 0; i < tmpN; i++)
      {
        breathe(i);
        float t = (float)i / SR, f = CF[k];
        w[i] = (sinf(t * PI2 * f) * expf(-t * 4) + sinf(t * PI2 * f * 2.76f) * 0.35f * expf(-t * 9) + sinf(t * PI2 * f * 5.4f) * 0.15f * expf(-t * 18) +
                noise() * expf(-t * 900) * 0.3f) *
               min(1.0f, t / 0.001f);
      }
      set(CS[k], 0.13f);
    }
  }
  // ノッカー: ゴン (箱に響く低い音)
  w = newWave(0.35f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = tanhf(sinf(t * PI2 * (75 + 60 * expf(-t * 40))) * 4) * expf(-t * 14) + noise() * expf(-t * 250) * 0.8f +
           sinf(t * PI2 * 310) * expf(-t * 30) * 0.3f;
  }
  set(Sfx::Knocker, 0.4f);
  // キックアウト: ガコン
  w = newWave(0.3f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    w[i] = tanhf(sinf(t * PI2 * 120) * 3) * expf(-t * 25) + sinf(t * PI2 * 1350) * expf(-t * 35) * 0.35f + noise() * expf(-t * 300) * 0.6f;
  }
  set(Sfx::Kickout, 0.26f);

  // ---- 7 本目 (SMARTBALL) ----
  // 釘: 小さな金属のチッ
  w = newWave(0.05f);
  for (int i = 0; i < tmpN; i++)
  {
    float t = (float)i / SR;
    w[i] = sinf(t * PI2 * 3900) * expf(-t * 140) + sinf(t * PI2 * 6100) * 0.5f * expf(-t * 200) + noise() * expf(-t * 1200) * 0.4f;
  }
  set(Sfx::Nail, 0.07f);
  // 穴に入る: コトン (木の箱に落ちる) と、小さなカラカラ
  w = newWave(0.4f);
  for (int i = 0; i < tmpN; i++)
  {
    breathe(i);
    float t = (float)i / SR;
    float v = sinf(t * PI2 * 380) * expf(-t * 30) + sinf(t * PI2 * 900) * expf(-t * 60) * 0.4f + noise() * expf(-t * 400) * 0.5f;
    float u = t - 0.09f;
    if (u > 0)
      v += (sinf(u * PI2 * 520) * expf(-u * 45) + noise() * expf(-u * 500) * 0.3f) * 0.5f;
    w[i] = v;
  }
  set(Sfx::HoleIn, 0.24f);
  // 払い出し: 玉がたくさん流れ出るジャラジャラ
  w = newWave(0.9f);
  for (int k = 0; k < 40; k++)
  {
    vTaskDelay(1);
    float st = rnd01() * 0.7f;
    int s0 = (int)(SR * st);
    float f = 1700 + rnd01() * 1600, a = (0.25f + rnd01() * 0.35f) * (1 - st / 0.9f);
    for (int i = 0; s0 + i < tmpN && i < SR * 0.04f; i++)
    {
      float u = (float)i / SR;
      w[s0 + i] += (sinf(u * PI2 * f) * expf(-u * 130) + noise() * expf(-u * 700) * 0.4f) * a;
    }
  }
  set(Sfx::Payout, 0.2f);
  // 玉が転がってくる: こもったゴロゴロ
  w = newWave(0.45f);
  {
    float lp = 0;
    for (int i = 0; i < tmpN; i++)
    {
      breathe(i);
      float t = (float)i / SR;
      lp += (noise() - lp) * 0.06f;
      w[i] = lp * 3 * (0.6f + 0.4f * sinf(t * PI2 * 28)) * sinf(min(1.0f, t / 0.45f) * 3.1416f);
    }
  }
  set(Sfx::Feed, 0.16f);
  // 妖怪に当たる: 音程が下がるぽよん (少しふるえる)
  w = newWave(0.25f);
  {
    float ph = 0;
    for (int i = 0; i < tmpN; i++)
    {
      float t = (float)i / SR;
      float f = (260 + 640 * expf(-t * 9)) * (1 + 0.04f * sinf(t * PI2 * 24));
      ph += PI2 * f / SR;
      w[i] = (sinf(ph) + 0.25f * sinf(ph * 2)) * expf(-t * 12) * min(1.0f, t * 400);
    }
  }
  set(Sfx::Yokai, 0.2f);
  free(tmp);
  tmp = nullptr;
}

// ---- 鳴りつづける音 ----
// sinf は 1 回に 2〜3 us かかり、標本ごとに何回も呼ぶとコア 0 (液晶へ送る仕事も持っている) が埋まるので、
// 鳴りつづける音のサインは表を引く (1024 分割、間はまっすぐつなぐ)。引数は「周」(1 周 = 0〜1)
float SINT[1025];
void initSin()
{
  for (int i = 0; i <= 1024; i++)
    SINT[i] = sinf(i * PI2 / 1024);
}
inline float tsin(float cyc) // cyc は 0 以上 (floorf は遅いので、整数にして引く)
{
  cyc -= (int)cyc;
  float f = cyc * 1024;
  int i = (int)f;
  return SINT[i] + (SINT[i + 1] - SINT[i]) * (f - i);
}
float engPh, engLfo, ufoPh, ufoLfo, beamPh, momPh;
float rainLp, rainHp, rainLp2, engOut, engOut2;
float bird, birdPh, birdF, birdDf;
int birdLeft, birdGap = SR;
float curEngine, curRain, curUfo, curBeam, curMom, curBirds, curPitch = 1, curBank;
float curHum, curRoar, humPh, humPh2, humLp, chugPh, chugLp, roarLp, roarLp2, roarAm = 1, roarAmS = 1;
float curCity, curWind, cityLp, cityLp2, swellPh, windLp, windLp2, gustPh;
int roarN;

inline float wrap(float ph) { return ph >= 1 ? ph - (int)ph : ph; } // 位相は 0〜1 で持つ (float の桁が足りなくならないように。1 回に進むのは 1 未満)
float toward(float cur, float target) { return cur + (target - cur) * 0.08f; }

// 小鳥 (ヒバリのつもり): 間をおいて、高い声が速く上下するさえずりを鳴らす
float birdSample()
{
  if (birdLeft <= 0)
  {
    if (--birdGap > 0)
      return 0;
    birdLeft = (int)(SR * (0.04f + rnd01() * 0.09f));
    birdF = 2600 + rnd01() * 2400;
    birdDf = (rnd01() - 0.5f) * 120000.0f / SR;
    bird = 0;
    birdGap = (rng >> 7) % 4 == 0 ? (int)(SR * (0.6f + rnd01() * 1.8f)) : (int)(SR * (0.02f + rnd01() * 0.06f));
  }
  birdLeft--;
  birdF = max(1800.0f, min(6500.0f, birdF + birdDf));
  birdPh = wrap(birdPh + birdF / SR);
  bird = min(1.0f, bird + 0.01f);
  float env = min(bird, min(1.0f, birdLeft / (SR * 0.01f)));
  return tsin(birdPh) * env;
}

void loopSounds(float *mix, bool mute)
{
  Sound &s = *self;
  // 1 枠ごとに少しずつ目標の音量へ寄せる (急に切れてプツッと鳴らないように)
  curEngine = toward(curEngine, mute ? 0 : s.Engine);
  curPitch += (s.EnginePitch - curPitch) * 0.2f;
  curBank += (s.EngineBank - curBank) * 0.2f;
  curRain = toward(curRain, mute ? 0 : s.Rain);
  curUfo = toward(curUfo, mute ? 0 : s.UfoHum);
  curBeam = toward(curBeam, mute ? 0 : s.Beam);
  curMom = toward(curMom, mute ? 0 : s.MomHum);
  curBirds = toward(curBirds, mute ? 0 : s.Birds);
  curHum = toward(curHum, mute ? 0 : s.Hum);
  curRoar = toward(curRoar, mute ? 0 : s.Roar);
  curCity = toward(curCity, mute ? 0 : s.City);
  curWind = toward(curWind, mute ? 0 : s.WindS);
  for (int i = 0; i < BUF; i++)
  {
    float v = 0;
    if (curEngine > 0.001f)
    {
      // プロペラ: 低いうなりに、羽根が空気をたたく脈を重ねる。音程は操作で変わり、旋回中はうなりが少し揺れる
      engLfo = wrap(engLfo + 7.0f / SR);
      float f = 62 * curPitch * (1 + 0.012f * curBank * tsin(engLfo));
      engPh = wrap(engPh + f / SR);
      float saw = engPh * 2 - 1;
      float sn = max(0.0f, tsin(engPh * 2)), puls = sn * sn * sn;
      puls *= puls;
      engOut += (saw * 0.5f + puls - engOut) * 0.25f;
      engOut2 += (engOut - engOut2) * 0.12f; // 小さなスピーカーでは高い倍音がジーと耳につくので、もう 1 段こもらせる (約 400Hz から上を弱める)
      v += engOut2 * 0.35f * curEngine * (0.85f + 0.3f * min(1.5f, curPitch - 0.5f));
    }
    if (curRain > 0.001f)
    {
      rainLp += (noise() - rainLp) * 0.35f;
      rainHp = rainLp - rainLp2;
      rainLp2 += (rainLp - rainLp2) * 0.02f;
      v += rainHp * 0.3f * curRain; // 筐体に入れたらケースで響いてうるさかったので下げた (0.55 → 0.3)
    }
    if (curUfo > 0.001f || curBeam > 0.001f)
      ufoLfo = wrap(ufoLfo + 6.5f / SR);
    if (curUfo > 0.001f)
    {
      // 円盤: ゆらゆら揺れる高い音
      ufoPh = wrap(ufoPh + (420 + 70 * tsin(ufoLfo)) / SR);
      v += tsin(ufoPh) * 0.22f * curUfo;
    }
    if (curBeam > 0.001f)
    {
      beamPh = wrap(beamPh + (880 + 40 * tsin(ufoLfo * 3)) / SR);
      v += (tsin(beamPh) + 0.5f * tsin(beamPh * 3)) * 0.12f * curBeam;
    }
    if (curMom > 0.001f)
    {
      // 母船: 低くうねる音 (うねりは 24 周に 1 回)
      momPh += 48.0f / SR;
      if (momPh >= 24)
        momPh -= 24;
      float env = 0.6f + 0.4f * tsin(momPh / 24);
      v += (tsin(momPh) + 0.4f * tsin(momPh * 4.02f)) * env * 0.35f * curMom;
    }
    if (curBirds > 0.001f)
      v += birdSample() * 0.09f * curBirds; // 筐体で響くので半分に (0.18 → 0.09)
    if (curHum > 0.001f)
    {
      // 工場のうなり: 少しずれた 2 つの低い音のうねりと、こもった空気の音
      humPh = wrap(humPh + 110.0f / SR);
      humPh2 = wrap(humPh2 + 110.6f / SR);
      humLp += (noise() - humLp) * 0.02f;
      // 大きな機関のドッドッドッ (1 秒に 3 回ほど、こもった雑音が脈打つ)
      chugPh = wrap(chugPh + 3.1f / SR);
      float pulse = 1 - chugPh * 3;
      pulse = pulse > 0 ? pulse * pulse : 0;
      chugLp += (noise() - chugLp) * 0.06f;
      v += ((tsin(humPh) + tsin(humPh2)) * 0.08f + humLp * 0.5f + chugLp * pulse * 1.6f) * curHum;
    }
    if (curRoar > 0.001f)
    {
      // 溶鉱炉: 低い雑音が、炎のようにゆらぐ
      if (++roarN >= SR / 12)
      {
        roarN = 0;
        roarAm = 0.6f + rnd01() * 0.4f;
      }
      roarLp += (noise() - roarLp) * 0.05f;
      roarLp2 += (roarLp - roarLp2) * 0.1f;
      roarAmS += (roarAm - roarAmS) * 0.002f; // 急に変えるとプツッと鳴るので、なめらかに寄せる
      v += roarLp2 * 2.2f * roarAmS * curRoar;
    }
    if (curCity > 0.001f)
    {
      // 街: 遠くの車の流れの低いざわめきが、ゆっくりふくらんだりしぼんだりする
      swellPh = wrap(swellPh + 0.09f / SR);
      cityLp += (noise() - cityLp) * 0.03f;
      cityLp2 += (cityLp - cityLp2) * 0.2f;
      v += cityLp2 * 1.3f * (0.65f + 0.35f * tsin(swellPh)) * curCity;
    }
    if (curWind > 0.001f)
    {
      // 風: 高めの雑音が、突風のたびに強まる
      gustPh = wrap(gustPh + 0.23f / SR);
      windLp += (noise() - windLp) * 0.12f;
      windLp2 += (windLp - windLp2) * 0.02f;
      float g = 0.55f + 0.45f * tsin(gustPh);
      v += (windLp - windLp2) * 0.45f * g * g * curWind;
    }
    mix[i] += v;
  }
}

// ---- 作った音をフラッシュ (LittleFS) に覚えておく ----
// 作るのに 50 秒ほどかかるので、一度作ったら /sfx.bin に保存し、次からは読むだけにする (1.4MB ほど)。
// 見出しに版の番号 (CACHE_KEY) を入れておき、音の作り方を変えて番号を上げたら作りなおす
const char *const CACHE = "/sfx.bin";
const char CACHE_KEY[40] = "SKYLARK-SFX v9"; // 鳴らしきりの音の作り方を変えたら、番号を上げる (作りなおす)

bool loadCache()
{
  File f = LittleFS.open(CACHE, "r");
  if (!f)
    return false;
  char key[40];
  int32_t count = 0;
  bool ok = f.read((uint8_t *)key, 40) == 40 && memcmp(key, CACHE_KEY, 40) == 0 && f.read((uint8_t *)&count, 4) == 4 &&
            count == (int)Sfx::Count;
  for (int i = 0; ok && i < count; i++)
  {
    int32_t n;
    float vol;
    ok = f.read((uint8_t *)&n, 4) == 4 && f.read((uint8_t *)&vol, 4) == 4 && n > 0 && n < SR * 5;
    int16_t *d = ok ? (int16_t *)ps_malloc(n * 2) : nullptr;
    ok = d && f.read((uint8_t *)d, n * 2) == (size_t)n * 2;
    if (!ok)
    {
      free(d);
      break;
    }
    waves[i].n = n;
    waves[i].vol = vol;
    __sync_synchronize();
    waves[i].w = d;
  }
  f.close();
  return ok;
}

void saveCache()
{
  File f = LittleFS.open(CACHE, "w");
  if (!f)
    return;
  int32_t count = (int)Sfx::Count;
  bool ok = f.write((const uint8_t *)CACHE_KEY, 40) == 40 && f.write((const uint8_t *)&count, 4) == 4;
  for (int i = 0; ok && i < count; i++)
  {
    int32_t n = waves[i].n;
    ok = waves[i].w && f.write((const uint8_t *)&n, 4) == 4 && f.write((const uint8_t *)&waves[i].vol, 4) == 4 &&
         f.write((const uint8_t *)waves[i].w, n * 2) == (size_t)n * 2;
  }
  f.close();
  if (!ok)
    LittleFS.remove(CACHE); // 途中で失敗したら残さない (次に作りなおす)
}

void audioTask(void *)
{
  initSin();
  uint32_t t0 = millis();
  bool fs = LittleFS.begin(true); // はじめて使うときは領域を整える
  if (fs && loadCache())
    Serial.printf("sound waves loaded %lu ms\n", millis() - t0);
  else
  {
    for (Wave &w : waves) // 読みかけの音は捨てて、作りなおす
    {
      int16_t *d = w.w;
      w.w = nullptr;
      free(d);
    }
    vTaskPrioritySet(nullptr, 1); // 作っているあいだは、液晶へ送るタスク (優先度 2) を先にする
    makeWaves();                  // 鳴らしきりの音を作る (50 秒ほどかかる。できた音から鳴らせる)
    Serial.printf("sound waves made %lu ms\n", millis() - t0);
    if (fs)
    {
      uint32_t t1 = millis();
      saveCache();
      Serial.printf("sound waves saved %lu ms\n", millis() - t1);
    }
    vTaskPrioritySet(nullptr, 3);
  }
  static float mix[BUF];
  static int16_t out[BUF * 2];
  for (;;)
  {
    uint32_t m0 = micros();
    memset(mix, 0, sizeof(mix));
    bool mute = self->Muted;
    portENTER_CRITICAL(&mux);
    Voice vs[VOICES];
    memcpy(vs, voices, sizeof(vs));
    portEXIT_CRITICAL(&mux);
    for (int k = 0; k < VOICES; k++)
    {
      Voice &v = vs[k];
      if (!v.on)
        continue;
      int n = min(BUF, v.w->n - v.pos);
      if (!mute)
      {
        float g = v.vol / 32767.0f;
        const int16_t *src = v.w->w + v.pos;
        for (int i = 0; i < n; i++)
          mix[i] += src[i] * g;
      }
      v.pos += n;
    }
    portENTER_CRITICAL(&mux);
    for (int k = 0; k < VOICES; k++) // 鳴らしているあいだに Play が足した声はそのまま
      if (voices[k].on && voices[k].w == vs[k].w && vs[k].on)
      {
        voices[k].pos = vs[k].pos;
        if (vs[k].pos >= vs[k].w->n)
          voices[k].on = false;
      }
    portEXIT_CRITICAL(&mux);
    uint32_t m1 = micros();
    loopSounds(mix, mute);
    self->usLoop += micros() - m1;
    float m = mute ? 0 : self->Master;
    for (int i = 0; i < BUF; i++)
    {
      float x = mix[i] * m;
      x = x / (1 + fabsf(x) * 0.6f); // やわらかく頭を抑える
      int16_t s = (int16_t)(max(-1.0f, min(1.0f, x)) * 30000);
      out[i * 2] = out[i * 2 + 1] = s;
    }
    self->usMix += micros() - m0;
    size_t written;
    i2s_write(I2S_NUM_0, out, sizeof(out), &written, portMAX_DELAY); // DMA が空くまで待つので、これで速さが合う
  }
}
} // namespace

void Sound::begin()
{
  self = this;
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SR;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = BUF;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE; // 0 のままだと GPIO0 (BOOT ボタン) に MCLK が出てしまう
  pins.bck_io_num = AMP_BCLK;
  pins.ws_io_num = AMP_LRC;
  pins.data_out_num = AMP_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK || i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK)
  {
    Serial.println("i2s init failed!");
    return;
  }
  xTaskCreatePinnedToCore(audioTask, "audio", 8192, nullptr, 3, nullptr, 0);
}

void Sound::Play(Sfx s, float vol)
{
  if (Muted || !self)
    return;
  const Wave *w = &waves[(int)s];
  if (!w->w)
    return;
  portENTER_CRITICAL(&mux);
  // 同じ音を同時にいくつも重ねない (機銃や円盤の弾が鳴りすぎないように)
  int same = 0, freeK = -1;
  for (int k = 0; k < VOICES; k++)
  {
    if (voices[k].on && voices[k].w == w && voices[k].pos < SR / 20)
      same++;
    if (!voices[k].on && freeK < 0)
      freeK = k;
  }
  if (same < 2 && freeK >= 0)
    voices[freeK] = {w, 0, vol * w->vol, true};
  portEXIT_CRITICAL(&mux);
}
