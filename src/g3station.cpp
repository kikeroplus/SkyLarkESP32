// 3 本目 (LONGSHOT) のシーン 2: 駅前のロータリー。時計台のある駅舎、ガラスのひさし、バス停とタクシー乗り場
#include "g3world.h"
#include <math.h>

namespace w3
{
static const uint32_t IRON = 0x2a2a2e, BRICK = 0xa4523c, BRICK_D = 0x7e3a2a, STONE = 0xd8ccb2, STONE_D = 0xb0a488, COPPER = 0x5a8a78;
static const float WX[6] = {10, 28, 46, 182, 200, 218}, WY[2] = {70, 104};
static const float WW_ = 12, WH_ = 17;

static void archWin(float cx, float y, float w, float h, bool lit, uint32_t litC)
{
  // 上が丸い窓: 石の縁、ガラス、桟
  C(cx, y + w / 2, w / 2 + 1.2f, STONE);
  R(cx - w / 2 - 1.2f, y + w / 2, w + 2.4f, h - w / 2 + 1, STONE);
  C(cx, y + w / 2, w / 2, 0x3a3428);
  R(cx - w / 2, y + w / 2, w, h - w / 2, 0x3a3428);
  for (int i = 0; i < (int)(h * 4); i++)
  {
    float k = i / (h * 4), yy = y + i / 4.0f;
    float half = yy < y + w / 2 ? sqrtf(max(0.0f, (w / 2 - 0.5f) * (w / 2 - 0.5f) - (y + w / 2 - yy) * (y + w / 2 - yy))) : w / 2 - 0.5f;
    uint32_t g = lit ? (mix(shade(litC, 0.8f), litC, k) | EMIT) : tod == DAY ? mix(0xa8c4d8, 0x445e74, k) : (mix(0x283244, 0x121822, k) | EMIT);
    R(cx - half, yy, half * 2, 0.25f, g);
  }
  L(cx, y + 0.6f, cx, y + h, 0.4f, 0x3a3428);
  R(cx - w / 2, y + w / 2 + 2, w, 0.4f, 0x3a3428);
  R(cx - w / 2 - 1.6f, y + h, w + 3.2f, 1.2f, STONE);
  R(cx - w / 2 - 1.2f, y + h + 1.2f, w + 2.4f, 0.6f, 0x000000, 60);
  if (!lit && tod == DAY)
    L(cx - w / 2 + 1.5f, y + h - 1, cx + w / 2 - 2, y + w / 2 - 1, 0.5f, 0xffffff, 50);
}

static bool winOn(int i) { return hh(i, 77) < 0.55f; }

static void station()
{
  bool night = tod != DAY;
  // ---- 時計台 ----
  R(103, 6, 34, 40, STONE);
  tex(103, 6, 34, 40, STONE, 0.08f, 8, 21);
  R(103, 6, 1.2f, 40, 0xece2cc);
  R(135.4f, 6, 1.6f, 40, STONE_D);
  T(100, 6, 140, 6, 120, -8, COPPER);
  T(120, -8, 140, 6, 120, 6, shade(COPPER, 0.8f));
  for (float k = 103; k < 138; k += 3)
    L(k, 6, 120 + (k - 120) * 0.15f, -6, 0.2f, shade(COPPER, 0.7f), 160);
  R(99, 5, 42, 2, STONE_D);
  L(120, -8, 120, -14, 0.4f, IRON); // 避雷針
  // 時計
  C(120, 24, 11, STONE_D);
  C(120, 24, 10, 0x2a2622);
  C(120, 24, 9.2f, night ? (0xfff2d0 | EMIT) : 0xf4efe2);
  for (int i = 0; i < 12; i++)
  {
    float a = i * 0.5236f;
    float r0 = i % 3 ? 7.6f : 6.8f;
    L(120 + cosf(a) * r0, 24 + sinf(a) * r0, 120 + cosf(a) * 8.6f, 24 + sinf(a) * 8.6f, i % 3 ? 0.3f : 0.6f, 0x1a1a1a);
  }
  L(120, 24, 120 + cosf(-2.0f) * 5, 24 + sinf(-2.0f) * 5, 0.7f, 0x1a1a1a); // 時針 (10 時)
  L(120, 24, 120 + cosf(-1.0f) * 7.4f, 24 + sinf(-1.0f) * 7.4f, 0.45f, 0x1a1a1a);
  C(120, 24, 0.6f, 0x1a1a1a);
  for (float wx : {110.0f, 130.0f}) // 細い窓
  {
    R(wx - 1.5f, 38, 3, 6, 0x3a3428);
    C(wx, 38, 1.5f, 0x3a3428);
  }
  // ---- 両翼 (れんが) ----
  for (int s = 0; s < 2; s++)
  {
    float x0 = s ? 168 : 2, w = 70;
    tex(x0, 48, w, 128, BRICK, 0.12f, 6, 31 + s);
    for (float y = 49; y < 176; y += 1.6f) // れんがの目地
    {
      R(x0, y, w, 0.2f, BRICK_D, 120);
      float off = ((int)(y / 1.6f) % 2) * 1.6f;
      for (float x = x0 + off; x < x0 + w; x += 3.2f)
        R(x, y, 0.2f, 1.6f, BRICK_D, 80);
    }
    R(x0 - 1, 44, w + 2, 4, STONE);
    R(x0 - 1.5f, 43, w + 3, 1.2f, STONE_D);
    for (float x = x0; x < x0 + w; x += 2)
      R(x, 47, 1, 1, STONE_D);
    R(x0, 48, w, 1.4f, 0x000000, 60);
    for (float y : {94.0f, 128.0f})
    {
      R(x0, y, w, 1.8f, STONE);
      R(x0, y + 1.8f, w, 0.5f, STONE_D);
    }
    // 屋根の小窓
    for (int k = 0; k < 3; k++)
    {
      float dx = x0 + 12 + k * 22;
      T(dx - 5, 44, dx + 5, 44, dx, 36, 0x4a4e58);
      R(dx - 2.4f, 38.4f, 4.8f, 5.6f, STONE);
      R(dx - 1.6f, 39.4f, 3.2f, 4.6f, night && hh(k, s) < 0.5f ? (0xf2c46c | EMIT) : 0x3a4656);
    }
    // 1 階のアーケード
    for (int k = 0; k < 4; k++)
    {
      float ax = x0 + 9 + k * 17.3f;
      C(ax, 148, 7, STONE);
      R(ax - 7, 148, 14, 28, STONE);
      C(ax, 148, 6, 0x2a2420);
      R(ax - 6, 148, 12, 28, 0x2a2420);
      bool shop = k != 0 && k != 3;
      R(ax - 5.4f, 152, 10.8f, 24, shop ? (night ? (0xf0c878 | EMIT) : 0x6a7880) : 0x3a3430);
      if (shop)
      {
        R(ax - 5.4f, 166, 10.8f, 1.6f, night ? (0x8a5a30 | EMIT) : 0x4a3424);
        for (int j = 0; j < 4; j++)
          C(ax - 3.6f + j * 2.4f, 164.6f, 0.8f, j % 2 ? 0xd84a4a : 0xf0d040);
      }
    }
    sign(x0 + w / 2 - signW(s ? "TICKETS" : "KIOSK", 0.5f) / 2, 131.4f, 0.5f, s ? "TICKETS" : "KIOSK", 0x2a2420);
  }
  for (int i = 0; i < 12; i++) // 両翼の窓
  {
    float x = WX[i % 6], y = WY[i / 6];
    archWin(x + WW_ / 2, y, WW_, WH_, night && winOn(i), hh(i, 78) < 0.3f ? 0xcfe0f0 : 0xf2c46c);
  }
  // ---- 真ん中の大きな建物 ----
  tex(70, 40, 100, 136, STONE, 0.07f, 8, 41);
  for (float y = 43; y < 176; y += 3.4f)
    R(70, y, 100, 0.25f, STONE_D, 120);
  R(70, 40, 1.4f, 136, 0xece2cc);
  R(168.4f, 40, 1.6f, 136, STONE_D);
  R(68, 38, 104, 3, STONE_D);
  R(67, 36.6f, 106, 1.6f, STONE);
  // 三角の破風
  T(68, 37, 172, 37, 120, 26, STONE);
  T(76, 36, 164, 36, 120, 29.5f, STONE_D);
  // 大きなアーチの窓 (扇形の桟)
  float acx = 120, acy = 92, ar = 36;
  C(acx, acy, ar + 3, 0xe6dcc4);
  R(acx - ar - 3, acy, (ar + 3) * 2, 58, 0xe6dcc4);
  for (int i = -3; i <= 3; i++) // 要石とアーチの石
  {
    float a = -1.5708f + i * 0.22f;
    Q(acx + cosf(a - 0.09f) * (ar + 0.5f), acy + sinf(a - 0.09f) * (ar + 0.5f), acx + cosf(a + 0.09f) * (ar + 0.5f), acy + sinf(a + 0.09f) * (ar + 0.5f),
      acx + cosf(a + 0.09f) * (ar + 3.5f), acy + sinf(a + 0.09f) * (ar + 3.5f), acx + cosf(a - 0.09f) * (ar + 3.5f), acy + sinf(a - 0.09f) * (ar + 3.5f),
      i == 0 ? 0xf2ead8 : STONE);
  }
  for (int i = 0; i < (int)(ar * 4) + 58 * 4; i++)
  {
    float yy = acy - ar + i / 4.0f, k = (yy - (acy - ar)) / (ar + 58);
    float half = yy < acy ? sqrtf(max(0.0f, ar * ar - (acy - yy) * (acy - yy))) : ar;
    uint32_t g = night ? (mix(0xf8dca0, 0xc89850, k) | EMIT) : mix(0xb8d0e0, 0x4a6478, k);
    R(acx - half, yy, half * 2, 0.25f, g);
  }
  if (night) // 中の大きな時刻表と、天井の灯り
  {
    R(100, 118, 40, 9, 0x1a1a20 | EMIT);
    for (int r = 0; r < 3; r++)
      for (int c = 0; c < 6; c++)
        R(102 + c * 6.2f, 119.5f + r * 2.3f, 4.6f, 1.2f, (c == 0 ? 0xffa030 : 0xf8e8a0) | EMIT, 220);
  }
  else
  {
    Q(acx - 22, 150, acx - 14, 150, acx + 10, 60, acx + 2, 60, 0xffffff, 40);
  }
  for (int i = 0; i <= 8; i++) // 扇の桟
  {
    float a = 3.1416f + i * 3.1416f / 8;
    L(acx, acy, acx + cosf(a) * ar, acy + sinf(a) * ar, 0.5f, 0x3a3a36);
  }
  ring(acx, acy, ar * 0.45f, 0.5f, 0x3a3a36);
  for (float k = -ar; k <= ar; k += 9)
    L(acx + k, acy, acx + k, acy + 58, 0.5f, 0x3a3a36);
  for (float y = acy + 14; y < acy + 58; y += 14)
    R(acx - ar, y, ar * 2, 0.5f, 0x3a3a36);
  // 駅の名前
  R(76, 150, 88, 8, STONE_D);
  R(76, 150, 88, 0.6f, 0xece2cc);
  {
    const char *nm = "CENTRAL STATION";
    float cell = 0.72f;
    sign(120 - signW(nm, cell) / 2 + 0.3f, 151.6f, cell, nm, 0x3a2a18);
    sign(120 - signW(nm, cell) / 2, 151.3f, cell, nm, night ? (0xffe0a0 | EMIT) : 0xd8b048);
  }
  // 入口 (3 つのアーチの扉)
  for (int k = 0; k < 3; k++)
  {
    float dx = 92 + k * 28;
    C(dx, 164, 8, 0xe6dcc4);
    R(dx - 8, 164, 16, 12, 0xe6dcc4);
    C(dx, 164, 6.6f, 0x241e18);
    R(dx - 6.6f, 164, 13.2f, 12, 0x241e18);
    R(dx - 5.6f, 162, 11.2f, 14, night ? (0xf8d890 | EMIT) : 0x7a8a96);
    L(dx, 158, dx, 176, 0.4f, 0x5a4a30);
  }
  // ガラスのひさしと柱
  Q(72, 158, 168, 158, 172, 165, 68, 165, night ? (0x8aa0b0 | EMIT) : 0xb8d4e0, 200);
  for (float k = 72; k <= 168; k += 8)
    L(k, 158, k + (k - 120) * 0.08f, 165, 0.35f, IRON);
  R(68, 165, 104, 1, IRON);
  R(68, 166, 104, 2.6f, 0x000000, 60);
  for (float px : {72.0f, 104.0f, 136.0f, 168.0f})
  {
    L(px, 165, px, 186, 0.8f, IRON);
    R(px - 1.2f, 185, 2.4f, 1.4f, IRON);
  }
}

static void plaza()
{
  bool on = tod != DAY;
  // 石畳の広場 (大きな四角の石を斜めに)
  tex(0, 176, WW, 40, 0xc2bdb2, 0.06f, 32, 51, 0.05f);
  for (float k = -60; k < 300; k += 9)
  {
    L(k, 176, k + 40, 216, 0.15f, 0x9a958a, 140);
    L(k + 40, 176, k, 216, 0.15f, 0x9a958a, 140);
  }
  R(0, 176, WW, 1.6f, 0x000000, 60);
  // 花壇
  for (float fx : {30.0f, 210.0f})
  {
    R(fx - 16, 197, 32, 6, 0x8a8478);
    R(fx - 16, 197, 32, 1, 0xb0aa9e);
    E(fx, 197.4f, 15, 2.6f, 0x3f6a34);
    for (int i = 0; i < 20; i++)
      C(fx - 14 + hh(i, fx) * 28, 196 + hh(i, fx, 1) * 3, 0.7f, i % 3 == 0 ? 0xe84a5a : i % 3 == 1 ? 0xf0d040 : 0xf4f0ec);
  }
  if (on)
  {
    for (float lx : {60.0f, 180.0f})
      E(lx, 208, 20, 5, 0xffd890 | EMIT, tod == NIGHT ? 70 : 40);
    E(120, 182, 46, 5, 0xf8d890 | EMIT, 60);
  }
  // 縁石
  R(0, 213.4f, WW, 1.4f, 0xd4d0c8);
  R(0, 214.8f, WW, 1.2f, 0x86827a);
  // ---- ロータリーの車道 ----
  tex(0, 216, WW, 46, 0x4c4f54, 0.06f, 4, 53, 0.16f);
  R(0, 216, WW, 20, 0x8a3a2a, 40); // バス専用の赤い車線
  sign(150, 223, 0.9f, "BUS", 0xe8e4d8, 200);
  sign(20, 223, 0.9f, "BUS", 0xe8e4d8, 200);
  for (float x = -4; x < WW + 4; x += 16)
    R(x, 237.2f, 8, 0.9f, 0xe8e4d8, 220);
  R(40, 216, 40, 0.8f, 0xf0d040, 220); // 停留所の黄色い線
  sign(46, 217.6f, 0.5f, "BUS STOP", 0xf0d040, 200);
  for (int i = 0; i < 5; i++) // 横断歩道
  {
    float cx = 196 + i * 6.2f;
    R(cx, 216.6f, 3.6f, 44.6f, 0xdcd8cc, 230);
  }
  E(130, 252, 3.8f, 1.7f, 0x2e3034);
  E(130, 252, 3.2f, 1.3f, 0x45484e);
  R(0, 262, WW, 2.4f, 0xd0ccc4);
  R(0, 262, WW, 0.5f, 0x86827a);
  // ---- 手前の歩道 ----
  tex(0, 264.4f, WW, 36, 0xb4afa4, 0.06f, 16, 55, 0.05f);
  for (float y = 268; y < 300; y += 4)
    R(0, y, WW, 0.2f, 0x8f8a80, 110);
  for (int row = 0; row < 9; row++)
    for (float x = (row % 2) * 4; x < WW; x += 8)
      R(x, 264.4f + row * 4, 0.2f, 4, 0x8f8a80, 90);
  // 点字ブロック
  for (float x = 0; x < WW; x += 2)
    R(x + 0.2f, 266, 1.6f, 1.6f, 0xe8c040);
  // バス停の屋根つきの待合 (ガラスの壁、ベンチ、広告)
  R(38, 266, 52, 1.4f, IRON);
  R(38, 264.6f, 52, 1.4f, 0x6a7078);
  R(40, 267.4f, 48, 20, on ? (0x6a7a88 | EMIT) : 0x9ab4c4, 120);
  for (float k = 40; k <= 88; k += 12)
    R(k - 0.4f, 266, 0.8f, 24, IRON);
  R(64, 268, 10, 16, on ? (0xf8f0d8 | EMIT) : 0xe8e0c8); // 広告
  R(65, 269, 8, 9, on ? (0x4a8ad0 | EMIT) : 0x3a6aa0);
  C(69, 273.5f, 2.4f, on ? (0xf8e070 | EMIT) : 0xf0d040);
  R(46, 282, 16, 1.2f, 0x9a6a44);
  R(47, 283.2f, 0.8f, 4, IRON);
  R(60, 283.2f, 0.8f, 4, IRON);
  L(94, 292, 94, 266, 0.5f, IRON); // 停留所の標識
  C(94, 266, 2.6f, 0x2a6a9a);
  C(94, 266, 1.9f, 0xf4f0e8);
  sign(92.6f, 265.1f, 0.26f, "BUS", 0x2a6a9a);
  if (on)
    E(64, 288, 30, 4, 0xd8e8f0 | EMIT, 50);
  // タクシー乗り場の標識
  L(150, 292, 150, 270, 0.5f, IRON);
  R(146, 268, 8, 4, 0x1a1a1a);
  sign(146.6f, 268.8f, 0.36f, "TAXI", on ? (0xf8d040 | EMIT) : 0xf0c830);
  // 柱の時計
  L(214, 292, 214, 270, 0.9f, 0x1e3a30);
  R(212.6f, 290.6f, 2.8f, 1.6f, 0x1e3a30);
  C(214, 268, 3.4f, 0x1e3a30);
  C(214, 268, 2.8f, on ? (0xfff2d0 | EMIT) : 0xf4efe2);
  L(214, 268, 212.6f, 266.8f, 0.3f, 0x1a1a1a);
  L(214, 268, 214, 265.8f, 0.25f, 0x1a1a1a);
  // 木の植えます
  R(119, 287, 12, 4, 0x2a2622);
  R(124, 266, 2.2f, 22, 0x5b4330);
  R(124, 266, 0.7f, 22, 0x7a5a40);
  for (float lx : {60.0f, 180.0f})
    lampPost(lx, 213, 34);
  // 旗の竿 (両翼の屋根)
  for (float fx : {37.0f, 203.0f})
  {
    L(fx, 24, fx, 44, 0.5f, 0xd0d0d0);
    C(fx, 23.6f, 0.6f, 0xd8b048);
  }
}

static void draw(int r0, int r1)
{
  if (r0 < 60 * Z)
  {
    sky();
    skyline(48);
  }
  station();
  if (r1 > 174 * Z)
    plaza();
}

static void dyn(int layer, float wind, int t)
{
  if (layer == 0)
  {
    drawFlag(37, 24.4f, 11, wind, t, 0xf4f0e8, 0x2a6a9a, 0);
    drawFlag(203, 24.4f, 11, wind, t + 5, 0xf4f0e8, 0x2a6a9a, 1.7f);
  }
}

static Box WINS[12];
static uint8_t LIT[12];
static bool initWins = []() {
  for (int i = 0; i < 12; i++)
  {
    WINS[i] = {WX[i % 6] + 1, WY[i / 6] + 6, WW_ - 2, WH_ - 6};
    LIT[i] = winOn(i);
  }
  return true;
}();

extern const Scene STATION_SCENE = {
    "CENTRAL STATION", draw, dyn,
    2, {{-8, 184, 256, 18}, {-8, 270, 256, 26}, {}},
    2, {219, 241}, {-1, 1}, {20, 26}, {(1 << CAR_BUS) | (1 << CAR_SEDAN) | (1 << CAR_VAN), (1 << CAR_TAXI)}, {44, -1},
    12, WINS, LIT,
    3, {{60, 208, 24}, {180, 208, 24}, {64, 284, 22}},
    2, {{76, 176, 88, 14}, {2, 176, 70, 8}},
    1, {{125, 262, 0.95f}},
    {{168, 288, 36, 8}, {90, 200, 60, 10}}, false,
    176, 380, 0.9f, 176, 0.6f, false};
} // namespace w3
