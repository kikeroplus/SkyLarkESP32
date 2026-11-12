// 5 本目 (ORBIT): 宇宙のピンボール。遊びの流れは CORNERS (game4.cpp) にそろえる
#include "game5.h"
#include "g2draw.h"
#include <algorithm>

static const int W = 240, H = 320;
static const int FPS = Game5::FPS;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
static const float PI_F = 3.14159265f;

// ---- 台 (液晶の座標)。上の 22 行は点数などを出すところ ----
static const int HUD_H = 22;
static const float BALL_R = 5.0f;     // ボールの半径
static const float WR = 1.5f;     // 壁の太さの半分
static const float RB = BALL_R + WR;  // 壁の線からボールの中心まで
static const float GRAV = 420;    // 台の傾きで下へ引かれる (点/秒^2)
static const float VMAX = 1100;
static const float XL = 6, XD = 216, XO = 232, YT = 30; // 左の壁、打ち出すレーンとの仕切り、右の外の壁、上の壁
static const float ARC_R = 56, ARC_LX = 62, ARC_RX = 176, ARC_Y = 86; // 上の角の丸み
static const float GATE_Y0 = 112, GATE_Y1 = 100; // レーンの上の、一方通行の板
static const float PLY = 300, PL_TRAVEL = 14;    // 打ち出す棒の上面と、引いたときに下がる量
static const float LANE_X = 224;
static const float DRAIN_Y = 334;
// フリッパー (左。右は左右を写す)
static const float FPX[2] = {68, 154}, FPY = 282, FLEN = 38, FR0 = 5.5f, FR1 = 3.0f;
static const float A_REST = 0.49f, A_UP = -0.45f;
static const float W_UP = 13, W_DOWN = 8; // 回る速さ (ラジアン/秒)
static const float E_FLIP = 0.3f;
// バンパー (惑星): 0 = 赤い星、1 = 青い星、2 = 太陽
static const float BX[3] = {80, 142, 111}, BY[3] = {108, 108, 146}, BUMP_R = 11, BUMP_KICK = 240;
// ブラックホール
static const float HX = 186, HY = 160;
static const int HOLD_N = 45;
// 上のレーン
static const float POST_X[4] = {64, 95, 127, 158}, POST_Y0 = 44, POST_Y1 = 60, LANE_SENSE_Y = 54, LAMP_Y = 70;
// 彗星ターゲット (左の斜めの列)
static const float DR_X0 = 6, DR_Y0 = 130, DR_X1 = 28, DR_Y1 = 192;
static const float DR_S0[3] = {9, 27, 45}, DR_LEN = 14;
// スリングショット (左。右は写す)。A-C が蹴る面
static const float SL_AX = 36, SL_AY = 212, SL_BX = 36, SL_BY = 236, SL_CX = 58, SL_CY = 252;
static const float SLING_KICK = 230;
// インレーン・アウトレーンを通ったのを見る高さ
static const float INLANE_Y = 222;
// 中の明かり
static const float MULT_X[4] = {78, 100, 122, 144}, MULT_Y = 196;
static const float SAVE_Y = 268;

static const uint32_t INK = 0x000000, CREAM = 0xF4F6FF, GOLD = 0xFF9A30, REDT = 0xFF4A50, MUTE = 0xA8B8E0, CYAN = 0xFFD028; // CYAN は見出しと明かりの黄色

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float mirX(float x) { return 222 - x; } // 台の真ん中 (x = 111) で左右を写す

// ---- 壁 ----
enum : uint8_t
{
  K_WALL,
  K_SLING0,
  K_SLING1,
  K_DROP,
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
  addW(XL, ARC_Y, XL, 250, 0.45f);
  addW(XL, 250, 60, 318, 0.45f);
  addArc(ARC_LX, ARC_Y, ARC_R, PI_F, 1.5f * PI_F, 10, 0.45f);
  addW(ARC_LX, YT, ARC_RX, YT, 0.45f);
  addArc(ARC_RX, ARC_Y, ARC_R, 1.5f * PI_F, 2 * PI_F, 10, 0.45f);
  addW(XO, ARC_Y, XO, DRAIN_Y, 0.3f);
  addW(XD, GATE_Y0, XD, DRAIN_Y, 0.3f);
  addW(XD, GATE_Y0, XO, GATE_Y1, 0.3f, K_GATE);
  addW(XD, 250, mirX(60), 318, 0.45f);
  // インレーンの仕切り
  addW(20, 206, 20, 242, 0.4f);
  addW(20, 242, 62, 276, 0.4f);
  addW(mirX(20), 206, mirX(20), 242, 0.4f);
  addW(mirX(20), 242, mirX(62), 276, 0.4f);
  // スリングショット
  for (int s = 0; s < 2; s++)
  {
    float ax, ay, bx, by, cx, cy;
    slingPts(s, ax, ay, bx, by, cx, cy);
    addW(ax, ay, bx, by, 0.5f);
    addW(bx, by, cx, cy, 0.5f);
    addW(ax, ay, cx, cy, 0.6f, s ? K_SLING1 : K_SLING0);
  }
  // 上のレーンの仕切り
  for (int i = 0; i < 4; i++)
    addW(POST_X[i], POST_Y0, POST_X[i], POST_Y1, 0.5f);
  // 彗星ターゲットの列 (倒れても後ろの壁で返す)
  addW(DR_X0, DR_Y0, DR_X1, DR_Y1, 0.4f, K_DROP);
}

float Game5::rnd()
{
  seed = seed * 1664525u + 1013904223u;
  return (seed >> 8) * (1.0f / 16777216.0f);
}

void Game5::sfx(Sfx s, float vol)
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

// 0 = 台の面、1 = 外の枠、2 = 打ち出すレーン、3 = 下のエプロン、4 = ターゲットの裏 (届かない所)
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
  if (y > ay)
    return 3;
  if (y > DR_Y0 && y < DR_Y1 && x < DR_X0 + (y - DR_Y0) * (DR_X1 - DR_X0) / (DR_Y1 - DR_Y0))
    return 4;
  return 0;
}

// 球 (左上から光が当たる)。惑星のもようは band で
static void sphere(float cx, float cy, float r, uint32_t col, uint32_t col2, float band, float glow)
{
  Surf *s = d2::S;
  int y0 = std::max((int)floorf(cy - r - 1), s->y0), y1 = std::min((int)ceilf(cy + r + 1), s->y0 + s->h - 1);
  for (int y = y0; y <= y1; y++)
    for (int x = (int)floorf(cx - r - 1); x <= (int)ceilf(cx + r + 1); x++)
    {
      float dx = x - cx, dy = y - cy, dd = dx * dx + dy * dy;
      if (dd > (r + 0.5f) * (r + 0.5f) || x < 0 || x >= W)
        continue;
      float d = sqrtf(dd), cov = std::min(1.0f, r + 0.5f - d);
      float nx = dx / r, ny = dy / r, nz2 = 1 - nx * nx - ny * ny, nz = nz2 > 0 ? sqrtf(nz2) : 0;
      float lam = std::max(0.0f, -0.5f * nx - 0.5f * ny + 0.7f * nz);
      float b = band > 0 ? 0.5f + 0.5f * sinf((ny * 3.2f + d2::vn(x * 0.4f, y * 0.4f) * 0.8f) * band) : 0;
      uint32_t c = mix(col, col2, b);
      c = scale(c, glow + (1 - glow) * (0.25f + 0.95f * lam));
      float sp = powf(lam, 18) * 0.7f;
      c = mix(c, 0xFFFFFF, sp);
      d2::P(x, y, c, (int)(cov * 255));
    }
}

// 黄色い三角の明かり (黒い縁)。dir は先の向き (ラジアン)
static void arrowLamp(float cx, float cy, float dir, float r)
{
  float ax = cosf(dir), ay = sinf(dir), nx = -ay, ny = ax;
  d2::T(cx + ax * r * 1.3f, cy + ay * r * 1.3f, cx - ax * r * 0.75f + nx * r * 1.05f, cy - ay * r * 0.75f + ny * r * 1.05f,
        cx - ax * r * 0.75f - nx * r * 1.05f, cy - ay * r * 0.75f - ny * r * 1.05f, 0x140C10);
  float q = r - 1.5f;
  d2::T(cx + ax * q * 1.25f, cy + ay * q * 1.25f, cx - ax * q * 0.6f + nx * q, cy - ay * q * 0.6f + ny * q, cx - ax * q * 0.6f - nx * q,
        cy - ay * q * 0.6f - ny * q, 0xFFCC20);
  d2::C(cx + ax * q * 0.1f, cy + ay * q * 0.1f, q * 0.35f, 0xFFF4B0);
}

// 雲 (白い玉を重ね、下を青く)
static void cloud(float cx, float cy, float r)
{
  d2::C(cx, cy + r * 0.3f, r, 0x8098D0);
  d2::C(cx, cy, r, 0xE8F0FF);
  d2::C(cx - r * 0.3f, cy - r * 0.3f, r * 0.5f, 0xFFFFFF);
}

void Game5::buildTable()
{
  Surf s;
  s.buf = bg;
  s.y0 = 0;
  s.h = H;
  d2::S = &s;
  // 上の帯 (点数など)
  for (int y = 0; y < HUD_H; y++)
    d2::R(0, y, W, 1, mix(0x08103A, 0x142060, y / (float)HUD_H));
  d2::R(0, HUD_H - 1, W, 1, 0xE0A830);
  // 面
  for (int y = HUD_H; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      int rg = region(x, y);
      uint32_t c;
      if (rg == 0)
      {
        // 宇宙: 深い藍から明るい青へ、青い星雲と星
        float k = (y - YT) / (318.0f - YT);
        c = mix(0x0C1650, 0x1C2E86, k);
        float n1 = d2::vn(x * 0.022f, y * 0.022f), n2 = d2::vn(x * 0.045f + 40, y * 0.045f + 17);
        c = mix(c, 0x3450B8, std::max(0.0f, n1 - 0.45f) * 1.4f);
        c = mix(c, 0x3A2A8A, std::max(0.0f, n2 - 0.55f) * 1.2f);
        float h = d2::h2(x, y);
        if (h > 0.988f)
          c = mix(c, h > 0.997f ? 0xFFFFFF : 0xC8D4FF, 0.4f + (h - 0.988f) * 60);
      }
      else if (rg == 1)
      {
        // 外の枠: 赤いつやのある板 (縦に光の筋)
        float sh = 0.82f + 0.25f * sinf(x * 0.45f) * sinf(x * 0.45f);
        c = scale(mix(0xB01420, 0x8A0E18, (float)y / H), sh);
      }
      else if (rg == 2)
        c = mix(0x0E1640, 0x22306A, 0.5f + 0.5f * sinf((x - XD) * 0.4f)); // レーン: 溝のある紺の金属
      else if (rg == 3)
        c = ((x + y) / 4) % 3 == 0 ? 0xE0A020 : 0x16225E; // エプロン: 紺に黄色の縞
      else
        c = 0x0A1030;
      d2::P(x, y, c);
    }
  // 太陽のまわりの軌道 (うすい楕円)
  for (int k = 0; k < 3; k++)
  {
    float rx = 34 + k * 22, ry = 14 + k * 9;
    for (int i = 0; i < 360; i++)
    {
      float a = i * PI_F / 180;
      float x = BX[2] + rx * cosf(a), y = BY[2] + 22 + ry * sinf(a);
      if (region((int)x, (int)y) == 0)
        d2::P(x, y, 0x90A8F0, 45);
    }
  }
  // まんなかのバンパーのまわりの光
  for (int y = (int)BY[2] - 26; y <= (int)BY[2] + 26; y++)
    for (int x = (int)BX[2] - 26; x <= (int)BX[2] + 26; x++)
    {
      float d = hypotf(x - BX[2], y - BY[2]);
      if (d < 26 && d > 10)
        d2::P(x, y, 0xFFB040, (int)(60 * (1 - (d - 10) / 16)));
    }
  // 下の、赤と黄色の爆発のような光 (フリッパーのあいだ)
  for (int i = 0; i < 16; i++)
  {
    float a = PI_F + i * PI_F / 15, r = (i % 2) ? 20 : 30;
    d2::T(111, 316, 111 + cosf(a - 0.09f) * 4, 316 + sinf(a - 0.09f) * 4, 111 + cosf(a) * r, 316 + sinf(a) * r, 0xE02A20);
  }
  for (int i = 0; i < 16; i++)
  {
    float a = PI_F + (i + 0.5f) * PI_F / 15, r = (i % 2) ? 13 : 19;
    d2::T(111, 316, 111 + cosf(a - 0.12f) * 3, 316 + sinf(a - 0.12f) * 3, 111 + cosf(a) * r, 316 + sinf(a) * r, 0xFFC820);
  }
  d2::C(111, 316, 7, 0xFFF0A0);
  // オレンジの土星 (左)
  {
    float px = 60, py = 170;
    for (int i = 0; i < 180; i++) // 環の奥
    {
      float a = PI_F + i * PI_F / 180;
      d2::P(px + 13 * cosf(a), py + 3.5f * sinf(a) - 2 * cosf(a) * 0.3f, 0xF0C080);
    }
    sphere(px, py, 7, 0xF08A30, 0xC85A20, 2.5f, 0);
    for (int i = 0; i < 180; i++) // 環の手前
    {
      float a = i * PI_F / 180;
      d2::P(px + 13 * cosf(a), py + 3.5f * sinf(a) - 2 * cosf(a) * 0.3f, 0xFFE0A0);
      d2::P(px + 12 * cosf(a), py + 3.0f * sinf(a) - 2 * cosf(a) * 0.3f, 0xD8A060);
    }
  }
  // ロケットと、その煙の雲 (まんなかの下)
  {
    const float rx = 111, top = 207;
    // 炎: 外は橙、中は黄色、芯は白
    d2::T(rx - 6, top + 33, rx + 6, top + 33, rx, top + 58, 0xF06020);
    d2::T(rx - 4, top + 33, rx + 4, top + 33, rx, top + 50, 0xFFC830);
    d2::T(rx - 2, top + 33, rx + 2, top + 33, rx, top + 42, 0xFFFFE0);
    cloud(rx - 15, top + 53, 6);
    cloud(rx + 15, top + 53, 6);
    cloud(rx - 6, top + 56, 6);
    cloud(rx + 7, top + 57, 5);
    // 羽
    d2::T(rx - 5, top + 20, rx - 11, top + 34, rx - 5, top + 31, 0xD8202A);
    d2::T(rx + 5, top + 20, rx + 11, top + 34, rx + 5, top + 31, 0xD8202A);
    // 胴: 白から灰色へ (左から光)
    for (int x = -5; x <= 5; x++)
    {
      float k = (x + 5) / 10.0f;
      uint32_t c = mix(0xFFFFFF, 0x8890A8, k * k);
      float h0 = top + 9 - (5 - abs(x)) * 0.6f;
      d2::R(rx + x, h0, 1, top + 32 - h0, c);
    }
    // 先の赤い円錐
    d2::T(rx, top, rx - 5, top + 9, rx + 5, top + 9, 0xE02830);
    d2::L(rx - 1, top + 2, rx - 3, top + 8, 0xFF9090);
    // 窓
    d2::C(rx, top + 16, 2.8f, 0x8890A8);
    d2::C(rx, top + 16, 2, 0x3A70E0);
    d2::P(rx - 1, top + 15, 0xC8E0FF);
    // 噴き出し口
    d2::R(rx - 3, top + 32, 7, 2, 0x50586A);
    d2::R(rx - 6, top + 26, 1, 6, 0xA02028);
  }
  // 黄色い三角の明かり: 彗星ターゲットへ向く 3 つと、ブラックホールへ向く 3 つ
  {
    float len = hypotf(DR_X1 - DR_X0, DR_Y1 - DR_Y0), ux = (DR_X1 - DR_X0) / len, uy = (DR_Y1 - DR_Y0) / len, nx = uy, ny = -ux;
    for (int i = 0; i < 3; i++)
    {
      float sc = DR_S0[i] + DR_LEN / 2;
      arrowLamp(DR_X0 + ux * sc + nx * 16, DR_Y0 + uy * sc + ny * 16, atan2f(-ny, -nx), 4.5f);
    }
    for (int i = 0; i < 3; i++)
      arrowLamp(HX, HY + 22 + i * 10, -PI_F / 2, 4.2f);
  }
  // ブラックホール: 降着円盤 (黄色から橙、赤へ。渦)
  for (int y = (int)HY - 16; y <= (int)HY + 16; y++)
    for (int x = (int)HX - 16; x <= (int)HX + 16; x++)
    {
      float dx = x - HX, dy = y - HY, d = hypotf(dx, dy);
      if (d > 15.5f)
        continue;
      float a = atan2f(dy, dx);
      if (d < 6.5f)
        d2::P(x, y, mix(0x000000, 0x0A0A20, d / 6.5f));
      else
      {
        float sw = 0.5f + 0.5f * sinf(a * 3 + d * 0.9f);
        uint32_t c = mix(0xFFE060, 0xD02820, (d - 6.5f) / 9);
        int al = (int)(255 * (1 - (d - 6.5f) / 9) * (0.55f + 0.45f * sw));
        d2::P(x, y, c, std::min(255, al));
      }
    }
  d2::CR(HX, HY, 6.5f, 0xFFF0B0, 220);
  // 壁: 金のふちどりに、明るいクロームの手すり
  for (int pass = 0; pass < 3; pass++)
    for (int i = 0; i < nW; i++)
    {
      const Wall &w = walls[i];
      if (w.kind == K_DROP || w.kind == K_SLING0 || w.kind == K_SLING1)
        continue;
      if (pass == 0)
        d2::LT(w.x0, w.y0, w.x1, w.y1, 5, 0xC89020);
      else if (pass == 1)
        d2::LT(w.x0, w.y0, w.x1, w.y1, 3, 0xA8B8D8);
      else
        d2::L(w.x0, w.y0, w.x1, w.y1, 0xFFFFFF);
    }
  // 上のレーンの柱の頭
  for (int i = 0; i < 4; i++)
  {
    d2::C(POST_X[i], POST_Y0, 2.5f, 0xE8EEFF);
    d2::C(POST_X[i], POST_Y1, 2.5f, 0xE8EEFF);
  }
  // レーンの明かりの台
  for (int i = 0; i < 3; i++)
  {
    float lx = (POST_X[i] + POST_X[i + 1]) / 2;
    d2::C(lx, LAMP_Y, 4.5f, 0x4A1018);
    d2::CR(lx, LAMP_Y, 4.5f, 0xC89020);
  }
  // スリングショット: 赤い板、黄色のふち、白いゴム
  for (int sd = 0; sd < 2; sd++)
  {
    float ax, ay, bx, by, cx, cy;
    slingPts(sd, ax, ay, bx, by, cx, cy);
    Pt tri[3] = {{ax, ay}, {bx, by}, {cx, cy}};
    d2::poly(tri, 3, 0xA80E18);
    float gx = (ax + bx + cx) / 3, gy = (ay + by + cy) / 3;
    Pt in[3] = {{gx + (ax - gx) * 0.65f, gy + (ay - gy) * 0.65f}, {gx + (bx - gx) * 0.65f, gy + (by - gy) * 0.65f}, {gx + (cx - gx) * 0.65f, gy + (cy - gy) * 0.65f}};
    d2::poly(in, 3, 0xE83038);
    d2::C(gx, gy, 2, 0xFFE8A0);
    d2::LT(ax, ay, bx, by, 2, 0xFFCC20);
    d2::LT(bx, by, cx, cy, 2, 0xFFCC20);
    d2::LT(ax, ay, cx, cy, 3, 0xFFFFFF);
    d2::C(ax, ay, 2.5f, 0xE8EEFF);
    d2::C(bx, by, 2.5f, 0xE8EEFF);
    d2::C(cx, cy, 2.5f, 0xE8EEFF);
  }
  // 彗星ターゲットの台
  d2::LT(DR_X0, DR_Y0, DR_X1, DR_Y1, 5, 0x1A2040);
  d2::L(DR_X0 + 2, DR_Y0, DR_X1 + 2, DR_Y1, 0xC89020);
  // バンパー: 金の台、白い輪、赤い帽子
  for (int k = 0; k < 3; k++)
  {
    d2::C(BX[k] + 1.5f, BY[k] + 2.5f, BUMP_R + 1, 0x000000, 100);
    d2::C(BX[k], BY[k], BUMP_R + 0.5f, 0xE0A830);
    d2::CR(BX[k], BY[k], BUMP_R + 0.5f, 0x7A5010);
    d2::C(BX[k], BY[k], BUMP_R - 1.5f, 0xF4F4FA);
    sphere(BX[k], BY[k], BUMP_R - 3.5f, 0xF02830, 0xA01018, 0, 0.1f);
    d2::C(BX[k], BY[k], 2, 0xFFE8C0);
  }
  // 倍率の明かりの台
  for (int i = 0; i < 4; i++)
  {
    d2::C(MULT_X[i], MULT_Y, 8, 0x4A1018);
    d2::CR(MULT_X[i], MULT_Y, 8, 0xC89020);
  }
  // ボールセーブの明かりの台
  d2::R(97, SAVE_Y - 5, 28, 10, 0x4A1018);
  d2::R(97, SAVE_Y - 5, 28, 1, 0xC89020);
  d2::R(97, SAVE_Y + 4, 28, 1, 0xC89020);
}

bool Game5::begin()
{
  seed = esp_random() | 1;
  buildWalls();
  bg = (uint16_t *)ps_malloc(W * H * 2);
  if (!bg)
    return false;
  buildTable();
  for (int k = 0; k < 2; k++)
    fl[k] = Flip{k ? PI_F - A_REST : A_REST, k ? PI_F - A_REST : A_REST, false};
  startDemo();
  return true;
}

// ---- 流れ ----
void Game5::serve()
{
  bx = LANE_X;
  by = PLY - BALL_R - 0.5f;
  bvx = bvy = 0;
  st = BallSt::Plunger;
  saveT = 0;
  slowT = 0;
  holdT = 0;
  holeCool = 0;
  tilted = false;
  tiltW = 0;
  mult = 1;
  bonus = 0;
  aiT = 0;
  aiP = 0.75f + rnd() * 0.25f;
  plMax = 0;
}

void Game5::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  score = 0;
  ballNo = 1;
  jackpot = 2000;
  saveUsed = false;
  for (int i = 0; i < 3; i++)
    lane[i] = false, drop[i] = true;
  serve();
}

void Game5::newGame()
{
  startDemo();
  mode = Mode::Play;
  msgT = 0;
  autoPlay = false;
  cheated = false;
  sfx(Sfx::PinStart);
}

void Game5::debugDrain()
{
  if (st == BallSt::Live || st == BallSt::Plunger)
  {
    cheated = true;
    saveT = 0;
    bx = 111;
    by = DRAIN_Y + 1;
    lost();
  }
}

void Game5::add(int pts, int bon)
{
  if (tilted)
    return;
  score += pts;
  bonus += bon;
}

void Game5::message(int kind, int val, int frames)
{
  msgKind = kind;
  msgVal = val;
  msgT = frames;
}

// ---- 動き ----
// 線の壁にぶつかる
void Game5::collideSeg(float x0, float y0, float x1, float y1, float e, int kind)
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
    // 上からだけ止める (下から上がっていくボールは通す)
    float ux = dy / len, uy = -dx / len; // 上を向いた法線
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
  else if (kind == K_DROP && imp > 30)
  {
    float sp = u * len;
    for (int i = 0; i < 3; i++)
      if (drop[i] && sp >= DR_S0[i] - 2 && sp <= DR_S0[i] + DR_LEN + 2)
      {
        drop[i] = false;
        add(250, 100);
        sfx(Sfx::DropT);
        if (!drop[0] && !drop[1] && !drop[2])
        {
          jackpot += 2000;
          dropResetT = 45;
          add(1000, 200);
          message(3, jackpot);
          sfx(Sfx::Jackpot, 0.7f);
        }
        break;
      }
  }
}

// フリッパー: 根元が太く先が細い棒。回っている速さも打つ力になる
void Game5::collideFlipper(int k, float dt)
{
  const Flip &f = fl[k];
  float px = FPX[k], py = FPY, dx = FLEN * cosf(f.a), dy = FLEN * sinf(f.a);
  float u = clampf(((bx - px) * dx + (by - py) * dy) / (FLEN * FLEN), 0, 1);
  float qx = px + dx * u, qy = py + dy * u, ex = bx - qx, ey = by - qy, dd = ex * ex + ey * ey;
  float rad = FR0 + (FR1 - FR0) * u, lim = rad + BALL_R;
  if (dd >= lim * lim || dd < 1e-8f)
    return;
  float d = sqrtf(dd), nx = ex / d, ny = ey / d;
  bx = qx + nx * lim;
  by = qy + ny * lim;
  float w = (f.a - f.prevA) / dt;
  float rx = qx + nx * rad - px, ry = qy + ny * rad - py;
  float svx = -w * ry, svy = w * rx; // 当たった所の、フリッパーの面の速さ
  float vn = (bvx - svx) * nx + (bvy - svy) * ny;
  if (vn < 0)
  {
    bvx -= (1 + E_FLIP) * vn * nx;
    bvy -= (1 + E_FLIP) * vn * ny;
  }
}

// 通り道のスイッチ (前の位置 px, py から今の位置へ)
void Game5::sensors(float px, float py)
{
  if (py < LANE_SENSE_Y && by >= LANE_SENSE_Y)
    for (int i = 0; i < 3; i++)
      if (bx > POST_X[i] && bx < POST_X[i + 1])
      {
        if (laneFlashT > 0)
          add(50, 0);
        else if (!lane[i])
        {
          lane[i] = true;
          add(100, 50);
          if (lane[0] && lane[1] && lane[2])
          {
            if (mult < 5)
              mult++;
            laneFlashT = 30;
            add(500, 100);
            message(1, mult);
            sfx(Sfx::Jackpot, 0.6f);
          }
          else
            sfx(Sfx::Rollover);
        }
        else
        {
          add(50, 10);
          sfx(Sfx::Rollover, 0.5f);
        }
      }
  if (py < INLANE_Y && by >= INLANE_Y)
  {
    if ((bx > 20 && bx < SL_AX) || (bx > mirX(SL_AX) && bx < mirX(20)))
    {
      add(100, 50);
      sfx(Sfx::Rollover, 0.5f);
    }
    else if (bx < 20 || (bx > mirX(20) && bx < XD))
    {
      add(500, 100);
      sfx(Sfx::Rollover, 0.5f);
    }
  }
}

void Game5::physics()
{
  const int SUB = 10;
  const float dt = 1.0f / FPS / SUB;
  for (int sstep = 0; sstep < SUB; sstep++)
  {
    // フリッパーを回す
    for (int k = 0; k < 2; k++)
    {
      Flip &f = fl[k];
      f.prevA = f.a;
      float rest = k ? PI_F - A_REST : A_REST, upA = k ? PI_F - A_UP : A_UP;
      float target = f.up ? upA : rest, sp = (f.up ? W_UP : W_DOWN) * dt;
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
    // 打ち出す棒の上面
    float ply = PLY + plP * PL_TRAVEL;
    if (bx > XD && by > ply - BALL_R && by < ply + 12)
    {
      by = ply - BALL_R;
      if (bvy > 0)
        bvy = bvy > 40 ? -bvy * 0.2f : 0;
    }
    // バンパー
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
          add(100, 20);
          sfx(Sfx::Bumper);
        }
        bumpT[k] = 4;
      }
    }
    for (int k = 0; k < 2; k++)
      collideFlipper(k, dt);
    // ブラックホール: 遅いボールを引きよせ、真ん中に来たらつかまえる
    if (holeCool <= 0)
    {
      float hx = bx - HX, hy = by - HY, hd = hypotf(hx, hy), sp = hypotf(bvx, bvy);
      if (hd < 5 && sp < 520)
      {
        st = BallSt::Hold;
        holdT = 0;
        bx = HX;
        by = HY;
        bvx = bvy = 0;
        int j = jackpot;
        add(j, 500);
        message(2, j);
        jackpot = 2000;
        sfx(Sfx::Saucer);
        continue;
      }
      if (hd < 15 && hd > 0.5f && sp < 360)
      {
        bvx -= hx / hd * 700 * dt;
        bvy -= hy / hd * 700 * dt;
      }
    }
    sensors(px, py);
    if (st == BallSt::Plunger && bx < XD - 2)
    {
      st = BallSt::Live;
      if (!saveUsed)
        saveT = 8 * FPS;
    }
    if (by > DRAIN_Y)
    {
      lost();
      return;
    }
  }
}

void Game5::lost()
{
  if (saveT > 0 && !tilted)
  {
    saveUsed = true;
    message(4);
    sfx(Sfx::Select);
    int m = mult, bo = bonus;
    serve();
    mult = m;
    bonus = bo;
    return;
  }
  sfx(Sfx::Drain, 0.8f);
  st = BallSt::Bonus;
  bonusT = 0;
  bonusTotal = tilted ? 0 : bonus * mult;
  bonusShown = 0;
  bonusDone = 25;
  for (int k = 0; k < 2; k++)
    fl[k].up = false;
}

void Game5::stepBonus()
{
  bonusT++;
  if (bonusT < 25)
    return;
  if (bonusShown < bonusTotal)
  {
    int step = std::max(100, bonusTotal / 30 / 100 * 100);
    int d = std::min(step, bonusTotal - bonusShown);
    bonusShown += d;
    score += d;
    bonusDone = bonusT;
    if (bonusT % 2 == 0)
      sfx(Sfx::Tick, 0.5f);
    return;
  }
  if (bonusT - bonusDone < 40)
    return;
  // 次のボール
  ballNo++;
  saveUsed = false;
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

// 打ち出す: 引いているとき・止めているときは引いた量を覚える。急に戻した (手を離した) ら、その量で打つ
void Game5::stepPlunger(float pull)
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
      // ボールが棒の上にのっていれば打ち出す
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

// 自動で遊ぶ: 打ち出しは強めに。フリッパーは、ボールが届く所へ下りてきたら打つ
void Game5::stepAI(bool &l, bool &r, float &pull)
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
  for (int k = 0; k < 2; k++)
  {
    float lx = k ? FPX[1] - bx : bx - FPX[0], ly = by - FPY;
    if (aiFlipT[k] > 0)
      aiFlipT[k]--;
    else if (aiFlipT[k] < 0)
      aiFlipT[k]++;
    else if (st == BallSt::Live && lx > 4 && lx < FLEN + 4 && ly > -24 && ly < 10 && bvy > -40)
      aiFlipT[k] = 7;
    if (aiFlipT[k] == 1)
      aiFlipT[k] = -5; // 下ろしたら少し待つ
    if (aiFlipT[k] > 0)
      (k ? r : l) = true;
  }
}

void Game5::stepBall(const Input *in)
{
  bool l = false, r = false;
  float pull = 0;
  if (in)
  {
    l = in->Fire;
    r = in->Bomb;
    pull = std::max(0.0f, in->AY); // 後ろ (手前) が正
    // 前へ押すと台を揺らす
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
          message(6, 0, 3 * FPS);
          sfx(Sfx::Tilt);
        }
        else if (tiltW >= 2)
        {
          message(5, 0, FPS);
          sfx(Sfx::Warning, 0.6f);
        }
      }
    }
    fwdWas = fwd;
    // フリッパーを押した瞬間、点いているレーンを左右へずらす
    if (st == BallSt::Live && laneFlashT <= 0 && (in->FirePressed || in->BombPressed))
    {
      bool o[3] = {lane[0], lane[1], lane[2]};
      for (int i = 0; i < 3; i++)
        lane[i] = in->FirePressed ? o[(i + 1) % 3] : o[(i + 2) % 3];
    }
  }
  else
    stepAI(l, r, pull);
  if (tilted || st == BallSt::Bonus)
    l = r = false;
  if (l && !fl[0].up)
    sfx(Sfx::Flipper, 0.7f);
  if (r && !fl[1].up)
    sfx(Sfx::Flipper, 0.7f);
  fl[0].up = l;
  fl[1].up = r;
  stepPlunger(pull);

  // タイマー
  for (int k = 0; k < 3; k++)
    if (bumpT[k] > 0)
      bumpT[k]--;
  for (int k = 0; k < 2; k++)
    if (slingT[k] > 0)
      slingT[k]--;
  if (laneFlashT > 0 && --laneFlashT == 0)
    lane[0] = lane[1] = lane[2] = false;
  if (dropResetT > 0 && --dropResetT == 0)
  {
    drop[0] = drop[1] = drop[2] = true;
    sfx(Sfx::DropT, 0.6f);
  }
  if (holeCool > 0)
    holeCool--;
  if (shakeT > 0)
    shakeT--;
  if (msgT > 0)
    msgT--;
  if (st == BallSt::Live && saveT > 0)
    saveT--;
  tiltW = std::max(0.0f, tiltW - 0.6f / FPS);

  if (st == BallSt::Bonus)
  {
    physics(); // フリッパーが下りる
    stepBonus();
    return;
  }
  if (st == BallSt::Hold)
  {
    if (++holdT >= HOLD_N)
    {
      st = BallSt::Live;
      bvx = -230 + rnd() * 40;
      bvy = 40;
      holeCool = 20;
      sfx(Sfx::Launch, 0.6f);
    }
    physics();
    return;
  }
  physics();
  // 引っかかって止まったら、少し押す
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
int Game5::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

void Game5::stepEntry(const Input &in)
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

void Game5::finishEntry()
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
void Game5::Step(const Input &in)
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
// 銀のボール: 上は空 (明るい青白)、下は台 (暗い紫) を映し、左上に強い光
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
      uint32_t c = mix(0x1A2050, 0xE8EEFF, env);
      if (ny > 0.25f) // 下の縁に台の照り返し
        c = mix(c, 0x3A58C0, (ny - 0.25f) * 0.8f);
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

static void drawFlipper(float px, float py, float a, bool lit)
{
  float dx = cosf(a), dy = sinf(a), nx = -dy, ny = dx;
  float tx = px + dx * FLEN, ty = py + dy * FLEN;
  // 影
  Pt sh[4] = {{px + nx * FR0 + 2, py + ny * FR0 + 3}, {tx + nx * FR1 + 2, ty + ny * FR1 + 3}, {tx - nx * FR1 + 2, ty - ny * FR1 + 3},
              {px - nx * FR0 + 2, py - ny * FR0 + 3}};
  d2::poly(sh, 4, 0x000000, 90);
  // 赤いゴム
  Pt o[4] = {{px + nx * FR0, py + ny * FR0}, {tx + nx * FR1, ty + ny * FR1}, {tx - nx * FR1, ty - ny * FR1}, {px - nx * FR0, py - ny * FR0}};
  d2::poly(o, 4, 0xD02848);
  d2::C(px, py, FR0, 0xD02848);
  d2::C(tx, ty, FR1, 0xD02848);
  // 白い板
  float r0 = FR0 - 1.5f, r1 = FR1 - 1.3f;
  Pt in[4] = {{px + nx * r0, py + ny * r0}, {tx + nx * r1, ty + ny * r1}, {tx - nx * r1, ty - ny * r1}, {px - nx * r0, py - ny * r0}};
  d2::poly(in, 4, lit ? 0xFFFFFF : 0xE8E4F0);
  d2::C(px, py, r0, 0xE8E4F0);
  d2::L(px - nx * r0 * 0.4f, py - ny * r0 * 0.4f, tx - nx * r1 * 0.4f, ty - ny * r1 * 0.4f, 0xFFFFFF);
  // 軸
  d2::C(px, py, 1.6f, 0x8890A8);
}

static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, 1, INK);
}
static void panel(int x, int y, int w, int h)
{
  d2::R(x, y, w, h, 0x0A1238, 225);
  d2::R(x, y, w, 2, 0xD8202A);
  d2::R(x, y + h - 2, w, 2, 0xD8202A);
  d2::R(x, y, 1, h, 0xE0A830);
  d2::R(x + w - 1, y, 1, h, 0xE0A830);
}

void Game5::drawDynamic(Gfx &g)
{
  bool blink = (t / 4) % 2;
  // レーンの明かり
  for (int i = 0; i < 3; i++)
  {
    float lx = (POST_X[i] + POST_X[i + 1]) / 2;
    bool on = laneFlashT > 0 ? blink : lane[i];
    if (on)
    {
      d2::C(lx, LAMP_Y, 7, 0xFFD050, 60);
      d2::C(lx, LAMP_Y, 3.8f, 0xFFE070);
      d2::P(lx - 1, LAMP_Y - 1, 0xFFFFFF);
    }
  }
  // 倍率の明かり
  for (int i = 0; i < 4; i++)
    if (mult >= i + 2)
    {
      d2::C(MULT_X[i], MULT_Y, 11, 0xFFC830, 60);
      d2::C(MULT_X[i], MULT_Y, 7, 0xFFD040);
    }
  // ボールセーブ
  if (saveT > 0 && (saveT > 2 * FPS || blink))
  {
    d2::R(98, SAVE_Y - 4, 26, 8, 0xFFD040);
    d2::R(95, SAVE_Y - 7, 32, 14, 0xFFC830, 60);
  }
  // 彗星ターゲット
  {
    float len = hypotf(DR_X1 - DR_X0, DR_Y1 - DR_Y0), ux = (DR_X1 - DR_X0) / len, uy = (DR_Y1 - DR_Y0) / len, nx = -uy, ny = ux;
    if (nx < 0)
      nx = -nx, ny = -ny; // 台の面のほう (右上)
    for (int i = 0; i < 3; i++)
    {
      float s0 = DR_S0[i], s1 = s0 + DR_LEN;
      float x0 = DR_X0 + ux * s0, y0 = DR_Y0 + uy * s0, x1 = DR_X0 + ux * s1, y1 = DR_Y0 + uy * s1;
      if (drop[i])
      {
        Pt q[4] = {{x0, y0}, {x1, y1}, {x1 + nx * 4, y1 + ny * 4}, {x0 + nx * 4, y0 + ny * 4}};
        d2::poly(q, 4, dropResetT > 0 && blink ? 0xFFFFFF : 0xE82838);
        d2::L(x0 + nx * 4, y0 + ny * 4, x1 + nx * 4, y1 + ny * 4, 0xFFC8C8);
        // 彗星のしるし
        float mx = (x0 + x1) / 2 + nx * 2, my = (y0 + y1) / 2 + ny * 2;
        d2::P(mx, my, 0xFFFFFF);
      }
      else
        d2::LT(x0, y0, x1, y1, 2, 0x5A1018);
    }
  }
  // バンパーが光る
  for (int k = 0; k < 3; k++)
    if (bumpT[k] > 0)
    {
      d2::CR(BX[k], BY[k], BUMP_R + 1.5f, 0xFFFFFF);
      d2::C(BX[k], BY[k], BUMP_R + 5, 0xFFF0C0, 70);
    }
  // スリングショットが光る
  for (int sd = 0; sd < 2; sd++)
    if (slingT[sd] > 0)
    {
      float ax, ay, bx2, by2, cx, cy;
      slingPts(sd, ax, ay, bx2, by2, cx, cy);
      Pt tri[3] = {{ax, ay}, {bx2, by2}, {cx, cy}};
      d2::poly(tri, 3, 0xFFD040, 150);
      d2::LT(ax, ay, cx, cy, 3, 0xFFFFFF);
    }
  // ブラックホール: つかまえているあいだは光が渦を巻く
  if (st == BallSt::Hold)
  {
    float k = holdT / (float)HOLD_N;
    for (int i = 0; i < 8; i++)
    {
      float a = i * PI_F / 4 + t * 0.4f, rr = 14 * (1 - k * 0.5f);
      d2::C(HX + cosf(a) * rr, HY + sinf(a) * rr, 1.5f, 0xFFD890);
    }
  }
  else if (jackpot > 2000 && blink)
    d2::CR(HX, HY, 15, 0xFFD050, 160);
  // 打ち出す棒とバネ
  {
    float ply = PLY + plP * PL_TRAVEL;
    for (float y = ply + 4; y < 319; y += 2.5f + (319 - ply) * 0.02f)
      d2::L(XD + 3, y, XO - 3, y + 1.5f, 0x8890A8);
    d2::R(LANE_X - 1, ply, 3, 320 - ply, 0xC0C8E0);
    d2::R(XD + 2, ply, XO - XD - 3, 4, 0xD8202A);
    d2::R(XD + 2, ply, XO - XD - 3, 1, 0xFF9090);
  }
  // フリッパー
  for (int k = 0; k < 2; k++)
    drawFlipper(FPX[k], FPY, fl[k].a, fl[k].up);
  // ボール
  if (st != BallSt::Bonus || by < DRAIN_Y)
  {
    if (st == BallSt::Hold)
    {
      float k = std::min(1.0f, holdT / 10.0f), r = BALL_R * (1 - 0.35f * k);
      if (holdT > HOLD_N - 8)
        r = BALL_R * (0.65f + 0.35f * (holdT - (HOLD_N - 8)) / 8.0f);
      drawBall(bx, by, r);
    }
    else
    {
      d2::C(bx + 2, by + 3, BALL_R, 0x000000, 80);
      drawBall(bx, by, BALL_R);
    }
  }
  (void)g;
}

void Game5::drawHud(Gfx &g)
{
  char s[24];
  snprintf(s, sizeof(s), "%07d", score);
  txt(g, 4, 3, CREAM, s, false, 3);
  snprintf(s, sizeof(s), "BALL %d", std::min(ballNo, 3));
  txt(g, W - 4 - (int)strlen(s) * 6, 3, MUTE, s);
  snprintf(s, sizeof(s), "x%d", mult);
  txt(g, W - 4 - (int)strlen(s) * 6, 12, mult > 1 ? CYAN : MUTE, s);
  // 倍率の明かりの字
  static const char *ML[4] = {"2X", "3X", "4X", "5X"};
  for (int i = 0; i < 4; i++)
    txt(g, (int)MULT_X[i], (int)MULT_Y - 3, mult >= i + 2 ? 0x401008 : 0xD07070, ML[i], true);
  if (saveT > 0 && (saveT > 2 * FPS || (t / 4) % 2))
    txt(g, 111, (int)SAVE_Y - 3, 0x401008, "SAVE", true);
  else
    txt(g, 111, (int)SAVE_Y - 3, 0xD07070, "SAVE", true);
}

void Game5::drawOverlay(Gfx &g)
{
  char s[40];
  if (volT > 0)
  {
    panel(70, 236, 100, 22);
    txt(g, 78, 244, CREAM, VolumeLevel == 0 ? "VOL OFF" : "VOL");
    for (int i = 0; i < 4; i++)
      d2::R(110 + i * 13, 244, 10, 8, i < VolumeLevel ? CYAN : 0x1A2450);
  }
  if (msgT > 0 && (mode == Mode::Play || mode == Mode::Attract))
  {
    int y = 166;
    switch (msgKind)
    {
    case 1:
      snprintf(s, sizeof(s), "%dX BONUS", msgVal);
      txt(g, 111, y, CYAN, s, true, 3);
      break;
    case 2:
      txt(g, 111, y, GOLD, "JACKPOT", true, 3);
      snprintf(s, sizeof(s), "%d", msgVal);
      txt(g, 111, y + 16, CREAM, s, true);
      break;
    case 3:
      txt(g, 111, y, CYAN, "COMETS!", true, 3);
      snprintf(s, sizeof(s), "JACKPOT %d", msgVal);
      txt(g, 111, y + 16, GOLD, s, true);
      break;
    case 4:
      txt(g, 111, y, 0x60F090, "BALL SAVED", true, 3);
      break;
    case 5:
      txt(g, 111, y, REDT, "DANGER", true, 3);
      break;
    case 6:
      txt(g, 111, y, REDT, "TILT", true, 5);
      break;
    }
  }
  if (st == BallSt::Bonus && (mode == Mode::Play || mode == Mode::Attract) && bonusT > 5)
  {
    panel(36, 130, 150, 64);
    txt(g, 111, 138, GOLD, tilted ? "TILT" : "BONUS", true, 3);
    snprintf(s, sizeof(s), "%d x %d", tilted ? 0 : bonus, mult);
    txt(g, 111, 160, CREAM, s, true);
    snprintf(s, sizeof(s), "%d", bonusShown);
    txt(g, 111, 174, CYAN, s, true);
  }
  switch (mode)
  {
  case Mode::Attract:
  {
    int ph = (modeT / (8 * FPS)) % 3;
    if (ph == 0)
    {
      panel(20, 88, 182, 120);
      txt(g, 111, 100, CYAN, "ORBIT", true, 5);
      txt(g, 111, 130, CREAM, "SPACE PINBALL", true);
      txt(g, 111, 142, MUTE, "3 BALLS", true);
      if ((modeT / 15) % 2 == 0)
        txt(g, 111, 160, CREAM, "PUSH SW1", true, 3);
      txt(g, 111, 192, MUTE, "SW3 HOLD: MENU", true);
    }
    else if (ph == 1)
    {
      panel(16, 84, 190, 128);
      txt(g, 111, 94, GOLD, "HOW TO PLAY", true, 3);
      txt(g, 111, 118, CREAM, "SW1 / SW2: FLIPPERS", true);
      txt(g, 111, 132, CREAM, "PULL STICK & LET GO:", true);
      txt(g, 111, 142, CREAM, "LAUNCH", true);
      txt(g, 111, 156, CREAM, "PUSH STICK: NUDGE", true);
      txt(g, 111, 172, MUTE, "LIGHT 3 LANES = MULT", true);
      txt(g, 111, 184, MUTE, "COMETS -> BLACK HOLE", true);
      txt(g, 111, 196, MUTE, "= JACKPOT", true);
    }
    else
    {
      panel(20, 80, 182, 140);
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
      txt(g, 111, 214, CREAM, "PULL THE STICK BACK", true);
      txt(g, 111, 226, CREAM, "AND LET GO", true);
    }
    if (autoPlay)
      txt(g, 8, 34, MUTE, "AUTO");
    if (paused)
    {
      panel(30, 120, 162, 64);
      txt(g, 111, 132, CREAM, "PAUSE", true, 3);
      txt(g, 111, 158, MUTE, "SW3: RESUME", true);
      txt(g, 111, 170, MUTE, "SW2: QUIT", true);
    }
    break;
  case Mode::Over:
    panel(20, 110, 182, 80);
    txt(g, 111, 122, REDT, "GAME OVER", true, 3);
    snprintf(s, sizeof(s), "%d", score);
    txt(g, 111, 148, CREAM, s, true, 3);
    if (cheated)
      txt(g, 111, 174, MUTE, "TEST PLAY (NO RECORD)", true);
    else if (score > Table[0].Score)
      txt(g, 111, 174, GOLD, "NEW HIGH SCORE!", true);
    break;
  case Mode::NameEntry:
  {
    panel(20, 90, 182, 120);
    txt(g, 111, 102, GOLD, "NEW RECORD", true, 3);
    snprintf(s, sizeof(s), "%d", score);
    txt(g, 111, 124, CREAM, s, true);
    for (int i = 0; i < 3; i++)
    {
      char c[2] = {entry[i], 0};
      int x = 111 - 22 + i * 22;
      txt(g, x, 146, i == entryPos ? CYAN : CREAM, c, true, 5);
      if (i == entryPos && (t / 8) % 2)
        d2::R(x - 8, 172, 16, 2, CYAN);
    }
    txt(g, 111, 186, MUTE, "STICK: LETTER  SW1: OK", true);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, 186, 100, MUTE, s);
    break;
  }
  case Mode::Ranking:
  {
    panel(20, 80, 182, 150);
    txt(g, 111, 94, GOLD, "HIGH SCORES", true, 3);
    static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
    for (int i = 0; i < 5; i++)
    {
      snprintf(s, sizeof(s), "%s %8d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
      bool mine = i == newRank;
      if (!mine || (t / 8) % 2)
        txt(g, 111, 122 + i * 18, mine ? REDT : i == 0 ? GOLD : CREAM, s, true);
    }
    snprintf(s, sizeof(s), "YOU %d", score);
    txt(g, 111, 214, MUTE, s, true);
    break;
  }
  }
}

void Game5::Draw(Gfx &gx)
{
  uint32_t t0 = micros();
  int ltop = gx.top * 2 / 3;
  int sh = shakeT > 0 ? ((shakeT & 1) ? -2 : 2) : 0; // 揺らしたときは台を上下にずらす
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
