// 3 本目 (LONGSHOT) のシーン 1: ホテル前の通り。石造りのホテルと、1 階のカフェ・花屋・玄関、2 車線の通り
#include "g3world.h"
#include <math.h>

namespace w3
{
static const uint32_t IRON = 0x2a2a2e, BRASS = 0xb08d3a;
static const uint32_t STONE = 0xcdbb9c, STONE_D = 0xa8967a, STONE_L = 0xe2d6bd, CORN = 0x9b8a70;
static const int COLS = 8, ROWS = 5;
static inline float winX(int c) { return 16 + c * 27; }
static inline float winY(int r) { return 52 + r * 22; }
static const float WIN_W = 17, WIN_H = 15;
static const float TREE_X[3] = {26, 118, 210};
static const int NTREE = 3;

// ---- 窓の決まった見た目 ----
static float wh(int r, int c, int k) { return d2::hs(r * 31 + c * 7 + k * 101 + 0.5f); }
static bool winLit(int r, int c) { return wh(r, c, 1) < 0.5f; }
static const uint32_t CURTAIN[6] = {0xe0d4b8, 0xb8c8d0, 0xc89a8a, 0xd8c48a, 0x9ab09a, 0xf0ece4};

static void window(int r, int c)
{
  float x = winX(c), y = winY(r);
  bool lit = tod != DAY && winLit(r, c);
  int ct = (int)(wh(r, c, 2) * 4); // 0 なし / 1 両側 / 2 上半分 / 3 ブラインド
  uint32_t cc = CURTAIN[(int)(wh(r, c, 3) * 6)];
  uint32_t lc = wh(r, c, 4) < 0.15f ? 0x8ab0f0 : wh(r, c, 4) < 0.3f ? 0xe0ecf4 : 0xf2c46c; // テレビの青・白い蛍光灯・暖かい電球
  // 石の枠、まぐさ石と要石、窓台
  R(x - 1.8f, y - 1.8f, WIN_W + 3.6f, WIN_H + 3.2f, STONE_L);
  R(x - 2.8f, y - 3.4f, WIN_W + 5.6f, 2, STONE_L);
  R(x - 2.8f, y - 1.6f, WIN_W + 5.6f, 0.4f, STONE_D);
  Q(x + 7, y - 3.8f, x + 10, y - 3.8f, x + 9.6f, y - 0.8f, x + 7.4f, y - 0.8f, 0xece2cc);
  R(x - 2.8f, y + WIN_H + 0.6f, WIN_W + 5.6f, 1.4f, STONE_L);
  R(x - 2.4f, y + WIN_H + 2, WIN_W + 4.8f, 0.7f, 0x000000, 70);
  R(x - 0.6f, y - 0.6f, WIN_W + 1.2f, WIN_H + 1.2f, 0x4a3c30);
  // ガラス
  for (int i = 0; i < (int)(WIN_H * 4); i++)
  {
    float k = i / (WIN_H * 4);
    uint32_t g;
    if (lit)
      g = mix(shade(lc, 0.75f), lc, 0.4f + k * 0.6f) | EMIT;
    else if (tod == DAY)
      g = mix(0xa8c8dc, 0x3e5a70, k);
    else
      g = mix(0x2a3446, 0x101620, k) | EMIT;
    R(x, y + i / 4.0f, WIN_W, 0.25f, g);
  }
  if (lit) // 部屋の中: 奥の壁と、天井の明かり、棚
  {
    R(x, y + WIN_H - 4, WIN_W, 4, shade(lc, 0.5f) | EMIT);
    if (wh(r, c, 5) < 0.5f)
      R(x + 2 + wh(r, c, 6) * 8, y + 4, 4, 7, shade(lc, 0.55f) | EMIT);
    C(x + 4 + wh(r, c, 7) * 9, y + 1.6f, 0.9f, 0xfff4d0 | EMIT);
  }
  else if (tod == DAY) // 空の映り込みの斜めの筋
  {
    Q(x + 2, y + WIN_H, x + 5, y + WIN_H, x + 12, y, x + 9, y, 0xffffff, 40);
    Q(x + 6, y + WIN_H, x + 7, y + WIN_H, x + 14, y, x + 13, y, 0xffffff, 30);
  }
  // カーテン
  uint32_t cu = lit ? (mix(cc, lc, 0.45f) | EMIT) : cc;
  if (ct == 1)
  {
    for (int s = 0; s < 2; s++)
    {
      float cx = s ? x + WIN_W - 4.2f : x;
      R(cx, y, 4.2f, WIN_H, cu);
      for (float f = 0.7f; f < 4; f += 1.1f)
        R(cx + f, y, 0.3f, WIN_H, shade(cu, 0.8f));
    }
    R(x, y, WIN_W, 1.2f, shade(cu, 0.85f));
  }
  else if (ct == 2)
  {
    R(x, y, WIN_W, 5.5f, cu);
    for (int i = 0; i < 6; i++)
      C(x + 1.4f + i * 2.9f, y + 5.5f, 1.4f, cu);
    for (float f = 1; f < WIN_W; f += 2.9f)
      R(x + f, y, 0.3f, 5.5f, shade(cu, 0.82f));
  }
  else if (ct == 3)
  {
    for (float f = 0; f < 8; f += 0.8f)
      R(x, y + f, WIN_W, 0.45f, cu, 220);
    L(x + 3, y, x + 3, y + 8.5f, 0.2f, shade(cu, 0.6f));
  }
  // 窓の桟
  R(x + 8, y, 1, WIN_H, 0x5a4a3a);
  R(x, y + 5, WIN_W, 0.8f, 0x5a4a3a);
  if (r != 4 && wh(r, c, 8) < 0.22f) // 窓台の鉢植え
  {
    float px = x + 2 + wh(r, c, 9) * 10;
    R(px, y + WIN_H - 1.6f, 3.4f, 2.2f, 0xa0522d);
    C(px + 1.7f, y + WIN_H - 2.4f, 1.6f, 0x4a7a3a);
    C(px + 0.8f, y + WIN_H - 3.2f, 0.6f, wh(r, c, 10) < 0.5f ? 0xd84a4a : 0xf0d040);
    C(px + 2.6f, y + WIN_H - 3, 0.6f, 0xe86aa0);
  }
  if (r < 3 && wh(r, c, 11) < 0.2f) // 窓の下のエアコンの室外機
  {
    float ax = x + 9 + wh(r, c, 12) * 4;
    R(ax, y + WIN_H + 3, 7, 4.6f, 0xd4d4d0);
    R(ax, y + WIN_H + 3, 7, 0.5f, 0xeeeeea);
    ring(ax + 4.6f, y + WIN_H + 5.3f, 1.6f, 0.25f, 0x6a6a6a);
    L(ax + 3, y + WIN_H + 5.3f, ax + 6.2f, y + WIN_H + 5.3f, 0.2f, 0x6a6a6a);
    for (float g = 0.5f; g < 2.4f; g += 0.6f)
      R(ax + 0.6f, y + WIN_H + 3.6f + g, 1.8f, 0.25f, 0x8a8a8a);
    R(ax + 0.4f, y + WIN_H + 7.6f, 6.2f, 0.8f, 0x000000, 50);
    L(ax + 1, y + WIN_H + 7.6f, ax + 1.2f, y + WIN_H + 12, 0.3f, 0x6a5a48, 90); // 水の垂れたしみ
  }
}

static void balcony(int c)
{
  float x = winX(c) - 3, y = winY(4);
  R(x, y + WIN_H + 0.8f, WIN_W + 6, 1.8f, STONE_L);
  R(x + 0.4f, y + WIN_H + 2.6f, WIN_W + 5.2f, 0.8f, 0x000000, 70);
  for (float k = 0; k <= WIN_W + 6; k += 1.4f)
    L(x + k, y + 9, x + k, y + WIN_H + 0.8f, 0.3f, IRON);
  R(x, y + 8.6f, WIN_W + 6, 0.7f, IRON);
  R(x, y + 12.5f, WIN_W + 6, 0.4f, IRON);
  // 手すりの花箱
  R(x + 1, y + 7, WIN_W + 4, 1.8f, 0x7a4a30);
  for (int i = 0; i < 9; i++)
  {
    float fx = x + 2 + i * 2.2f;
    C(fx, y + 6.6f, 1.1f, i % 2 ? 0x4a7a3a : 0x5a8a44);
    C(fx + 0.5f, y + 6, 0.5f, i % 3 == 0 ? 0xe04848 : i % 3 == 1 ? 0xf0f0f0 : 0xe878b0);
  }
}

static void hotel()
{
  // 屋上の物 (欄干の後ろ)
  L(24, 8, 24, 36, 0.6f, 0xd0d0d0);
  C(24, 7.6f, 0.7f, 0xd8b048);
  R(38, 28, 18, 8, 0xb8b8b4);
  R(38, 28, 18, 0.6f, 0xd8d8d4);
  ring(43, 32, 2.4f, 0.35f, 0x6a6a6a);
  ring(51, 32, 2.4f, 0.35f, 0x6a6a6a);
  // 給水塔
  for (float lx : {188.0f, 197.0f, 206.0f})
    L(lx, 26, lx + (lx < 197 ? 1.5f : lx > 197 ? -1.5f : 0), 37, 0.5f, 0x3a3a3a);
  L(188.5f, 31, 205.5f, 31, 0.3f, 0x3a3a3a);
  R(186, 13, 22, 13, 0x7a5a40);
  for (float k = 0; k < 22; k += 2.2f)
    R(186 + k, 13, 0.3f, 13, 0x5a4030);
  for (float hy : {15.0f, 19.5f, 24.0f})
    R(186, hy, 22, 0.6f, 0x2a2a2a);
  R(186, 13, 1.2f, 13, 0x9a7a58);
  T(184.6f, 13, 209.4f, 13, 197, 7, 0x5a4030);
  T(184.6f, 13, 197, 13, 197, 7, 0x6e5038);
  // アンテナ
  L(218, 2, 218, 36, 0.5f, 0x3a3a40);
  L(214, 8, 222, 8, 0.4f, 0x3a3a40);
  L(215, 14, 221, 14, 0.4f, 0x3a3a40);
  if (tod != DAY)
    C(218, 2, 0.8f, 0xff3020 | EMIT);
  // 屋上の看板 (鉄骨の枠と文字)
  const char *name = "HOTEL MIRADOR";
  float cell = 1.05f, w = signW(name, cell), x0 = 120 - w / 2;
  for (float k = x0 - 3; k <= x0 + w + 3; k += 7)
  {
    L(k, 26, k, 36, 0.5f, IRON);
    if (k + 7 <= x0 + w + 3)
    {
      L(k, 27, k + 7, 35, 0.25f, IRON);
      L(k, 35, k + 7, 27, 0.25f, IRON);
    }
  }
  L(x0 - 3, 27, x0 + w + 3, 27, 0.5f, IRON);
  L(x0 - 3, 35, x0 + w + 3, 35, 0.5f, IRON);
  if (tod == DAY)
  {
    sign(x0 + 0.35f, 27.3f, cell, name, 0x5a2020);
    sign(x0, 26.9f, cell, name, 0xf2e6c8);
  }
  else
  {
    for (int i = 0; i < 3; i++) // ネオンのにじみ
      sign(x0 - 0.5f, 26.4f, cell * 1.14f, name, 0xff6040 | EMIT, 40);
    sign(x0, 26.9f, cell, name, (tod == NIGHT ? 0xffb07a : 0xffd0a0) | EMIT);
  }

  // 両隣のビル (左はれんが、右は灰色のタイル)
  R(0, 30, 6, 168, 0x8a4e3c);
  for (float y = 34; y < 196; y += 1.6f)
    R(0, y, 6, 0.2f, 0x6a3a2c, 140);
  R(234, 22, 6, 176, 0x7a8088);
  for (float y = 26; y < 196; y += 3)
    R(234, y, 6, 0.2f, 0x5a6068, 140);
  for (float y = 44; y < 160; y += 22)
  {
    R(1, y, 4, 12, tod != DAY && hh(y, 1) < 0.5f ? (0xf2c46c | EMIT) : 0x3a4656);
    R(235, y + 6, 4, 12, tod != DAY && hh(y, 2) < 0.5f ? (0xe0ecf4 | EMIT) : 0x3a4656);
  }
  R(5, 30, 1, 168, 0x000000, 70);
  R(234, 22, 1, 176, 0x000000, 50);
  // 壁 (石積み)
  tex(6, 38, 228, 122, STONE, 0.10f, 8, 1);
  for (float y = 41; y < 160; y += 3) // 石の目地
  {
    R(6, y, 228, 0.25f, STONE_D, 120);
    float off = ((int)(y / 3) % 2) * 3.5f;
    for (float x = 6 + off; x < 234; x += 7)
      R(x, y, 0.25f, 3, STONE_D, 90);
  }
  // 隅石と付け柱
  for (float y = 38; y < 160; y += 6)
  {
    R(6, y, 6, 2.8f, STONE_L);
    R(228, y, 6, 2.8f, STONE_L);
    R(6, y + 3, 4, 2.8f, STONE_L);
    R(230, y + 3, 4, 2.8f, STONE_L);
  }
  for (float px : {89.5f, 170.5f})
  {
    R(px, 38, 5, 122, STONE_L);
    R(px + 4.2f, 38, 0.8f, 122, STONE_D);
    R(px - 0.6f, 38, 0.6f, 122, 0x000000, 40);
  }
  // 階ごとの帯
  for (int r = 0; r < ROWS; r++)
  {
    float y = winY(r) - 4.6f;
    R(6, y, 228, 1.2f, STONE_L);
    R(6, y + 1.2f, 228, 0.5f, STONE_D);
  }
  // 軒の蛇腹と歯飾り
  R(5, 33.6f, 230, 2.4f, STONE_D);
  R(4, 36, 232, 3, CORN);
  R(3, 38.8f, 234, 1.8f, STONE_L);
  for (float x = 5; x < 235; x += 2)
    R(x, 40.6f, 1, 1.4f, STONE_D);
  R(6, 42, 228, 1.5f, 0x000000, 70);
  // 窓とバルコニー
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      window(r, c);
  for (int c : {1, 3, 4, 6})
    balcony(c);

  // ---- 1 階 ----
  tex(6, 160, 228, 38, 0xbfae90, 0.08f, 8, 9);
  for (float y = 161; y < 198; y += 3)
    R(6, y, 228, 0.5f, 0x7a6a54, 150); // 1 階は深い目地 (ルスティカ)
  R(6, 159, 228, 1.2f, STONE_L);
  for (float px : {92.0f, 160.0f})
  {
    R(px, 160, 4, 38, STONE_L);
    R(px + 3.4f, 160, 0.6f, 38, STONE_D);
  }
  // カフェ
  bool warm = tod != DAY;
  R(10, 170, 80, 28, 0x3a2a22);
  for (float wx : {13.0f, 59.0f})
  {
    for (int i = 0; i < 21 * 4; i++)
    {
      float k = i / 84.0f;
      R(wx, 175 + i / 4.0f, 28, 0.25f, warm ? (mix(0xe8a860, 0x9a5a30, k) | EMIT) : mix(0x6a7a82, 0x2a3036, k));
    }
    R(wx, 188, 28, 2.6f, warm ? (0x6a3a20 | EMIT) : 0x4a3424);   // カウンター
    for (int i = 0; i < 3; i++)
    {
      float lx = wx + 5 + i * 9;
      L(lx, 175, lx, 178, 0.2f, 0x1a1a1a);
      T(lx - 1.4f, 179, lx + 1.4f, 179, lx, 177.6f, warm ? (0xffe0a0 | EMIT) : 0x2a2a2a);
    }
    if (!warm)
      Q(wx + 3, 196, wx + 7, 196, wx + 16, 175, wx + 12, 175, 0xffffff, 35);
    R(wx + 13.6f, 175, 0.8f, 21, 0x3a2a22);
  }
  R(46, 176, 10, 22, warm ? (0xd89858 | EMIT) : 0x4a5a62);
  R(45.5f, 175.5f, 11, 0.8f, 0x2a1c16);
  R(45.5f, 175.5f, 0.8f, 22.5f, 0x2a1c16);
  R(55.2f, 175.5f, 0.8f, 22.5f, 0x2a1c16);
  R(53, 186, 0.6f, 3, BRASS);
  // カフェの日よけ (赤と白の縞、下は波形)
  R(9, 162, 82, 1.5f, 0x7a1f1a);
  for (int i = 0; i < 20; i++)
  {
    uint32_t sc = i % 2 ? 0xc8423a : 0xf0e6d4;
    float ax = 9 + i * 4.1f;
    Q(ax, 163.5f, ax + 4.1f, 163.5f, ax + 4.1f, 170, ax, 170, sc);
  }
  R(9, 163.5f, 82, 1.2f, 0xffffff, 40);
  R(9, 168, 82, 2, 0x000000, 45);
  R(9, 170, 82, 3, 0xb0352e);
  sign(50 - signW("CAFE", 0.42f) / 2, 170.5f, 0.42f, "CAFE", 0xf6eedc);
  for (int i = 0; i < 20; i++)
    T(9 + i * 4.1f, 173, 13.1f + i * 4.1f, 173, 11.05f + i * 4.1f, 174.6f, 0xb0352e);
  R(10, 174.6f, 80, 3, 0x000000, 80); // 日よけの影
  // 花屋
  R(98, 170, 60, 28, 0x23402e);
  for (int i = 0; i < 18 * 4; i++)
  {
    float k = i / 72.0f;
    R(101, 174 + i / 4.0f, 54, 0.25f, warm ? (mix(0xf0e0b0, 0xa08850, k) | EMIT) : mix(0x7a8a8a, 0x34403e, k));
  }
  for (int i = 0; i < 14; i++) // 店の中の花
  {
    float fx = 103 + i * 3.7f, fy = 184 + (i % 3) * 1.5f;
    C(fx, fy, 1.4f, 0x3a6a34);
    C(fx - 0.4f, fy - 0.8f, 0.7f, i % 4 == 0 ? 0xe84a5a : i % 4 == 1 ? 0xf0d040 : i % 4 == 2 ? 0xf0f0f0 : 0xb070d0);
  }
  R(127, 174, 0.8f, 18, 0x23402e);
  Q(96, 163, 160, 163, 162, 171, 94, 171, 0x2f6a45);
  for (float k = 0; k < 66; k += 4.4f)
    Q(96 + k, 163, 98.2f + k, 163, 96.4f + k * 1.03f, 171, 94 + k * 1.03f, 171, 0x3f8a5a);
  R(94, 171, 68, 2.6f, 0x24553a);
  sign(128 - signW("FLOWERS", 0.34f) / 2, 171.4f, 0.34f, "FLOWERS", 0xf0ecd8);
  R(98, 173.6f, 60, 2.4f, 0x000000, 70);
  // 店先の花の段
  R(100, 193, 56, 5, 0x6a4a30);
  R(102, 189, 52, 4, 0x7a5a3a);
  R(100, 193, 56, 0.5f, 0x9a7a5a);
  static const uint32_t FL[6] = {0xe84a5a, 0xf0d040, 0xf4f0ec, 0xb070d0, 0xf08a40, 0xe878b0};
  for (int i = 0; i < 9; i++)
    for (int row = 0; row < 2; row++)
    {
      float bx = 103 + i * 5.6f + row * 2.6f, by = row ? 190 : 194;
      if (bx > 152)
        continue;
      R(bx, by - 2.6f, 3.6f, 3.4f, 0x9aa0a6);
      R(bx, by - 2.6f, 0.8f, 3.4f, 0xc8ccd0);
      C(bx + 1.8f, by - 3.6f, 1.9f, 0x3a6a34);
      for (int k = 0; k < 4; k++)
        C(bx + 0.6f + (k % 2) * 2.4f, by - 4.4f + (k / 2) * 1.1f, 0.75f, FL[(i + row * 3 + k) % 6]);
    }
  // ホテルの玄関: 丸い上の入口、回転扉、ひさし、鉢植え
  C(198, 176, 16, STONE_L);
  R(182, 176, 32, 22, STONE_L);
  C(198, 176, 14, 0x2b2420);
  R(184, 176, 28, 22, 0x2b2420);
  for (int i = 0; i < 7; i++) // 扇形の明かり取り
  {
    float a = 3.1416f + i * 3.1416f / 6;
    L(198, 176, 198 + cosf(a) * 13.6f, 176 + sinf(a) * 13.6f, 0.3f, BRASS);
  }
  C(198, 176, 12.6f, warm ? (0xf8d890 | EMIT) : 0x7a8a96, 110);
  R(186, 175, 24, 23, warm ? (0xf0c878 | EMIT) : 0x8aa0ae);
  for (float k = 186; k <= 210; k += 6)
    R(k - 0.3f, 175, 0.6f, 23, BRASS);
  R(185, 174, 26, 1.4f, BRASS);
  Q(190, 198, 192, 198, 196, 175, 194, 175, 0xffffff, 50);
  // ひさし
  R(176, 166.5f, 44, 4.2f, 0x6e1f24);
  R(176, 166.5f, 44, 0.6f, 0x9a3a3a);
  R(176, 170.3f, 44, 0.8f, 0xc9a24a);
  sign(198 - signW("MIRADOR", 0.38f) / 2, 167.4f, 0.38f, "MIRADOR", 0xe8c860);
  R(176, 171.1f, 44, 2.2f, 0x000000, 70);
  L(178, 171, 178, 198, 0.6f, BRASS);
  L(218, 171, 218, 198, 0.6f, BRASS);
  for (float wx : {181.0f, 215.0f}) // 壁の灯り
  {
    R(wx - 0.5f, 177, 1, 2.4f, BRASS);
    C(wx, 176.4f, 1, warm ? (0xffe8b0 | EMIT) : 0xe8dcc0);
    if (warm)
      C(wx, 176.4f, 3, 0xffd890 | EMIT, 50);
  }
  for (float px : {172.5f, 223.5f}) // 鉢植えの丸い木
  {
    R(px - 2.6f, 190, 5.2f, 8, 0x3a3a40);
    R(px - 3, 190, 6, 1, 0x55555c);
    R(px - 2.6f, 190, 1, 8, 0x5a5a62);
    L(px, 186, px, 190, 0.5f, 0x5b4330);
    C(px, 185.5f, 3.6f, 0x3a6a32);
    C(px - 1, 184.4f, 2, 0x5a8f48);
    C(px + 1.4f, 186.6f, 1.4f, 0x2e5428);
  }
}

static void street()
{
  bool on = tod != DAY;
  // ---- 奥の歩道 ----
  tex(0, 198, WW, 24, 0xbab5ab, 0.07f, 24, 3, 0.05f);
  for (float y = 204; y < 222; y += 6)
    R(0, y, WW, 0.25f, 0x8f8a80, 110);
  for (int row = 0; row < 4; row++)
    for (float x = (row % 2) * 4; x < WW; x += 8)
      R(x, 198 + row * 6, 0.25f, 6, 0x8f8a80, 90);
  R(6, 198, 228, 1.5f, 0x000000, 60);
  R(186, 198, 24, 5, 0x8a1f24); // ホテルの赤いじゅうたんと、ひさしの支柱の足 (歩道の上に描く)
  R(186, 198, 24, 0.6f, 0x6a161a);
  R(186, 202.4f, 24, 0.6f, 0x5a1216);
  L(178, 197, 178, 206, 0.6f, BRASS);
  L(218, 197, 218, 206, 0.6f, BRASS);
  E(178, 206.2f, 1, 0.35f, BRASS);
  E(218, 206.2f, 1, 0.35f, BRASS);
  if (on) // 街灯と店の明かりが歩道に落ちる
  {
    for (float lx : {60.0f, 150.0f})
      E(lx, 216, 18, 5, 0xffd890 | EMIT, tod == NIGHT ? 70 : 40);
    E(50, 201, 40, 3.5f, 0xf0a860 | EMIT, 50);
    E(128, 201, 30, 3.5f, 0xf0e0a0 | EMIT, 45);
    E(198, 202, 16, 4, 0xf0c878 | EMIT, 55);
  }
  // カフェの机といす
  for (float tx : {22.0f, 70.0f})
  {
    for (int s = -1; s <= 1; s += 2)
    {
      float cx = tx + s * 5.5f;
      L(cx - s * 0.4f, 199.5f, cx - s * 0.6f, 206, 0.45f, 0x8a6a40);
      R(cx - 1.6f, 203.4f, 3.2f, 0.8f, 0x9a7a48);
      L(cx - 1.4f, 204.2f, cx - 1.6f, 207, 0.3f, 0x6a4a28);
      L(cx + 1.4f, 204.2f, cx + 1.6f, 207, 0.3f, 0x6a4a28);
    }
    L(tx, 202.6f, tx, 206.4f, 0.5f, 0x2a2a2a);
    E(tx, 206.7f, 1.8f, 0.5f, 0x2a2a2a);
    E(tx, 202.3f, 4, 1.1f, 0xeae6de);
    E(tx, 202.6f, 4, 0.8f, 0xb8b4ac, 120);
    C(tx - 1, 201.6f, 0.5f, 0xffffff);
    C(tx + 1.4f, 201.8f, 0.4f, 0xd84a4a);
  }
  // 黒板の立て看板
  T(80, 207.5f, 84, 207.5f, 82, 199, 0x5a4030);
  R(79.4f, 200.4f, 5.2f, 6, 0x5a4030);
  R(79.9f, 200.9f, 4.2f, 5, 0x2a322c);
  for (int i = 0; i < 3; i++)
    L(80.4f, 202 + i * 1.3f, 83.4f - i * 0.5f, 202 + i * 1.3f, 0.2f, 0xe8e8e0, 200);
  // 消火栓、ごみ箱、車止め
  R(106.6f, 215.4f, 2.8f, 5.6f, 0xb8302a);
  R(105.6f, 216.8f, 4.8f, 1.3f, 0xa02820);
  C(108, 215.4f, 1.4f, 0xc8382f);
  R(106.6f, 215.4f, 0.7f, 5.6f, 0xe86050);
  R(106, 220.6f, 4, 0.9f, 0x6a1a14);
  R(130, 212.4f, 4.4f, 8.6f, 0x2f5a3a);
  for (float k = 0.6f; k < 4.4f; k += 1.1f)
    R(130 + k, 213, 0.3f, 8, 0x24452c);
  E(132.2f, 212.4f, 2.5f, 0.8f, 0x24452c);
  for (float bx : {172.0f, 216.0f, 236.0f})
  {
    R(bx - 0.8f, 216, 1.6f, 5.6f, 0x3a3a3e);
    C(bx, 216, 0.85f, 0x4a4a50);
    R(bx - 0.8f, 217.2f, 1.6f, 0.5f, 0xd8d8d0);
  }
  // 縁石
  R(0, 221.4f, WW, 1.4f, 0xd4d0c8);
  R(0, 222.8f, WW, 1.2f, 0x86827a);
  for (float gx : {38.0f, 196.0f})
  {
    R(gx, 221.6f, 6, 1.2f, 0x2a2a2a);
    for (float k = 0.5f; k < 6; k += 1)
      R(gx + k, 221.6f, 0.3f, 1.2f, 0x6a6a6a);
  }
  // ---- 車道 ----
  tex(0, 224, WW, 44, 0x4a4d52, 0.06f, 4, 5, 0.16f);
  R(0, 229, WW, 5, 0x000000, 22); // 車の通る筋
  R(0, 251, WW, 5, 0x000000, 22);
  R(88, 231, 16, 6, 0x55585d, 140); // 補修の跡
  R(140, 256, 22, 5, 0x404348, 140);
  L(20, 260, 34, 262, 0.25f, 0x2a2c30, 160);
  L(34, 262, 40, 261, 0.25f, 0x2a2c30, 160);
  E(160, 236, 5, 1.6f, 0x2a2420, 60); // 油のしみ
  for (float x = -4; x < WW + 4; x += 16)
    R(x, 245.2f, 8, 0.9f, 0xe8e4d8, 220);
  for (int i = 0; i < 6; i++) // 横断歩道
  {
    float cx = 176 + i * 6.2f;
    R(cx, 224.8f, 3.6f, 42.4f, 0xdcd8cc, 230);
    for (int k = 0; k < 10; k++) // すり減り
      R(cx + d2::hs(i * 13 + k) * 3.2f, 225 + d2::hs(i * 7 + k * 3) * 41, 0.5f, 0.4f, 0x4a4d52, 160);
  }
  R(170, 247, 1.2f, 20, 0xe8e4d8, 220); // 停止線
  R(214.8f, 225, 1.2f, 20, 0xe8e4d8, 220);
  for (float mx : {56.0f, 132.0f}) // マンホール
  {
    float my = mx < 100 ? 237 : 259;
    E(mx, my, 3.8f, 1.7f, 0x2e3034);
    E(mx, my, 3.2f, 1.3f, 0x45484e);
    for (int k = -2; k <= 2; k++)
      R(mx - 2.6f, my + k * 0.45f - 0.1f, 5.2f, 0.18f, 0x2e3034);
  }
  if (on)
    E(60, 226, 14, 2, 0xffd890 | EMIT, 30);
  // 手前の縁石
  R(0, 268, WW, 2.4f, 0xd0ccc4);
  R(0, 268, WW, 0.5f, 0x86827a);
  // ---- 手前の歩道 (れんが) ----
  R(0, 270.4f, WW, 30, 0x8f8578);
  for (int row = 0; row < 15; row++)
    for (int i = -1; i < 61; i++)
    {
      float bx = i * 4 + (row % 2) * 2, by = 270.6f + row * 2;
      float h = d2::hs(row * 67 + i * 3.7f);
      uint32_t bc = mix(0xa65a44, 0x8a4a3a, h);
      if (h > 0.85f)
        bc = 0xb87a5a;
      R(bx + 0.15f, by + 0.15f, 3.7f, 1.7f, bc);
      R(bx + 0.15f, by + 0.15f, 3.7f, 0.3f, shade(bc, 1.15f));
    }
  R(0, 270.4f, WW, 2.4f, 0xbdb8ae);
  R(0, 272.6f, WW, 0.4f, 0x8a867e);
  // 木の植えます
  for (int i = 0; i < NTREE; i++)
  {
    float x = TREE_X[i];
    R(x - 6, 285, 12, 4, 0x2a2622);
    for (float k = 0.6f; k < 12; k += 1.5f)
      R(x - 6 + k, 285, 0.5f, 4, 0x5a554e);
    R(x - 6, 285, 12, 0.5f, 0x6a655e);
    R(x - 1.1f, 266, 2.2f, 21, 0x5b4330);
    R(x - 1.1f, 266, 0.7f, 21, 0x7a5a40);
    L(x, 272, x + 3, 268, 0.6f, 0x5b4330);
    L(x, 274, x - 3, 270, 0.5f, 0x5b4330);
  }
  // ベンチ
  for (float lx : {63.0f, 84.0f})
  {
    R(lx, 289, 1, 6, IRON);
    R(lx - 0.6f, 294.4f, 2.2f, 0.6f, IRON);
    L(lx + 0.5f, 289, lx + 0.5f, 283.5f, 0.5f, IRON);
  }
  for (float sy : {283.6f, 285.4f})
    R(62, sy, 24, 1.1f, 0x9a6a44);
  for (float sy : {288.4f, 289.8f})
    R(61.6f, sy, 24.8f, 1.1f, 0xa87a50);
  R(61.6f, 291, 24.8f, 0.8f, 0x000000, 60);
  // 赤い郵便ポスト
  R(147.6f, 281, 5.8f, 12.6f, 0xc0302a);
  C(150.5f, 281, 2.9f, 0xc0302a);
  R(147.6f, 281, 1.2f, 12.6f, 0xe06050);
  R(148.6f, 284.4f, 3.8f, 0.6f, 0x2a0a08);
  R(148.6f, 287, 3.8f, 2.2f, 0xe8e0d0);
  R(147, 293.4f, 7, 1.4f, 0x2a2a2a);
  // 止めてある自転車
  uint32_t bk = 0x2a7a8a;
  ring(171, 293.5f, 3, 0.45f, 0x1a1a1a);
  ring(183, 293.5f, 3, 0.45f, 0x1a1a1a);
  ring(171, 293.5f, 2.4f, 0.15f, 0x9a9a9a);
  ring(183, 293.5f, 2.4f, 0.15f, 0x9a9a9a);
  L(171, 293.5f, 176, 293.5f, 0.45f, bk);
  L(176, 293.5f, 175, 288.6f, 0.45f, bk);
  L(175, 288.6f, 181, 288.6f, 0.45f, bk);
  L(181, 288.6f, 176, 293.5f, 0.45f, bk);
  L(181, 288.6f, 183, 293.5f, 0.45f, bk);
  L(171, 293.5f, 175, 288.6f, 0.35f, bk);
  L(181, 288.6f, 180.4f, 286.6f, 0.4f, 0x3a3a3a);
  L(179.4f, 286.6f, 182, 286.4f, 0.4f, 0x3a3a3a);
  E(175, 287.8f, 1.3f, 0.45f, 0x1a1a1a);
  for (float lx : {60.0f, 150.0f})
    lampPost(lx, 221.5f, 38);
}

static void flags(float wind, int t)
{
  drawFlag(24, 8.4f, 13, wind, t, 0xf4f0e8, 0xc8382f, 0);
  // 玄関の上の 2 本の旗 (壁から斜めに出た竿)
  for (int i = 0; i < 2; i++)
  {
    float bx = i ? 222 : 174, ex = bx + (i ? 6 : -6);
    L(bx, 162, ex, 152, 0.4f, BRASS);
    C(ex, 152, 0.45f, 0xd8b048);
    drawFlag(ex, 152.4f, 8, wind, t + i * 7, 0x2c3e6b, 0xc9a24a, i * 2.1f);
  }
}

static void draw(int r0, int r1)
{
  if (r0 < 48 * Z)
  {
    sky();
    skyline(40);
  }
  hotel();
  if (r1 > 196 * Z)
    street();
}

static void dyn(int layer, float wind, int t)
{
  if (layer == 0)
    flags(wind, t);
}

static Box WINS[COLS * ROWS];
static uint8_t LIT[COLS * ROWS];
static bool initWins = []() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
    {
      WINS[r * COLS + c] = {winX(c), winY(r), WIN_W, WIN_H};
      LIT[r * COLS + c] = winLit(r, c);
    }
  return true;
}();

extern const Scene HOTEL_SCENE = {
    "HOTEL MIRADOR ST.", draw, dyn,
    2, {{-8, 208, 256, 11}, {-8, 276, 256, 21}, {}},
    2, {228, 250}, {-1, 1}, {22, 26}, {0x0f, 0x0f}, {-1, -1},
    COLS * ROWS, WINS, LIT,
    2, {{60, 219, 22}, {150, 219, 22}},
    3, {{10, 198, 80, 14}, {98, 198, 60, 14}, {184, 198, 28, 14}},
    3, {{26, 262, 1}, {118, 262, 1}, {210, 262, 1}},
    {{128, 289, 40, 8}, {28, 210, 40, 8}}, false,
    198, 330, 0.9f, 196, 0.55f, false};
} // namespace w3
