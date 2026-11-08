// 2 本目 (廃工場の多脚戦車)。動きはデザイン案のページ (foundry-design.html) と同じ。遊びの流れは SKYLARK (game.cpp) にそろえる
#include "game2.h"
#include "g2bg.h"
#include "g2draw.h"
#include <algorithm>

using namespace d2;

static const int W = 240, H = 320;
static const float SPEED = 0.55f; // 背景が流れる速さ (点/フレーム)
static const float HIPS[6][2] = {{-8, -8}, {8, -8}, {-10, 0}, {10, 0}, {-8, 8}, {8, 8}};
static const float REST[6][2] = {{-19, -18}, {19, -18}, {-23, 1}, {23, 1}, {-19, 18}, {19, 18}};
static const int GRP[6] = {0, 1, 1, 0, 0, 1}; // 0: 左前・右中・左後 / 1: 右前・左中・右後 (3 本ずつ交互に上げる)
static const uint32_t RED = 0xff3b2a;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
static const int ENTER_FRAMES = 40, INV_FRAMES = 100, DEAD_FRAMES = 50;

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

bool Game2::begin()
{
  uint32_t t0 = millis();
  bool ok = bg2::build();
  buildMs = millis() - t0;
  startDemo();
  return ok;
}

void Game2::initFeet()
{
  int sc = (int)scroll;
  for (int i = 0; i < 6; i++)
    feet[i] = {tx + REST[i][0], ty - sc + REST[i][1], 0, 0, -1};
}

// ---- 流れ ----
void Game2::resetWorld()
{
  scroll = 0;
  t = 0;
  shots.clear();
  eb.clear();
  en.clear();
  fx.clear();
  lastZone = -1;
  bannerT = 0;
  extendT = 0;
  deadT = invT = enterT = 0;
  fireCool = 0;
  ang = -PIf / 2;
}

void Game2::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  resetWorld();
  score = 0;
  tankOn = true;
  tx = W / 2;
  ty = 240;
  initFeet();
}

void Game2::newGame()
{
  mode = Mode::Play;
  paused = false;
  resetWorld();
  score = 0;
  lives = 2;
  nextExtend = 40000;
  continues = 0;
  enterTank();
}

void Game2::enterTank()
{
  tankOn = true;
  tx = W / 2;
  ty = H + 30;
  enterT = ENTER_FRAMES;
  invT = ENTER_FRAMES + INV_FRAMES;
  ang = -PIf / 2;
  eb.clear();
  initFeet();
}

void Game2::killTank()
{
  tankOn = false;
  deadT = DEAD_FRAMES;
  fx.push_back({2, tx, ty, hs(t) * 6, 0, 40, true});
  for (int i = 0; i < 4; i++)
    fx.push_back({0, tx + (hs(t + i) - 0.5f) * 30, ty + (hs(t * 2 + i) - 0.5f) * 26, hs(t + i * 3) * 6, -i * 5, 26, true});
}

void Game2::endGame()
{
  mode = Mode::GameOver;
  modeT = 4 * FPS;
  tankOn = false;
}

int Game2::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

// 名前の 3 文字: 上下で文字を選び、SW1 で決めて次の文字へ (左か SW2 で戻る)
void Game2::stepEntry(const Input &in)
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
    int n = strlen(LETTERS), k = strchr(LETTERS, entry[entryPos]) - LETTERS;
    entry[entryPos] = LETTERS[(k + dir + n) % n];
  }
  if (in.LeftPressed || in.BackPressed)
    entryPos = max(0, entryPos - 1);
  if (in.RightPressed && entryPos < 2)
    entryPos++;
  if (in.FirePressed)
  {
    if (entryPos < 2)
      entryPos++;
    else
      finishEntry();
  }
}

void Game2::finishEntry()
{
  char name[4];
  memcpy(name, entry, 4);
  for (int i = 2; i >= 0 && name[i] == ' '; i--)
    name[i] = 0;
  if (!name[0])
    strcpy(name, "---");
  memcpy(LastName, entry, 4);
  int i = rankOf(score);
  if (i >= 0)
  {
    for (int j = 4; j > i; j--)
      Table[j] = Table[j - 1];
    Table[i].Score = score;
    memcpy(Table[i].Name, name, 4);
    newRank = i;
    if (TableChanged)
      TableChanged();
  }
  mode = Mode::Ranking;
  modeT = 8 * FPS;
}

void Game2::Step(const Input &in)
{
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
    stepWorld(nullptr);
    break;
  case Mode::Play:
    if (in.PausePressed || (paused && in.VolumePressed)) // 長押しで止める。止めているあいだは短く押しても戻る
      paused = !paused;
    if (paused)
    {
      if (in.QuitPressed)
        startDemo();
      return;
    }
    stepWorld(&in);
    break;
  case Mode::Continue:
    if (in.FirePressed || in.StartPressed)
    {
      mode = Mode::Play;
      lives = 2;
      continues++;
      if (score % 10 < 9) // 点のいちばん下の桁は、コンティニューした回数
        score++;
      enterTank();
      return;
    }
    if (in.PausePressed || in.QuitPressed) // やめる
      modeT = 1;
    if (--modeT <= 0)
      endGame();
    break;
  case Mode::GameOver:
    stepWorld(nullptr);
    if (--modeT <= 0 || (modeT < 3 * FPS && (in.FirePressed || in.StartPressed)))
    {
      if (rankOf(score) >= 0)
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
    stepWorld(nullptr);
    stepEntry(in);
    break;
  case Mode::Ranking:
    stepWorld(nullptr);
    if (--modeT <= 0 || (modeT < 7 * FPS && (in.FirePressed || in.StartPressed)))
      startDemo();
    break;
  }
}

// 世界を 1 フレーム進める。戦車は、遊んでいるときは in で、デモでは自動で動かす
void Game2::stepWorld(const Input *in)
{
  t++;
  scroll += SPEED;
  if (tankOn)
  {
    if (mode == Mode::Attract)
    {
      float gx = 120 + 48 * sinf(t * 0.011f) + 14 * sinf(t * 0.029f), gy = 236 + 12 * sinf(t * 0.017f);
      stepTank(clampf((gx - tx) / 2, -1, 1), clampf((gy - ty) / 2, -1, 1), true, false);
    }
    else if (enterT > 0)
    {
      enterT--;
      stepTank(0, -1, false, false);
    }
    else if (in)
      stepTank(in->AX, in->AY, in->Fire, in->Slow);
    else
      stepTank(0, 0, false, false);
  }
  if (invT > 0)
    invT--;
  if (deadT > 0 && --deadT == 0 && mode == Mode::Play)
  {
    if (lives > 0)
    {
      lives--;
      enterTank();
    }
    else
    {
      mode = Mode::Continue;
      modeT = 10 * FPS - 1;
    }
  }

  enemyStep();

  if (mode == Mode::Play && tankOn && invT == 0)
  {
    bool hit = false;
    for (auto &b : eb)
      if (!b.dead && fabsf(b.x - tx) < 5 && fabsf(b.y - ty) < 7)
      {
        b.dead = true;
        hit = true;
      }
    for (auto &e : en)
      if (e.k == 1 && fabsf(e.x - tx) < 14 && fabsf(e.y - ty) < 14)
        hit = true;
    if (hit)
      killTank();
  }
  if (mode == Mode::Play && score >= nextExtend)
  {
    lives++;
    nextExtend += 60000;
    extendT = 2 * FPS;
  }
  if (extendT > 0)
    extendT--;
  int z = zoneAt(H / 2);
  if (z != lastZone)
  {
    lastZone = z;
    if (mode == Mode::Play)
    {
      bannerZone = z;
      bannerT = 3 * FPS;
    }
  }
  if (bannerT > 0)
    bannerT--;
  eb.erase(std::remove_if(eb.begin(), eb.end(), [](const Shot &s) { return s.dead; }), eb.end());
  shots.erase(std::remove_if(shots.begin(), shots.end(), [](const Shot &s) { return s.dead; }), shots.end());
}

void Game2::stepTank(float ax, float ay, bool fire, bool slow)
{
  int sc = (int)scroll;
  float sp = slow ? 1.0f : 2.0f;
  float ntx = clampf(tx + ax * sp, 26, W - 26), nty = ty + ay * sp;
  if (enterT == 0)
    nty = clampf(nty, 110, H - 24);
  float vx = ntx - tx, vy = nty - ty - SPEED;
  tx = ntx;
  ty = nty;

  // 脚: 床に着いた足は床の位置 (y は流れる前の座標) のまま。離れすぎた組から 3 本ずつ踏み出す
  float bx = tx, by = ty - sc;
  bool stepping = false;
  for (auto &f : feet)
    if (f.p >= 0)
      stepping = true;
  if (!stepping)
  {
    float err[2] = {0, 0};
    for (int i = 0; i < 6; i++)
      err[GRP[i]] = max(err[GRP[i]], hypotf(bx + REST[i][0] - feet[i].x, by + REST[i][1] - feet[i].y));
    int g = err[0] >= err[1] ? 0 : 1;
    if (err[g] > 6)
      for (int i = 0; i < 6; i++)
        if (GRP[i] == g)
        {
          feet[i].p = 0;
          feet[i].sx = feet[i].x;
          feet[i].sy = feet[i].y;
        }
  }
  for (int i = 0; i < 6; i++)
  {
    Foot &f = feet[i];
    if (f.p < 0)
      continue;
    f.p = min(1.0f, f.p + 1 / 7.0f);
    float gx = bx + REST[i][0] + vx * 5, gy = by + REST[i][1] + vy * 5;
    float e = f.p * f.p * (3 - 2 * f.p);
    f.x = f.sx + (gx - f.sx) * e;
    f.y = f.sy + (gy - f.sy) * e;
    if (f.p >= 1)
      f.p = -1;
  }

  // 砲塔: いちばん近い前の敵へ向く (上から左右 0.6 まで)
  const Enemy *best = nullptr;
  float bd = 1e9f;
  for (auto &e : en)
  {
    if (e.y > ty - 10 || e.y < 0)
      continue;
    float d = hypotf(e.x - tx, e.y - ty);
    if (d < bd)
    {
      bd = d;
      best = &e;
    }
  }
  float ta = -PIf / 2;
  if (best)
    ta = clampf(atan2f(best->y - ty, best->x - tx), -PIf / 2 - 0.6f, -PIf / 2 + 0.6f);
  ang += (ta - ang) * 0.15f;

  if (fireCool > 0)
    fireCool--;
  if (fire && fireCool == 0 && ty < H - 10)
  {
    float dx = cosf(ang), dy = sinf(ang);
    for (float s : {-2.5f, 2.5f})
      shots.push_back({tx - dy * s + dx * 16, ty - 1 + dx * s + dy * 16, dx * 7, dy * 7, false});
    rec = 3;
    fireCool = 6;
  }
  rec = max(0.0f, rec - 0.6f);
}

void Game2::enemyStep()
{
  int sc = (int)scroll;
  if (t % 100 == 0) // 敵の出る間隔 (試作の仮の値)
    en.push_back({0, 30 + hs(t) * 180, 0, -16, 0, 3, t, 0, false});
  if (t % 140 == 50)
    en.push_back({1, 0, 40 + hs(t * 2.1f) * 160, -16, -16.0f - sc, 5, t, 0, false});
  for (auto &b : shots)
  {
    b.x += b.dx;
    b.y += b.dy;
    if (b.y < -10 || b.x < -10 || b.x > W + 10)
      b.dead = true;
  }
  for (auto &e : en)
  {
    int age = t - e.t0;
    if (e.k == 0)
    {
      e.y += 0.9f;
      e.x = e.bx + 26 * sinf(age * 0.03f);
    }
    else
    {
      e.walking = age % 120 < 70;
      if (e.walking)
        e.wy += 0.35f;
      e.y = e.wy + sc;
    }
    if (e.hit)
      e.hit--;
    bool fire = e.k == 0 ? age % 70 == 30 : age % 120 == 80;
    if (fire && tankOn && enterT == 0 && e.y > 0 && e.y < H - 60)
    {
      float a0 = atan2f(ty - e.y, tx - e.x), s = 2;
      if (e.k == 0)
        for (float d : {-0.35f, 0.0f, 0.35f})
          eb.push_back({e.x, e.y + 4, cosf(a0 + d) * s, sinf(a0 + d) * s, false});
      else
        for (float o : {-2.0f, 2.0f})
          eb.push_back({e.x + o, e.y + 8, cosf(a0) * s, sinf(a0) * s, false});
    }
  }
  for (auto &b : shots)
    for (auto &e : en)
    {
      if (b.dead || e.hp <= 0)
        continue;
      if (fabsf(b.x - e.x) < 10 && fabsf(b.y - e.y) < 10)
      {
        b.dead = true;
        e.hp--;
        e.hit = 3;
        if (e.hp <= 0)
        {
          if (mode == Mode::Play)
            score += e.k == 0 ? 100 : 200;
          fx.push_back({0, e.x, e.y, hs(t) * 6, 0, 26, e.k == 1});
          fx.push_back({1, e.x, e.y, hs(t + 1) * 6, 0, 40, false});
        }
      }
    }
  en.erase(std::remove_if(en.begin(), en.end(), [](const Enemy &e) { return e.hp <= 0 || e.y > H + 30; }), en.end());
  for (auto &b : eb)
  {
    b.x += b.dx;
    b.y += b.dy;
    if (b.y > H + 8 || b.y < -8 || b.x < -8 || b.x > W + 8)
      b.dead = true;
  }
  for (auto &f : fx)
  {
    f.t++;
    if (f.ground)
      f.y += SPEED;
  }
  fx.erase(std::remove_if(fx.begin(), fx.end(), [](const Fx &f) { return f.t > f.life; }), fx.end());
}

// ---- 描く部品 (戦車・敵・弾) ----
static void kneeOf(float X, float Y, float fx, float fy, float cx, float cy, float &kx, float &ky)
{
  const float a = 10, b = 13;
  float dx = fx - X, dy = fy - Y, d = hypotf(dx, dy);
  d = clampf(d, fabsf(a - b) + 0.01f, a + b - 0.01f);
  float an = atan2f(dy, dx), off = acosf(clampf((a * a + d * d - b * b) / (2 * a * d), -1, 1));
  float bd = -1;
  for (int s : {1, -1})
  {
    float k = an + off * s, x = X + cosf(k) * a, y = Y + sinf(k) * a, dd = (x - cx) * (x - cx) + (y - cy) * (y - cy);
    if (dd > bd)
    {
      bd = dd;
      kx = x;
      ky = y;
    }
  }
}

static void drawTank(float cx, float cy, const float *fxs, const float *fys, const float *lift, float ang, float rec, int t)
{
  C(cx + 3, cy + 5, 15, 0, 102);
  for (int i = 0; i < 6; i++)
  {
    float X = cx + HIPS[i][0], Y = cy + HIPS[i][1], lf = lift[i];
    float fx = fxs[i] - lf * 1.5f, fy = fys[i] - lf * 3;
    float kx, ky;
    kneeOf(X, Y, fx, fy, cx, cy, kx, ky);
    kx += (kx - cx) * 0.12f * lf;
    ky -= 2 * lf;
    if (lf > 0.05f)
      C(fxs[i] + 1, fys[i] + 2, 2, 0, 89);
    LT(kx, ky, fx, fy, 3, 0x26282a);
    L(kx - 1, ky - 1, fx - 1, fy - 1, 0x5a5e62);
    LT(X, Y, kx, ky, 4, 0x6b7075);
    L(X - 1, Y - 1, kx - 1, ky - 1, 0xc4c9cc);
    C(kx, ky, 3, 0x3a3d40);
    C(kx - 1, ky - 1, 1, lf > 0.05f ? 0xffb23e : 0x9aa0a5);
    float sx = fx - cx >= 0 ? 1 : -1;
    T(fx - 2, fy - 3, fx + 2, fy - 3, fx + sx, fy + 3, 0x1a1b1c);
    P(fx - 1, fy - 2, lf > 0.05f ? 0xb3b8bc : 0x6b7075);
  }
  R(cx - 14, cy - 10, 5, 20, 0x55585b);
  R(cx + 9, cy - 10, 5, 20, 0x55585b);
  R(cx - 14, cy - 10, 1, 20, 0x9aa0a5);
  R(cx + 13, cy - 10, 1, 20, 0x2a2c2e);
  for (int k = 0; k < 4; k++)
  {
    R(cx - 13, cy - 8 + k * 5, 3, 1, 0x2a2c2e);
    R(cx + 10, cy - 8 + k * 5, 3, 1, 0x2a2c2e);
  }
  R(cx - 9, cy - 13, 18, 26, 0x8a8f94);
  T(cx - 9, cy - 13, cx + 8, cy - 13, cx, cy - 19, 0xa3a8ac);
  T(cx - 9, cy + 12, cx + 8, cy + 12, cx, cy + 16, 0x5a5e62);
  R(cx - 9, cy - 13, 1, 26, 0xd8dcdf);
  L(cx - 9, cy - 13, cx, cy - 19, 0xeef1f2);
  R(cx + 8, cy - 13, 1, 26, 0x3a3d40);
  R(cx - 6, cy - 10, 12, 1, 0x5a5e62);
  R(cx - 1, cy - 17, 2, 4, 0x3a3d40);
  for (int k = 0; k < 3; k++)
    R(cx - 6, cy + 6 + k * 2, 12, 1, 0x3a3d40);
  uint32_t ex = (t >> 2) & 1 ? 0xff9a3c : 0x7a2a12;
  R(cx - 6, cy + 13, 3, 2, ex);
  R(cx + 3, cy + 13, 3, 2, ex);
  P(cx - 3, cy - 8, 0xffb23e);
  P(cx + 2, cy - 8, 0xffb23e);
  float dx = cosf(ang), dy = sinf(ang), px = -dy, py = dx, ty = cy - 1;
  for (float s : {-2.5f, 2.5f})
  {
    float bx = cx + px * s, by = ty + py * s, len = 16 - rec;
    LT(bx + dx * 4, by + dy * 4, bx + dx * len, by + dy * len, 2, 0x2a2c2e);
    L(bx + dx * 4 - 1, by + dy * 4, bx + dx * len - 1, by + dy * len, 0x8a8f94);
    P(bx + dx * len, by + dy * len, 0x0e0f10);
  }
  C(cx, ty, 7, 0x7d8388);
  CR(cx, ty, 7, 0x2a2c2e);
  C(cx - 2, ty - 2, 3, 0xb3b8bc);
  P(cx - 3, ty - 3, 0xf2f4f5);
  R(cx + 1, ty + 2, 3, 3, 0x4a4e52);
  C(cx + dx * 5, ty + dy * 5, 1, 0x6ab8ff);
}

static void redEye(float x, float y, float r, bool hit)
{
  C(x, y, r + 2, 0xff3c28, 56);
  C(x, y, r, hit ? 0xffffff : RED);
  P(x, y, 0xffe0d8);
}
static void drawTwin(float x, float y, int t, bool hit)
{
  C(x + 9, y + 18, 7, 0, 71);
  for (int s : {-1, 1})
  {
    float rx = x + s * 11;
    LT(x + s * 3, y, rx, y, 2, 0x55585b);
    C(rx, y, 7, 0xa0a0a0, 46);
    CR(rx, y, 7, 0x8a8f94);
    CR(rx, y, 6, 0x3a3d40);
    for (int k = 0; k < 2; k++)
    {
      float a = t * 0.9f * s + k * PIf;
      L(rx - cosf(a) * 6, y - sinf(a) * 6, rx + cosf(a) * 6, y + sinf(a) * 6, 0xc4c9cc);
    }
    C(rx, y, 1, 0x2a2c2e);
  }
  C(x, y + 1, 5, 0x2a2c2e);
  C(x, y, 4, hit ? 0xc4c9cc : 0x6b7075);
  C(x - 1, y - 1, 2, 0xa3a8ac);
  R(x - 2, y - 6, 4, 3, 0x4a4e52);
  redEye(x, y + 2, 2, hit);
}
static void drawWalker(float x, float y, int t, bool hit)
{
  C(x + 3, y + 4, 10, 0, 89);
  static const int LEG[4][3] = {{-1, -1, 0}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 0}};
  for (auto &l : LEG)
  {
    float st = sinf(t * 0.2f + l[2] * PIf) * 2;
    float hx = x + l[0] * 6, hy = y + l[1] * 4, kx = x + l[0] * 11, ky = y + l[1] * 4 - 3, fx = x + l[0] * 13, fy = y + l[1] * 10 + st;
    LT(hx, hy, kx, ky, 3, 0x55585b);
    L(hx, hy - 1, kx, ky - 1, 0x9aa0a5);
    LT(kx, ky, fx, fy, 2, 0x2a2c2e);
    C(kx, ky, 1, 0x8a8f94);
    R(fx - 2, fy - 1, 4, 2, 0x1a1b1c);
  }
  R(x - 8, y - 8, 16, 15, hit ? 0xd8d0c0 : 0x8a7a62);
  R(x - 8, y - 8, 16, 1, 0xc4b49a);
  R(x - 8, y - 8, 1, 15, 0xa89880);
  R(x + 7, y - 8, 1, 15, 0x4a3e30);
  R(x - 8, y + 6, 16, 1, 0x4a3e30);
  C(x + 4, y + 3, 2, 0x6b3a1e);
  P(x - 5, y + 4, 0x8a4f28);
  R(x - 6, y - 12, 12, 4, 0x5a5e62);
  R(x - 6, y - 12, 12, 1, 0x8a8f94);
  R(x - 4, y - 4, 8, 8, 0x4a4e52);
  R(x - 4, y - 4, 8, 1, 0x7d8388);
  R(x - 3, y + 3, 2, 7, 0x2a2c2e);
  R(x + 1, y + 3, 2, 7, 0x2a2c2e);
  redEye(x, y - 1, 2, hit);
  L(x + 5, y - 8, x + 7, y - 14, 0x2a2c2e);
  P(x + 7, y - 15, (t >> 3) & 1 ? RED : 0x7a1a10);
}
static void eShot(float x, float y, float dx, float dy)
{
  float l = hypotf(dx, dy);
  if (l == 0)
    l = 1;
  float ux = dx / l, uy = dy / l, nx = -uy, ny = ux;
  C(x, y, 3, 0xff3c28, 56);
  Pt d[4] = {{x + ux * 4, y + uy * 4}, {x + nx * 1.8f, y + ny * 1.8f}, {x - ux * 3, y - uy * 3}, {x - nx * 1.8f, y - ny * 1.8f}};
  poly(d, 4, RED);
  P(x + ux, y + uy, 0xffe0d8);
}
static void drawFx(int k, float x, float y, float s, int t, int life)
{
  float q = (float)t / life;
  if (k == 0)
  {
    if (q > 0.35f)
      C(x + 2, y - q * 10, 4 + q * 10, 0x3c3832, (int)(115 * (1 - q)));
    C(x, y, 5 + q * 13, 0xff8c28, (int)(115 * (1 - q)));
    if (q < 0.55f)
    {
      float s2 = 1 - q / 0.55f;
      C(x, y, 3 + 9 * s2, 0xff7a1f);
      C(x - 1, y - 1, 2 + 6 * s2, 0xffc23e);
      C(x - 1, y - 1, 1 + 3 * s2, 0xfff6d0);
    }
    for (int i = 0; i < 12; i++)
    {
      float an = i * 0.52f + s, d = 3 + q * 22 * (0.6f + hs(i + s) * 0.6f);
      P(x + cosf(an) * d, y + sinf(an) * d, i % 3 ? 0xffd25a : 0xff5a1f);
      if (i % 4 == 0)
        R(x + cosf(an) * d * 0.7f, y + sinf(an) * d * 0.7f, 2, 2, 0x2a2420);
    }
  }
  else
  {
    for (int i = 1; i < 6; i++)
    {
      float tt = max(0, t - i * 2);
      P(x + sinf(tt * 0.3f + s) * 7, y - tt * 1.4f, RED, (int)(178 * (1 - i / 6.0f)));
    }
    float ox = x + sinf(t * 0.3f + s) * 7, oy = y - t * 1.4f;
    C(ox, oy, 3, 0xff3c28, 77);
    C(ox, oy, 2, RED);
    P(ox, oy, 0xffe0d8);
  }
}


// 戦車がやられたときの大きな爆発 (脚の破片が飛ぶ)
static void drawBigBoom(float x, float y, float s, int t, int life)
{
  float q = (float)t / life;
  C(x, y, 8 + q * 26, 0xff8c28, (int)(120 * (1 - q)));
  if (q < 0.5f)
  {
    float k = 1 - q / 0.5f;
    C(x, y, 6 + 16 * k, 0xff7a1f);
    C(x - 1, y - 2, 4 + 11 * k, 0xffc23e);
    C(x - 2, y - 3, 2 + 6 * k, 0xfff6d0);
  }
  if (q > 0.3f)
    for (int i = 0; i < 3; i++)
      C(x + (i - 1) * 8, y - q * 24 - i * 3, 5 + q * 12, 0x3c3832, (int)(120 * (1 - q)));
  for (int i = 0; i < 6; i++) // 脚の破片
  {
    float an = i * 1.047f + s, d = 6 + q * 40;
    float px = x + cosf(an) * d, py = y + sinf(an) * d * 0.8f;
    LT(px, py, px + cosf(an + q * 6) * 5, py + sinf(an + q * 6) * 5, 2, 0x55585b);
  }
  for (int i = 0; i < 16; i++)
  {
    float an = i * 0.39f + s, d = 4 + q * 34 * (0.5f + hs(i + s) * 0.7f);
    P(x + cosf(an) * d, y + sinf(an) * d, i % 3 ? 0xffd25a : 0xff5a1f);
  }
}

int Game2::zoneAt(int sy) const
{
  int sc = (int)scroll;
  int wu = ((sc + H - sy) % bg2::TOT + bg2::TOT) % bg2::TOT; // 下から数えた位置
  int z = 0;
  for (int i = 0; i < bg2::NZ; i++)
    if (wu >= bg2::STARTS[i])
      z = i;
  return z;
}

// ---- 帯 1 本を描く ----
void Game2::Draw(Gfx &gx)
{
  const int TOT = bg2::TOT;
  int ltop = gx.top * 2 / 3, sc = (int)scroll;
  uint32_t t0 = micros();
  // 背景 (取っておいた絵を写す)
  for (int ly = 0; ly < LSTRIP; ly++)
  {
    int r = ((TOT - H - sc + ltop + ly) % TOT + TOT) % TOT;
    memcpy(gx.buf + ly * LW, bg2::bgBuf + (size_t)r * LW, LW * 2);
  }
  uint32_t t1 = micros();
  Surf s;
  s.buf = gx.buf;
  s.y0 = ltop;
  s.h = LSTRIP;
  S = &s;
  // 区画の動くもの
  for (int k = sc / TOT - 1; k <= (sc + H) / TOT + 1; k++)
    for (int i = 0; i < bg2::NZ; i++)
    {
      const bg2::Zone &z = bg2::ZONES[i];
      int z0 = k * TOT + bg2::STARTS[i], top = H - (z0 + z.h - sc);
      if (top + z.h <= ltop || top >= ltop + LSTRIP)
        continue;
      s.oy = top;
      s.cy0 = max(top, ltop);
      s.cy1 = min(top + z.h - 1, ltop + LSTRIP - 1);
      z.anim(z.h, t);
    }
  s.oy = 0;
  s.cy0 = -100000;
  s.cy1 = 100000;
  uint32_t t2 = micros();
  // 地上の敵、自分の弾、戦車
  for (auto &e : en)
    if (e.k == 1 && e.y > ltop - 30 && e.y < ltop + LSTRIP + 30)
      drawWalker(e.x, e.y, e.walking ? t : 0, e.hit > 0);
  for (auto &b : shots)
    if (b.y > ltop - 8 && b.y < ltop + LSTRIP + 8)
    {
      R(b.x - 2, b.y - 5, 5, 11, 0x468cff, 71);
      R(b.x - 1, b.y - 4, 3, 9, 0x3a7bff);
      R(b.x, b.y - 4, 1, 8, 0xe0f0ff);
    }
  bool blink = invT > 0 && enterT == 0 && (t & 2); // 入り直したあとの点滅
  if (tankOn && !blink && ty > ltop - 40 && ty < ltop + LSTRIP + 40)
  {
    float fxs[6], fys[6], lift[6];
    for (int i = 0; i < 6; i++)
    {
      fxs[i] = feet[i].x;
      fys[i] = feet[i].y + sc;
      lift[i] = feet[i].p >= 0 ? sinf(PIf * feet[i].p) : 0;
    }
    drawTank(tx, ty, fxs, fys, lift, ang, rec, t);
  }
  uint32_t t3 = micros();
  // 手前の絵 (頭上の配管や梁)
  for (int ly = 0; ly < LSTRIP; ly++)
  {
    int r = ((TOT - H - sc + ltop + ly) % TOT + TOT) % TOT;
    if (!bg2::foreRow[r])
      continue;
    const uint16_t *src = bg2::foreBuf + (size_t)r * LW;
    uint16_t *dst = gx.buf + ly * LW;
    for (int x = 0; x < LW; x++)
      if (src[x] != bg2::KEY)
        dst[x] = src[x];
  }
  uint32_t t4 = micros();
  // 空の敵、爆発、敵の弾
  for (auto &e : en)
    if (e.k == 0 && e.y > ltop - 30 && e.y < ltop + LSTRIP + 30)
      drawTwin(e.x, e.y, t, e.hit > 0);
  for (auto &f : fx)
    if (f.t >= 0 && f.y > ltop - 70 && f.y < ltop + LSTRIP + 50)
    {
      if (f.k == 2)
        drawBigBoom(f.x, f.y, f.s, f.t, f.life);
      else
        drawFx(f.k, f.x, f.y, f.s, f.t, f.life);
    }
  for (auto &b : eb)
    if (b.y > ltop - 6 && b.y < ltop + LSTRIP + 6)
      eShot(b.x, b.y, b.dx, b.dy);
  drawOverlay(gx, ltop);
  uint32_t t5 = micros();
  profBg += t1 - t0;
  profAnim += t2 - t1;
  profObj += (t3 - t2) + (t5 - t4);
  profFore += t4 - t3;
}

// ---- 文字と枠 ----
static const uint32_t INK = 0x000000, CREAM = 0xF2F0E8, GOLD = 0xC9A227, REDT = 0xFF6A5A, MUTE = 0x9A958A;
// 液晶の座標で文字を書く。size は 1 (6x8)・3 (12x16)・5 (18x24)。center なら x が真ん中
static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, 1, INK);
}
static void panel(int x, int y, int w, int h)
{
  R(x, y, w, h, 0x0c0b0a, 190);
  R(x, y, w, 1, 0x8a4f28);
  R(x, y + h - 1, w, 1, 0x8a4f28);
  R(x, y, 1, h, 0x6b3a1e);
  R(x + w - 1, y, 1, h, 0x6b3a1e);
  for (int i = 0; i < w; i += 8) // 上の縁に黄色と黒のすじ
    R(x + i, y + 2, 4, 2, GOLD);
}
static void lifeIcon(int x, int y)
{
  for (int s : {-1, 1})
  {
    L(x + s * 2, y + 1, x + s * 5, y - 2, 0x6b7075);
    L(x + s * 2, y + 4, x + s * 5, y + 7, 0x6b7075);
  }
  R(x - 2, y - 1, 5, 8, 0x8a8f94);
  R(x - 2, y - 1, 1, 8, 0xd8dcdf);
  C(x, y + 2, 1, 0x6ab8ff);
}

void Game2::drawOverlay(Gfx &g, int ltop)
{
  char s[40];
  bool playing = mode == Mode::Play || mode == Mode::Continue;
  // 上の点数
  if (mode != Mode::Attract)
  {
    if (!playing || (t / 16) % 2)
      txt(g, 4, 2, GOLD, "1UP");
    snprintf(s, sizeof(s), "%07d", score);
    txt(g, 4, 11, CREAM, s);
  }
  txt(g, W / 2 + 8, 2, GOLD, "HI-SCORE", true);
  snprintf(s, sizeof(s), "%07d", max(score, Table[0].Score));
  txt(g, W / 2 + 8, 11, CREAM, s, true);
  if (mode != Mode::Attract)
  {
    const char *zn = bg2::ZONES[lastZone < 0 ? 0 : lastZone].name;
    txt(g, W - 4 - (int)strlen(zn) * 6, 2, GOLD, zn);
    snprintf(s, sizeof(s), "ZONE %d", (lastZone < 0 ? 0 : lastZone) + 1);
    txt(g, W - 4 - (int)strlen(s) * 6, 11, MUTE, s);
  }
  // 残機
  if (playing && ltop + LSTRIP > 300)
    for (int i = 0; i < min(lives, 8); i++)
      lifeIcon(10 + i * 13, 308);

  switch (mode)
  {
  case Mode::Attract:
    if ((modeT / (8 * FPS)) % 2 == 0)
    {
      panel(20, 70, 200, 154);
      txt(g, W / 2, 86, GOLD, "FOUNDRY", true, 5);
      txt(g, W / 2, 116, CREAM, "HEXAPOD TANK", true);
      txt(g, W / 2, 132, MUTE, "THE MACHINES WANT TO LIVE", true);
      if ((modeT / 15) % 2 == 0)
        txt(g, W / 2, 158, CREAM, "PUSH SW1", true, 3);
      txt(g, W / 2, 210, MUTE, "SW3 HOLD: MENU", true);
    }
    else
    {
      panel(30, 70, 180, 150);
      txt(g, W / 2, 84, GOLD, "BEST 5", true, 3);
      static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
      for (int i = 0; i < 5; i++)
      {
        snprintf(s, sizeof(s), "%s %07d %-3s", ORD[i], Table[i].Score, Table[i].Name);
        txt(g, W / 2, 112 + i * 20, i == 0 ? GOLD : CREAM, s, true);
      }
    }
    break;
  case Mode::Play:
    if (bannerT > 0 && bannerT < 3 * FPS - 6)
    {
      panel(30, 120, 180, 44);
      snprintf(s, sizeof(s), "ZONE %d", bannerZone + 1);
      txt(g, W / 2, 128, GOLD, s, true);
      txt(g, W / 2, 142, CREAM, bg2::ZONES[bannerZone].name, true, 3);
    }
    if (extendT > 0 && (extendT / 6) % 2)
      txt(g, W / 2, 180, GOLD, "EXTEND!", true, 3);
    if (paused)
    {
      panel(40, 120, 160, 64);
      txt(g, W / 2, 132, CREAM, "PAUSE", true, 3);
      txt(g, W / 2, 158, MUTE, "SW3: RESUME", true);
      txt(g, W / 2, 170, MUTE, "SW2: QUIT", true);
    }
    break;
  case Mode::Continue:
    panel(40, 110, 160, 90);
    txt(g, W / 2, 122, CREAM, "CONTINUE?", true, 3);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, W / 2, 146, GOLD, s, true, 5);
    txt(g, W / 2, 182, MUTE, "SW1: CONTINUE", true);
    break;
  case Mode::GameOver:
    txt(g, W / 2, 140, REDT, "GAME OVER", true, 5);
    break;
  case Mode::NameEntry:
  {
    panel(30, 90, 180, 120);
    txt(g, W / 2, 102, GOLD, "NEW RECORD", true, 3);
    snprintf(s, sizeof(s), "SCORE %07d", score);
    txt(g, W / 2, 124, CREAM, s, true);
    for (int i = 0; i < 3; i++)
    {
      char c[2] = {entry[i], 0};
      int x = W / 2 - 30 + i * 22;
      txt(g, x, 146, i == entryPos ? GOLD : CREAM, c, true, 5);
      if (i == entryPos && (t / 8) % 2)
        R(x - 8, 172, 16, 2, GOLD);
    }
    txt(g, W / 2, 186, MUTE, "STICK: LETTER  SW1: OK", true);
    snprintf(s, sizeof(s), "%d", modeT / FPS);
    txt(g, 200, 100, MUTE, s);
    break;
  }
  case Mode::Ranking:
  {
    panel(30, 70, 180, 160);
    txt(g, W / 2, 84, GOLD, "BEST 5", true, 3);
    static const char *ORD[5] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
    for (int i = 0; i < 5; i++)
    {
      snprintf(s, sizeof(s), "%s %07d %-3s", ORD[i], Table[i].Score, Table[i].Name);
      bool mine = i == newRank;
      if (!mine || (t / 8) % 2)
        txt(g, W / 2, 112 + i * 20, mine ? REDT : i == 0 ? GOLD : CREAM, s, true);
    }
    snprintf(s, sizeof(s), "YOUR SCORE %07d", score);
    txt(g, W / 2, 214, MUTE, s, true);
    break;
  }
  }
}
