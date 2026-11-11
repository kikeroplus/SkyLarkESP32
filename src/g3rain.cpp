// 3 本目 (LONGSHOT) のシーン 4: 雨の夜の繁華街。雑居ビルのネオン、ぬれた路面の映り込み、屋台と自動販売機
#include "g3world.h"
#include <math.h>

namespace w3
{
static const uint32_t IRON = 0x2a2a2e;
struct Bld
{
  float x, w, top;
  uint32_t c;
};
static const Bld BLD[4] = {{0, 62, 22, 0x5a5e66}, {62, 62, 6, 0x6a5a52}, {124, 58, 30, 0x7a7468}, {182, 58, 14, 0x4e5662}};
static const int NWIN = 24;
static Box WINS[NWIN];
static uint8_t LIT[NWIN];
static const uint32_t NEON[6] = {0xff3a6a, 0x3ae0ff, 0xffd040, 0x6aff6a, 0xff8a2a, 0xc070ff};

// 縦長のネオン看板 (文字を縦に並べ、にじみを重ねる)
static void vsign(float x, float y, const char *s, uint32_t c, uint32_t bg)
{
  int n = strlen(s);
  float cell = 0.6f, h = n * cell * 8.4f + 2;
  R(x - 0.6f, y - 0.6f, 6.4f, h + 1.2f, IRON);
  R(x, y, 5.2f, h, bg);
  for (int i = 0; i < n; i++)
  {
    char ch[2] = {s[i], 0};
    float cy = y + 1 + i * cell * 8.4f;
    sign(x + 1.1f - 0.4f, cy - 0.4f, cell * 1.15f, ch, c | EMIT, 60);
    sign(x + 1.1f, cy, cell, ch, c | EMIT);
  }
  for (int k = 1; k <= 3; k++)
    R(x - k * 1.2f, y - k, 5.2f + k * 2.4f, h + k * 2, c | EMIT, 14);
  L(x - 1, y + 2, x - 4, y + 2, 0.4f, IRON);
  L(x - 1, y + h - 2, x - 4, y + h - 2, 0.4f, IRON);
}

static void buildings()
{
  sky();
  skyline(40);
  // 雨雲
  for (int i = 0; i < 8; i++)
    E(hh(i, 1) * 240, 6 + hh(i, 2) * 14, 30 + hh(i, 3) * 20, 4, 0x1a2030 | EMIT, 150);
  int wi = 0;
  for (int b = 0; b < 4; b++)
  {
    const Bld &B = BLD[b];
    tex(B.x, B.top, B.w, 160 - B.top, B.c, 0.08f, 6, b * 7 + 1);
    for (float y = B.top + 3; y < 160; y += 3) // タイルの目地
      R(B.x, y, B.w, 0.2f, shade(B.c, 0.8f), 120);
    R(B.x, B.top - 1.4f, B.w, 1.6f, shade(B.c, 0.7f));
    R(B.x, B.top, 1, 160 - B.top, shade(B.c, 1.15f));
    R(B.x + B.w - 1, B.top, 1, 160 - B.top, shade(B.c, 0.7f));
    // 屋上の物
    R(B.x + B.w * 0.6f, B.top - 6, 8, 4.6f, 0x6a6a70);
    for (int k = 0; k < 3; k++)
      R(B.x + B.w * 0.6f + 1 + k * 2.4f, B.top - 5.2f, 1.6f, 3, 0x4a4a50);
    // 窓 (階ごとに 3 つ)
    for (float y = B.top + 8; y < 150; y += 18)
      for (int k = 0; k < 3; k++)
      {
        float wx = B.x + 6 + k * (B.w - 12) / 3, ww = (B.w - 12) / 3 - 4;
        bool lit = hh(wx, y) < 0.55f;
        uint32_t lc = hh(wx, y, 2) < 0.25f ? 0x9ab8ff : hh(wx, y, 2) < 0.5f ? 0xe0ecf4 : 0xf2c46c;
        R(wx - 0.8f, y - 0.8f, ww + 1.6f, 11.6f, shade(B.c, 0.6f));
        for (int i = 0; i < 40; i++)
          R(wx, y + i / 4.0f, ww, 0.25f, lit ? (mix(shade(lc, 0.7f), lc, i / 40.0f) | EMIT) : 0x141820);
        if (lit && hh(wx, y, 3) < 0.5f)
          R(wx, y, ww, 3, shade(lc, 0.6f) | EMIT); // 半分おりたブラインド
        R(wx + ww / 2 - 0.2f, y, 0.4f, 10, shade(B.c, 0.5f));
        if (hh(wx, y, 4) < 0.3f) // 室外機
        {
          R(wx + ww - 4, y + 10.6f, 5, 3.4f, 0xa8a8a4);
          ring(wx + ww - 1.8f, y + 12.3f, 1.1f, 0.25f, 0x5a5a5a);
        }
        if (wi < NWIN && k != 1)
        {
          WINS[wi] = {wx, y, ww, 10};
          LIT[wi] = lit;
          wi++;
        }
      }
  }
  // 非常階段 (3 つ目のビルの横)
  for (float y = 44; y < 160; y += 18)
  {
    R(176, y + 10, 10, 0.6f, IRON);
    for (float k = 176; k < 186; k += 1.2f)
      L(k, y + 7, k, y + 10, 0.15f, IRON);
    R(176, y + 7, 10, 0.3f, IRON);
    L(177, y + 10, 185, y + 28, 0.4f, IRON);
  }
  // 大きな広告の画面 (枠だけ。中は動く)
  R(70, 20, 46, 28, 0x1a1a1e);
  R(71, 21, 44, 26, 0x0a0c10 | EMIT);
  // 縦のネオン
  vsign(55, 40, "BAR", NEON[0], 0x1a0a10 | EMIT);
  vsign(118, 54, "KARAOKE", NEON[1], 0x0a1418 | EMIT);
  vsign(176.5f, 48, "HOTEL", NEON[5], 0x140a1a | EMIT);
  vsign(233, 34, "GAME", NEON[2], 0x1a1408 | EMIT);
  // 横のネオン
  {
    const char *s = "PACHINKO";
    float w = signW(s, 0.9f);
    R(184, 126, w + 4, 9, 0x101014 | EMIT);
    for (int k = 1; k <= 3; k++)
      R(184 - k, 126 - k, w + 4 + k * 2, 9 + k * 2, NEON[3] | EMIT, 14);
    sign(186, 127.4f, 0.9f, s, NEON[3] | EMIT);
  }
  // 電柱と電線
  L(100, 300, 100, 30, 1.4f, 0x4a4a4e);
  L(92, 36, 108, 36, 0.6f, 0x3a3a3e);
  L(94, 42, 106, 42, 0.5f, 0x3a3a3e);
  R(101.4f, 46, 4, 6, 0x6a6a70);
  for (int k = 0; k < 3; k++)
  {
    float y0 = 36 + k * 0.6f;
    for (float x = -10; x < 250; x += 10)
    {
      float sagA = 3 * (1 - ((x - 100) / 140) * ((x - 100) / 140)), sagB = 3 * (1 - ((x + 10 - 100) / 140) * ((x + 10 - 100) / 140));
      L(x, y0 + 4 - sagA + k * 3, x + 10, y0 + 4 - sagB + k * 3, 0.15f, 0x1a1a1e);
    }
  }
}

static void shops()
{
  // 1 階の店: コンビニ、ラーメン屋、バー、ゲームセンター
  R(0, 160, WW, 42, 0x2a2a30);
  // コンビニ (白く明るい)
  R(2, 166, 56, 36, 0xe8f4f0 | EMIT);
  R(2, 162, 56, 4, 0x2a8a5a | EMIT);
  sign(6, 162.6f, 0.4f, "24H MART", 0xffffff | EMIT);
  for (int r = 0; r < 3; r++)
    for (int k = 0; k < 9; k++)
      R(5 + k * 5.8f, 172 + r * 6, 4.6f, 3, NEON[(r + k) % 6] | EMIT, 120);
  R(26, 180, 10, 22, 0xc8dcd8 | EMIT);
  R(30.8f, 180, 0.4f, 22, 0x5a6a68);
  // ラーメン屋 (のれんと赤ちょうちん)
  R(64, 162, 56, 40, 0x3a2418);
  R(66, 168, 52, 34, 0xf0b060 | EMIT);
  R(64, 162, 56, 5, 0x1a1210);
  sign(72, 162.8f, 0.5f, "RAMEN", 0xfff0d0 | EMIT);
  for (int k = 0; k < 6; k++) // のれん
  {
    R(68 + k * 8.4f, 168, 7.6f, 10, 0x2a3a6a);
    R(68 + k * 8.4f, 168, 7.6f, 1, 0x1a2a5a);
  }
  sign(76, 172, 0.5f, "MEN-YA", 0xf4f0e8);
  for (float lx : {62.0f, 122.0f}) // ちょうちん
  {
    L(lx, 162, lx, 165, 0.2f, IRON);
    E(lx, 168.5f, 2.4f, 3.4f, 0xff4a2a | EMIT);
    R(lx - 2, 165.2f, 4, 0.6f, 0x1a1a1a);
    R(lx - 2, 171.4f, 4, 0.6f, 0x1a1a1a);
    for (int k = 1; k <= 3; k++)
      C(lx, 168.5f, 3 + k * 2, 0xff6a3a | EMIT, 16);
  }
  // バー (暗い木の扉と、小さな丸い灯り)
  R(128, 164, 46, 38, 0x2a1a14);
  R(144, 176, 14, 26, 0x5a3a24);
  C(156, 189, 0.6f, 0xd8b048);
  C(151, 170, 2, 0xff6a9a | EMIT);
  for (int k = 1; k <= 3; k++)
    C(151, 170, 2 + k * 1.6f, 0xff6a9a | EMIT, 22);
  R(130, 178, 12, 14, 0xa04a6a | EMIT, 160);
  // ゲームセンター (色の変わる入口)
  R(184, 164, 54, 38, 0x14101a);
  for (int i = 0; i < 6; i++)
    R(186 + i * 8.6f, 168, 7.8f, 34, NEON[i] | EMIT, 150);
  R(184, 162, 54, 3, 0xffd040 | EMIT);
  // ---- ぬれた歩道と車道 (ネオンが縦に映る) ----
  tex(0, 202, WW, 20, 0x2a2c32, 0.08f, 12, 81, 0.06f);
  for (float y = 206; y < 222; y += 4)
    R(0, y, WW, 0.2f, 0x1a1c22, 150);
  R(0, 221.4f, WW, 1.4f, 0x5a5a60);
  R(0, 222.8f, WW, 1.2f, 0x2a2a30);
  tex(0, 224, WW, 38, 0x18191e, 0.08f, 4, 83, 0.12f);
  for (float x = -4; x < WW + 4; x += 16)
    R(x, 242.6f, 8, 0.8f, 0x8a8678, 200);
  R(0, 262, WW, 2.4f, 0x4a4a50);
  tex(0, 264.4f, WW, 36, 0x26282e, 0.08f, 12, 85, 0.06f);
  // 映り込み: 店と看板の色を下へ縦の筋で
  struct Ref
  {
    float x, w;
    uint32_t c;
  };
  static const Ref REF[] = {{4, 52, 0xe8f4f0}, {66, 52, 0xf0b060}, {130, 12, 0xa04a6a}, {186, 50, 0xffd040}, {55, 6, 0xff3a6a}, {118, 6, 0x3ae0ff}, {62, 4, 0xff4a2a}, {122, 4, 0xff4a2a}};
  for (auto &r : REF) // 光の柱が下へ伸び、さざ波で横にゆれて切れ切れになる
  {
    float len = r.w < 10 ? 58 : 44;
    for (float y = 203; y < 203 + len; y += 0.5f)
    {
      float k = (y - 203) / len, rip = 0.5f + 0.5f * sinf(y * 2.3f + r.x);
      float wob = sinf(y * 1.7f + r.x) * (0.4f + k * 1.2f), narrow = r.w * (1 - k * 0.35f);
      int a = (int)(75 * powf(1 - k, 1.5f) * (0.35f + 0.65f * rip));
      if (y > 221.4f && y < 224) // 縁石の段では映らない
        continue;
      R(r.x + (r.w - narrow) / 2 + wob, y, narrow, 0.5f, r.c | EMIT, a);
    }
  }
  // 水たまり
  for (int i = 0; i < 9; i++)
  {
    float x = hh(i, 90) * 230 + 5, y = 206 + hh(i, 91) * 90;
    if (y > 222 && y < 226)
      continue;
    E(x, y, 4 + hh(i, 92) * 6, 0.8f + hh(i, 93) * 0.8f, 0x3a4a6a | EMIT, 120);
    E(x - 1, y - 0.2f, 2, 0.3f, 0x8ab0e0 | EMIT, 90);
  }
  // 屋台 (赤ちょうちんと、のれん)
  R(14, 272, 40, 2, 0x3a2a1a);
  T(10, 272, 58, 272, 34, 266, 0x6a3a2a);
  R(16, 274, 2, 20, 0x4a3420);
  R(50, 274, 2, 20, 0x4a3420);
  R(14, 284, 40, 10, 0x7a5a3a);
  R(14, 284, 40, 1, 0x9a7a5a);
  for (int k = 0; k < 4; k++)
    R(18 + k * 8, 274, 7, 5, 0xe8e0d0, 220);
  E(34, 279, 18, 6, 0xffb060 | EMIT, 60);
  C(12, 276, 2.4f, 0xff4a2a | EMIT);
  for (int k = 1; k <= 3; k++)
    C(12, 276, 2.4f + k * 2, 0xff6a3a | EMIT, 18);
  // 自動販売機 (光る)
  for (int k = 0; k < 2; k++)
  {
    float vx = 196 + k * 12;
    R(vx, 266, 11, 26, k ? (0xe83a3a | EMIT) : (0x2a6ae8 | EMIT));
    R(vx + 1, 268, 9, 12, 0xf4f8f8 | EMIT);
    for (int r = 0; r < 3; r++)
      for (int c = 0; c < 4; c++)
        R(vx + 1.6f + c * 2.1f, 269 + r * 3.8f, 1.4f, 2.6f, NEON[(r * 4 + c) % 6] | EMIT);
    R(vx + 2, 284, 7, 3, 0x1a1a1a);
    E(vx + 5.5f, 294, 10, 2, (k ? 0xe83a3a : 0x2a6ae8) | EMIT, 60);
  }
}

static void draw(int r0, int r1)
{
  if (r0 < 162 * Z)
    buildings();
  if (r1 > 158 * Z)
    shops();
}

// 広告の画面の絵と、またたくネオン
static void dyn(int layer, float wind, int t)
{
  if (layer != 0)
    return;
  int scn = (t / 90) % 3;
  float k = (t % 90) / 90.0f;
  if (scn == 0) // 飲み物の広告: 青い地に光る瓶
  {
    R(71, 21, 44, 26, 0x1a4aa0 | EMIT);
    R(88, 25 + (1 - k) * 4, 7, 18, 0xe8f4ff | EMIT);
    R(90, 22 + (1 - k) * 4, 3, 4, 0xe8f4ff | EMIT);
    for (int i = 0; i < 6; i++)
      C(75 + i * 7, 44 - fmodf(t * 0.4f + i * 5, 20), 0.8f, 0xbfe0ff | EMIT);
  }
  else if (scn == 1) // 映画の予告: 夕焼けと人影
  {
    R(71, 21, 44, 26, 0xe86a3a | EMIT);
    R(71, 34, 44, 13, 0x3a1a2a | EMIT);
    C(93, 34, 6, 0xffd070 | EMIT);
    R(71 + k * 30, 30, 2, 6, 0x1a0a10 | EMIT);
    C(72 + k * 30, 29, 1.2f, 0x1a0a10 | EMIT);
  }
  else // 文字の広告
  {
    R(71, 21, 44, 26, 0x0a0a0a | EMIT);
    sign(74, 28, 0.9f, "NEON", ((t / 6) % 2 ? 0xff3a6a : 0xffd040) | EMIT);
    sign(77, 37, 0.6f, "NIGHT", 0x3ae0ff | EMIT);
  }
  if ((t / 7) % 9 == 0) // 「HOTEL」のネオンがときどき消える
    R(176.5f, 48, 5.2f, 5 * 0.6f * 8.4f + 2, 0x140a1a | EMIT);
}

static bool initWins = []() {
  // 窓の場所は buildings() が描くときに決まる。先に同じ計算で埋めておく
  int wi = 0;
  for (int b = 0; b < 4; b++)
  {
    const Bld &B = BLD[b];
    for (float y = B.top + 8; y < 150; y += 18)
      for (int k = 0; k < 3; k++)
      {
        float wx = B.x + 6 + k * (B.w - 12) / 3, ww = (B.w - 12) / 3 - 4;
        if (wi < NWIN && k != 1)
        {
          WINS[wi] = {wx, y, ww, 10};
          LIT[wi] = hh(wx, y) < 0.55f;
          wi++;
        }
      }
  }
  return true;
}();

extern const Scene RAIN_SCENE = {
    "NEON ALLEY", draw, dyn,
    2, {{-8, 206, 256, 14}, {-8, 272, 256, 24}, {}},
    1, {232, 0}, {-1, 1}, {22, 0}, {(1 << CAR_TAXI) | (1 << CAR_SEDAN), 0}, {-1, -1},
    NWIN, WINS, LIT,
    4, {{30, 290, 26}, {207, 292, 22}, {30, 218, 30}, {210, 218, 30}},
    4, {{2, 202, 56, 18}, {64, 202, 56, 18}, {184, 202, 54, 18}, {128, 202, 46, 10}},
    0, {},
    {{0, 0, 0, 0}, {0, 0, 0, 0}}, false,
    202, 300, 0.8f, 202, 0.5f, true};
} // namespace w3
