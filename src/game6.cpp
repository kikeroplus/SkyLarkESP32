// 6 本目 (JUKEBOX): ダイナーふうのレトロなピンボール。動きの作りは ORBIT (game5.cpp) と同じで、台の配置と決まりと絵が違う
#include "game6.h"
#include "g2draw.h"
#include <algorithm>

static const int W = 240, H = 320;
static const int FPS = Game6::FPS;
static const int NF = Game6::NF;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
static const float PI_F = 3.14159265f;

// ---- 台 (液晶の座標)。上の 22 行は点数のリール ----
static const int HUD_H = 22;
static const float BALL_R = 5.0f;
static const float WR = 1.5f;
static const float RB = BALL_R + WR;
static const float GRAV = 420;
static const float VMAX = 1100;
static const float XL = 6, XD = 216, XO = 232, YT = 30;
static const float ARC_R = 56, ARC_LX = 62, ARC_RX = 176, ARC_Y = 86;
static const float GATE_Y0 = 112, GATE_Y1 = 100;
static const float PLY = 300, PL_TRAVEL = 14;
static const float LANE_X = 224;
static const float DRAIN_Y = 334;
// フリッパー: 下の 2 本と、真ん中の壁ぎわの小さい 2 本
struct FDef
{
  float px, py, len, r0, r1, rest, up;
};
static const FDef FD[NF] = {
    {68, 282, 38, 5.5f, 3.0f, 0.49f, -0.45f},
    {154, 282, 38, 5.5f, 3.0f, PI_F - 0.49f, PI_F + 0.45f},
    {17, 168, 26, 4.5f, 2.5f, 0.6f, -0.3f},
    {205, 168, 26, 4.5f, 2.5f, PI_F - 0.6f, PI_F + 0.3f},
};
static const float W_UP = 13, W_DOWN = 8;
static const float E_FLIP = 0.3f;
// ポップバンパー
static const float BX[3] = {70, 152, 111}, BY[3] = {108, 108, 138}, BUMP_R = 11, BUMP_KICK = 240;
// キックアウトの穴
static const float HX = 111, HY = 186;
static const int HOLD_N = 30;
// 上のレーン (1・2・3)
static const float POST_X[4] = {64, 95, 127, 158}, POST_Y0 = 44, POST_Y1 = 60, LANE_SENSE_Y = 54, LAMP_Y = 70;
// 壁の的 (4・5)
static const float TG_Y0 = 118, TG_Y1 = 146;
// ロールオーバーのボタン
static const float BTN_X[4] = {38, 184, 46, 176}, BTN_Y[4] = {100, 100, 148, 148};
// スリングショット (左。右は写す)
static const float SL_AX = 36, SL_AY = 212, SL_BX = 36, SL_BY = 236, SL_CX = 58, SL_CY = 252;
static const float SLING_KICK = 230;
static const float INLANE_Y = 222;
// 明かり
static const float NUM_X[5] = {75, 93, 111, 129, 147}, NUM_Y = 216;
static const float DBL_Y = 163;
static const float AGAIN_Y = 270;

// 色: クリームの面、赤と青緑、クロームの手すり、くるみの木
static const uint32_t INK = 0x000000, CREAM = 0xFFF4DC, RED = 0xE8323A, TEAL = 0x2AB8B0, GOLD = 0xFFC840, MUTE = 0xC8B898, DKRED = 0x7A1218;

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float mirX(float x) { return 222 - x; }

// ---- 壁 ----
enum : uint8_t
{
  K_WALL,
  K_SLING0,
  K_SLING1,
  K_TGT0, // 左の壁の的 (4)
  K_TGT1, // 右の壁の的 (5)
  K_GATE,
};
struct Wall
{
  float x0, y0, x1, y1, e;
  uint8_t kind;
};
static Wall walls[96];
static int nW = 0;
static void addW(float x0, float y0, float x1, float y1, float e, uint8_t k = K_WALL)
{
  if (nW < 96)
    walls[nW++] = {x0, y0, x1, y1, e, k};
}
static void addArc(float cx, float cy, float r, float a0, float a1, int n, float e)
{
  for (int i = 0; i < n; i++)
  {
    float u0 = a0 + (a1 - a0) * i / n, u1 = a0 + (a1 - a0) * (i + 1) / n;
    addW(cx + r * cosf(u0), cy + r * sinf(u0), cx + r * cosf(u1), cy + r * sinf(u1), e);
  }
}
static void slingPts(int s, float &ax, float &ay, float &bx, float &by, float &cx, float &cy)
{
  ax = s ? mirX(SL_AX) : SL_AX, ay = SL_AY;
  bx = s ? mirX(SL_BX) : SL_BX, by = SL_BY;
  cx = s ? mirX(SL_CX) : SL_CX, cy = SL_CY;
}
static void buildWalls()
{
  nW = 0;
  addW(XL, ARC_Y, XL, TG_Y0, 0.45f);
  addW(XL, TG_Y0, XL, TG_Y1, 0.45f, K_TGT0);
  addW(XL, TG_Y1, XL, 250, 0.45f);
  addW(XL, 250, 60, 318, 0.45f);
  addArc(ARC_LX, ARC_Y, ARC_R, PI_F, 1.5f * PI_F, 10, 0.45f);
  addW(ARC_LX, YT, ARC_RX, YT, 0.45f);
  addArc(ARC_RX, ARC_Y, ARC_R, 1.5f * PI_F, 2 * PI_F, 10, 0.45f);
  addW(XO, ARC_Y, XO, DRAIN_Y, 0.3f);
  addW(XD, GATE_Y0, XD, TG_Y0, 0.3f);
  addW(XD, TG_Y0, XD, TG_Y1, 0.45f, K_TGT1);
  addW(XD, TG_Y1, XD, DRAIN_Y, 0.3f);
  addW(XD, GATE_Y0, XO, GATE_Y1, 0.3f, K_GATE);
  addW(XD, 250, mirX(60), 318, 0.45f);
  addW(20, 206, 20, 242, 0.4f);
  addW(20, 242, 62, 276, 0.4f);
  addW(mirX(20), 206, mirX(20), 242, 0.4f);
  addW(mirX(20), 242, mirX(62), 276, 0.4f);
  for (int s = 0; s < 2; s++)
  {
    float ax, ay, bx, by, cx, cy;
    slingPts(s, ax, ay, bx, by, cx, cy);
    addW(ax, ay, bx, by, 0.5f);
    addW(bx, by, cx, cy, 0.5f);
    addW(ax, ay, cx, cy, 0.6f, s ? K_SLING1 : K_SLING0);
  }
  for (int i = 0; i < 4; i++)
    addW(POST_X[i], POST_Y0, POST_X[i], POST_Y1, 0.5f);
}

float Game6::rnd()
{
  seed = seed * 1664525u + 1013904223u;
  return (seed >> 8) * (1.0f / 16777216.0f);
}

void Game6::sfx(Sfx s, float vol)
{
  if (!Snd || sndN >= 5)
    return;
  sndN++;
  Snd->Play(s, mode == Mode::Attract ? vol * 0.35f : vol);
}

// ---- 台の絵 ----
static uint32_t mix(uint32_t a, uint32_t c, float k)
{
  k = clampf(k, 0, 1);
  int ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
  int r = ar + (int)((((int)(c >> 16) & 255) - ar) * k);
  int g = ag + (int)((((int)(c >> 8) & 255) - ag) * k);
  int bb = ab + (int)((((int)c & 255) - ab) * k);
  return (r << 16) | (g << 8) | bb;
}
static uint32_t scale(uint32_t c, float k)
{
  int r = clampf(((c >> 16) & 255) * k, 0, 255), g = clampf(((c >> 8) & 255) * k, 0, 255), b = clampf((c & 255) * k, 0, 255);
  return (r << 16) | (g << 8) | b;
}

// 0 = 台の面、1 = 外の枠、2 = 打ち出すレーン、3 = 下のエプロン
static int region(int x, int y)
{
  if (y < YT || x < XL || x > XO)
    return 1;
  if (x < ARC_LX && y < ARC_Y && hypotf(x - ARC_LX, y - ARC_Y) > ARC_R)
    return 1;
  if (x > ARC_RX && y < ARC_Y && hypotf(x - ARC_RX, y - ARC_Y) > ARC_R)
    return 1;
  if (x > XD)
  {
    float gy = GATE_Y0 + (x - XD) * (GATE_Y1 - GATE_Y0) / (XO - XD);
    return y > gy ? 2 : 0;
  }
  float ay = x <= 60 ? 250 + (x - XL) * (68.0f / 54) : x >= mirX(60) ? 250 + (XD - x) * (68.0f / 54) : 318;
  return y > ay ? 3 : 0;
}

// 台の絵に文字を焼きこむ (液晶の 6x8 の文字。size 倍)
static void bgText(float x, float y, const char *s, uint32_t c, int size, bool center = true, int a = 255)
{
  int n = strlen(s);
  if (center)
    x -= n * 6 * size / 2.0f;
  for (int i = 0; i < n; i++)
  {
    const uint8_t *g = fontGlyph((uint8_t)s[i]);
    if (!g)
      continue;
    for (int gx = 0; gx < 5; gx++)
      for (int row = 0; row < 8; row++)
        if (g[gx] >> row & 1)
          d2::R(x + i * 6 * size + gx * size, y + row * size, size, size, c, a);
  }
}

// 五つの角の星
static void star(float cx, float cy, float r, uint32_t c, int a = 255)
{
  for (int i = 0; i < 5; i++)
  {
    float a0 = -PI_F / 2 + i * 2 * PI_F / 5, a1 = a0 + PI_F / 5, a2 = a0 - PI_F / 5;
    d2::T(cx + cosf(a0) * r, cy + sinf(a0) * r, cx + cosf(a1) * r * 0.42f, cy + sinf(a1) * r * 0.42f, cx + cosf(a2) * r * 0.42f,
          cy + sinf(a2) * r * 0.42f, c, a);
  }
  d2::C(cx, cy, r * 0.45f, c, a);
}

// ブーメラン (50 年代の模様): 太い弧
static void boomerang(float cx, float cy, float r, float a0, uint32_t c, int a)
{
  for (int i = 0; i <= 20; i++)
  {
    float u = a0 + i * 0.09f, th = 1 + 3 * sinf(i / 20.0f * PI_F);
    d2::C(cx + cosf(u) * r, cy + sinf(u) * r, th, c, a);
  }
}

// 光が当たったつやのある円盤 (バンパーの帽子など)
static void dome(float cx, float cy, float r, uint32_t col)
{
  for (int y = (int)floorf(cy - r - 1); y <= (int)ceilf(cy + r + 1); y++)
    for (int x = (int)floorf(cx - r - 1); x <= (int)ceilf(cx + r + 1); x++)
    {
      float dx = x - cx, dy = y - cy, d = hypotf(dx, dy);
      if (d > r + 0.5f)
        continue;
      float cov = std::min(1.0f, r + 0.5f - d), nx = dx / r, ny = dy / r, nz = sqrtf(std::max(0.0f, 1 - nx * nx - ny * ny));
      float lam = std::max(0.0f, -0.5f * nx - 0.5f * ny + 0.7f * nz);
      uint32_t c = scale(col, 0.55f + 0.6f * lam);
      c = mix(c, 0xFFFFFF, powf(lam, 16) * 0.8f);
      d2::P(x, y, c, (int)(cov * 255));
    }
}

void Game6::buildTable()
{
  Surf s;
  s.buf = bg;
  s.y0 = 0;
  s.h = H;
  d2::S = &s;
  // 上の帯: 黒い箱
  for (int y = 0; y < HUD_H; y++)
    d2::R(0, y, W, 1, mix(0x100808, 0x201010, y / (float)HUD_H));
  d2::R(0, HUD_H - 1, W, 1, 0xB8BCC8);
  for (int y = HUD_H; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      int rg = region(x, y);
      uint32_t c;
      if (rg == 0)
      {
        // クリームの面: 古い紙のようなむら、まわりが少し焼けている
        float ux = (x - 111) / 110.0f, uy = (y - 175) / 150.0f;
        c = mix(0xCDB68A, 0xA88C62, (ux * ux + uy * uy) * 0.7f); // 落ち着いた生成り (古い紙の色)
        c = scale(c, 1 + (d2::vn(x * 0.08f, y * 0.08f) - 0.5f) * 0.06f + (d2::h2(x, y) - 0.5f) * 0.03f);
        // 下はダイナーの床の市松
        if (y > 262)
        {
          bool chk = ((x / 8) + (y / 8)) % 2;
          float k = std::min(1.0f, (y - 262) / 10.0f);
          c = mix(c, chk ? 0x1A1614 : 0xBFB194, k * 0.85f);
        }
      }
      else if (rg == 1)
      {
        // くるみの木
        float gr = d2::vn(x * 0.9f, y * 0.04f), fine = d2::h2(x, y / 3) * 0.08f;
        if (y < YT + 2 || (x > 20 && x < 222 && y < 40))
          gr = d2::vn(x * 0.04f, y * 0.9f), fine = d2::h2(x / 3, y) * 0.08f;
        c = mix(0x3A1A0A, 0x6A3618, gr * 0.9f + fine);
      }
      else if (rg == 2)
        c = mix(0x2A1810, 0x4A2C1A, 0.5f + 0.5f * sinf((x - XD) * 0.4f));
      else
        c = ((x + y) / 4) % 2 ? 0xB0202A : 0xC83038; // エプロン: 赤い縞
      d2::P(x, y, c);
    }
  // 50 年代の模様: 青緑のブーメランと、赤い星のきらめき
  boomerang(40, 64, 22, 0.2f, TEAL, 120);
  boomerang(182, 64, 22, 1.2f, TEAL, 120);
  boomerang(111, 116, 40, 0.6f, TEAL, 70);
  boomerang(60, 190, 18, 4.0f, TEAL, 110);
  boomerang(162, 190, 18, 4.5f, TEAL, 110);
  for (auto p : {Pt{30, 60}, Pt{192, 76}, Pt{111, 92}, Pt{40, 178}, Pt{182, 178}, Pt{86, 160}, Pt{136, 160}})
  {
    for (int i = 0; i < 8; i++)
    {
      float a = i * PI_F / 4, r = i % 2 ? 4 : 7;
      d2::L(p.x, p.y, p.x + cosf(a) * r, p.y + sinf(a) * r, RED, 170);
    }
    d2::C(p.x, p.y, 1.5f, RED);
  }
  // 名前の看板: 赤い丸みの帯にクリームの字
  {
    float y0 = 236, y1 = 256;
    d2::R(64, y0, 94, y1 - y0, DKRED);
    d2::C(64, (y0 + y1) / 2, 10, DKRED);
    d2::C(158, (y0 + y1) / 2, 10, DKRED);
    d2::R(64, y0 + 1, 94, y1 - y0 - 2, RED);
    d2::C(64, (y0 + y1) / 2, 9, RED);
    d2::C(158, (y0 + y1) / 2, 9, RED);
    bgText(112, y0 + 3, "JUKEBOX", 0x5A0A10, 2);
    bgText(111, y0 + 2, "JUKEBOX", CREAM, 2);
  }
  // 壁: クロームの手すり
  for (int i = 0; i < nW; i++)
  {
    const Wall &w = walls[i];
    if (w.kind == K_SLING0 || w.kind == K_SLING1)
      continue;
    d2::LT(w.x0 + 1, w.y0 + 1.5f, w.x1 + 1, w.y1 + 1.5f, 3, 0x000000, 50);
  }
  for (int i = 0; i < nW; i++)
  {
    const Wall &w = walls[i];
    if (w.kind == K_SLING0 || w.kind == K_SLING1)
      continue;
    d2::LT(w.x0, w.y0, w.x1, w.y1, 3, 0x9098A8);
  }
  for (int i = 0; i < nW; i++)
  {
    const Wall &w = walls[i];
    if (w.kind == K_SLING0 || w.kind == K_SLING1)
      continue;
    d2::L(w.x0, w.y0, w.x1, w.y1, 0xF0F4FF);
  }
  for (int i = 0; i < 4; i++)
  {
    d2::C(POST_X[i], POST_Y0, 2.5f, 0xE0E4EC);
    d2::C(POST_X[i], POST_Y1, 2.5f, 0xE0E4EC);
  }
  // レーンの明かりの台と数字
  for (int i = 0; i < 3; i++)
  {
    float lx = (POST_X[i] + POST_X[i + 1]) / 2;
    d2::C(lx, LAMP_Y, 6, 0x6A1A1A);
    d2::CR(lx, LAMP_Y, 6, 0x9098A8);
  }
  // 壁の的の台
  d2::R(XL + 1, TG_Y0 + 2, 5, TG_Y1 - TG_Y0 - 4, 0x5A5A60);
  d2::R(XD - 5, TG_Y0 + 2, 5, TG_Y1 - TG_Y0 - 4, 0x5A5A60);
  // スリングショット: 赤い板と白いゴム
  for (int sd = 0; sd < 2; sd++)
  {
    float ax, ay, bx, by, cx, cy;
    slingPts(sd, ax, ay, bx, by, cx, cy);
    Pt tri[3] = {{ax, ay}, {bx, by}, {cx, cy}};
    d2::poly(tri, 3, DKRED);
    float gx = (ax + bx + cx) / 3, gy = (ay + by + cy) / 3;
    Pt in[3] = {{gx + (ax - gx) * 0.6f, gy + (ay - gy) * 0.6f}, {gx + (bx - gx) * 0.6f, gy + (by - gy) * 0.6f}, {gx + (cx - gx) * 0.6f, gy + (cy - gy) * 0.6f}};
    d2::poly(in, 3, RED);
    star(gx, gy + 1, 3.5f, CREAM);
    d2::LT(ax, ay, bx, by, 2, 0xE8E8E8);
    d2::LT(bx, by, cx, cy, 2, 0xE8E8E8);
    d2::LT(ax, ay, cx, cy, 3, 0xFFFFFF);
    d2::C(ax, ay, 2.5f, 0xE0E4EC);
    d2::C(bx, by, 2.5f, 0xE0E4EC);
    d2::C(cx, cy, 2.5f, 0xE0E4EC);
  }
  // ポップバンパー: クロームの輪、赤い胴、クリームの帽子に赤い星
  for (int k = 0; k < 3; k++)
  {
    d2::C(BX[k] + 1.5f, BY[k] + 2.5f, BUMP_R + 1, 0x000000, 70);
    d2::C(BX[k], BY[k], BUMP_R + 0.5f, 0xB8BCC8);
    d2::CR(BX[k], BY[k], BUMP_R + 0.5f, 0x606878);
    d2::C(BX[k], BY[k], BUMP_R - 1.5f, k == 2 ? TEAL : RED);
    dome(BX[k], BY[k], 6.5f, 0xE0D0B0);
    star(BX[k], BY[k], 5, k == 2 ? TEAL : RED);
  }
  // キックアウトの穴: クロームの縁と黒い穴、上に赤い矢じるし
  d2::C(HX, HY, 8.5f, 0xB8BCC8);
  d2::C(HX, HY, 6.5f, 0x101010);
  d2::C(HX - 1, HY - 1, 4.5f, 0x000000);
  d2::T(HX, HY - 17, HX - 5, HY - 11, HX + 5, HY - 11, RED);
  // ボタン
  for (int i = 0; i < 4; i++)
  {
    d2::C(BTN_X[i], BTN_Y[i], 4.5f, 0x9098A8);
    d2::C(BTN_X[i], BTN_Y[i], 3, 0xB02028);
  }
  // 1〜5 の明かりの台
  for (int i = 0; i < 5; i++)
  {
    d2::C(NUM_X[i], NUM_Y, 7.5f, 0x5A1414);
    d2::CR(NUM_X[i], NUM_Y, 7.5f, 0x9098A8);
  }
  // ダブルボーナスと、シュートアゲインの明かりの台
  d2::R(73, DBL_Y - 6, 76, 12, 0x5A1414);
  d2::R(73, DBL_Y - 6, 76, 1, 0x9098A8);
  d2::R(73, DBL_Y + 5, 76, 1, 0x9098A8);
  d2::R(73, AGAIN_Y - 6, 76, 12, 0x5A1414);
  d2::R(73, AGAIN_Y - 6, 76, 1, 0x9098A8);
  d2::R(73, AGAIN_Y + 5, 76, 1, 0x9098A8);
  // インレーンの SPECIAL の明かりの台
  for (int sd = 0; sd < 2; sd++)
  {
    float x = sd ? mirX(28) : 28;
    d2::C(x, 228, 4, 0x5A1414);
    d2::CR(x, 228, 4, 0x9098A8);
  }
}

bool Game6::begin()
{
  seed = esp_random() | 1;
  buildWalls();
  bg = (uint16_t *)ps_malloc(W * H * 2);
  if (!bg)
    return false;
  buildTable();
  for (int k = 0; k < NF; k++)
    fl[k] = Flip{FD[k].rest, FD[k].rest, false};
  startDemo();
  return true;
}

// ---- 流れ ----
void Game6::serve()
{
  bx = LANE_X;
  by = PLY - BALL_R - 0.5f;
  bvx = bvy = 0;
  st = BallSt::Plunger;
  slowT = holdT = holeCool = 0;
  tilted = false;
  tiltW = 0;
  mult = 1;
  bonus = 0;
  special = false;
  aiT = 0;
  aiP = 0.75f + rnd() * 0.25f;
  plMax = 0;
}

void Game6::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  score = 0;
  ballNo = 1;
  extra = 0;
  for (int i = 0; i < 5; i++)
    num[i] = false;
  serve();
}

void Game6::newGame()
{
  startDemo();
  mode = Mode::Play;
  msgT = 0;
  autoPlay = false;
  cheated = false;
  sfx(Sfx::PinStart);
}

void Game6::debugDrain()
{
  if (st == BallSt::Live || st == BallSt::Plunger)
  {
    cheated = true;
    bx = 111;
    by = DRAIN_Y + 1;
    lost();
  }
}

// 点数を足す。古い台のように、点の大きさでチャイムを鳴らす
void Game6::add(int pts, int bon)
{
  if (tilted)
    return;
  score += pts;
  bonus += bon;
  if (pts >= 1000)
    sfx(Sfx::Chime1000);
  else if (pts >= 100)
    sfx(Sfx::Chime100);
  else if (pts > 0)
    sfx(Sfx::Chime10);
}

void Game6::message(int kind, int val, int frames)
{
  msgKind = kind;
  msgVal = val;
  msgT = frames;
}

// 1〜5 の数字を点ける。全部点いたら SPECIAL
void Game6::lightNum(int i)
{
  if (numFlashT > 0)
  {
    add(100, 0);
    return;
  }
  if (num[i])
  {
    add(100, 0);
    return;
  }
  num[i] = true;
  add(1000, 1000);
  if (num[0] && num[1] && num[2] && num[3] && num[4] && !tilted)
  {
    numFlashT = 40;
    special = true;
    add(5000, 2000);
    message(3);
    sfx(Sfx::Jackpot, 0.7f);
  }
}

// ---- 動き ----
void Game6::collideSeg(float x0, float y0, float x1, float y1, float e, int kind)
{
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
  float u = clampf(((bx - x0) * dx + (by - y0) * dy) / l2, 0, 1);
  float qx = x0 + dx * u, qy = y0 + dy * u, ex = bx - qx, ey = by - qy, dd = ex * ex + ey * ey;
  if (dd >= RB * RB)
    return;
  float len = sqrtf(l2), d = sqrtf(dd), nx, ny;
  if (d > 1e-4f)
    nx = ex / d, ny = ey / d;
  else
    nx = dy / len, ny = -dx / len;
  if (kind == K_GATE)
  {
    float ux = dy / len, uy = -dx / len;
    if ((bx - x0) * ux + (by - y0) * uy < 0 || bvx * ux + bvy * uy >= 0)
      return;
  }
  bx = qx + nx * RB;
  by = qy + ny * RB;
  float vn = bvx * nx + bvy * ny;
  if (vn >= 0)
    return;
  bvx -= (1 + e) * vn * nx;
  bvy -= (1 + e) * vn * ny;
  float imp = -vn;
  if ((kind == K_SLING0 || kind == K_SLING1) && imp > 20)
  {
    int s = kind - K_SLING0;
    if (slingT[s] <= 0)
    {
      bvx += nx * SLING_KICK;
      bvy += ny * SLING_KICK;
      slingT[s] = 5;
      add(10, 10);
      sfx(Sfx::Sling, 0.8f);
    }
  }
  else if ((kind == K_TGT0 || (kind == K_TGT1 && bx < XD)) && imp > 30)
  {
    int s = kind - K_TGT0;
    if (tgtT[s] <= 0)
    {
      tgtT[s] = 8;
      sfx(Sfx::DropT, 0.7f);
      lightNum(3 + s);
    }
  }
}

void Game6::collideFlipper(int k, float dt)
{
  const Flip &f = fl[k];
  const FDef &D = FD[k];
  float px = D.px, py = D.py, dx = D.len * cosf(f.a), dy = D.len * sinf(f.a);
  float u = clampf(((bx - px) * dx + (by - py) * dy) / (D.len * D.len), 0, 1);
  float qx = px + dx * u, qy = py + dy * u, ex = bx - qx, ey = by - qy, dd = ex * ex + ey * ey;
  float rad = D.r0 + (D.r1 - D.r0) * u, lim = rad + BALL_R;
  if (dd >= lim * lim || dd < 1e-8f)
    return;
  float d = sqrtf(dd), nx = ex / d, ny = ey / d;
  bx = qx + nx * lim;
  by = qy + ny * lim;
  float w = (f.a - f.prevA) / dt;
  float rx = qx + nx * rad - px, ry = qy + ny * rad - py;
  float svx = -w * ry, svy = w * rx;
  float vn = (bvx - svx) * nx + (bvy - svy) * ny;
  if (vn < 0)
  {
    bvx -= (1 + E_FLIP) * vn * nx;
    bvy -= (1 + E_FLIP) * vn * ny;
  }
}

void Game6::sensors(float px, float py)
{
  if (py < LANE_SENSE_Y && by >= LANE_SENSE_Y)
    for (int i = 0; i < 3; i++)
      if (bx > POST_X[i] && bx < POST_X[i + 1])
        lightNum(i);
  // ボタン
  for (int i = 0; i < 4; i++)
    if (btnT[i] <= 0 && hypotf(bx - BTN_X[i], by - BTN_Y[i]) < 6)
    {
      btnT[i] = 10;
      add(100, 100);
    }
  if (py < INLANE_Y && by >= INLANE_Y)
  {
    if ((bx > 20 && bx < SL_AX) || (bx > mirX(SL_AX) && bx < mirX(20)))
    {
      if (special && !tilted)
      {
        special = false;
        extra++;
        message(4, 0, 3 * FPS);
        sfx(Sfx::Knocker);
        for (int i = 0; i < 5; i++)
          num[i] = false;
      }
      else
        add(1000, 100);
    }
    else if (bx < 20 || (bx > mirX(20) && bx < XD))
      add(1000, 500);
  }
}

void Game6::physics()
{
  const int SUB = 10;
  const float dt = 1.0f / FPS / SUB;
  for (int sstep = 0; sstep < SUB; sstep++)
  {
    for (int k = 0; k < NF; k++)
    {
      Flip &f = fl[k];
      f.prevA = f.a;
      float target = f.up ? FD[k].up : FD[k].rest, sp = (f.up ? W_UP : W_DOWN) * dt;
      if (f.a < target)
        f.a = std::min(target, f.a + sp);
      else
        f.a = std::max(target, f.a - sp);
    }
    if (st == BallSt::Hold || st == BallSt::Bonus)
      continue;
    float px = bx, py = by;
    bvy += GRAV * dt;
    float v = hypotf(bvx, bvy);
    if (v > VMAX)
    {
      bvx *= VMAX / v;
      bvy *= VMAX / v;
    }
    bx += bvx * dt;
    by += bvy * dt;
    for (int i = 0; i < nW; i++)
      collideSeg(walls[i].x0, walls[i].y0, walls[i].x1, walls[i].y1, walls[i].e, walls[i].kind);
    float ply = PLY + plP * PL_TRAVEL;
    if (bx > XD && by > ply - BALL_R && by < ply + 12)
    {
      by = ply - BALL_R;
      if (bvy > 0)
        bvy = bvy > 40 ? -bvy * 0.2f : 0;
    }
    for (int k = 0; k < 3; k++)
    {
      float ex = bx - BX[k], ey = by - BY[k], dd = ex * ex + ey * ey, R = BUMP_R + BALL_R;
      if (dd >= R * R || dd < 1e-6f)
        continue;
      float d = sqrtf(dd), nx = ex / d, ny = ey / d;
      bx = BX[k] + nx * R;
      by = BY[k] + ny * R;
      float vn = bvx * nx + bvy * ny;
      if (vn < 0)
      {
        bvx -= 1.5f * vn * nx;
        bvy -= 1.5f * vn * ny;
      }
      if (vn < 0 || bumpT[k] <= 0)
      {
        bvx += nx * BUMP_KICK;
        bvy += ny * BUMP_KICK;
        if (bumpT[k] <= 0)
        {
          add(100, 10);
          sfx(Sfx::Bumper, 0.8f);
        }
        bumpT[k] = 4;
      }
    }
    for (int k = 0; k < NF; k++)
      collideFlipper(k, dt);
    // キックアウトの穴
    if (holeCool <= 0)
    {
      float hx = bx - HX, hy = by - HY, hd = hypotf(hx, hy), sp = hypotf(bvx, bvy);
      if (hd < 5 && sp < 480)
      {
        st = BallSt::Hold;
        holdT = 0;
        bx = HX;
        by = HY;
        bvx = bvy = 0;
        sfx(Sfx::Kickout);
        if (mult >= 2)
        {
          add(5000, 1000);
          message(2, 5000);
        }
        else
        {
          mult = 2;
          add(3000, 1000);
          message(1);
        }
        continue;
      }
      if (hd < 13 && hd > 0.5f && sp < 320)
      {
        bvx -= hx / hd * 600 * dt;
        bvy -= hy / hd * 600 * dt;
      }
    }
    sensors(px, py);
    if (st == BallSt::Plunger && bx < XD - 2)
      st = BallSt::Live;
    if (by > DRAIN_Y)
    {
      lost();
      return;
    }
  }
}

void Game6::lost()
{
  st = BallSt::Bonus;
  bonusT = 0;
  bonusTotal = tilted ? 0 : bonus * mult;
  bonusShown = 0;
  bonusDone = 25;
  for (int k = 0; k < NF; k++)
    fl[k].up = false;
}

// ボーナスを 1000 ずつ数えて足す (チャイムが鳴る)
void Game6::stepBonus()
{
  bonusT++;
  if (bonusT < 25)
    return;
  if (bonusShown < bonusTotal)
  {
    if (bonusT % 3)
      return;
    int d = std::min(1000, bonusTotal - bonusShown);
    bonusShown += d;
    score += d;
    sfx(Sfx::Chime1000, 0.7f);
    bonusDone = bonusT;
    return;
  }
  if (bonusT - bonusDone < 40)
    return;
  if (extra > 0)
  {
    extra--;
    message(5, 0, 2 * FPS);
    serve();
    return;
  }
  ballNo++;
  if (ballNo > 3)
  {
    if (mode == Mode::Play)
    {
      mode = Mode::Over;
      modeT = 0;
      sfx(Sfx::GameOver);
    }
    else
      startDemo();
    return;
  }
  serve();
}

void Game6::stepPlunger(float pull)
{
  if (plShootT > 0)
  {
    plShootT--;
    plP *= 0.25f;
  }
  else
    plP += (pull - plP) * 0.6f;
  if (pull >= plMax - 0.02f || fabsf(pull - prevPull) < 0.06f)
  {
    plMax = pull;
    plAge = 0;
  }
  else
    plAge++;
  if (pull < 0.08f)
  {
    if (plAge <= 3 && plMax >= 0.15f)
    {
      float p = plMax;
      plShootT = 3;
      float ply = PLY + plP * PL_TRAVEL;
      if (bx > XD && by > ply - BALL_R - 6 && fabsf(bvy) < 80)
      {
        bvy = -(200 + 600 * p);
        sfx(Sfx::Launch, 0.4f + 0.6f * p);
      }
    }
    plMax = 0;
    plAge = 0;
  }
  prevPull = pull;
}

// 自動で遊ぶ: 左右それぞれ、下か真ん中のフリッパーの上にボールが来たら打つ
void Game6::stepAI(bool &l, bool &r, float &pull)
{
  l = r = false;
  pull = 0;
  if (bx > XD && by > 270 && (aiT > 0 || fabsf(bvy) < 30) && st != BallSt::Bonus) // 引きはじめたら、棒といっしょにボールが下がっても続ける
  {
    aiT++;
    if (aiT > 20 && aiT < 46)
      pull = aiP * std::min(1.0f, (aiT - 20) / 18.0f);
    if (aiT >= 46)
      aiT = 0;
  }
  else
    aiT = 0;
  for (int side = 0; side < 2; side++)
  {
    bool near = false;
    for (int k = side; k < NF; k += 2)
    {
      const FDef &D = FD[k];
      float lx = side ? D.px - bx : bx - D.px, ly = by - D.py;
      if (st == BallSt::Live && lx > 3 && lx < D.len + 4 && ly > -22 && ly < 10 && bvy > -40)
        near = true;
    }
    if (aiFlipT[side] > 0)
      aiFlipT[side]--;
    else if (aiFlipT[side] < 0)
      aiFlipT[side]++;
    else if (near)
      aiFlipT[side] = 7;
    if (aiFlipT[side] == 1)
      aiFlipT[side] = -5;
    if (aiFlipT[side] > 0)
      (side ? r : l) = true;
  }
}

void Game6::stepBall(const Input *in)
{
  bool l = false, r = false;
  float pull = 0;
  if (in)
  {
    l = in->Fire;
    r = in->Bomb;
    pull = std::max(0.0f, in->AY);
    bool fwd = in->AY < -0.55f;
    if (fwd && !fwdWas && st == BallSt::Live)
    {
      bvy -= 90;
      bvx += in->AX * 60;
      shakeT = 6;
      if (!tilted)
      {
        tiltW += 1;
        if (tiltW >= 3)
        {
          tilted = true;
          message(7, 0, 3 * FPS);
          sfx(Sfx::Tilt);
        }
        else if (tiltW >= 2)
        {
          message(6, 0, FPS);
          sfx(Sfx::Warning, 0.6f);
        }
      }
    }
    fwdWas = fwd;
  }
  else
    stepAI(l, r, pull);
  if (tilted || st == BallSt::Bonus)
    l = r = false;
  if (l && !fl[0].up)
    sfx(Sfx::Flipper, 0.7f);
  if (r && !fl[1].up)
    sfx(Sfx::Flipper, 0.7f);
  for (int k = 0; k < NF; k++)
    fl[k].up = (k & 1) ? r : l;
  stepPlunger(pull);

  for (int k = 0; k < 3; k++)
    if (bumpT[k] > 0)
      bumpT[k]--;
  for (int k = 0; k < 2; k++)
  {
    if (slingT[k] > 0)
      slingT[k]--;
    if (tgtT[k] > 0)
      tgtT[k]--;
  }
  for (int k = 0; k < 4; k++)
    if (btnT[k] > 0)
      btnT[k]--;
  if (numFlashT > 0 && --numFlashT == 0)
    for (int i = 0; i < 5; i++)
      num[i] = false;
  if (holeCool > 0)
    holeCool--;
  if (shakeT > 0)
    shakeT--;
  if (msgT > 0)
    msgT--;
  tiltW = std::max(0.0f, tiltW - 0.6f / FPS);

  if (st == BallSt::Bonus)
  {
    physics();
    stepBonus();
    return;
  }
  if (st == BallSt::Hold)
  {
    if (++holdT >= HOLD_N)
    {
      // 上へ蹴り出す (バンパーのほうへ、少し左右に振る)
      st = BallSt::Live;
      bvx = (rnd() < 0.5f ? -1 : 1) * (60 + rnd() * 90);
      bvy = -380;
      holeCool = 20;
      sfx(Sfx::Kickout);
    }
    physics();
    return;
  }
  physics();
  if (st == BallSt::Live && hypotf(bvx, bvy) < 8 && by < 258)
  {
    if (++slowT > 2 * FPS)
    {
      bvy = -150;
      bvx = (rnd() - 0.5f) * 120;
      slowT = 0;
    }
  }
  else
    slowT = 0;
}

// ---- 名前とランキング ----
int Game6::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

void Game6::stepEntry(const Input &in)
{
  if (--modeT <= 0)
  {
    finishEntry();
    return;
  }
  int dir = 0;
  if (in.UpPressed)
    dir = 1;
  else if (in.DownPressed)
    dir = -1;
  else if (in.Up || in.Down)
  {
    if (++entryRepeat > 8 && entryRepeat % 3 == 0)
      dir = in.Up ? 1 : -1;
  }
  if (!in.Up && !in.Down)
    entryRepeat = 0;
  if (dir != 0)
  {
    sfx(Sfx::Select);
    int n = strlen(LETTERS), k = strchr(LETTERS, entry[entryPos]) - LETTERS;
    entry[entryPos] = LETTERS[(k + dir + n) % n];
  }
  if (in.LeftPressed || in.BackPressed)
    entryPos = std::max(0, entryPos - 1);
  if (in.RightPressed && entryPos < 2)
    entryPos++;
  if (in.FirePressed)
  {
    sfx(Sfx::Select);
    if (entryPos < 2)
      entryPos++;
    else
      finishEntry();
  }
}

void Game6::finishEntry()
{
  char name[4];
  memcpy(name, entry, 4);
  for (int i = 2; i >= 0 && name[i] == ' '; i--)
    name[i] = 0;
  if (!name[0])
    strcpy(name, "---");
  memcpy(LastName, entry, 4);
  int i = rankOf(score);
  if (i >= 0)
  {
    for (int j = 4; j > i; j--)
      Table[j] = Table[j - 1];
    Table[i].Score = score;
    memcpy(Table[i].Name, name, 4);
    newRank = i;
    if (TableChanged)
      TableChanged();
  }
  mode = Mode::Ranking;
  modeT = 8 * FPS;
}

// ---- 1 フレーム ----
void Game6::Step(const Input &in)
{
  sndN = 0;
  if (in.VolumePressed && !(mode == Mode::Play && paused))
  {
    VolumeLevel = (VolumeLevel + 1) % 5;
    VolumeChanged = true;
    volT = 2 * FPS;
  }
  if (volT > 0)
    volT--;
  t++;
  // 点数のリール: 1 桁ずつ回って、いまの点数に追いつく
  {
    int sc = score;
    for (int i = 6; i >= 0; i--)
    {
      int d = sc % 10;
      sc /= 10;
      if ((int)reel[i] != d)
      {
        reel[i] += 0.5f;
        if (reel[i] >= 10)
          reel[i] -= 10;
      }
      else
        reel[i] = (float)d;
    }
  }
  switch (mode)
  {
  case Mode::Attract:
    if (in.PausePressed)
    {
      WantMenu = true;
      return;
    }
    if (in.FirePressed || in.BombPressed || in.StartPressed)
    {
      newGame();
      return;
    }
    modeT++;
    stepBall(nullptr);
    break;
  case Mode::Play:
    if (in.PausePressed || (paused && in.VolumePressed))
    {
      paused = !paused;
      sfx(Sfx::Pause);
    }
    if (paused)
    {
      if (in.QuitPressed)
        startDemo();
      return;
    }
    modeT++;
    stepBall(autoPlay ? nullptr : &in);
    if (autoPlay)
      cheated = true;
    break;
  case Mode::Over:
    modeT++;
    if ((modeT > 2 * FPS && in.FirePressed) || modeT > 5 * FPS)
    {
      if (!cheated && rankOf(score) >= 0)
      {
        mode = Mode::NameEntry;
        modeT = 40 * FPS;
        for (int i = 0; i < 3; i++)
          entry[i] = LastName[i] && strchr(LETTERS, LastName[i]) ? LastName[i] : 'A';
        entry[3] = 0;
        entryPos = 0;
      }
      else
      {
        mode = Mode::Ranking;
        newRank = -1;
        modeT = 6 * FPS;
      }
    }
    break;
  case Mode::NameEntry:
    stepEntry(in);
    break;
  case Mode::Ranking:
    if (--modeT <= 0 || (modeT < 7 * FPS && (in.FirePressed || in.StartPressed)))
      startDemo();
    break;
  }
}

// ---- 描く ----
// 銀のボール: 上はクリームの照明、下は台の色を映す
static void drawBall(float cx, float cy, float r)
{
  Surf *s = d2::S;
  cy += s->oy;
  int y0 = std::max((int)floorf(cy - r - 1), s->y0), y1 = std::min((int)ceilf(cy + r + 1), s->y0 + s->h - 1);
  int x0 = std::max((int)floorf(cx - r - 1), 0), x1 = std::min((int)ceilf(cx + r + 1), W - 1);
  if (y0 > y1 || x0 > x1)
    return;
  float inv = 1 / r;
  for (int y = y0; y <= y1; y++)
  {
    uint16_t *row = s->buf + (y - s->y0) * s->w;
    float dy = y - cy;
    for (int x = x0; x <= x1; x++)
    {
      float dx = x - cx, dd = dx * dx + dy * dy;
      if (dd > (r + 0.5f) * (r + 0.5f))
        continue;
      float d = sqrtf(dd), cov = std::min(1.0f, r + 0.5f - d);
      float nx = dx * inv, ny = dy * inv, nz2 = 1 - nx * nx - ny * ny, nz = nz2 > 0 ? sqrtf(nz2) : 0;
      float env = clampf(0.5f - 0.75f * ny, 0, 1);
      uint32_t c = mix(0x403028, 0xFFF4E0, env);
      if (ny > 0.25f)
        c = mix(c, 0xC09060, (ny - 0.25f) * 0.8f);
      float lam = -0.5f * nx - 0.6f * ny + 0.62f * nz;
      if (lam > 0)
      {
        float l2 = lam * lam, l4 = l2 * l2, l8 = l4 * l4, l16 = l8 * l8;
        c = mix(c, 0xFFFFFF, l16 * 0.95f);
      }
      c = scale(c, 0.7f + 0.3f * nz);
      row[x] = cov >= 0.99f ? d2::c565(c) : d2::blend(row[x], c, (int)(cov * 255));
    }
  }
}

static void drawFlipper(const FDef &D, float a, bool lit)
{
  float dx = cosf(a), dy = sinf(a), nx = -dy, ny = dx;
  float px = D.px, py = D.py, tx = px + dx * D.len, ty = py + dy * D.len;
  Pt sh[4] = {{px + nx * D.r0 + 2, py + ny * D.r0 + 3}, {tx + nx * D.r1 + 2, ty + ny * D.r1 + 3}, {tx - nx * D.r1 + 2, ty - ny * D.r1 + 3},
              {px - nx * D.r0 + 2, py - ny * D.r0 + 3}};
  d2::poly(sh, 4, 0x000000, 80);
  Pt o[4] = {{px + nx * D.r0, py + ny * D.r0}, {tx + nx * D.r1, ty + ny * D.r1}, {tx - nx * D.r1, ty - ny * D.r1}, {px - nx * D.r0, py - ny * D.r0}};
  d2::poly(o, 4, RED);
  d2::C(px, py, D.r0, RED);
  d2::C(tx, ty, D.r1, RED);
  float r0 = D.r0 - 1.5f, r1 = D.r1 - 1.3f;
  Pt in[4] = {{px + nx * r0, py + ny * r0}, {tx + nx * r1, ty + ny * r1}, {tx - nx * r1, ty - ny * r1}, {px - nx * r0, py - ny * r0}};
  d2::poly(in, 4, lit ? 0xFFFFFF : 0xF4ECDC);
  d2::C(px, py, r0, 0xF4ECDC);
  d2::C(px, py, 1.6f, 0x9098A8);
}

static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1, int fx = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, fx, INK);
}
static void panel(int x, int y, int w, int h)
{
  d2::R(x, y, w, h, 0x200808, 248);
  d2::R(x, y, w, 2, RED);
  d2::R(x, y + h - 2, w, 2, RED);
  d2::R(x, y, 1, h, 0xB8BCC8);
  d2::R(x + w - 1, y, 1, h, 0xB8BCC8);
}

void Game6::drawDynamic(Gfx &g)
{
  bool blink = (t / 4) % 2;
  // レーンの明かり (1・2・3)
  for (int i = 0; i < 3; i++)
  {
    float lx = (POST_X[i] + POST_X[i + 1]) / 2;
    if (numFlashT > 0 ? blink : num[i])
    {
      d2::C(lx, LAMP_Y, 9, 0xFFC840, 70);
      d2::C(lx, LAMP_Y, 5.5f, 0xFFD860);
    }
  }
  // 1〜5 の明かり
  for (int i = 0; i < 5; i++)
    if (numFlashT > 0 ? blink : num[i])
    {
      d2::C(NUM_X[i], NUM_Y, 11, 0xFFC840, 60);
      d2::C(NUM_X[i], NUM_Y, 7, 0xFFD860);
    }
  // 壁の的 (4・5)
  for (int s = 0; s < 2; s++)
  {
    float x = s ? XD - 5 : XL + 1;
    bool on = numFlashT > 0 ? blink : num[3 + s];
    uint32_t c = tgtT[s] > 0 ? 0xFFFFFF : on ? 0xFFD860 : 0xC8BC9C;
    d2::R(x, TG_Y0 + 3, 5, TG_Y1 - TG_Y0 - 6, c);
    d2::R(s ? x : x + 4, TG_Y0 + 3, 1, TG_Y1 - TG_Y0 - 6, RED);
  }
  // ダブルボーナス・シュートアゲイン・SPECIAL
  if (mult >= 2)
    d2::R(74, DBL_Y - 5, 74, 10, 0xFFC840);
  if (extra > 0)
    d2::R(74, AGAIN_Y - 5, 74, 10, 0xFFC840);
  if (special && blink)
    for (int sd = 0; sd < 2; sd++)
    {
      float x = sd ? mirX(28) : 28;
      d2::C(x, 228, 8, 0xFFC840, 70);
      d2::C(x, 228, 3.5f, 0xFFE070);
    }
  // ボタン
  for (int i = 0; i < 4; i++)
    if (btnT[i] > 0)
    {
      d2::C(BTN_X[i], BTN_Y[i], 3, 0xFFE070);
      d2::C(BTN_X[i], BTN_Y[i], 7, 0xFFC840, 70);
    }
  // バンパー
  for (int k = 0; k < 3; k++)
    if (bumpT[k] > 0)
    {
      d2::C(BX[k], BY[k], 6.5f, 0xFFF080, 170);
      d2::CR(BX[k], BY[k], BUMP_R + 1.5f, 0xFFFFFF);
      d2::C(BX[k], BY[k], BUMP_R + 5, 0xFFE890, 70);
    }
  for (int sd = 0; sd < 2; sd++)
    if (slingT[sd] > 0)
    {
      float ax, ay, bx2, by2, cx, cy;
      slingPts(sd, ax, ay, bx2, by2, cx, cy);
      Pt tri[3] = {{ax, ay}, {bx2, by2}, {cx, cy}};
      d2::poly(tri, 3, 0xFFC840, 140);
      d2::LT(ax, ay, cx, cy, 3, 0xFFFFFF);
    }
  // 明かりの字
  static const char *LN[3] = {"1", "2", "3"};
  for (int i = 0; i < 3; i++)
  {
    float lx = (POST_X[i] + POST_X[i + 1]) / 2;
    bool on = num[i] && numFlashT == 0;
    txt(g, (int)lx + 1, (int)LAMP_Y - 3, on ? 0x401010 : 0xF0C0A0, LN[i], true, 1, 0);
  }
  static const char *NN[5] = {"1", "2", "3", "4", "5"};
  for (int i = 0; i < 5; i++)
  {
    bool on = num[i] && numFlashT == 0;
    txt(g, (int)NUM_X[i] + 1, (int)NUM_Y - 3, on ? 0x401010 : 0xF0C0A0, NN[i], true, 1, 0);
  }
  if (shakeT == 0)
  {
    txt(g, 13, (int)TG_Y0 + 9, 0x7A1218, "4", false, 1, 0);
    txt(g, (int)XD - 12, (int)TG_Y0 + 9, 0x7A1218, "5", false, 1, 0);
  }
  txt(g, 111, (int)DBL_Y - 3, mult >= 2 ? 0x401010 : 0xF0C0A0, "DOUBLE BONUS", true, 1, 0);
  txt(g, 111, (int)AGAIN_Y - 3, extra > 0 ? 0x401010 : 0xF0C0A0, "SHOOT AGAIN", true, 1, 0);
  // 打ち出す棒とバネ
  {
    float ply = PLY + plP * PL_TRAVEL;
    for (float y = ply + 4; y < 319; y += 2.5f + (319 - ply) * 0.02f)
      d2::L(XD + 3, y, XO - 3, y + 1.5f, 0xA0A8B8);
    d2::R(LANE_X - 1, ply, 3, 320 - ply, 0xD0D4E0);
    d2::R(XD + 2, ply, XO - XD - 3, 4, RED);
    d2::R(XD + 2, ply, XO - XD - 3, 1, 0xFF9090);
  }
  for (int k = 0; k < NF; k++)
    drawFlipper(FD[k], fl[k].a, fl[k].up);
  if (st != BallSt::Bonus || by < DRAIN_Y)
  {
    if (st == BallSt::Hold)
      drawBall(bx, by, BALL_R * 0.8f);
    else
    {
      d2::C(bx + 2, by + 3, BALL_R, 0x000000, 70);
      drawBall(bx, by, BALL_R);
    }
  }
}

void Game6::drawHud(Gfx &g)
{
  char s[24];
  // 点数のリール: クリームの窓に黒い数字
  for (int i = 0; i < 7; i++)
  {
    int x = 4 + i * 15;
    d2::R(x, 2, 13, 18, 0x000000);
    for (int y = 0; y < 16; y++)
    {
      float k = fabsf(y - 7.5f) / 8;
      d2::R(x + 1, 3 + y, 11, 1, mix(0xFFF8E8, 0x9A8C70, k * k));
    }
    int dg = ((int)reel[i]) % 10;
    char c[2] = {(char)(dg == 0 ? 'O' : '0' + dg), 0}; // この字の 0 には斜線があるので、O で出す
    txt(g, x + 1, 3, 0x181010, c, false, 3, 0);
  }
  snprintf(s, sizeof(s), "BALL %d", std::min(ballNo, 3));
  txt(g, W - 4 - (int)strlen(s) * 6, 3, CREAM, s);
  if (extra > 0)
    txt(g, W - 4 - 4 * 6, 12, GOLD, "+1 B");
  else if (mult > 1)
    txt(g, W - 4 - 2 * 6, 12, GOLD, "x2");
}

void Game6::drawOverlay(Gfx &g)
{
  char s[40];
  if (volT > 0)
  {
    panel(70, 286, 100, 22);
    txt(g, 78, 294, CREAM, VolumeLevel == 0 ? "VOL OFF" : "VOL");
    for (int i = 0; i < 4; i++)
      d2::R(110 + i * 13, 294, 10, 8, i < VolumeLevel ? GOLD : 0x402020);
  }
  if (msgT > 0 && (mode == Mode::Play || mode == Mode::Attract))
  {
    int y = 170;
    switch (msgKind)
    {
    case 1:
      txt(g, 111, y, RED, "DOUBLE BONUS", true, 3);
      break;
    case 2:
      txt(g, 111, y, RED, "KICKOUT 5000", true, 3);
      break;
    case 3:
      txt(g, 111, y, RED, "SPECIAL LIT", true, 3);
      txt(g, 111, y + 16, 0x401010, "ROLL AN INLANE", true);
      break;
    case 4:
      txt(g, 111, y, RED, "EXTRA BALL", true, 3);
      break;
    case 5:
      txt(g, 111, y, RED, "SHOOT AGAIN", true, 3);
      break;
    case 6:
      txt(g, 111, y, RED, "DANGER", true, 3);
      break;
    case 7:
      txt(g, 111, y, RED, "TILT", true, 5);
      break;
    }
  }
  if (st == BallSt::Bonus && (mode == Mode::Play || mode == Mode::Attract) && bonusT > 5)
  {
    panel(36, 128, 150, 66);
    txt(g, 111, 137, GOLD, tilted ? "TILT" : "BONUS", true, 3);
    snprintf(s, sizeof(s), mult > 1 ? "%d x 2" : "%d", tilted ? 0 : bonus);
    txt(g, 111, 160, CREAM, s, true);
    snprintf(s, sizeof(s), "%d", bonusShown);
    txt(g, 111, 174, GOLD, s, true);
  }
  switch (mode)
  {
  case Mode::Attract:
  {
    int ph = (modeT / (8 * FPS)) % 3;
    if (ph == 0)
    {
      panel(18, 84, 186, 126);
      txt(g, 111, 96, RED, "JUKEBOX", true, 5);
      txt(g, 111, 126, CREAM, "DINER PINBALL", true);
      txt(g, 111, 138, MUTE, "3 BALLS - 5 CENTS", true);
      if ((modeT / 15) % 2 == 0)
        txt(g, 111, 158, GOLD, "PUSH SW1", true, 3);
      txt(g, 111, 192, MUTE, "SW3 HOLD: MENU", true);
    }
    else if (ph == 1)
    {
      panel(14, 84, 194, 128);
      txt(g, 111, 94, GOLD, "HOW TO PLAY", true, 3);
      txt(g, 111, 118, CREAM, "SW1 / SW2: FLIPPERS", true);
      txt(g, 111, 132, CREAM, "PULL STICK & LET GO:", true);
      txt(g, 111, 142, CREAM, "LAUNCH", true);
      txt(g, 111, 156, CREAM, "PUSH STICK: NUDGE", true);
      txt(g, 111, 172, MUTE, "LIGHT 1-5 = SPECIAL", true);
      txt(g, 111, 184, MUTE, "SPECIAL INLANE = EXTRA", true);
      txt(g, 111, 196, MUTE, "KICKOUT = DOUBLE BONUS", true);
    }
    else
    {
      panel(18, 80, 186, 140);
      txt(g, 111, 94, GOLD, "HIGH SCORES", true, 3);
      static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
      for (int i = 0; i < 5; i++)
      {
        snprintf(s, sizeof(s), "%s %8d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
        txt(g, 111, 122 + i * 18, i == 0 ? GOLD : CREAM, s, true);
      }
    }
    break;
  }
  case Mode::Play:
    if (ballNo == 1 && score == 0 && st == BallSt::Plunger && plP < 0.05f && bx > XD)
    {
      panel(40, 196, 142, 30);
      txt(g, 111, 202, CREAM, "PULL THE STICK BACK", true);
      txt(g, 111, 214, CREAM, "AND LET GO", true);
    }
    if (autoPlay)
      txt(g, 8, 34, 0x401010, "AUTO", false, 1, 0);
    if (paused)
    {
      panel(30, 120, 162, 64);
      txt(g, 111, 132, CREAM, "PAUSE", true, 3);
      txt(g, 111, 158, MUTE, "SW3: RESUME", true);
      txt(g, 111, 170, MUTE, "SW2: QUIT", true);
    }
    break;
  case Mode::Over:
    panel(18, 110, 186, 80);
    txt(g, 111, 122, RED, "GAME OVER", true, 3);
    snprintf(s, sizeof(s), "%d", score);
    txt(g, 111, 148, CREAM, s, true, 3);
    if (cheated)
      txt(g, 111, 174, MUTE, "TEST PLAY (NO RECORD)", true);
    else if (score > Table[0].Score)
      txt(g, 111, 174, GOLD, "NEW HIGH SCORE!", true);
    break;
  case Mode::NameEntry:
  {
    panel(18, 90, 186, 120);
    txt(g, 111, 102, GOLD, "NEW RECORD", true, 3);
    snprintf(s, sizeof(s), "%d", score);
    txt(g, 111, 124, CREAM, s, true);
    for (int i = 0; i < 3; i++)
    {
      char c[2] = {entry[i], 0};
      int x = 111 - 22 + i * 22;
      txt(g, x, 146, i == entryPos ? GOLD : CREAM, c, true, 5);
      if (i == entryPos && (t / 8) % 2)
        d2::R(x - 8, 172, 16, 2, GOLD);
    }
    txt(g, 111, 186, MUTE, "STICK: LETTER  SW1: OK", true);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, 186, 100, MUTE, s);
    break;
  }
  case Mode::Ranking:
  {
    panel(18, 80, 186, 150);
    txt(g, 111, 94, GOLD, "HIGH SCORES", true, 3);
    static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
    for (int i = 0; i < 5; i++)
    {
      snprintf(s, sizeof(s), "%s %8d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
      bool mine = i == newRank;
      if (!mine || (t / 8) % 2)
        txt(g, 111, 122 + i * 18, mine ? RED : i == 0 ? GOLD : CREAM, s, true);
    }
    snprintf(s, sizeof(s), "YOU %d", score);
    txt(g, 111, 214, MUTE, s, true);
    break;
  }
  }
}

void Game6::Draw(Gfx &gx)
{
  uint32_t t0 = micros();
  int ltop = gx.top * 2 / 3;
  int sh = shakeT > 0 ? ((shakeT & 1) ? -2 : 2) : 0;
  for (int r = 0; r < LSTRIP; r++)
  {
    int y = ltop + r, sy = y;
    if (y >= HUD_H && sh)
      sy = std::min(H - 1, std::max(HUD_H, y - sh));
    memcpy(gx.buf + r * W, bg + sy * W, W * 2);
  }
  Surf s;
  s.buf = gx.buf;
  s.y0 = ltop;
  s.h = LSTRIP;
  s.oy = sh;
  s.cy0 = HUD_H;
  d2::S = &s;
  drawDynamic(gx);
  s.oy = 0;
  s.cy0 = -100000;
  drawHud(gx);
  drawOverlay(gx);
  profDraw += micros() - t0;
}
