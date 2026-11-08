// 2 本目 (廃工場) の描く部品
#include "g2draw.h"

namespace d2
{
Surf *S = nullptr;

uint16_t c565(uint32_t c) { return ((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F); }

uint16_t blend(uint16_t v, uint32_t c, int a)
{
  int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
  int cr = (c >> 19) & 31, cg = (c >> 10) & 63, cb = (c >> 3) & 31;
  int na = 255 - a;
  r = (r * na + cr * a + 127) / 255;
  g = (g * na + cg * a + 127) / 255;
  b = (b * na + cb * a + 127) / 255;
  return (r << 11) | (g << 5) | b;
}

static void spanTo(Surf *s, int x0, int x1, int y, uint32_t c, int a)
{
  if (y < s->cy0 || y > s->cy1)
    return;
  int ry = y - s->y0;
  if (ry < 0 || ry >= s->h)
    return;
  if (x0 < s->cx0)
    x0 = s->cx0;
  if (x1 > s->cx1)
    x1 = s->cx1;
  if (x0 > x1)
    return;
  uint16_t *p = s->buf + ry * s->w + x0;
  int n = x1 - x0 + 1;
  if (a >= 255)
  {
    uint16_t v = c565(c);
    if (s->key && v == s->key)
      v ^= 0x0020;
    while (n--)
      *p++ = v;
  }
  else if (a > 0)
  {
    if (s->key)
      while (n--)
      {
        if (*p != s->key)
          *p = blend(*p, c, a);
        p++;
      }
    else
      while (n--)
      {
        *p = blend(*p, c, a);
        p++;
      }
  }
}

void span(int x0, int x1, int y, uint32_t c, int a)
{
  Surf *s = (a < 255 && S->alt) ? S->alt : S;
  spanTo(s, x0 + S->ox, x1 + S->ox, y + S->oy, c, a);
}

void R(float x, float y, float w, float h, uint32_t c, int a)
{
  int X = lroundf(x), Y = lroundf(y), Wd = lroundf(w), Hd = lroundf(h);
  if (Wd <= 0 || Hd <= 0)
    return;
  for (int r = 0; r < Hd; r++)
    span(X, X + Wd - 1, Y + r, c, a);
}

void P(float x, float y, uint32_t c, int a)
{
  int X = lroundf(x), Y = lroundf(y);
  span(X, X, Y, c, a);
}

void L(float fx0, float fy0, float fx1, float fy1, uint32_t c, int a)
{
  int x0 = lroundf(fx0), y0 = lroundf(fy0), x1 = lroundf(fx1), y1 = lroundf(fy1);
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int e = dx + dy;
  for (int n = 0; n < 900; n++)
  {
    span(x0, x0, y0, c, a);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * e;
    if (e2 >= dy)
    {
      e += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      e += dx;
      y0 += sy;
    }
  }
}

void LT(float x0, float y0, float x1, float y1, int t, uint32_t c, int a)
{
  bool steep = fabsf(y1 - y0) > fabsf(x1 - x0);
  for (int i = 0; i < t; i++)
  {
    int o = i - ((t - 1) >> 1);
    if (steep)
      L(x0 + o, y0, x1 + o, y1, c, a);
    else
      L(x0, y0 + o, x1, y1 + o, c, a);
  }
}

void C(float fcx, float fcy, float fr, uint32_t c, int a)
{
  int cx = lroundf(fcx), cy = lroundf(fcy), r = lroundf(fr);
  if (r < 0)
    return;
  for (int dy = -r; dy <= r; dy++)
  {
    int w = (int)floorf(sqrtf((float)(r * r - dy * dy)) + 0.5f);
    span(cx - w, cx + w, cy + dy, c, a);
  }
}

void CR(float fcx, float fcy, float fr, uint32_t c, int a)
{
  int cx = lroundf(fcx), cy = lroundf(fcy), r = lroundf(fr);
  for (int d = -r; d <= r; d++)
  {
    int w = lroundf(sqrtf((float)(r * r - d * d)));
    span(cx - w, cx - w, cy + d, c, a);
    span(cx + w, cx + w, cy + d, c, a);
    span(cx + d, cx + d, cy - w, c, a);
    span(cx + d, cx + d, cy + w, c, a);
  }
}

void poly(const Pt *p, int n, uint32_t c, int a)
{
  float mn = 1e9f, mx = -1e9f;
  for (int i = 0; i < n; i++)
  {
    mn = min(mn, p[i].y);
    mx = max(mx, p[i].y);
  }
  for (int y = lroundf(mn); y <= lroundf(mx); y++)
  {
    float lo = 1e9f, hi = -1e9f;
    for (int i = 0; i < n; i++)
    {
      const Pt &A = p[i], &B = p[(i + 1) % n];
      float y0 = min(A.y, B.y), y1 = max(A.y, B.y);
      if (y < y0 - 0.5f || y > y1 + 0.5f)
        continue;
      if (A.y == B.y)
      {
        lo = min(lo, min(A.x, B.x));
        hi = max(hi, max(A.x, B.x));
      }
      else
      {
        float k = constrain((y - A.y) / (B.y - A.y), 0.0f, 1.0f);
        float x = A.x + (B.x - A.x) * k;
        lo = min(lo, x);
        hi = max(hi, x);
      }
    }
    if (hi >= lo)
      span(lroundf(lo), lroundf(hi), y, c, a);
  }
}

void T(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c, int a)
{
  Pt p[3] = {{x0, y0}, {x1, y1}, {x2, y2}};
  poly(p, 3, c, a);
}

float hs(float n)
{
  uint32_t h = (uint32_t)(int32_t)lrintf(n * 1024.0f) * 0x9E3779B1u;
  h ^= h >> 15;
  h *= 0x85EBCA77u;
  h ^= h >> 13;
  h *= 0xC2B2AE3Du;
  h ^= h >> 16;
  return (h >> 8) * (1.0f / 16777216.0f);
}

float h2(float x, float y)
{
  uint32_t h = (uint32_t)(int32_t)lrintf(x * 16.0f) * 0x9E3779B1u ^ (uint32_t)(int32_t)lrintf(y * 16.0f) * 0x85EBCA77u;
  h ^= h >> 15;
  h *= 0x2C1B3C6Du;
  h ^= h >> 12;
  h *= 0x297A2D39u;
  h ^= h >> 15;
  return (h >> 8) * (1.0f / 16777216.0f);
}

float vn(float x, float y)
{
  float xi = floorf(x), yi = floorf(y), xf = x - xi, yf = y - yi;
  float u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
  float a = h2(xi, yi), b = h2(xi + 1, yi), c = h2(xi, yi + 1), d = h2(xi + 1, yi + 1);
  return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}

Pts pathPts(const Pts &segs, float step)
{
  Pts out;
  for (size_t i = 0; i + 1 < segs.size(); i++)
  {
    const Pt &a = segs[i], &b = segs[i + 1];
    float l = hypotf(b.x - a.x, b.y - a.y);
    int n = max(1, (int)ceilf(l / step));
    for (int k = 0; k < n; k++)
      out.push_back({a.x + (b.x - a.x) * k / n, a.y + (b.y - a.y) * k / n});
  }
  if (!segs.empty())
    out.push_back(segs.back());
  return out;
}

void arc(Pts &out, float cx, float cy, float r, float a0, float a1, int n)
{
  for (int k = 0; k <= n; k++)
  {
    float a = a0 + (a1 - a0) * k / n;
    out.push_back({cx + cosf(a) * r, cy + sinf(a) * r});
  }
}
} // namespace d2
