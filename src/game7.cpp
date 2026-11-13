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
// 玉どろぼう: ときどき左右の壁の裏から出てきて、転がっている玉を追いかけ、取ったら壁の裏へ逃げる (その玉はなくなる)。
// 逃げているところへほかの玉を当てると、取った玉を落とす
static const float TH_GRAB = 10;    // 玉の中心がこれより近づくと取る
static const float TH_R = 7;        // 逃げているときの当たりの半径 (ほかの玉が当たると落とす)
static const float TH_CHASE = 55, TH_RUN = 75, TH_FLEE = 110, TH_WANDER = 45; // 追いかける・玉を持って逃げる・手ぶらで逃げる速さ
static const float TH_OUT = 16;     // 壁からこれだけ裏へ入ったら消える
static const float TH_FEAR = 36;    // ほかの妖怪がこれより近いと、びくっとしてよける
static const int TRIP_N = 26;       // 釘につまずいて転んでいるフレーム
// 玉どろぼうの巣: 左右の壁の、この高さにあるねずみの穴。盗んだ玉がたまり、NEST_FULL 個であふれて台へ戻る
static const float NEST_Y = 266, NEST_R = 11;
static const int NEST_FULL = 10;
static const float NEST_PILE[10][2] = {{2.5f, 8}, {6.6f, 8.3f}, {10.7f, 8.6f}, {4.5f, 4.4f}, {8.6f, 4.6f}, {2.5f, 0.6f}, {6.6f, 0.8f}, {4.5f, -3.2f}, {2.5f, -7}, {6.6f, -6.8f}};
static const int KOBAN_V = 3;       // 玉を当てて取り返したときの小判の払い出し
static const float NEKO_V = 80, NEKO_R = 8; // 猫又の速さと当たりの半径
static const int BAKE_N = 9 * 30;   // 化け狸の穴が 10 に化けているフレーム
static const float BAKE_WIN = 0.5f; // 化けた穴に入ったとき、本当に 10 出る割合 (ほかは葉っぱ)
static const int PARADE_EVERY = 100, PARADE_N = 14 * 30; // 百鬼夜行: 払い出しの合計がこの数を越えるたびに、この長さ
static const float PARADE_Y = 140, PARADE_GAP = 30, PARADE_V = 32;

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
  // 玉どろぼうの巣: 左右の壁をかじって開けた、ねずみの穴
  for (int sd = 0; sd < 2; sd++)
  {
    float wx = sd ? XD : XL, dir = sd ? -1 : 1;
    for (int y = (int)(NEST_Y - NEST_R - 2); y <= (int)(NEST_Y + NEST_R + 2); y++)
      for (int x = (int)(wx - 3); x <= (int)(wx + NEST_R + 3) || x <= (int)(wx + 3); x++)
      {
        int X = sd ? (int)(2 * wx) - x : x; // 右は左右を返す
        float dx = (X - wx) * dir, dy = y - NEST_Y;
        if (dx < -2)
          continue;
        float d = hypotf(std::max(0.0f, dx), dy) + (d2::h2(X * 5, y * 3) - 0.5f) * 1.6f; // かじったぎざぎざ
        if (d > NEST_R + 1.5f)
          continue;
        uint32_t c = d > NEST_R - 1 ? mix(0x6A4424, 0x3A2412, d2::h2(X, y)) : mix(0x0C0806, 0x22160C, clampf((dy + NEST_R) / (2 * NEST_R), 0, 1) * 0.8f);
        d2::P(X, y, c);
      }
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
  // 百鬼夜行の夜: 青く暗くした台を 3 段階
  for (int k = 0; k < 3; k++)
  {
    bgDark[k] = (uint16_t *)ps_malloc(W * H * 2);
    if (!bgDark[k])
      return false;
    for (int i = 0; i < W * H; i++)
      bgDark[k][i] = i < HUD_H * W ? bg[i] : d2::c565(mix(to888(bg[i]), 0x0A0E26, 0.2f * (k + 1)));
  }
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
  th = Thief{};
  th.st = 0;
  th.t = (int)((8 + rnd() * 8) * FPS);
  th.face = 1;
  neko = Neko{};
  neko.t = (int)((30 + rnd() * 30) * FPS);
  for (auto &n : nest)
    n = Nest{};
  bake = Bake{-1, (int)((15 + rnd() * 15) * FPS), 0, -1};
  zw = Warashi{};
  zw.t = (int)((50 + rnd() * 60) * FPS);
  coin.t = 0;
  paradeT = -1;
  nextParade = PARADE_EVERY;
}

// 近くに出ているほかの妖怪から離れる向き (fx, fy) と、怖さ (0〜1)
float Game7::yokaiFear(float x, float y, float &fx, float &fy)
{
  fx = fy = 0;
  float best = 0;
  for (int i = 0; i < 3; i++)
  {
    const Yokai &q = yk[i];
    if (q.k < 0.6f)
      continue;
    float dx = x - q.x, dy = y - q.y, d = hypotf(dx, dy);
    if (d < TH_FEAR && d > 0.1f)
    {
      float s = 1 - d / TH_FEAR;
      fx += dx / d * s;
      fy += dy / d * s;
      best = std::max(best, s);
    }
  }
  return best;
}

void Game7::addPop(float x, float y, int v, int kind)
{
  for (auto &p : pops)
    if (p.t <= 0)
    {
      p = Pop{x, y, v, 40, kind};
      return;
    }
}

// 払い出す。座敷わらしがいるあいだは 2 倍。払い出しの合計が節目を越えたら百鬼夜行
void Game7::pay(float x, float y, int v)
{
  if (zw.k > 0.5f)
    v *= 2;
  pool += v;
  win += v;
  statWin += v;
  sfx(Sfx::Payout, v >= 3 ? 1 : 0.6f);
  addPop(x, y, v, 0);
  if (win >= nextParade)
  {
    while (win >= nextParade)
      nextParade += PARADE_EVERY;
    if (paradeT < 0)
    {
      paradeT = 0;
      sfx(Sfx::Matsuri);
    }
  }
}

// 玉どろぼうが持っている玉を落として、手ぶらで逃げる。koban: 玉を当てて取り返したので、ごほうびの小判も落とす
void Game7::thiefDrop(bool koban, float nx)
{
  Ball &c = b[th.ball];
  c.st = 1;
  c.x = clampf(th.x + th.face * 7, XL + RB, XD - RB);
  c.y = clampf(th.y, 100, 290);
  c.vx = -nx * 90 + (rnd() - 0.5f) * 40;
  c.vy = -70;
  c.t = 0;
  c.nocap = 0;
  th.st = 3;
  th.hitT = 14;
  th.trip = 0;
  th.exit = th.x < 111 ? -1 : 1;
  sfx(Sfx::Yokai3); // きゅっ
  if (koban && coin.t <= 0)
  {
    coin = Coin{th.x, th.y - 6, -140, 1};
    sfx(Sfx::Koban);
  }
}

// 玉どろぼう: 隠れている (st 0) → 巣から出て玉を追う (1) → 取ったらにやりとし、しばらく持ったままうろうろしてから巣へ逃げる (2) / あきらめて帰る (3)
void Game7::stepThief()
{
  Thief &h = th;
  if (h.hitT > 0)
    h.hitT--;
  if (h.tripCool > 0)
    h.tripCool--;
  if (h.fearCool > 0)
    h.fearCool--;
  auto moveOn = [&]() {
    h.x += h.vx / FPS;
    h.y = clampf(h.y + h.vy / FPS, 100, 292);
    if (fabsf(h.vx) > 4)
      h.face = h.vx > 0 ? 1 : -1;
    h.ph += hypotf(h.vx, h.vy) / FPS * 0.5f;
  };
  // (tx, ty) へ向かう。ほかの妖怪が近いと、びくっとしてよける
  auto steer = [&](float tx, float ty, float sp, float acc) {
    float dx = tx - h.x, dy = ty - h.y, d = std::max(1.0f, hypotf(dx, dy));
    float ux = dx / d, uy = dy / d, fx, fy;
    if (yokaiFear(h.x, h.y, fx, fy) > 0)
    {
      ux += fx * 2.5f;
      uy += fy * 2.5f;
      float l = std::max(0.01f, hypotf(ux, uy));
      ux /= l;
      uy /= l;
      sp *= 1.4f;
      if (h.fearCool == 0)
      {
        h.fearCool = 45;
        h.hitT = 8;
        sfx(Sfx::Yokai3, 0.5f);
        if (h.st == 2) // 反対側へうろうろしなおす
        {
          h.tx = h.x < 111 ? XD - 40 : XL + 40;
          h.ty = 120 + rnd() * 160;
        }
      }
    }
    h.vx += (ux * sp - h.vx) * acc;
    h.vy += (uy * sp - h.vy) * acc;
  };
  // 巣 (exit の側の壁の穴) へ向かい、入りきったら true
  auto toNest = [&](float v) {
    float wx = h.exit < 0 ? XL : XD, mouthX = wx - h.exit * 14, outX = wx + h.exit * TH_OUT;
    bool inside = h.exit < 0 ? h.x > XL + 2 : h.x < XD - 2;
    float tx = inside && fabsf(h.y - NEST_Y) > 3 ? mouthX : outX;
    float dx = tx - h.x, dy = NEST_Y - h.y, d = std::max(1.0f, hypotf(dx, dy));
    h.vx += (dx / d * v - h.vx) * 0.35f;
    h.vy += (dy / d * v - h.vy) * 0.35f;
    moveOn();
    if (inside && fabsf(h.y - NEST_Y) > 6)
      h.x = clampf(h.x, XL + 9, XD - 9);
    return h.exit < 0 ? h.x <= outX + 1 : h.x >= outX - 1;
  };
  switch (h.st)
  {
  case 0:
    if (neko.st != 0 && h.t < FPS)
      h.t = FPS; // 猫又がいるあいだは出てこない
    if (--h.t <= 0)
    {
      h.exit = rnd() < 0.5f ? -1 : 1;
      h.x = h.exit < 0 ? XL - TH_OUT : XD + TH_OUT;
      h.y = NEST_Y;
      h.vx = h.vy = 0;
      h.face = -h.exit;
      h.st = 1;
      h.t = (int)((6 + rnd() * 4) * FPS);
      h.trip = 0;
    }
    break;
  case 1:
  {
    // いちばん近い、台の上を転がっている玉を追う (宙を跳んでいる玉と、打ち出すレーンの玉は追わない)
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < NB; i++)
    {
      const Ball &o = b[i];
      if (o.st != 1 || o.nocap || o.x > XD - 4 || o.y < 90 || o.y > TRAY_Y)
        continue;
      float d = hypotf(o.x - h.x, o.y - h.y);
      if (d < bd)
        bd = d, best = i;
    }
    float tx = best >= 0 ? b[best].x : 111 + 50 * sinf(h.ph * 0.13f), ty = best >= 0 ? b[best].y : h.y;
    steer(tx, ty, best >= 0 ? TH_CHASE : TH_CHASE * 0.5f, 0.25f);
    // 巣から出てくるあいだは、まっすぐ横へ
    if (h.exit < 0 ? h.x < XL + 10 : h.x > XD - 10)
    {
      h.vx = -h.exit * 45;
      h.vy = 0;
    }
    moveOn();
    if (h.x > XL + 9 && h.x < XD - 9)
      h.x = clampf(h.x, XL + 9, XD - 9);
    if (--h.t <= 0)
    {
      h.st = 3; // あきらめて、近いほうの巣へ帰る
      h.exit = h.x < 111 ? -1 : 1;
    }
    break;
  }
  case 2:
  {
    Ball &o = b[h.ball];
    if (h.trip > 0)
    {
      // 釘につまずいて転んだ: 玉が手から跳ねて、起き上がってまた抱える
      h.trip--;
      h.vx *= 0.8f;
      h.vy *= 0.8f;
      float k = std::min(1.0f, (TRIP_N - h.trip) / (float)TRIP_N * 1.6f), up = sinf(PI_F * k);
      o.x = h.x + h.face * (7 + 9 * up);
      o.y = h.y + 3 - 14 * up;
      break;
    }
    if (h.t > 0)
    {
      h.t--; // 取った玉を見せびらかして、にやりとする
      h.vx *= 0.7f;
      h.vy *= 0.7f;
    }
    else if (h.wt > 0)
    {
      // 玉を持ったまま、しばらく台の上をうろうろする。ときどき立ち止まってにやり
      h.wt--;
      if (hypotf(h.tx - h.x, h.ty - h.y) < 6)
      {
        h.tx = XL + 20 + rnd() * (XD - XL - 40);
        h.ty = 110 + rnd() * 175;
        if (rnd() < 0.45f)
          h.t = 12 + (int)(rnd() * 20);
      }
      else
        steer(h.tx, h.ty, TH_WANDER * (0.8f + 0.4f * sinf(h.ph * 0.37f)), 0.2f); // 足取りに緩急
      moveOn();
      h.x = clampf(h.x, XL + 9, XD - 9);
      // 足もとの釘につまずく
      if (!h.tripCool && hypotf(h.vx, h.vy) > 25)
      {
        float fx = h.x + h.face * 3, fy = h.y + 6;
        for (int k = 0; k < nN; k++)
          if (fabsf(NX[k] - fx) < 4 && fabsf(NY[k] - fy) < 4)
          {
            if (rnd() < 0.06f)
            {
              h.trip = TRIP_N;
              h.tripCool = 3 * FPS;
              sfx(Sfx::Yokai4, 0.8f); // ぽん
            }
            break;
          }
      }
      if (h.wt == 0)
        h.exit = h.x < 111 ? -1 : 1; // 飽きたら、近いほうの巣へ
    }
    else if (toNest(TH_RUN))
    {
      // 巣へ逃げきった: 玉は巣にたまり、NEST_FULL 個であふれて台へ戻ってくる
      o.st = 0;
      statStolen++;
      Nest &n = nest[h.exit < 0 ? 0 : 1];
      if (n.spill == 0 && ++n.n >= NEST_FULL)
      {
        n.spill = n.n;
        n.spillT = 25;
      }
      h.st = 0;
      h.t = (int)((12 + rnd() * 12) * FPS);
      sfx(Sfx::Yokai6, 0.8f); // けけっ
      break;
    }
    o.x = h.x + h.face * 7;
    o.y = h.y + 3;
    break;
  }
  default:
    if (toNest(neko.st == 1 ? TH_FLEE * 0.85f : TH_FLEE))
    {
      h.st = 0;
      h.t = (int)((10 + rnd() * 10) * FPS);
    }
    break;
  }
}

// 猫又: 玉どろぼうが台にいると、まれに反対側の壁から出てきて追いかけ回す。どろぼうは玉を放り出して巣へ逃げる
void Game7::stepNeko()
{
  Neko &n = neko;
  if (n.hitT > 0)
    n.hitT--;
  float outX = n.side ? XD + 16 : XL - 16;
  switch (n.st)
  {
  case 0:
    if (--n.t <= 0)
    {
      if ((th.st == 1 || th.st == 2) && th.x > XL + 14 && th.x < XD - 14 && paradeT < 0)
      {
        n.side = th.x < 111 ? 1 : 0; // どろぼうから遠いほうの壁から
        n.x = outX = n.side ? XD + 16 : XL - 16;
        n.y = clampf(th.y, 110, 290);
        n.vx = n.vy = 0;
        n.face = n.side ? -1 : 1;
        n.st = 1;
        n.t = 8 * FPS;
        sfx(Sfx::Neko);
        // どろぼうはびっくりして、玉を放り出して猫と反対の巣へ
        if (th.st == 2)
          thiefDrop(false, n.side ? -1 : 1);
        th.st = 3;
        th.hitT = 14;
        th.exit = n.side ? -1 : 1;
      }
      else
        n.t = 2 * FPS;
    }
    break;
  case 1:
  {
    if (th.st == 0 || --n.t <= 0)
    {
      n.st = 2; // 追いはらって満足
      n.t = (int)(2.2f * FPS);
      n.vx = n.vy = 0;
      if (th.st == 0)
        sfx(Sfx::Neko, 0.7f);
      break;
    }
    float dx = th.x - n.x, dy = th.y - n.y, d = std::max(1.0f, hypotf(dx, dy));
    n.vx += (dx / d * NEKO_V - n.vx) * 0.15f;
    n.vy += (dy / d * NEKO_V - n.vy) * 0.15f;
    if (n.side ? n.x > XD - 11 : n.x < XL + 11)
    {
      n.vx = (n.side ? -1 : 1) * 60;
      n.vy = 0;
    }
    n.x += n.vx / FPS;
    n.y = clampf(n.y + n.vy / FPS, 100, 292);
    if (n.x > XL + 11 && n.x < XD - 11)
      n.x = clampf(n.x, XL + 11, XD - 11);
    if (fabsf(n.vx) > 4)
      n.face = n.vx > 0 ? 1 : -1;
    n.ph += hypotf(n.vx, n.vy) / FPS * 0.4f;
    break;
  }
  case 2:
    n.ph += 0.02f;
    if (--n.t <= 0)
      n.st = 3;
    break;
  default:
    // 来た壁へ、ゆうゆうと帰る
    n.vx = (n.side ? 1 : -1) * 55;
    n.face = n.side ? 1 : -1;
    n.x += n.vx / FPS;
    n.ph += 55.0f / FPS * 0.4f;
    if (n.side ? n.x >= outX : n.x <= outX)
    {
      n.st = 0;
      n.t = (int)((40 + rnd() * 40) * FPS);
    }
    break;
  }
}

// 巣からあふれる玉、化け狸の穴、座敷わらし、小判、百鬼夜行
void Game7::stepExtras()
{
  // 巣があふれる: 玉を 1 個ずつ台へ転がり出す (台の玉がいっぱいなら持ち玉へ)
  for (int s = 0; s < 2; s++)
  {
    Nest &n = nest[s];
    if (n.spill <= 0 || --n.spillT > 0)
      continue;
    if (n.spill == n.n && n.n >= NEST_FULL)
    {
      addPop(s ? XD - 24 : XL + 24, NEST_Y - 18, 0, 2);
      sfx(Sfx::Payout, 0.8f);
    }
    n.spillT = 5;
    bool put = false;
    for (int i = 0; i < NB && !put; i++)
      if (b[i].st == 0)
      {
        float dir = s ? -1 : 1;
        b[i] = Ball{s ? XD - RB - 1 : XL + RB + 1, NEST_Y + 2, dir * (70 + rnd() * 80), -40 - rnd() * 70, 1, 0, 0, rnd() * RUB_TILT, 0};
        put = true;
        sfx(Sfx::BallClick, 0.4f);
      }
    if (!put)
      pool++;
    n.n--;
    n.spill--;
  }
  // 化け狸: 1 の穴をひとつ選んで、しばらく 10 に化けさせる
  if (bake.puffT > 0)
    bake.puffT--;
  if (bake.hole < 0)
  {
    if (--bake.t <= 0)
    {
      int cand[NH], nc = 0;
      for (int h = 0; h < NH; h++)
        if (HOLES[h].v == 1)
          cand[nc++] = h;
      bake.hole = cand[(int)(rnd() * nc) % nc];
      bake.t = BAKE_N;
      bake.puffT = 14;
      bake.puffHole = bake.hole;
      sfx(Sfx::Doron);
    }
  }
  else if (--bake.t <= 0)
  {
    bake.puffT = 14;
    bake.puffHole = bake.hole;
    bake.hole = -1;
    bake.t = (int)((30 + rnd() * 25) * FPS);
    sfx(Sfx::Doron, 0.5f);
  }
  // 座敷わらし: 隠れている (0) → 現れる (1) → 座っている (2) → 消える (3)
  switch (zw.st)
  {
  case 0:
    if (--zw.t <= 0)
    {
      zw.x = rnd() < 0.5f ? 19 : 202;
      zw.y = 294;
      zw.st = 1;
      sfx(Sfx::Yokai8); // ちりん
    }
    break;
  case 1:
    zw.k = std::min(1.0f, zw.k + 1.0f / (0.8f * FPS));
    if (zw.k >= 1)
    {
      zw.st = 2;
      zw.t = 12 * FPS;
    }
    break;
  case 2:
    if (--zw.t <= 0)
      zw.st = 3;
    break;
  default:
    zw.k = std::max(0.0f, zw.k - 1.0f / (0.8f * FPS));
    if (zw.k <= 0)
    {
      zw.st = 0;
      zw.t = (int)((80 + rnd() * 80) * FPS);
    }
    break;
  }
  // 小判: ひらりと跳ねて落ち、払い出す
  if (coin.t > 0)
  {
    coin.t++;
    coin.vy += 500.0f / FPS;
    coin.y += coin.vy / FPS;
    if (coin.t > 22)
    {
      coin.t = 0;
      pay(coin.x, coin.y - 6, KOBAN_V);
    }
  }
  if (paradeT >= 0 && ++paradeT > PARADE_N)
    paradeT = -1;
}

// 妖怪: 隠れている (st 0) → 出てくる (1) → 出ている (2) → 引っこむ (3) → 隠れている
void Game7::stepYokai()
{
  for (int i = 0; i < 3; i++)
  {
    Yokai &y = yk[i];
    if (paradeT >= 0) // 百鬼夜行のあいだは、みんな出てくる
    {
      if (y.st == 0)
        y.t = 0;
      if (y.st == 2 && y.t < 30)
        y.t = 30;
    }
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

void Game7::debugEvent(char c)
{
  if (c == 'P' && paradeT < 0)
  {
    paradeT = 0;
    sfx(Sfx::Matsuri);
  }
  else if (c == 'B' && bake.hole < 0)
    bake.t = 1;
  else if (c == 'W' && zw.st == 0)
    zw.t = 1;
  else if (c == 'T' && th.st == 0)
    th.t = 1;
  else if (c == 'K' && neko.st == 0)
    neko.t = 1;
  else if (c == 'E')
    nest[0].n = nest[1].n = NEST_FULL - 1;
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
          // 10 種類の声からでたらめに。続けて同じ声にはしない
          int v = (int)(rnd() * 10) % 10;
          if (v == lastYkSnd)
            v = (v + 1 + (int)(rnd() * 9)) % 10;
          lastYkSnd = v;
          sfx((Sfx)((int)Sfx::Yokai0 + v));
        }
      }
      // 玉どろぼう: 追いかけているときは触れた玉を取る。逃げているときは、ほかの玉が当たると取った玉を落とす
      if (th.st == 1 && !o.nocap && o.x < XD - 2)
      {
        float dx = o.x - th.x, dy = o.y - th.y;
        if (dx * dx + dy * dy < TH_GRAB * TH_GRAB)
        {
          o.st = 4;
          th.ball = i;
          th.st = 2;
          th.t = 12;
          th.wt = (int)((6 + rnd() * 5) * FPS); // 取ったあと、しばらくうろうろする
          th.tx = th.x;
          th.ty = th.y; // 着いたことにして、すぐ次の行き先を決める
          th.exit = th.x < 111 ? -1 : 1;
          th.vy = 0;
          sfx(Sfx::Steal);
          continue;
        }
      }
      else if (th.st == 2 && i != th.ball && !o.nocap)
      {
        const float RR = TH_R + BALL_R;
        float dx = o.x - th.x, dy = o.y - th.y, dd = dx * dx + dy * dy;
        if (dd < RR * RR && dd > 1e-6f)
        {
          float d = sqrtf(dd), nx = dx / d, ny = dy / d;
          o.x = th.x + nx * RR;
          o.y = th.y + ny * RR;
          float vn = o.vx * nx + o.vy * ny;
          if (vn < 0)
          {
            o.vx -= (1 + E_YOKAI) * vn * nx;
            o.vy -= (1 + E_YOKAI) * vn * ny;
          }
          thiefDrop(true, nx); // 取り返した: 玉を落とし、ごほうびの小判も落とす
        }
      }
      // 猫又: 当たるとはじく
      if (neko.st != 0 && neko.x > XL + 4 && neko.x < XD - 4)
      {
        const float RR = NEKO_R + BALL_R;
        float dx = o.x - neko.x, dy = o.y - neko.y, dd = dx * dx + dy * dy;
        if (dd < RR * RR && dd > 1e-6f)
        {
          float d = sqrtf(dd), nx = dx / d, ny = dy / d;
          o.x = neko.x + nx * RR;
          o.y = neko.y + ny * RR;
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
          if (neko.hitT < 4)
          {
            neko.hitT = 12;
            sfx(Sfx::Yokai1, 0.7f); // ひゅっ
          }
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
  stepThief();
  stepNeko();
  stepExtras();
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
      if (bake.hole == o.hole)
      {
        // 化けた穴: 本当に 10 出るか、狸の葉っぱ (はずれ) か。どちらでも化けの皮ははがれる
        bake.puffT = 14;
        bake.puffHole = bake.hole;
        bake.hole = -1;
        bake.t = (int)((30 + rnd() * 25) * FPS);
        sfx(Sfx::Doron, 0.7f);
        if (rnd() < BAKE_WIN)
          v = 10;
        else
        {
          v = 0;
          addPop(HOLES[o.hole].x, HOLES[o.hole].y - 10, 0, 1);
          sfx(Sfx::Yokai9); // ぷぅ
        }
      }
      if (v > 0)
        pay(HOLES[o.hole].x, HOLES[o.hole].y - 10, v);
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
      sfx(Sfx::Spit);
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

// 描く範囲の行を、いま描いている帯の中だけにする (帯ごとに呼ばれるので、帯の外まで計算すると重い)
static void stripRows(float top, float bot, int &y0, int &y1)
{
  const Surf *s = d2::S;
  y0 = std::max((int)floorf(top), s->y0 - s->oy);
  y1 = std::min((int)ceilf(bot), s->y0 + s->h - 1 - s->oy);
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
  int ry0, ry1;
  stripRows(cy - R, cy + R, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - R); px <= (int)ceilf(cx + R); px++)
    {
      if (((side == 1 || side == 3) && px <= XL + 1) || ((side == 2 || side == 3) && px >= XD - 1))
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
  int ry0, ry1;
  stripRows(cy - r, cy + r, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
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

// 玉どろぼう: 灰色のねずみの妖怪に、緑の唐草の手ぬぐいでほっかむり (あごの下で結ぶ)。face: 向き、ph: 走る足、grin: にやり、hit: 当たってびくっ
// 壁の裏にいる所は描かない
static void drawThief(float cx, float cy, int face, float ph, bool grin, bool hit, bool fallen = false)
{
  float bob = fallen ? 3 : fabsf(sinf(ph)) * -1.2f;
  cy += bob;
  if (hit)
    cy -= 1.5f;
  int step = sinf(ph) > 0 ? 1 : -1;
  int ry0, ry1;
  stripRows(cy - 10, cy + 9, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - 14); px <= (int)ceilf(cx + 14); px++)
    {
      if (px <= XL + 1 || px >= XD - 1)
        continue;
      float dx = (px - cx) * face, dy = fallen ? cy - py : py - cy; // 転ぶと、ひっくり返って足が上
      uint32_t c = 0;
      bool on = false;
      // しっぽ: 後ろへ細く、くねらせる
      if (dx < -6 && dx > -14)
      {
        float ty = 3 - (dx + 6) * 0.35f + 1.6f * sinf(dx * 0.7f + ph * 1.3f);
        if (fabsf(dy - ty) < 0.75f)
          c = 0xA08078, on = true;
      }
      // 足 (走るとき前後に入れかわる)
      float f1 = -3 + step * 1.5f, f2 = 3 - step * 1.5f;
      if ((dx - f1) * (dx - f1) + (dy - 6.8f) * (dy - 6.8f) < 2.0f || (dx - f2) * (dx - f2) + (dy - 6.8f) * (dy - 6.8f) < 2.0f)
        c = 0x7A5A52, on = true;
      // 胴
      float ex = (dx + 0.5f) / 7, ey = (dy - 2) / 5;
      if (ex * ex + ey * ey <= 1)
      {
        c = mix(0x8E8678, 0x524C44, clampf((dy + 1) / 8, 0, 1));
        if (dx > 2.5f && dy > 1)
          c = mix(c, 0xB4AC98, 0.35f); // おなか
        on = true;
      }
      // 頭と鼻先
      float hx = dx - 4, hy = dy + 2;
      bool head = hx * hx + hy * hy <= 4.4f * 4.4f;
      float sx = (dx - 6.6f) / 2.4f, sy = (dy + 0.6f) / 1.8f;
      if (head || sx * sx + sy * sy <= 1)
        c = mix(0xA29A8C, 0x6A6458, clampf((dy + 2) / 6, 0, 1)), on = true;
      // ほっかむり: 頭の上と後ろを包む (顔の前と下は出す)
      if (hx * hx + hy * hy <= 5.0f * 5.0f && !(dx > 3.2f && dy > -3.6f))
      {
        c = 0x2E6644;
        if (sinf(dx * 2.2f + sinf(dy * 1.8f) * 2.2f) * sinf(dy * 2.4f + dx * 0.6f) > 0.55f)
          c = 0xD6CCB0; // 唐草の白
        on = true;
      }
      // 結び目: あごの下
      if ((dx - 2.6f) * (dx - 2.6f) + (dy - 1.6f) * (dy - 1.6f) < 2.4f || (dx > 1 && dx < 2.4f && dy > 2 && dy < 4.2f && fabsf(dx - 1.7f - (dy - 2) * 0.3f) < 0.6f))
        c = 0x2E6644, on = true;
      // 耳: 手ぬぐいから突き出す
      float ax = dx - 2, ay = dy + 6.8f;
      if (ax * ax + ay * ay < 2.1f * 2.1f)
        c = ax * ax + ay * ay < 1.1f * 1.1f ? 0xB88888 : 0x8A8276, on = true;
      if (!on)
        continue;
      // 目 (にやりとするときは細める)
      float edx = dx - 5.3f, edy = dy + 1.9f;
      if (grin || hit)
      {
        if (fabsf(edy + (grin ? 0.2f * edx * edx : 0)) < 0.55f && fabsf(edx) < 1.4f)
          c = 0x140C08;
      }
      else if (edx * edx + edy * edy < 1.15f * 1.15f)
        c = edx > 0.1f && edy < -0.2f ? 0xE8E0C8 : 0x140C08;
      // 鼻
      if ((dx - 8.8f) * (dx - 8.8f) + (dy + 0.9f) * (dy + 0.9f) < 1.0f)
        c = 0xC06A6A;
      d2::P(px, py, c);
    }
  // ひげ
  for (int k = -1; k <= 1; k += 2)
    for (float u = 0; u < 4; u += 0.7f)
    {
      float x = cx + face * (8.6f + u), y = cy + (fallen ? -0.2f : 0.2f) + k * u * 0.35f;
      if (x > XL + 1 && x < XD - 1)
        d2::P(x, y, 0x3A3028, 160);
    }
  // 目を回した星
  if (fallen)
    for (int k = 0; k < 3; k++)
    {
      float a = ph * 0 + k * 2.094f + micros() * 0.000008f;
      float x = cx + face * 3 + cosf(a) * 6, y = cy + 6 + sinf(a) * 2;
      d2::P(x, y, 0xF0D050);
      d2::P(x + 1, y, 0xF0D050, 120);
      d2::P(x - 1, y, 0xF0D050, 120);
      d2::P(x, y + 1, 0xF0D050, 120);
      d2::P(x, y - 1, 0xF0D050, 120);
    }
}

// 猫又: 色あせた三毛の猫に、ふたまたのしっぽ。content: 満足して目を細める、hit: 当たってびくっ
static void drawNeko(float cx, float cy, int face, float ph, bool content, bool hit, bool sitting)
{
  cy += sitting ? 0 : fabsf(sinf(ph)) * -1.3f;
  if (hit)
    cy -= 1.5f;
  int step = sinf(ph) > 0 ? 1 : -1;
  int ry0, ry1;
  stripRows(cy - 17, cy + 10, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - 17); px <= (int)ceilf(cx + 15); px++)
    {
      if (px <= XL + 1 || px >= XD - 1)
        continue;
      float dx = (px - cx) * face, dy = py - cy;
      uint32_t c = 0;
      bool on = false;
      // ふたまたのしっぽ: 後ろから上へ、2 本に分かれてゆらゆら
      if (dx < -6 && dx > -16)
      {
        float u = -(dx + 6); // 0〜10
        for (int k = 0; k < 2; k++)
        {
          float ty = 0 - u * 0.9f - (k ? u * u * 0.06f : -u * u * 0.03f) + 1.2f * sinf(u * 0.5f + ph * 0.8f + k * 1.5f);
          float tx = k ? -u * 0.15f : u * 0.25f;
          if (fabsf(dy - ty) < 1.1f - u * 0.03f && fabsf(tx) < 4)
          {
            c = u > 7.5f ? 0x3A2E26 : 0xD2C6A8;
            on = true;
          }
        }
      }
      // 足
      if (!sitting)
      {
        float f1 = -4 + step * 1.8f, f2 = 4 - step * 1.8f;
        if ((dx - f1) * (dx - f1) + (dy - 7.5f) * (dy - 7.5f) < 2.6f || (dx - f2) * (dx - f2) + (dy - 7.5f) * (dy - 7.5f) < 2.6f)
          c = 0xB8AC90, on = true;
      }
      // 胴 (座っているときは丸く)
      float ex = (dx + 0.5f) / (sitting ? 6.5f : 8.5f), ey = (dy - 2) / (sitting ? 6.5f : 5.5f);
      bool body = ex * ex + ey * ey <= 1;
      // 頭
      float hx = dx - (sitting ? 3.5f : 6), hy = dy + (sitting ? 5 : 3);
      bool head = hx * hx + hy * hy <= 5 * 5;
      // 三角の耳
      bool ear = false;
      for (int k = 0; k < 2; k++)
      {
        float ax = hx - (k ? 2.8f : -2.2f), ay = hy + 4.2f;
        if (ay < 0 && ay > -4.2f && fabsf(ax) < (ay + 4.2f) * 0.55f)
          ear = true;
      }
      if (body || head || ear)
      {
        c = mix(0xDCD0B4, 0xA89C80, clampf((dy + 2) / 10, 0, 1));
        // 三毛のぶち (色あせた茶と黒)
        float m = sinf(dx * 0.55f + 1.3f) * sinf(dy * 0.7f + 0.4f) + 0.4f * sinf(dx * 1.3f - dy * 0.8f);
        if (m > 0.55f)
          c = mix(c, 0xB8743A, 0.8f);
        else if (m < -0.75f)
          c = mix(c, 0x3A2E26, 0.85f);
        if (ear && !head && !body)
          c = 0x8A6A50;
        on = true;
      }
      if (!on)
        continue;
      if (head)
      {
        // 目: 金色に細い瞳。満足すると ^ の形
        for (int k = 0; k < 2; k++)
        {
          float ex2 = hx - (k ? 2.6f : -1.0f), ey2 = hy + 0.6f;
          if (content || hit)
          {
            if (fabsf(ey2 + (content ? -0.35f * ex2 * ex2 : 0)) < 0.5f && fabsf(ex2) < 1.3f)
              c = 0x1A1008;
          }
          else if (ex2 * ex2 + ey2 * ey2 < 1.3f * 1.3f)
            c = fabsf(ex2) < 0.4f ? 0x1A1008 : 0xD8B040;
        }
        if ((hx - 4.6f) * (hx - 4.6f) + (hy - 1.4f) * (hy - 1.4f) < 0.8f)
          c = 0xC06A6A; // 鼻
      }
      d2::P(px, py, c);
    }
}

// 座敷わらし: おかっぱ頭に赤い着物の女の子。k: 濃さ (現れる・消える)
static void drawWarashi(float cx, float cy, float k, int t)
{
  if (k <= 0)
    return;
  int a = (int)(255 * k);
  cy += sinf(t * 0.08f) * 0.6f;
  int ry0, ry1;
  stripRows(cy - 10, cy + 10, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - 9); px <= (int)ceilf(cx + 9); px++)
    {
      float dx = px - cx, dy = py - cy;
      uint32_t c = 0;
      bool on = false;
      // 着物 (座ってすそが広がる)
      if (dy >= 0 && dy <= 9.5f && fabsf(dx) <= 3.6f + dy * 0.5f)
      {
        c = mix(0xB84A40, 0x7A2A26, clampf(dy / 10, 0, 1));
        if (d2::h2(floorf(dx * 0.6f) + 30, floorf(dy * 0.6f)) > 0.8f)
          c = 0xE0C8A0; // 小さな花の柄
        if (dy > 1.6f && dy < 3.2f)
          c = 0xD0A840; // 帯
        on = true;
      }
      // 顔
      float fx = dx, fy = dy + 4.6f;
      if (fx * fx + fy * fy <= 4.2f * 4.2f)
      {
        c = 0xEAD8BC;
        if (fabsf(fy - 0.4f) < 0.7f && fabsf(fabsf(fx) - 1.6f) < 0.6f)
          c = 0x2A1A10; // 目
        if (fabsf(fy - 1.8f) < 0.6f && fabsf(fabsf(fx) - 2.8f) < 0.7f)
          c = mix(c, 0xE07070, 0.6f); // ほお
        if (fabsf(fy - 2.6f) < 0.45f && fabsf(fx) < 0.6f)
          c = 0xB04040; // 口
        on = true;
      }
      // おかっぱ: まっすぐな前髪と、横の髪
      float hx = dx, hy = dy + 5.2f;
      if (hx * hx + hy * hy <= 5.0f * 5.0f && (hy < -1.6f || (fabsf(hx) > 3.1f && hy < 3.4f)))
        c = 0x1A1410, on = true;
      if (fabsf(hx) > 3.1f && fabsf(hx) < 5 && hy >= 0 && hy < 3.4f)
        c = 0x1A1410, on = true;
      if (on)
        d2::P(px, py, c, a);
    }
  // きらきら
  for (int i = 0; i < 3; i++)
  {
    float u = (t + i * 23) % 70 / 70.0f, x = cx + sinf(i * 2.3f + t * 0.01f) * 9, y = cy - 6 - u * 16;
    int al = (int)(k * 200 * sinf(PI_F * u));
    d2::P(x, y, 0xFFF0B0, al);
    d2::P(x + 1, y, 0xFFF0B0, al / 2);
    d2::P(x - 1, y, 0xFFF0B0, al / 2);
    d2::P(x, y - 1, 0xFFF0B0, al / 2);
    d2::P(x, y + 1, 0xFFF0B0, al / 2);
  }
}

// 化け狸の葉っぱ: 1 の数字を隠して、10 と書いてある
static void drawLeaf(float cx, float cy, int t)
{
  float rot = -0.35f + 0.08f * sinf(t * 0.15f);
  float cr = cosf(rot), sr = sinf(rot);
  int ry0, ry1;
  stripRows(cy - 12, cy + 12, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - 15); px <= (int)ceilf(cx + 15); px++)
    {
      float dx = px - cx, dy = py - cy;
      float u = (dx * cr + dy * sr) / 14, v = (-dx * sr + dy * cr) / 9.5f;
      float w = 1 - u * u; // 先がとがった葉の形
      if (fabsf(v) > w * (1 - 0.25f * u) || fabsf(u) > 1)
      {
        // 柄
        if (u < -1 && u > -1.25f && fabsf(v) < 0.08f)
          d2::P(px, py, 0x5A4A20);
        continue;
      }
      uint32_t c = mix(0x5A8A34, 0x2E5A20, clampf((v + 1) / 2, 0, 1));
      if (fabsf(v) < 0.07f || fabsf(fabsf(v) - fabsf(u + 0.2f) * 0.6f) < 0.05f)
        c = mix(c, 0xA8C070, 0.6f); // 葉脈
      d2::P(px, py, c);
    }
  wornText(cx, cy - 7, "10", 0x6A2410, 2, 255, 2.0f);
}

// どろんの煙
static void drawPuff(float cx, float cy, int puffT)
{
  float k = 1 - puffT / 14.0f;
  for (int i = 0; i < 6; i++)
  {
    float a = i * 1.047f + k * 0.8f, r = 4 + k * 10;
    d2::C(cx + cosf(a) * r, cy + sinf(a) * r * 0.7f, 4 + 3 * (1 - k), 0xE8E0D0, (int)(200 * (1 - k)));
  }
  d2::C(cx, cy, 7 * (1 - k) + 2, 0xF0E8D8, (int)(220 * (1 - k)));
}

// 小判: 金色の楕円がくるくる回る
static void drawCoin(float cx, float cy, int t)
{
  float w = fabsf(cosf(t * 0.5f)) * 3.6f + 0.6f, h = 5.2f;
  int ry0, ry1;
  stripRows(cy - h - 1, cy + h + 1, ry0, ry1);
  for (int py = ry0; py <= ry1; py++)
    for (int px = (int)floorf(cx - w - 1); px <= (int)ceilf(cx + w + 1); px++)
    {
      float u = (px - cx) / w, v = (py - cy) / h, d = u * u + v * v;
      if (d > 1)
        continue;
      uint32_t c = mix(0xF0D070, 0xA87820, clampf(u * 0.5f + v * 0.5f + 0.5f, 0, 1));
      if (d > 0.7f)
        c = 0x8A6018;
      if (fabsf(v) < 0.5f && fabsf(fabsf(v) - 0.25f) < 0.12f && w > 2)
        c = mix(c, 0x8A6018, 0.6f); // 刻み
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
  // 化け狸の葉っぱと、どろんの煙
  if (bake.hole >= 0)
  {
    const Hole &o = HOLES[bake.hole];
    float ty = o.y > 230 ? o.y - 33 : o.y + 15; // 数字の場所 (台の絵と同じ)
    drawLeaf(o.x, ty + 8, t);
  }
  if (bake.puffT > 0 && bake.puffHole >= 0)
  {
    const Hole &o = HOLES[bake.puffHole];
    drawPuff(o.x, o.y > 230 ? o.y - 25 : o.y + 23, bake.puffT);
  }
  // 巣にたまった玉
  for (int sd = 0; sd < 2; sd++)
  {
    int n = std::min(nest[sd].n, NEST_FULL);
    for (int i = 0; i < n; i++)
    {
      float x = sd ? XD - NEST_PILE[i][0] : XL + NEST_PILE[i][0];
      drawBall(x, NEST_Y + NEST_PILE[i][1], 2.2f, 0.35f);
    }
  }
  // 座敷わらし
  if (zw.k > 0)
    drawWarashi(zw.x, zw.y, zw.k, t);
  // 玉どろぼうと、持っている玉
  if (th.st != 0)
  {
    if (th.x > XL + 8 && th.x < XD - 8)
      d2::C(th.x + 2, th.y + 6, 7, 0x000000, 55);
    drawThief(th.x, th.y, th.face, th.ph, th.st == 2 && th.t > 0, th.hitT > 0, th.st == 2 && th.trip > 10);
    if (th.st == 2)
    {
      const Ball &o = b[th.ball];
      if (o.x > XL + 2 && o.x < XD - 2)
        drawBall(o.x, o.y, BALL_R * 0.85f, 0.1f);
    }
  }
  // 猫又
  if (neko.st != 0)
  {
    if (neko.x > XL + 10 && neko.x < XD - 10)
      d2::C(neko.x + 2, neko.y + 7, 8, 0x000000, 55);
    drawNeko(neko.x, neko.y, neko.face, neko.ph, neko.st == 2, neko.hitT > 0, neko.st == 2);
  }
  // 小判
  if (coin.t > 0)
    drawCoin(coin.x, coin.y, coin.t);
  // 百鬼夜行: みんなで右から左へ練り歩く (玉には当たらない)
  if (paradeT >= 0)
  {
    static const int ORDER[8] = {0, 3, 1, 4, 2, 5, 6, 0};
    float go = PARADE_V * paradeT / FPS;
    for (int i = 0; i < 8; i++)
    {
      float x = XD + 18 + i * PARADE_GAP - go, ph = go * 0.25f + i;
      if (x < XL - 20 || x > XD + 20)
        continue;
      float y = PARADE_Y + (i % 2) * 4 + sinf(ph * 2) * 1.5f;
      switch (ORDER[i])
      {
      case 3:
        drawThief(x, y, -1, ph * 2, true, false);
        if (x - 7 > XL + 2 && x - 7 < XD - 2)
          drawBall(x - 7, y + 3, BALL_R * 0.85f, 0.1f);
        break;
      case 4:
        drawNeko(x, y, -1, ph * 2, false, false, false);
        break;
      case 5:
        if (x > XL + 10 && x < XD - 10)
          drawWarashi(x, y, 1, t);
        break;
      case 6:
        if (x > XL + 8 && x < XD - 8)
          drawHoleYokai(x, y + 4, 1, (paradeT / 10) % 2);
        break;
      default:
        drawYokai(ORDER[i], x, y, 1, false, 3);
        break;
      }
    }
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
  if (zw.k > 0.5f && (t / 10) % 4) // 座敷わらしがいるあいだは 2 倍
    txt(g, 84, 5, REDT, "x2", false, 2);
  // 樋に落ちている持ち玉の絵 (最大 12 個)
  // 払い出しの数が浮かぶ
  for (auto &p : pops)
    if (p.t > 0)
    {
      int py = (int)(p.y - (40 - p.t) * 0.5f);
      if (p.kind == 1)
        txt(g, (int)p.x, py, 0x9AC070, "LEAF...", true);
      else if (p.kind == 2)
        txt(g, (int)p.x, py, BRASS, "SPILL!", true, 2);
      else
      {
        snprintf(s, sizeof(s), "+%d", p.v);
        txt(g, (int)p.x, py, p.v >= 3 ? REDT : CREAM, s, true, p.v >= 3 ? 3 : 1);
      }
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
  if (paradeT >= 0 && paradeT < 3 * FPS && (mode == Mode::Play || mode == Mode::Attract))
  {
    txt(g, 111, 58, REDT, "HYAKKI YAGYO", true, 3);
    txt(g, 111, 78, CREAM, "NIGHT PARADE OF YOKAI", true);
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
  int dark = paradeT < 0 ? 0 : std::min(3, std::min(paradeT / 8, (PARADE_N - paradeT) / 8)); // 百鬼夜行の夜
  for (int r = 0; r < LSTRIP; r++)
  {
    int y = ltop + r;
    uint16_t *dst = gx.buf + r * W;
    const uint16_t *src = (dark ? bgDark[dark - 1] : bg) + y * W;
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
