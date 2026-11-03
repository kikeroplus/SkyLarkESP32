// 描く部品。ゲームのコードの座標 (1.5 倍) を 2/3 にして、液晶の点に直接描く。円や楕円の点の並びは LovyanGFX と同じ
#include "gfx.h"
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>

// Adafruit GFX の 'classic' 5x7 文字 (BSD License, Copyright (c) 2012 Adafruit Industries)。' ' 〜 0x7F
static const uint8_t FONT[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14,
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49, 0x56, 0x20, 0x50, 0x00, 0x08, 0x07, 0x03, 0x00,
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x08, 0x08, 0x3E, 0x08, 0x08,
    0x00, 0x80, 0x70, 0x30, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x60, 0x60, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x72, 0x49, 0x49, 0x49, 0x46, 0x21, 0x41, 0x49, 0x4D, 0x33,
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x41, 0x21, 0x11, 0x09, 0x07,
    0x36, 0x49, 0x49, 0x49, 0x36, 0x46, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x40, 0x34, 0x00, 0x00,
    0x00, 0x08, 0x14, 0x22, 0x41, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x59, 0x09, 0x06,
    0x3E, 0x41, 0x5D, 0x59, 0x4E, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
    0x7F, 0x41, 0x41, 0x41, 0x3E, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x3E, 0x41, 0x41, 0x51, 0x73,
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41,
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x26, 0x49, 0x49, 0x49, 0x32,
    0x03, 0x01, 0x7F, 0x01, 0x03, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F,
    0x63, 0x14, 0x08, 0x14, 0x63, 0x03, 0x04, 0x78, 0x04, 0x03, 0x61, 0x59, 0x49, 0x4D, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x41,
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x41, 0x7F, 0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x03, 0x07, 0x08, 0x00, 0x20, 0x54, 0x54, 0x78, 0x40, 0x7F, 0x28, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28,
    0x38, 0x44, 0x44, 0x28, 0x7F, 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x08, 0x7E, 0x09, 0x02, 0x18, 0xA4, 0xA4, 0x9C, 0x78,
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x20, 0x40, 0x40, 0x3D, 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x78, 0x04, 0x78, 0x7C, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38,
    0xFC, 0x18, 0x24, 0x24, 0x18, 0x18, 0x24, 0x24, 0x18, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48, 0x54, 0x54, 0x54, 0x24,
    0x04, 0x04, 0x3F, 0x44, 0x24, 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x3C, 0x40, 0x30, 0x40, 0x3C,
    0x44, 0x28, 0x10, 0x28, 0x44, 0x4C, 0x90, 0x90, 0x90, 0x7C, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00,
    0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x41, 0x36, 0x08, 0x00, 0x02, 0x01, 0x02, 0x04, 0x02, 0x3C, 0x26, 0x23, 0x26, 0x3C,
};

static inline uint16_t blend565(uint16_t v, uint32_t c, int a) // v に c を a (0〜256) だけ重ねる
{
  uint32_t o = to888(v);
  int na = 256 - a;
  int r = (((o >> 16) & 255) * na + ((c >> 16) & 255) * a) >> 8;
  int g = (((o >> 8) & 255) * na + ((c >> 8) & 255) * a) >> 8;
  int b = ((o & 255) * na + (c & 255) * a) >> 8;
  return to565((r << 16) | (g << 8) | b);
}

// ---- 点の絵 ----
static void parsePal(Pal &p, const char *s)
{
  while (*s)
  {
    while (*s == ' ')
      s++;
    if (!*s)
      break;
    char ch = s[0];
    p.c[ch & 127] = strtoul(s + 2, nullptr, 16);
    s += 2;
    while (*s && *s != ' ')
      s++;
  }
}

// mirror: 左半分 (真ん中の列まで) だけ書いて、右半分は鏡に映して作る
Sprite::Sprite(std::initializer_list<const char *> r, const char *palette, int ox, int oy, bool mirror) : OX(ox), OY(oy)
{
  memset(&pal, 0, sizeof(pal));
  for (const char *row : r)
  {
    std::string h(row);
    if (mirror)
      for (int i = (int)h.size() - 2; i >= 0; i--)
        h += row[i];
    rows.push_back(h);
    W = max(W, (int)h.size());
  }
  H = rows.size();
  parsePal(pal, palette);
}

Pal Sprite::recolor(const char *palette) const
{
  Pal p = pal;
  parsePal(p, palette);
  return p;
}

// 縮めた版を作る: 描く座標の点 [u, u+1) が、液晶の点 L ([1.5L, 1.5L+1.5)) にかかる割合で色を混ぜる
const Sprite::Low &Sprite::low(const Pal *p, bool flip) const
{
  for (Low *l : lows)
    if (l->pal == p && l->flip == flip)
      return *l;
  Low *l = new Low();
  l->pal = p;
  l->flip = flip;
  int umin = flip ? OX - (W - 1) : -OX, vmin = -OY;
  l->x0 = (int)floorf(umin / 1.5f);
  l->y0 = (int)floorf(vmin / 1.5f);
  l->w = (int)floorf((umin + W) / 1.5f) - l->x0 + 1;
  l->h = (int)floorf((vmin + H) / 1.5f) - l->y0 + 1;
  std::vector<float> acc(l->w * l->h * 4, 0.0f);
  for (int r = 0; r < H; r++)
    for (int c = 0; c < (int)rows[r].size(); c++)
    {
      char ch = rows[r][c];
      if (ch == '.')
        continue;
      uint32_t col = p->c[ch & 127];
      int u = flip ? OX - c : c - OX, v = r - OY;
      for (int ly = (int)floorf(v / 1.5f); ly * 1.5f < v + 1; ly++)
      {
        float wy = min(v + 1.0f, ly * 1.5f + 1.5f) - max((float)v, ly * 1.5f);
        if (wy <= 0)
          continue;
        for (int lx = (int)floorf(u / 1.5f); lx * 1.5f < u + 1; lx++)
        {
          float wx = min(u + 1.0f, lx * 1.5f + 1.5f) - max((float)u, lx * 1.5f);
          if (wx <= 0)
            continue;
          float wt = wx * wy / 2.25f;
          float *a = &acc[((ly - l->y0) * l->w + (lx - l->x0)) * 4];
          a[0] += ((col >> 16) & 255) * wt;
          a[1] += ((col >> 8) & 255) * wt;
          a[2] += (col & 255) * wt;
          a[3] += wt;
        }
      }
    }
  l->px.resize(l->w * l->h);
  for (int i = 0; i < l->w * l->h; i++)
  {
    float *a = &acc[i * 4];
    if (a[3] < 0.05f)
    {
      l->px[i] = 0;
      continue;
    }
    uint32_t A = (uint32_t)(min(1.0f, a[3]) * 255);
    l->px[i] = (A << 24) | ((uint32_t)(a[0] / a[3]) << 16) | ((uint32_t)(a[1] / a[3]) << 8) | (uint32_t)(a[2] / a[3]);
  }
  lows.push_back(l);
  return *l;
}

// ---- 下書き (1.5 倍の絵) ----
void HiCanvas::fillRect(int x, int y, int rw, int rh, uint32_t c)
{
  int x0 = max(x, 0), y0 = max(y, 0), x1 = min(x + rw, w), y1 = min(y + rh, h);
  uint16_t col = to565(c);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++)
      buf[yy * w + xx] = col;
}

void HiCanvas::fillCircle(int x, int y, int r, uint32_t c)
{
  fillRect(x - r, y, (r << 1) + 1, 1, c);
  if (r <= 0)
    return;
  int f = 1 - r, ddFy = -(r << 1), ddFx = 1, i = 0;
  do
  {
    int len = 0;
    while (f < 0)
    {
      f += (ddFx += 2);
      ++len;
    }
    i += len;
    f += (ddFy += 2);
    if (len > 0)
      fillRect(x - r, y + i - len + 1, (r << 1) + 1, len, c);
    fillRect(x - i, y + r, (i << 1) + 1, 1, c);
    fillRect(x - i, y - r, (i << 1) + 1, 1, c);
    if (len > 0)
      fillRect(x - r, y - i, (r << 1) + 1, len, c);
  } while (i < --r);
}

// 3x3 の点を 2x2 の点にする。それぞれの点は、かかる面積の割合 (4:2:2:1) で 4 つの点を混ぜる
void HiCanvas::shrinkTo(uint16_t *dst) const
{
  int dw = w * 2 / 3;
  for (int by = 0; by + 2 < h; by += 3)
  {
    const uint16_t *r0 = buf + by * w, *r1 = r0 + w, *r2 = r1 + w;
    uint16_t *o0 = dst + (by / 3 * 2) * dw, *o1 = o0 + dw;
    for (int bx = 0; bx + 2 < w; bx += 3)
    {
      int R[9], G[9], B[9];
      const uint16_t *src[3] = {r0 + bx, r1 + bx, r2 + bx};
      for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
        {
          uint16_t v = src[j][i];
          R[j * 3 + i] = v >> 11;
          G[j * 3 + i] = (v >> 5) & 63;
          B[j * 3 + i] = v & 31;
        }
      auto px = [&](int a, int b, int c, int d) -> uint16_t { // a が 4、b と c が 2、d が 1
        int r = (R[a] * 4 + R[b] * 2 + R[c] * 2 + R[d] + 4) / 9, g = (G[a] * 4 + G[b] * 2 + G[c] * 2 + G[d] + 4) / 9,
            bb = (B[a] * 4 + B[b] * 2 + B[c] * 2 + B[d] + 4) / 9;
        return (r << 11) | (g << 5) | bb;
      };
      int ox = bx / 3 * 2;
      o0[ox] = px(0, 1, 3, 4);
      o0[ox + 1] = px(2, 1, 5, 4);
      o1[ox] = px(6, 3, 7, 4);
      o1[ox + 1] = px(8, 5, 7, 4);
    }
  }
}

// ---- 帯 ----
static void *allocFast(size_t n)
{
  void *p = heap_caps_malloc(n, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p ? p : ps_malloc(n);
}

// ---- 液晶へ送るタスク (コア 0) ----
struct PushJob
{
  Arduino_GFX *panel;
  const uint16_t *buf;
  int y;
};
static QueueHandle_t pushQ;
static SemaphoreHandle_t pushDone; // 送りおえた帯の数だけ与えられる (はじめは 2 枚とも空き)

static void pushTask(void *)
{
  PushJob j;
  for (;;)
    if (xQueueReceive(pushQ, &j, portMAX_DELAY) == pdTRUE)
    {
      j.panel->draw16bitRGBBitmap(0, j.y, (uint16_t *)j.buf, LW, LSTRIP);
      xSemaphoreGive(pushDone);
    }
}

bool Gfx::begin()
{
  bufs[0] = (uint16_t *)allocFast(LW * LSTRIP * 2);
  bufs[1] = (uint16_t *)allocFast(LW * LSTRIP * 2);
  buf = bufs[0];
  mask = (uint8_t *)allocFast(LW * LSTRIP);
  if (!bufs[0] || !bufs[1] || !mask)
    return false;
  pushQ = xQueueCreate(2, sizeof(PushJob));
  pushDone = xSemaphoreCreateCounting(2, 1); // もう 1 枚 (buf) はいま描いている
  xTaskCreatePinnedToCore(pushTask, "push", 4096, nullptr, 2, nullptr, 0);
  memset(mask, 0, LW * LSTRIP);
  return true;
}

void Gfx::startStrip(int s)
{
  top = s * STRIP;
  ltop = s * LSTRIP;
  maskMode = false;
  mx0 = my0 = 9999;
  mx1 = my1 = -1;
  clearClipRect();
}

void Gfx::setClipRect(int x, int y, int w, int h)
{
  clipL = max(0, toLow(x));
  clipR = min(LW - 1, toLowCeil(x + w) - 1);
  clipT = max(0, toLow(y) - ltop);
  clipB = min(LSTRIP - 1, toLowCeil(y + h) - 1 - ltop);
}

void Gfx::clearClipRect()
{
  clipL = clipT = 0;
  clipR = LW - 1;
  clipB = LSTRIP - 1;
}

void Gfx::lrect(int x, int y, int w, int h, uint16_t col)
{
  y -= ltop;
  int x0 = max(x, clipL), y0 = max(y, clipT), x1 = min(x + w - 1, clipR), y1 = min(y + h - 1, clipB);
  if (x0 > x1 || y0 > y1)
    return;
  if (maskMode)
  {
    for (int yy = y0; yy <= y1; yy++)
      memset(mask + yy * LW + x0, 1, x1 - x0 + 1);
    mx0 = min(mx0, x0);
    mx1 = max(mx1, x1);
    my0 = min(my0, y0);
    my1 = max(my1, y1);
    return;
  }
  for (int yy = y0; yy <= y1; yy++)
  {
    uint16_t *p = buf + yy * LW + x0;
    for (int xx = x0; xx <= x1; xx++)
      *p++ = col;
  }
}

void Gfx::fillRect(int x, int y, int w, int h, uint32_t c)
{
  if (w < 0)
  {
    x += w + 1;
    w = -w;
  }
  if (h < 0)
  {
    y += h + 1;
    h = -h;
  }
  if (w == 0 || h == 0)
    return;
  int x0 = toLow(x), x1 = toLowCeil(x + w), y0 = toLow(y), y1 = toLowCeil(y + h);
  if (y1 <= ltop || y0 >= ltop + LSTRIP)
    return;
  lrect(x0, y0, max(1, x1 - x0), max(1, y1 - y0), to565(c));
}

void Gfx::drawPixel(int x, int y, uint32_t c)
{
  int ly = toLow(y) - ltop, lx = toLow(x);
  if (lx < clipL || lx > clipR || ly < clipT || ly > clipB)
    return;
  if (maskMode)
  {
    mask[ly * LW + lx] = 1;
    mx0 = min(mx0, lx);
    mx1 = max(mx1, lx);
    my0 = min(my0, ly);
    my1 = max(my1, ly);
  }
  else
    buf[ly * LW + lx] = to565(c);
}

void Gfx::blendPixel(int x, int y, uint32_t c, int a)
{
  if (a <= 0 || !rep(x, y))
    return;
  int ly = toLow(y) - ltop, lx = toLow(x);
  if (lx < clipL || lx > clipR || ly < clipT || ly > clipB)
    return;
  uint16_t &v = buf[ly * LW + lx];
  v = blend565(v, c, min(a, 256));
}

int Gfx::get(int x, int y) const
{
  int ly = toLow(y) - ltop, lx = toLow(x);
  if (lx < 0 || lx >= LW || ly < 0 || ly >= LSTRIP)
    return -1;
  return to888(buf[ly * LW + lx]);
}

void Gfx::copyRow(int ly, const uint16_t *src, int n)
{
  ly -= ltop;
  if (ly >= 0 && ly < LSTRIP)
    memcpy(buf + ly * LW, src, n * 2);
}

void Gfx::copySpan(int lx, int ly, const uint16_t *src, int n)
{
  ly -= ltop;
  if (ly < clipT || ly > clipB)
    return;
  int a = max(lx, clipL), b = min(lx + n - 1, clipR);
  if (a <= b)
    memcpy(buf + ly * LW + a, src + (a - lx), (b - a + 1) * 2);
}

void Gfx::drawRect(int x, int y, int w, int h, uint32_t c)
{
  drawFastHLine(x, y, w, c);
  drawFastHLine(x, y + h - 1, w, c);
  drawFastVLine(x, y + 1, h - 2, c);
  drawFastVLine(x + w - 1, y + 1, h - 2, c);
}

// ブレゼンハムの線 (横か縦の一続きの点をまとめて塗る)。端を液晶の座標にしてから引く
void Gfx::drawLine(int x0, int y0, int x1, int y1, uint32_t c)
{
  x0 = toLow(x0);
  y0 = toLow(y0);
  x1 = toLow(x1);
  y1 = toLow(y1);
  if (max(y0, y1) < ltop || min(y0, y1) >= ltop + LSTRIP)
    return;
  uint16_t col = to565(c);
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep)
  {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1)
  {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }
  int dy = abs(y1 - y0), ystep = y1 > y0 ? 1 : -1, dx = x1 - x0, err = dx >> 1;
  int xs = x0, len = 0;
  for (; x0 <= x1; x0++)
  {
    len++;
    if ((err -= dy) < 0)
    {
      if (steep)
        lrect(y0, xs, 1, len, col);
      else
        lrect(xs, y0, len, 1, col);
      err += dx;
      xs = x0 + 1;
      len = 0;
      y0 += ystep;
    }
  }
  if (len > 0)
  {
    if (steep)
      lrect(y0, xs, 1, len, col);
    else
      lrect(xs, y0, len, 1, col);
  }
}

static inline int lowR(int r) { return r <= 0 ? 0 : (r * 2 + 1) / 3; } // 半径を 2/3 に丸める

void Gfx::drawCircle(int x, int y, int r, uint32_t c)
{
  x = toLow(x);
  y = toLow(y);
  r = lowR(r);
  if (y + r < ltop || y - r >= ltop + LSTRIP)
    return;
  uint16_t col = to565(c);
  if (r <= 0)
  {
    lpix(x, y, col);
    return;
  }
  int f = 1 - r, ddFy = -(r << 1), ddFx = 1, i = 0, j = -1;
  do
  {
    while (f < 0)
    {
      ++i;
      f += (ddFx += 2);
    }
    f += (ddFy += 2);
    lrect(x - i, y + r, i - j, 1, col);
    lrect(x - i, y - r, i - j, 1, col);
    lrect(x + j + 1, y - r, i - j, 1, col);
    lrect(x + j + 1, y + r, i - j, 1, col);
    lrect(x + r, y + j + 1, 1, i - j, col);
    lrect(x + r, y - i, 1, i - j, col);
    lrect(x - r, y - i, 1, i - j, col);
    lrect(x - r, y + j + 1, 1, i - j, col);
    j = i;
  } while (i < --r);
}

void Gfx::fillCircle(int x, int y, int r, uint32_t c)
{
  x = toLow(x);
  y = toLow(y);
  r = lowR(r);
  if (y + r < ltop || y - r >= ltop + LSTRIP)
    return;
  uint16_t col = to565(c);
  lrect(x - r, y, (r << 1) + 1, 1, col);
  if (r <= 0)
    return;
  int f = 1 - r, ddFy = -(r << 1), ddFx = 1, i = 0;
  do
  {
    int len = 0;
    while (f < 0)
    {
      f += (ddFx += 2);
      ++len;
    }
    i += len;
    f += (ddFy += 2);
    if (len > 0)
      lrect(x - r, y + i - len + 1, (r << 1) + 1, len, col);
    lrect(x - i, y + r, (i << 1) + 1, 1, col);
    lrect(x - i, y - r, (i << 1) + 1, 1, col);
    if (len > 0)
      lrect(x - r, y - i, (r << 1) + 1, len, col);
  } while (i < --r);
}

void Gfx::fillEllipse(int x, int y, int rx, int ry, uint32_t c)
{
  if (rx < 0 || ry < 0)
    return;
  x = toLow(x);
  y = toLow(y);
  rx = lowR(rx);
  ry = lowR(ry);
  if (y + ry < ltop || y - ry >= ltop + LSTRIP)
    return;
  uint16_t col = to565(c);
  if (ry == 0)
  {
    lrect(x - rx, y, (rx << 1) + 1, 1, col);
    return;
  }
  if (rx == 0)
  {
    lrect(x, y - ry, 1, (ry << 1) + 1, col);
    return;
  }
  int64_t rx2 = rx * rx, ry2 = ry * ry, s;
  int xt, yt, i;
  lrect(x - rx, y, (rx << 1) + 1, 1, col);
  i = 0;
  yt = 0;
  xt = rx;
  s = (rx2 << 1) + ry2 * (1 - (rx << 1));
  do
  {
    while (s < 0)
      s += rx2 * ((++yt << 2) + 2);
    lrect(x - xt, y - yt, (xt << 1) + 1, yt - i, col);
    lrect(x - xt, y + i + 1, (xt << 1) + 1, yt - i, col);
    i = yt;
    s -= (--xt) * ry2 << 2;
  } while (rx2 * yt <= ry2 * xt);
  xt = 0;
  yt = ry;
  s = (ry2 << 1) + rx2 * (1 - (ry << 1));
  do
  {
    while (s < 0)
      s += ry2 * ((++xt << 2) + 2);
    lrect(x - xt, y - yt, (xt << 1) + 1, 1, col);
    lrect(x - xt, y + yt, (xt << 1) + 1, 1, col);
    s -= (--yt) * rx2 << 2;
  } while (ry2 * xt <= rx2 * yt);
}

// 三角形の塗り。上から 1 行ずつ、左右の辺のあいだを塗る
void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t c)
{
  x0 = toLow(x0);
  y0 = toLow(y0);
  x1 = toLow(x1);
  y1 = toLow(y1);
  x2 = toLow(x2);
  y2 = toLow(y2);
  if (y0 > y1)
  {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }
  if (y1 > y2)
  {
    std::swap(y2, y1);
    std::swap(x2, x1);
  }
  if (y0 > y1)
  {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }
  if (y2 < ltop || y0 >= ltop + LSTRIP)
    return;
  uint16_t col = to565(c);
  if (y0 == y2)
  {
    int a = min(x0, min(x1, x2)), b = max(x0, max(x1, x2));
    lrect(a, y0, b - a + 1, 1, col);
    return;
  }
  int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
  int sa = 0, sb = 0, last = y1 == y2 ? y1 : y1 - 1, yy;
  for (yy = y0; yy <= last; yy++)
  {
    int a = x0 + (dy01 != 0 ? sa / dy01 : 0), b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b)
      std::swap(a, b);
    lrect(a, yy, b - a + 1, 1, col);
  }
  sa = dx12 * (yy - y1);
  sb = dx02 * (yy - y0);
  for (; yy <= y2; yy++)
  {
    int a = x1 + (dy12 != 0 ? sa / dy12 : 0), b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b)
      std::swap(a, b);
    lrect(a, yy, b - a + 1, 1, col);
  }
}

// 点の絵: 縮めた版を、覆っている割合で重ねる (縁はなめらかになる)
void Gfx::blit(const Sprite &s, int x, int y, bool flip, const Pal *pal)
{
  const Sprite::Low &l = s.low(pal ? pal : &s.pal, flip);
  int lx0 = toLow(x) + l.x0, ly0 = toLow(y) + l.y0 - ltop;
  if (ly0 + l.h <= 0 || ly0 >= LSTRIP)
    return;
  for (int j = 0; j < l.h; j++)
  {
    int yy = ly0 + j;
    if (yy < clipT || yy > clipB)
      continue;
    for (int i = 0; i < l.w; i++)
    {
      int xx = lx0 + i;
      uint32_t p = l.px[j * l.w + i];
      int A = p >> 24;
      if (A == 0 || xx < clipL || xx > clipR)
        continue;
      if (maskMode)
      {
        if (A >= 128)
        {
          mask[yy * LW + xx] = 1;
          mx0 = min(mx0, xx);
          mx1 = max(mx1, xx);
          my0 = min(my0, yy);
          my1 = max(my1, yy);
        }
        continue;
      }
      uint16_t &v = buf[yy * LW + xx];
      v = A >= 250 ? to565(p) : blend565(v, p, A + 1);
    }
  }
}

void Gfx::applyShadow(uint32_t shade, int a)
{
  if (mx1 < 0)
    return;
  int sr = (shade >> 16) & 255, sg = (shade >> 8) & 255, sb = shade & 255, na = 256 - a;
  for (int y = my0; y <= my1; y++)
  {
    uint8_t *m = mask + y * LW;
    uint16_t *b = buf + y * LW;
    for (int x = mx0; x <= mx1; x++)
      if (m[x])
      {
        m[x] = 0;
        uint32_t v = to888(b[x]);
        int r = (((v >> 16) & 255) * na + sr * a) >> 8, g = (((v >> 8) & 255) * na + sg * a) >> 8, bb = ((v & 255) * na + sb * a) >> 8;
        b[x] = to565((r << 16) | (g << 8) | bb);
      }
  }
  mx0 = my0 = 9999;
  mx1 = my1 = -1;
}

IRAM_ATTR void Gfx::tint(const uint8_t *lr, const uint8_t *lg, const uint8_t *lb, int ds)
{
  if (!ds) // 色を抜かないときは、色ごとの表を引くだけ (5・6 ビットの値から直接 RGB565 の各部分へ)
  {
    uint16_t tr[32], tg[64], tb[32];
    for (int i = 0; i < 32; i++)
    {
      tr[i] = (lr[i << 3] & 0xF8) << 8;
      tb[i] = lb[i << 3] >> 3;
    }
    for (int i = 0; i < 64; i++)
      tg[i] = (lg[i << 2] & 0xFC) << 3;
    uint32_t *p = (uint32_t *)buf, *end = p + LW * LSTRIP / 2; // 2 点ずつ
    while (p < end)
    {
      uint32_t v = *p, lo = v & 0xFFFF, hi = v >> 16;
      *p++ = (tr[lo >> 11] | tg[(lo >> 5) & 63] | tb[lo & 31]) | ((uint32_t)(tr[hi >> 11] | tg[(hi >> 5) & 63] | tb[hi & 31]) << 16);
    }
    return;
  }
  for (int n = 0; n < LW * LSTRIP; n++)
  {
    uint16_t v = buf[n];
    int r = ((v >> 11) & 31) << 3, g = ((v >> 5) & 63) << 2, b = (v & 31) << 3;
    int yy = (r * 77 + g * 150 + b * 29) >> 8;
    r += ((yy - r) * ds) >> 8;
    g += ((yy - g) * ds) >> 8;
    b += ((yy - b) * ds) >> 8;
    buf[n] = ((lr[r] & 0xF8) << 8) | ((lg[g] & 0xFC) << 3) | (lb[b] >> 3);
  }
}

void Gfx::dim()
{
  for (int n = 0; n < LW * LSTRIP; n++)
    buf[n] = (buf[n] >> 1) & 0x7BEF;
}

// ---- 文字 ----
// 描く座標の大きさ size を、液晶の文字の大きさにする (2/3 倍に丸める)
static int lowSize(int size) { return max(1, (size * 2 + 1) / 3); }

int Gfx::textWidth(const char *s, int size) { return strlen(s) * 6 * lowSize(size) * 3 / 2; }

void Gfx::glyph(int lx, int ly, int ch, int size, uint16_t col)
{
  if (ch < 32 || ch > 127)
    return;
  const uint8_t *g = FONT + (ch - 32) * 5;
  for (int gx = 0; gx < 5; gx++)
  {
    int line = g[gx];
    for (int row = 0; row < 8; row++)
      if (line >> row & 1)
        lrect(lx + gx * size, ly + row * size, size, size, col);
  }
}

void Gfx::text(int x, int y, uint32_t c, const char *s, int datum, int size, int fx, uint32_t ink)
{
  int sz = lowSize(size), n = strlen(s);
  int lx = toLow(x), ly = toLow(y);
  if (ly + 8 * sz + 1 < ltop || ly - 1 >= ltop + LSTRIP || n == 0)
    return;
  if (datum == Datum::TC)
    lx -= n * 6 * sz / 2;
  auto put = [&](int ox, int oy, uint32_t cc) {
    uint16_t col = to565(cc);
    for (int i = 0; i < n; i++)
      glyph(lx + ox + i * 6 * sz, ly + oy, (uint8_t)s[i], sz, col);
  };
  if (fx == 1)
    put(1, 1, ink);
  else if (fx == 2)
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
        if (dx || dy)
          put(dx, dy, ink);
  put(0, 0, c);
}

void Gfx::finishStrip(Arduino_GFX *panel)
{
  PushJob j = {panel, buf, ltop};
  xQueueSend(pushQ, &j, portMAX_DELAY);
  uint32_t t0 = micros();
  xSemaphoreTake(pushDone, portMAX_DELAY); // もう 1 枚が送りおわるのを待ってから、そこに次を描く
  usPush += micros() - t0;
  cur ^= 1;
  buf = bufs[cur];
}
