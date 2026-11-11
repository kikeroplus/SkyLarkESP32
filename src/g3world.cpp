// 3 本目 (LONGSHOT) の景色と、動くものの描き方。座標は世界 (240x300) の点で書き、K 倍して描く
#include "g3world.h"
#include <math.h>

namespace w3
{
int tod = DAY;
uint16_t *hi = nullptr, *lo = nullptr;
float K = 1, OX = 0, OY = 0, lit = 0;
static float SH = 0, SY = 0; // 人が倒れるときの傾き: x に SH * (SY - y) を足す

const uint32_t HAT_C[H_N] = {0, 0xc8382f, 0x2c3e6b, 0x1e1e22, 0xc9b38a, 0xe8e6e0, 0x3f7a4a};
const uint32_t BAG_C[B_N] = {0, 0x1a1a1d, 0x7a4e2c, 0xa8302a};
const uint32_t COAT_C[CO_N + 1] = {0x7d8590, 0x34466e, 0x4f7a52, 0xd8b23c, 0xe2ddd0, 0x7c5a3c, 0x4a7fb8, 0x7a3040, 0x3a3f48};
const char *const HAT_NAME[H_N] = {"", "RED CAP", "NAVY CAP", "BLACK CAP", "BEIGE HAT", "WHITE CAP", "GREEN CAP"};
const char *const BAG_NAME[B_N] = {"", "BLACK BAG", "BROWN BAG", "RED BAG"};
const char *const COAT_NAME[CO_N] = {"GREY COAT", "NAVY COAT", "GREEN COAT", "YELLOW COAT", "WHITE COAT", "BROWN COAT", "BLUE COAT", "WINE COAT"};
static const uint32_t SKIN[4] = {0xf0c8a0, 0xd9a77e, 0xb07a52, 0x7a4e32};
static const uint32_t HAIR[5] = {0x2a2420, 0x5a3a22, 0x9a6a3a, 0xd8c08a, 0x8a8a8a};
static const uint32_t PANTS[4] = {0x2e3440, 0x3b4a66, 0x5a5048, 0x1e1e22};
static const uint32_t IRON = 0x2a2a2e, BRASS = 0xb08d3a;
static const uint32_t STONE = 0xcdbb9c, STONE_D = 0xa8967a, STONE_L = 0xe2d6bd, CORN = 0x9b8a70;
int sceneNo = 0;
float rainAmt = 0;

// ---- 色 ----
uint32_t rgb(int r, int g, int b)
{
  r = r < 0 ? 0 : r > 255 ? 255 : r;
  g = g < 0 ? 0 : g > 255 ? 255 : g;
  b = b < 0 ? 0 : b > 255 ? 255 : b;
  return (r << 16) | (g << 8) | b;
}
uint32_t mix(uint32_t a, uint32_t b, float t)
{
  int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255, br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
  return rgb(ar + (br - ar) * t, ag + (bg - ag) * t, ab + (bb - ab) * t) | (a & EMIT);
}
uint32_t shade(uint32_t c, float k)
{
  return rgb((c >> 16 & 255) * k, (c >> 8 & 255) * k, (c & 255) * k) | (c & EMIT);
}
static uint32_t grade(uint32_t c)
{
  float r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255;
  if (tod == DUSK)
    return rgb(r * 0.78f + 16, g * 0.56f + 8, b * 0.52f + 22);
  float l = 0.3f * r + 0.59f * g + 0.11f * b;
  return rgb((0.35f * r + 0.65f * l) * 0.26f, (0.35f * g + 0.65f * l) * 0.30f, (0.35f * b + 0.65f * l) * 0.42f + 10);
}
uint32_t col(uint32_t c)
{
  if (c & EMIT)
    return c & 0xFFFFFF;
  if (tod == DAY)
    return c;
  uint32_t g = grade(c);
  if (lit > 0) // 街灯の暖かい光
  {
    uint32_t w = rgb((c >> 16 & 255) * 0.95f, (c >> 8 & 255) * 0.8f, (c & 255) * 0.55f);
    g = mix(g, w, lit * (tod == NIGHT ? 0.75f : 0.5f));
  }
  return g;
}

// ---- 図形 (世界の座標 → 描く先の点) ----
static inline float sx(float x, float y) { return (x + SH * (SY - y) - OX) * K; }
static inline float sy(float y) { return (y - OY) * K; }

void R(float x, float y, float w, float h, uint32_t c, int a)
{
  if (w <= 0 || h <= 0)
    return;
  if (SH != 0)
  {
    Q(x, y, x + w, y, x + w, y + h, x, y + h, c, a);
    return;
  }
  int X0 = lroundf(sx(x, y)), X1 = lroundf(sx(x + w, y)), Y0 = lroundf(sy(y)), Y1 = lroundf(sy(y + h));
  if (X1 == X0 && w * K > 0.3f)
    X1 = X0 + 1;
  if (Y1 == Y0 && h * K > 0.3f)
    Y1 = Y0 + 1;
  uint32_t cc = col(c);
  for (int yy = Y0; yy < Y1; yy++)
    d2::span(X0, X1 - 1, yy, cc, a);
}
void C(float x, float y, float r, uint32_t c, int a) { d2::C(sx(x, y), sy(y), r * K, col(c), a); }
void E(float x, float y, float rx, float ry, uint32_t c, int a)
{
  float cx = sx(x, y), cy = sy(y), RX = rx * K, RY = ry * K;
  if (RY < 0.5f)
  {
    d2::span(lroundf(cx - RX), lroundf(cx + RX), lroundf(cy), col(c), a);
    return;
  }
  uint32_t cc = col(c);
  int y0 = lroundf(cy - RY), y1 = lroundf(cy + RY);
  for (int yy = y0; yy <= y1; yy++)
  {
    float d = (yy - cy) / RY;
    if (d * d > 1)
      continue;
    float w = RX * sqrtf(1 - d * d);
    d2::span(lroundf(cx - w), lroundf(cx + w), yy, cc, a);
  }
}
void L(float x0, float y0, float x1, float y1, float w, uint32_t c, int a)
{
  int t = max(1, (int)lroundf(w * K));
  float X0 = sx(x0, y0), Y0 = sy(y0), X1 = sx(x1, y1), Y1 = sy(y1);
  if (t <= 2)
  {
    d2::LT(X0, Y0, X1, Y1, t, col(c), a);
    return;
  }
  // 太い線は四角形で描く (端が欠けないように)
  float dx = X1 - X0, dy = Y1 - Y0, l = sqrtf(dx * dx + dy * dy);
  if (l < 0.01f)
  {
    d2::C(X0, Y0, t * 0.5f, col(c), a);
    return;
  }
  float nx = -dy / l * t * 0.5f, ny = dx / l * t * 0.5f;
  Pt p[4] = {{X0 + nx, Y0 + ny}, {X1 + nx, Y1 + ny}, {X1 - nx, Y1 - ny}, {X0 - nx, Y0 - ny}};
  d2::poly(p, 4, col(c), a);
}
void T(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c, int a)
{
  Pt p[3] = {{sx(x0, y0), sy(y0)}, {sx(x1, y1), sy(y1)}, {sx(x2, y2), sy(y2)}};
  d2::poly(p, 3, col(c), a);
}
void Q(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t c, int a)
{
  Pt p[4] = {{sx(x0, y0), sy(y0)}, {sx(x1, y1), sy(y1)}, {sx(x2, y2), sy(y2)}, {sx(x3, y3), sy(y3)}};
  d2::poly(p, 4, col(c), a);
}
void ring(float x, float y, float r, float w, uint32_t c, int a)
{
  const int N = 20;
  for (int i = 0; i < N; i++)
  {
    float a0 = i * 6.2832f / N, a1 = (i + 1) * 6.2832f / N;
    L(x + cosf(a0) * r, y + sinf(a0) * r, x + cosf(a1) * r, y + sinf(a1) * r, w, c, a);
  }
}

// 点ごとに明るさをばらつかせて塗る (石や舗装の肌理)。cell は描く先の点でのまだらの大きさ。動かない景色を描くときだけ使う
void tex(float x, float y, float w, float h, uint32_t base, float amt, int cell, float seed, float fine)
{
  Surf *s = d2::S;
  int X0 = lroundf(sx(x, y)), X1 = lroundf(sx(x + w, y)) - 1, Y0 = lroundf(sy(y)), Y1 = lroundf(sy(y + h)) - 1;
  X0 = max(X0, s->cx0);
  X1 = min(X1, s->cx1);
  Y0 = max(max(Y0, s->cy0), s->y0);
  Y1 = min(min(Y1, s->cy1), s->y0 + s->h - 1);
  uint32_t b = col(base);
  int br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
  for (int yy = Y0; yy <= Y1; yy++)
  {
    uint16_t *p = s->buf + (yy - s->y0) * s->w;
    for (int xx = X0; xx <= X1; xx++)
    {
      float v = 1 + (d2::h2(xx / cell + seed, yy / cell) - 0.5f) * amt + (d2::h2(xx + seed * 7, yy * 1.3f) - 0.5f) * fine;
      p[xx] = d2::c565(rgb(br * v, bg * v, bb * v));
    }
  }
}

// ---- 看板の文字 (5x7) ----
static const char *const GL_CH = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-&";
static const uint8_t GL[][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {15, 16, 16, 16, 16, 16, 15}, {30, 17, 17, 17, 17, 17, 30}, // A B C D
    {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16}, {15, 16, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, // E F G H
    {14, 4, 4, 4, 4, 4, 14},      {7, 2, 2, 2, 2, 18, 12},      {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31}, // I J K L
    {17, 27, 21, 21, 17, 17, 17}, {17, 17, 25, 21, 19, 17, 17}, {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16}, // M N O P
    {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17}, {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       // Q R S T
    {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 27, 17}, {17, 17, 10, 4, 10, 17, 17},  // U V W X
    {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},                                                                 // Y Z
    {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},     // 0 1 2 3
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 30, 1, 1, 17, 14},   {6, 8, 16, 30, 17, 17, 14},   {31, 1, 2, 4, 8, 8, 8},       // 4 5 6 7
    {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 2, 12},                                                               // 8 9
    {0, 0, 0, 0, 0, 12, 12},      {0, 0, 0, 14, 0, 0, 0},       {12, 18, 20, 8, 21, 18, 13},                                // . - &
};
void sign(float x, float y, float cell, const char *s, uint32_t c, int a)
{
  for (; *s; s++, x += cell * 6.4f)
  {
    const char *p = strchr(GL_CH, *s);
    if (!p || *s == ' ')
      continue;
    const uint8_t *g = GL[p - GL_CH];
    for (int r = 0; r < 7; r++)
      for (int k = 0; k < 5; k++)
        if (g[r] >> (4 - k) & 1)
          R(x + k * cell, y + r * cell, cell * 1.02f, cell * 1.02f, c, a);
  }
}
float signW(const char *s, float cell) { return strlen(s) * cell * 6.4f - cell * 1.4f; }
float hh(float a, float b, float c) { return d2::hs(a * 3.17f + b * 11.3f + c * 29.1f + 0.5f); }

// ============ 動かない景色 ============
void sky(float bottom)
{
  for (int i = 0; i < (int)(bottom * 4); i++)
  {
    float y = i / 4.0f, k = y / (bottom - 2);
    uint32_t c;
    if (tod == DAY)
      c = mix(0x5e9bd0, 0xcfe3ec, powf(k, 1.3f));
    else if (tod == DUSK)
      c = k < 0.55f ? mix(0x2a3466, 0xb8607a, k / 0.55f) : mix(0xb8607a, 0xf4b26a, (k - 0.55f) / 0.45f);
    else
      c = mix(0x050914, 0x17203c, k);
    R(0, y, WW, 0.25f, c | EMIT);
  }
  if (tod == DAY) // 雲: ふくらんだ楕円を重ね、下側を少し暗く
  {
    static const float CL[][3] = {{30, 10, 1}, {104, 5, 0.8f}, {168, 13, 1.2f}, {226, 7, 0.7f}};
    for (auto &c : CL)
    {
      float x = c[0], y = c[1], s = c[2];
      E(x, y + 1.5f * s, 14 * s, 2.5f * s, 0xc8d8e4 | EMIT, 200);
      C(x - 6 * s, y, 4 * s, 0xf4f8fa | EMIT, 230);
      C(x, y - 1.5f * s, 5.5f * s, 0xffffff | EMIT, 240);
      C(x + 6 * s, y + 0.5f * s, 4.2f * s, 0xf4f8fa | EMIT, 230);
      E(x, y + 2.6f * s, 12 * s, 1.2f * s, 0xb4c8d6 | EMIT, 140);
    }
  }
  else if (tod == DUSK) // 沈む日と、赤く染まった筋雲
  {
    for (int i = 6; i >= 1; i--)
      C(150, 40, 9 + i * 3.5f, 0xffc070 | EMIT, 22);
    C(150, 40, 8, 0xffe2b0 | EMIT);
    static const float ST[][3] = {{20, 12, 34}, {70, 20, 26}, {120, 8, 40}, {200, 18, 30}};
    for (auto &s : ST)
    {
      E(s[0], s[1], s[2], 1.3f, 0xf08a7a | EMIT, 150);
      E(s[0] + 3, s[1] - 0.6f, s[2] * 0.7f, 0.6f, 0xffc0a0 | EMIT, 140);
    }
  }
  else // 星と月
  {
    for (int i = 0; i < 70; i++)
    {
      float x = d2::hs(i * 3.1f) * WW, y = d2::hs(i * 5.7f + 1) * 40, b = d2::hs(i * 1.3f + 2);
      R(x, y, 0.3f + b * 0.3f, 0.3f + b * 0.3f, (b > 0.7f ? 0xffffff : 0x9aa4c0) | EMIT);
    }
    for (int i = 5; i >= 1; i--)
      C(44, 14, 5 + i * 2.5f, 0xc8d4f0 | EMIT, 16);
    C(44, 14, 5, 0xf2eedc | EMIT);
    C(42.5f, 13, 1.1f, 0xd8d2c0 | EMIT);
    C(45.6f, 16, 0.8f, 0xd8d2c0 | EMIT);
    C(46, 12.2f, 0.5f, 0xd8d2c0 | EMIT);
  }
}

void skyline(float bottom)
{
  struct B
  {
    float x, w, top;
  };
  static const B back[] = {{0, 18, 22}, {16, 14, 14}, {28, 22, 26}, {48, 12, 8}, {58, 26, 20}, {82, 16, 12}, {96, 30, 24},
                           {124, 14, 16}, {136, 20, 6}, {154, 26, 18}, {178, 18, 10}, {194, 24, 22}, {216, 26, 14}};
  uint32_t haze = tod == DAY ? 0xa9bccd : tod == DUSK ? 0x7a5a78 : 0x141a2c;
  for (auto &b : back)
  {
    R(b.x, b.top, b.w, bottom - b.top, haze | EMIT);
    for (float y = b.top + 2; y < bottom; y += 2.5f)
      for (float x = b.x + 1.5f; x < b.x + b.w - 1; x += 2.5f)
      {
        float h = d2::h2(x, y);
        if (tod == DAY)
          R(x, y, 1, 1, shade(haze, 0.92f) | EMIT);
        else if (h < 0.35f)
          R(x, y, 1, 1, (h < 0.1f ? 0xd0e0f0 : 0xe8c070) | EMIT, tod == NIGHT ? 220 : 110);
      }
  }
  // 遠くの工事のクレーン
  uint32_t cr = tod == DAY ? 0x8a9aaa : tod == DUSK ? 0x5a3a52 : 0x0c1020;
  L(64, 2, 64, 30, 0.8f, cr | EMIT);
  L(48, 4, 96, 4, 0.7f, cr | EMIT);
  L(64, 2, 50, 4, 0.3f, cr | EMIT);
  L(64, 2, 92, 4, 0.3f, cr | EMIT);
  L(86, 4, 86, 12, 0.2f, cr | EMIT);
  R(48, 4, 4, 2, cr | EMIT);
  static const B front[] = {{0, 26, 26}, {24, 20, 18}, {42, 28, 28}, {68, 18, 14}, {84, 14, 24}, {100, 22, 20},
                            {120, 18, 26}, {136, 24, 22}, {158, 16, 12}, {172, 22, 24}, {192, 18, 28}, {208, 32, 20}};
  for (int i = 0; i < 12; i++)
  {
    const B &b = front[i];
    uint32_t base = 0x8094a8;
    R(b.x, b.top, b.w, bottom - b.top, base);
    R(b.x, b.top, 0.8f, bottom - b.top, shade(base, 1.12f)); // 日の当たる縁
    R(b.x + b.w - 1.2f, b.top, 1.2f, bottom - b.top, shade(base, 0.85f));
    R(b.x - 0.4f, b.top - 0.8f, b.w + 0.8f, 0.8f, shade(base, 0.75f)); // 屋上のへり
    for (float y = b.top + 2.5f; y < bottom; y += 3)
      for (float x = b.x + 2; x < b.x + b.w - 2; x += 3.5f)
      {
        float h = d2::h2(x + i, y);
        if (tod != DAY && h < 0.4f)
          R(x, y, 1.6f, 1.2f, (h < 0.12f ? 0xc8dcf0 : 0xf0c878) | EMIT, tod == NIGHT ? 230 : 130);
        else
          R(x, y, 1.6f, 1.2f, h < 0.5f ? 0x6a7e92 : 0x9cb0c2);
      }
    if (i % 3 == 1) // 屋上の給水塔とアンテナ
    {
      float x = b.x + b.w * 0.6f;
      R(x - 2, b.top - 4, 4, 3, 0x6a6a70);
      T(x - 2.4f, b.top - 4, x + 2.4f, b.top - 4, x, b.top - 5.5f, 0x5a5a60);
      L(x - 1.5f, b.top - 1, x - 1.5f, b.top, 0.3f, 0x4a4a50);
      L(x + 1.5f, b.top - 1, x + 1.5f, b.top, 0.3f, 0x4a4a50);
    }
    if (i == 2 || i == 10)
    {
      float x = b.x + 6;
      L(x, b.top - 10, x, b.top, 0.35f, 0x5a6070);
      if (tod != DAY)
        C(x, b.top - 10, 0.6f, 0xff3020 | EMIT);
    }
  }
}


void lampPost(float x, float baseY, float h)
{
  float top = baseY - h;
  R(x - 1.3f, baseY - 2.6f, 2.6f, 2.6f, IRON);
  R(x - 0.9f, baseY - 3.4f, 1.8f, 0.8f, IRON);
  L(x, baseY - 3, x, top + 3.6f, 0.7f, IRON);
  R(x - 0.8f, baseY - h * 0.45f, 1.6f, 0.8f, IRON);
  R(x - 0.35f, top + 3.6f, 0.3f, baseY - top - 7, 0x5a5a62); // 光の当たる側
  bool on = tod != DAY;
  T(x - 2.2f, top + 0.4f, x + 2.2f, top + 0.4f, x, top - 1.6f, IRON);
  Q(x - 1.8f, top + 0.4f, x + 1.8f, top + 0.4f, x + 1.2f, top + 3.4f, x - 1.2f, top + 3.4f, on ? (0xffecb0 | EMIT) : 0xd8dccc);
  R(x - 1.4f, top + 3.4f, 2.8f, 0.6f, IRON);
  L(x, top + 0.4f, x, top + 3.4f, 0.2f, IRON);
  if (on)
    for (int i = 4; i >= 1; i--)
      C(x, top + 1.8f, 1.5f + i * 1.8f, 0xffd890 | EMIT, 26);
}


// ---- 少しずつ描く ----
static int buildRow = -1;
static const int CHUNK = 100;
bool alloc()
{
  if (!hi)
    hi = (uint16_t *)ps_malloc((size_t)HW * HH * 2);
  if (!lo)
    lo = (uint16_t *)ps_malloc((size_t)WW * WH * 2);
  return hi && lo;
}
static int buildScene = 0;
void buildBegin(int sc, int t)
{
  tod = t;
  sceneNo = buildScene = sc;
  buildRow = 0;
}
float buildProgress() { return buildRow < 0 ? 1 : (float)buildRow / (HH + CHUNK); }
bool buildStep()
{
  if (buildRow < 0)
    return true;
  if (buildRow < HH)
  {
    Surf s;
    s.buf = hi;
    s.w = HW;
    s.y0 = 0;
    s.h = HH;
    s.cx0 = 0;
    s.cx1 = HW - 1;
    s.cy0 = buildRow;
    s.cy1 = min(HH - 1, buildRow + CHUNK - 1);
    Surf *prev = d2::S;
    d2::S = &s;
    K = Z;
    OX = OY = 0;
    SH = 0;
    lit = 0;
    memset(hi + (size_t)s.cy0 * HW, 0, (size_t)(s.cy1 - s.cy0 + 1) * HW * 2); // 前のシーンの絵を消す
    SC().draw(s.cy0, s.cy1);
    d2::S = prev;
    buildRow += CHUNK;
    return false;
  }
  // 縮めた絵を作る (4x4 の平均)
  for (int y = 0; y < WH; y++)
    for (int x = 0; x < WW; x++)
    {
      int r = 0, g = 0, b = 0;
      for (int j = 0; j < Z; j++)
      {
        const uint16_t *p = hi + (size_t)(y * Z + j) * HW + x * Z;
        for (int i = 0; i < Z; i++)
        {
          uint16_t v = p[i];
          r += v >> 11;
          g += (v >> 5) & 63;
          b += v & 31;
        }
      }
      lo[y * WW + x] = ((r >> 4) << 11) | ((g >> 4) << 5) | (b >> 4);
    }
  buildRow = -1;
  return true;
}

// ---- 小さな下書き (見渡す画面で、4 倍で描いて縮める) ----
static const int MW = 224, MH = 112; // 下書きの大きさ (世界の 56x28 まで。バスが入る)
static uint16_t miniBuf[MW * MH];
static const uint16_t MKEY = 0x0821;
static Surf miniS, *miniPrev;
static float mK, mOX, mOY;
static int mx0, my0, mw, mh;
void miniBegin(float x0, float y0, float w, float h)
{
  miniPrev = d2::S;
  mK = K;
  mOX = OX;
  mOY = OY;
  mx0 = (int)floorf(x0);
  my0 = (int)floorf(y0);
  mw = min(MW / Z, (int)ceilf(x0 + w) - mx0);
  mh = min(MH / Z, (int)ceilf(y0 + h) - my0);
  miniS = Surf();
  miniS.buf = miniBuf;
  miniS.w = MW;
  miniS.y0 = 0;
  miniS.h = MH;
  miniS.cx0 = 0;
  miniS.cx1 = mw * Z - 1;
  miniS.cy0 = 0;
  miniS.cy1 = mh * Z - 1;
  miniS.key = MKEY;
  for (int y = 0; y < mh * Z; y++)
    for (int x = 0; x < mw * Z; x++)
      miniBuf[y * MW + x] = MKEY;
  d2::S = &miniS;
  K = Z;
  OX = mx0;
  OY = my0;
}
void miniEnd()
{
  d2::S = miniPrev;
  K = mK;
  OX = mOX;
  OY = mOY;
  Surf *s = d2::S;
  for (int by = 0; by < mh; by++)
  {
    int Y = lroundf((my0 + by - OY) * K) + s->oy;
    if (Y < s->cy0 || Y > s->cy1 || Y < s->y0 || Y >= s->y0 + s->h)
      continue;
    uint16_t *row = s->buf + (Y - s->y0) * s->w;
    for (int bx = 0; bx < mw; bx++)
    {
      int X = lroundf((mx0 + bx - OX) * K) + s->ox;
      if (X < s->cx0 || X > s->cx1)
        continue;
      int r = 0, g = 0, b = 0, n = 0;
      for (int j = 0; j < Z; j++)
      {
        const uint16_t *p = miniBuf + (by * Z + j) * MW + bx * Z;
        for (int i = 0; i < Z; i++)
          if (p[i] != MKEY)
          {
            r += p[i] >> 11;
            g += (p[i] >> 5) & 63;
            b += p[i] & 31;
            n++;
          }
      }
      if (!n)
        continue;
      uint32_t c = ((r / n) << 19) | ((g / n) << 10) | ((b / n) << 3);
      row[X] = n == Z * Z ? d2::c565(c) : d2::blend(row[X], c, n * 255 / (Z * Z));
    }
  }
}

// ============ 動くもの ============
float lightAt(float x, float y)
{
  if (tod == DAY)
    return 0;
  const Scene &sc = SC();
  float l = 0.2f;
  for (int i = 0; i < sc.nLamp; i++)
  {
    const Lamp &m = sc.lamp[i];
    float dx = (x - m.x) / m.r, dy = (y - m.y) * 2.5f / m.r;
    l = max(l, 1 - sqrtf(dx * dx + dy * dy));
  }
  for (int i = 0; i < sc.nBright; i++)
  {
    const Box &b = sc.bright[i];
    if (x > b.x && x < b.x + b.w && y > b.y && y < b.y + b.h)
      l = max(l, 0.6f);
  }
  return l;
}

// 頭 (顔は横向き)。(hx, hy) が頭の真ん中
static void head(float hx, float hy, int d, const Look &lk)
{
  uint32_t sk = SKIN[lk.skin & 3], hc = HAIR[lk.hair % 5];
  if (lk.hairKind == 1) // 長い髪 (後ろに垂れる)
    Q(hx - d * 0.4f, hy - 1, hx - d * 1.5f, hy - 0.6f, hx - d * 1.6f, hy + 2.6f, hx - d * 0.2f, hy + 2.2f, hc);
  C(hx, hy, 1.3f, sk);
  C(hx - d * 0.25f, hy + 0.15f, 0.32f, shade(sk, 0.8f)); // 耳
  T(hx + d * 1.1f, hy - 0.3f, hx + d * 1.65f, hy + 0.3f, hx + d * 1.1f, hy + 0.45f, sk); // 鼻
  if (lk.hairKind != 2)
  {
    E(hx - d * 0.15f, hy - 0.75f, 1.38f, 0.75f, hc);
    Q(hx - d * 1.35f, hy - 0.7f, hx - d * 0.3f, hy - 0.7f, hx - d * 0.5f, hy + 0.6f, hx - d * 1.3f, hy + 0.4f, hc);
  }
  C(hx + d * 0.72f, hy - 0.15f, 0.2f, 0x1a1a1a); // 目
  R(hx + d * 0.55f - 0.25f, hy + 0.75f, 0.5f, 0.15f, shade(sk, 0.6f));
  if (lk.glasses)
  {
    float gx = hx + d * 0.72f;
    R(gx - 0.5f, hy - 0.5f, 1.0f, 0.75f, 0x101010);
    R(gx - 0.32f, hy - 0.33f, 0.64f, 0.42f, 0x9ab8c8, 200);
    L(gx - d * 0.5f, hy - 0.25f, hx - d * 0.25f, hy - 0.1f, 0.2f, 0x101010);
  }
  if (lk.hat)
  {
    uint32_t c = HAT_C[lk.hat];
    if (lk.hat == H_BEIGE) // 中折れ帽
    {
      E(hx, hy - 1.05f, 2.4f, 0.38f, shade(c, 0.85f));
      Q(hx - 1.25f, hy - 1.1f, hx + 1.25f, hy - 1.1f, hx + 1.05f, hy - 2.5f, hx - 1.05f, hy - 2.5f, c);
      R(hx - 1.25f, hy - 1.55f, 2.5f, 0.42f, 0x3a2a20);
      R(hx - 0.3f, hy - 2.55f, 0.6f, 0.3f, shade(c, 0.75f));
    }
    else // つば付きの帽子 (つばは前)
    {
      E(hx - d * 0.05f, hy - 0.95f, 1.45f, 0.8f, c);
      R(hx - 1.45f, hy - 0.95f, 2.9f, 0.5f, c);
      Q(hx + d * 0.5f, hy - 0.55f, hx + d * 2.3f, hy - 0.4f, hx + d * 2.3f, hy - 0.15f, hx + d * 0.5f, hy - 0.15f, shade(c, 0.78f));
      E(hx - d * 0.3f, hy - 1.3f, 0.6f, 0.3f, shade(c, 1.2f));
      C(hx, hy - 1.75f, 0.2f, shade(c, 0.7f));
    }
  }
}

void drawPerson(float x, float y, int d, float ph, bool walk, const Look &lk, float down, int fd, bool shadow)
{
  lit = lightAt(x, y);
  uint32_t coat = COAT_C[min((int)lk.coat, (int)CO_SIL)], pants = PANTS[lk.pants & 3], sk = SKIN[lk.skin & 3];
  if (shadow && tod != NIGHT && down < 0.6f)
  {
    float sl = tod == DUSK ? 3.2f : 1.3f;
    E(x + sl, y + 0.05f, 1.6f + sl, 0.5f, 0x000000, tod == DUSK ? 50 : 70);
  }
  if (down >= 0.6f) // 倒れている
  {
    float k = min(1.0f, (down - 0.6f) / 0.4f);
    float gy = y - 0.6f;
    E(x + fd * 1, y + 0.1f, 6, 0.6f, 0x000000, 60);
    L(x - fd * 3.6f, gy, x - fd * 0.4f, gy - 0.2f, 1.0f, pants);
    L(x - fd * 3.4f, gy + 0.5f, x - fd * 0.6f, gy + 0.1f, 0.9f, shade(pants, 0.8f));
    E(x - fd * 4, gy + 0.3f, 0.7f, 0.4f, 0x1a1a1a);
    Q(x - fd * 0.6f, gy - 1.3f, x + fd * 3.4f, gy - 1.4f, x + fd * 3.4f, gy + 0.4f, x - fd * 0.6f, gy + 0.5f, coat);
    if (lk.bag)
      R(x + fd * 0.6f - 1, gy + 0.2f, 2.2f, 1.2f, BAG_C[lk.bag]);
    C(x + fd * 4.7f, gy - 0.3f - (1 - k) * 0.6f, 1.25f, sk);
    E(x + fd * 4.9f, gy - 0.9f, 1.1f, 0.6f, HAIR[lk.hair % 5]);
    if (lk.glasses)
      R(x + fd * 4.7f - 0.5f, gy - 0.6f, 1, 0.6f, 0x101010);
    if (lk.hat)
      E(x + fd * 7.2f, gy + 0.1f, 1.4f, 0.55f, HAT_C[lk.hat]); // 帽子が転がる
    lit = 0;
    return;
  }
  if (down > 0) // 倒れはじめ: 上ほど倒れる向きへずらす
  {
    SH = -fd * down * 1.4f;
    SY = y;
    y += down * 2;
  }
  float s = walk ? sinf(ph) : 0, sc = walk ? cosf(ph) : 0;
  float bob = walk ? fabsf(sc) * 0.25f : 0;
  float hipY = y - 4.2f + bob, shY = y - 7.7f + bob;
  // 後ろの脚と腕
  float fA = x + s * 1.3f, fB = x - s * 1.3f;
  if (lk.skirt)
  {
    L(x - 0.3f, hipY + 1.6f, fB, y - 0.3f, 0.6f, shade(sk, 0.85f));
    L(x + 0.3f, hipY + 1.6f, fA, y - 0.3f, 0.6f, sk);
  }
  else
    L(x - 0.25f, hipY, fB, y - 0.4f, 1.0f, shade(pants, 0.78f));
  E(fB + d * 0.35f, y - 0.3f, 0.75f, 0.35f, 0x141414);
  L(x - d * 0.2f, shY + 0.4f, x - s * 1.2f, hipY - 0.2f, 0.75f, shade(coat, 0.72f));
  C(x - s * 1.2f, hipY, 0.42f, shade(sk, 0.85f));
  // 前の脚
  if (!lk.skirt)
    L(x + 0.25f, hipY, fA, y - 0.4f, 1.0f, pants);
  E(fA + d * 0.35f, y - 0.3f, 0.75f, 0.35f, 0x1a1a1a);
  // 胴 (長いコートは膝まで)
  float bot = lk.longCoat ? y - 2.0f : lk.skirt ? hipY + 1.8f : hipY + 0.2f;
  float wb = lk.longCoat || lk.skirt ? 1.75f : 1.3f;
  Q(x - 1.45f, shY, x + 1.45f, shY, x + wb, bot, x - wb, bot, coat);
  Q(x - d * 1.45f, shY, x - d * 0.4f, shY, x - d * 0.5f, bot, x - d * wb, bot, shade(coat, 0.82f)); // 背中側の陰
  L(x + d * 1.3f, shY + 0.3f, x + d * (wb - 0.1f), bot, 0.2f, shade(coat, 1.15f));               // 前の縁の光
  T(x + d * 0.2f, shY, x + d * 1.1f, shY, x + d * 0.75f, shY + 1.1f, shade(coat, 1.25f));            // えり
  if (!lk.longCoat && !lk.skirt)
    R(x - 1.3f, hipY - 0.35f, 2.6f, 0.35f, shade(pants, 0.6f));
  else if (lk.longCoat)
    for (float by = shY + 1.4f; by < bot - 0.5f; by += 1.4f)
      C(x + d * 0.85f, by, 0.16f, shade(coat, 0.55f));
  // かばん (肩からかけて、手前の腰に)
  if (lk.bag)
  {
    uint32_t bc = BAG_C[lk.bag];
    float bx = x - d * 0.9f + s * 0.25f, by = hipY - 1.2f;
    L(x + d * 0.7f, shY + 0.2f, bx, by, 0.25f, shade(bc, 0.8f));
    R(bx - 1.0f, by, 2.0f, 2.2f, bc);
    R(bx - 1.0f, by, 2.0f, 0.7f, shade(bc, 1.25f));
    R(bx - 0.15f, by + 0.6f, 0.3f, 0.3f, 0xc8a850);
  }
  // 前の腕 (傘を持つ人は胸の前に上げる)
  float hx = lk.umbrella ? x + d * 0.9f : x + s * 1.25f, hy = lk.umbrella ? shY + 0.6f : hipY + 0.15f;
  L(x + d * 0.1f, shY + 0.4f, hx, hy - 0.2f, 0.8f, coat);
  C(hx, hy, 0.45f, sk);
  // 首と頭
  R(x - 0.35f, shY - 0.65f, 0.7f, 0.8f, shade(sk, 0.85f));
  head(x + d * 0.1f, shY - 1.65f, d, lk);
  if (lk.umbrella) // 傘: 柄と、骨で区切った丸い布
  {
    static const uint32_t UMB[7] = {0, 0x1a1a1d, 0xdcdcd8, 0xc8382f, 0x2c3e6b, 0xd8b23c, 0x6a3a7a};
    uint32_t uc = UMB[lk.umbrella % 7];
    float ax = hx - d * 0.2f, ay = shY - 5.6f + bob * 0.5f;
    L(hx, hy + 0.3f, ax, ay, 0.22f, 0x2a2a2a);
    const int N = 6;
    for (int i = 0; i < N; i++)
    {
      float u0 = -1 + 2.0f * i / N, u1 = -1 + 2.0f * (i + 1) / N;
      float x0 = ax + u0 * 4.0f, x1 = ax + u1 * 4.0f;
      float y0 = ay + 1.0f + (1 - u0 * u0) * -1.4f + 1.4f, y1 = ay + 1.0f + (1 - u1 * u1) * -1.4f + 1.4f;
      T(ax, ay - 0.2f, x0, y0, x1, y1, i % 2 ? uc : shade(uc, 0.85f));
      E((x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, 0.3f, i % 2 ? uc : shade(uc, 0.85f));
    }
    L(ax, ay - 0.2f, ax, ay - 1, 0.2f, 0x2a2a2a);
    if (tod != DAY && uc == 0xdcdcd8)
      E(ax, ay + 1.4f, 3.8f, 0.6f, 0xffffff, 40);
  }
  SH = 0;
  lit = 0;
}

void drawWinPerson(float x, float y, float w, float h, float u, int d, const Look &lk, bool litWin)
{
  float px = winPersonX(x, w, u), b = y + h;
  uint32_t coat = COAT_C[min((int)lk.coat, (int)CO_SIL)];
  lit = tod == DAY ? 0 : litWin ? 0.6f : 0.25f;
  Q(px - 2.1f, b - 4.4f, px + 2.1f, b - 4.4f, px + 2.4f, b, px - 2.4f, b, coat);
  Q(px - d * 2.1f, b - 4.4f, px - d * 0.6f, b - 4.4f, px - d * 0.7f, b, px - d * 2.4f, b, shade(coat, 0.8f));
  T(px + d * 0.2f, b - 4.4f, px + d * 1.1f, b - 4.4f, px + d * 0.7f, b - 3.2f, shade(coat, 1.25f));
  R(px - 0.35f, b - 5.1f, 0.7f, 0.8f, shade(SKIN[lk.skin & 3], 0.85f));
  head(px + d * 0.1f, b - 6.7f, d, lk);
  lit = 0;
}

float carLen(int k)
{
  static const float LEN[CAR_N] = {22, 22, 24, 19, 46, 30, 8};
  return LEN[k];
}
float carTop(int k)
{
  static const float TOP[CAR_N] = {0.6f, 0.6f, -1, 0.6f, -6, -5, -6};
  return TOP[k];
}

// 路線バス: 2 色の車体、窓の列、前の扉、行き先の表示
static void drawBus(float x, float y, int d, uint32_t color, float wheel)
{
  const float len = 46;
  auto X = [&](float u) { return d > 0 ? x + u : x + len - u; };
  auto RX = [&](float u0, float u1, float yy, float hh, uint32_t c, int a = 255) { R(min(X(u0), X(u1)), yy, fabsf(u1 - u0), hh, c, a); };
  bool night = tod != DAY;
  E(x + len / 2 + (night ? 0 : 2), y + 10.6f, len / 2 + 2, 1.2f, 0x000000, 80);
  RX(0, len, y - 5, 14, 0xe8e4d8);
  RX(0, len, y + 3.6f, 5.6f, color);
  RX(0, len, y + 3.2f, 0.5f, shade(color, 1.3f));
  RX(0.6f, len - 0.6f, y - 6.2f, 1.4f, 0xd8d4c8);
  RX(12, 26, y - 7.4f, 1.4f, 0xb8b8b4); // 屋根の冷房
  for (int i = 0; i < 7; i++)
  {
    float u = 2 + i * 5.2f;
    RX(u, u + 4.4f, y - 3.4f, 5.4f, night ? (0xd8e8d8 | EMIT) : 0x7a90a0);
    if (night && hh(i, x) < 0.6f)
      C(X(u + 2.2f), y - 0.4f, 0.9f, 0x3a3a40);
    else if (!night)
      L(X(u + 0.6f), y + 1.6f, X(u + 2.2f), y - 3, 0.25f, 0xffffff, 110);
  }
  RX(38.4f, 43.6f, y - 3.6f, 12.2f, night ? (0xc8d8d0 | EMIT) : 0x8aa0b0); // 扉
  L(X(41), y - 3.6f, X(41), y + 8.6f, 0.25f, 0x3a3a3a);
  Q(X(44), y - 5, X(46), y - 2, X(46), y + 3, X(44), y + 3, night ? 0x1e2630 : 0x9fb8c8); // 前の窓
  RX(39, 45.4f, y - 5.2f, 1.6f, 0x1a1a1a);
  sign(min(X(39.6f), X(45)) + 0.2f, y - 5, 0.2f, "CITY", 0xffa030 | EMIT);
  RX(len - 0.8f, len, y + 5.2f, 1.4f, night ? (0xfff8e0 | EMIT) : 0xe8e8d8);
  RX(0, 0.8f, y + 5, 1.6f, night ? (0xff3020 | EMIT) : 0xb02020);
  RX(-0.3f, len + 0.3f, y + 8.4f, 0.9f, 0x2a2a2e);
  for (float u : {8.0f, 36.0f})
  {
    float cx = X(u);
    C(cx, y + 8.8f, 2.8f, 0x141414);
    C(cx, y + 9, 2.2f, 0x1e1e1e);
    C(cx, y + 9, 1.2f, 0xa8acb0);
    float a = wheel * d;
    L(cx - cosf(a), y + 9 - sinf(a), cx + cosf(a), y + 9 + sinf(a), 0.25f, 0x6a6e72);
  }
  if (night)
    T(X(len), y + 5.8f, X(len) + d * 22, y + 3, X(len) + d * 22, y + 11, 0xfff2c0 | EMIT, 40);
}

// 港のトラック: 前に運転台、後ろに荷台の箱
static void drawTruck(float x, float y, int d, uint32_t color, float wheel)
{
  const float len = 30;
  auto X = [&](float u) { return d > 0 ? x + u : x + len - u; };
  auto RX = [&](float u0, float u1, float yy, float hh, uint32_t c, int a = 255) { R(min(X(u0), X(u1)), yy, fabsf(u1 - u0), hh, c, a); };
  bool night = tod != DAY;
  E(x + len / 2 + (night ? 0 : 2), y + 10.6f, len / 2 + 2, 1.2f, 0x000000, 80);
  RX(0, 20.5f, y - 5, 12.6f, 0xd8d8d0); // 荷台
  for (float u = 2; u < 20; u += 2.5f)
    RX(u, u + 0.3f, y - 4.6f, 11.8f, 0xb8b8b0);
  RX(0, 20.5f, y - 5, 0.6f, 0xf0f0e8);
  RX(2, 18, y - 1, 3, color);
  sign(min(X(3), X(17)) + 0.6f, y - 0.4f, 0.3f, "PORT EXPRESS", 0xffffff);
  RX(21, 28, y - 2, 11, color); // 運転台
  Q(X(28), y - 2, X(29.6f), y + 1.4f, X(29.6f), y + 9, X(28), y + 9, color);
  RX(23, 27.4f, y - 1, 4, night ? 0x1e2630 : 0x9fb8c8);
  RX(-0.3f, len, y + 7.8f, 1.2f, 0x2a2a2e);
  RX(29, 29.8f, y + 4.4f, 1.2f, night ? (0xfff8e0 | EMIT) : 0xe8e8d8);
  for (float u : {5.0f, 9.5f, 25.0f})
  {
    float cx = X(u);
    C(cx, y + 9, 2.2f, 0x141414);
    C(cx, y + 9, 1.1f, 0x9a9ea2);
    float a = wheel * d;
    L(cx - cosf(a), y + 9 - sinf(a), cx + cosf(a), y + 9 + sinf(a), 0.25f, 0x5a5e62);
  }
  if (night)
    T(X(len), y + 5, X(len) + d * 20, y + 2.5f, X(len) + d * 20, y + 10, 0xfff2c0 | EMIT, 40);
}

// 公園の自転車と、こいでいる人
static void drawBike(float x, float y, int d, uint32_t color, float ph, const Look &lk)
{
  auto X = [&](float u) { return d > 0 ? x + u : x + 8 - u; };
  uint32_t coat = COAT_C[min((int)lk.coat, (int)CO_SIL)], pants = PANTS[lk.pants & 3], sk = SKIN[lk.skin & 3];
  lit = lightAt(x + 4, y + 9);
  if (tod != NIGHT)
    E(x + 5, y + 9.8f, 5, 0.5f, 0x000000, 60);
  float wy = y + 7.4f;
  ring(X(1.4f), wy, 2.2f, 0.3f, 0x1a1a1a);
  ring(X(6.6f), wy, 2.2f, 0.3f, 0x1a1a1a);
  float cx = X(3.8f), cyy = wy;
  L(X(1.4f), wy, cx, cyy, 0.35f, color);
  L(cx, cyy, X(3.0f), y + 3.2f, 0.35f, color);
  L(X(3.0f), y + 3.2f, X(6.0f), y + 3.4f, 0.35f, color);
  L(X(6.0f), y + 3.4f, cx, cyy, 0.35f, color);
  L(X(6.0f), y + 3.4f, X(6.6f), wy, 0.35f, color);
  L(X(1.4f), wy, X(3.0f), y + 3.2f, 0.3f, color);
  L(X(6.0f), y + 3.4f, X(6.3f), y + 2.0f, 0.3f, 0x2a2a2a);
  // 脚 (ペダルを回す)
  float hipX = X(3.1f), hipY = y + 2.4f;
  for (int k = 0; k < 2; k++)
  {
    float a = ph + k * 3.1416f;
    float fx = cx + cosf(a) * 1.1f * d, fy = cyy + sinf(a) * 1.1f;
    float kx = (hipX + fx) / 2 + d * 1.0f, ky = (hipY + fy) / 2 - 0.6f;
    uint32_t pc = k ? shade(pants, 0.8f) : pants;
    L(hipX, hipY, kx, ky, 0.9f, pc);
    L(kx, ky, fx, fy, 0.8f, pc);
    E(fx + d * 0.3f, fy, 0.6f, 0.3f, 0x1a1a1a);
  }
  // 体と腕と頭
  float shX = X(4.8f), shY = y - 1.6f;
  L(hipX, hipY, shX, shY, 2.3f, coat);
  L(hipX - d * 0.6f, hipY, shX - d * 0.7f, shY, 0.6f, shade(coat, 0.8f));
  if (lk.bag)
    R(hipX - d * 1.4f - 0.9f, y - 0.4f, 1.8f, 2, BAG_C[lk.bag]);
  L(shX, shY + 0.3f, X(6.2f), y + 2.0f, 0.7f, coat);
  C(X(6.2f), y + 2.0f, 0.4f, sk);
  head(X(5.4f), y - 3.4f, d, lk);
  lit = 0;
}

void drawCar(float x, float y, int d, int kind, uint32_t color, float wheel, const Look *rider, float ph)
{
  if (kind == CAR_BUS)
  {
    drawBus(x, y, d, color, wheel);
    return;
  }
  if (kind == CAR_TRUCK)
  {
    drawTruck(x, y, d, color, wheel);
    return;
  }
  if (kind == CAR_BIKE)
  {
    if (rider)
      drawBike(x, y, d, color, ph, *rider);
    return;
  }
  float len = carLen(kind);
  auto X = [&](float u) { return d > 0 ? x + u : x + len - u; };
  bool night = tod != DAY;
  if (!night)
    E(x + len / 2 + 1.5f, y + 10.9f, len / 2 + 1.5f, 1.1f, 0x000000, 80);
  else
    E(x + len / 2, y + 10.9f, len / 2 + 1, 0.9f, 0x000000, 60);
  uint32_t glass = night ? 0x1e2630 : 0x9fb8c8;
  float roofY = kind == CAR_VAN ? y - 1 : y + 0.6f;
  // 車体
  if (kind == CAR_VAN)
  {
    Q(X(0), y - 1, X(19), y - 1, X(19), y + 9.2f, X(0), y + 9.2f, color);
    Q(X(19), y - 1, X(20.5f), y - 1, X(24), y + 4.6f, X(19), y + 4.6f, color);
    Q(X(19), y + 4.6f, X(24), y + 4.6f, X(24), y + 9.2f, X(19), y + 9.2f, color);
    Q(X(19.4f), y - 0.2f, X(20.3f), y - 0.2f, X(23.3f), y + 4.2f, X(19.4f), y + 4.2f, glass);
    R(min(X(15), X(18.4f)), y + 0.2f, 3.4f, 3.8f, glass);
    L(X(14.5f), y - 0.6f, X(14.5f), y + 8.8f, 0.2f, shade(color, 0.7f));
    L(X(8), y - 0.6f, X(8), y + 8.8f, 0.2f, shade(color, 0.7f));
    R(min(X(1), X(13)), y + 1.2f, 12, 0.8f, 0x2a6a9a); // 横の帯
  }
  else
  {
    float c0 = kind == CAR_HATCH ? 3.5f : 5, c1 = kind == CAR_HATCH ? 4.2f : 7.5f, c2 = kind == CAR_HATCH ? 13 : 14, c3 = kind == CAR_HATCH ? 15.5f : 17;
    Q(X(0.4f), y + 4.4f, X(len - 0.6f), y + 4.4f, X(len), y + 9.2f, X(0), y + 9.2f, color);
    R(min(X(0), X(len)), y + 5.4f, len, 3.8f, color);
    Q(X(c0), y + 4.6f, X(c1), roofY, X(c2), roofY, X(c3), y + 4.6f, color);
    Q(X(c0 + 1.1f), y + 4.2f, X(c1 + 0.7f), roofY + 0.7f, X((c1 + c2) / 2 - 0.3f), roofY + 0.7f, X((c1 + c2) / 2 - 0.3f), y + 4.2f, glass);
    Q(X((c1 + c2) / 2 + 0.4f), y + 4.2f, X((c1 + c2) / 2 + 0.4f), roofY + 0.7f, X(c2 - 0.4f), roofY + 0.7f, X(c3 - 1.1f), y + 4.2f, glass);
    if (!night)
      L(X(c1 + 1.5f), y + 3.8f, X(c1 + 2.8f), roofY + 1.2f, 0.3f, 0xffffff, 120);
    L(X((c1 + c2) / 2 + 0.05f), y + 4.4f, X((c1 + c2) / 2 + 0.05f), y + 8.8f, 0.2f, shade(color, 0.65f));
    L(X(c3 - 0.5f), y + 4.6f, X(c3 + 0.2f), y + 8.8f, 0.2f, shade(color, 0.65f));
    R(min(X((c1 + c2) / 2 + 1.2f), X((c1 + c2) / 2 + 2.4f)), y + 5.6f, 1.2f, 0.35f, shade(color, 0.6f));
    R(min(X(c0 + 1.5f), X(c0 + 2.7f)), y + 5.6f, 1.2f, 0.35f, shade(color, 0.6f));
    if (kind == CAR_TAXI)
    {
      R(min(X(9), X(12.4f)), roofY - 1.4f, 3.4f, 1.4f, night ? (0xfff4c0 | EMIT) : 0xf4f0e0);
      for (int i = 0; i < 9; i++)
        R(min(X(3 + i * 2), X(5 + i * 2)), y + 6.8f, 1, 0.7f, i % 2 ? 0x1a1a1a : 0xf0f0f0);
    }
  }
  R(min(X(0), X(len)), y + 4.6f, len, 0.45f, shade(color, 1.25f)); // 上の縁の光
  R(min(X(0), X(len)), y + 8.2f, len, 1.0f, shade(color, 0.7f));
  R(min(X(-0.3f), X(len + 0.3f)), y + 8.6f, len + 0.6f, 0.7f, 0x2a2a2e); // バンパー
  // 灯り
  R(min(X(len - 0.9f), X(len)), y + 5.2f, 0.9f, 1.2f, night ? (0xfff8e0 | EMIT) : 0xe8e8d8);
  R(min(X(0), X(0.8f)), y + 5.4f, 0.8f, 1.2f, night ? (0xff3020 | EMIT) : 0xb02020);
  if (night)
  {
    float hx = X(len);
    T(hx, y + 5.4f, hx + d * 20, y + 3, hx + d * 20, y + 10.5f, 0xfff2c0 | EMIT, 40);
    T(hx, y + 5.6f, hx + d * 12, y + 4.4f, hx + d * 12, y + 8.6f, 0xfff2c0 | EMIT, 40);
    C(X(0), y + 6, 1.6f, 0xff3020 | EMIT, 60);
  }
  // 車輪
  float wl = kind == CAR_VAN ? 5 : 4.5f, wr = len - (kind == CAR_VAN ? 4.5f : 4.5f);
  for (float u : {wl, wr})
  {
    float cx = X(u);
    C(cx, y + 9.2f, 2.3f, 0x141414);
    C(cx, y + 9.4f, 1.75f, 0x1e1e1e);
    C(cx, y + 9.4f, 1.0f, 0xa8acb0);
    for (int k = 0; k < 2; k++)
    {
      float a = wheel * d + k * 1.5708f;
      L(cx - cosf(a) * 0.9f, y + 9.4f - sinf(a) * 0.9f, cx + cosf(a) * 0.9f, y + 9.4f + sinf(a) * 0.9f, 0.2f, 0x6a6e72);
    }
    C(cx, y + 9.4f, 0.3f, 0x4a4e52);
  }
  // ミラー
  float mu = kind == CAR_VAN ? 18.6f : kind == CAR_HATCH ? 12.4f : 14.2f;
  R(min(X(mu), X(mu + 0.8f)), y + 3.6f, 0.8f, 0.7f, shade(color, 0.8f));
}

void drawPigeon(float x, float y, int d, bool fly, float ph, bool gull)
{
  uint32_t body = gull ? 0xf2f2ee : 0x7a7f8a, wing = gull ? 0xa8b0b8 : shade(body, 0.85f), headc = gull ? 0xf8f8f4 : 0x5a6070;
  float k = gull ? 1.35f : 1.0f;
  if (!fly)
  {
    if (tod == DAY)
      E(x + 0.4f, y + 0.05f, 1.1f * k, 0.25f, 0x000000, 60);
    T(x - d * 0.6f * k, y - 1.1f * k, x - d * 1.9f * k, y - 1.0f * k, x - d * 0.6f * k, y - 0.5f * k, gull ? 0x2a2a2a : shade(body, 0.7f));
    E(x, y - 0.95f * k, 1.05f * k, 0.6f * k, body);
    E(x - d * 0.2f * k, y - 1.1f * k, 0.7f * k, 0.35f * k, wing);
    float bob = sinf(ph) * 0.3f;
    C(x + d * 0.85f * k, y - 1.6f * k + bob, 0.45f * k, headc);
    if (!gull)
      C(x + d * 0.6f, y - 1.2f, 0.32f, 0x4a8a6a);
    R(min(x + d * 1.25f * k, x + d * 1.6f * k), y - 1.65f * k + bob, 0.35f * k, 0.18f * k, gull ? 0xe8c030 : 0xd8c8a0);
    L(x, y - 0.4f, x, y, 0.15f, gull ? 0xe8a040 : 0xc87070);
  }
  else
  {
    float w = sinf(ph) * 1.8f * k;
    E(x, y, 1.0f * k, 0.45f * k, body);
    T(x - 0.4f * k, y - 0.1f, x + 0.5f * k, y - 0.1f, x - 0.3f * d * k, y - w, shade(wing, 1.1f));
    T(x - 0.4f * k, y + 0.1f, x + 0.5f * k, y + 0.1f, x + 0.2f * d * k, y - w * 0.6f + 0.6f, shade(wing, 0.8f));
    if (gull)
      C(x - 0.3f * d * k, y - w, 0.3f, 0x2a2a2a);
    C(x + d * 0.9f * k, y - 0.25f, 0.38f * k, headc);
    T(x - d * 0.8f * k, y, x - d * 1.7f * k, y - 0.4f, x - d * 1.7f * k, y + 0.4f, shade(body, 0.8f));
  }
}

// 旗: 竿の先 (px, py) から風下へなびく。弱い風では垂れ、強いと横に張って速くはためく
void drawFlag(float px, float py, float len, float wind, int t, uint32_t c1, uint32_t c2, float seed)
{
  int dir = wind >= 0 ? 1 : -1;
  float aw = fabsf(wind), ext = constrain(0.3f + aw * 0.13f, 0.3f, 1.0f), h = len * 0.6f;
  float fr = 0.12f + aw * 0.035f;
  const int N = 10;
  float px0 = px, top0 = py, x1, top1;
  for (int i = 1; i <= N; i++)
  {
    float u = (float)i / N;
    float wv = sinf(t * fr * 2 - u * 5.5f + seed) * u * (0.4f + aw * 0.12f);
    x1 = px + dir * u * len * ext;
    top1 = py + (1 - ext) * u * len * 0.95f + wv;
    float b0 = top0 + h, b1 = top1 + h;
    float sl = cosf(t * fr * 2 - u * 5.5f + seed);
    uint32_t a = shade(c1, 0.82f + 0.2f * sl), b = shade(c2, 0.82f + 0.2f * sl);
    float m0 = top0 + h * 0.5f, m1 = top1 + h * 0.5f;
    Q(px0, top0, x1, top1, x1, m1, px0, m0, a);
    Q(px0, m0, x1, m1, x1, b1, px0, b0, b);
    px0 = x1;
    top0 = top1;
  }
}


// 街路樹: 葉のかたまりを重ねて、風でゆれる。(x, cy) が葉の真ん中、sc は大きさ
void drawTree(float x, float cy, float sc, float wind, int t, int i)
{
  float aw = fabsf(wind), dir = wind >= 0 ? 1 : -1;
  float sw = (sinf(t * (0.05f + aw * 0.01f) + i * 2.1f) * min(1.4f, aw * 0.25f) + dir * aw * 0.12f) * sc;
  uint32_t d0 = 0x2f5a2c, d1 = 0x3f7a3a, d2c = 0x5a9a48, d3 = 0x8ac06a;
  C(x + sw * 0.5f, cy + 2 * sc, 8 * sc, d0);
  static const float BL[][3] = {{-5, 1, 4.6f}, {5, 1.5f, 4.4f}, {-2, -3.5f, 5}, {3, -3, 4.6f}, {0, 3.5f, 4.8f}, {-6, -1, 3.4f}, {6.5f, -1.5f, 3.2f}};
  for (int k = 0; k < 7; k++)
  {
    float bx = x + BL[k][0] * sc + sw * (0.6f - BL[k][1] * 0.05f), by = cy + BL[k][1] * sc;
    C(bx, by, BL[k][2] * sc, d1);
    C(bx - sc, by - 1.2f * sc, BL[k][2] * 0.6f * sc, d2c);
  }
  for (int k = 0; k < 26; k++) // 葉の明るい点
  {
    float a = d2::hs(i * 50 + k) * 6.283f, r = d2::hs(i * 50 + k + 0.5f) * 7.5f * sc;
    float lx = x + cosf(a) * r + sw * 0.8f + sinf(t * 0.2f + k) * 0.15f * aw, ly = cy + sinf(a) * r * 0.8f - 0.5f;
    C(lx, ly, 0.55f * sc, k % 3 ? d3 : d2c);
  }
}

// 雨: 風に流される筋と、地面のはね
void drawRain(int t, float wind, float amt)
{
  int n = (int)(160 * amt);
  float sl = wind * 0.35f;
  for (int i = 0; i < n; i++)
  {
    float sp = 7 + hh(i, 1) * 3;
    float y = fmodf(hh(i, 2) * 330 + t * sp, 330) - 15;
    float x = fmodf(hh(i, 3) * 280 + t * sl + 1000, 280) - 20;
    float l = 3.5f + hh(i, 4) * 2;
    L(x, y, x + sl * l / sp, y + l, 0.12f, 0xb8c8e0 | EMIT, 70 + (int)(hh(i, 5) * 60));
  }
  for (int i = 0; i < (int)(40 * amt); i++) // 地面のはね (歩道と車道)
  {
    int ph = (t + i * 7) % 12;
    if (ph > 5)
      continue;
    float x = hh(i, 6, t / 12) * 240, y = 200 + hh(i, 7, t / 12) * 98;
    E(x, y, 0.3f + ph * 0.25f, 0.12f + ph * 0.06f, 0xc8d8f0 | EMIT, 120 - ph * 20);
  }
}

extern const Scene HOTEL_SCENE, STATION_SCENE, HARBOR_SCENE, RAIN_SCENE, PARK_SCENE;
const Scene *const SCENES[NSCENE] = {&HOTEL_SCENE, &STATION_SCENE, &HARBOR_SCENE, &RAIN_SCENE, &PARK_SCENE};
} // namespace w3