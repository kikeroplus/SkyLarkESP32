// 3 本目 (LONGSHOT)。動きはデザイン案のページ (longshot.html) と同じ。遊びの流れは SKYLARK (game.cpp) にそろえる
#include "game3.h"
#include <algorithm>

static const int W = 240, H = 320;
static const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
static const int FPS = Game3::FPS;
static const int STAGE_TIME = 75 * FPS;
static const float SCX = 120, SCY = 150, SCR = 112; // スコープの円 (画面の座標)
static const int MAX_AMMO = 5, RELOAD = 24;
static const uint32_t INK = 0x000000, CREAM = 0xF2F0E8, GOLD = 0xD9A441, REDT = 0xFF6A5A, MUTE = 0x9A9A92, GREEN = 0x7FB069;

struct StageDef
{
  int scene, tod;
  float wind, gust;
  int crowd;
  float rain;
  Game3::Clue c[3];
};
// 1 シーン 2 面ずつ。ホテル前 → 駅前 → 港 → 雨の繁華街 → 公園
static const int NSTAGE = 10;
static const StageDef STAGES[NSTAGE] = {
    {w3::SC_HOTEL, w3::DAY, 2.5f, 0.6f, 12, 0, {{0, w3::H_RED}, {1, w3::B_BLACK}, {2, 1}}},
    {w3::SC_HOTEL, w3::DUSK, 3.5f, 0.9f, 14, 0, {{0, w3::H_WHITE}, {3, w3::CO_GREEN}, {1, w3::B_BROWN}}},
    {w3::SC_STATION, w3::DAY, -3.0f, 1.0f, 16, 0, {{0, w3::H_NAVY}, {1, w3::B_RED}, {2, 1}}},
    {w3::SC_STATION, w3::DUSK, 4.0f, 1.2f, 18, 0, {{0, w3::H_BEIGE}, {3, w3::CO_YELLOW}, {1, w3::B_BLACK}}},
    {w3::SC_HARBOR, w3::DAY, 5.5f, 1.4f, 12, 0, {{0, w3::H_GREEN}, {3, w3::CO_BLUE}, {1, w3::B_BROWN}}},
    {w3::SC_HARBOR, w3::DUSK, -6.0f, 1.6f, 14, 0, {{0, w3::H_BLACK}, {2, 1}, {3, w3::CO_WHITE}}},
    {w3::SC_RAIN, w3::NIGHT, 2.0f, 0.8f, 16, 0.6f, {{2, 1}, {1, w3::B_RED}, {3, w3::CO_MAROON}}},
    {w3::SC_RAIN, w3::NIGHT, -3.0f, 1.2f, 18, 1.0f, {{3, w3::CO_YELLOW}, {1, w3::B_BLACK}, {2, 1}}},
    {w3::SC_PARK, w3::DAY, 3.0f, 1.0f, 16, 0, {{0, w3::H_RED}, {3, w3::CO_NAVY}, {2, 1}}},
    {w3::SC_PARK, w3::NIGHT, -4.5f, 1.4f, 18, 0, {{0, w3::H_WHITE}, {1, w3::B_BROWN}, {3, w3::CO_GREY}}},
};
static const uint32_t CAR_COL[7] = {0xb8bcc4, 0xa33a32, 0x2f4f7a, 0xe8e2d2, 0x3a3a3e, 0x5a7a5a, 0x8a6a9a};

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

float Game3::rnd()
{
  seed = seed * 1664525u + 1013904223u;
  return (seed >> 8) * (1.0f / 16777216.0f);
}

void Game3::sfx(Sfx s, float vol)
{
  if (Snd)
    Snd->Play(s, mode == Mode::Attract ? vol * 0.4f : vol);
}

float Game3::wind() const
{
  float s = t / (float)FPS;
  return windBase + gustAmt * (sinf(s * 0.35f) * 0.8f + sinf(s * 1.1f + 1) * 0.4f);
}
float Game3::swayX() const
{
  float s = t / (float)FPS;
  return amp * (sinf(s * 1.7f) * 0.7f + sinf(s * 0.63f + 1) * 0.5f);
}
float Game3::swayY() const
{
  float s = t / (float)FPS;
  float beat = sinf(s * 7.5f);
  return amp * (sinf(s * 1.31f + 2) * 0.6f + sinf(s * 0.47f) * 0.5f + (beat > 0 ? beat * 0.15f : 0));
}

// ---- 始める ----
bool Game3::begin()
{
  seed = esp_random() | 1;
  if (!w3::alloc())
    return false;
  uint32_t t0 = millis();
  w3::buildAll(w3::SC_HOTEL, w3::DAY);
  buildMs = millis() - t0;
  startDemo();
  return true;
}

void Game3::startDemo()
{
  mode = Mode::Attract;
  modeT = 0;
  paused = false;
  stage = 0;
  setupStage();
  if (w3::tod != w3::DAY || w3::sceneNo != w3::SC_HOTEL || w3::buildProgress() < 1)
    w3::buildAll(w3::SC_HOTEL, w3::DAY);
  windBase = 2;
}

void Game3::newGame()
{
  score = 0;
  lives = 3;
  stage = 0;
  continues = 0;
  startBrief();
}

void Game3::startBrief()
{
  mode = Mode::Brief;
  modeT = 0;
  paused = false;
  setupStage();
  if (w3::tod != todNow || w3::sceneNo != sceneNow || w3::buildProgress() < 1)
    w3::buildBegin(sceneNow, todNow);
  sfx(Sfx::Mission);
}

void Game3::startPlay()
{
  mode = Mode::Play;
  modeT = 0;
  sfx(Sfx::Bolt);
}

void Game3::newCar(Car &c, int lane)
{
  const w3::Scene &sc = w3::SCENES[sceneNow][0];
  uint8_t kinds = sc.laneKinds[lane];
  int k;
  do
    k = (int)(rnd() * w3::CAR_N);
  while (!(kinds >> k & 1));
  c.kind = k;
  c.color = k == w3::CAR_TAXI ? 0xe8b830 : k == w3::CAR_VAN ? 0xe4e4e0 : k == w3::CAR_BUS ? 0x2f7a5a : k == w3::CAR_TRUCK ? 0x2a5a9a
                                                                                                                      : CAR_COL[(int)(rnd() * 7)];
  c.stopped = false;
  c.stopT = 0;
  if (k == w3::CAR_BIKE)
    c.rider = randomLook();
}

void Game3::setupStage()
{
  const StageDef &d = STAGES[stage % NSTAGE];
  int lap = stage / NSTAGE;
  if (mode == Mode::Attract)
  {
    sceneNow = w3::SC_HOTEL;
    todNow = w3::DAY;
    rainNow = 0;
  }
  else
  {
    sceneNow = d.scene;
    todNow = d.tod;
    rainNow = d.rain;
  }
  const w3::Scene &sc = *w3::SCENES[sceneNow];
  windBase = d.wind * (1 + 0.25f * lap);
  gustAmt = d.gust * (1 + 0.2f * lap);
  for (int i = 0; i < 3; i++)
    clues[i] = d.c[i];
  panicMax = max(5 * FPS, (10 - lap) * FPS);
  timeLeft = STAGE_TIME;
  ammo = MAX_AMMO;
  shotsFired = 0;
  panic = false;
  failReason = 0;
  clearT = 0;
  headShot = false;
  bullets.clear();
  hits.clear();
  makeCrowd(d.crowd + 3 * lap);
  // 窓の人
  wins.clear();
  for (int i = 0; i < 6 && sc.nWin > 0; i++)
  {
    WinP w;
    w.idx = (int)(rnd() * sc.nWin);
    bool dup = false;
    for (auto &o : wins)
      dup |= o.idx == w.idx;
    if (dup)
      continue;
    w.on = rnd() < 0.5f;
    w.tm = 30 + (int)(rnd() * 200);
    w.u = rnd();
    w.dir = rnd() < 0.5f ? -1 : 1;
    w.lk = randomLook();
    w.lk.umbrella = 0;
    wins.push_back(w);
  }
  // 車 (車線ごとに同じ速さで流れる。前が止まれば後ろも止まる)
  cars.clear();
  for (int lane = 0; lane < sc.nLane; lane++)
  {
    int n = sc.laneKinds[lane] == (1 << w3::CAR_BIKE) ? 3 : 2;
    for (int i = 0; i < n; i++)
    {
      Car c;
      c.lane = lane;
      c.dir = sc.laneDir[lane];
      c.spd = sc.laneSpd[lane];
      c.x = i * (260.0f / n) + rnd() * 30 - 10;
      c.wheel = 0;
      newCar(c, lane);
      cars.push_back(c);
    }
  }
  // 鳩 (港ではカモメ)
  birds.clear();
  for (int i = 0; i < 9; i++)
  {
    Bird b;
    const w3::Box &z = sc.birdZone[i < 5 ? 0 : 1];
    if (z.w <= 0)
      continue;
    b.x = z.x + rnd() * z.w;
    b.y = z.y + rnd() * z.h;
    b.dir = rnd() < 0.5f ? -1 : 1;
    b.fly = false;
    b.vx = b.vy = 0;
    b.ph = rnd() * 6;
    b.tm = (int)(rnd() * 60);
    birds.push_back(b);
  }
  ax = 120;
  ay = 214;
  scope = false;
  scopeK = 0;
  recoil = 0;
  reload = 0;
  breath = 1;
  tired = 0;
  amp = 1.2f;
  holding = false;
  flash = 0;
}

w3::Look Game3::randomLook()
{
  w3::Look lk;
  lk.hat = rnd() < 0.45f ? w3::H_NONE : (uint8_t)(1 + rnd() * (w3::H_N - 1));
  lk.bag = rnd() < 0.5f ? w3::B_NONE : (uint8_t)(1 + rnd() * (w3::B_N - 1));
  lk.coat = (uint8_t)(rnd() * w3::CO_N);
  lk.glasses = rnd() < 0.3f;
  lk.skirt = rnd() < 0.25f;
  lk.longCoat = !lk.skirt && rnd() < 0.3f;
  lk.hair = (uint8_t)(rnd() * 5);
  lk.hairKind = rnd() < 0.2f ? 1 : rnd() < 0.12f ? 2 : 0;
  lk.skin = (uint8_t)(rnd() * 4);
  lk.pants = (uint8_t)(rnd() * 4);
  lk.umbrella = rainNow > 0 && rnd() < 0.65f ? (uint8_t)(1 + rnd() * 6) : 0;
  return lk;
}

// 手がかりに合わせる (match) か、わざと外す
static void applyClue(w3::Look &lk, const Game3::Clue &c, bool match, Game3 *g, float (*rf)(Game3 *))
{
  switch (c.type)
  {
  case 0:
    if (match)
      lk.hat = c.v;
    else
      while (lk.hat == c.v)
        lk.hat = rf(g) < 0.4f ? w3::H_NONE : (uint8_t)(1 + rf(g) * (w3::H_N - 1));
    break;
  case 1:
    if (match)
      lk.bag = c.v;
    else
      while (lk.bag == c.v)
        lk.bag = rf(g) < 0.4f ? w3::B_NONE : (uint8_t)(1 + rf(g) * (w3::B_N - 1));
    break;
  case 2:
    lk.glasses = match;
    break;
  default:
    if (match)
      lk.coat = c.v;
    else
      while (lk.coat == c.v)
        lk.coat = (uint8_t)(rf(g) * w3::CO_N);
    break;
  }
}

void Game3::makeCrowd(int n)
{
  peds.clear();
  auto rf = [](Game3 *g) { return g->rnd(); };
  for (int i = 0; i < n; i++)
  {
    Ped p;
    p.lk = randomLook();
    bool share[3] = {false, false, false};
    if (i == 0)
      share[0] = share[1] = share[2] = true;
    else
    {
      static const int K[5] = {0, 1, 2, 1, 2}; // 手がかりをいくつ持たせるか (0〜2)
      int k = K[i % 5];
      int a = (int)(rnd() * 3), b = (a + 1 + (int)(rnd() * 2)) % 3;
      if (k >= 1)
        share[a] = true;
      if (k >= 2)
        share[b] = true;
    }
    for (int c = 0; c < 3; c++)
      applyClue(p.lk, clues[c], share[c], this, rf);
    p.target = i == 0;
    const w3::Scene &sc = *w3::SCENES[sceneNow];
    const w3::Box &z = sc.walk[(int)(rnd() * sc.nWalk)];
    p.y = z.y + rnd() * z.h;
    p.x = p.target ? max(z.x, 30.0f) + rnd() * (min(z.x + z.w, 210.0f) - max(z.x, 30.0f)) : z.x + rnd() * z.w;
    p.zone = &z - sc.walk;
    p.dir = rnd() < 0.5f ? -1 : 1;
    p.spd = 2.0f + rnd() * 1.6f;
    p.ph = rnd() * 6;
    p.st = rnd() < 0.7f ? 0 : 1;
    p.tm = 40 + (int)(rnd() * 200);
    peds.push_back(p);
  }
  order.resize(peds.size());
}

// ---- 撃つ ----
void Game3::fire()
{
  float px = ax + swayX(), py = ay + swayY() - recoil;
  float d = distAt(py), w = wind();
  Bullet b;
  b.x0 = px;
  b.y0 = py;
  b.x = px + w * d / 700;
  b.y = py + (d / 250) * (d / 250);
  b.total = b.t = max(3, (int)lroundf(d / 850 * FPS));
  bullets.push_back(b);
  ammo--;
  shotsFired++;
  reload = RELOAD;
  recoil = 5;
  flash = 3;
  sfx(Sfx::Rifle);
}

void Game3::startPanic(float x)
{
  if (panic)
    return;
  panic = true;
  panicX = x;
  panicT = panicMax;
  sfx(Sfx::Crowd, 0.8f);
  for (auto &p : peds)
  {
    if (!p.alive || p.gone)
      continue;
    p.st = 2;
    if (p.target) // 標的は近いほうの端へ。panicMax のあいだに出ていく速さで
    {
      p.dir = p.x < 120 ? -1 : 1;
      float edge = p.dir < 0 ? p.x + 10 : 250 - p.x;
      p.spd = max(7.0f, edge / (panicMax / (float)FPS));
    }
    else
    {
      p.dir = p.x < x ? -1 : 1;
      p.spd = 12 + rnd() * 4;
    }
  }
  for (auto &b : birds)
    if (!b.fly)
    {
      b.fly = true;
      b.vx = b.dir * (0.5f + rnd() * 0.4f);
      b.vy = -(0.6f + rnd() * 0.4f);
    }
  sfx(Sfx::Flutter, 0.7f);
  for (auto &w : wins) // 窓の人は隠れる
    if (w.on)
      w.tm = min(w.tm, 10 + (int)(rnd() * 30));
}

void Game3::resolve(const Bullet &b)
{
  // 手前の人から
  for (int k = (int)order.size() - 1; k >= 0; k--)
  {
    Ped &p = peds[order[k]];
    if (!p.alive || p.gone)
      continue;
    float hx = p.x + p.dir * 0.1f, hy = p.y - 9.35f;
    float dx = b.x - hx, dy = b.y - hy;
    bool head = dx * dx + dy * dy < 1.55f * 1.55f || (fabsf(dx) < 1.5f && dy > -2.2f && dy < 0);
    bool body = fabsf(b.x - p.x) < 1.6f && b.y > p.y - 7.9f && b.y < p.y + 0.3f;
    if (!head && !body)
      continue;
    p.alive = false;
    p.down = 0.01f;
    p.fd = -p.dir;
    sfx(Sfx::Impact, 0.6f);
    if (p.target)
    {
      headShot = head;
      clearT = 1;
    }
    else
      fail(1);
    startPanic(b.x);
    return;
  }
  const w3::Scene &sc = w3::SC();
  for (auto &w : wins)
  {
    if (!w.on)
      continue;
    const w3::Box &wb = sc.win[w.idx];
    float px = w3::winPersonX(wb.x, wb.w, w.u);
    if (fabsf(b.x - px) < 2.4f && b.y > wb.y + wb.h - 8.2f && b.y < wb.y + wb.h)
    {
      w.on = false;
      hits.push_back({b.x, b.y, 0, 3});
      sfx(Sfx::Glass);
      fail(5);
      startPanic(b.x);
      return;
    }
  }
  for (auto &c : cars)
  {
    float y0 = sc.laneY[c.lane], len = w3::carLen(c.kind);
    if (b.x > c.x && b.x < c.x + len && b.y > y0 + w3::carTop(c.kind) && b.y < y0 + 11)
    {
      if (c.kind == w3::CAR_BIKE) // 自転車の人に当たった
      {
        c.kind = w3::CAR_N; // 消す (倒れた人は描かない)
        sfx(Sfx::Impact, 0.6f);
        fail(1);
        startPanic(b.x);
        return;
      }
      hits.push_back({b.x, b.y, 0, 2});
      sfx(Sfx::Clank);
      sfx(Sfx::Horn, 0.7f);
      startPanic(b.x);
      return;
    }
  }
  int kind = b.y < sc.groundY ? 0 : 1;
  for (int i = 0; i < sc.nWin; i++)
  {
    const w3::Box &wb = sc.win[i];
    if (b.x > wb.x && b.x < wb.x + wb.w && b.y > wb.y && b.y < wb.y + wb.h)
      kind = 3;
  }
  hits.push_back({b.x, b.y, 0, kind});
  sfx(kind == 3 ? Sfx::Glass : Sfx::Impact);
  startPanic(b.x);
}

void Game3::fail(int reason)
{
  if (mode != Mode::Play || failReason)
    return;
  failReason = reason;
  mode = Mode::Failed;
  modeT = 0;
  lives--;
  sfx(Sfx::Fail);
}

// 確かめる用: n 番目の面へすぐ飛ぶ (景色は今描く)
void Game3::debugStage(int n)
{
  stage = n;
  score = 0;
  lives = 3;
  startBrief();
  w3::buildAll(sceneNow, todNow);
  startPlay();
}
void Game3::debugScope() { scope = !scope; }
void Game3::debugAimTarget()
{
  if (!peds.empty())
  {
    ax = peds[0].x;
    ay = peds[0].y - 7;
  }
}

void Game3::skipStage()
{
  if (mode != Mode::Play)
    return;
  peds[0].alive = false;
  peds[0].down = 0.01f;
  headShot = true;
  clearT = 1;
}

void Game3::endGame()
{
  mode = Mode::GameOver;
  modeT = 4 * FPS;
  sfx(Sfx::GameOver);
}

int Game3::rankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

void Game3::stepEntry(const Input &in)
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
    entryPos = max(0, entryPos - 1);
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

void Game3::finishEntry()
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

// ---- 1 フレーム ----
void Game3::stepAmbience()
{
  if (!Snd)
    return;
  bool quiet = paused || mode == Mode::Brief || mode == Mode::Continue || mode == Mode::NameEntry;
  float k = quiet ? 0 : mode == Mode::Attract ? 0.45f : 1;
  Snd->City = k * (todNow == w3::NIGHT ? 0.6f : 1.0f);
  Snd->WindS = k * min(1.0f, fabsf(wind()) / 6) * (scopeK > 0.5f ? 1.0f : 0.6f);
  Snd->Rain = k * rainNow * 0.8f;
}

void Game3::Step(const Input &in)
{
  if (in.VolumePressed && !(mode == Mode::Play && paused))
  {
    VolumeLevel = (VolumeLevel + 1) % 5;
    VolumeChanged = true;
    volT = 2 * FPS;
  }
  if (volT > 0)
    volT--;
  stepAmbience();
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
    t++;
    stepWorld();
    stepAim(nullptr);
    break;
  case Mode::Brief:
    w3::buildStep();
    modeT++;
    if (modeT > 20 && w3::buildProgress() >= 1 && (in.FirePressed || in.StartPressed))
      startPlay();
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
    t++;
    modeT++;
    stepWorld();
    stepAim(&in);
    if (clearT > 0 && ++clearT > 40)
    {
      mode = Mode::Clear;
      modeT = 0;
      int lap = stage / 6;
      bonus[0] = 1000 + 500 * (stage % 6) + 3000 * lap;
      bonus[1] = headShot ? 1000 : 0;
      bonus[2] = timeLeft / FPS * 20;
      bonus[3] = ammo * 300;
      bonus[4] = shotsFired == 1 ? 1500 : 0;
      for (int b : bonus)
        score += b;
      sfx(Sfx::Clear);
      break;
    }
    if (clearT == 0)
    {
      if (--timeLeft <= 0)
        fail(4);
      else if (panic && peds[0].gone)
        fail(2);
      else if (ammo == 0 && bullets.empty() && reload == 0)
        fail(3);
    }
    break;
  case Mode::Clear:
    t++;
    modeT++;
    stepWorld();
    stepAim(nullptr);
    if ((modeT > 2 * FPS && in.FirePressed) || modeT > 8 * FPS)
    {
      stage++;
      startBrief();
    }
    break;
  case Mode::Failed:
    t++;
    modeT++;
    stepWorld();
    stepAim(nullptr);
    if ((modeT > FPS && in.FirePressed) || modeT > 5 * FPS)
    {
      if (lives > 0)
        startBrief();
      else
      {
        mode = Mode::Continue;
        modeT = 10 * FPS;
      }
    }
    break;
  case Mode::Continue:
    if (in.FirePressed || in.StartPressed)
    {
      lives = 3;
      continues++;
      if (score % 10 < 9)
        score++;
      startBrief();
      return;
    }
    if (in.PausePressed || in.QuitPressed)
      modeT = 1;
    if (modeT % FPS == 0 && modeT > 0)
      sfx(Sfx::Tick);
    if (--modeT <= 0)
      endGame();
    break;
  case Mode::GameOver:
    t++;
    stepWorld();
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
    t++;
    stepWorld();
    stepEntry(in);
    break;
  case Mode::Ranking:
    t++;
    stepWorld();
    if (--modeT <= 0 || (modeT < 7 * FPS && (in.FirePressed || in.StartPressed)))
      startDemo();
    break;
  }
}

void Game3::stepWorld()
{
  // 人
  for (auto &p : peds)
  {
    if (!p.alive)
    {
      p.down = min(1.0f, p.down + 1.0f / 18);
      continue;
    }
    if (p.gone)
      continue;
    if (p.st == 2)
    {
      p.x += p.dir * p.spd / FPS;
      p.ph += p.spd * 0.045f;
      if (p.x < -10 || p.x > 250)
        p.gone = true;
      else
      {
        const w3::Box &z = w3::SC().walk[p.zone];
        if (z.x > 0 && (p.x < z.x - 4 || p.x > z.x + z.w + 4)) // 囲まれた所は端で消える (建物や木の陰へ)
          p.gone = true;
      }
      continue;
    }
    if (--p.tm <= 0)
    {
      if (p.st == 0)
      {
        p.st = 1;
        p.tm = 40 + (int)(rnd() * 90);
      }
      else
      {
        p.st = 0;
        p.tm = 120 + (int)(rnd() * 200);
        if (rnd() < 0.25f)
          p.dir = -p.dir;
      }
    }
    if (p.st == 0)
    {
      p.x += p.dir * p.spd / FPS;
      p.ph += p.spd * 0.04f;
      const w3::Box &z = w3::SC().walk[p.zone];
      if (z.x <= 0) // 画面の端から端まで歩ける所は、出たら反対から
      {
        if (p.x < -8)
          p.x = 248;
        if (p.x > 248)
          p.x = -8;
      }
      else if (p.x < z.x || p.x > z.x + z.w) // 囲まれた所は端で向きを変える
      {
        p.dir = p.x < z.x ? 1 : -1;
        p.x = max(z.x, min(z.x + z.w, p.x));
      }
    }
  }
  for (size_t i = 0; i < peds.size(); i++)
    order[i] = i;
  std::sort(order.begin(), order.end(), [&](int a, int b) { return peds[a].y < peds[b].y; });
  // 窓の人
  for (auto &w : wins)
  {
    if (--w.tm <= 0)
    {
      w.on = !w.on && !panic;
      w.tm = w.on ? 90 + (int)(rnd() * 150) : 60 + (int)(rnd() * 140);
      w.u = rnd();
      w.dir = rnd() < 0.5f ? -1 : 1;
    }
  }
  // 車
  const w3::Scene &sc = w3::SC();
  for (auto &c : cars)
  {
    if (c.kind >= w3::CAR_N)
      continue;
    float len = w3::carLen(c.kind), v = c.spd;
    float stopX = sc.laneStop[c.lane];
    if (c.stopT > 0)
    {
      v = 0;
      if (--c.stopT == 0)
        c.stopped = true;
    }
    else if (c.kind == w3::CAR_BUS && stopX >= 0 && !c.stopped)
    {
      float front = c.dir > 0 ? c.x + len : c.x;
      if ((c.dir > 0 && front >= stopX) || (c.dir < 0 && front <= stopX))
        c.stopT = 6 * FPS;
    }
    for (auto &o : cars) // 前の車に近すぎたら止まる
    {
      if (&o == &c || o.lane != c.lane || o.kind >= w3::CAR_N)
        continue;
      float gap = c.dir > 0 ? o.x - (c.x + len) : c.x - (o.x + w3::carLen(o.kind));
      if (gap > -len && gap < 5)
        v = 0;
    }
    c.x += c.dir * v / FPS;
    c.wheel += v / FPS / 1.8f;
    c.ph += v / FPS * 1.4f;
    bool out = c.dir < 0 ? c.x < -50 : c.x > 250;
    if (out)
    {
      float far = c.dir < 0 ? 250 : -50;
      for (auto &o : cars) // 同じ車線の最後の車のうしろへ
        if (&o != &c && o.lane == c.lane)
          far = c.dir < 0 ? max(far, o.x + 60) : min(far, o.x - 60);
      c.x = far - c.dir * rnd() * 50;
      newCar(c, c.lane);
    }
  }
  // 鳩: 人が近くに来たら飛んでいく
  bool flapped = false;
  for (auto &b : birds)
  {
    if (b.fly)
    {
      b.x += b.vx;
      b.y += b.vy;
      b.vy -= 0.01f;
      b.ph += 0.9f;
      continue;
    }
    b.ph += 0.25f;
    if (--b.tm <= 0)
    {
      b.tm = 20 + (int)(rnd() * 60);
      if (rnd() < 0.3f)
        b.dir = -b.dir;
      b.x += b.dir * 0.6f;
    }
    for (auto &p : peds)
      if (p.alive && !p.gone && fabsf(p.x - b.x) < 5 && fabsf(p.y - b.y) < 3)
      {
        b.fly = true;
        b.dir = p.x < b.x ? 1 : -1;
        b.vx = b.dir * (0.5f + rnd() * 0.3f);
        b.vy = -(0.5f + rnd() * 0.3f);
        flapped = true;
        break;
      }
  }
  if (flapped && mode != Mode::Attract)
    sfx(Sfx::Flutter, 0.4f);
  birds.erase(std::remove_if(birds.begin(), birds.end(), [](const Bird &b) { return b.y < -10 || b.x < -10 || b.x > 250; }), birds.end());
  // 弾
  for (auto &b : bullets)
    if (--b.t == 0)
      resolve(b);
  bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) { return b.t <= 0; }), bullets.end());
  for (auto &h : hits)
    h.t++;
  hits.erase(std::remove_if(hits.begin(), hits.end(), [](const Hit &h) { return h.kind != 3 && h.t > 30; }), hits.end());
  if (panic && panicT > 0)
    panicT--;
}

// 照準・スコープ・息。in が nullptr ならデモの自動 (撃たない)
void Game3::stepAim(const Input *in)
{
  float jx = 0, jy = 0;
  bool hold = false, fireP = false, scopeP = false;
  if (in && mode == Mode::Play)
  {
    jx = in->AX;
    jy = in->AY;
    if (jx == 0 && jy == 0)
    {
      jx = (in->Right ? 1 : 0) - (in->Left ? 1 : 0);
      jy = (in->Down ? 1 : 0) - (in->Up ? 1 : 0);
    }
    hold = in->Slow;
    fireP = in->FirePressed;
    scopeP = in->BombPressed;
  }
  else if (mode == Mode::Attract)
  {
    // デモ: 8 秒ごとに 見渡す → スコープで人を追う
    int ph = (modeT / (8 * FPS)) % 3;
    bool want = ph == 1;
    if (want != scope)
      scope = want;
    if (modeT % (3 * FPS) == 0 || demoTarget >= (int)peds.size())
      demoTarget = (int)(rnd() * peds.size());
    const Ped &p = peds[demoTarget];
    ax += (p.x - ax) * 0.04f;
    ay += (p.y - 7 - ay) * 0.04f;
    hold = scope && (modeT / 45) % 3 == 0;
  }
  auto curve = [](float v) { return v < 0 ? -powf(-v, 1.5f) : powf(v, 1.5f); };
  float sp = (scope ? 18.0f : 85.0f) / FPS;
  ax = clampf(ax + curve(jx) * sp, 2, 238);
  ay = clampf(ay + curve(jy) * sp, 6, 298);
  if (scopeP)
  {
    scope = !scope;
    sfx(Sfx::ScopeIn);
  }
  scopeK += ((scope ? 1.0f : 0.0f) - scopeK) * 0.3f;
  if (fabsf(scopeK - (scope ? 1 : 0)) < 0.01f)
    scopeK = scope ? 1 : 0;
  recoil *= 0.84f;
  if (reload > 0 && --reload == RELOAD - 12)
    sfx(Sfx::Bolt);
  if (flash > 0)
    flash--;
  // 息
  bool was = holding;
  holding = false;
  if (tired > 0)
  {
    tired--;
    breath = min(1.0f, breath + 1.0f / (6 * FPS));
  }
  else if (hold && breath > 0)
  {
    holding = true;
    breath -= 1.0f / (3.5f * FPS);
    if (breath <= 0)
    {
      breath = 0;
      tired = 2 * FPS;
      sfx(Sfx::Exhale);
    }
  }
  else
    breath = min(1.0f, breath + 1.0f / (5 * FPS));
  if (was && !holding && tired == 0 && mode == Mode::Play)
    sfx(Sfx::Exhale, 0.5f);
  if (holding && mode == Mode::Play)
  {
    if (--heartT <= 0)
    {
      sfx(Sfx::Heart);
      heartT = 26 - (int)((1 - breath) * 10);
    }
  }
  else
    heartT = 0;
  float tgt = tired > 0 ? 2.4f : holding ? 0.12f : 1.2f;
  amp += (tgt - amp) * (tgt < amp ? 0.2f : 0.1f);
  if (fireP && reload == 0 && ammo > 0 && clearT == 0)
    fire();
}

// ============ 描く ============
static void txt(Gfx &g, int lx, int ly, uint32_t c, const char *s, bool center = false, int size = 1)
{
  g.text(lx * 3 / 2, ly * 3 / 2, c, s, center ? Datum::TC : Datum::TL, size, 1, INK);
}
static void panel(int x, int y, int w, int h)
{
  d2::R(x, y, w, h, 0x0a0c0f, 200);
  d2::R(x, y, w, 1, GOLD);
  d2::R(x, y + h - 1, w, 1, 0x5a4a2a);
  d2::R(x, y, 1, h, 0x5a4a2a);
  d2::R(x + w - 1, y, 1, h, 0x5a4a2a);
  // 四隅の照準のかぎ
  for (int s = 0; s < 2; s++)
  {
    int cx = s ? x + w - 7 : x + 2;
    d2::R(cx, y + 2, 5, 1, GOLD);
    d2::R(s ? x + w - 3 : x + 2, y + 2, 1, 5, GOLD);
  }
}
static void arrow(int x, int y, float w, uint32_t c) // 風の向きと強さの矢印 (x が真ん中)
{
  int dir = w >= 0 ? 1 : -1;
  int len = 4 + (int)min(14.0f, fabsf(w) * 2.5f);
  d2::R(x - len / 2, y - 1, len, 2, c);
  int tip = x + dir * (len / 2 + 3);
  d2::T(tip, y, tip - dir * 4, y - 4, tip - dir * 4, y + 3, c);
}
static void cartridge(int x, int y, bool full)
{
  if (!full)
  {
    d2::R(x, y + 2, 3, 8, 0x2a2a2a);
    return;
  }
  d2::R(x, y + 3, 3, 7, 0xc8a040);
  d2::R(x, y + 3, 1, 7, 0xf0d070);
  d2::R(x, y + 9, 3, 1, 0x8a6a20);
  d2::T(x, y + 3, x + 2, y + 3, x + 1, y, 0xb06a3a);
}

void Game3::Draw(Gfx &gx)
{
  int ltop = gx.top * 2 / 3;
  Surf s;
  s.buf = gx.buf;
  s.y0 = ltop;
  s.h = LSTRIP;
  d2::S = &s;
  uint32_t t0 = micros();
  if (mode == Mode::Brief)
  {
    drawBrief(gx, ltop);
    drawOverlay(gx, ltop);
    return;
  }
  if (scopeK < 0.999f)
    drawWide(gx, ltop);
  if (scopeK > 0.02f)
    drawScope(gx, ltop);
  uint32_t t1 = micros();
  s.cx0 = 0;
  s.cx1 = W - 1;
  s.cy0 = -100000;
  s.cy1 = 100000;
  drawHud(gx, ltop);
  drawOverlay(gx, ltop);
  profObj += t1 - t0;
  profHud += micros() - t1;
}

// 景色の中の動くもの。scopeView なら 4 倍で直接、そうでなければ小さな下書きで描く
void Game3::drawScene(int ltop, bool scopeView)
{
  using namespace w3;
  float wd = wind();
  auto vis = [&](float x0, float y0, float w, float h) {
    float a = (y0 - OY) * K, b = (y0 + h - OY) * K;
    if (b < ltop || a >= ltop + LSTRIP)
      return false;
    float l = (x0 - OX) * K, r = (x0 + w - OX) * K;
    return r >= d2::S->cx0 && l <= d2::S->cx1;
  };
  auto draw = [&](float x0, float y0, float w, float h, auto fn) {
    if (!vis(x0, y0, w, h))
      return;
    if (scopeView)
      fn();
    else
    {
      miniBegin(x0, y0, w, h);
      fn();
      miniEnd();
    }
  };
  const Scene &sc = SC();
  // 窓の人とガラスのひび
  for (auto &w : wins)
    if (w.on)
    {
      const Box &b = sc.win[w.idx];
      bool lw = sc.winLitTab && sc.winLitTab[w.idx];
      draw(b.x, b.y + b.h - 10, b.w, 10, [&] { drawWinPerson(b.x, b.y, b.w, b.h, w.u, w.dir, w.lk, lw); });
    }
  for (auto &h : hits)
    if (h.kind == 3 && vis(h.x - 3, h.y - 3, 6, 6))
    {
      for (int i = 0; i < 6; i++)
      {
        float a = i * 1.047f + 0.3f, r = 2.2f + (i % 2) * 0.8f;
        L(h.x, h.y, h.x + cosf(a) * r, h.y + sinf(a) * r, 0.12f, 0xf0f4f8 | EMIT, 200);
      }
      C(h.x, h.y, 0.35f, 0x101010);
    }
  sc.dyn(0, wd, t);
  float split = sc.nLane ? sc.laneY[0] + 6 : 1e9f; // これより奥の人は車より先に描く
  // 奥の人と鳩
  for (int k : order)
  {
    const Ped &p = peds[k];
    if (p.gone || p.y > split)
      continue;
    draw(p.x - 8, p.y - 16.5f, 16, 18, [&] { drawPerson(p.x, p.y, p.dir, p.ph, p.st != 1 && p.alive, p.lk, p.alive ? 0 : p.down, p.fd); });
  }
  for (auto &b : birds)
    if (!b.fly && b.y < split)
      draw(b.x - 3, b.y - 3, 6, 3.5f, [&] { drawPigeon(b.x, b.y, b.dir, false, b.ph, sc.gulls); });
  sc.dyn(1, wd, t);
  // 車
  for (auto &c : cars)
  {
    if (c.kind >= CAR_N)
      continue;
    float y0 = sc.laneY[c.lane], top = carTop(c.kind);
    draw(c.x - 3, y0 + top - 2, carLen(c.kind) + 6, 14 - top, [&] { drawCar(c.x, y0, c.dir, c.kind, c.color, c.wheel, &c.rider, c.ph); });
  }
  // 手前の人と鳩
  for (int k : order)
  {
    const Ped &p = peds[k];
    if (p.gone || p.y <= split)
      continue;
    draw(p.x - 8, p.y - 16.5f, 16, 18, [&] { drawPerson(p.x, p.y, p.dir, p.ph, p.st != 1 && p.alive, p.lk, p.alive ? 0 : p.down, p.fd); });
  }
  for (auto &b : birds)
    if (!b.fly && b.y >= split)
      draw(b.x - 3, b.y - 3, 6, 3.5f, [&] { drawPigeon(b.x, b.y, b.dir, false, b.ph, sc.gulls); });
  // 木
  for (int i = 0; i < sc.nTree; i++)
  {
    float tx = sc.tree[i][0], ty = sc.tree[i][1], ts = sc.tree[i][2];
    draw(tx - 12 * ts, ty - 11 * ts, 24 * ts, 26 * ts, [&] { drawTree(tx, ty, ts, wd, t, i); });
  }
  // 着弾の煙と火花
  for (auto &h : hits)
  {
    if (h.kind == 3 || !vis(h.x - 4, h.y - 6, 8, 8))
      continue;
    float a = h.t / 30.0f;
    if (h.kind == 2)
    {
      if (h.t < 6)
        for (int i = 0; i < 5; i++)
        {
          float an = i * 1.25f + h.t, r = 0.6f + h.t * 0.5f;
          L(h.x, h.y, h.x + cosf(an) * r, h.y + sinf(an) * r - h.t * 0.1f, 0.12f, 0xffe28a | EMIT);
        }
    }
    else
    {
      uint32_t c = h.kind == 0 ? 0xd8ccb4 : 0xc8b898;
      int al = (int)(200 * (1 - a));
      C(h.x, h.y - a * 2.5f, 0.6f + a * 2.2f, c, al);
      C(h.x + 0.8f + a, h.y - a * 1.6f, 0.4f + a * 1.6f, c, al * 3 / 4);
      if (h.t < 8)
        for (int i = 0; i < 4; i++)
          C(h.x + (i - 1.5f) * h.t * 0.25f, h.y - h.t * 0.3f + i % 2 * 0.3f, 0.2f, shade(c, 0.7f));
    }
  }
  // 飛んでいる鳩
  for (auto &b : birds)
    if (b.fly)
      draw(b.x - 3, b.y - 3.5f, 6, 6, [&] { drawPigeon(b.x, b.y, b.dir, true, b.ph, sc.gulls); });
  sc.dyn(2, wd, t);
  if (rainNow > 0)
    drawRain(t, wd, rainNow);
}

void Game3::drawWide(Gfx &g, int ltop)
{
  for (int ly = 0; ly < LSTRIP; ly++)
  {
    int y = ltop + ly;
    if (y < w3::WH)
      memcpy(g.buf + ly * LW, w3::lo + y * LW, LW * 2);
    else
      for (int x = 0; x < LW; x++)
        g.buf[ly * LW + x] = 0x0882; // 下の表示帯の地
  }
  d2::S->cx0 = 0;
  d2::S->cx1 = W - 1;
  d2::S->cy0 = 0;
  d2::S->cy1 = w3::WH - 1;
  w3::K = 1;
  w3::OX = w3::OY = 0;
  if (ltop < w3::WH)
    drawScene(ltop, false);
  // 照準の十字 (遊んでいるとき)
  if (mode == Mode::Play && scopeK < 0.5f)
  {
    int x = lroundf(ax), y = lroundf(ay);
    for (int s : {-1, 1})
    {
      d2::R(x + s * 3 + (s < 0 ? -4 : 0), y, 5, 1, 0xffffff);
      d2::R(x, y + s * 3 + (s < 0 ? -4 : 0), 1, 5, 0xffffff);
    }
    d2::P(x, y, REDT);
  }
}

void Game3::drawScope(Gfx &g, int ltop)
{
  float k = scopeK, e = k * k * (3 - 2 * k);
  float rad = 30 + (SCR - 30) * e, cx = SCX, cy = SCY;
  bool full = k > 0.95f;
  float vx = ax + swayX(), vy = ay + swayY() - recoil;
  int hoX = lroundf(vx * w3::Z - cx), hoY = lroundf(vy * w3::Z - cy);
  w3::K = w3::Z;
  w3::OX = hoX / (float)w3::Z;
  w3::OY = hoY / (float)w3::Z;
  float r2 = rad * rad;
  // 景色を写す (円の外は黒か、暗くする)
  for (int ly = 0; ly < LSTRIP; ly++)
  {
    int Y = ltop + ly;
    uint16_t *row = g.buf + ly * LW;
    float dy = Y + 0.5f - cy;
    int xl = W, xr = -1;
    if (dy * dy < r2)
    {
      float half = sqrtf(r2 - dy * dy);
      xl = max(0, (int)ceilf(cx - half));
      xr = min(W - 1, (int)floorf(cx + half));
    }
    for (int x = 0; x < W; x++)
    {
      if (x >= xl && x <= xr)
        continue;
      if (full)
        row[x] = 0;
      else
        row[x] = d2::blend(row[x], 0, (int)(220 * e));
    }
    if (xr < xl)
      continue;
    int hy = Y + hoY;
    if (hy < 0 || hy >= w3::HH)
    {
      uint16_t c = hy < 0 ? d2::c565(w3::col(w3::tod == w3::NIGHT ? 0x050914 | w3::EMIT : w3::tod == w3::DUSK ? 0x2a3466 | w3::EMIT : 0x5e9bd0 | w3::EMIT)) : 0;
      for (int x = xl; x <= xr; x++)
        row[x] = c;
      continue;
    }
    const uint16_t *src = w3::hi + (size_t)hy * w3::HW;
    for (int x = xl; x <= xr; x++)
    {
      int hx = x + hoX;
      row[x] = hx >= 0 && hx < w3::HW ? src[hx] : 0;
    }
  }
  // 動くもの (円の四角の中だけ)
  Surf *s = d2::S;
  s->cx0 = max(0, (int)(cx - rad));
  s->cx1 = min(W - 1, (int)(cx + rad));
  s->cy0 = (int)(cy - rad);
  s->cy1 = (int)(cy + rad);
  drawScene(ltop, true);
  // 飛んでいく弾の空気のゆらぎ
  for (auto &b : bullets)
  {
    float p = 1 - (float)b.t / b.total;
    float ex = (b.x - w3::OX) * w3::K, ey = (b.y - w3::OY) * w3::K;
    float sx0 = (b.x0 - w3::OX) * w3::K, sy0 = cy + rad * 0.9f;
    float bx = sx0 + (ex - sx0) * p, by = sy0 + (ey - sy0) * p;
    d2::C(bx, by, 1 + 4 * (1 - p), 0xffffff, 40);
    d2::C(bx, by, 0.5f + 2 * (1 - p), 0xffffff, 50);
  }
  // 円の外を消し直し、縁を暗くし、撃った瞬間は明るく
  for (int ly = 0; ly < LSTRIP; ly++)
  {
    int Y = ltop + ly;
    uint16_t *row = g.buf + ly * LW;
    float dy = Y + 0.5f - cy;
    if (dy * dy >= r2)
    {
      if (full)
        memset(row, 0, LW * 2);
      continue;
    }
    float half = sqrtf(r2 - dy * dy);
    int xl = max(0, (int)ceilf(cx - half)), xr = min(W - 1, (int)floorf(cx + half));
    if (full)
    {
      for (int x = 0; x < xl; x++)
        row[x] = 0;
      for (int x = xr + 1; x < W; x++)
        row[x] = 0;
    }
    // 縁の暗がり (端から 8 点)
    for (int x = xl; x <= xr; x++)
    {
      if (x == xl + 10 && fabsf(dy) < rad - 9 && xr - 10 > x)
        x = xr - 10; // 真ん中は縁から遠いので飛ばす
      float ddx = x + 0.5f - cx, dist = rad - sqrtf(ddx * ddx + dy * dy);
      if (dist < 8)
        row[x] = dist <= 0 ? 0 : d2::blend(row[x], 0, (int)(255 * (1 - dist / 8) * (1 - dist / 8)));
    }
    if (flash > 0)
      for (int x = xl; x <= xr; x++)
        row[x] = d2::blend(row[x], 0xfff4e0, 30 * flash);
  }
  // 照準の線と目盛り
  uint32_t rc = w3::tod == w3::DAY ? 0x101010 : 0xff3a2a;
  int icx = (int)cx, icy = (int)cy, ir = (int)rad;
  if (ir > 64)
  {
    d2::R(icx - ir, icy - 1, ir - 60, 3, rc);
    d2::R(icx + 61, icy - 1, ir - 60, 3, rc);
    d2::R(icx - 1, icy + 61, 3, ir - 60, rc);
    d2::R(icx, icy - ir, 1, ir - 60, rc);
  }
  d2::R(icx - 60, icy, 121, 1, rc);
  d2::R(icx, icy - 60, 1, 121, rc);
  for (int i = -7; i <= 7; i++)
  {
    if (!i)
      continue;
    d2::R(icx - 1 + i * 8, icy - 1, 3, 3, rc);
    d2::R(icx - 1, icy - 1 + i * 8, 3, 3, rc);
  }
  if (w3::tod != w3::DAY)
    d2::C(icx, icy, 1, 0xff8070);
}

void Game3::drawHud(Gfx &g, int ltop)
{
  char s[32];
  float wd = wind();
  bool playing = mode == Mode::Play || mode == Mode::Clear || mode == Mode::Failed;
  if (!playing)
    return;
  bool sc = scopeK > 0.5f;
  if (!sc)
  {
    if (ltop + LSTRIP > 300)
    {
      d2::R(0, 300, W, 20, 0x0d1013);
      d2::R(0, 300, W, 1, 0x3a3020);
      txt(g, 4, 306, MUTE, "WIND");
      arrow(42, 310, wd, GOLD);
      snprintf(s, sizeof(s), "%.1f", fabsf(wd));
      txt(g, 56, 306, CREAM, s);
      snprintf(s, sizeof(s), "%dM", distAt(ay));
      txt(g, 104, 306, CREAM, s, true);
      for (int i = 0; i < MAX_AMMO; i++)
        cartridge(140 + i * 6, 304, i < ammo);
      d2::R(178, 307, 56, 6, 0x2a2e34);
      d2::R(178, 307, (int)(56 * breath), 6, tired > 0 ? 0xd0483d : holding ? GOLD : GREEN);
    }
    if (ltop < 24)
    {
      snprintf(s, sizeof(s), "%07d", score);
      txt(g, 4, 3, CREAM, s);
      snprintf(s, sizeof(s), "STAGE %d", stage + 1);
      txt(g, W - 4 - (int)strlen(s) * 6, 3, GOLD, s);
      snprintf(s, sizeof(s), "TIME %02d", (timeLeft + FPS - 1) / FPS);
      txt(g, W - 4 - (int)strlen(s) * 6, 13, timeLeft < 15 * FPS && (t / 8) % 2 ? REDT : CREAM, s);
      for (int i = 0; i < lives; i++) // 残り
        d2::C(8 + i * 9, 18, 3, GOLD), d2::C(8 + i * 9, 18, 1, 0x0a0c0f);
    }
  }
  else
  {
    if (ltop < 40)
    {
      snprintf(s, sizeof(s), "%dM", distAt(ay + swayY()));
      txt(g, 6, 6, CREAM, s, false, 3);
      txt(g, 6, 24, MUTE, "RANGE");
      snprintf(s, sizeof(s), "%.1fM/S", fabsf(wd));
      txt(g, W - 6 - (int)strlen(s) * 6, 6, CREAM, s);
      arrow(W - 26, 22, wd, GOLD);
      snprintf(s, sizeof(s), "TIME %02d", (timeLeft + FPS - 1) / FPS);
      txt(g, W / 2, 4, timeLeft < 15 * FPS && (t / 8) % 2 ? REDT : MUTE, s, true);
    }
    if (ltop + LSTRIP > 268)
    {
      for (int i = 0; i < MAX_AMMO; i++)
        cartridge(8 + i * 7, 300, i < ammo);
      txt(g, 150, 302, MUTE, "BREATH");
      d2::R(150, 312, 84, 5, 0x2a2e34);
      d2::R(150, 312, (int)(84 * breath), 5, tired > 0 ? 0xd0483d : holding ? GOLD : GREEN);
      if (reload > 0)
        txt(g, W / 2, 278, MUTE, "RELOAD", true);
      else if (holding)
        txt(g, W / 2, 278, GOLD, "HOLD", true);
      else if (tired > 0)
        txt(g, W / 2, 278, REDT, "GASP", true);
    }
  }
  if (panic && clearT == 0 && mode == Mode::Play && ltop < 70 && ltop + LSTRIP > 40)
  {
    if ((t / 6) % 2)
      txt(g, W / 2, sc ? 44 : 30, REDT, "TARGET RUNNING", true);
  }
}

static void clueText(const Game3::Clue &c, char *s, int n)
{
  switch (c.type)
  {
  case 0:
    snprintf(s, n, "%s", w3::HAT_NAME[c.v]);
    break;
  case 1:
    snprintf(s, n, "%s", w3::BAG_NAME[c.v]);
    break;
  case 2:
    snprintf(s, n, "GLASSES");
    break;
  default:
    snprintf(s, n, "%s", w3::COAT_NAME[c.v]);
    break;
  }
}

void Game3::drawBrief(Gfx &g, int ltop)
{
  char s[40];
  d2::R(0, ltop, W, LSTRIP, 0x0e1114);
  // 見出し
  d2::R(0, 0, W, 24, GOLD);
  snprintf(s, sizeof(s), "MISSION %d", stage + 1);
  g.text(8 * 3 / 2, 5 * 3 / 2, 0x14171c, s, Datum::TL, 3, 0, 0);
  static const char *TOD[3] = {"DAY", "DUSK", "NIGHT"};
  g.text((W - 8) * 3 / 2 - Gfx::textWidth(TOD[todNow], 1), 9 * 3 / 2, 0x14171c, TOD[todNow], Datum::TL, 1, 0, 0);
  txt(g, 10, 30, MUTE, w3::SCENES[sceneNow]->name);
  // 標的の姿 (手がかりだけ色をつけた影)
  d2::R(12, 44, 100, 128, 0x1d2228);
  d2::R(15, 47, 94, 122, 0x262c33);
  for (int y = 47; y < 169; y += 6)
    d2::R(15, y, 94, 1, 0x2c333b);
  w3::Look sil;
  sil.coat = w3::CO_SIL;
  sil.hat = w3::H_NONE;
  sil.bag = w3::B_NONE;
  sil.glasses = false;
  sil.hair = 0;
  sil.skin = 1;
  sil.pants = 3;
  for (auto &c : clues)
  {
    if (c.type == 0)
      sil.hat = c.v;
    else if (c.type == 1)
      sil.bag = c.v;
    else if (c.type == 2)
      sil.glasses = true;
    else
      sil.coat = c.v;
  }
  int keepTod = w3::tod;
  w3::tod = w3::DAY;
  w3::K = 8.5f;
  w3::OX = -60 / 8.5f;
  w3::OY = -164 / 8.5f;
  Surf *sf = d2::S;
  sf->cx0 = 15;
  sf->cx1 = 108;
  sf->cy0 = 47;
  sf->cy1 = 168;
  w3::drawPerson(0, 0, 1, 0, false, sil, 0, 1, false);
  sf->cx0 = 0;
  sf->cx1 = W - 1;
  sf->cy0 = -100000;
  sf->cy1 = 100000;
  w3::tod = keepTod;
  txt(g, 18, 50, REDT, "TARGET");
  // 手がかり
  txt(g, 122, 46, MUTE, "LOOK FOR");
  for (int i = 0; i < 3; i++)
  {
    const Clue &c = clues[i];
    int y = 62 + i * 30;
    d2::R(122, y, 14, 14, 0x2c323c);
    if (c.type == 2)
    {
      d2::R(123, y + 5, 5, 4, 0x101010);
      d2::R(130, y + 5, 5, 4, 0x101010);
      d2::R(124, y + 6, 3, 2, 0x9ab8c8);
      d2::R(131, y + 6, 3, 2, 0x9ab8c8);
      d2::R(128, y + 6, 2, 1, 0x101010);
    }
    else
    {
      uint32_t cc = c.type == 0 ? w3::HAT_C[c.v] : c.type == 1 ? w3::BAG_C[c.v] : w3::COAT_C[c.v];
      d2::R(124, y + 2, 10, 10, cc);
    }
    clueText(c, s, sizeof(s));
    txt(g, 142, y + 4, CREAM, s);
  }
  txt(g, 122, 152, MUTE, "ALL 3 MUST MATCH");
  // 条件
  d2::R(12, 182, 216, 1, 0x2c323c);
  float w0 = windBase;
  txt(g, 14, 192, MUTE, "RANGE");
  {
    const w3::Scene &sc = *w3::SCENES[sceneNow];
    int dn = 9999, df = 0;
    for (int i = 0; i < sc.nWalk; i++)
    {
      int sv = w3::sceneNo;
      w3::sceneNo = sceneNow;
      dn = min(dn, w3::distAt(sc.walk[i].y + sc.walk[i].h));
      df = max(df, w3::distAt(sc.walk[i].y));
      w3::sceneNo = sv;
    }
    snprintf(s, sizeof(s), "%d - %d M", dn, df);
    txt(g, 120, 192, CREAM, s);
    // 目安: いちばん近い距離での落ちと、風での流れ (目盛り 1 つ = 2 点)
    float drop = (dn / 250.0f) * (dn / 250.0f) / 2, drift = fabsf(windBase) * dn / 700.0f / 2;
    snprintf(s, sizeof(s), "%dM: AIM %.1f DOT HIGH", dn, drop);
    txt(g, W / 2, 260, MUTE, s, true);
    snprintf(s, sizeof(s), "WIND: %.1f DOT UPWIND", drift);
    txt(g, W / 2, 272, MUTE, s, true);
  }
  txt(g, 14, 208, MUTE, "WIND");
  arrow(130, 212, w0, GOLD);
  snprintf(s, sizeof(s), "%.1f M/S", fabsf(w0));
  txt(g, 146, 208, CREAM, s);
  txt(g, 14, 224, MUTE, "AMMO");
  for (int i = 0; i < MAX_AMMO; i++)
    cartridge(120 + i * 7, 222, true);
  txt(g, 14, 240, MUTE, "TIME LIMIT");
  snprintf(s, sizeof(s), "%d S", STAGE_TIME / FPS);
  txt(g, 120, 240, CREAM, s);
  for (int i = 0; i < lives; i++)
  {
    d2::C(20 + i * 10, 300, 3, GOLD);
    d2::C(20 + i * 10, 300, 1, 0x0a0c0f);
  }
  float p = w3::buildProgress();
  if (p < 1)
  {
    d2::R(80, 298, 100, 5, 0x2a2e34);
    d2::R(80, 298, (int)(100 * p), 5, GOLD);
    txt(g, 130, 306, MUTE, "LOADING", true);
  }
  else if ((modeT / 15) % 2 == 0)
    txt(g, 140, 296, CREAM, "PUSH SW1", true, 3);
}

void Game3::drawOverlay(Gfx &g, int ltop)
{
  char s[40];
  if (volT > 0)
  {
    panel(70, 262, 100, 22);
    txt(g, 78, 270, CREAM, VolumeLevel == 0 ? "VOL OFF" : "VOL");
    for (int i = 0; i < 4; i++)
      d2::R(110 + i * 13, 270, 10, 8, i < VolumeLevel ? GOLD : 0x2a2420);
  }
  switch (mode)
  {
  case Mode::Attract:
  {
    int ph = (modeT / (8 * FPS)) % 3;
    if (ph == 0)
    {
      panel(20, 70, 200, 150);
      txt(g, W / 2, 86, GOLD, "LONGSHOT", true, 5);
      txt(g, W / 2, 116, CREAM, "ONE SHOT, ONE TARGET", true);
      txt(g, W / 2, 132, MUTE, "FIND THEM IN THE CROWD", true);
      if ((modeT / 15) % 2 == 0)
        txt(g, W / 2, 158, CREAM, "PUSH SW1", true, 3);
      txt(g, W / 2, 200, MUTE, "SW3 HOLD: MENU", true);
    }
    else if (ph == 1)
    {
      if ((modeT / 15) % 2 == 0)
        txt(g, W / 2, 284, CREAM, "PUSH SW1", true);
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
  }
  case Mode::Play:
    if (modeT < 2 * FPS && (modeT / 8) % 2 == 0)
      txt(g, W / 2, 120, GOLD, "FIND THE TARGET", true, 3);
    if (paused)
    {
      panel(40, 120, 160, 64);
      txt(g, W / 2, 132, CREAM, "PAUSE", true, 3);
      txt(g, W / 2, 158, MUTE, "SW3: RESUME", true);
      txt(g, W / 2, 170, MUTE, "SW2: QUIT", true);
    }
    break;
  case Mode::Clear:
  {
    panel(24, 80, 192, 150);
    txt(g, W / 2, 92, GOLD, "TARGET DOWN", true, 3);
    static const char *LB[5] = {"MISSION", "HEAD SHOT", "TIME", "AMMO LEFT", "ONE SHOT"};
    int row = 0;
    for (int i = 0; i < 5; i++)
    {
      if (i > 0 && bonus[i] == 0)
        continue;
      if (modeT < 10 + row * 8)
        break;
      int y = 118 + row * 14;
      txt(g, 38, y, MUTE, LB[i]);
      snprintf(s, sizeof(s), "%6d", bonus[i]);
      txt(g, 200 - 36, y, CREAM, s);
      row++;
    }
    snprintf(s, sizeof(s), "SCORE %07d", score);
    txt(g, W / 2, 196, GOLD, s, true);
    if (modeT > 2 * FPS && (modeT / 15) % 2 == 0)
      txt(g, W / 2, 212, MUTE, "SW1: NEXT", true);
    break;
  }
  case Mode::Failed:
  {
    static const char *WHY[6] = {"", "WRONG TARGET", "TARGET ESCAPED", "OUT OF AMMO", "TIME UP", "WRONG TARGET"};
    panel(30, 110, 180, 80);
    txt(g, W / 2, 122, REDT, "MISSION FAILED", true, 3);
    txt(g, W / 2, 146, CREAM, WHY[failReason], true);
    snprintf(s, sizeof(s), "CHANCES LEFT %d", max(0, lives));
    txt(g, W / 2, 162, MUTE, s, true);
    if (modeT > FPS && (modeT / 15) % 2 == 0)
      txt(g, W / 2, 176, MUTE, "SW1: NEXT", true);
    break;
  }
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
        d2::R(x - 8, 172, 16, 2, GOLD);
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
  default:
    break;
  }
}
