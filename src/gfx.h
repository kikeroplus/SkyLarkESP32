#pragma once
// 画面の絵を描くところ。ゲームのコードは Windows 版と同じ「1.5 倍の点」(360x480) の座標で描くが、
// 実際には液晶の点 (240x320) に直接描く (座標と大きさを 2/3 にする。1.5 倍で描いて縮めると ESP32 では間に合わない)。
// ・四角・円・楕円・線・三角は、2/3 にした座標で液晶の点の並びのまま描く (形がつぶれない)
// ・点ごとに描く絵 (円盤や家の陰影) は、点を 2/3 の位置に置く (いくつかの点は重なって上書きされる)
// ・文字で描いた点の絵 (羊、飛行機など) は、はじめて描くときに色を面積で混ぜて縮めた版を作っておき、それを描く
// ・文字は液晶の 6x8 の文字で描く
// 画面は上から 64 行ずつ 5 本の帯に分けて描き、帯ごとに液晶へ送る (全画面ぶんのメモリを内蔵 RAM に取らないため)
#include <Arduino.h>
#include <vector>
#include <string>

const int SW = 360, SH = 480;   // ゲームのコードが描く座標 (液晶の 1.5 倍)
const int LW = 240, LH = 320;   // 液晶
const int STRIP = 96;           // 帯 1 本の高さ (描く座標)。液晶では 64 行
const int LSTRIP = STRIP * 2 / 3;
const int STRIPS = SH / STRIP;

namespace Datum
{
const int TL = 0; // 左上
const int TC = 1; // 上の真ん中
}

inline uint16_t to565(uint32_t c) { return ((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F); }
inline uint32_t to888(uint16_t v)
{
  uint32_t r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
  return ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
}
// 描く座標 → 液晶の座標 (2/3 にして切り捨て / 切り上げ)。÷3 は遅いので掛け算でする
inline int toLow(int v) { return v >= 0 ? (v * 43691) >> 16 : -((-2 * v + 2) / 3); }
inline int toLowCeil(int v) { return -toLow(-v); }

// 文字で描いた点の絵。'.' は透明、ほかの文字は pal の色。pal は "o=2E3420 G=6E7444 ..." の形で渡す
struct Pal
{
  uint32_t c[128];
};
class Sprite
{
public:
  std::vector<std::string> rows;
  int W = 0, H = 0, OX, OY;
  Pal pal;

  Sprite(std::initializer_list<const char *> r, const char *palette, int ox, int oy, bool mirror = false);
  Pal recolor(const char *palette) const; // 一部の色だけ差しかえた色の表

  // 縮めた版 (色の表と左右反転ごとに作って覚えておく)
  struct Low
  {
    const Pal *pal;
    bool flip;
    int x0, y0, w, h;          // 原点から見た左上と大きさ (液晶の点)
    std::vector<uint32_t> px;  // 0xAARRGGBB (A = 覆っている割合)
  };
  mutable std::vector<Low *> lows;
  const Low &low(const Pal *p, bool flip) const;
};

// 地面の行など、描く座標のまま点を並べる下書き (縮める前の 1.5 倍の絵)
class HiCanvas
{
public:
  uint16_t *buf;
  int w, h;
  void fillRect(int x, int y, int rw, int rh, uint32_t c);
  void drawPixel(int x, int y, uint32_t c) { fillRect(x, y, 1, 1, c); }
  void drawFastVLine(int x, int y, int rh, uint32_t c) { fillRect(x, y, 1, rh, c); }
  void fillCircle(int x, int y, int r, uint32_t c);
  void shrinkTo(uint16_t *dst) const; // 3x3 の点を 2x2 にして (面積で色を混ぜて) dst へ
};

class Gfx
{
public:
  uint16_t *buf = nullptr; // 帯の点 (LW x LSTRIP、RGB565)
  uint8_t *mask = nullptr; // 影の印
  bool maskMode = false;
  int top = 0; // 帯の上端 (描く座標)

  bool begin();
  void startStrip(int s);

  bool visible(int y0, int y1) const { return y1 >= top - 2 && y0 < top + STRIP + 2; } // y0〜y1 がこの帯にかかるか
  // 点ごとに描くとき、この点が液晶の点の代表か (3 点のうち 2 点だけが代表。重い絵は代表の点だけ計算すれば同じ絵になる)
  static bool rep(int x, int y) { return ((x % 3) + 3) % 3 != 1 && ((y % 3) + 3) % 3 != 1; }

  void setClipRect(int x, int y, int w, int h);
  void clearClipRect();

  void fillRect(int x, int y, int w, int h, uint32_t c);
  void drawPixel(int x, int y, uint32_t c);
  void blendPixel(int x, int y, uint32_t c, int a); // 半透明に重ねる (a は 0〜256)。代表の点だけ塗る (重ねすぎないように)
  void drawFastHLine(int x, int y, int w, uint32_t c) { fillRect(x, y, w, 1, c); }
  void drawFastVLine(int x, int y, int h, uint32_t c) { fillRect(x, y, 1, h, c); }
  void drawRect(int x, int y, int w, int h, uint32_t c);
  void drawLine(int x0, int y0, int x1, int y1, uint32_t c);
  void drawCircle(int x, int y, int r, uint32_t c);
  void fillCircle(int x, int y, int r, uint32_t c);
  void fillEllipse(int x, int y, int rx, int ry, uint32_t c);
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t c);
  void blit(const Sprite &s, int x, int y, bool flip, const Pal *pal = nullptr);
  void lowPixel(int lx, int ly, uint16_t col) { lrect(lx, ly, 1, 1, col); } // 液晶の座標 (画面全体) の点
  int get(int x, int y) const; // その点の色 (0xRRGGBB。帯の外は -1)
  void copyRow(int ly, const uint16_t *src, int n); // 液晶の行 ly (画面の座標) に、描き置きの行を写す
  void copySpan(int lx, int ly, const uint16_t *src, int n); // 液晶の (lx, ly) から横に n 点を写す (切り抜きつき)

  void applyShadow(uint32_t shade, int a); // 印をつけたところを shade の色へ a (0〜256) だけ寄せて暗くし、印を消す
  void tint(const uint8_t *lr, const uint8_t *lg, const uint8_t *lb, int ds); // 色の変換表を帯全体に掛ける
  void dim();                                                              // 半分の明るさにする

  // ---- 文字 (6x8 の点の文字。大きさ size は描く座標での倍率で、液晶では 2/3 倍に丸める) ----
  // fx: 0 = そのまま、1 = 右下に影、2 = まわりを縁どる (縁と影の色は ink)
  static int textWidth(const char *s, int size); // 描く座標での幅
  void text(int x, int y, uint32_t c, const char *s, int datum = Datum::TL, int size = 1, int fx = 0, uint32_t ink = 0);

  // 帯を液晶へ送る。送るのはコア 0 のタスクで、そのあいだにもう 1 枚の帯に次を描く (前の帯を送りおえるまでは待つ)
  void finishStrip(class Arduino_GFX *panel);
  uint32_t usPush = 0; // 送りおえるのを待った時間 (確かめる用)

private:
  uint16_t *bufs[2] = {};
  int cur = 0;
  int ltop = 0;                   // 帯の上端 (液晶の座標)
  int clipL, clipT, clipR, clipB; // 描ける範囲 (液晶の座標、帯の中。端も含む)
  int mx0, my0, mx1, my1;         // 影の印をつけた範囲 (applyShadow で見るところ)

  void lrect(int x, int y, int w, int h, uint16_t col); // 液晶の座標 (画面全体) の四角
  void lpix(int x, int y, uint16_t col) { lrect(x, y, 1, 1, col); }
  void glyph(int lx, int ly, int ch, int size, uint16_t col);
};
