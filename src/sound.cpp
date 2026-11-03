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
  explosion(0.55f, 0.12f, 70);
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
  free(tmp);
  tmp = nullptr;
}

// ---- 鳴りつづける音 ----
float engPh, engLfo, ufoPh, ufoLfo, beamPh, momPh;
float rainLp, rainHp, rainLp2, engOut, engOut2;
float bird, birdPh, birdF, birdDf;
int birdLeft, birdGap = SR;
float curEngine, curRain, curUfo, curBeam, curMom, curBirds, curPitch = 1, curBank;

inline float wrap(float ph) { return ph - floorf(ph); } // 位相は 0〜1 で持つ (float の桁が足りなくならないように)
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
  return sinf(birdPh * PI2) * env;
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
  for (int i = 0; i < BUF; i++)
  {
    float v = 0;
    if (curEngine > 0.001f)
    {
      // プロペラ: 低いうなりに、羽根が空気をたたく脈を重ねる。音程は操作で変わり、旋回中はうなりが少し揺れる
      engLfo = wrap(engLfo + 7.0f / SR);
      float f = 62 * curPitch * (1 + 0.012f * curBank * sinf(engLfo * PI2));
      engPh = wrap(engPh + f / SR);
      float saw = engPh * 2 - 1;
      float sn = max(0.0f, sinf(engPh * 2 * PI2)), puls = sn * sn * sn;
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
      v += rainHp * 0.9f * curRain;
    }
    if (curUfo > 0.001f || curBeam > 0.001f)
      ufoLfo = wrap(ufoLfo + 6.5f / SR);
    if (curUfo > 0.001f)
    {
      // 円盤: ゆらゆら揺れる高い音
      ufoPh = wrap(ufoPh + (420 + 70 * sinf(ufoLfo * PI2)) / SR);
      v += sinf(ufoPh * PI2) * 0.22f * curUfo;
    }
    if (curBeam > 0.001f)
    {
      beamPh = wrap(beamPh + (880 + 40 * sinf(ufoLfo * 3 * PI2)) / SR);
      v += (sinf(beamPh * PI2) + 0.5f * sinf(beamPh * 3 * PI2)) * 0.12f * curBeam;
    }
    if (curMom > 0.001f)
    {
      // 母船: 低くうねる音 (うねりは 24 周に 1 回)
      momPh += 48.0f / SR;
      if (momPh >= 24)
        momPh -= 24;
      float env = 0.6f + 0.4f * sinf(momPh * PI2 / 24);
      v += (sinf(momPh * PI2) + 0.4f * sinf(momPh * 4.02f * PI2)) * env * 0.35f * curMom;
    }
    if (curBirds > 0.001f)
      v += birdSample() * 0.18f * curBirds;
    mix[i] += v;
  }
}

// ---- 作った音をフラッシュ (LittleFS) に覚えておく ----
// 作るのに 50 秒ほどかかるので、一度作ったら /sfx.bin に保存し、次からは読むだけにする (1.4MB ほど)。
// 見出しに sound.cpp をビルドした日時を入れておき、音の作り方を変えて sound.cpp がビルドしなおされたら作りなおす
const char *const CACHE = "/sfx.bin";
const char CACHE_KEY[40] = "SKYLARK-SFX " __DATE__ " " __TIME__;

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
    loopSounds(mix, mute);
    float m = mute ? 0 : self->Master;
    for (int i = 0; i < BUF; i++)
    {
      float x = mix[i] * m;
      x = x / (1 + fabsf(x) * 0.6f); // やわらかく頭を抑える
      int16_t s = (int16_t)(max(-1.0f, min(1.0f, x)) * 30000);
      out[i * 2] = out[i * 2 + 1] = s;
    }
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
