// 7 本目 (SMARTBALL): 古い店のスマートボール。打ち出しと壁は ORBIT (game5.cpp) と同じ作り、台は釘と穴
#include "game7.h"
#include "g2draw.h"
#include <algorithm>

static const int W = 240, H = 320;
static const int FPS = Game7::FPS;
static const int NB = Game7::NB;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
static const float PI_F = 3.14159265f;

// ---- 台 (液晶の座標)。上の 22 行は持ち玉など ----
static const int HUD_H = 22;
static const float BALL_R = 6.0f;  // ビー玉ほどの白い玉
static const float NR = 1.5f;      // 釘の太さ (半径)
static const float WR = 1.5f;
static const float RB = BALL_R + WR;
static const float GRAV = 300;     // ピンボールより台がねている
static const float VMAX = 900;
static const float XL = 6, XD = 216, XO = 232, YT = 30;
static const float ARC_R = 56, ARC_LX = 62, ARC_RX = 176, ARC_Y = 86;
static const float GATE_Y0 = 112, GATE_Y1 = 100;
static const float PLY = 300, PL_TRAVEL = 14;
static const float LANE_X = 224;
static const float TRAY_Y = 304; // ここから下は樋 (入らなかった玉が落ちる)
static const float OUT_Y = 318;
static const float E_NAIL = 0.45f;
// 左上の返しゴム (水平から 60 度ほど立てる): 強く打って上の丸い壁を回ってきた玉を、台の中へはね返す。両端は丸い壁にほぼ付けて、裏へ玉が入らないようにする
static const float RUB_X0 = 9.2f, RUB_Y0 = 81, RUB_X1 = 31.6f, RUB_Y1 = 42.6f, E_RUB = 0.8f;
static const float RUB_TILT = 22 * PI_F / 180; // 玉ごとに、ゴムを 0〜22 度でたらめに立てたように返す (同じ強さで打っても同じ道をたどらない)
// 左右の壁ぎわの釘: 壁に沿ってすんなり落ちる玉を、台の中へ寄せる。壁とのすき間は玉より狭いので、玉は内側へそれる
static const float SIDE_Y = 138, SIDE_LX = 11, SIDE_RX = 211;
// 妖怪: 上を左右にただよう提灯おばけと、左右の壁から顔を出すから傘おばけ・一つ目小僧。ときどき出てきて、玉が当たるとはじく
static const float YR = 7;             // 当たりの半径
static const float Y_TOP_Y = 45;       // 上の妖怪の高さ (上の壁と玉が通れないすき間)
static const float Y_SIDE_Y[2] = {192, 200}; // 左・右の妖怪の真ん中の高さ (上下にゆれる)
static const float SPIT_P = 0.33f; // 10 の穴に入った玉を、穴の妖怪が吐き出す割合
static const int HOP_N = 10;      // 吐いた玉が宙を跳んでいるフレーム (釘を飛び越し、穴にも入らない)
static const int SPIT_AT = 20, SPIT_END = 36; // 顔を出してから吐くまで、引っこむまでのフレーム
static const float E_YOKAI = 0.6f, KICK_YOKAI = 110; // はね返り。当たったら少なくともこの速さではじく (上に乗って止まらない)

// 穴と払い出しの数
struct Hole
{
  float x, y;
  int v;
};
static const Hole HOLES[] = {
    {52, 94, 1}, {111, 84, 1}, {170, 94, 1}, //
    {87, 152, 3}, {135, 152, 3},             //
    {111, 198, 10},                          //
    {40, 240, 1}, {182, 240, 1},             //
    {68, 280, 1}, {111, 278, 1}, {154, 280, 1},
};
static const int NH = sizeof(HOLES) / sizeof(HOLES[0]);
static const float RING_R = 12;  // 穴を囲む釘の輪
static const float RING_LOW = 11.7f;
static const float CAP_R = 5.5f; // 穴の中心からこれより近づくと入る (輪の底の釘に乗って止まった玉は 4.5 の所にいるので、それより広く)
static const int FALL_N = 10;

// 釘
static float NX[300], NY[300];
static int nN = 0;

static const uint32_t INK = 0x000000, CREAM = 0xE8DFC4, BRASS = 0xC8A860, RUST = 0xC8642A, MUTE = 0xA89C80, REDT = 0xE06048;

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

// ---- 壁 ----
struct Wall
{
  float x0, y0, x1, y1, e;
  bool gate;
};
static Wall walls[40];
static int nW = 0;
static void addW(float x0, float y0, float x1, float y1, float e, bool gate = false)
{
  if (nW < 40)
    walls[nW++] = {x0, y0, x1, y1, e, gate};
}
static void addArc(float cx, float cy, float r, float a0, float a1, int n, float e)
{
  for (int i = 0; i < n; i++)
  {
    float u0 = a0 + (a1 - a0) * i / n, u1 = a0 + (a1 - a0) * (i + 1) / n;
    addW(cx + r * cosf(u0), cy + r * sinf(u0), cx + r * cosf(u1), cy + r * sinf(u1), e);
  }
}

// 0 = 台の面、1 = 外の枠、2 = 打ち出すレーン、3 = 下の樋
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
  return y >= TRAY_Y ? 3 : 0;
}

static void buildWalls()
{
  nW = 0;
  addW(XL, ARC_Y, XL, 330, 0.4f);
  addArc(ARC_LX, ARC_Y, ARC_R, PI_F, 1.5f * PI_F, 10, 0.4f);
  addW(ARC_LX, YT, ARC_RX, YT, 0.4f);
  addArc(ARC_RX, ARC_Y, ARC_R, 1.5f * PI_F, 2 * PI_F, 10, 0.4f);
  addW(XO, ARC_Y, XO, 330, 0.3f);
  addW(XD, GATE_Y0, XD, 330, 0.3f);
  addW(XD, GATE_Y0, XO, GATE_Y1, 0.3f, true);
  addW(RUB_X0, RUB_Y0, RUB_X1, RUB_Y1, E_RUB); // 返しゴム (いちばん最後に足す)
}

static void addNail(float x, float y)
{
  if (nN < 300)
  {
    NX[nN] = x;
    NY[nN] = y;
    nN++;
  }
}

static float segDist(float px, float py, float x0, float y0, float x1, float y1)
{
  float dx = x1 - x0, dy = y1 - y0;
  float u = clampf(((px - x0) * dx + (py - y0) * dy) / (dx * dx + dy * dy), 0, 1);
  return hypotf(px - x0 - dx * u, py - y0 - dy * u);
}

static void buildNails()
{
  nN = 0;
  // 穴を囲む輪: 上を開け、横と下を 45 度おきの 7 本で囲む
  for (int h = 0; h < NH; h++)
    for (int k = 0; k < 7; k++)
    {
      float a = (-48 + k * 46) * PI_F / 180; // 開きは 84 度 (玉がやっと通る)
      float r = HOLES[h].y > 230 && HOLES[h].v == 1 ? RING_LOW : RING_R; // 下の段の 1 は口を狭くして入りにくく
      addNail(HOLES[h].x + r * cosf(a), HOLES[h].y + r * sinf(a));
    }
  // 10 の穴は、上に 2 本足して入りにくくする
  addNail(103, 176);
  addNail(119, 176);
  addNail(111, 161); // 真上から落ちてきた玉は、これではじく
  addNail(SIDE_LX, SIDE_Y);
  addNail(SIDE_RX, SIDE_Y);
  // ほかの釘: 互いちがいの並び。穴のまわりと壁ぎわはあける
  for (int row = 0;; row++)
  {
    float y = 66 + row * 26;
    if (y > 296)
      break;
    for (float x = 25 + (row % 2) * 15; x <= 194; x += 30) // 壁とのすき間は玉より広く (はさまらない)
    {
      bool ok = region((int)x, (int)y) == 0 && region((int)x - 10, (int)y - 10) == 0 && region((int)x + 10, (int)y - 10) == 0;
      for (int h = 0; h < NH && ok; h++)
        if (hypotf(x - HOLES[h].x, y - HOLES[h].y) < 22)
          ok = false;
      for (int i = 0; i < nW && ok; i++) // 壁とのすき間が玉より狭いと、玉がはさまって止まる
        if (segDist(x, y, walls[i].x0, walls[i].y0, walls[i].x1, walls[i].y1) < 17)
          ok = false;
      if (ok && (hypotf(x - SIDE_LX, y - SIDE_Y) < 17 || hypotf(x - SIDE_RX, y - SIDE_Y) < 17))
        ok = false;
      if (ok && hypotf(x - 111, y - 177) < 14)
        ok = false;
      if (ok)
        addNail(x, y);
    }
  }
}

float Game7::rnd()
{
  seed = seed * 1664525u + 1013904223u;
  return (seed >> 8) * (1.0f / 16777216.0f);
}

void Game7::sfx(Sfx s, float vol)
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

// はげかけた塗りの文字 (液晶の 6x8 の文字を size 倍)。ところどころ塗りが欠ける
static void wornText(float x, float y, const char *s, uint32_t c, int size, int a = 220, float wear = 0.7f)
{
  int n = strlen(s);
  x -= n * 6 * size / 2.0f;
  for (int i = 0; i < n; i++)
  {
    const uint8_t *g = fontGlyph((uint8_t)s[i]);
    if (!g)
      continue;
    for (int gx = 0; gx < 5; gx++)
      for (int row = 0; row < 8; row++)
        if (g[gx] >> row & 1)
          for (int py = 0; py < size; py++)
            for (int px = 0; px < size; px++)
            {
              float X = x + i * 6 * size + gx * size + px, Y = y + row * size + py;
              float wv = d2::vn(X * 0.35f, Y * 0.35f);
              if (wv > wear)
                continue; // はげた所
              d2::P(X, Y, c, (int)(a * (0.65f + 0.35f * d2::h2(X, Y))));
            }
  }
}

// 塗った円盤 (はげかけ)
static void wornDisc(float cx, float cy, float r, uint32_t c, int a)
{
  for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
    for (int x = (int)(cx - r); x <= (int)(cx + r); x++)
    {
      float d = hypotf(x - cx, y - cy);
      if (d > r)
        continue;
      float wv = d2::vn(x * 0.18f + cx, y * 0.18f);
      if (wv > 0.66f)
        continue;
      int al = (int)(a * std::min(1.0f, (r - d) * 0.7f) * (0.75f + 0.25f * d2::h2(x, y)));
      d2::P(x, y, c, al);
    }
}

void Game7::buildTable()
{
  Surf s;
  s.buf = bg;
  s.y0 = 0;
  s.h = H;
  d2::S = &s;
  // 上の帯: 緑のほうろう看板 (さびの点)
  for (int y = 0; y < HUD_H; y++)
    for (int x = 0; x < W; x++)
    {
      uint32_t c = mix(0x1C3024, 0x2A4234, d2::vn(x * 0.1f, y * 0.2f));
      if (d2::h2(x, y) > 0.985f || d2::vn(x * 0.3f + 7, y * 0.3f) > 0.86f)
        c = mix(c, 0x6A3A1A, 0.7f);
      d2::P(x, y, c);
    }
  d2::R(0, HUD_H - 1, W, 1, 0x8A7A50);
  // 面
  for (int y = HUD_H; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      int rg = region(x, y);
      uint32_t c;
      if (rg == 0)
      {
        // 黄ばんだクリームの板: まわりほど焼けて暗く、水のしみ、細かい汚れ
        float ux = (x - 111) / 108.0f, uy = (y - 170) / 150.0f;
        c = mix(0xC8BA92, 0x8A7650, (ux * ux + uy * uy) * 0.75f);
        float st = d2::vn(x * 0.03f + 3, y * 0.03f);
        c = mix(c, 0x7A6440, std::max(0.0f, st - 0.55f) * 1.3f);
        float ring = fabsf(st - 0.55f); // しみのふち
        if (ring < 0.01f)
          c = mix(c, 0x6A5434, 0.18f);
        c = scale(c, 1 + (d2::h2(x, y) - 0.5f) * 0.07f + (d2::vn(x * 0.2f, y * 0.2f) - 0.5f) * 0.05f);
      }
      else if (rg == 1)
      {
        // 外の枠: 木に緑のペンキ。角や縁ははげて木が出ている
        float gr = d2::vn(x * 0.9f, y * 0.05f);
        uint32_t wood = mix(0x3A2614, 0x5A3C20, gr);
        float chip = d2::vn(x * 0.12f + 20, y * 0.12f);
        c = chip > 0.58f ? wood : mix(0x3A5A44, 0x2E4A38, gr * 0.6f + d2::h2(x, y) * 0.2f);
      }
      else if (rg == 2)
        c = mix(0x2A1E14, 0x3E2C1C, 0.5f + 0.5f * sinf((x - XD) * 0.4f));
      else
      {
        // 樋: さびたブリキ
        c = mix(0x3A3428, 0x2A2418, (y - TRAY_Y) / 16.0f);
        if (d2::vn(x * 0.2f, y * 0.4f) > 0.6f)
          c = mix(c, 0x6A3818, 0.6f);
      }
      d2::P(x, y, c);
    }
  // 色あせた放射の線 (10 の穴から)
  for (int i = 0; i < 24; i++)
  {
    float a = i * PI_F / 12;
    for (float r = 22; r < 170; r += 1.5f)
    {
      float x = 111 + cosf(a) * r, y = 198 + sinf(a) * r;
      if (region((int)x, (int)y) == 0 && d2::vn(x * 0.1f, y * 0.1f) < 0.62f)
        d2::P(x, y, 0xC88A50, 40);
    }
  }
  // 名前: はげかけた赤い字に、ずれた青い影
  wornText(112, 42, "SMART BALL", 0x4A5A7A, 2, 170, 0.8f);
  wornText(111, 41, "SMART BALL", 0xA8402E, 2, 230, 0.8f);
  // 穴のまわりの塗りの円盤と、払い出しの数
  for (int h = 0; h < NH; h++)
  {
    const Hole &o = HOLES[h];
    uint32_t c = o.v >= 10 ? 0xC09040 : o.v >= 3 ? 0xB05A4A : o.v >= 2 ? 0xC08A50 : 0x5A7A94;
    wornDisc(o.x, o.y, o.v >= 10 ? 19 : 16.5f, c, 150);
    char t[4];
    snprintf(t, sizeof(t), "%d", o.v);
    int sz = o.v >= 10 ? 3 : 2;
    float ty = o.y > 230 ? o.y - 33 : o.y + 15; // 下の段は上に書く
    wornText(o.x, ty, t, o.v >= 10 ? 0x8A2216 : o.v >= 3 ? 0x7A2A1A : 0x2A2A40, sz, 235, 0.8f);
  }
  // 穴: くすんだ真鍮の縁 (緑青の点) と黒い穴
  for (int h = 0; h < NH; h++)
  {
    const Hole &o = HOLES[h];
    for (int y = (int)o.y - 8; y <= (int)o.y + 8; y++)
      for (int x = (int)o.x - 8; x <= (int)o.x + 8; x++)
      {
        float dx = x - o.x, dy = y - o.y, d = hypotf(dx, dy);
        if (d > 7.5f)
          continue;
        uint32_t c;
        if (d > 5.3f)
        {
          float lit = clampf(0.6f - (dx + dy) / 14, 0, 1);
          c = mix(0x5A4420, 0xB89850, lit);
          if (d2::h2(x * 3, y) > 0.8f)
            c = mix(c, 0x4A7A60, 0.6f); // 緑青
        }
        else
          c = mix(0x000000, 0x181208, clampf((dy + 5) / 12, 0, 1) * 0.6f);
        d2::P(x, y, c, d > 7 ? 140 : 255);
      }
  }
  // 壁: くすんだ鉄の手すりに、さび (いちばん最後の返しゴムは別に描く)
  const int nIron = nW - 1;
  for (int i = 0; i < nIron; i++)
    d2::LT(walls[i].x0, walls[i].y0, walls[i].x1, walls[i].y1, 3, 0x7A7468);
  for (int i = 0; i < nIron; i++)
    d2::L(walls[i].x0, walls[i].y0, walls[i].x1, walls[i].y1, 0xA8A294);
  // 返しゴム: 2 本の真鍮の柱に張った、古くなって白っぽい黒ゴム
  d2::LT(RUB_X0 + 1.5f, RUB_Y0 + 2, RUB_X1 + 1.5f, RUB_Y1 + 2, 5, 0x000000, 90);
  d2::LT(RUB_X0, RUB_Y0, RUB_X1, RUB_Y1, 5, 0x2A2420);
  d2::L(RUB_X0 + 1, RUB_Y0 + 1, RUB_X1 + 1, RUB_Y1 + 1, 0x5A5048);
  for (float u = 0.1f; u < 0.9f; u += 0.05f)
    if (d2::h2(u * 97, 3) > 0.7f) // ひび割れ
      d2::P(RUB_X0 + (RUB_X1 - RUB_X0) * u, RUB_Y0 + (RUB_Y1 - RUB_Y0) * u, 0x7A7064);
  for (int k = 0; k < 2; k++)
  {
    float px = k ? RUB_X1 : RUB_X0, py = k ? RUB_Y1 : RUB_Y0;
    d2::C(px + 1, py + 1.5f, 3.5f, 0x000000, 100);
    d2::C(px, py, 3.2f, 0x8A7038);
    d2::C(px - 0.8f, py - 0.8f, 1.4f, 0xC8A860);
  }
  for (int i = 0; i < nIron; i++)
    for (float u = 0; u <= 1; u += 0.08f)
    {
      float x = walls[i].x0 + (walls[i].x1 - walls[i].x0) * u, y = walls[i].y0 + (walls[i].y1 - walls[i].y0) * u;
      if (d2::h2(x, y) > 0.6f)
        d2::C(x + (d2::h2(y, x) - 0.5f) * 3, y, 1.5f + d2::h2(x + 1, y) * 1.5f, 0x7A3E18, 150);
    }
  // 釘: 影、さびのしみ、頭
  for (int i = 0; i < nN; i++)
  {
    bool rusty = d2::h2(NX[i] * 7, NY[i] * 3) > 0.74f;
    if (rusty)
      d2::C(NX[i] + 0.5f, NY[i] + 1, 3.5f, 0x7A4A20, 50);
    d2::C(NX[i] + 1, NY[i] + 1.5f, 1.8f, 0x000000, 100);
  }
  for (int i = 0; i < nN; i++)
  {
    bool rusty = d2::h2(NX[i] * 7, NY[i] * 3) > 0.74f;
    d2::C(NX[i], NY[i], 1.7f, rusty ? 0x8A4A22 : 0x9A9284);
    d2::P(NX[i] - 0.5f, NY[i] - 0.5f, rusty ? 0xB87A48 : 0xE0DCD0);
  }
  // 傷: 細い線をあちこちに
  for (int i = 0; i < 60; i++)
  {
    float x = 10 + d2::hs(i * 3.1f) * 200, y = 34 + d2::hs(i * 7.7f) * 268, a = d2::hs(i * 1.3f) * PI_F, l = 4 + d2::hs(i * 5.9f) * 14;
    if (region((int)x, (int)y) == 0)
      d2::L(x, y, x + cosf(a) * l, y + sinf(a) * l, d2::hs(i * 2.2f) > 0.5f ? 0xE8DCC0 : 0x5A4830, 45);
  }
  // ガラスの映りこみ: ななめの淡い帯
  for (int y = HUD_H; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      float u = x * 0.8f + y;
      float band = std::max(0.0f, 1 - fabsf(fmodf(u, 190) - 60) / 9) + std::max(0.0f, 1 - fabsf(fmodf(u, 190) - 80) / 3) * 0.6f;
      if (band > 0)
        d2::P(x, y, 0xFFFFF0, (int)(16 * band));
    }
  // 全体を色あせさせる: セピアに寄せ、黒を少し浮かせる
  for (int i = HUD_H * W; i < W * H; i++)
  {
    uint32_t c = to888(bg[i]);
    int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    float l = (r * 0.3f + g * 0.59f + b * 0.11f) / 255;
    uint32_t sep = ((int)clampf(l * 255 * 1.07f, 0, 255) << 16) | ((int)clampf(l * 255 * 0.95f, 0, 255) << 8) | (int)clampf(l * 255 * 0.78f, 0, 255);
    c = mix(c, sep, 0.35f);
    c = mix(c, 0x2A2418, 0.06f);
    bg[i] = d2::c565(c);
  }
}

bool Game7::begin()
{
  seed = esp_random() | 1;
  buildWalls();
  buildNails();
  bg = (uint16_t *)ps_malloc(W * H * 2);
  if (!bg)
    return false;
  buildTable();
  startDemo();
  return true;
}

// ---- 流れ ----
void Game7::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  pool = START_BALLS;
  win = 0;
  for (int i = 0; i < NB; i++)
    b[i].st = 0;
  for (auto &p : pops)
    p.t = 0;
  feedT = 20;
  lockT = 0;
  tiltW = 0;
  aiT = 0;
  spitT = -1;
  resetYokai();
}

void Game7::resetYokai()
{
  for (int i = 0; i < 3; i++)
  {
    Yokai &y = yk[i];
    y = Yokai{0, 0, 0, 0, (int)((1.5f + rnd() * 3) * FPS), 0, rnd() * 6.28f};
  }
}

// 妖怪: 隠れている (st 0) → 出てくる (1) → 出ている (2) → 引っこむ (3) → 隠れている
void Game7::stepYokai()
{
  for (int i = 0; i < 3; i++)
  {
    Yokai &y = yk[i];
    switch (y.st)
    {
    case 0:
      if (--y.t <= 0)
        y.st = 1;
      break;
    case 1:
      y.k = std::min(1.0f, y.k + 1.0f / (0.4f * FPS));
      if (y.k >= 1)
      {
        y.st = 2;
        y.t = (int)((6 + rnd() * 7) * FPS);
      }
      break;
    case 2:
      if (--y.t <= 0)
        y.st = 3;
      break;
    default:
      y.k = std::max(0.0f, y.k - 1.0f / (0.4f * FPS));
      if (y.k <= 0)
      {
        y.st = 0;
        y.t = (int)((2 + rnd() * 5) * FPS);
      }
      break;
    }
    if (y.hitT > 0)
      y.hitT--;
    if (i == 0)
    {
      // 上: 左右にゆっくりただよい、少し上下にゆれる
      y.ph += 0.55f / FPS;
      y.x = 111 + 44 * sinf(y.ph);
      y.y = Y_TOP_Y + 1.5f * sinf(y.ph * 5.3f);
    }
    else
    {
      // 左右: 壁の裏から横へすべり出て、上下にゆれる
      y.ph += 0.8f / FPS;
      float out = YR * (2 * y.k - 1);
      y.x = i == 1 ? XL + WR + out : XD - WR - out;
      y.y = Y_SIDE_Y[i - 1] + 16 * sinf(y.ph);
    }
  }
}


void Game7::newGame()
{
  startDemo();
  mode = Mode::Play;
  autoPlay = false;
  cheated = false;
  sfx(Sfx::Rack);
}

void Game7::debugEnd()
{
  cheated = true;
  pool = 0;
}

bool Game7::laneBusy() const
{
  for (int i = 0; i < NB; i++)
    if (b[i].st == 1 && b[i].x > XD - 2 && b[i].y > GATE_Y0 - 4)
      return true;
  return false;
}

// ---- 動き ----
void Game7::collideSeg(Ball &o, float x0, float y0, float x1, float y1, float e, bool gate, float rot)
{
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
  float u = clampf(((o.x - x0) * dx + (o.y - y0) * dy) / l2, 0, 1);
  float qx = x0 + dx * u, qy = y0 + dy * u, ex = o.x - qx, ey = o.y - qy, dd = ex * ex + ey * ey;
  if (dd >= RB * RB)
    return;
  float len = sqrtf(l2), d = sqrtf(dd), nx, ny;
  if (d > 1e-4f)
    nx = ex / d, ny = ey / d;
  else
    nx = dy / len, ny = -dx / len;
  if (gate)
  {
    float ux = dy / len, uy = -dx / len; // 上を向いた法線。上からだけ止める
    if ((o.x - x0) * ux + (o.y - y0) * uy < 0 || o.vx * ux + o.vy * uy >= 0)
      return;
  }
  o.x = qx + nx * RB;
  o.y = qy + ny * RB;
  if (rot != 0) // はね返す向きだけ回す (ゴムが rot だけ立っているように)
  {
    float c = cosf(rot), sn = sinf(rot), mx = nx * c + ny * sn, my = -nx * sn + ny * c;
    nx = mx;
    ny = my;
  }
  float vn = o.vx * nx + o.vy * ny;
  if (vn < 0)
  {
    if (e >= E_RUB && vn < -120)
      sfx(Sfx::Sling, std::min(1.0f, -vn / 500)); // 返しゴム: ボヨン
    o.vx -= (1 + e) * vn * nx;
    o.vy -= (1 + e) * vn * ny;
  }
}

void Game7::physics()
{
  const int SUB = 8;
  const float dt = 1.0f / FPS / SUB;
  const float NRR = BALL_R + NR;
  float ply = PLY + plP * PL_TRAVEL;
  for (int sstep = 0; sstep < SUB; sstep++)
  {
    for (int i = 0; i < NB; i++)
    {
      Ball &o = b[i];
      if (o.st != 1)
        continue;
      o.vy += GRAV * dt;
      float dr = 1 - 0.25f * dt;
      o.vx *= dr;
      o.vy *= dr;
      float v = hypotf(o.vx, o.vy);
      if (v > VMAX)
      {
        o.vx *= VMAX / v;
        o.vy *= VMAX / v;
      }
      o.x += o.vx * dt;
      o.y += o.vy * dt;
      for (int k = 0; k < nW; k++)
        collideSeg(o, walls[k].x0, walls[k].y0, walls[k].x1, walls[k].y1, walls[k].e, walls[k].gate, k == nW - 1 ? o.rub : 0);
      // 打ち出す棒
      if (o.x > XD && o.y > ply - BALL_R && o.y < ply + 12)
      {
        o.y = ply - BALL_R;
        if (o.vy > 0)
          o.vy = o.vy > 40 ? -o.vy * 0.2f : 0;
      }
      // 釘 (吐かれて宙を跳んでいる玉は当たらない)
      for (int k = 0; k < nN && !o.nocap; k++)
      {
        float dx = o.x - NX[k];
        if (dx >= NRR || dx <= -NRR)
          continue;
        float dy = o.y - NY[k];
        if (dy >= NRR || dy <= -NRR)
          continue;
        float dd = dx * dx + dy * dy;
        if (dd >= NRR * NRR || dd < 1e-6f)
          continue;
        float d = sqrtf(dd), nx = dx / d, ny = dy / d;
        o.x = NX[k] + nx * NRR;
        o.y = NY[k] + ny * NRR;
        float vn = o.vx * nx + o.vy * ny;
        if (vn < 0)
        {
          o.vx -= (1 + E_NAIL) * vn * nx;
          o.vy -= (1 + E_NAIL) * vn * ny;
          if (-vn > 60 && nailN < 2)
          {
            nailN++;
            sfx(Sfx::Nail, clampf(-vn / 400, 0.2f, 1));
          }
        }
        // 釘のてっぺんで釣り合って止まらないように、ほんの少しゆらす
        if (ny < -0.97f && fabsf(o.vx) < 6)
          o.vx += (rnd() - 0.5f) * 12;
      }
      // 妖怪 (出ているときだけ)
      for (int k = 0; k < 3; k++)
      {
        Yokai &y = yk[k];
        if (y.k < 0.6f)
          continue;
        const float RR = YR + BALL_R;
        float dx = o.x - y.x, dy = o.y - y.y, dd = dx * dx + dy * dy;
        if (dd >= RR * RR || dd < 1e-6f)
          continue;
        float d = sqrtf(dd), nx = dx / d, ny = dy / d;
        o.x = y.x + nx * RR;
        o.y = y.y + ny * RR;
        float vn = o.vx * nx + o.vy * ny;
        if (vn < 0)
        {
          o.vx -= (1 + E_YOKAI) * vn * nx;
          o.vy -= (1 + E_YOKAI) * vn * ny;
        }
        vn = o.vx * nx + o.vy * ny;
        if (vn < KICK_YOKAI)
        {
          o.vx += (KICK_YOKAI - vn) * nx;
          o.vy += (KICK_YOKAI - vn) * ny;
        }
        if (y.hitT < 4)
        {
          y.hitT = 12;
          sfx(Sfx::Yokai);
        }
      }
    }
    // 玉どうし
    for (int i = 0; i < NB; i++)
    {
      Ball &p = b[i];
      if (p.st != 1)
        continue;
      for (int j = i + 1; j < NB; j++)
      {
        Ball &q = b[j];
        if (q.st != 1)
          continue;
        float dx = q.x - p.x, dy = q.y - p.y, dd = dx * dx + dy * dy;
        if (dd >= 4 * BALL_R * BALL_R || dd < 1e-6f)
          continue;
        float d = sqrtf(dd), nx = dx / d, ny = dy / d, ov = (2 * BALL_R - d) / 2;
        p.x -= nx * ov;
        p.y -= ny * ov;
        q.x += nx * ov;
        q.y += ny * ov;
        float rel = (p.vx - q.vx) * nx + (p.vy - q.vy) * ny;
        if (rel > 0)
        {
          float imp = 0.9f * rel;
          p.vx -= imp * nx;
          p.vy -= imp * ny;
          q.vx += imp * nx;
          q.vy += imp * ny;
          if (rel > 40)
            sfx(Sfx::BallClick, clampf(rel / 500, 0.1f, 0.6f));
        }
      }
    }
    // 穴と樋
    for (int i = 0; i < NB; i++)
    {
      Ball &o = b[i];
      if (o.st != 1 || o.nocap)
        continue;
      for (int h = 0; h < NH; h++)
        if (fabsf(o.x - HOLES[h].x) < CAP_R && fabsf(o.y - HOLES[h].y) < CAP_R && hypotf(o.x - HOLES[h].x, o.y - HOLES[h].y) < CAP_R)
        {
          o.st = 2;
          o.hole = h;
          o.t = 0;
          sfx(Sfx::HoleIn);
          break;
        }
      if (o.st == 1 && o.y > OUT_Y)
        o.st = 0;
    }
  }
}

// 打ち出す: 引いているとき・止めているときは引いた量を覚える。急に戻した (手を離した) ら、その量で打つ
void Game7::stepPlunger(float pull)
{
  if (lockT > 0)
    pull = 0;
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
      for (int i = 0; i < NB; i++)
      {
        Ball &o = b[i];
        if (o.st == 1 && o.x > XD && o.y > ply - BALL_R - 6 && fabsf(o.vy) < 80)
        {
          o.vy = -(230 + 520 * powf(p, 1.7f)); // 弱い側を細かく: 半分引いて 3 割ほどの強さ。いっぱいに引けば今までと同じ
          sfx(Sfx::Launch, 0.4f + 0.6f * p);
        }
      }
    }
    plMax = 0;
    plAge = 0;
  }
  prevPull = pull;
}

void Game7::nudge(int dir)
{
  if (lockT > 0)
    return;
  for (int i = 0; i < NB; i++)
    if (b[i].st == 1 && b[i].x < XD)
    {
      b[i].vx += dir * 70;
      b[i].vy -= 25;
    }
  shakeT = 5;
  shakeDir = dir;
  sfx(Sfx::Cushion, 0.6f);
  tiltW += 1;
  if (tiltW >= 3.2f)
  {
    lockT = 3 * FPS;
    tiltW = 0;
    sfx(Sfx::Warning, 0.7f);
  }
}

void Game7::stepBoard(const Input *in)
{
  float pull = 0;
  if (in)
  {
    pull = std::max(0.0f, in->AY);
    if (in->FirePressed)
      nudge(-1);
    if (in->BombPressed)
      nudge(1);
  }
  else
  {
    // 自動: 打ち出す所に玉があれば、強さを決めて引いて離す。ときどき揺らす
    bool ready = false;
    for (int i = 0; i < NB; i++)
      if (b[i].st == 1 && b[i].x > XD && b[i].y > 270 && (aiT > 0 || fabsf(b[i].vy) < 30))
        ready = true;
    if (ready && lockT == 0)
    {
      if (aiT == 0)
        aiP = 0.3f + rnd() * 0.7f;
      aiT++;
      if (aiT > 8 && aiT < 30)
        pull = aiP * std::min(1.0f, (aiT - 8) / 12.0f);
      if (aiT >= 30)
        aiT = 0;
    }
    else
      aiT = 0;
    if (--aiNudgeT <= 0)
    {
      aiNudgeT = 40 + (int)(rnd() * 120);
      nudge(rnd() < 0.5f ? -1 : 1);
    }
  }
  stepPlunger(pull);
  stepYokai();
  if (lockT > 0)
    lockT--;
  if (shakeT > 0)
    shakeT--;
  tiltW = std::max(0.0f, tiltW - 0.8f / FPS);
  // 次の玉を打ち出す所へ
  if (feedT > 0)
    feedT--;
  if (feedT <= 0 && pool > 0 && !laneBusy())
  {
    for (int i = 0; i < NB; i++)
      if (b[i].st == 0)
      {
        b[i] = Ball{LANE_X, 250, 0, 60, 1, 0, 0, rnd() * RUB_TILT};
        pool--;
        statShots++;
        feedT = 10;
        sfx(Sfx::Feed, 0.7f);
        break;
      }
  }
  physics();
  // 穴に落ちた玉: 少しして払い出す
  for (int i = 0; i < NB; i++)
  {
    Ball &o = b[i];
    if (o.nocap)
      o.nocap--;
    if (o.st == 2 && ++o.t >= FALL_N && HOLES[o.hole].v >= 10 && spitT < 0 && rnd() < SPIT_P)
    {
      // 10 の穴: ときどき穴の妖怪が玉をくわえて、吐き出す
      o.st = 3;
      spitT = 0;
      spitBall = i;
    }
    if (o.st == 2 && o.t >= FALL_N)
    {
      o.st = 0;
      int v = HOLES[o.hole].v;
      pool += v;
      win += v;
      statWin += v;
      sfx(Sfx::Payout, v >= 3 ? 1 : 0.6f);
      for (auto &p : pops)
        if (p.t <= 0)
        {
          p = Pop{HOLES[o.hole].x, HOLES[o.hole].y - 10, v, 40};
          break;
        }
    }
    // 引っかかって止まった玉は少し押す
    if (o.st == 1 && o.x < XD)
    {
      if (hypotf(o.vx, o.vy) < 5)
      {
        if (++o.t > 3 * FPS)
        {
          o.vx = (rnd() - 0.5f) * 120;
          o.vy = -40;
          o.t = 0;
        }
      }
      else
        o.t = 0;
    }
  }
  if (spitT >= 0)
  {
    spitT++;
    if (spitT == SPIT_AT && b[spitBall].st == 3)
    {
      // ぺっ: 玉は宙を跳んで釘を飛び越え (HOP_N フレームのあいだ釘に当たらない)、左上か右上へ散る
      // (釘に当てると、10 の穴を囲む釘の筒の中ではね返って同じ穴へ落ち直してしまう)
      Ball &o = b[spitBall];
      const Hole &h = HOLES[o.hole];
      float a = (rnd() < 0.5f ? -1 : 1) * (0.25f + rnd() * 0.65f), sp = 240 + rnd() * 80;
      o.x = h.x;
      o.y = h.y - 2;
      o.vx = sinf(a) * sp;
      o.vy = -cosf(a) * sp;
      o.st = 1;
      o.t = 0;
      o.nocap = HOP_N;
      sfx(Sfx::Yokai);
      sfx(Sfx::Launch, 0.5f);
    }
    if (spitT >= SPIT_END)
      spitT = -1;
  }
  for (auto &p : pops)
    if (p.t > 0)
      p.t--;
  // 持ち玉も台の玉もなくなったら終わり
  bool any = false;
  for (int i = 0; i < NB; i++)
    if (b[i].st)
      any = true;
  if (!any && pool <= 0)
  {
    if (mode == Mode::Play)
    {
      mode = Mode::Over;
      modeT = 0;
      sfx(Sfx::GameOver);
    }
    else
      startDemo();
  }
}

// ---- 名前とランキング ----
int Game7::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

void Game7::stepEntry(const Input &in)
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

void Game7::finishEntry()
{
  char name[4];
  memcpy(name, entry, 4);
  for (int i = 2; i >= 0 && name[i] == ' '; i--)
    name[i] = 0;
  if (!name[0])
    strcpy(name, "---");
  memcpy(LastName, entry, 4);
  int i = rankOf(win);
  if (i >= 0)
  {
    for (int j = 4; j > i; j--)
      Table[j] = Table[j - 1];
    Table[i].Score = win;
    memcpy(Table[i].Name, name, 4);
    newRank = i;
    if (TableChanged)
      TableChanged();
  }
  mode = Mode::Ranking;
  modeT = 8 * FPS;
}

// ---- 1 フレーム ----
void Game7::Step(const Input &in)
{
  sndN = 0;
  nailN = 0;
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
    stepBoard(nullptr);
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
    stepBoard(autoPlay ? nullptr : &in);
    if (autoPlay)
      cheated = true;
    break;
  case Mode::Over:
    modeT++;
    if ((modeT > 2 * FPS && in.FirePressed) || modeT > 5 * FPS)
    {
      if (!cheated && rankOf(win) >= 0)
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
// 白い玉 (少し黄ばんだ瀬戸物のような白)。上の照明と、下の板の色を映す
static void drawBall(float cx, float cy, float r, float dark)
{
  Surf *s = d2::S;
  cx += s->ox;
  int y0 = std::max((int)floorf(cy - r - 1), s->y0), y1 = std::min((int)ceilf(cy + r + 1), s->y0 + s->h - 1);
  int x0 = std::max((int)floorf(cx - r - 1), 0), x1 = std::min((int)ceilf(cx + r + 1), W - 1);
  if (y0 > y1 || x0 > x1 || r < 0.5f)
    return;
  float inv = 1 / r, lit = 1 - dark;
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
      float lam = std::max(0.0f, -0.45f * nx - 0.6f * ny + 0.66f * nz);
      uint32_t c = scale(0xDCD6C6, 0.42f + 0.62f * lam);
      if (ny > 0.3f)
        c = mix(c, 0xA89270, (ny - 0.3f) * 0.6f);
      float l2 = lam * lam, l4 = l2 * l2, l8 = l4 * l4;
      c = mix(c, 0xFFFCF0, l8 * l4 * 0.8f);
      c = scale(c, lit);
      row[x] = cov >= 0.99f ? d2::c565(c) : d2::blend(row[x], c, (int)(cov * 255));
    }
  }
}

static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, 1, INK);
}
static void panel(int x, int y, int w, int h)
{
  d2::R(x, y, w, h, 0x1A2A20, 240);
  d2::R(x, y, w, 1, BRASS);
  d2::R(x, y + h - 1, w, 1, 0x6A5A38);
  d2::R(x, y, 1, h, 0x6A5A38);
  d2::R(x + w - 1, y, 1, h, 0x6A5A38);
}

// 妖怪を 1 匹描く。色は台に合わせて色あせた絵の具。hit: 当たった直後 (目をつぶってびくっとする)
// side: 1 = 左の壁から、2 = 右の壁から出ているので、壁より外は描かない
static void drawYokai(int kind, float cx, float cy, float sc, bool hit, int side)
{
  if (sc < 0.05f)
    return;
  if (hit)
    cx += (kind == 0 ? 0 : side == 1 ? -1 : 1), cy -= 1;
  float R = 9 * sc;
  for (int py = (int)floorf(cy - R); py <= (int)ceilf(cy + R); py++)
    for (int px = (int)floorf(cx - R); px <= (int)ceilf(cx + R); px++)
    {
      if ((side == 1 && px <= XL + 1) || (side == 2 && px >= XD - 1))
        continue;
      float dx = (px - cx) / sc, dy = (py - cy) / sc; // 大きさ 1 のときの座標
      uint32_t c = 0;
      bool on = false;
      if (kind == 0)
      {
        // 提灯おばけ: 赤い提灯に、ひとつ目と垂れた舌
        bool cap = fabsf(dx) <= 3.5f && ((dy >= -8.5f && dy < -6.3f) || (dy > 6.3f && dy <= 8.5f));
        float e = (dx * dx) / (6.6f * 6.6f) + (dy * dy) / (7.4f * 7.4f);
        if (cap)
          c = 0x2A2018, on = true;
        else if (e <= 1)
        {
          c = mix(0xB86040, 0x7A3424, clampf((dx + 3) / 9, 0, 1));
          if (((int)floorf(dy + 40)) % 3 == 0)
            c = mix(c, 0x5A2418, 0.45f); // 竹の骨
          on = true;
        }
        if (dy > 3 && dy < 7 && dx > 0.3f && dx < 2.6f)
          c = 0xB04848, on = true; // 舌
      }
      else if (kind == 1)
      {
        // から傘おばけ: 色あせた紫の傘に、ひとつ目と舌、細い一本足
        float top = -7.5f;
        if (dy >= top && dy <= 4.5f && fabsf(dx) <= (dy - top) * 0.62f + 0.5f)
        {
          float rib = fabsf(dx) / (dy - top + 0.5f);
          c = mix(0x8A6A8E, 0x5A3E62, clampf((dy + 6) / 12, 0, 1));
          if (fmodf(rib * 6, 1) < 0.18f)
            c = mix(c, 0x3A2840, 0.5f); // 骨
          on = true;
          if (dy > 3.3f)
            c = 0x4A3050; // 縁
        }
        if (dy > 4.5f && dy <= 8.5f && fabsf(dx) <= 0.6f)
          c = 0x6A4A2A, on = true; // 足
        if (dy > 1.5f && dy < 5.5f && dx > 2.5f && dx < 4.3f && dy > dx - 2)
          c = 0xB04848, on = true; // 舌
      }
      else
      {
        // 一つ目小僧: 坊主頭に大きなひとつ目、赤いほお
        float d = sqrtf(dx * dx + dy * dy);
        if (d <= 6.6f)
        {
          c = mix(0xC8A27A, 0x8A6A4A, clampf((dx * 0.4f + dy * 0.6f + 4) / 10, 0, 1));
          if (dy < -3.5f)
            c = mix(c, 0x6A6A70, 0.35f); // 剃りあと
          if (fabsf(dy - 2.2f) < 1 && fabsf(fabsf(dx) - 3.8f) < 1.2f)
            c = mix(c, 0xC06050, 0.6f); // ほお
          if (dy > 3.4f && dy < 4.4f && fabsf(dx) < 1.4f)
            c = 0x4A2018; // 口
          on = true;
        }
      }
      if (!on)
        continue;
      // ひとつ目 (当たった直後は閉じる)
      float ex = kind == 1 ? 0 : -0.3f, ey = kind == 0 ? -1.6f : kind == 1 ? -1 : -1, er = kind == 2 ? 2.9f : 2.4f;
      float edx = dx - ex, edy = dy - ey, ed = sqrtf(edx * edx + edy * edy);
      if (hit)
      {
        if (fabsf(edy) < 0.6f && fabsf(edx) < er)
          c = 0x1A1008;
      }
      else if (ed <= er)
      {
        c = 0xCFC6AE;
        float pdx = edx - (side == 1 ? 0.6f : side == 2 ? -0.6f : 0), pdy = edy - 0.3f;
        if (pdx * pdx + pdy * pdy <= er * er * 0.25f)
          c = 0x1A1008;
        if (ed > er - 0.7f)
          c = mix(c, 0x3A2010, 0.6f);
      }
      d2::P(px, py, c);
    }
}

// 10 の穴の妖怪: 穴から黒い頭と黄色い目を出し、吐くときに口を開ける。k: 出ている量 (0〜1)
static void drawHoleYokai(float hx, float hy, float k, bool mouth)
{
  if (k <= 0)
    return;
  float cy = hy + 7 - 10 * k, r = 6;
  for (int py = (int)floorf(cy - r); py <= (int)ceilf(cy + r); py++)
    for (int px = (int)floorf(hx - r); px <= (int)ceilf(hx + r); px++)
    {
      float dx = px - hx, dy = py - cy;
      if (dx * dx + dy * dy > r * r)
        continue;
      float hdx = px - hx, hdy = py - hy;
      if (py > hy - 1 && hdx * hdx + hdy * hdy > 5.3f * 5.3f) // 穴より下は穴の中だけ見える
        continue;
      uint32_t c = mix(0x3A3048, 0x18121E, clampf((dx * 0.3f + dy * 0.7f + 3) / 8, 0, 1));
      if (fabsf(fabsf(dx) - 2.4f) < 1.3f && fabsf(dy + 1.2f) < 1.1f)
        c = mouth ? 0x8A7A30 : 0xD0B848; // 目 (吐くときは細める)
      if (mouth && dx * dx + (dy - 2.4f) * (dy - 2.4f) < 2.6f)
        c = 0x6A1E1E; // 開けた口
      d2::P(px, py, c);
    }
}

void Game7::drawDynamic()
{
  if (spitT >= 0)
  {
    int hi = 0;
    while (HOLES[hi].v < 10)
      hi++;
    const Hole &h = HOLES[hi]; // 吐いた玉は、もうほかの穴に入っているかもしれないので、10 の穴を探す
    float k = spitT < 12 ? spitT / 12.0f : spitT <= SPIT_AT ? 1 : std::max(0.0f, 1 - (spitT - SPIT_AT) / 14.0f);
    drawHoleYokai(h.x, h.y, k, spitT >= SPIT_AT - 5 && spitT < SPIT_AT + 5);
  }
  // 妖怪 (影と体)
  for (int i = 0; i < 3; i++)
  {
    const Yokai &y = yk[i];
    if (y.k <= 0)
      continue;
    float sc = i == 0 ? y.k : 1;
    if (i == 0) // 左右の妖怪は壁の裏から出てくるので、影は描かない (壁にかかる)
      d2::C(y.x + 2, y.y + 3, YR * sc, 0x000000, 60);
    drawYokai(i, y.x, y.y, sc, y.hitT > 0, i);
  }
  // 打ち出す棒とバネ (さびた鉄、木の頭)
  {
    float ply = PLY + plP * PL_TRAVEL;
    for (float y = ply + 4; y < 319; y += 2.5f + (319 - ply) * 0.02f)
      d2::L(XD + 3, y, XO - 3, y + 1.5f, 0x7A6A58);
    d2::R(LANE_X - 1, ply, 3, 320 - ply, 0x8A8070);
    d2::R(XD + 2, ply, XO - XD - 3, 4, 0x6A4428);
    d2::R(XD + 2, ply, XO - XD - 3, 1, 0x9A7048);
  }
  // 穴に落ちている玉
  for (int i = 0; i < NB; i++)
    if (b[i].st == 2)
    {
      float k = b[i].t / (float)FALL_N;
      const Hole &o = HOLES[b[i].hole];
      drawBall(b[i].x + (o.x - b[i].x) * k, b[i].y + (o.y - b[i].y) * k, BALL_R * (1 - 0.4f * k), 0.8f * k);
    }
  // 影と玉
  // 吐かれて跳んでいる玉は、高さ (hop) のぶん大きく描き、影を離す
  auto hop = [&](const Ball &o) { return o.nocap ? sinf(PI_F * o.nocap / HOP_N) : 0.0f; };
  for (int i = 0; i < NB; i++)
    if (b[i].st == 1)
      d2::C(b[i].x + 1.5f + 6 * hop(b[i]), b[i].y + 2.5f + 9 * hop(b[i]), BALL_R, 0x000000, 70 - (int)(30 * hop(b[i])));
  for (int i = 0; i < NB; i++)
    if (b[i].st == 1)
      drawBall(b[i].x, b[i].y, BALL_R * (1 + 0.4f * hop(b[i])), 0);
}

void Game7::drawHud(Gfx &g)
{
  char s[24];
  txt(g, 4, 3, MUTE, "BALLS");
  snprintf(s, sizeof(s), "%d", pool);
  txt(g, 38, 2, CREAM, s, false, 3);
  snprintf(s, sizeof(s), "WIN %d", win);
  txt(g, W - 4 - (int)strlen(s) * 12, 2, BRASS, s, false, 3);
  // 樋に落ちている持ち玉の絵 (最大 12 個)
  // 払い出しの数が浮かぶ
  for (auto &p : pops)
    if (p.t > 0)
    {
      snprintf(s, sizeof(s), "+%d", p.v);
      txt(g, (int)p.x, (int)(p.y - (40 - p.t) * 0.5f), p.v >= 3 ? REDT : CREAM, s, true, p.v >= 3 ? 3 : 1);
    }
}

void Game7::drawOverlay(Gfx &g)
{
  char s[40];
  if (volT > 0)
  {
    panel(70, 236, 100, 22);
    txt(g, 78, 244, CREAM, VolumeLevel == 0 ? "VOL OFF" : "VOL");
    for (int i = 0; i < 4; i++)
      d2::R(110 + i * 13, 244, 10, 8, i < VolumeLevel ? BRASS : 0x2A3428);
  }
  if (lockT > 0 && (mode == Mode::Play || mode == Mode::Attract) && (t / 6) % 2)
  {
    txt(g, 111, 116, REDT, "DON'T SHAKE!", true, 3);
    txt(g, 111, 134, CREAM, "WAIT A MOMENT", true);
  }
  switch (mode)
  {
  case Mode::Attract:
  {
    int ph = (modeT / (8 * FPS)) % 3;
    if (ph == 0)
    {
      panel(18, 92, 186, 116);
      txt(g, 111, 104, REDT, "SMART BALL", true, 4);
      txt(g, 111, 128, CREAM, "OLD SHOP CLASSIC", true);
      snprintf(s, sizeof(s), "%d BALLS TO START", Game7::START_BALLS);
      txt(g, 111, 140, MUTE, s, true);
      if ((modeT / 15) % 2 == 0)
        txt(g, 111, 158, BRASS, "PUSH SW1", true, 3);
      txt(g, 111, 192, MUTE, "SW3 HOLD: MENU", true);
    }
    else if (ph == 1)
    {
      panel(14, 88, 194, 120);
      txt(g, 111, 98, BRASS, "HOW TO PLAY", true, 3);
      txt(g, 111, 122, CREAM, "PULL STICK & LET GO:", true);
      txt(g, 111, 132, CREAM, "SHOOT A BALL", true);
      txt(g, 111, 146, CREAM, "SW1 / SW2: SHAKE L / R", true);
      txt(g, 111, 162, MUTE, "HOLE = WIN THAT MANY", true);
      txt(g, 111, 174, MUTE, "BALLS. NO BALLS LEFT =", true);
      txt(g, 111, 186, MUTE, "GAME OVER", true);
    }
    else
    {
      panel(18, 80, 186, 140);
      txt(g, 111, 94, BRASS, "BEST WINS", true, 3);
      static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
      for (int i = 0; i < 5; i++)
      {
        snprintf(s, sizeof(s), "%s  %5d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
        txt(g, 111, 122 + i * 18, i == 0 ? BRASS : CREAM, s, true);
      }
    }
    break;
  }
  case Mode::Play:
    if (pool == START_BALLS - 1 && win == 0 && modeT < 20 * FPS && statShots >= 0)
    {
      bool waiting = false;
      for (int i = 0; i < NB; i++)
        if (b[i].st == 1 && b[i].x > XD && b[i].y > 270)
          waiting = true;
      if (waiting && plP < 0.05f)
      {
        panel(40, 226, 142, 30);
        txt(g, 111, 232, CREAM, "PULL THE STICK BACK", true);
        txt(g, 111, 244, CREAM, "AND LET GO", true);
      }
    }
    if (autoPlay)
      txt(g, 8, 34, CREAM, "AUTO");
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
    txt(g, 111, 122, REDT, "NO MORE BALLS", true, 3);
    snprintf(s, sizeof(s), "WIN %d", win);
    txt(g, 111, 148, CREAM, s, true, 3);
    if (cheated)
      txt(g, 111, 174, MUTE, "TEST PLAY (NO RECORD)", true);
    else if (win > Table[0].Score)
      txt(g, 111, 174, BRASS, "NEW RECORD!", true);
    break;
  case Mode::NameEntry:
  {
    panel(18, 90, 186, 120);
    txt(g, 111, 102, BRASS, "NEW RECORD", true, 3);
    snprintf(s, sizeof(s), "WIN %d", win);
    txt(g, 111, 124, CREAM, s, true);
    for (int i = 0; i < 3; i++)
    {
      char c[2] = {entry[i], 0};
      int x = 111 - 22 + i * 22;
      txt(g, x, 146, i == entryPos ? BRASS : CREAM, c, true, 5);
      if (i == entryPos && (t / 8) % 2)
        d2::R(x - 8, 172, 16, 2, BRASS);
    }
    txt(g, 111, 186, MUTE, "STICK: LETTER  SW1: OK", true);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, 186, 100, MUTE, s);
    break;
  }
  case Mode::Ranking:
  {
    panel(18, 80, 186, 150);
    txt(g, 111, 94, BRASS, "BEST WINS", true, 3);
    static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
    for (int i = 0; i < 5; i++)
    {
      snprintf(s, sizeof(s), "%s  %5d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
      bool mine = i == newRank;
      if (!mine || (t / 8) % 2)
        txt(g, 111, 122 + i * 18, mine ? REDT : i == 0 ? BRASS : CREAM, s, true);
    }
    snprintf(s, sizeof(s), "YOU %d", win);
    txt(g, 111, 214, MUTE, s, true);
    break;
  }
  }
}

void Game7::Draw(Gfx &gx)
{
  uint32_t t0 = micros();
  int ltop = gx.top * 2 / 3;
  int sh = shakeT > 0 ? shakeDir * ((shakeT & 1) ? 2 : 1) : 0; // 揺らしたときは台を左右にずらす
  for (int r = 0; r < LSTRIP; r++)
  {
    int y = ltop + r;
    uint16_t *dst = gx.buf + r * W;
    const uint16_t *src = bg + y * W;
    if (y < HUD_H || sh == 0)
      memcpy(dst, src, W * 2);
    else if (sh > 0)
    {
      memcpy(dst + sh, src, (W - sh) * 2);
      for (int x = 0; x < sh; x++)
        dst[x] = src[0];
    }
    else
    {
      memcpy(dst, src - sh, (W + sh) * 2);
      for (int x = W + sh; x < W; x++)
        dst[x] = src[W - 1];
    }
  }
  Surf s;
  s.buf = gx.buf;
  s.y0 = ltop;
  s.h = LSTRIP;
  s.ox = sh;
  s.cy0 = HUD_H;
  d2::S = &s;
  drawDynamic();
  s.ox = 0;
  s.cy0 = -100000;
  drawHud(gx);
  drawOverlay(gx);
  profDraw += micros() - t0;
}
