// 4 本目 (CORNERS): 四隅のポケットだけの玉突き。遊びの流れは SKYLARK (game.cpp) にそろえる
#include "game4.h"
#include "g2draw.h"
#include <algorithm>

static const int W = 240, H = 320;
static const int FPS = Game4::FPS;
static const int NB = Game4::NB;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";

// 台 (液晶の座標)。上の 22 行は打数などを出すところ
static const int HUD_H = 22;
static const float L = 17, R = 223, T = 39, B = 303; // 玉が転がる面 (クッションの内側)
static const float BALL_R = 6.0f;                         // 玉の半径
static const float MOUTH = 15;                        // 隅からクッションが始まるまで (ポケットの口)
static const float CAP = 10.5f;                       // ポケットの中心からこれより近づくと落ちる
static const float JAW_R = 1.5f;                      // クッションの端 (ポケットの角)
static const float HEAD_Y = T + (B - T) * 0.75f;      // 手玉を置く所
static const float FOOT_Y = T + (B - T) * 0.30f;      // 三角に並べた先頭の玉
static const float PX[4] = {L - 2, R + 2, L - 2, R + 2}, PY[4] = {T - 2, T - 2, B + 2, B + 2};
static const int FALL_N = 9; // 落ちるのにかかるフレーム

// 動き (点/秒)
static const float DECEL = 160;   // 転がる抵抗 (一定の減速)
static const float DRAG = 0.25f;  // 速さに比べた抵抗
static const float STOP_V = 4;    // これより遅くなったら止める
static const float E_BALL = 0.95f, E_CUSH = 0.75f, E_JAW = 0.6f;
static const float V_MIN = 30, V_MAX = 760; // いちばん弱い / 強い打球
static const float P_MIN = 0.3f;            // これより弱く引いて離したら、打たない (やめた)

static const uint32_t INK = 0x000000, CREAM = 0xF2F0E8, GOLD = 0xE0B050, REDT = 0xFF6A5A, MUTE = 0x9AA49E;
static const uint32_t BALL_COL[NB] = {0xF2EEE2, 0xF2C230, 0x2456C8, 0xD8382E, 0x7A3CB0, 0xF07A20,
                                      0x2E9E50, 0x8E2A26, 0x1C1C20, 0xEE6EA4, 0x26B0B8};

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float speedOf(float p) // 引いた量 → 打球の速さ
{
  float u = clampf((p - P_MIN) / (1 - P_MIN), 0, 1);
  return V_MIN + (V_MAX - V_MIN) * powf(u, 1.5f);
}
static float powerFor(float v) // 打球の速さ → 引く量
{
  float u = powf(clampf((v - V_MIN) / (V_MAX - V_MIN), 0, 1), 1 / 1.5f);
  return P_MIN + (1 - P_MIN) * u;
}

float Game4::rnd()
{
  seed = seed * 1664525u + 1013904223u;
  return (seed >> 8) * (1.0f / 16777216.0f);
}

void Game4::sfx(Sfx s, float vol)
{
  if (!Snd || sndN >= 5)
    return;
  sndN++;
  Snd->Play(s, mode == Mode::Attract ? vol * 0.4f : vol);
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

void Game4::buildTable()
{
  Surf s;
  s.buf = bg;
  s.y0 = 0;
  s.h = H;
  d2::S = &s;
  // 上の帯 (打数など)
  for (int y = 0; y < HUD_H; y++)
    d2::R(0, y, W, 1, mix(0x0A0E12, 0x1A2026, y / (float)HUD_H));
  d2::R(0, HUD_H - 1, W, 1, 0x6A5030);
  // 木のふち: 長いほうは縦の木目、短いほうは横の木目
  for (int y = HUD_H; y < H; y++)
    for (int x = 0; x < W; x++)
    {
      bool side = x < L - 4 || x > R + 4;
      float gr = side ? d2::vn(x * 0.9f, y * 0.04f) : d2::vn(x * 0.04f, y * 0.9f);
      float fine = d2::h2(side ? x : y, side ? y / 3 : x / 3) * 0.08f;
      uint32_t c = mix(0x4A2410, 0x7A4220, gr * 0.9f + fine);
      // 外の縁を明るく、内の縁を暗く (面取り)
      int dOut = std::min(std::min(x, W - 1 - x), std::min(y - HUD_H, H - 1 - y));
      if (dOut == 0)
        c = 0x1A0C06;
      else if (dOut == 1)
        c = mix(c, 0xC08050, 0.5f);
      else if (dOut == 2)
        c = mix(c, 0xC08050, 0.25f);
      d2::P(x, y, c);
    }
  // 内の縁の影 (クッションの外側)
  d2::R(L - 6, T - 6, R - L + 12, 1, 0x2A1408);
  d2::R(L - 6, B + 5, R - L + 12, 1, 0x2A1408);
  d2::R(L - 6, T - 6, 1, B - T + 12, 0x2A1408);
  d2::R(R + 5, T - 6, 1, B - T + 12, 0x2A1408);
  // ポケットの口のあたり (クッションの無いところ) は暗い羅紗
  for (int k = 0; k < 4; k++)
  {
    int cx = (k & 1) ? R + 5 - MOUTH - 5 : L - 5;
    int cy = (k & 2) ? B + 5 - MOUTH - 5 : T - 5;
    d2::R(cx, cy, MOUTH + 5, MOUTH + 5, 0x0C4A2A);
  }
  // 羅紗: 真ん中が明るく、まわりが少し暗い。細かい織り目
  float cx0 = (L + R) / 2, cy0 = (T + B) / 2;
  for (int y = (int)T; y <= (int)B; y++)
    for (int x = (int)L; x <= (int)R; x++)
    {
      float ux = (x - cx0) / ((R - L) / 2), uy = (y - cy0) / ((B - T) / 2);
      float k = (ux * ux + uy * uy) * 0.5f;
      uint32_t c = mix(0x1F8A55, 0x10583A, k);
      float n = (d2::h2(x, y) - 0.5f) * 0.06f + (d2::vn(x * 0.15f, y * 0.15f) - 0.5f) * 0.05f;
      d2::P(x, y, scale(c, 1 + n));
    }
  // クッション: 口の手前で斜めに切る。内側の縁は明るく
  {
    const uint32_t CU = 0x15693F, CL = 0x2A9A60, CD = 0x0B3E24;
    Pt top[4] = {{L + MOUTH, T}, {R - MOUTH, T}, {R - MOUTH + 4, T - 4}, {L + MOUTH - 4, T - 4}};
    Pt bot[4] = {{L + MOUTH - 4, B + 4}, {R - MOUTH + 4, B + 4}, {R - MOUTH, B}, {L + MOUTH, B}};
    Pt lef[4] = {{L - 4, T + MOUTH - 4}, {L, T + MOUTH}, {L, B - MOUTH}, {L - 4, B - MOUTH + 4}};
    Pt rig[4] = {{R, T + MOUTH}, {R + 4, T + MOUTH - 4}, {R + 4, B - MOUTH + 4}, {R, B - MOUTH}};
    d2::poly(top, 4, CU);
    d2::poly(bot, 4, CU);
    d2::poly(lef, 4, CU);
    d2::poly(rig, 4, CU);
    d2::R(L + MOUTH, T - 1, R - L - 2 * MOUTH, 1, CL);
    d2::R(L + MOUTH, B, R - L - 2 * MOUTH, 1, CD);
    d2::R(L - 1, T + MOUTH, 1, B - T - 2 * MOUTH, CL);
    d2::R(R, T + MOUTH, 1, B - T - 2 * MOUTH, CD);
  }
  // ポケット: 革の縁と、黒い穴 (奥ほど暗い)
  for (int k = 0; k < 4; k++)
  {
    d2::C(PX[k], PY[k], 13, 0x2A1A10);
    d2::CR(PX[k], PY[k], 13, 0x0E0806);
    d2::C(PX[k], PY[k], 11, 0x080808);
    d2::C(PX[k] + ((k & 1) ? 1 : -1), PY[k] + ((k & 2) ? 1 : -1), 8, 0x000000);
  }
  // 目印の菱形 (貝の白)
  for (int k = 1; k <= 3; k++)
  {
    float x = L + (R - L) * k / 4;
    for (float y : {(float)(HUD_H + 8), (float)(H - 8)})
    {
      d2::R(x - 1, y, 3, 1, 0xE8E0C8);
      d2::P(x, y - 1, 0xE8E0C8);
      d2::P(x, y + 1, 0xE8E0C8);
    }
  }
  for (int k = 1; k <= 7; k++)
  {
    float y = T + (B - T) * k / 8;
    for (float x : {7.0f, (float)(W - 8)})
    {
      d2::R(x - 1, y, 3, 1, 0xE8E0C8);
      d2::P(x, y - 1, 0xE8E0C8);
      d2::P(x, y + 1, 0xE8E0C8);
    }
  }
  // 玉を並べる点と、手玉を置く線
  d2::C(W / 2, FOOT_Y, 1, 0xDDE8DD, 160);
  for (int x = (int)L + 4; x < (int)R - 4; x += 4)
    d2::P(x, HEAD_Y, 0xDDE8DD, 50);
}

bool Game4::begin()
{
  seed = esp_random() | 1;
  bg = (uint16_t *)ps_malloc(W * H * 2);
  if (!bg)
    return false;
  buildTable();
  startDemo();
  return true;
}

// ---- 並べる ----
void Game4::rack()
{
  int ord[10];
  for (int i = 0; i < 10; i++)
    ord[i] = i + 1;
  for (int i = 9; i > 0; i--)
    std::swap(ord[i], ord[(int)(rnd() * (i + 1)) % (i + 1)]);
  for (int i = 0; i < NB; i++)
    b[i] = Ball{0, 0, 0, 0, true, 0, 0, 0, 0};
  b[0].x = W / 2;
  b[0].y = HEAD_Y;
  float gap = 2 * BALL_R + 0.4f;
  int n = 0;
  for (int row = 0; row < 4; row++)
    for (int j = 0; j <= row; j++)
    {
      Ball &o = b[ord[n++]];
      o.x = W / 2 + (j - row / 2.0f) * gap + (rnd() - 0.5f) * 0.3f;
      o.y = FOOT_Y - row * gap * 0.866f + (rnd() - 0.5f) * 0.3f;
    }
  phase = Phase::Aim;
  aiming = false;
  aimP = 0;
  aiT = 0;
  scratched = false;
  needCenter = false;
}

void Game4::respotCue()
{
  Ball &c = b[0];
  c = Ball{W / 2.0f, HEAD_Y, 0, 0, true, 0, 0, 0, 0};
  for (int k = 0; k < 200; k++) // 置けるところを、手玉の線から少しずつ広げて探す
  {
    float x = W / 2 + ((k % 2) ? 1 : -1) * (k / 2) * 3.0f, y = HEAD_Y + (k / 30) * 13.0f * ((k / 30) % 2 ? -1 : 1);
    x = clampf(x, L + BALL_R + 1, R - BALL_R - 1);
    bool ok = true;
    for (int i = 1; i < NB && ok; i++)
      if (b[i].on && hypotf(b[i].x - x, b[i].y - y) < 2 * BALL_R + 1)
        ok = false;
    if (ok)
    {
      c.x = x;
      c.y = y;
      return;
    }
  }
}

int Game4::left() const
{
  int n = 0;
  for (int i = 1; i < NB; i++)
    if (b[i].on)
      n++;
  return n;
}

void Game4::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  strokes = 0;
  rack();
}

void Game4::newGame()
{
  mode = Mode::Play;
  modeT = 0;
  paused = false;
  strokes = 0;
  msgT = 0;
  autoPlay = false;
  cheated = false;
  rack();
  sfx(Sfx::Rack);
}

void Game4::debugClear()
{
  if (phase != Phase::Aim)
    return;
  cheated = true;
  bool kept = false;
  for (int i = 1; i < NB; i++)
    if (b[i].on)
    {
      if (kept)
        b[i].on = false;
      kept = true;
    }
}

// ---- 動き ----
void Game4::physics()
{
  const int SUB = 8;
  const float dt = 1.0f / FPS / SUB;
  for (int s = 0; s < SUB; s++)
  {
    for (int i = 0; i < NB; i++)
    {
      Ball &o = b[i];
      if (!o.on || o.fall)
        continue;
      o.x += o.vx * dt;
      o.y += o.vy * dt;
      // クッション (ポケットの口のところには無い)
      float hit = 0;
      if (o.x > L + MOUTH && o.x < R - MOUTH)
      {
        if (o.y < T + BALL_R && o.vy < 0)
          hit = -o.vy, o.y = 2 * (T + BALL_R) - o.y, o.vy = -o.vy * E_CUSH, o.vx *= 0.97f;
        else if (o.y > B - BALL_R && o.vy > 0)
          hit = o.vy, o.y = 2 * (B - BALL_R) - o.y, o.vy = -o.vy * E_CUSH, o.vx *= 0.97f;
      }
      if (o.y > T + MOUTH && o.y < B - MOUTH)
      {
        if (o.x < L + BALL_R && o.vx < 0)
          hit = -o.vx, o.x = 2 * (L + BALL_R) - o.x, o.vx = -o.vx * E_CUSH, o.vy *= 0.97f;
        else if (o.x > R - BALL_R && o.vx > 0)
          hit = o.vx, o.x = 2 * (R - BALL_R) - o.x, o.vx = -o.vx * E_CUSH, o.vy *= 0.97f;
      }
      if (hit > 20)
        sfx(Sfx::Cushion, clampf(hit / 500, 0.1f, 1));
      // ポケットの角 (クッションの端)
      for (int k = 0; k < 4; k++)
      {
        float cx = (k & 1) ? R : L, cy = (k & 2) ? B : T, sx = (k & 1) ? -1 : 1, sy = (k & 2) ? -1 : 1;
        Pt jaw[2] = {{cx + sx * MOUTH, cy}, {cx, cy + sy * MOUTH}};
        for (const Pt &j : jaw)
        {
          float dx = o.x - j.x, dy = o.y - j.y, d = hypotf(dx, dy);
          if (d < BALL_R + JAW_R && d > 0.001f)
          {
            float nx = dx / d, ny = dy / d;
            o.x = j.x + nx * (BALL_R + JAW_R);
            o.y = j.y + ny * (BALL_R + JAW_R);
            float vn = o.vx * nx + o.vy * ny;
            if (vn < 0)
            {
              o.vx -= (1 + E_JAW) * vn * nx;
              o.vy -= (1 + E_JAW) * vn * ny;
              if (-vn > 20)
                sfx(Sfx::Cushion, clampf(-vn / 500, 0.1f, 1));
            }
          }
        }
        // 口の中でクッションの線を越えたか、ポケットの中心に近づいたら落ちる
        float ix = (o.x - cx) * sx, iy = (o.y - cy) * sy;
        if ((ix < MOUTH && iy < MOUTH && (ix < 1 || iy < 1)) || hypotf(o.x - PX[k], o.y - PY[k]) < CAP)
        {
          o.fall = 1;
          o.pocket = k;
          o.fx = o.x;
          o.fy = o.y;
          float v = hypotf(o.vx, o.vy);
          sfx(Sfx::Pocket, clampf(0.5f + v / 800, 0.5f, 1));
          if (i > 0)
            potThisShot++;
          break;
        }
      }
    }
    // 玉どうし
    for (int i = 0; i < NB; i++)
    {
      Ball &p = b[i];
      if (!p.on || p.fall)
        continue;
      for (int j = i + 1; j < NB; j++)
      {
        Ball &q = b[j];
        if (!q.on || q.fall)
          continue;
        float dx = q.x - p.x, dy = q.y - p.y, d2 = dx * dx + dy * dy;
        if (d2 >= 4 * BALL_R * BALL_R || d2 < 1e-6f)
          continue;
        float d = sqrtf(d2), nx = dx / d, ny = dy / d, ov = (2 * BALL_R - d) / 2;
        p.x -= nx * ov;
        p.y -= ny * ov;
        q.x += nx * ov;
        q.y += ny * ov;
        float rel = (p.vx - q.vx) * nx + (p.vy - q.vy) * ny;
        if (rel > 0)
        {
          float imp = (1 + E_BALL) / 2 * rel;
          p.vx -= imp * nx;
          p.vy -= imp * ny;
          q.vx += imp * nx;
          q.vy += imp * ny;
          if (rel > 12)
            sfx(Sfx::BallClick, clampf(rel / 450, 0.08f, 1));
        }
      }
    }
    // 転がる抵抗
    for (int i = 0; i < NB; i++)
    {
      Ball &o = b[i];
      if (!o.on || o.fall)
        continue;
      float v = hypotf(o.vx, o.vy);
      if (v < 1e-3f)
        continue;
      float nv = v - DECEL * dt - v * DRAG * dt;
      if (nv < STOP_V)
        o.vx = o.vy = 0;
      else
      {
        o.vx *= nv / v;
        o.vy *= nv / v;
      }
    }
  }
  // 落ちる途中の玉
  for (int i = 0; i < NB; i++)
  {
    Ball &o = b[i];
    if (o.on && o.fall && ++o.fall > FALL_N)
    {
      o.on = false;
      o.fall = 0;
      if (i == 0)
        scratched = true;
    }
  }
}

// 打ちおえて全部止まった
void Game4::endShot()
{
  if (scratched)
  {
    scratched = false;
    strokes++; // 手玉を落としたら 1 打足す
    respotCue();
    msgT = 2 * FPS;
    msgKind = 1;
    sfx(Sfx::Fail, 0.6f);
  }
  else if (potThisShot >= 2)
  {
    msgT = 2 * FPS;
    msgKind = 2;
  }
  potThisShot = 0;
  phase = Phase::Aim;
  aiming = false;
  aimP = 0;
  aiT = 0;
  if (left() == 0)
  {
    if (mode == Mode::Play)
    {
      mode = Mode::Clear;
      modeT = 0;
      sfx(Sfx::Clear);
    }
    else
    {
      strokes = 0;
      rack();
      sfx(Sfx::Rack);
    }
  }
  else if (mode == Mode::Attract && strokes >= 40)
  {
    strokes = 0;
    rack();
  }
}

void Game4::strike(float dx, float dy, float p)
{
  shotDx = dx;
  shotDy = dy;
  shotP = p;
  strikeT = 0;
  phase = Phase::Strike;
  aiming = false;
}

// ---- 狙う: 倒した向きの反対へ、倒した量の強さで。はね戻った瞬間か、倒したまま SW1 を押すと打つ ----
void Game4::stepAimInput(const Input &in)
{
  float sx = in.AX, sy = in.AY; // 右・下が正 (画面と同じ)
  float m = hypotf(sx, sy);
  if (m > 1)
  {
    sx /= m;
    sy /= m;
    m = 1;
  }
  // SW1 で打ったあとは、スティックがいったん中央へ戻るまで構えない (倒したまま待っていて、戻したときに打たないように)
  if (needCenter)
  {
    if (m < 0.1f)
      needCenter = false;
    prevM = m;
    return;
  }
  if (!aiming && m >= P_MIN)
  {
    aiming = true;
    aimP = 0;
    heldAge = 0;
    smx = sx;
    smy = sy;
  }
  // 向きは少しなめらかにする (細かな手のぶれを消す)
  smx += (sx - smx) * 0.5f;
  smy += (sy - smy) * 0.5f;
  if (aiming)
  {
    // 引いているとき・止めているときは覚える。急に戻っている途中 (はね戻り) は覚えない
    if (m >= aimP - 0.02f || fabsf(m - prevM) < 0.08f)
    {
      float sm = hypotf(smx, smy);
      if (sm > 0.05f)
      {
        aimDx = -smx / sm;
        aimDy = -smy / sm;
      }
      aimP = m;
      heldAge = 0;
    }
    else
      heldAge++;
    if (in.FirePressed && aimP >= P_MIN) // SW1: いま構えている向きと強さで打つ
    {
      strike(aimDx, aimDy, aimP);
      needCenter = true;
    }
    else if (m < 0.1f)
    {
      if (heldAge <= 3 && aimP >= P_MIN)
        strike(aimDx, aimDy, aimP);
      aiming = false;
    }
  }
  prevM = m;
}

// 点 (cx, cy) から線分までの近さ
static float segDist(float x0, float y0, float x1, float y1, float cx, float cy)
{
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
  float u = l2 > 0 ? clampf(((cx - x0) * dx + (cy - y0) * dy) / l2, 0, 1) : 0;
  return hypotf(x0 + dx * u - cx, y0 + dy * u - cy);
}

bool Game4::pathClear(float x0, float y0, float x1, float y1, int skipA, int skipB) const
{
  for (int i = 0; i < NB; i++)
    if (i != skipA && i != skipB && b[i].on && !b[i].fall && segDist(x0, y0, x1, y1, b[i].x, b[i].y) < 2 * BALL_R)
      return false;
  return true;
}

// 自動で打つ: 落としやすい玉とポケットを選ぶ
bool Game4::plan(float &dx, float &dy, float &p)
{
  const Ball &c = b[0];
  if (left() == 10 && strokes == 0) // ブレイク
  {
    dx = (rnd() - 0.5f) * 0.03f;
    dy = -1;
    p = 1;
    return true;
  }
  float best = -1e9f;
  for (int i = 1; i < NB; i++)
  {
    if (!b[i].on)
      continue;
    for (int k = 0; k < 4; k++)
    {
      float tx = PX[k] - b[i].x, ty = PY[k] - b[i].y, bp = hypotf(tx, ty);
      float ux = tx / bp, uy = ty / bp;
      float gx = b[i].x - ux * 2 * BALL_R, gy = b[i].y - uy * 2 * BALL_R; // 当てる瞬間の手玉の位置
      if (gx < L + BALL_R || gx > R - BALL_R || gy < T + BALL_R || gy > B - BALL_R)
        continue;
      float cx = gx - c.x, cy = gy - c.y, dc = hypotf(cx, cy);
      if (dc < 1)
        continue;
      float vx = cx / dc, vy = cy / dc, cosv = vx * ux + vy * uy;
      if (cosv < 0.3f)
        continue;
      if (!pathClear(c.x, c.y, gx, gy, 0, i) || !pathClear(b[i].x, b[i].y, PX[k], PY[k], i, 0))
        continue;
      float sc = cosv * 2 - (dc + bp) / 500;
      if (sc > best)
      {
        best = sc;
        dx = vx;
        dy = vy;
        float vObj = sqrtf(2 * DECEL * bp) * 1.25f + 60;
        float vCue = vObj / (cosv * 0.97f) * 1.1f + sqrtf(2 * DECEL * dc);
        p = powerFor(vCue);
      }
    }
  }
  if (best > -1e9f)
    return true;
  // 落とせる筋が無い: いちばん近い玉に強めに当てる
  float nd = 1e9f;
  for (int i = 1; i < NB; i++)
    if (b[i].on)
    {
      float d = hypotf(b[i].x - c.x, b[i].y - c.y);
      if (d < nd)
      {
        nd = d;
        dx = (b[i].x - c.x) / d;
        dy = (b[i].y - c.y) / d;
      }
    }
  p = 0.75f;
  return nd < 1e9f;
}

void Game4::stepAI()
{
  aiT++;
  if (aiT == 1)
  {
    if (!plan(aiDx, aiDy, aiP))
      aiDx = 0, aiDy = -1, aiP = 0.6f;
    float er = (rnd() - 0.5f) * (mode == Mode::Attract ? 0.025f : 0.0f); // デモは少し外す
    float c = cosf(er), s = sinf(er);
    float x = aiDx * c - aiDy * s, y = aiDx * s + aiDy * c;
    aiDx = x;
    aiDy = y;
    aimDx = aiDx;
    aimDy = aiDy;
    aiming = true;
    aimP = 0;
  }
  if (aiT <= 30)
    aimP = aiP * std::min(1.0f, aiT / 22.0f);
  if (aiT == 44)
    strike(aiDx, aiDy, aiP);
}

void Game4::stepTable(const Input *in)
{
  switch (phase)
  {
  case Phase::Aim:
    if (in)
      stepAimInput(*in);
    else
      stepAI();
    break;
  case Phase::Strike:
    if (++strikeT >= 3)
    {
      float v = speedOf(shotP);
      b[0].vx = shotDx * v;
      b[0].vy = shotDy * v;
      strokes++;
      potThisShot = 0;
      phase = Phase::Roll;
      sfx(Sfx::CueHit, 0.35f + 0.65f * v / V_MAX);
    }
    break;
  case Phase::Roll:
  {
    physics();
    bool moving = false;
    for (int i = 0; i < NB; i++)
      if (b[i].on && (b[i].fall || b[i].vx != 0 || b[i].vy != 0))
        moving = true;
    if (!moving && !(scratched && b[0].on)) // 手玉が落ちきるまで待つ
      endShot();
    break;
  }
  }
}

// ---- 名前とランキング (打数は少ないほど上) ----
int Game4::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s < Table[i].Score)
      return i;
  return -1;
}

void Game4::stepEntry(const Input &in)
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

void Game4::finishEntry()
{
  char name[4];
  memcpy(name, entry, 4);
  for (int i = 2; i >= 0 && name[i] == ' '; i--)
    name[i] = 0;
  if (!name[0])
    strcpy(name, "---");
  memcpy(LastName, entry, 4);
  int i = rankOf(strokes);
  if (i >= 0)
  {
    for (int j = 4; j > i; j--)
      Table[j] = Table[j - 1];
    Table[i].Score = strokes;
    memcpy(Table[i].Name, name, 4);
    newRank = i;
    if (TableChanged)
      TableChanged();
  }
  mode = Mode::Ranking;
  modeT = 8 * FPS;
}

// ---- 1 フレーム ----
void Game4::Step(const Input &in)
{
  sndN = 0;
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
    if (in.FirePressed || in.StartPressed)
    {
      newGame();
      return;
    }
    modeT++;
    if (msgT > 0)
      msgT--;
    stepTable(nullptr);
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
    if (msgT > 0)
      msgT--;
    stepTable(autoPlay ? nullptr : &in);
    break;
  case Mode::Clear:
    modeT++;
    if ((modeT > 2 * FPS && in.FirePressed) || modeT > 10 * FPS)
    {
      if (!cheated && !autoPlay && rankOf(strokes) >= 0)
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
// 玉: 左上から光が当たった球 (縁はなめらかに、つやの点つき)。dark は 0〜1 で黒へ寄せる (ポケットに落ちる途中)
static void drawBall(float cx, float cy, float r, uint32_t col, float dark)
{
  Surf *s = d2::S;
  int y0 = std::max((int)floorf(cy - r - 1), s->y0), y1 = std::min((int)ceilf(cy + r + 1), s->y0 + s->h - 1);
  int x0 = std::max((int)floorf(cx - r - 1), 0), x1 = std::min((int)ceilf(cx + r + 1), W - 1);
  if (y0 > y1 || x0 > x1)
    return;
  float cr = (col >> 16) & 255, cg = (col >> 8) & 255, cb = col & 255, inv = 1 / r, lit = 1 - dark;
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
      float lam = -0.45f * nx - 0.55f * ny + 0.70f * nz;
      float sh = 0.32f + 0.85f * std::max(0.0f, lam), sp = 0;
      if (lam > 0)
      {
        float l2 = lam * lam, l4 = l2 * l2, l8 = l4 * l4, l16 = l8 * l8;
        sp = l16 * l4 * 0.95f;
      }
      sh += 0.1f * std::max(0.0f, ny) * nz; // 下からの羅紗の照り返し
      int rr = clampf((cr * sh + 255 * sp) * lit, 0, 255), gg = clampf((cg * sh + 255 * sp + 10 * std::max(0.0f, ny)) * lit, 0, 255),
          bb = clampf((cb * sh + 255 * sp) * lit, 0, 255);
      uint32_t c = (rr << 16) | (gg << 8) | bb;
      row[x] = cov >= 0.99f ? d2::c565(c) : d2::blend(row[x], c, (int)(cov * 255));
    }
  }
}

void Game4::drawBalls(Gfx &g)
{
  // 落ちる途中の玉 (穴の中なので、ほかの玉より先に)
  for (int i = 0; i < NB; i++)
  {
    const Ball &o = b[i];
    if (!o.on || !o.fall)
      continue;
    float k = o.fall / (float)FALL_N, e = k * (2 - k);
    float x = o.fx + (PX[o.pocket] - o.fx) * e, y = o.fy + (PY[o.pocket] - o.fy) * e;
    drawBall(x, y, BALL_R * (1 - 0.45f * k), BALL_COL[i], 0.85f * k);
  }
  // 影
  for (int i = 0; i < NB; i++)
    if (b[i].on && !b[i].fall)
      d2::C(b[i].x + 1.5f, b[i].y + 2.5f, BALL_R, 0x000000, 80);
  for (int i = 0; i < NB; i++)
    if (b[i].on && !b[i].fall)
    {
      drawBall(b[i].x, b[i].y, BALL_R, BALL_COL[i], 0);
      if (i == 8) // 黒い玉には白い丸
        d2::C(b[i].x - 0.5f, b[i].y - 0.5f, 1.6f, 0xE8E4D8, 220);
    }
}

// 手玉から打つ向きへ光線を飛ばし、最初に当たるもの (玉か、クッション) までの長さ
float Game4::cast(float ox, float oy, float dx, float dy, int &hit) const
{
  hit = -1;
  float tm = 1e9f;
  if (dx > 1e-4f)
    tm = std::min(tm, (R - BALL_R - ox) / dx);
  else if (dx < -1e-4f)
    tm = std::min(tm, (L + BALL_R - ox) / dx);
  if (dy > 1e-4f)
    tm = std::min(tm, (B - BALL_R - oy) / dy);
  else if (dy < -1e-4f)
    tm = std::min(tm, (T + BALL_R - oy) / dy);
  for (int i = 1; i < NB; i++)
  {
    if (!b[i].on || b[i].fall)
      continue;
    float fx = b[i].x - ox, fy = b[i].y - oy, pb = fx * dx + fy * dy, c = fx * fx + fy * fy - 4 * BALL_R * BALL_R;
    float disc = pb * pb - c;
    if (pb <= 0 || disc <= 0)
      continue;
    float t0 = pb - sqrtf(disc);
    if (t0 > 0 && t0 < tm)
    {
      tm = t0;
      hit = i;
    }
  }
  return std::max(0.0f, tm);
}

static void dotted(float x0, float y0, float x1, float y1, uint32_t c, int a, float step = 4)
{
  float len = hypotf(x1 - x0, y1 - y0);
  if (len < 1)
    return;
  float ux = (x1 - x0) / len, uy = (y1 - y0) / len;
  for (float s = 0; s <= len; s += step)
    d2::P(x0 + ux * s, y0 + uy * s, c, a);
}

void Game4::drawGuide()
{
  bool show = (phase == Phase::Aim && aiming) || phase == Phase::Strike;
  if (!show || !b[0].on)
    return;
  float dx = phase == Phase::Strike ? shotDx : aimDx, dy = phase == Phase::Strike ? shotDy : aimDy;
  float p = phase == Phase::Strike ? shotP : aimP;
  int a = p >= P_MIN ? 200 : 70; // 弱すぎて打てないときは薄く
  const Ball &c = b[0];
  int hit;
  float tm = cast(c.x, c.y, dx, dy, hit);
  float hx = c.x + dx * tm, hy = c.y + dy * tm;
  dotted(c.x + dx * (BALL_R + 2), c.y + dy * (BALL_R + 2), hx, hy, 0xF4F4EC, a);
  d2::CR(hx, hy, BALL_R, 0xF4F4EC, a * 2 / 3); // 当たる瞬間の手玉
  if (hit > 0)
  {
    float nx = (b[hit].x - hx) / (2 * BALL_R), ny = (b[hit].y - hy) / (2 * BALL_R);
    float dot = dx * nx + dy * ny;
    // 的玉の進む向き (まっすぐ当てるほど長い)
    float ol = 14 + 34 * dot;
    d2::L(b[hit].x + nx * (BALL_R + 1), b[hit].y + ny * (BALL_R + 1), b[hit].x + nx * (BALL_R + ol), b[hit].y + ny * (BALL_R + ol), GOLD, a);
    // 手玉のそれる向き
    float tx = dx - dot * nx, ty = dy - dot * ny, tl = hypotf(tx, ty);
    if (tl > 0.05f)
      dotted(hx, hy, hx + tx / tl * 22 * tl, hy + ty / tl * 22 * tl, 0xF4F4EC, a / 2, 3);
  }
  else
  {
    // クッションで返る向きを少しだけ
    float rx = dx, ry = dy;
    if (hx <= L + BALL_R + 0.5f || hx >= R - BALL_R - 0.5f)
      rx = -rx;
    if (hy <= T + BALL_R + 0.5f || hy >= B - BALL_R - 0.5f)
      ry = -ry;
    dotted(hx, hy, hx + rx * 26, hy + ry * 26, 0xF4F4EC, a / 2);
  }
}

void Game4::drawCue()
{
  if (!b[0].on || !(phase == Phase::Strike || (phase == Phase::Aim && aiming)))
    return;
  float dx = phase == Phase::Strike ? shotDx : aimDx, dy = phase == Phase::Strike ? shotDy : aimDy;
  float p = phase == Phase::Strike ? shotP : aimP;
  float pull = 3 + p * 30;
  if (phase == Phase::Strike)
    pull *= 1 - std::min(1.0f, (strikeT + 1) / 3.0f);
  const Ball &c = b[0];
  // キューは手玉の後ろ (打つ向きの反対) に、引いた分だけ離して置く
  float tx = c.x - dx * (BALL_R + 1 + pull), ty = c.y - dy * (BALL_R + 1 + pull);
  float ex = tx - dx * 120, ey = ty - dy * 120;
  float mx = tx - dx * 70, my = ty - dy * 70;
  d2::LT(tx + 2, ty + 3, ex + 2, ey + 3, 3, 0x000000, 70);   // 影
  d2::LT(tx, ty, mx, my, 2, 0xD8B07A);                       // 先のほう (明るい木)
  d2::L(tx - dy * 0.5f, ty + dx * 0.5f, mx - dy * 0.5f, my + dx * 0.5f, 0xF0D4A0);
  d2::LT(mx, my, ex, ey, 3, 0x3A1E10);                       // 握り (濃い木)
  d2::L(mx - dx * 2, my - dy * 2, mx - dx * 4, my - dy * 4, 0xE8E0C8);
  d2::LT(tx, ty, tx - dx * 3, ty - dy * 3, 2, 0x7AA0D0);     // 先のチョーク
}

static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, 1, INK);
}
static void panel(int x, int y, int w, int h)
{
  d2::R(x, y, w, h, 0x08100C, 210);
  d2::R(x, y, w, 1, GOLD);
  d2::R(x, y + h - 1, w, 1, 0x5A4A2A);
  d2::R(x, y, 1, h, 0x5A4A2A);
  d2::R(x + w - 1, y, 1, h, 0x5A4A2A);
}

void Game4::drawHud(Gfx &g)
{
  char s[24];
  snprintf(s, sizeof(s), "SHOTS %d", strokes);
  txt(g, 5, 7, CREAM, s);
  snprintf(s, sizeof(s), "BEST %d", Table[0].Score);
  txt(g, W - 5 - (int)strlen(s) * 6, 7, MUTE, s);
  // 残っている玉
  for (int i = 1; i < NB; i++)
  {
    float x = 72 + (i - 1) * 10, y = 10.5f;
    if (b[i].on && !b[i].fall)
      drawBall(x, y, 3.5f, BALL_COL[i], 0);
    else
      d2::CR(x, y, 3, 0x3A4248);
  }
  // 強さの目盛り (右の木のふち)
  if ((phase == Phase::Aim && aiming) || phase == Phase::Strike)
  {
    float p = phase == Phase::Strike ? shotP : aimP;
    const int gx = W - 12, gy0 = 64, gh = 210;
    d2::R(gx - 1, gy0 - 1, 7, gh + 2, 0x0A0604);
    int fill = (int)(gh * p);
    for (int y = 0; y < fill; y++)
    {
      float k = y / (float)gh;
      uint32_t c = k < 0.5f ? mix(0x40C060, 0xF0D040, k * 2) : mix(0xF0D040, 0xF04030, (k - 0.5f) * 2);
      if (p < P_MIN)
        c = 0x606868;
      d2::R(gx, gy0 + gh - 1 - y, 5, 1, c);
    }
    d2::R(gx - 3, gy0 + gh - (int)(gh * P_MIN), 9, 1, 0xE8E0C8, 160); // ここより弱いと打たない
  }
}

void Game4::drawOverlay(Gfx &g)
{
  char s[40];
  if (volT > 0)
  {
    panel(70, 262, 100, 22);
    txt(g, 78, 270, CREAM, VolumeLevel == 0 ? "VOL OFF" : "VOL");
    for (int i = 0; i < 4; i++)
      d2::R(110 + i * 13, 270, 10, 8, i < VolumeLevel ? GOLD : 0x2A2420);
  }
  if (msgT > 0 && (mode == Mode::Play || mode == Mode::Attract))
  {
    if (msgKind == 1)
      txt(g, W / 2, 150, REDT, "SCRATCH +1", true, 3);
    else if (msgKind == 2)
      txt(g, W / 2, 150, GOLD, "NICE!", true, 3);
  }
  switch (mode)
  {
  case Mode::Attract:
  {
    int ph = (modeT / (8 * FPS)) % 3;
    if (ph == 0)
    {
      panel(24, 84, 192, 128);
      txt(g, W / 2, 98, GOLD, "CORNERS", true, 5);
      txt(g, W / 2, 128, CREAM, "SINK ALL 10 BALLS", true);
      txt(g, W / 2, 142, MUTE, "CORNER POCKETS ONLY", true);
      if ((modeT / 15) % 2 == 0)
        txt(g, W / 2, 164, CREAM, "PUSH SW1", true, 3);
      txt(g, W / 2, 196, MUTE, "SW3 HOLD: MENU", true);
    }
    else if (ph == 1)
    {
      if ((modeT / 15) % 2 == 0)
        txt(g, W / 2, 280, CREAM, "PUSH SW1", true);
    }
    else
    {
      panel(30, 80, 180, 140);
      txt(g, W / 2, 94, GOLD, "FEWEST SHOTS", true, 3);
      static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
      for (int i = 0; i < 5; i++)
      {
        snprintf(s, sizeof(s), "%s  %3d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
        txt(g, W / 2, 122 + i * 18, i == 0 ? GOLD : CREAM, s, true);
      }
    }
    break;
  }
  case Mode::Play:
    if (strokes == 0 && !aiming && phase == Phase::Aim)
    {
      txt(g, W / 2, 262, CREAM, "PULL THE STICK BACK", true);
      txt(g, W / 2, 274, CREAM, "AND LET GO (OR SW1)", true);
    }
    if (autoPlay)
      txt(g, 5, 26, MUTE, "AUTO");
    if (paused)
    {
      panel(40, 120, 160, 64);
      txt(g, W / 2, 132, CREAM, "PAUSE", true, 3);
      txt(g, W / 2, 158, MUTE, "SW3: RESUME", true);
      txt(g, W / 2, 170, MUTE, "SW2: QUIT", true);
    }
    break;
  case Mode::Clear:
    panel(30, 100, 180, 100);
    txt(g, W / 2, 114, GOLD, "TABLE CLEAR", true, 3);
    snprintf(s, sizeof(s), "%d SHOTS", strokes);
    txt(g, W / 2, 140, CREAM, s, true, 3);
    if (cheated || autoPlay)
      txt(g, W / 2, 166, MUTE, "TEST PLAY (NO RECORD)", true);
    else if (strokes < Table[0].Score)
      txt(g, W / 2, 166, REDT, "NEW BEST!", true);
    if (modeT > 2 * FPS && (modeT / 15) % 2 == 0)
      txt(g, W / 2, 182, MUTE, "SW1: NEXT", true);
    break;
  case Mode::NameEntry:
  {
    panel(30, 90, 180, 120);
    txt(g, W / 2, 102, GOLD, "NEW RECORD", true, 3);
    snprintf(s, sizeof(s), "%d SHOTS", strokes);
    txt(g, W / 2, 124, CREAM, s, true);
    for (int i = 0; i < 3; i++)
    {
      char c[2] = {entry[i], 0};
      int x = W / 2 - 30 + i * 22;
      txt(g, x, 146, i == entryPos ? GOLD : CREAM, c, true, 5);
      if (i == entryPos && (t / 8) % 2)
        d2::R(x - 8, 172, 16, 2, GOLD);
    }
    txt(g, W / 2, 186, MUTE, "STICK: LETTER  SW1: OK", true);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, 196, 100, MUTE, s);
    break;
  }
  case Mode::Ranking:
  {
    panel(30, 80, 180, 150);
    txt(g, W / 2, 94, GOLD, "FEWEST SHOTS", true, 3);
    static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
    for (int i = 0; i < 5; i++)
    {
      snprintf(s, sizeof(s), "%s  %3d  %-3s", ORD[i], Table[i].Score, Table[i].Name);
      bool mine = i == newRank;
      if (!mine || (t / 8) % 2)
        txt(g, W / 2, 122 + i * 18, mine ? REDT : i == 0 ? GOLD : CREAM, s, true);
    }
    snprintf(s, sizeof(s), "YOU  %d SHOTS", strokes);
    txt(g, W / 2, 214, MUTE, s, true);
    break;
  }
  }
}

void Game4::Draw(Gfx &gx)
{
  uint32_t t0 = micros();
  int ltop = gx.top * 2 / 3;
  memcpy(gx.buf, bg + ltop * W, W * LSTRIP * 2);
  Surf s;
  s.buf = gx.buf;
  s.y0 = ltop;
  s.h = LSTRIP;
  d2::S = &s;
  drawBalls(gx);
  drawGuide();
  drawCue();
  drawHud(gx);
  drawOverlay(gx);
  profDraw += micros() - t0;
}
