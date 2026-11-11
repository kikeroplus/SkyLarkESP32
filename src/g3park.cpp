// 3 本目 (LONGSHOT) のシーン 5: 公園と広場。木立と白い音楽堂、芝生と花壇、真ん中の噴水、自転車道、ベンチとアイスの屋台
#include "g3world.h"
#include <math.h>

namespace w3
{
static const uint32_t IRON = 0x2a2a2e, GRASS = 0x5a8a44, GRASS_D = 0x46703a;
static const float FX = 120, FY = 196; // 噴水の池の真ん中

static void treeMass(float x, float y, float r, float seed)
{
  uint32_t d0 = 0x2a4a26, d1 = 0x3a6232, d2c = 0x4e7a3e;
  C(x, y + r * 0.3f, r, d0);
  for (int k = 0; k < 6; k++)
  {
    float a = hh(seed, k) * 6.28f, rr = r * 0.55f;
    C(x + cosf(a) * rr, y + sinf(a) * rr * 0.7f - r * 0.1f, r * 0.6f, d1);
  }
  for (int k = 0; k < 5; k++)
  {
    float a = 3.4f + hh(seed, k + 9) * 1.6f;
    C(x + cosf(a) * r * 0.4f, y + sinf(a) * r * 0.5f, r * 0.35f, d2c);
  }
  for (int k = 0; k < 14; k++)
    C(x + (hh(seed, k + 20) - 0.5f) * r * 1.6f, y + (hh(seed, k + 40) - 0.6f) * r * 1.2f, r * 0.08f, 0x6a9a50);
}

static void back()
{
  sky();
  skyline(60);
  for (int i = 0; i < 40 * 4; i++) // 木立の下の暗がり (木の間から空が見えないように)
    R(0, 52 + i / 4.0f, WW, 0.25f, mix(0x8aa0b4, 0x22381e, min(1.0f, i / 40.0f)));
  // 木立 (奥から手前へ)
  for (int i = 0; i < 14; i++)
    treeMass(i * 18 + hh(i, 1) * 8, 54 + hh(i, 2) * 8, 13 + hh(i, 3) * 5, i);
  for (int i = 0; i < 10; i++)
    treeMass(i * 26 + 6 + hh(i, 4) * 10, 74 + hh(i, 5) * 8, 12 + hh(i, 6) * 4, i + 30);
  // 芝生
  tex(0, 92, WW, 58, GRASS, 0.12f, 6, 101, 0.1f);
  for (int i = 0; i < 400; i++) // 草の筋
  {
    float x = hh(i, 7) * 240, y = 93 + hh(i, 8) * 56;
    L(x, y, x + 0.3f, y - 0.8f, 0.12f, hh(i, 9) < 0.5f ? 0x76a85a : GRASS_D, 160);
  }
  R(0, 92, WW, 2, 0x000000, 50);
  // 白い音楽堂 (八角の屋根と柱)
  float gx = 180, gy = 112;
  E(gx, gy + 4, 26, 3, 0x000000, 50);
  R(gx - 22, gy - 2, 44, 5, 0xe8e4dc);
  R(gx - 22, gy - 2, 44, 1, 0xf8f6f0);
  R(gx - 20, gy - 24, 40, 22, 0x2a3a2a, 120); // 中の陰
  for (int k = 0; k < 6; k++)
  {
    float cx = gx - 19 + k * 7.6f;
    R(cx - 0.8f, gy - 24, 1.6f, 22, 0xf4f0e8);
    R(cx + 0.4f, gy - 24, 0.4f, 22, 0xc8c4bc);
  }
  for (float k = gx - 19; k < gx + 19; k += 1.6f) // 手すり
    L(k, gy - 6, k, gy - 2, 0.15f, 0xf4f0e8);
  R(gx - 20, gy - 6.4f, 40, 0.6f, 0xf4f0e8);
  Q(gx - 25, gy - 24, gx + 25, gy - 24, gx + 16, gy - 34, gx - 16, gy - 34, 0x6a8a9a);
  Q(gx - 16, gy - 34, gx + 16, gy - 34, gx + 6, gy - 40, gx - 6, gy - 40, 0x5a7a8a);
  for (float k = -24; k <= 24; k += 4)
    L(gx + k, gy - 24, gx + k * 0.6f, gy - 34, 0.2f, 0x4a6a7a);
  R(gx - 26, gy - 25, 52, 1.6f, 0xf4f0e8);
  L(gx, gy - 40, gx, gy - 48, 0.4f, IRON);
  C(gx, gy - 40.4f, 1, 0xd8b048);
  // 花壇 (丸く、色ごとの輪)
  float bx = 60, by = 126;
  E(bx, by, 30, 7, 0x6a4a30);
  static const uint32_t FL[4] = {0xe84a5a, 0xf0d040, 0xf4f0ec, 0xb070d0};
  for (int ring_ = 0; ring_ < 4; ring_++)
    for (int i = 0; i < 40 - ring_ * 8; i++)
    {
      float a = i * 6.283f / (40 - ring_ * 8), r = 27 - ring_ * 6;
      C(bx + cosf(a) * r, by + sinf(a) * r * 0.22f, 1.1f, FL[ring_]);
      C(bx + cosf(a) * r + 0.3f, by + sinf(a) * r * 0.22f - 0.5f, 0.4f, 0xffffff, 120);
    }
  C(bx, by - 1, 2, 0x3a6a34);
  // 公園の名前の札
  R(10, 140, 26, 7, 0x4a3a2a);
  sign(12, 141.4f, 0.5f, "PARK", 0xf0ecd8);
  R(14, 147, 1, 4, 0x4a3a2a);
  R(31, 147, 1, 4, 0x4a3a2a);
}

static void front()
{
  bool on = tod != DAY;
  // 砂利の小道
  tex(0, 150, WW, 16, 0xc8b898, 0.12f, 3, 111, 0.15f);
  R(0, 150, WW, 0.8f, 0x8a7a5a);
  R(0, 165.4f, WW, 0.8f, 0x8a7a5a);
  // 石の広場 (噴水のまわりの丸い模様)
  tex(0, 166, WW, 74, 0xc4bfb4, 0.06f, 24, 113, 0.05f);
  for (int k = 1; k <= 6; k++)
  {
    float r = 44 + k * 14;
    for (int i = 0; i < 80; i++)
    {
      float a0 = i * 6.283f / 80, a1 = (i + 1) * 6.283f / 80;
      L(FX + cosf(a0) * r, FY + sinf(a0) * r * 0.3f, FX + cosf(a1) * r, FY + sinf(a1) * r * 0.3f, 0.2f, 0x9a958a, 150);
    }
  }
  for (int i = 0; i < 24; i++)
  {
    float a = i * 6.283f / 24;
    L(FX + cosf(a) * 44, FY + sinf(a) * 13, FX + cosf(a) * 128, FY + sinf(a) * 38, 0.2f, 0x9a958a, 120);
  }
  // 噴水の池
  E(FX, FY + 1.5f, 44, 12, 0x000000, 50);
  E(FX, FY, 43, 12, 0xd8d0c0);
  E(FX, FY - 0.6f, 40, 10.6f, 0xb0a890);
  for (int i = 0; i < 40; i++)
  {
    float k = i / 40.0f;
    E(FX, FY - 0.4f + k * 0.8f, 38 - k * 2, 9.8f - k * 3, mix(0x6a9ab0, 0x3a6a88, k) | (on ? 0 : 0), 40);
  }
  E(FX, FY, 38, 9.6f, tod == DAY ? 0x5a92ac : 0x3a6a88);
  for (int i = 0; i < 30; i++) // 水面のきらめき
    R(FX - 34 + hh(i, 120) * 68, FY - 7 + hh(i, 121) * 14, 1 + hh(i, 122) * 2, 0.25f, 0xe8f4fa, 150);
  // 真ん中の台と 2 段の皿
  R(FX - 3, FY - 14, 6, 14, 0xd8d0c0);
  R(FX - 3, FY - 14, 1, 14, 0xf0ead8);
  E(FX, FY - 14, 12, 2.6f, 0xd8d0c0);
  E(FX, FY - 14.6f, 11, 2, 0x5a92ac);
  R(FX - 1.6f, FY - 24, 3.2f, 10, 0xd8d0c0);
  E(FX, FY - 24, 6.4f, 1.6f, 0xd8d0c0);
  E(FX, FY - 24.4f, 5.6f, 1.1f, 0x5a92ac);
  if (on)
    for (int k = 1; k <= 4; k++)
      E(FX, FY - 10, 20 + k * 6, 10 + k * 3, 0x8ac8ff | EMIT, 14);
  if (on)
    for (float lx : {60.0f, 180.0f})
      E(lx, 232, 20, 5, 0xffd890 | EMIT, tod == NIGHT ? 70 : 40);
  // 自転車道 (赤い舗装と白い自転車の印)
  R(0, 240, WW, 1.2f, 0xd0ccc4);
  tex(0, 241.2f, WW, 15, 0x9a5a4a, 0.07f, 4, 117, 0.12f);
  R(0, 256.2f, WW, 1.2f, 0xd0ccc4);
  for (float x = 30; x < WW; x += 90) // 自転車の印
  {
    ring(x, 250, 1.6f, 0.3f, 0xf0ece4, 220);
    ring(x + 5, 250, 1.6f, 0.3f, 0xf0ece4, 220);
    L(x, 250, x + 2.4f, 247.6f, 0.3f, 0xf0ece4, 220);
    L(x + 2.4f, 247.6f, x + 5, 250, 0.3f, 0xf0ece4, 220);
    L(x + 2.4f, 247.6f, x + 1.6f, 246.6f, 0.3f, 0xf0ece4, 220);
  }
  // 手前の芝生と小道
  tex(0, 257.4f, WW, 43, GRASS, 0.12f, 6, 119, 0.1f);
  for (int i = 0; i < 300; i++)
  {
    float x = hh(i, 130) * 240, y = 258 + hh(i, 131) * 42;
    L(x, y, x + 0.3f, y - 0.9f, 0.12f, hh(i, 132) < 0.5f ? 0x76a85a : GRASS_D, 170);
  }
  tex(0, 270, WW, 22, 0xc8b898, 0.12f, 3, 121, 0.15f);
  R(0, 270, WW, 0.8f, 0x8a7a5a);
  R(0, 291.4f, WW, 0.8f, 0x8a7a5a);
  // ベンチ (芝生の上、小道の奥)
  for (float bxp : {40.0f, 150.0f})
  {
    for (float lx : {bxp, bxp + 20})
      R(lx, 264, 0.9f, 5.4f, IRON);
    for (float sy : {259.6f, 261.2f})
      R(bxp - 1, sy, 22, 1, 0x9a6a44);
    for (float sy : {264.0f, 265.3f})
      R(bxp - 1.4f, sy, 22.8f, 1, 0xa87a50);
    R(bxp - 1.4f, 266.4f, 22.8f, 0.6f, 0x000000, 60);
  }
  // アイスの屋台 (しま模様のパラソル)
  float cx = 200;
  R(cx - 9, 280, 18, 10, 0xf4f0e8);
  R(cx - 9, 280, 18, 1.2f, 0x5ab0d0);
  sign(cx - 7.6f, 283, 0.42f, "GELATO", 0xd84a6a);
  C(cx - 6, 290.6f, 1.4f, 0x2a2a2a);
  C(cx + 6, 290.6f, 1.4f, 0x2a2a2a);
  L(cx, 280, cx, 266, 0.4f, 0xd0d0d0);
  for (int i = 0; i < 8; i++)
  {
    float u0 = -1 + i / 4.0f, u1 = -1 + (i + 1) / 4.0f;
    T(cx, 263.4f, cx + u0 * 12, 268 - (1 - u0 * u0) * 0.6f, cx + u1 * 12, 268 - (1 - u1 * u1) * 0.6f, i % 2 ? 0xf4f0e8 : 0xe85a6a);
  }
  for (int k = 0; k < 3; k++)
    C(cx - 4 + k * 4, 279, 1.3f, k == 0 ? 0xf8c8d8 : k == 1 ? 0xf8f0c0 : 0xb8e0b0);
  for (float lx : {60.0f, 180.0f})
    lampPost(lx, 239, 34);
  // 木の幹 (葉は動く)
  for (int i = 0; i < 4; i++)
  {
    static const float TX[4] = {12, 228, 30, 212}, TY0[4] = {258, 258, 170, 170}, TY1[4] = {276, 276, 186, 186};
    R(TX[i] - 1.2f, TY0[i], 2.4f, TY1[i] - TY0[i], 0x5b4330);
    R(TX[i] - 1.2f, TY0[i], 0.8f, TY1[i] - TY0[i], 0x7a5a40);
    E(TX[i], TY1[i], 4, 1, 0x000000, 60);
  }
}

static void draw(int r0, int r1)
{
  if (r0 < 152 * Z)
    back();
  if (r1 > 148 * Z)
    front();
}

// 噴水の水 (人の後ろの水は layer 1 で、手前の広場の人より奥)。音楽堂の旗
static void dyn(int layer, float wind, int t)
{
  if (layer == 0)
    drawFlag(180, 64.4f, 10, wind, t, 0xf4f0e8, 0x3a7a4a, 0.5f);
  if (layer != 1)
    return;
  bool on = tod != DAY;
  uint32_t wc = on ? (0xc8e8ff | EMIT) : 0xe8f4fa;
  // 上の皿から吹き上がる水と、こぼれる水
  float top = FY - 42 + sinf(t * 0.13f) * 1.5f;
  L(FX, FY - 25, FX, top, 0.9f, wc, 200);
  L(FX, FY - 25, FX, top, 0.4f, 0xffffff, 220);
  for (int s = -1; s <= 1; s += 2)
  {
    for (int i = 0; i < 10; i++) // 落ちる水の弧 (風で流れる)
    {
      float u = i / 10.0f, u1 = (i + 1) / 10.0f;
      float x0 = FX + s * u * 8 + wind * u * u * 1.2f, y0 = top + u * u * 18;
      float x1 = FX + s * u1 * 8 + wind * u1 * u1 * 1.2f, y1 = top + u1 * u1 * 18;
      L(x0, y0, x1, y1, 0.5f, wc, 170);
    }
    for (int i = 0; i < 8; i++) // 下の皿からのしずく
    {
      float ph = fmodf(t * 0.08f + i * 0.125f, 1.0f);
      float x = FX + s * (11 + ph * 3) + wind * ph * 0.6f, y = FY - 14 + ph * ph * 12;
      C(x, y, 0.35f, wc, 200);
    }
  }
  for (int i = 0; i < 18; i++) // しぶき
  {
    float ph = fmodf(t * 0.05f + hh(i, 140), 1.0f);
    float x = FX + (hh(i, 141) - 0.5f) * 24 + wind * ph * 2, y = FY - 16 - ph * 8 + ph * ph * 10;
    C(x, y, 0.3f, wc, (int)(160 * (1 - ph)));
  }
  for (int i = 0; i < 6; i++) // 池の波紋
  {
    float ph = fmodf(t * 0.03f + i / 6.0f, 1.0f);
    E(FX + (hh(i, 150) - 0.5f) * 20, FY - 3 + hh(i, 151) * 4, 2 + ph * 8, 0.6f + ph * 2, 0xe8f4fa, (int)(120 * (1 - ph)));
  }
}

extern const Scene PARK_SCENE = {
    "RIVERSIDE PARK", draw, dyn,
    3, {{-8, 152, 256, 12}, {-8, 216, 256, 20}, {-8, 272, 256, 18}},
    1, {240, 0}, {-1, 1}, {34, 0}, {1 << CAR_BIKE, 0}, {-1, -1},
    0, nullptr, nullptr,
    3, {{60, 232, 24}, {180, 232, 24}, {120, 196, 40}},
    0, {},
    4, {{12, 250, 1.1f}, {228, 250, 1.1f}, {30, 160, 0.9f}, {212, 160, 0.9f}},
    {{70, 226, 100, 8}, {90, 278, 60, 10}}, false,
    0, 280, 0.8f, 150, 0.5f, false};
} // namespace w3
