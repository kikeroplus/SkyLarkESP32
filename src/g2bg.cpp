// 2 本目 (廃工場) の背景の区画。デザイン案のページ (foundry-design.html) の描き方をそのまま移したもの
#include "g2bg.h"
#include <esp_heap_caps.h>

using namespace d2;

namespace bg2
{
static const int W = 240;

// ---- 色 ----
static const uint32_t CONC[4] = {0x47484a, 0x4f5052, 0x57585a, 0x5f6062};
static const uint32_t DARK[4] = {0x1f2123, 0x242628, 0x2a2c2e, 0x303234};
static const uint32_t OUTP[4] = {0x46473f, 0x4e4f45, 0x56574b, 0x5e5e51};
static const uint32_t WARM[4] = {0x433f34, 0x4a4639, 0x524d3e, 0x5a5443};
static const uint32_t WATER[4] = {0x0c1d1a, 0x0f2420, 0x132a25, 0x17302a};
static const uint32_t YEL = 0xc9a227, YELD = 0x7a6420, LIGHT = 0xffecc4;

enum
{
  SILVER,
  RUST,
  GREEN,
  RED,
  YELLOW,
  LAG,
  BLACK,
  BLUE,
  NPP
};
static const uint32_t PP[NPP][4] = {
    {0x33363a, 0x7d8388, 0xc4c9cc, 0xeef1f2}, {0x2e170d, 0x6b3a1e, 0xa8683a, 0xd09a6a}, {0x1f3330, 0x3f6f6a, 0x6fa39b, 0xb8d8d0},
    {0x3a1410, 0x7a2e22, 0xb4614c, 0xe8a898}, {0x3a3010, 0x8a7020, 0xc9a227, 0xf0dc90}, {0x5a5650, 0xa8a498, 0xd4d0c4, 0xf2efe6},
    {0x0e0f10, 0x2a2c2e, 0x4f5357, 0x8a8f94}, {0x173455, 0x2f5f96, 0x6a9ed2, 0xcfe3f5},
};
static int A(float a) { return (int)lroundf(constrain(a, 0.0f, 1.0f) * 255); }
static float fmod1(float v) { return v - floorf(v); }

// ---- 部品 ----
static void tex(float x, float y, int w, int h, const uint32_t *pal, int n, float sc, float seed)
{
  for (int yy = 0; yy < h; yy += 4)
    for (int xx = 0; xx < w; xx += 4)
    {
      float X = x + xx, Y = y + yy;
      float v = vn(X * sc + seed * 7.3f, Y * sc + seed * 3.1f) * 0.65f + h2(X + seed * 31, Y) * 0.35f;
      int i = constrain((int)floorf((v - 0.18f) / 0.64f * n), 0, n - 1);
      R(X, Y, min(4, w - xx), min(4, h - yy), pal[i]);
    }
}
static void specks(float x, float y, float w, float h, int n, float seed, uint32_t c0, uint32_t c1)
{
  for (int i = 0; i < n; i++)
    P(x + hs(seed + i * 1.1f) * w, y + hs(seed + i * 2.3f) * h, i % 2 ? c1 : c0);
}
static void cracks(float x, float y, float w, float h, int n, float seed, uint32_t c)
{
  for (int i = 0; i < n; i++)
  {
    float px = x + hs(seed + i * 1.7f) * w, py = y + hs(seed + i * 2.9f) * h, a = hs(seed + i * 4.1f) * 6.28f;
    for (int k = 0; k < 8; k++)
    {
      a += (hs(seed + i * 13 + k) - 0.5f) * 1.4f;
      float l = 3 + hs(seed + i * 7 + k) * 5, nx = px + cosf(a) * l, ny = py + sinf(a) * l;
      L(px, py, nx, ny, c);
      px = nx;
      py = ny;
    }
  }
}
static void drips(float x, float y, float w, float maxl, int n, float seed)
{
  for (int i = 0; i < n; i++)
  {
    float xx = x + hs(seed + i * 1.9f) * w, l = 2 + hs(seed + i * 3.3f) * maxl;
    R(xx, y, 1, l, 0x6b3a1e);
    P(xx, y, 0xa8683a);
    if (hs(seed + i) > 0.6f)
      R(xx + 1, y + 1, 1, l * 0.6f, 0x4a2a1a);
  }
}
static void tuft(float x, float y, float seed, int kind) // 0 枯れ草 1 緑 2 ススキ
{
  int n = 4 + (int)(hs(seed) * 4);
  static const uint32_t G[3] = {0x4f5f32, 0x6b7d3e, 0x8a9a52}, D[3] = {0x7d6c40, 0x9f8b55, 0xc2ad74};
  const uint32_t *cols = kind == 1 ? G : D;
  P(x + 1, y + 1, 0, 102);
  for (int k = 0; k < n; k++)
  {
    float a = -PIf / 2 + (hs(seed + k * 1.3f) - 0.5f) * 1.7f;
    float l = (kind == 2 ? 9 : 4) + hs(seed + k * 2.1f) * (kind == 2 ? 7 : 4);
    float ex = x + cosf(a) * l, ey = y + sinf(a) * l * 0.85f;
    L(x, y, ex, ey, cols[k % 3]);
    if (kind == 2)
    {
      P(ex, ey, 0xeee4cc);
      P(ex + 1, ey + 1, 0xd8ccb0);
      P(ex - 1, ey + 1, 0xcfc3a4);
      P(ex, ey - 1, 0xf6efdc);
    }
  }
}
static void vine(float x, float y, float len, float seed, int dir)
{
  for (int i = 0; i < len; i++)
  {
    x += (hs(seed + i * 0.7f) - 0.5f) * 1.6f;
    y += dir;
    P(x, y, 0x3b4a26);
    if (i % 3 == 0)
      P(x + (i % 6 ? 1 : -1), y, 0x6b7d3e);
    if (i % 7 == 0)
      P(x + (i % 2 ? 2 : -2), y - 1, 0x8a9a52);
  }
}
static void block(float fx, float fgy, int w, int d, float fhh, uint32_t top, uint32_t front)
{
  int x = lroundf(fx), gy = lroundf(fgy), hh = lroundf(fhh);
  for (int r = 0; r <= hh; r++)
    R(x + w, gy - hh + d + r, lroundf(r * 0.5f) + 1, 1, 0, 82);
  R(x + 2, gy + d, w + lroundf(hh * 0.5f) - 1, 2, 0, 82);
  R(x, gy - hh, w, d, top);
  R(x, gy - hh, w, 1, 0xffffff, 46);
  R(x, gy - hh, 1, d, 0xffffff, 26);
  if (hh > 0)
  {
    R(x, gy - hh + d, w, hh, front);
    R(x, gy - hh + d, w, 1, 0, 77);
  }
  R(x + w - 1, gy - hh, 1, d + hh, 0, 64);
}
static void railH(float x, float y, float w)
{
  R(x + 2, y + 2, w, 1, 0, 89);
  for (float px = x; px <= x + w; px += 10)
    R(px, y - 8, 1, 9, YELD);
  R(x, y - 8, w, 1, YEL);
  R(x, y - 4, w, 1, YELD);
}
static void railV(float x, float y, float h)
{
  R(x + 3, y, 1, h, 0, 89);
  for (float py = y; py < y + h; py += 10)
  {
    R(x + 1, py - 6, 2, 6, YELD);
    P(x + 1, py, 0x3a3018);
  }
  R(x, y - 8, 1, h, YEL);
  R(x - 1, y - 8, 1, h, 0xe2c25a);
}
static void shaft(float x0, float y0, int w, int h, float slope, float a, int gx, int gy)
{
  int al = A(a);
  for (int r = 0; r < h; r++)
  {
    if (gy && (r % gy) < 2)
      continue;
    int xs = lroundf(x0 + r * slope);
    if (gx)
    {
      for (int c = 0; c < w; c += gx)
        R(xs + c, y0 + r, min(gx - 2, w - c), 1, LIGHT, al);
    }
    else
      R(xs, y0 + r, w, 1, LIGHT, al);
  }
}
static void grate(float x, float y, float w, float h)
{
  R(x, y, w, h, 0x121314);
  for (int yy = 0; yy < h; yy += 3)
    R(x, y + yy, w, 1, 0x3e4144);
  for (int xx = 0; xx < w; xx += 6)
    R(x + xx, y, 1, h, 0x2c2f32);
}
static void tube(const Pts &segs, float r, const uint32_t *pal, int seam, int shadowA)
{
  Pts pts = pathPts(segs);
  if (shadowA)
    for (auto &p : pts)
      C(p.x + 5, p.y + 6, r, 0, shadowA);
  for (auto &p : pts)
    C(p.x, p.y, r, pal[0]);
  for (auto &p : pts)
    C(p.x - 1, p.y - 1, r - 2, pal[1]);
  for (auto &p : pts)
    C(p.x - r * 0.35f, p.y - r * 0.35f, max(1.0f, r * 0.4f), pal[2]);
  for (auto &p : pts)
    C(p.x - r * 0.45f, p.y - r * 0.45f, max(0.0f, r * 0.13f), pal[3]);
  if (seam)
    for (size_t i = 1; i + 1 < pts.size(); i += seam)
    {
      float dx = pts[i + 1].x - pts[i - 1].x, dy = pts[i + 1].y - pts[i - 1].y, l = hypotf(dx, dy);
      if (l == 0)
        l = 1;
      float nx = -dy / l, ny = dx / l;
      L(pts[i].x - nx * (r - 1), pts[i].y - ny * (r - 1), pts[i].x + nx * (r - 1), pts[i].y + ny * (r - 1), pal[0]);
    }
}
static void gallery(float x0, float y0, float x1, float y1, float wd, float seed)
{
  float dx = x1 - x0, dy = y1 - y0, l = hypotf(dx, dy), ux = dx / l, uy = dy / l, nx = -uy, ny = ux, hw = wd / 2;
  Pt q[4] = {{x0 + nx * hw + 7, y0 + ny * hw + 9}, {x1 + nx * hw + 7, y1 + ny * hw + 9}, {x1 - nx * hw + 7, y1 - ny * hw + 9}, {x0 - nx * hw + 7, y0 - ny * hw + 9}};
  poly(q, 4, 0, 97);
  float ox = nx * (hw + 4), oy = ny * (hw + 4);
  LT(x0 + ox, y0 + oy, x1 + ox, y1 + oy, 2, 0x4a2a1a);
  for (float s = 0; s < l - 8; s += 10)
  {
    L(x0 + ux * s + ox, y0 + uy * s + oy, x0 + ux * (s + 5) + nx * hw, y0 + uy * (s + 5) + ny * hw, 0x6b3a1e);
    L(x0 + ux * (s + 5) + nx * hw, y0 + uy * (s + 5) + ny * hw, x0 + ux * (s + 10) + ox, y0 + uy * (s + 10) + oy, 0x6b3a1e);
  }
  for (float s = 0; s < l; s += 0.6f)
  {
    float px = x0 + ux * s, py = y0 + uy * s;
    int corr = ((int)(s / 3)) % 2;
    float rust = vn(s * 0.05f, seed) * 0.7f + vn(s * 0.2f, seed + 9) * 0.3f;
    for (int k = -(int)hw; k <= (int)hw; k++)
    {
      float f = (k + hw) / wd;
      bool rr = rust + (hs(lroundf(s) * 3 + k + seed) - 0.5f) * 0.15f > 0.55f;
      uint32_t c;
      if (rr)
        c = f < 0.3f ? 0xa8683a : f < 0.7f ? (corr ? 0x8a4f28 : 0x7a4422) : 0x4a2a1a;
      else
        c = f < 0.3f ? 0xb8ab98 : f < 0.7f ? (corr ? 0x9c8f80 : 0x8a7e70) : 0x5a5248;
      P(px + nx * k, py + ny * k, c);
    }
  }
  LT(x0 - nx * hw, y0 - ny * hw, x1 - nx * hw, y1 - ny * hw, 1, 0xd0c4b0);
  for (int i = 0; i < 5; i++)
  {
    float s = hs(seed + i) * l;
    vine(x0 + ux * s - nx * hw, y0 + uy * s - ny * hw, 16 + hs(seed + i * 3) * 14, seed + i * 11, 1);
  }
}
static void ledge(float seed)
{
  R(0, 0, W, 3, 0x76777a);
  R(0, 3, W, 8, 0x4a4b4d);
  R(0, 11, W, 2, 0, 89);
  drips(0, 3, W, 7, 34, seed);
}
static void fadedHaz(int x, int y, int w, int h)
{
  for (int yy = 0; yy < h; yy++)
    for (int xx = 0; xx < w; xx += 2)
    {
      if (yy > 3 && yy < h - 4 && xx > 3 && xx < w - 5)
        continue;
      if (hs((x + xx) * 3 + (y + yy)) < 0.25f)
        continue;
      R(x + xx, y + yy, 2, 1, (((x + xx + y + yy) >> 3) & 1) ? 0x8a7428 : 0x2a2926);
    }
}
static void tree(float x, float y, float len, float a, int depth, float seed)
{
  float ex = x + cosf(a) * len, ey = y + sinf(a) * len;
  L(x, y, ex, ey, depth > 2 ? 0x4a4038 : 0x6a5e52);
  if (depth <= 0)
    return;
  tree(ex, ey, len * 0.7f, a - 0.5f - hs(seed) * 0.4f, depth - 1, seed + 1.3f);
  tree(ex, ey, len * 0.7f, a + 0.4f + hs(seed + 2) * 0.4f, depth - 1, seed + 2.7f);
}

// ---- 配管 ----
static void valve(float x, float y, uint32_t c)
{
  CR(x, y, 4, c);
  L(x - 3, y, x + 3, y, c);
  L(x, y - 3, x, y + 3, c);
  C(x, y, 1, 0xd8d4c8);
}
static void gauge(float x, float y, float a)
{
  C(x, y, 3, 0xe0dccf);
  CR(x, y, 3, 0x1a1a1a);
  L(x, y, x + cosf(a) * 2.5f, y + sinf(a) * 2.5f, 0xa02a1a);
}
// fl: フランジの間隔 (点の数)、sx/sy: 影のずらし (0 なら影なし)、valveAt: バルブの位置 (0〜1。0 ならなし)
static void pipe(const Pts &segs, float r, const uint32_t *pal, int fl = 0, float sx = 0, float sy = 0, bool cap = false,
                 float valveAt = 0, uint32_t vc = 0xb0302a)
{
  Pts pts = pathPts(segs);
  if (sx || sy)
    for (auto &p : pts)
      C(p.x + sx, p.y + sy, r, 0, 18);
  tube(segs, r, pal, 0, 0);
  if (fl)
    for (size_t i = fl >> 1; i + 1 < pts.size(); i += fl)
    {
      float dx = pts[i + 1].x - pts[i - 1].x, dy = pts[i + 1].y - pts[i - 1].y, l = hypotf(dx, dy);
      if (l == 0)
        l = 1;
      float nx = -dy / l, ny = dx / l, tx = dx / l, ty = dy / l, x = pts[i].x, y = pts[i].y;
      L(x - nx * (r + 1), y - ny * (r + 1), x + nx * (r + 1), y + ny * (r + 1), pal[1]);
      L(x - nx * (r + 1) + tx, y - ny * (r + 1) + ty, x + nx * (r + 1) + tx, y + ny * (r + 1) + ty, pal[0]);
      P(x - nx * (r + 1), y - ny * (r + 1), pal[3]);
    }
  if (cap)
  {
    const Pt &s = pts.front(), &e = pts.back();
    C(s.x, s.y, r + 1, pal[1]);
    C(e.x, e.y, r + 1, pal[1]);
    C(e.x - 1, e.y - 1, max(1.0f, r - 1), pal[2]);
  }
  if (valveAt > 0)
  {
    const Pt &v = pts[(size_t)(pts.size() * valveAt)];
    valve(v.x - r - 2, v.y - r - 2, vc);
  }
}
struct PipeSpec
{
  int r, k;
};
static int bundleW(std::initializer_list<PipeSpec> list)
{
  int w = 0;
  for (auto &p : list)
    w += 2 * p.r + 2;
  return w;
}
static void bundleV(float x, float y0, float y1, std::initializer_list<PipeSpec> list, float sx, float sy)
{
  float xx = x;
  int i = 0;
  for (auto &p : list)
  {
    pipe({{xx + p.r, y0}, {xx + p.r, y1}}, p.r, PP[p.k], 18 + p.r * 3 + i * 5, sx, sy);
    xx += 2 * p.r + 2;
    i++;
  }
  for (float y = y0 + 20; y < y1; y += 44)
  {
    R(x - 1, y, xx - x + 1, 3, 0x1e1f20);
    R(x - 1, y, xx - x + 1, 1, 0x55585b);
    P(x, y + 1, 0x8a8f94);
    P(xx - 2, y + 1, 0x8a8f94);
  }
}
static void bundleH(float y, float x0, float x1, std::initializer_list<PipeSpec> list, float sx, float sy)
{
  float yy = y;
  int i = 0;
  for (auto &p : list)
  {
    pipe({{x0, yy + p.r}, {x1, yy + p.r}}, p.r, PP[p.k], 18 + p.r * 3 + i * 5, sx, sy);
    yy += 2 * p.r + 2;
    i++;
  }
  for (float x = x0 + 16; x < x1; x += 52)
  {
    R(x, y - 1, 3, yy - y + 1, 0x1e1f20);
    R(x, y - 1, 1, yy - y + 1, 0x55585b);
  }
}
static void maze(float x, float y, float w, float h, int n, float seed, float sx, float sy)
{
  static const int RS[7] = {2, 2, 2, 3, 3, 4, 5};
  for (int i = 0; i < n; i++)
  {
    float px = x + hs(seed + i) * w, py = y + hs(seed + i * 1.3f) * h;
    int dir = hs(seed + i * 2) < 0.5f ? 0 : 1;
    Pts segs = {{px, py}};
    int nseg = 2 + (int)(hs(seed + i * 3.1f) * 3);
    for (int k = 0; k < nseg; k++)
    {
      float len = 16 + hs(seed + i * 7 + k) * 60, sg = hs(seed + i * 5 + k) < 0.5f ? -1 : 1;
      if (dir == 0)
        px = constrain(px + len * sg, x, x + w);
      else
        py = constrain(py + len * sg, y, y + h);
      segs.push_back({px, py});
      dir ^= 1;
    }
    int r = RS[(int)(hs(seed + i * 9) * 7)];
    const uint32_t *pal = PP[(int)(hs(seed + i * 11) * NPP)];
    pipe(segs, r, pal, 14 + r * 2, sx, sy, true, hs(seed + i * 13) < 0.35f ? 0.5f : 0);
  }
}
static void steam(float x, float y, int t, float s = 1)
{
  for (int k = 0; k < 5; k++)
  {
    float p = fmod1(t * 0.015f + k / 5.0f);
    C(x + sinf(p * 6 + k) * 3 * s, y - p * 40 * s, (2 + p * 7) * s, 0xc8cdd2, A(0.3f * (1 - p)));
  }
}

// ---- 梁・電灯・床の番号 ----
static const char *glyph(char ch)
{
  switch (ch)
  {
  case 'A': return "010101111101101";
  case 'B': return "110101110101110";
  case 'C': return "011100100100011";
  case '-': return "000000111000000";
  case '0': return "111101101101111";
  case '1': return "010110010010111";
  case '2': return "111001111100111";
  case '6': return "111100111101111";
  }
  return nullptr;
}
static void mark(float x, float y, const char *str, int s, uint32_t c, int a)
{
  float xx = x;
  for (const char *p = str; *p; p++)
  {
    const char *f = glyph(*p);
    if (f)
      for (int i = 0; i < 15; i++)
        if (f[i] == '1')
          R(xx + (i % 3) * s, y + (i / 3) * s, s, s, c, a);
    xx += 4 * s;
  }
  float w = xx - x + s, hh = 5 * s;
  R(x - 2 * s, y - 2 * s, w + 2 * s, 1, c, a);
  R(x - 2 * s, y + hh + s, w + 2 * s, 1, c, a);
}
static void truss(float y, bool lamps)
{
  R(14, y + 18, W, 12, 0, 71);
  R(0, y, W, 3, 0x6b3a1e);
  R(0, y, W, 1, 0xb07040);
  R(0, y + 12, W, 3, 0x4a2a1a);
  R(0, y + 12, W, 1, 0x8a4f28);
  for (int x = 0; x < W; x += 12)
  {
    LT(x, y + 3, x + 6, y + 12, 2, 0x7a4422);
    LT(x + 6, y + 12, x + 12, y + 3, 2, 0x5a3018);
    P(x, y + 1, 0xd09a6a);
  }
  for (int i = 0; i < 10; i++)
    R(hs(i * 3.3f + y) * W, y + 15, 1, 2 + hs(i + y) * 5, 0x6b3a1e);
  for (int i = 0; i < 3; i++)
    vine(30 + hs(i + y) * 180, y + 14, 8 + hs(i * 2 + y) * 10, y + i, 1);
  if (lamps)
    for (int x : {56, 184})
    {
      L(x, y + 15, x, y + 22, 0x2a2b2c);
      C(x, y + 25, 4, 0x4a4e52);
      CR(x, y + 25, 4, 0x2a2c2e);
      C(x - 1, y + 24, 1, 0x8a8f94);
      C(x, y + 28, 3, 0xffe6aa, 89);
      P(x, y + 28, 0xfff1c8);
    }
}
static void lampPool(float x, float y)
{
  for (int r = 30; r > 6; r -= 6)
    C(x, y, r, 0xffe6aa, 13);
}

// ================= 区画 (ローカル座標: y=0 が上=奥) =================

// 1. 草むらの搬入口
static void z1(int h)
{
  tex(0, 0, W, h, OUTP, 4, 0.045f, 1);
  specks(0, 40, W, h - 40, 300, 1, 0x6d6c5e, 0x3c3d36);
  cracks(0, 50, W, h - 50, 12, 5, 0x33342e);
  for (int i = 0; i < 46; i++)
  {
    float x = hs(i * 6.1f) * W, y = 50 + hs(i * 2.9f) * (h - 50), r = 4 + hs(i * 1.7f) * 9;
    C(x, y, r, i % 3 ? 0x3d5226 : 0x4a5e2c);
    C(x - 2, y - 2, r * 0.6f, 0x55702f);
    specks(x - r, y - r, r * 2, r * 2, 10, i, 0x6f8a3a, 0x2e3e1c);
  }
  for (int y = 46; y < h; y += 9)
  {
    if (hs(y * 0.71f) < 0.3f)
      continue;
    R(118, y, 30, 4, 0x4a3b2c);
    R(118, y, 30, 1, 0x5e4b37);
  }
  for (int x : {124, 141})
  {
    R(x, 42, 2, h - 42, 0x5a3420);
    R(x, 42, 1, h - 42, 0x8a5a34);
  }
  mark(54, 72, "A-01", 3, 0xe1dccd, 102);
  for (int y = 46; y < h; y += 9)
  {
    if (hs(y * 0.37f) < 0.12f)
      continue;
    R(168, y, 32, 4, 0x4a3b2c);
    R(168, y, 32, 1, 0x5e4b37);
  }
  for (int x : {175, 192})
  {
    R(x, 42, 2, h - 42, 0x6b3a1e);
    R(x, 42, 1, h - 42, 0xa8683a);
  }
  C(62, 250, 9, 0x3a3f3c);
  C(67, 253, 7, 0x3a3f3c);
  R(56, 246, 8, 2, 0x8a979a);
  R(60, 252, 10, 1, 0x6f7c80);
  for (int y = 236; y < h; y += 30)
  {
    block(224, y, 4, 4, 12, 0x6b5a44, 0x4a3b2c);
    L(226, y - 6, 226, y + 24, 0x7d7a70);
  }
  for (int i = 0; i < 260; i++)
  {
    float e = hs(i * 5.1f);
    float x = e < 0.35f ? hs(i * 1.3f) * 64 : e < 0.7f ? 176 + hs(i * 1.3f) * 64 : hs(i * 1.3f) * W;
    float y = 48 + hs(i * 2.7f) * (h - 48), k = hs(i * 9.1f);
    tuft(x, y, i * 3.3f, k < 0.18f ? 2 : k < 0.75f ? 1 : 0);
  }
  gallery(-6, 418, 150, 168, 18, 3);
  for (int i = 0; i < 20; i++)
    tuft(10 + hs(i * 4.7f) * 150, 300 + hs(i * 2.2f) * 120, i * 7.7f, i % 3 ? 0 : 2);
  // 建屋の壁と入口
  R(0, 0, W, 6, 0x6a6b6d);
  static const uint32_t WALL[3] = {0x3f4042, 0x47484a, 0x4f5052};
  tex(0, 6, W, 34, WALL, 3, 0.08f, 9);
  drips(0, 6, W, 22, 44, 2);
  for (int x : {12, 52, 172, 210})
  {
    R(x, 13, 20, 14, 0x141516);
    R(x + 2, 15, 7, 5, 0x5d6f78);
    R(x + 12, 20, 5, 4, 0x43535b);
    R(x, 13, 20, 1, 0x6b3a1e);
    R(x + 10, 13, 1, 14, 0x3a2a1e);
  }
  R(88, 6, 64, 34, 0x3a2418);
  R(91, 8, 58, 32, 0x0d0e0f);
  R(91, 8, 58, 3, 0x1c1d1e);
  R(0, 40, W, 3, 0, 102);
  for (int i = 0; i < 26; i++)
    tuft(hs(i * 3.1f) * W, 42 + hs(i) * 6, i * 5.5f, i % 4 ? 0 : 1);
  vine(40, 6, 30, 4, 1);
  vine(160, 6, 34, 8, 1);
  vine(205, 6, 26, 12, 1);
}
static void z1a(int h, int t)
{
  for (int i = 0; i < 16; i++)
  {
    float ph = fmod1(t * 0.003f + hs(i));
    float x = fmodf(hs(i + 3) * W + ph * 90, W), y = 60 + hs(i + 9) * (h - 80) - ph * 40 + sinf(t * 0.05f + i) * 3;
    P(x, y, 0xefe6d0);
  }
}
static void z1f(int h)
{
  for (int y = 70; y < h; y += 60)
  {
    R(4, y, 2, 30, 0x4a2a1a);
    R(28, y, 2, 30, 0x4a2a1a);
    L(4, y, 29, y + 30, 0x6b3a1e);
    L(29, y, 4, y + 30, 0x6b3a1e);
  }
  gallery(17, h + 10, 17, 44, 22, 11);
  truss(250, false);
}

// 2. 基礎の並ぶ建屋
static void z2(int h)
{
  tex(0, 0, W, h, WARM, 4, 0.05f, 2);
  specks(0, 0, W, h, 200, 7, 0x3a362d, 0x6a6450);
  cracks(0, 0, W, h, 10, 9, 0x36322a);
  for (int i = 0; i < 9; i++)
  {
    float x = 20 + hs(i * 4.4f) * 200, y = 40 + hs(i * 6.1f) * 380;
    C(x, y, 5 + hs(i) * 6, 0x6e5320);
    C(x + 5, y + 2, 4 + hs(i + 1) * 5, 0x7a5a22);
    R(x - 3, y - 2, 5, 1, 0xb08a3a);
  }
  for (int i = 0; i < 16; i++)
  {
    float x = hs(i * 8.8f) * W, y = hs(i * 3.9f) * h;
    C(x, y, 2 + hs(i) * 3, 0x4f5f32);
    P(x - 1, y - 1, 0x6b7d3e);
  }
  shaft(0, 24, 176, 120, 0.25f, 0.11f, 16, 12);
  shaft(40, 176, 176, 128, 0.25f, 0.11f, 16, 12);
  shaft(10, 326, 176, 100, 0.25f, 0.11f, 16, 12);
  for (int k = 0; k < 9; k++)
    for (int r = 0; r < h; r++)
      R(lroundf(k * 36 - 90 + r * 0.25f), r, 3, 1, 0, 36);
  for (int y = 56; y < h - 24; y += 34)
    for (int x = 46; x < 190; x += 26)
    {
      float s = x * 7 + y;
      if (hs(s) < 0.12f)
        continue;
      int hh = (int)(8 + hs(s + 1) * 8);
      float xx = x + (hs(s + 3) - 0.5f) * 3;
      block(xx, y, 14, 10, hh, hs(s + 2) < 0.25f ? 0x6f7a4a : 0x8d8a80, 0x5e5c55);
      drips(xx, y - hh + 10, 14, hh - 3, 3, s);
    }
  for (int y = 80; y < h; y += 110)
    for (int x : {4, 212})
    {
      block(x, y, 24, 18, 40, 0x7d7b74, 0x55534d);
      drips(x, y - 22, 24, 30, 7, y + x);
      vine(x + 6 + hs(y) * 10, y - 22, 34, y + x, 1);
    }
  lampPool(66, 182);
  lampPool(194, 182);
  mark(150, 392, "B-02", 3, 0xe1dccd, 107);
  ledge(21);
}
static void z2a(int h, int t)
{
  for (int i = 0; i < 34; i++)
  {
    float ph = fmod1(t * 0.002f + hs(i * 1.7f));
    float x = 10 + hs(i * 2.3f) * 200 + sinf(t * 0.02f + i) * 4, y = 30 + hs(i * 3.1f) * (h - 60) - ph * 30;
    P(x, y, 0xfff0d2, A(0.6f * sinf(ph * PIf)));
  }
}
static void z2f(int h)
{
  truss(110, true);
  bundleH(250, 0, W, {{4, RUST}, {3, LAG}, {2, SILVER}, {2, RED}}, 12, 16);
  pipe({{60, 250}, {60, 200}, {0, 200}}, 3, PP[SILVER], 16, 12, 16);
}

// 3. ダクトと階段
static void z3(int h)
{
  tex(0, 0, W, h, DARK, 4, 0.06f, 3);
  specks(0, 0, W, h, 160, 3, 0x36383a, 0x1a1b1c);
  for (int y = 300; y < 470; y += 22)
    for (int x = 0; x < 60; x += 22)
    {
      R(x, y, 20, 20, 0x4a4440);
      R(x, y, 20, 1, 0x6a625c);
      for (int k = 0; k < 4; k++)
        C(x + hs(x + y + k) * 18, y + hs(x * y + k) * 18, 1 + hs(k + y) * 2, k % 2 ? 0x8a4f28 : 0x6b3a1e);
    }
  grate(56, 480, 128, 40);
  grate(56, 118, 128, 52);
  for (int y = 172; y < 478; y += 9)
  {
    R(94, y, 52, 6, 0x7f8384);
    R(94, y, 52, 1, 0xb3b7b8);
    R(94, y + 6, 52, 3, 0x3b3e40);
    if (hs(y * 0.13f) > 0.6f)
      R(96 + hs(y) * 30, y + 1, 12, 4, 0x8a5a3a);
  }
  R(90, 170, 4, 310, 0x3a3d40);
  R(146, 170, 4, 310, 0x3a3d40);
  railV(88, 172, 306);
  railV(152, 172, 306);
  railH(56, 172, 34);
  railH(150, 172, 34);
  // 奥の壁と扉と電灯
  R(0, 0, W, 6, 0x4a4b4d);
  static const uint32_t WALL[3] = {0x2a2b2c, 0x303132, 0x363738};
  tex(0, 6, W, 112, WALL, 3, 0.07f, 13);
  drips(0, 6, W, 40, 50, 6);
  for (int k = 0; k < 4; k++)
    R(0, 26 + k * 6, W, 3, k % 2 ? 0x3a3d40 : 0x505458);
  R(98, 72, 44, 46, 0x3a2a1e);
  R(101, 75, 38, 43, 0x0c0d0e);
  R(118, 75, 1, 43, 0x1c1d1e);
  for (int r = 34; r > 4; r -= 6)
    C(120, 62, r, 0xffe6aa, 13);
  R(114, 58, 12, 5, 0x6a6f73);
  R(116, 62, 8, 2, 0xfff1c8);
  shaft(92, 120, 56, 50, 0, 0.08f, 0, 0);
  block(30, 230, 34, 26, 22, 0x4a4f53, 0x353a3e);
  for (int x : {38, 54})
  {
    C(x, 216, 5, 0xd8d4c8);
    CR(x, 216, 5, 0x2a2b2c);
    L(x, 216, x + 3, 213, 0xa02a1a);
  }
  // 壁の配管と脇の配管
  bundleH(10, 0, W, {{2, YELLOW}, {3, SILVER}, {2, RED}, {4, RUST}}, 0, 0);
  static const int VX[6] = {12, 20, 44, 190, 204, 226}, VK[6] = {SILVER, RUST, GREEN, BLACK, YELLOW, SILVER};
  for (int i = 0; i < 6; i++)
    pipe({{(float)VX[i], 10}, {(float)VX[i], 118}}, 2, PP[VK[i]], 20);
  gauge(30, 80, -0.8f);
  gauge(214, 80, -0.8f);
  maze(0, 140, 60, 330, 9, 31, 2, 3);
  maze(180, 140, 60, 330, 9, 37, 2, 3);
  // ダクト
  Pts d1 = {{26, 560}, {26, 262}};
  arc(d1, 66, 262, 40, PIf, PIf * 1.5f);
  d1.push_back({66, 30});
  tube(d1, 15, PP[SILVER], 10, 20);
  Pts d2 = {{214, 560}, {214, 302}};
  arc(d2, 174, 302, 40, 0, -PIf / 2);
  d2.push_back({174, 30});
  tube(d2, 15, PP[BLUE], 10, 20);
  for (int x : {66, 174})
  {
    R(x - 18, 26, 36, 6, 0x5a5e62);
    R(x - 18, 26, 36, 1, 0x8a9094);
  }
  tube({{0, 205}, {(float)W, 205}}, 4, PP[SILVER], 0, 64);
}
static void z3a(int h, int t)
{
  if (hs(t / 5) > 0.93f)
  {
    C(120, 62, 30, 0x0a0a0a, 89);
    R(116, 62, 8, 2, 0x8a7a5a);
  }
  for (int i = 0; i < 14; i++)
  {
    float ph = fmod1(t * 0.004f + hs(i * 2.1f));
    P(96 + hs(i * 4.4f) * 48 + sinf(t * 0.03f + i) * 3, 70 + hs(i * 1.3f) * 90 - ph * 20, 0xffecbe, A(0.7f * sinf(ph * PIf)));
  }
  float p = (t % 70) / 70.0f;
  if (p < 0.5f)
    P(150, 210 + p * 120, 0x9fb6c0);
  steam(40, 300, t, 0.8f);
  steam(200, 220, t + 30, 0.8f);
}
static void z3f(int h)
{
  bundleH(320, 0, W, {{3, RUST}, {2, SILVER}, {2, YELLOW}}, 10, 14);
  pipe({{(float)W, 400}, {196, 400}, {196, 320}}, 3, PP[GREEN], 14, 10, 14);
}

// 4. 配管の迷路
static void z8(int h)
{
  tex(0, 0, W, h, DARK, 4, 0.06f, 23);
  specks(0, 0, W, h, 180, 23, 0x36383a, 0x1a1b1c);
  grate(70, 0, 100, h);
  R(68, 0, 2, h, 0x3a3d40);
  R(170, 0, 2, h, 0x3a3d40);
  maze(30, 16, 180, h - 32, 18, 3, 2, 3);
  struct
  {
    int y, k, r;
  } cr[2] = {{150, RUST, 4}, {372, BLACK, 5}};
  for (auto &c : cr)
  {
    pipe({{0, (float)c.y}, {(float)W, (float)c.y}}, c.r, PP[c.k], 16, 2, 3);
    block(78, c.y - 10, 84, 20, 5, 0x3e4144, 0x2a2c2e);
    for (int xx = 80; xx < 160; xx += 4)
      R(xx, c.y - 15, 1, 20, 0x1a1b1c);
  }
  bundleV(0, 0, h, {{4, LAG}, {3, RUST}, {5, SILVER}, {2, YELLOW}}, 3, 4);
  int rw = bundleW({{2, RED}, {5, GREEN}, {3, SILVER}, {4, BLACK}});
  bundleV(W - rw, 0, h, {{2, RED}, {5, GREEN}, {3, SILVER}, {4, BLACK}}, 3, 4);
  for (auto p : {Pt{50, 110}, Pt{190, 470}})
  {
    block(p.x - 15, p.y, 30, 22, 16, 0x4a5a52, 0x33403a);
    C(p.x, p.y - 6, 7, 0x5f7068);
    CR(p.x, p.y - 6, 7, 0x2a3430);
    C(p.x - 2, p.y - 8, 2, 0x8aa098);
    pipe({{p.x + 15, p.y - 4}, {p.x + 30, p.y - 4}, {p.x + 30, p.y + 30}}, 3, PP[GREEN], 12, 0, 0, true);
  }
  pipe({{30, 290}, {210, 290}}, 5, PP[RUST], 14, 3, 4, true);
  for (int k = 0; k < 8; k++)
  {
    float x = 44 + k * 22;
    pipe({{x, 296}, {x, 336}}, 2, PP[SILVER], 0, 0, 0, true);
    valve(x, 318, k % 3 ? 0xb0302a : 0xc9a227);
  }
  gauge(56, 278, -0.9f);
  gauge(184, 278, -0.9f);
  railV(66, 12, h - 12);
  railV(174, 12, h - 12);
  ledge(77);
}
static void z8a(int h, int t)
{
  steam(52, 290, t);
  steam(196, 150, t + 25, 0.8f);
  steam(120, 440, t + 50, 0.7f);
  gauge(56, 278, -0.9f + sinf(t * 0.13f) * 0.35f);
  gauge(184, 278, -0.9f + sinf(t * 0.13f + 2) * 0.35f);
  float p = (t % 50) / 50.0f;
  if (p < 0.6f)
    P(88, 160 + p * 60, 0x9fb6c0);
}
static void z8f(int h)
{
  bundleH(64, 0, W, {{3, SILVER}, {4, RUST}, {2, YELLOW}, {3, GREEN}}, 10, 14);
  bundleH(214, 0, W, {{5, LAG}, {3, BLACK}}, 12, 16);
  bundleH(420, 0, W, {{4, BLUE}, {3, RED}, {2, SILVER}}, 10, 14);
  pipe({{150, 92}, {150, 140}, {(float)W, 140}}, 3, PP[SILVER], 16, 10, 14);
  pipe({{40, 236}, {40, 270}, {0, 270}}, 2, PP[YELLOW], 12, 10, 14);
}

// 5. 止まったプレス
static void press3D(float cx, float cy, float elev, float seed)
{
  float bx = cx - 40, by = cy - 22;
  fadedHaz(lroundf(bx - 8), lroundf(by - 8), 96, 64);
  block(bx, by, 80, 44, 8, 0x5b5650, 0x3e3a36);
  R(cx - 24, by - 4, 48, 22, 0x6f747a);
  R(cx - 24, by - 4, 48, 1, 0x9aa0a5);
  for (int dx : {-40, 32})
    block(cx + dx, by - 4, 8, 8, 74, 0x7a6a5c, 0x56463a);
  R(bx + 4 + elev * 0.3f, by + elev * 0.4f - 2, 72, 32, 0, 89);
  float hy = by - elev;
  block(cx - 36, hy, 72, 30, 16, 0x7a6656, 0x55402f);
  for (int k = 1; k < 6; k++)
    R(cx - 36 + k * 12, hy - 14, 2, 28, 0x5f4d40);
  C(cx, hy - 1, 7, 0x8b8076);
  C(cx - 1, hy - 2, 3, 0xb0a596);
  drips(cx - 36, hy + 14, 72, 10, 12, seed);
  for (int i = 0; i < 3; i++)
    vine(cx - 30 + hs(seed + i) * 60, hy + 14, 10 + hs(seed + i * 2) * 14, seed + i * 5, 1);
  for (int dx : {-40, 32})
  {
    block(cx + dx, by + 36, 8, 8, 74, 0x7a6a5c, 0x56463a);
    drips(cx + dx, by - 30, 8, 30, 2, seed + dx);
  }
}
static void z4(int h)
{
  static const uint32_t OIL[4] = {0x33322f, 0x3a3936, 0x413f3b, 0x484640};
  tex(0, 0, W, h, OIL, 4, 0.05f, 4);
  specks(0, 0, W, h, 160, 4, 0x2a2927, 0x5a5852);
  for (int i = 0; i < 8; i++)
  {
    float x = hs(i * 2.2f) * W, y = hs(i * 5.5f) * h;
    C(x, y, 4 + hs(i) * 7, 0x2a2926);
    C(x + 3, y + 2, 3 + hs(i + 2) * 4, 0x26251f);
  }
  for (int i = 0; i < 6; i++)
  {
    float x = 20 + hs(i * 6.6f) * 200, y = 30 + hs(i * 1.4f) * 380;
    C(x, y, 4 + hs(i) * 4, 0x6e5320);
    R(x - 2, y - 2, 4, 1, 0xb08a3a);
  }
  for (int i = 0; i < 18; i++)
    R(hs(i * 3.3f) * W, hs(i * 7.1f) * h, 4 + hs(i) * 8, 2, i % 2 ? 0x6b3a1e : 0x5a5852);
  for (int y = 0; y < h; y += 12)
    R(124, y, 3, 6, 0xa08228, 115);
  press3D(68, 140, 30, 41);
  press3D(68, 360, 12, 77);
  int rw = bundleW({{3, RUST}, {2, SILVER}, {4, BLACK}});
  bundleV(W - rw, 13, h, {{3, RUST}, {2, SILVER}, {4, BLACK}}, 3, 4);
  pipe({{0, 250}, {24, 250}, {24, 300}, {110, 300}}, 3, PP[RUST], 14, 3, 4, true, 0.6f);
  pipe({{0, 40}, {140, 40}, {140, 70}}, 2, PP[YELLOW], 12, 3, 4, true);
  ledge(44);
}
static void z4a(int h, int t)
{
  float p = (t % 150) / 150.0f, z;
  if (p < 0.7f)
    z = 34;
  else if (p < 0.73f)
    z = 34 - (p - 0.7f) / 0.03f * 32;
  else if (p < 0.8f)
    z = 2;
  else
    z = 2 + (p - 0.8f) / 0.2f * 32;
  press3D(170, 252, z, 63);
  if (p >= 0.73f && p < 0.79f)
  {
    float q = (p - 0.73f) / 0.06f;
    for (int k = 0; k < 14; k++)
    {
      float a = k / 14.0f * PIf * 2 + 0.3f, r0 = 40 + q * 18;
      float x = 170 + cosf(a) * r0 * 1.1f, y = 238 + sinf(a) * r0 * 0.6f;
      L(x, y, x + cosf(a) * 4, y + sinf(a) * 2, q < 0.5f ? 0xffe27a : 0xf7841f);
    }
    for (int k = 0; k < 8; k++)
      C(130 + k * 11, 270 + sinf(k) * 3, 3 + q * 4, 0xa09682, A(0.25f * (1 - q)));
  }
}

// 6. 沈殿槽
static const int PX = 26, PW = 188, PY = 70, PH = 310;
static void z5(int h)
{
  tex(0, 0, W, h, CONC, 4, 0.05f, 5);
  specks(0, 0, W, h, 160, 5, 0x3a3b3d, 0x6a6b6d);
  R(PX - 6, PY - 6, PW + 12, 6, 0x77787a);
  static const uint32_t PW3[3] = {0x3a3c3b, 0x414342, 0x484a49};
  tex(PX, PY, PW, 50, PW3, 3, 0.07f, 15);
  drips(PX, PY, PW, 30, 40, 9);
  for (int xx = PX; xx < PX + PW; xx += 2)
  {
    float l = 6 + vn(xx * 0.1f, 2) * 10;
    R(xx, PY + 50 - l, 2, l, xx % 4 ? 0x2f4a32 : 0x3a5a3a);
  }
  tex(PX, PY + 50, PW, PH - 50, WATER, 4, 0.035f, 6);
  R(PX - 6, PY - 6, 6, PH + 12, 0x6f7072);
  R(PX + PW, PY - 6, 6, PH + 12, 0x6f7072);
  R(PX, PY + 50, 5, PH - 50, 0, 89);
  R(PX - 6, PY + PH, PW + 12, 6, 0x77787a);
  R(PX - 6, PY + PH + 6, PW + 12, 16, 0x4b4c4e);
  drips(PX - 6, PY + PH + 6, PW + 12, 12, 30, 19);
  R(PX - 6, PY + PH + 22, PW + 12, 3, 0, 89);
  railH(PX - 6, PY - 6, PW + 12);
  railV(PX - 4, PY - 2, PH);
  for (auto b : {Pt{8, 290}, Pt{10, 312}})
  {
    R(b.x, b.y, 12, 10, 0x24548a);
    R(b.x, b.y - 6, 12, 7, 0x2f63a0);
    R(b.x + 1, b.y - 5, 4, 1, 0x6a9ed2);
    C(b.x + 6, b.y - 3, 1, 0x173455);
  }
  for (int i = 0; i < 10; i++)
    tuft(hs(i * 2.8f) * W, PY + PH + 30 + hs(i) * 30, i * 4.1f, 1);
  pipe({{0, 50}, {(float)W, 50}}, 3, PP[RUST], 18, 3, 4);
  pipe({{0, 40}, {(float)W, 40}}, 2, PP[SILVER], 22, 3, 4);
  pipe({{0, 150}, {18, 150}, {18, 120}, {44, 120}, {44, 210}}, 3, PP[SILVER], 14, 3, 4, false, 0.3f);
  pipe({{228, 30}, {228, 400}}, 3, PP[GREEN], 18, 3, 4);
  pipe({{(float)W, 300}, {200, 300}, {200, 330}}, 2, PP[RED], 12, 0, 0, true);
  ledge(55);
}
static void z5a(int h, int t)
{
  for (int i = 0; i < 18; i++)
  {
    float p2 = fmod1(t * 0.01f + hs(i)), rx = PX + 10 + hs(i * 3.3f) * (PW - 30), ry = PY + 60 + hs(i * 1.7f) * (PH - 70);
    R(rx, ry, 4 + p2 * 10, 1, 0x5a8c78, A(0.35f * (1 - p2)));
  }
  for (int i = 0; i < 46; i++)
  {
    float dir = hs(i * 0.7f) > 0.5f ? 1 : -1;
    float dx = fmodf(fmodf(hs(i * 2) * PW + t * 0.06f * dir, PW) + PW, PW);
    float dy = PY + 56 + hs(i * 5) * (PH - 64) + sinf(t * 0.02f + i) * 2;
    P(PX + dx, dy, i % 3 ? 0xc9bf9a : 0x9e9472);
    if (i % 4 == 0)
      P(PX + dx + 1, dy, 0xb3a983);
  }
  float q = (t % 60) / 60.0f;
  CR(44, 214, 4 + q * 8, 0x78aa96, A(0.5f * (1 - q)));
  float cy = 50 + (h - 110) * (0.5f + 0.5f * sinf(t * 0.004f)), tx = 120 + 70 * sinf(t * 0.007f);
  R(0, cy + 24, W, 11, 0, 71);
  R(tx + 2, cy + 22, 18, 14, 0, 71);
  R(0, cy, W, 11, 0x2f6b52);
  R(0, cy, W, 1, 0x6fb595);
  R(0, cy + 10, W, 1, 0x1d3f31);
  R(0, cy + 2, W, 7, 0x24473a);
  for (int xx = 0; xx < W; xx += 8)
  {
    L(xx, cy + 2, xx + 4, cy + 8, 0x4f9a78);
    L(xx + 4, cy + 8, xx + 8, cy + 2, 0x4f9a78);
  }
  block(tx - 9, cy - 3, 18, 14, 6, 0xd0b040, 0x8a7020);
  L(tx, cy + 11, tx, cy + 17, 0x2a2b2c);
  CR(tx, cy + 19, 2, 0x3a3b3c);
}

// 7. 大きな建屋
static void chair(float x, float y)
{
  Pt q[4] = {{x, y + 7}, {x + 8, y + 7}, {x + 22, y + 15}, {x + 14, y + 15}};
  poly(q, 4, 0, 89);
  for (auto l : {Pt{x, y + 6}, Pt{x + 7, y + 6}, Pt{x, y}, Pt{x + 7, y}})
    L(l.x, l.y, l.x, l.y + 5, 0x3a3430);
  R(x, y, 8, 7, 0x8a5a32);
  R(x, y, 8, 1, 0xa87448);
  R(x, y - 9, 1, 9, 0x5a3a22);
  R(x + 7, y - 9, 1, 9, 0x5a3a22);
  R(x, y - 9, 8, 3, 0x7a4a2a);
}
static void z6(int h)
{
  static const uint32_t HALL[4] = {0x55524d, 0x5a5752, 0x625f59, 0x6a665f};
  tex(0, 0, W, h, HALL, 4, 0.03f, 7);
  specks(0, 0, W, h, 240, 7, 0x4a4744, 0x76726a);
  cracks(0, 80, W, h - 80, 8, 13, 0x484540);
  for (int i = 0; i < 6; i++)
    C(hs(i * 9.2f) * W, 100 + hs(i * 1.9f) * 400, 10 + hs(i) * 12, 0x28241e, 31);
  for (int k = 0; k < 10; k++)
    for (int r = 72; r < h; r++)
      R(lroundf(k * 34 - 120 + r * 0.45f), r, 4, 1, 0, 31);
  for (int y0 : {100, 214, 330, 430})
    shaft(-30, y0, 150, 74, 0.55f, 0.15f, 22, 18);
  C(168, 300, 10, 0x4a4844);
  C(176, 304, 8, 0x4a4844);
  R(164, 297, 14, 2, 0xa9a59a);
  for (int y = 120; y < h; y += 100)
    for (int x : {0, 216})
    {
      block(x, y, 24, 16, 56, 0x8d8a83, 0x6a6760);
      drips(x, y - 40, 24, 30, 6, x + y);
      if (hs(x + y) > 0.4f)
        vine(x + 8, y - 40, 40, x + y, 1);
    }
  static const uint32_t WALL[3] = {0x3d3b37, 0x45423d, 0x4d4a44};
  tex(0, 0, W, 72, WALL, 3, 0.06f, 8);
  drips(0, 0, W, 34, 50, 8);
  R(0, 50, W, 22, 0x141210, 89);
  R(14, 30, 26, 42, 0x2a2622);
  R(16, 32, 22, 40, 0x121110);
  R(30, 50, 2, 2, 0x8a8070);
  tube({{0, 12}, {(float)W, 12}}, 4, PP[SILVER], 0, 0);
  for (int i = 0; i < 9; i++)
  {
    float x = 72 + i * 12;
    tube({{x, 12}, {x, 60}}, 3, PP[SILVER], 0, 0);
    R(x - 3, 58, 6, 6, 0x3a3836);
    R(x - 1, 60, 2, 2, 0x7a3a2c);
  }
  R(62, 64, 120, 6, 0x5a4f45);
  R(62, 64, 120, 1, 0x7a6e62);
  bundleH(24, 0, W, {{2, RED}, {2, YELLOW}}, 0, 0);
  pipe({{190, 20}, {190, 44}, {228, 44}, {228, 72}}, 3, PP[RUST], 12);
  pipe({{50, 12}, {50, 52}, {20, 52}}, 2, PP[GREEN], 10);
  gauge(204, 34, -0.5f);
  R(0, 72, W, 5, 0, 89);
  chair(150, 104);
  lampPool(66, 372);
  lampPool(194, 372);
  mark(100, 480, "C-06", 3, 0xe1dccd, 107);
}
static void z6a(int h, int t)
{
  static const int BAND[4] = {100, 214, 330, 430};
  for (int i = 0; i < 40; i++)
  {
    float ph = fmod1(t * 0.0018f + hs(i * 1.3f));
    float r = hs(i * 4.1f) * 74, x = -30 + r * 0.55f + hs(i * 2.9f) * 140 + sinf(t * 0.015f + i) * 4, y = BAND[i % 4] + r - ph * 24;
    P(x, y, 0xfff4dc, A(0.75f * sinf(ph * PIf)));
  }
}
static void z6f(int h)
{
  truss(300, true);
  bundleH(186, 0, W, {{3, SILVER}, {4, RUST}, {2, YELLOW}}, 14, 18);
  bundleH(396, 0, W, {{5, LAG}, {3, BLACK}, {2, RED}}, 14, 18);
  pipe({{180, 186}, {180, 150}, {(float)W, 150}}, 3, PP[SILVER], 14, 14, 18);
}

// 8. 溶鉱炉の跡
static void z7(int h)
{
  tex(0, 0, W, h, OUTP, 4, 0.045f, 17);
  specks(0, 0, W, h, 280, 17, 0x6d6c5e, 0x3c3d36);
  for (int i = 0; i < 12; i++)
  {
    float x = hs(i * 3.7f) * W, y = hs(i * 8.1f) * h;
    for (int k = 0; k < 6; k++)
      C(x + (hs(i + k) - 0.5f) * 14, y + (hs(i * k + 1) - 0.5f) * 10, 3 + hs(k + i) * 3, k % 2 ? 0x3a4226 : 0x4a4530);
  }
  C(44, 96, 22, 0, 89);
  C(40, 90, 20, 0x6a6d70);
  CR(40, 90, 20, 0x3a3d40);
  C(38, 88, 14, 0x7d8184);
  C(36, 86, 6, 0xa3a8ac);
  C(40, 90, 3, 0x2a2b2c);
  drips(22, 96, 36, 12, 10, 3);
  const float tx = 150, ty = 120;
  LT(tx, ty + 100, tx + 74, ty + 100, 2, 0x4a2a1a);
  LT(tx, ty + 30, tx, ty + 100, 2, 0x4a2a1a);
  LT(tx + 74, ty + 30, tx + 74, ty + 100, 2, 0x4a2a1a);
  L(tx, ty + 100, tx + 74, ty + 30, 0x6b3a1e);
  L(tx + 74, ty + 100, tx, ty + 30, 0x6b3a1e);
  Pt s1[4] = {{tx + 6, ty + 30}, {tx + 82, ty + 30}, {tx + 94, ty + 76}, {tx + 18, ty + 76}};
  poly(s1, 4, 0, 77);
  grate(tx - 4, ty - 6, 82, 50);
  for (int dx : {0, 74})
    R(tx + dx - 1, ty + 44, 2, 56, 0x8a4f28);
  railH(tx - 4, ty - 4, 82);
  railV(tx - 4, ty, 40);
  static const uint32_t HOP[4] = {0x5a3a22, 0x4a2a1a, 0x3a1e12, 0x20120a};
  for (int k = 0; k < 4; k++)
    R(tx + 6 + k * 4, ty + 6 + k * 4, 30 - k * 8, 30 - k * 8, HOP[k]);
  Pt s2[4] = {{tx + 40, ty - 50}, {tx + 80, ty - 50}, {tx + 90, ty - 14}, {tx + 50, ty - 14}};
  poly(s2, 4, 0, 77);
  R(tx + 34, ty - 30, 42, 22, 0x6a5a4c);
  drips(tx + 34, ty - 30, 42, 14, 10, 31);
  T(tx + 30, ty - 30, tx + 80, ty - 30, tx + 55, ty - 66, 0x8a5a3a);
  T(tx + 30, ty - 30, tx + 55, ty - 66, tx + 30, ty - 62, 0xa8683a);
  R(tx + 30, ty - 63, 26, 1, 0xc98a52);
  for (int i = 0; i < 4; i++)
    vine(tx + 6 + i * 20, ty - 6, 30 + hs(i) * 20, 50 + i, 1);
  gallery(tx + 60, ty + 30, tx + 150, ty + 260, 16, 8);
  const float fx = 74, fy = 340;
  C(fx + 6, fy + 8, 60, 0, 97);
  for (int k = 0; k < 9; k++)
  {
    float a = k / 9.0f * PIf * 2 + 0.3f;
    LT(fx + cosf(a) * 52, fy + sinf(a) * 52, fx + cosf(a) * 68, fy + sinf(a) * 68, 3, 0x5a3422);
    C(fx + cosf(a) * 68, fy + sinf(a) * 68, 3, 0x6b3a1e);
  }
  C(fx, fy, 56, 0x5a3422);
  CR(fx, fy, 56, 0x8a4f28);
  CR(fx, fy, 53, 0x3a2216);
  for (float a = 0; a < 6.28f; a += 0.05f)
  {
    float v = vn(a * 3, 4);
    if (v > 0.5f)
      for (int r = 44; r < 54; r++)
        if (hs(a * 100 + r) > 0.55f)
          P(fx + cosf(a) * r, fy + sinf(a) * r, v > 0.65f ? 0xa8683a : 0x7a4422);
  }
  C(fx, fy, 44, 0x3d2a22);
  CR(fx, fy, 40, 0x2a1c16);
  CR(fx, fy, 36, 0x2a1c16);
  C(fx, fy, 34, 0x1f1b18);
  for (int i = 0; i < 26; i++)
  {
    float a = hs(i * 2.2f) * 6.28f, r = hs(i * 1.1f) * 28;
    tuft(fx + cosf(a) * r, fy + sinf(a) * r + 4, i * 9.9f, i % 3 ? 1 : 0);
  }
  for (int i = 0; i < 5; i++)
  {
    float a = -2.6f + i * 0.5f;
    vine(fx + cosf(a) * 56, fy + sinf(a) * 56, 20, 70 + i, 1);
  }
  gallery(fx + 50, fy - 30, tx + 10, ty + 90, 12, 21);
  tree(214, 420, 18, -1.9f, 4, 3);
  tree(18, 210, 16, -1.2f, 4, 8);
  tree(130, 470, 14, -1.5f, 3, 5);
  for (int i = 0; i < 150; i++)
  {
    float k = hs(i * 9.7f);
    tuft(hs(i * 1.9f) * W, hs(i * 4.3f) * h, i * 2.9f, k < 0.25f ? 2 : k < 0.5f ? 1 : 0);
  }
  for (int x : {8, 58, 108})
  {
    block(x, 262, 5, 5, 24, 0x7a4422, 0x4a2a1a);
    L(x, 262, x + 5, 238, 0x6b3a1e);
  }
  pipe({{0, 440}, {60, 440}, {60, 480}, {120, 480}}, 3, PP[RUST], 14, 3, 4, true);
}
static void z7a(int h, int t)
{
  for (int i = 0; i < 4; i++)
  {
    float bx = fmodf(t * (0.5f + i * 0.1f) + i * 80, W + 60) - 30, by = 60 + i * 90 + sinf(t * 0.02f + i) * 10;
    float f = (((t >> 3) + i) & 1) ? -2 : 1;
    C(bx + 6, by + 14, 1, 0, 89);
    L(bx - 4, by + f, bx, by, 0x141414);
    L(bx, by, bx + 4, by + f, 0x141414);
    P(bx, by + 1, 0x141414);
  }
  for (int i = 0; i < 10; i++)
  {
    float ph = fmod1(t * 0.003f + hs(i));
    P(fmodf(hs(i + 3) * W + ph * 90, W), 30 + hs(i + 9) * (h - 60) - ph * 40, 0xefe6d0);
  }
}
static void z7f(int h)
{
  bundleH(232, 0, 150, {{4, RUST}, {3, RUST}, {2, SILVER}}, 8, 12);
  pipe({{150, 236}, {180, 236}, {180, 200}}, 4, PP[RUST], 14, 8, 12, true);
}

// ---- 一覧 (下から順) ----
const Zone ZONES[] = {
    {"GRASS YARD", 420, z1, z1a, z1f},   {"FOOTINGS", 440, z2, z2a, z2f}, {"DUCTS", 520, z3, z3a, z3f},
    {"PIPE MAZE", 520, z8, z8a, z8f},    {"PRESSES", 420, z4, z4a, nullptr}, {"SETTLING TANK", 440, z5, z5a, nullptr},
    {"GREAT HALL", 520, z6, z6a, z6f},   {"FURNACE", 500, z7, z7a, z7f},
};
const int NZ = sizeof(ZONES) / sizeof(ZONES[0]);
int STARTS[sizeof(ZONES) / sizeof(ZONES[0])];
int TOT = 0;
uint16_t *bgBuf = nullptr, *foreBuf = nullptr;
uint8_t *foreRow = nullptr;

bool build()
{
  TOT = 0;
  for (int i = 0; i < NZ; i++)
  {
    STARTS[i] = TOT;
    TOT += ZONES[i].h;
  }
  bgBuf = (uint16_t *)heap_caps_malloc((size_t)W * TOT * 2, MALLOC_CAP_SPIRAM);
  foreBuf = (uint16_t *)heap_caps_malloc((size_t)W * TOT * 2, MALLOC_CAP_SPIRAM);
  foreRow = (uint8_t *)heap_caps_malloc(TOT, MALLOC_CAP_SPIRAM);
  if (!bgBuf || !foreBuf || !foreRow)
    return false;
  for (size_t i = 0; i < (size_t)W * TOT; i++)
    foreBuf[i] = KEY;
  memset(foreRow, 0, TOT);
  for (int i = 0; i < NZ; i++)
  {
    const Zone &z = ZONES[i];
    int row = TOT - STARTS[i] - z.h;
    Surf sb;
    sb.buf = bgBuf + (size_t)row * W;
    sb.h = z.h;
    S = &sb;
    z.stat(z.h);
    if (z.fore)
    {
      Surf sf;
      sf.buf = foreBuf + (size_t)row * W;
      sf.h = z.h;
      sf.key = KEY;
      sf.alt = &sb;
      S = &sf;
      z.fore(z.h);
    }
    for (int y = 0; y < z.h; y++)
    {
      const uint16_t *p = foreBuf + (size_t)(row + y) * W;
      for (int x = 0; x < W; x++)
        if (p[x] != KEY)
        {
          foreRow[row + y] = 1;
          break;
        }
    }
  }
  return true;
}
} // namespace bg2
