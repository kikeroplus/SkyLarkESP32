// 3 本目 (LONGSHOT) のシーン 3: 港の倉庫街。海と灯台、コンテナと岸壁のクレーン、れんがの倉庫、石畳の岸壁、木の桟橋
#include "g3world.h"
#include <math.h>

namespace w3
{
static const uint32_t IRON = 0x2a2a2e, BRICK = 0x9a4a36, BRICK_D = 0x6e3224, WOOD = 0x8a6a48;
static const float WINX[10] = {12, 34, 56, 78, 100, 134, 156, 178, 200, 222};
static const float HORIZON = 72;

static void seaAndSky()
{
  sky(HORIZON);
  // 遠くの山並み
  for (float x = 0; x < WW; x += 0.5f)
  {
    float h = 6 + sinf(x * 0.03f) * 3 + sinf(x * 0.11f + 1) * 1.5f;
    R(x, HORIZON - h, 0.5f, h, tod == DAY ? (0x8aa0b4 | EMIT) : tod == DUSK ? (0x6a4a68 | EMIT) : (0x101828 | EMIT));
  }
  // 海 (遠くは空の色、近くは深い色。光の筋)
  for (int i = 0; i < 46 * 4; i++)
  {
    float y = HORIZON + i / 4.0f, k = i / (46.0f * 4);
    uint32_t c = tod == DAY ? mix(0x8ab8d0, 0x2a5a78, k) : tod == DUSK ? mix(0xd89070, 0x3a3a5a, k) : mix(0x1a2438, 0x080c18, k);
    R(0, y, WW, 0.25f, c | EMIT);
  }
  for (int i = 0; i < 90; i++)
  {
    float y = HORIZON + 1 + hh(i, 1) * 44, w = 1 + hh(i, 2) * 5 * (y - HORIZON) / 44;
    float x = hh(i, 3) * 240;
    uint32_t c = tod == DAY ? 0xe8f4fa : tod == DUSK ? 0xffd0a0 : 0x8090b0;
    if (tod == DUSK && fabsf(x - 150) < 20)
      w *= 2;
    R(x, y, w, 0.3f, c | EMIT, 120);
  }
  // 灯台と防波堤
  R(170, HORIZON + 4, 70, 2.5f, 0x9a9890);
  R(170, HORIZON + 6.5f, 70, 1, 0x5a5a58);
  for (int i = 0; i < 6; i++)
    R(212.5f - i * 0.2f, HORIZON + 4 - (i + 1) * 4.4f, 7 + i * 0.4f, 4.4f, i % 2 ? 0xc8382f : 0xf4f0e8);
  R(211.4f, HORIZON - 26.4f, 9.2f, 1.2f, IRON);
  R(212.6f, HORIZON - 30, 6.8f, 3.6f, tod == DAY ? 0x9ab8c8 : (0xfff2b0 | EMIT));
  T(211.4f, HORIZON - 30, 220.6f, HORIZON - 30, 216, HORIZON - 33.4f, 0xc8382f);
}

static void quayFar()
{
  // 向こう岸の岸壁
  R(0, 110, WW, 10, 0xa8a49a);
  R(0, 110, WW, 1, 0xc8c4ba);
  R(0, 119, WW, 2, 0x6a665e);
  // コンテナの山 (段違いに積む)
  static const uint32_t CC[6] = {0xc8382f, 0x2a5a9a, 0xd88a2a, 0x3a7a4a, 0x8a8a8a, 0x6a3a6a};
  for (int col = 0; col < 7; col++)
  {
    int hgt = 1 + (int)(hh(col, 5) * 3);
    for (int r = 0; r < hgt; r++)
    {
      float x = 78 + col * 14, y = 110 - (r + 1) * 6.2f, w = 13.4f;
      uint32_t c = CC[(int)(hh(col, r) * 6)];
      R(x, y, w, 6, c);
      for (float k = 0.8f; k < w; k += 0.9f)
        R(x + k, y + 0.4f, 0.3f, 5.2f, shade(c, 0.78f));
      R(x, y, w, 0.5f, shade(c, 1.2f));
      R(x, y, 0.6f, 6, shade(c, 0.65f));
      R(x + w - 0.6f, y, 0.6f, 6, shade(c, 0.65f));
    }
  }
  // 岸壁のクレーン (赤と白の鉄骨)
  uint32_t cr = 0xc8382f, cw = 0xf0ece4;
  for (float lx : {16.0f, 52.0f})
  {
    L(lx, 30, lx - 4, 112, 1.3f, cr);
    L(lx + 6, 30, lx + 10, 112, 1.3f, cr);
    for (float y = 40; y < 110; y += 12)
      L(lx - (y - 30) * 0.05f, y, lx + 6 + (y - 30) * 0.05f, y + 12, 0.5f, cw);
  }
  R(8, 26, 70, 5, cr);
  R(8, 26, 70, 1, cw);
  for (float k = 10; k < 76; k += 5)
    L(k, 26, k + 5, 31, 0.3f, cw);
  L(78, 28, 104, 34, 1.2f, cr); // 海へ張り出す腕
  L(78, 26, 104, 33, 0.4f, cw);
  R(26, 16, 18, 10, 0xe8e4dc); // 運転室
  R(28, 18, 6, 4, tod == DAY ? 0x7a98aa : (0xf0c870 | EMIT));
  L(96, 33, 96, 60, 0.25f, IRON); // ワイヤーとつり具
  R(93, 60, 6, 2, 0xe0b030);
  if (tod != DAY)
    for (float lx : {12.0f, 62.0f})
      C(lx, 26, 0.8f, 0xff3020 | EMIT);
}

static void warehouse(float x0, float w, const char *name, uint32_t trim)
{
  bool night = tod != DAY;
  float top = 132, ridge = 118, bot = 206;
  // 切妻の屋根
  T(x0 - 2, top, x0 + w + 2, top, x0 + w / 2, ridge, 0x5a5048);
  T(x0 + 2, top - 0.5f, x0 + w - 2, top - 0.5f, x0 + w / 2, ridge + 1.5f, 0x6e6258);
  for (float k = 4; k < w; k += 4)
    L(x0 + w / 2, ridge + 1, x0 + k, top, 0.15f, 0x4a4038, 160);
  // 壁 (れんがの積み)
  tex(x0, top, w, bot - top, BRICK, 0.13f, 6, x0);
  for (float y = top + 0.6f; y < bot; y += 1.6f)
  {
    R(x0, y, w, 0.2f, BRICK_D, 120);
    float off = ((int)(y / 1.6f) % 2) * 1.6f;
    for (float x = x0 + off; x < x0 + w; x += 3.2f)
      R(x, y, 0.2f, 1.6f, BRICK_D, 70);
  }
  R(x0, top, w, 1.2f, 0x000000, 60);
  R(x0, top - 1, w, 1, trim);
  // 妻の丸窓と、荷を上げる梁
  C(x0 + w / 2, 126, 2.8f, 0xd8ccb0);
  C(x0 + w / 2, 126, 2.2f, night ? (0xf0c060 | EMIT) : 0x3a4656);
  L(x0 + w / 2 - 2.2f, 126, x0 + w / 2 + 2.2f, 126, 0.3f, 0xd8ccb0);
  // 名前
  sign(x0 + w / 2 - signW(name, 0.7f) / 2, 165, 0.7f, name, 0xf0ece0, 230);
  // 上の窓 (上が丸い、緑の鉄の枠)
  for (int i = 0; i < 5; i++)
  {
    float wx = WINX[(x0 > 100 ? 5 : 0) + i];
    bool lit = night && hh(wx, 9) < 0.45f;
    C(wx + 4, 144, 4.6f, 0xd8ccb0);
    R(wx - 0.6f, 144, 9.2f, 12.6f, 0xd8ccb0);
    C(wx + 4, 144, 4, 0x2a3a30);
    R(wx, 144, 8, 12, 0x2a3a30);
    for (int k = 0; k < 44; k++)
    {
      float yy = 140.6f + k * 0.25f, half = yy < 144 ? sqrtf(max(0.0f, 3.4f * 3.4f - (144 - yy) * (144 - yy))) : 3.4f;
      R(wx + 4 - half, yy, half * 2, 0.25f, lit ? (0xf2c46c | EMIT) : night ? (0x1e2632 | EMIT) : mix(0x9ab4c4, 0x3e5464, (yy - 140.6f) / 15));
    }
    L(wx + 4, 140.6f, wx + 4, 155.6f, 0.3f, 0x2a3a30);
    R(wx + 0.6f, 148, 6.8f, 0.3f, 0x2a3a30);
    R(wx - 1, 156.6f, 10, 1, 0xd8ccb0);
  }
  // 大きな荷の扉 (片方は開いていて中が見える)
  for (int k = 0; k < 2; k++)
  {
    float dx = x0 + 10 + k * (w - 40), dw = 20;
    R(dx - 1.5f, 174, dw + 3, 32, 0xd8ccb0);
    if (k == 0) // 開いている: 暗い中に木箱と吊り電灯
    {
      R(dx, 176, dw, 30, night ? (0x3a2a1a | EMIT) : 0x1e1a16);
      R(dx + 2, 198, 6, 8, 0x6a5038);
      R(dx + 9, 200, 5, 6, 0x5a4430);
      R(dx + 3, 194, 4, 4, 0x7a5a40);
      L(dx + 10, 176, dx + 10, 182, 0.2f, 0x1a1a1a);
      C(dx + 10, 182.6f, 1, night ? (0xffe0a0 | EMIT) : 0x4a4038);
      if (night)
        T(dx + 10, 183, dx + 2, 206, dx + 18, 206, 0xffd890 | EMIT, 50);
      // 開いた扉の板
      R(dx + dw + 1.5f, 176, 3, 30, 0x4a6a5a);
    }
    else // 閉まっている引き戸: 板と斜めの補強
    {
      R(dx, 176, dw, 30, 0x4a6a5a);
      for (float b = 0; b < dw; b += 2.5f)
        R(dx + b, 176, 0.3f, 30, 0x3a5a4a);
      L(dx, 176, dx + dw, 206, 0.6f, 0x3a5a4a);
      L(dx + dw, 176, dx, 206, 0.6f, 0x3a5a4a);
      R(dx, 175, dw, 1, IRON);
    }
  }
  // 壁の灯り
  for (float lx : {x0 + w / 2 - 6, x0 + w / 2 + 6})
  {
    L(lx, 172, lx, 169, 0.3f, IRON);
    T(lx - 1.4f, 172, lx + 1.4f, 172, lx, 170.6f, IRON);
    C(lx, 172.4f, 0.7f, night ? (0xffe8b0 | EMIT) : 0xd8d0b0);
    if (night)
      C(lx, 172.4f, 3, 0xffd890 | EMIT, 40);
  }
  R(x0 - 0.5f, top, 1, bot - top, 0x000000, 50);
}

static void quayNear()
{
  bool on = tod != DAY;
  // 石畳の岸壁
  tex(0, 206, WW, 20, 0x8a8478, 0.10f, 8, 61, 0.06f);
  for (int row = 0; row < 10; row++)
    for (int i = -1; i < 61; i++)
    {
      float bx = i * 4 + (row % 2) * 2, by = 206 + row * 2;
      R(bx, by, 4, 0.25f, 0x5a564e, 150);
      R(bx, by, 0.25f, 2, 0x5a564e, 130);
      R(bx + 0.4f, by + 0.3f, 3.2f, 0.4f, 0xa8a296, 90);
    }
  R(0, 206, WW, 1.2f, 0x000000, 70);
  if (on)
    for (float lx : {60.0f, 180.0f})
      E(lx, 222, 20, 4, 0xffd890 | EMIT, 60);
  // 線路 (枕木と 2 本のレール)
  R(0, 226, WW, 9, 0x5a554e);
  for (float x = 0; x < WW; x += 3)
    R(x, 226.6f, 1.6f, 7.8f, 0x4a3a2a);
  for (float ry : {227.6f, 232.4f})
  {
    R(0, ry, WW, 0.8f, 0x8a8e94);
    R(0, ry, WW, 0.3f, 0xc8ccd0);
  }
  // 車道 (コンクリート)
  tex(0, 235, WW, 18, 0x6a6862, 0.06f, 4, 63, 0.14f);
  for (float x = 0; x < WW; x += 24)
    R(x, 235, 0.3f, 18, 0x4a4842, 140);
  R(0, 252.6f, WW, 1.4f, 0xd8c040); // 黄色の線
  // 木の桟橋 (横の板)
  for (int i = 0; i < 24; i++)
  {
    float y = 254 + i * 2;
    uint32_t c = mix(WOOD, 0x6a4e34, hh(i, 70));
    R(0, y, WW, 1.8f, c);
    R(0, y + 1.8f, WW, 0.2f, 0x3a2a1c);
    for (float x = hh(i, 71) * 30; x < WW; x += 30 + hh(i, x) * 20)
      R(x, y, 0.25f, 1.8f, 0x3a2a1c);
  }
  // 岸壁の道具: 係船柱、ロープ、木箱、たる、浮き輪
  for (float bx : {14.0f, 94.0f, 160.0f, 226.0f})
  {
    R(bx - 1.6f, 219, 3.2f, 4, 0x1e1e20);
    E(bx, 219, 2.2f, 0.9f, 0x2a2a2c);
    E(bx - 0.5f, 218.8f, 1, 0.4f, 0x4a4a4e);
  }
  for (int k = 0; k < 3; k++)
    ring(102, 222.4f, 1.2f + k * 0.6f, 0.4f, 0xc8b080);
  for (int k = 0; k < 3; k++) // 木箱の山
  {
    float cx = 128 + (k % 2) * 6.5f, cy = 216 - (k / 2) * 6;
    R(cx, cy, 6, 6, 0xa07850);
    R(cx, cy, 6, 0.6f, 0xc09a70);
    L(cx, cy, cx + 6, cy + 6, 0.3f, 0x6a4a30);
    R(cx, cy, 0.5f, 6, 0x6a4a30);
    R(cx + 5.5f, cy, 0.5f, 6, 0x6a4a30);
  }
  for (float bx : {196.0f, 201.0f}) // たる
  {
    R(bx - 2, 215, 4, 7, 0x2a5a8a);
    for (float by : {216.0f, 220.0f})
      R(bx - 2.1f, by, 4.2f, 0.5f, 0x4a4a4e);
    E(bx, 215, 2, 0.6f, 0x3a6a9a);
  }
  L(60, 222, 60, 213, 0.4f, IRON); // 浮き輪の柱
  ring(60, 214, 1.7f, 0.9f, 0xe85a2a);
  ring(60, 214, 1.7f, 0.25f, 0xf4f0e8);
  // 桟橋の杭 (カモメが止まる)
  for (float px : {30.0f, 150.0f, 210.0f})
  {
    R(px - 1.6f, 282, 3.2f, 14, 0x5a4430);
    E(px, 282, 1.6f, 0.6f, 0x7a6048);
  }
  for (float lx : {60.0f, 180.0f})
    lampPost(lx, 224, 34);
  if (on)
    E(120, 290, 30, 5, 0xffd890 | EMIT, 40);
  // 倉庫の屋根の風見の竿
  L(60, 112, 60, 120, 0.4f, IRON);
  L(180, 112, 180, 120, 0.4f, IRON);
}

static void draw(int r0, int r1)
{
  if (r0 < 120 * Z)
  {
    seaAndSky();
    quayFar();
  }
  warehouse(2, 116, "WAREHOUSE 3", 0xd8ccb0);
  warehouse(122, 116, "PORT CO.", 0xd8ccb0);
  if (r1 > 204 * Z)
    quayNear();
}

// 沖を行く貨物船、カモメ、吹き流し
static void dyn(int layer, float wind, int t)
{
  if (layer == 0)
  {
    float sx = 250 - fmodf(t * 0.06f, 200);
    float y = HORIZON + 14;
    if (sx < 238)
    {
      Q(sx, y, sx + 44, y, sx + 41, y + 5, sx + 3, y + 5, 0x2a2e38);
      R(sx + 1, y + 3.6f, 42, 1.2f, 0x8a2a24);
      static const uint32_t CC[4] = {0xc8382f, 0x2a5a9a, 0xd88a2a, 0x3a7a4a};
      for (int i = 0; i < 6; i++)
        R(sx + 4 + i * 4.6f, y - 3 - (i % 2) * 2.2f, 4.4f, 3 + (i % 2) * 2.2f, CC[i % 4]);
      R(sx + 34, y - 7, 7, 7, 0xe8e4dc);
      R(sx + 35, y - 6, 5, 1, tod == DAY ? 0x5a7080 : (0xf0d070 | EMIT));
      R(sx + 36.4f, y - 10, 1.6f, 3, 0x2a2a2a);
      if (tod != DAY)
        C(sx + 44, y - 1, 0.6f, 0x40ff60 | EMIT);
    }
    // 吹き流し (倉庫の屋根)
    for (int i = 0; i < 2; i++)
      drawFlag(i ? 180.0f : 60.0f, 112.4f, 9, wind, t + i * 4, 0xe85a2a, 0xf4f0e8, i * 1.3f);
  }
  else if (layer == 2) // 空を回るカモメ
  {
    for (int i = 0; i < 4; i++)
    {
      float a = t * 0.012f + i * 1.7f;
      float x = 40 + i * 50 + cosf(a) * 30 + wind * 2, y = 40 + i * 9 + sinf(a * 1.3f) * 10;
      drawPigeon(x, y, cosf(a) < 0 ? 1 : -1, true, t * 0.3f + i, true);
    }
  }
}

static Box WINS[10];
static uint8_t LIT[10];
static bool initWins = []() {
  for (int i = 0; i < 10; i++)
  {
    WINS[i] = {WINX[i] + 0.6f, 146, 6.8f, 9.6f};
    LIT[i] = hh(WINX[i], 9) < 0.45f;
  }
  return true;
}();

extern const Scene HARBOR_SCENE = {
    "PIER 7 WAREHOUSES", draw, dyn,
    2, {{-8, 210, 256, 13}, {-8, 262, 256, 30}, {}},
    1, {237, 0}, {1, 1}, {24, 0}, {(1 << CAR_TRUCK) | (1 << CAR_VAN), 0}, {-1, -1},
    10, WINS, LIT,
    3, {{60, 222, 24}, {180, 222, 24}, {120, 290, 26}},
    1, {{12, 206, 20, 10}},
    0, {},
    {{20, 268, 200, 22}, {0, 0, 0, 0}}, true,
    206, 450, 1.1f, 206, 0.9f, false};
} // namespace w3
