// 2 本目 (廃工場) の敵: 区画ごとの出る表、動き、弱ったときの生き延びる動き、核の乗り移り、描き方。
// 形と動きは敵の図鑑のページ (廃工場の敵たち) と同じにしてある
#include "game2.h"
#include "g2bg.h"
#include "g2draw.h"
#include "g2parts.h"
#include <algorithm>

using namespace d2;
using namespace ep;
typedef Game2::Enemy Enemy;

static const int W = 240, H = 320;
static const float SNAKE_DU = 0.05f;
static const int VIRUS_FRAMES = 13 * 30; // ウイルスで止まる長さ (13 秒) // パイプヘビの節と節の間 (道のりの割合)

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float angDiff(float a, float b)
{
  float d = fmodf(a - b + PIf * 3, PIf * 2) - PIf;
  return d;
}

// ---- 種類ごとの決まり ----
static const int BASE_HP[] = {3, 5, 1, 6, 10, 8, 2, 4, 2, 1};
static const int POINTS[] = {100, 200, 50, 300, 500, 800, 200, 100, 300, 500};
static bool isAir(int k) { return k == Game2::TWIN || k == Game2::MAINT || k == Game2::CORE; }
static bool isMachine(int k) { return k != Game2::BUG && k != Game2::CORE; } // 核が乗り移れる
static bool dropsCore(int k) { return k != Game2::BUG && k != Game2::MAINT && k != Game2::CORE; }
static bool weak(const Enemy &e) { return e.hp * 3 <= e.maxhp; }
// 強くなった段 lvl だけ、撃つ間を詰める
// 撃つ間。強くなった段 lvl だけ詰め、序盤 (diff が小さい) ほど空ける
static int period(int base, int lvl, float diff) { return max(base, (int)(base * (2.2f - 0.7f * diff)) - base * lvl * 18 / 100); }
// 難しさ (0〜1)。1 周目は区画ごとに上がり、2 周目からは 1
static const float DIFF[] = {0.0f, 0.2f, 0.4f, 0.55f, 0.7f, 0.8f, 0.9f, 1.0f};

// ---- 区画ごとの出る表 ----
// z: 区画、o: 区画の下端からの位置 (画面の上端がそこまで来たら出る)、x: 横の位置、n: 数
struct Ev
{
  uint8_t z;
  uint16_t o;
  uint8_t k, x, n;
};
static constexpr uint8_t eTW = Game2::TWIN;
static constexpr uint8_t eWK = Game2::WALKER;
static constexpr uint8_t eBG = Game2::BUG;
static constexpr uint8_t eWD = Game2::WELDER;
static constexpr uint8_t eFK = Game2::FORK;
static constexpr uint8_t eSN = Game2::SNAKE;
static constexpr uint8_t eEY = Game2::EYE;
static constexpr uint8_t eSC = Game2::SCRAP;
static constexpr uint8_t eMT = Game2::MAINT;
static constexpr uint8_t bGANTRY = 100, bSTAMPER = 101, bHEARTH = 102; // ボス
static const Ev EVS[] = {
    // 0 草むらの搬入口: 少なめ。ツインファンとウォーカーから
    {0, 40, eTW, 60, 1}, {0, 110, eWK, 170, 1}, {0, 180, eTW, 150, 2}, {0, 250, eFK, 70, 1}, {0, 310, eBG, 120, 3},
    {0, 370, eTW, 90, 2},
    // 1 基礎の建屋: ボルトムシ (小さめの群れ) とメンテ
    {1, 30, eBG, 60, 4}, {1, 100, eWK, 160, 1}, {1, 160, eTW, 120, 2}, {1, 220, eBG, 170, 5}, {1, 270, eMT, 120, 1},
    {1, 330, eWK, 60, 1}, {1, 390, eTW, 90, 2}, {1, 425, eEY, 22, 1},
    // 2 ダクトと階段: ウェルダー
    {2, 30, eWD, 70, 1}, {2, 80, eTW, 160, 2}, {2, 130, eWD, 180, 1}, {2, 170, eWK, 110, 1}, {2, 220, eTW, 70, 3},
    {2, 260, eEY, 218, 1}, {2, 300, eWD, 120, 1}, {2, 330, eMT, 60, 1}, {2, 370, eWK, 60, 2}, {2, 420, eTW, 150, 2},
    {2, 470, eWD, 60, 1}, {2, 495, eWD, 180, 1},
    // 3 配管の迷路: パイプヘビ
    {3, 40, eSN, 50, 1}, {3, 110, eTW, 170, 2}, {3, 160, eBG, 170, 6}, {3, 220, eSN, 190, 1}, {3, 290, eTW, 60, 3},
    {3, 340, eBG, 70, 6}, {3, 380, eSN, 60, 1}, {3, 450, eTW, 140, 2}, {3, 505, eEY, 22, 1},
    // 4 止まったプレス: ダンゴとウェルダー
    {4, 30, eSC, 80, 1}, {4, 90, eWD, 180, 1}, {4, 140, eWK, 60, 2}, {4, 190, eSC, 170, 1}, {4, 215, bSTAMPER, 0, 1}, {4, 240, eTW, 120, 2},
    {4, 290, eWD, 50, 1}, {4, 330, eMT, 180, 1}, {4, 370, eSC, 110, 1},
    // 5 沈殿槽: 水の上なので空の敵
    {5, 30, eTW, 60, 3}, {5, 90, eTW, 180, 3}, {5, 150, eMT, 120, 1}, {5, 190, eTW, 120, 4}, {5, 250, eEY, 218, 1},
    {5, 290, eTW, 50, 3}, {5, 300, bGANTRY, 0, 1}, {5, 340, eTW, 190, 3}, {5, 400, eMT, 80, 1},
    // 6 大きな建屋: フォークとアイ
    {6, 30, eFK, 120, 1}, {6, 90, eWK, 60, 2}, {6, 140, eEY, 22, 1}, {6, 180, eFK, 180, 1}, {6, 230, eMT, 100, 1},
    {6, 260, eTW, 140, 2}, {6, 310, eWK, 170, 2}, {6, 360, eFK, 60, 1}, {6, 410, eBG, 120, 6}, {6, 450, eEY, 218, 1},
    {6, 480, eFK, 150, 1},
    // 7 溶鉱炉の跡: ダンゴ
    {7, 30, eSC, 60, 1}, {7, 80, eTW, 170, 3}, {7, 130, eWK, 110, 2}, {7, 180, eSC, 180, 1}, {7, 230, eTW, 60, 3},
    {7, 280, eSC, 120, 1}, {7, 330, eMT, 60, 1}, {7, 360, eWK, 170, 2}, {7, 400, bHEARTH, 0, 1}, {7, 420, eTW, 120, 4}, {7, 460, eSC, 70, 1},
};
static const int NEV = sizeof(EVS) / sizeof(EVS[0]);
static const int LEAD = 16; // 画面の上端より少し上で出す

static float evPos(int i, int lap) { return (float)lap * bg2::TOT + bg2::STARTS[EVS[i].z] + EVS[i].o; }

void Game2::resetSpawns(bool skipVisible)
{
  lap = 0;
  evIdx = 0;
  born.clear();
  if (skipVisible) // 始めから画面に見えている所の分は出さない
    while (evIdx < NEV && evPos(evIdx, 0) <= scroll + H + LEAD)
      evIdx++;
}

void Game2::spawnEvents()
{
  float line = scroll + H + LEAD;
  for (int guard = 0; guard < 8; guard++)
  {
    if (evIdx >= NEV)
    {
      evIdx = 0;
      lap++;
    }
    float p = evPos(evIdx, lap);
    if (p > line)
      break;
    const Ev &v = EVS[evIdx++];
    spawn(v.k, v.x, v.n, H - p);
  }
}

void Game2::skipZone()
{
  int sc = (int)scroll, TOT = bg2::TOT;
  int wu = sc + H + LEAD + 2, base = wu / TOT * TOT; // 画面の上端 (出る線) より先の、次の区画の始まり
  int next = base + TOT;
  for (int i = 0; i < bg2::NZ; i++)
    if (base + bg2::STARTS[i] > wu)
    {
      next = base + bg2::STARTS[i];
      break;
    }
  scroll = next - H - LEAD - 1; // 次の区画の始まりが、ちょうど出る線の手前に来る
  en.clear();
  born.clear();
  eb.clear();
  fx.clear();
  boss = Boss();
  spd = 0.55f;
  for (int g = 0; g < 400; g++)
  {
    if (evIdx >= NEV)
    {
      evIdx = 0;
      lap++;
    }
    if (evPos(evIdx, lap) > scroll + H + LEAD)
      break;
    evIdx++;
  }
  initFeet();
}

Enemy &Game2::addEnemy(int k, float x, float y)
{
  born.emplace_back();
  Enemy &e = born.back();
  e.k = k;
  e.id = nextId++;
  e.x = e.gx = x;
  e.y = y;
  e.t0 = t;
  int lvl = min(lap, 2);
  if (isMachine(k) && pendingLvl > 0) // 前に逃がした核が乗り移っている
  {
    pendingLvl--;
    lvl++;
  }
  e.lvl = min(3, lvl);
  int b = BASE_HP[k];
  e.hp = e.maxhp = k == BUG || k == CORE ? b : b + e.lvl * max(1, b / 2);
  return e;
}

void Game2::spawn(int k, int x, int n, float gyTop)
{
  if (k >= 100) // ボス
  {
    startBoss(k - 100);
    return;
  }
  int sc = (int)scroll;
  for (int i = 0; i < n; i++)
  {
    switch (k)
    {
    case TWIN:
    {
      Enemy &e = addEnemy(TWIN, clampf(x + (i - (n - 1) * 0.5f) * 36, 22, W - 22), -16 - i * 18);
      e.a = hs(t + i * 7) * 6;
      break;
    }
    case WALKER:
    {
      Enemy &e = addEnemy(WALKER, clampf(x + i * 46 - (n - 1) * 23, 20, W - 20), 0);
      e.gy = gyTop - i * 12;
      e.y = e.gy + sc;
      break;
    }
    case BUG:
    {
      Enemy &e = addEnemy(BUG, clampf(x + (hs(t + i * 3.3f) - 0.5f) * 40, 10, W - 10), 0);
      e.gy = gyTop - i * 8;
      e.y = e.gy + sc;
      e.grp = nextGrp;
      e.cnt = n;
      break;
    }
    case SNAKE:
    {
      Enemy &e = addEnemy(SNAKE, x, 0);
      e.gy = gyTop - 10;
      e.ux = W - x;
      e.uy = gyTop + 50;
      e.r = -0.05f; // 頭の位置 (道のりの割合)。0 で口から出る
      e.hidden = true;
      for (auto &s : e.seg)
        s = 2;
      break;
    }
    case EYE:
    {
      Enemy &e = addEnemy(EYE, x, 0);
      e.gy = gyTop;
      e.y = e.gy + sc;
      e.vx = PIf / 2 + (x < W / 2 ? -0.5f : 0.5f); // 見張る向きの真ん中 (下の、画面の内側)
      break;
    }
    case MAINT:
      addEnemy(MAINT, x, -14);
      break;
    default: // WELDER・FORK・SCRAP
    {
      Enemy &e = addEnemy(k, x, 0);
      e.gy = gyTop;
      e.y = e.gy + sc;
      if (k == SCRAP)
        e.r = 7;
      break;
    }
    }
    if (k == SNAKE || k == WELDER || k == FORK || k == EYE || k == SCRAP || k == MAINT)
      break; // 1 つずつ
  }
  if (k == BUG)
    nextGrp++;
}

Enemy *Game2::findId(int id)
{
  if (id < 0)
    return nullptr;
  for (auto &e : en)
    if (e.id == id && e.hp > 0 && !e.gone)
      return &e;
  return nullptr;
}

// パイプヘビの道のり u (0〜1) の画面の位置。口 A から、下へふくらみ、くねって、口 B へ
static void snakeAt(const Enemy &e, float u, int sc, float &x, float &y)
{
  float ax = e.gx, ay = e.gy + sc, bx = e.ux, by = e.uy + sc;
  float dx = bx - ax, dy = by - ay, l = max(1.0f, hypotf(dx, dy));
  float nx = -dy / l, ny = dx / l, w = sinf(u * 2 * PIf) * 22;
  x = clampf(ax + dx * u + nx * w, 8, W - 8);
  y = ay + dy * u + ny * w + sinf(u * PIf) * 70;
}

// ---- 動き ----
void Game2::enemyStep()
{
  int sc = (int)scroll;
  diff = lap > 0 ? 1.0f : DIFF[min(zoneAt(H / 2), 7)];
  spawnEvents();
  en.insert(en.end(), born.begin(), born.end());
  born.clear();

  // ボルトムシの群れの残り
  int alive[32] = {};
  for (auto &e : en)
    if (e.k == BUG && e.hp > 0)
      alive[e.grp & 31]++;

  for (auto &b : shots)
  {
    b.x += b.dx;
    b.y += b.dy;
    if (b.y < -10 || b.x < -10 || b.x > W + 10)
      b.dead = true;
  }
  for (auto &e : en)
  {
    if (e.k == BUG && e.st == 0 && alive[e.grp & 31] * 2 <= e.cnt)
      e.st = 1; // 群れが半分になったので散る
    stepOne(e);
  }

  // 自分の弾が当たる
  for (auto &b : shots)
  {
    if (b.dead)
      continue;
    if (bossShot(b.x, b.y))
    {
      b.dead = true;
      continue;
    }
    for (auto &e : en)
    {
      if (e.hp <= 0 || e.gone || (e.hidden && e.k != SNAKE))
        continue;
      int part = -1;
      float dx = b.x - e.x, dy = b.y - e.y;
      switch (e.k)
      {
      case BUG:
        part = fabsf(dx) < 7 && fabsf(dy) < 7 ? 0 : -1;
        break;
      case FORK:
        part = fabsf(dx) < 12 && dy > -16 && dy < 27 ? 0 : -1;
        break;
      case SCRAP:
        part = hypotf(dx, dy) < e.r + 2 ? 0 : -1;
        break;
      case EYE:
      case CORE:
        part = hypotf(dx, dy) < 7 ? 0 : -1;
        break;
      case MAINT:
        part = hypotf(dx, dy) < 9 ? 0 : -1;
        break;
      case SNAKE:
        if (!e.hidden && hypotf(dx, dy) < 8)
          part = 0;
        else
          for (int i = 0; i < 8 && part < 0; i++)
          {
            float u = e.r - (i + 1) * SNAKE_DU, sx, sy;
            if (!e.seg[i] || u < 0 || u > 1)
              continue;
            snakeAt(e, u, sc, sx, sy);
            if (hypotf(b.x - sx, b.y - sy) < 7)
              part = i + 1;
          }
        break;
      default: // TWIN・WALKER・WELDER
        part = fabsf(dx) < 11 && fabsf(dy) < 11 ? 0 : -1;
        break;
      }
      if (part >= 0)
      {
        b.dead = true;
        hitEnemy(e, part, b.y);
        break;
      }
    }
  }
  en.insert(en.end(), born.begin(), born.end()); // 壊れたところから出た核
  born.clear();
  en.erase(std::remove_if(en.begin(), en.end(), [](const Enemy &e) { return e.hp <= 0 || e.gone; }), en.end());

  for (auto &b : eb)
  {
    b.x += b.dx;
    b.y += b.dy;
    if (b.life > 0 && --b.life == 0)
      b.dead = true;
    if (b.y > H + 8 || b.y < -8 || b.x < -8 || b.x > W + 8)
      b.dead = true;
  }
  for (auto &f : fx)
  {
    f.t++;
    if (f.ground)
      f.y += spd;
  }
  fx.erase(std::remove_if(fx.begin(), fx.end(), [](const Fx &f) { return f.t > f.life; }), fx.end());
}

void Game2::stepOne(Enemy &e)
{
  int sc = (int)scroll, age = t - e.t0;
  bool canShoot = tankOn && enterT == 0 && e.y > 0 && e.y < H - 60;
  float sp = 1.5f + 0.5f * diff + 0.25f * e.lvl; // 弾の速さ (序盤は遅い)
  float toT = atan2f(ty - e.y, tx - e.x), dist = hypotf(tx - e.x, ty - e.y);
  auto shoot = [&](float x, float y, float a, float s, int k, int life) {
    if ((int)eb.size() < 14 + (int)(8 * diff)) // 画面の敵の弾は多くなりすぎないように
    {
      eb.push_back({x, y, cosf(a) * s, sinf(a) * s, false, k, life});
      if (k != 3)
        sfx(Sfx::EnemyShot, 0.5f);
    }
  };
  auto tick = [&](int base) { return age > 20 && (age + e.id * 7) % period(base, e.lvl, diff) == 0; };
  if (e.hit)
    e.hit--;
  if (e.virus > 0) // ウイルスで止まっている: 床の敵は床と一緒に流れるだけ、空の敵はゆっくり落ちる
  {
    e.virus--;
    e.t0++; // 動きの時間も止める
    if (e.k == SNAKE)
      snakeAt(e, clampf(e.r, 0, 1), sc, e.x, e.y);
    else if (isAir(e.k))
      e.y += 0.3f;
    else
    {
      e.x = e.gx;
      e.y = e.gy + sc;
    }
    if (e.k == TWIN)
      e.gx = e.x - 26 * sinf((t - e.t0) * 0.03f + e.a); // 動き出したときに跳ばないように
    if (e.y > H + 40)
      e.gone = true;
    return;
  }

  switch (e.k)
  {
  case TWIN:
    if (e.st == 0)
    {
      e.y += 0.9f;
      e.x = e.gx + 26 * sinf(age * 0.03f + e.a);
      if (tick(70) && canShoot)
        for (float d : {-0.35f, 0.0f, 0.35f})
          shoot(e.x, e.y + 4, toT + d, sp, 0, 0);
      if (weak(e)) // 片方の羽根が止まって、近い端へ逃げる
      {
        e.st = 1;
        e.vx = e.x < W / 2 ? -1.6f : 1.6f;
      }
    }
    else
    {
      e.x += e.vx;
      e.y += 0.25f;
      if (e.x < -24 || e.x > W + 24)
        e.gone = true;
    }
    break;

  case WALKER:
  {
    if (e.st == 0)
    {
      bool walking = age % 120 < 70;
      if (walking)
        e.gy += 0.35f;
      int c = age % 120;
      if (canShoot && ((c == 80 && ((age / 120 + e.id) % 2 == 0 || e.lvl > 0)) || (e.lvl > 1 && c == 100))) // 止まったとき、2 回に 1 回撃つ
        for (float o : {-2.0f, 2.0f})
          shoot(e.x + o, e.y + 8, toT, sp, 0, 0);
      if (weak(e)) // 端の物陰へ歩いて隠れる
        e.st = 1;
    }
    else
    {
      e.gx += e.gx < W / 2 ? -0.9f : 0.9f;
      if (e.gx < -20 || e.gx > W + 20)
        e.gone = true;
    }
    e.x = e.gx;
    e.y = e.gy + sc;
    break;
  }

  case BUG:
  {
    float dx = 0, dy = 1;
    if (tankOn)
    {
      dx = cosf(toT);
      dy = sinf(toT);
    }
    float s = 0.75f;
    if (e.st == 1) // 散り散りに逃げる
    {
      dx = -dx;
      dy = -dy;
      s = 1.8f;
    }
    float w = sinf(age * 0.15f + e.id) * 0.6f;
    float mx = (dx - dy * w) * s, my = (dy + dx * w) * s;
    e.gx += mx;
    e.gy += my - (e.st == 0 ? 0 : 0.28f);
    e.a = atan2f(my + spd, mx);
    e.x = e.gx;
    e.y = e.gy + sc;
    if (e.st == 1 && (e.x < -12 || e.x > W + 12 || e.y < -12))
      e.gone = true;
    break;
  }

  case WELDER:
  {
    e.x = e.gx;
    e.y = e.gy + sc;
    if (age % 25 == 0 && e.cnt > 0)
      e.cnt--; // 続けて撃たれた数は少しずつ忘れる
    if (e.st == 1) // 腕をたたんで隠れている
    {
      if (--e.tm <= 0)
        e.st = 0;
      break;
    }
    float aim = clampf(tankOn ? toT : PIf / 2, 0.3f, PIf - 0.3f);
    if (age < 2)
      e.a = aim;
    e.a += (aim + 0.4f * sinf(age * 0.05f) - e.a) * 0.08f;
    if (tick(50) && canShoot)
    {
      float a2 = e.a + 0.5f * sinf(age * 0.05f + 1);
      float tipx = e.x + cosf(e.a) * 14 + cosf(a2) * 12, tipy = e.y + sinf(e.a) * 14 + sinf(a2) * 12;
      shoot(tipx, tipy, atan2f(ty - tipy, tx - tipx), sp + 0.3f, 1, 0);
      e.tm = 6; // 先の火花
    }
    else if (e.tm > 0)
      e.tm--;
    break;
  }

  case FORK:
  {
    if (e.st == 0) // エンジンをふかす
    {
      if (e.tm == 0 && e.y > 0)
        sfx(Sfx::Growl, 0.35f);
      if (++e.tm >= 40)
      {
        e.st = 1;
        e.tm = 0;
        e.r = 0;
        e.a = clampf(tankOn ? toT : PIf / 2, PIf / 2 - 0.5f, PIf / 2 + 0.5f);
      }
    }
    else if (e.st == 1) // 一直線に突進
    {
      e.r = min(3.0f + 2.0f * diff, e.r + 0.35f); // 序盤は突進が遅い
      e.gx = clampf(e.gx + cosf(e.a) * e.r, 14, W - 14);
      e.gy += sinf(e.a) * e.r;
      if (++e.tm >= 22)
      {
        e.st = 2;
        e.tm = 0;
      }
    }
    else if (e.st == 2) // ゆっくり下がる
    {
      e.gy -= 0.75f;
      if (++e.tm >= 60)
      {
        e.st = 0;
        e.tm = 0;
      }
    }
    else // 3: 木箱を盾にして下がる
      e.gy -= 0.7f;
    if (e.st != 3 && e.hp * 5 <= e.maxhp * 2)
    {
      e.st = 3;
      e.cnt = 4; // 木箱の耐久
    }
    e.x = e.gx;
    e.y = e.gy + sc;
    break;
  }

  case SNAKE:
  {
    if (e.gy + sc > 30) // 口が画面に入ってから出てくる
      e.r += e.st == 1 ? 0.02f : 0.006f;
    float u = clampf(e.r, 0, 1);
    snakeAt(e, u, sc, e.x, e.y);
    e.hidden = e.r < 0 || e.r > 1;
    bool any = false;
    for (int i = 0; i < 8; i++)
      if (e.seg[i] && e.r - (i + 1) * SNAKE_DU <= 1)
        any = true;
    if (e.r > 1 && (!any || e.st == 1))
      e.gone = true; // 口 B へ潜りきった
    if (e.gy + sc > H + 140)
      e.gone = true;
    if (!e.hidden && e.st == 0 && dist < 70 && canShoot && age % 50 == 0) // 頭のバルブから蒸気
    {
      for (float d : {-0.3f, 0.0f, 0.3f})
        shoot(e.x, e.y, toT + d, 2.2f, 3, 18);
      sfx(Sfx::Steam);
    }
    break;
  }

  case EYE:
  {
    e.x = e.gx;
    e.y = e.gy + sc;
    if (e.st == 0) // 首を振って見張る
    {
      e.r += 0.03f;
      e.a = e.vx + 0.9f * sinf(e.r);
      if (tankOn && enterT == 0 && e.y > 0 && dist < 230 && fabsf(angDiff(toT, e.a)) < 0.28f)
      {
        e.st = 1;
        e.tm = 0;
        sfx(Sfx::Alarm);
        if (!e.alerted) // 仲間に知らせる: ツインファンが 2 機来る
        {
          e.alerted = true;
          for (int s : {-1, 1})
          {
            Enemy &n = addEnemy(TWIN, clampf(tx + s * 50, 22, W - 22), -16 - (s > 0 ? 20 : 0));
            n.a = hs(t + s) * 6;
          }
        }
      }
    }
    else if (e.st == 1) // 見つけた: 戦車を目で追いながら、赤く点滅して知らせる (撃ってはこない)
    {
      e.a += clampf(angDiff(toT, e.a), -0.04f, 0.04f);
      if (++e.tm >= 45)
      {
        e.st = 3;
        e.tm = 0;
      }
    }
    else if (++e.tm >= 40)
    {
      e.st = 0;
      e.r = asinf(clampf((e.a - e.vx) / 0.9f, -1, 1));
    }
    break;
  }

  case SCRAP:
  {
    e.r = min(18.0f, e.r + 11.0f / 300);
    e.maxhp = 4 + (int)((e.r - 7) * 12 / 11) + e.lvl * 2;
    e.hp = e.maxhp - e.cnt;
    if (tankOn)
      e.gx += clampf(tx - e.gx, -0.3f, 0.3f);
    e.gy += 0.25f;
    e.a += 0.03f;
    e.x = e.gx;
    e.y = e.gy + sc;
    if (e.r > 14 && tick(40) && canShoot) // 大きくなると破片を飛ばす
      for (float d : {-0.35f, 0.0f, 0.35f})
        shoot(e.x, e.y, toT + d, sp, 0, 0);
    break;
  }

  case MAINT:
  {
    if (e.st == 0)
    {
      for (auto &b : shots) // 狙われたら全力で逃げる
        if (!b.dead && hypotf(b.x - e.x, b.y - e.y) < 34)
        {
          e.st = 1;
          e.tm = 30;
          float a = atan2f(e.y - ty, e.x - tx) + (hs(t + e.id) - 0.5f);
          e.vx = cosf(a) * 2.6f;
          e.vy = sinf(a) * 2.6f;
          break;
        }
    }
    if (e.st == 1)
    {
      e.x += e.vx;
      e.y += e.vy;
      if (--e.tm <= 0)
        e.st = 0;
      if (e.x < -20 || e.x > W + 20 || e.y < -30)
        e.gone = true;
      break;
    }
    // いちばん傷ついた仲間に付く
    Enemy *tg = findId(e.link);
    if (!tg || tg->hp >= tg->maxhp)
    {
      tg = nullptr;
      float worst = 1;
      for (auto &o : en)
        if (o.hp > 0 && !o.gone && isMachine(o.k) && o.k != MAINT && o.y > 0 && o.y < H && o.hp < o.maxhp)
        {
          float q = (float)o.hp / o.maxhp;
          if (q < worst)
          {
            worst = q;
            tg = &o;
          }
        }
      e.link = tg ? tg->id : -1;
    }
    if (tg)
    {
      float gx = tg->x + 14, gy = tg->y - 16, d = hypotf(gx - e.x, gy - e.y);
      if (d > 1)
      {
        float s = min(1.6f, d);
        e.x += (gx - e.x) / d * s;
        e.y += (gy - e.y) / d * s;
      }
      if (d < 10 && ++e.tm >= 40)
      {
        e.tm = 0;
        tg->hp = min(tg->maxhp, tg->hp + 1);
        if (tg->k == SCRAP)
          tg->cnt = max(0, tg->cnt - 1);
        e.cnt = 10; // 直した光
      }
    }
    else
    {
      e.x += sinf(age * 0.04f) * 0.6f;
      e.y += 0.6f;
    }
    if (e.cnt > 0)
      e.cnt--;
    break;
  }

  case CORE:
  {
    if (e.st == 0 && e.link == -2) // 炉心に呼ばれた核: 炉心の目へ。着いたら炉心が直る
    {
      if (boss.k != 2 || boss.dieT > 0)
        e.st = 1;
      else
      {
        float gx = bossEyeX(), gy = bossEyeY(), d = hypotf(gx - e.x, gy - e.y);
        float ux = (gx - e.x) / max(d, 1.0f), uy = (gy - e.y) / max(d, 1.0f), z = cosf(age * 0.3f) * 1.6f;
        e.vx = ux * 1.5f - uy * z;
        e.vy = uy * 1.5f + ux * z;
        e.x += e.vx;
        e.y += e.vy;
        if (d < 8)
        {
          boss.hp[0] = min(boss.maxhp[0], boss.hp[0] + 6);
          sfx(Sfx::Possess, 0.8f);
          fx.push_back({4, gx, gy, 0, 0, 16, false});
          e.gone = true;
        }
      }
      break;
    }
    if (e.st == 0)
    {
      Enemy *tg = findId(e.link);
      if (!tg)
      {
        float bd = 260;
        for (auto &o : en)
          if (o.hp > 0 && !o.gone && o.virus == 0 && isMachine(o.k) && o.y > -10 && o.y < H + 10)
          {
            float d = hypotf(o.x - e.x, o.y - e.y);
            if (d < bd)
            {
              bd = d;
              tg = &o;
            }
          }
        e.link = tg ? tg->id : -1;
      }
      if (!tg || age > 260)
        e.st = 1; // 乗り移れる機械がない。上へ逃げる
      else
      {
        float d = hypotf(tg->x - e.x, tg->y - e.y), ux = (tg->x - e.x) / max(d, 1.0f), uy = (tg->y - e.y) / max(d, 1.0f);
        float z = cosf(age * 0.3f) * 1.6f; // ジグザグ
        e.vx = ux * 1.7f - uy * z;
        e.vy = uy * 1.7f + ux * z;
        e.x += e.vx;
        e.y += e.vy;
        if (d < 7) // 乗り移った: その機械が強くなる
        {
          int inc = max(1, BASE_HP[tg->k] / 2);
          tg->lvl = min(3, tg->lvl + 1);
          tg->maxhp += inc;
          tg->hp += inc;
          if (tg->k == SCRAP)
            tg->cnt = max(0, tg->cnt - inc);
          if (tg->k == WALKER || tg->k == TWIN || tg->k == FORK)
            tg->st = min(tg->st, tg->k == FORK ? 2 : 0); // 弱って逃げていたのをやめる
          fx.push_back({4, tg->x, tg->y, 0, 0, 16, !isAir(tg->k)});
          sfx(Sfx::Possess);
          e.gone = true;
        }
      }
    }
    if (e.st == 1)
    {
      e.vx = sinf(age * 0.3f) * 1.2f;
      e.vy = -2;
      e.x += e.vx;
      e.y += e.vy;
      if (e.y < -16)
      {
        e.gone = true;
        sfx(Sfx::Possess, 0.4f);
        if (diff >= 0.4f) // 序盤は逃がしても強くならない
          pendingLvl = min(3, pendingLvl + 1); // 次に出る機械に乗り移る (たまるのは 3 つまで)
      }
    }
    break;
  }
  }
  if (!e.gone && e.k != SNAKE && (e.y > H + 40 || e.y < -110 || e.x < -60 || e.x > W + 60))
    e.gone = true;
}

void Game2::hitEnemy(Enemy &e, int part, float by)
{
  int sc = (int)scroll;
  auto spark = [&](float x, float y) {
    fx.push_back({3, x, y, hs(t) * 6, 0, 5, false});
    sfx(Sfx::Ping);
  };
  if (e.k == WELDER && e.st == 1) // たたんだ腕に弾が効かない
  {
    spark(e.x, e.y - 4);
    return;
  }
  if (e.k == FORK && e.st == 3 && e.cnt > 0 && by > e.y + 10) // 木箱が受け止める
  {
    spark(e.x + (hs(t) - 0.5f) * 14, e.y + 16);
    if (--e.cnt == 0)
      fx.push_back({5, e.x, e.y + 20, hs(t) * 6, 0, 14, true});
    return;
  }
  if (e.k == SNAKE && part > 0) // 節が壊れたら、残りの節を切り捨てて頭だけで逃げる
  {
    int i = part - 1;
    e.hit = 3;
    if (--e.seg[i] > 0)
      return;
    for (int j = 0; j < 8; j++)
    {
      float u = e.r - (j + 1) * SNAKE_DU, sx, sy;
      if (!e.seg[j] && j != i)
        continue;
      if (u >= 0 && u <= 1)
      {
        snakeAt(e, u, sc, sx, sy);
        fx.push_back({5, sx, sy, hs(t + j) * 6, -j * 2, 14, true});
        if (mode == Mode::Play)
          score += 50;
      }
      e.seg[j] = 0;
    }
    sfx(Sfx::Flak);
    e.st = 1;
    return;
  }
  e.hit = 3;
  if (e.k == SCRAP)
  {
    e.cnt++;
    e.hp = e.maxhp - e.cnt;
  }
  else
    e.hp--;
  if (e.hp > 0)
    sfx(Sfx::MomHit, 0.3f);
  if (e.k == WELDER && e.hp > 0 && ++e.cnt >= 3) // 続けて撃たれたので隠れる
  {
    e.st = 1;
    e.tm = 75;
    e.cnt = 0;
  }
  if (e.hp <= 0)
    killEnemy(e);
}

void Game2::killEnemy(Enemy &e)
{
  e.hp = 0;
  int p = POINTS[e.k];
  if (e.k == SCRAP)
    p = 100 + (int)((e.r - 7) * 40);
  if (mode == Mode::Play)
    score += p * (2 + e.lvl) / 2;
  bool ground = !isAir(e.k) && e.k != EYE;
  if (e.k == CORE)
    sfx(Sfx::CoreKill);
  else if (e.k == BUG || e.k == MAINT)
    sfx(Sfx::Flak, 0.7f);
  else
    sfx(Sfx::G2Boom, e.k == FORK || e.k == SCRAP || e.k == SNAKE ? 1 : 0.7f);
  if (e.k == BUG || e.k == CORE || e.k == MAINT)
    fx.push_back({5, e.x, e.y, hs(t) * 6, 0, 16, ground});
  else
    fx.push_back({0, e.x, e.y, hs(t) * 6, 0, 26, ground || e.k == EYE});
  if (e.k == FORK || e.k == SCRAP || e.k == SNAKE)
    fx.push_back({0, e.x + 8, e.y - 6, hs(t + 3) * 6, -6, 26, true});
  if (dropsCore(e.k)) // 赤い核が飛び出す
  {
    Enemy &c = addEnemy(CORE, e.x, e.y);
    c.link = -1;
    c.lvl = 0;
    sfx(Sfx::CoreFree);
  }
  // 弾を強くする部品: 大きい機械 (フォーク・ウェルダー・パイプヘビ) と、部品を運ぶメンテが落とす。ほかは 15 体ごと
  bool every = e.k != CORE && e.k != BUG && ++killCount % 15 == 0;
  if (e.k == FORK || e.k == WELDER || e.k == SNAKE || e.k == MAINT || every)
    dropItem(e.x, e.y);
}

// 爆弾の爆発: まわり (半径 BLAST) の敵に 2 発ぶんの傷。上から落ちるので、フォークの木箱は効かない
void Game2::bombBlast(float x, float y)
{
  const float BLAST = 38;
  int sc = (int)scroll;
  fx.push_back({6, x, y, 0, 0, 14, true});
  sfx(Sfx::BigBoom, 0.6f);
  sfx(Sfx::Virus);
  fx.push_back({0, x, y, hs(t) * 6, 0, 26, true});
  for (int i = 0; i < 3; i++)
    fx.push_back({5, x + (hs(t + i) - 0.5f) * 40, y + (hs(t * 3 + i) - 0.5f) * 30, hs(t + i * 5) * 6, -3 - i * 3, 14, true});
  size_t n = en.size(); // 傷で生まれた核は born に入るので、en は増えない
  for (size_t i = 0; i < n; i++)
  {
    Enemy &e = en[i];
    if (e.hp <= 0 || e.gone || e.k == CORE)
      continue;
    if (e.k == SNAKE)
    {
      for (int j = 0; j < 8; j++)
      {
        float u = e.r - (j + 1) * SNAKE_DU, sx, sy;
        if (!e.seg[j] || u < 0 || u > 1)
          continue;
        snakeAt(e, u, sc, sx, sy);
        if (hypotf(sx - x, sy - y) < BLAST)
        {
          hitEnemy(e, j + 1, sy);
          break; // 節が 1 つ壊れると、残りは切り捨てられる
        }
      }
      if (e.hidden)
        continue;
    }
    float r = e.k == SCRAP ? e.r : 8;
    if (hypotf(e.x - x, e.y - y) > BLAST + r)
      continue;
    for (int k = 0; k < 2 && e.hp > 0; k++)
      hitEnemy(e, 0, e.y);
    if (e.hp > 0 && e.k != CORE) // 生き残った機械には、ウイルスを仕掛ける
      e.virus = VIRUS_FRAMES;
  }
  bossBomb(x, y);
}

void Game2::dropItem(float x, float y)
{
  if (items.size() < 3)
    items.push_back({clampf(x, 16, W - 16), y, 0});
}

bool Game2::touchesTank(const Enemy &e) const
{
  if (e.hp <= 0 || e.gone || e.virus > 0)
    return false;
  float dx = e.x - tx, dy = e.y - ty;
  switch (e.k)
  {
  case BUG:
    return fabsf(dx) < 8 && fabsf(dy) < 8;
  case WALKER:
    return fabsf(dx) < 14 && fabsf(dy) < 14;
  case WELDER:
    return fabsf(dx) < 12 && fabsf(dy) < 12;
  case FORK:
    return fabsf(dx) < 14 && dy > -30 && dy < 18; // 車体と爪 (爪は下に出ている)
  case SCRAP:
    return hypotf(dx, dy) < e.r + 6;
  case SNAKE:
  {
    if (!e.hidden && hypotf(dx, dy) < 9)
      return true;
    int sc = (int)scroll;
    for (int i = 0; i < 8; i++)
    {
      float u = e.r - (i + 1) * SNAKE_DU, sx, sy;
      if (!e.seg[i] || u < 0 || u > 1)
        continue;
      snakeAt(e, u, sc, sx, sy);
      if (hypotf(sx - tx, sy - ty) < 8)
        return true;
    }
    return false;
  }
  default:
    return false;
  }
}

// ---- 描く部品 (ボスも使う。g2parts.h) ----
namespace ep
{
void eye(float x, float y, float r, int t, bool flash, bool weakFlicker)
{
  if (weakFlicker && ((t >> 1) & 1)) // 弱ると目がちらつく
  {
    C(x, y, r, 0x2a0a08);
    return;
  }
  C(x, y, r + 3, RED, 26);
  C(x, y, r + 1, RED, 56);
  C(x, y, r, flash ? 0xffffff : RED2);
  if (r > 1)
    C(x, y, r - 1, flash ? 0xffffff : RED);
  P(x - (r > 2 ? 1 : 0), y - (r > 2 ? 1 : 0), REDW);
}

// 機械に這う赤い配線 (光が流れる)
void nerve(std::initializer_list<Pt> pts, int t, float ph)
{
  int i = 0;
  const Pt *p = pts.begin();
  for (size_t s = 0; s + 1 < pts.size(); s++)
  {
    float l = hypotf(p[s + 1].x - p[s].x, p[s + 1].y - p[s].y);
    int n = max(1, (int)ceilf(l / 1.2f));
    for (int k = 0; k < n; k++, i++)
    {
      float x = p[s].x + (p[s + 1].x - p[s].x) * k / n, y = p[s].y + (p[s + 1].y - p[s].y) * k / n;
      float m = fmodf(i - t * 0.6f + ph, 14);
      if (m < 0)
        m += 14;
      P(x, y, m < 2 ? RED : RED3);
    }
  }
}

void sparks(float x, float y, int t, int n)
{
  for (int k = 0; k < n; k++)
    if (hs(t * 3.1f + k) > 0.5f)
      P(x + (hs(t + k) - 0.5f) * 14, y + (hs(t * 1.7f + k) - 0.5f) * 14, k % 2 ? 0xffe27a : REDW);
}

void block(float fx, float fgy, int w, int d, float fhh, uint32_t top, uint32_t front)
{
  int x = lroundf(fx), gy = lroundf(fgy), hh = lroundf(fhh);
  for (int r = 0; r <= hh; r++)
    R(x + w, gy - hh + d + r, lroundf(r * 0.5f) + 1, 1, 0, 82);
  R(x + 2, gy + d, w + lroundf(hh * 0.5f) - 1, 2, 0, 82);
  R(x, gy - hh, w, d, top);
  R(x, gy - hh, w, 1, 0xffffff, 46);
  R(x, gy - hh, 1, d, 0xffffff, 26);
  if (hh > 0)
  {
    R(x, gy - hh + d, w, hh, front);
    R(x, gy - hh + d, w, 1, 0, 77);
  }
  R(x + w - 1, gy - hh, 1, d + hh, 0, 64);
}
} // namespace ep

static void drawTwin(float x, float y, int t, bool flash, bool hurt, bool broken)
{
  float tilt = broken ? 3 : 0; // 止まった羽根の側が下がる
  C(x + 9, y + 18, 7, 0, 71);
  for (int s : {-1, 1})
  {
    float rx = x + s * 11, ry = y + (s > 0 ? tilt : -tilt * 0.3f);
    bool stop = broken && s > 0;
    LT(x + s * 3, y, rx, ry, 2, 0x55585b);
    if (!stop)
      C(rx, ry, 7, 0xa0a0a0, 46);
    CR(rx, ry, 7, 0x8a8f94);
    CR(rx, ry, 6, 0x3a3d40);
    for (int k = 0; k < 2; k++)
    {
      float a = (stop ? 0.7f : t * 0.9f * s) + k * PIf;
      L(rx - cosf(a) * 6, ry - sinf(a) * 6, rx + cosf(a) * 6, ry + sinf(a) * 6, stop ? 0x8a8f94 : 0xc4c9cc);
    }
    C(rx, ry, 1, 0x2a2c2e);
  }
  C(x, y + 1, 5, 0x2a2c2e);
  C(x, y, 4, flash ? 0xc4c9cc : 0x6b7075);
  C(x - 1, y - 1, 2, 0xa3a8ac);
  R(x - 2, y - 6, 4, 3, 0x4a4e52);
  eye(x, y + 2, 2, t, flash, hurt);
  nerve({{x - 3, y + 4}, {x - 4, y + 9}}, t);
  nerve({{x + 3, y + 4}, {x + 4, y + 9}}, t, 6);
}

static void drawWalker(float x, float y, int t, int at, bool flash, bool hurt)
{
  C(x + 3, y + 4, 10, 0, 89);
  static const int LEG[4][3] = {{-1, -1, 0}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 0}};
  for (auto &l : LEG)
  {
    float st = sinf(at * 0.2f + l[2] * PIf) * 2;
    float hx = x + l[0] * 6, hy = y + l[1] * 4, kx = x + l[0] * 11, ky = y + l[1] * 4 - 3, fx = x + l[0] * 13, fy = y + l[1] * 10 + st;
    LT(hx, hy, kx, ky, 3, 0x55585b);
    L(hx, hy - 1, kx, ky - 1, 0x9aa0a5);
    LT(kx, ky, fx, fy, 2, 0x2a2c2e);
    C(kx, ky, 1, 0x8a8f94);
    R(fx - 2, fy - 1, 4, 2, 0x1a1b1c);
  }
  R(x - 8, y - 8, 16, 15, flash ? 0xd8d0c0 : 0x8a7a62);
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
  eye(x, y - 1, 2, t, flash, hurt);
  L(x + 5, y - 8, x + 7, y - 14, 0x2a2c2e);
  P(x + 7, y - 15, (t >> 3) & 1 ? RED : 0x7a1a10);
  nerve({{x - 8, y - 2}, {x - 12, y + 2}}, t);
  nerve({{x + 8, y - 2}, {x + 12, y + 2}}, t, 6);
}

static void drawBug(float x, float y, float a, int t, int i, bool flash)
{
  float ca = cosf(a), sa = sinf(a);
  C(x + 1, y + 2, 5, 0, 89);
  for (int k = 0; k < 4; k++)
  {
    int s = k < 2 ? -1 : 1, f = k % 2 ? 1 : -1;
    float ph = sinf(t * 0.5f + i + (((k % 2) ^ (k < 2 ? 0 : 1)) ? PIf : 0));
    float lx = f * 2, ly = s * 3, ex = f * (4 + ph * 1.6f), ey = s * 7;
    L(x + lx * ca - ly * sa, y + lx * sa + ly * ca, x + ex * ca - ey * sa, y + ex * sa + ey * ca, 0x2a2c2e);
    P(x + ex * ca - ey * sa, y + ex * sa + ey * ca, 0x6b7075);
  }
  C(x, y, 5, flash ? 0xd8dcdf : 0x6b7075);
  C(x - 1, y - 1, 4, 0x8a8f94);
  C(x - 2, y - 2, 1, 0xc4c9cc);
  C(x, y, 2, 0x141516);
  eye(x + ca, y + sa, 1, t, flash, false);
  L(x - ca * 4, y - sa * 4, x - ca * 8 + sinf(t * 0.3f + i), y - sa * 8, RED3);
}

static void drawWelder(float x, float y, float a1, float a2, bool folded, bool tipFlash, int t, bool flash, bool hurt)
{
  block(x - 11, y - 1, 22, 14, 6, 0x5a5e62, 0x3a3d40);
  Pt S = {x, y}, E, T;
  if (folded) // 腕をたたんで、土台の上に伏せる
  {
    E = {x + 7, y + 3};
    T = {x - 2, y + 5};
  }
  else
  {
    E = {x + cosf(a1) * 14, y + sinf(a1) * 14};
    T = {E.x + cosf(a2) * 12, E.y + sinf(a2) * 12};
  }
  LT(S.x + 3, S.y + 6, E.x + 3, E.y + 6, 4, 0, 77);
  LT(E.x + 3, E.y + 6, T.x + 3, T.y + 6, 3, 0, 77);
  C(x, y, 7, 0x8a7020);
  C(x, y, 5, flash ? 0xf0e0a0 : 0xc9a227);
  LT(S.x, S.y, E.x, E.y, 4, 0xa8641e);
  L(S.x - 1, S.y - 1, E.x - 1, E.y - 1, 0xd8954a);
  C(E.x, E.y, 3, 0x5a5e62);
  P(E.x - 1, E.y - 1, 0x9aa0a5);
  LT(E.x, E.y, T.x, T.y, 3, 0xa8641e);
  L(E.x - 1, E.y - 1, T.x - 1, T.y - 1, 0xd8954a);
  C(T.x, T.y, 2, 0x2a2c2e);
  nerve({S, E, T}, t);
  if (folded)
  {
    R(x - 6, y - 4, 12, 3, 0x8a8f94); // 目を覆う板
    R(x - 6, y - 4, 12, 1, 0xc4c9cc);
  }
  else
    eye(x, y, 2, t, flash, hurt);
  if (tipFlash)
    for (int k = 0; k < 6; k++)
    {
      float a = k * 1.05f + t, l = 3 + t % 3;
      L(T.x, T.y, T.x + cosf(a) * l, T.y + sinf(a) * l, k % 2 ? REDW : RED);
    }
}

static void drawFork(float x, float y, int st, int tm, int crate, int t, bool flash, bool hurt)
{
  if (st == 0)
    y += sinf(t * 1.3f) * 0.7f; // エンジンをふかして震える
  if (st == 1)
    for (int k = 0; k < 3; k++)
      C(x - 8 + k * 8, y - 18 - k * 2, 3 + tm / 6.0f, 0x968c78, 64);
  R(x - 8, y - 10, 24, 30, 0, 89);
  static const int TIRE[4][2] = {{-13, -10}, {10, -10}, {-13, 5}, {10, 5}};
  for (auto &w : TIRE)
    R(x + w[0], y + w[1], 3, 7, 0x141516);
  R(x - 10, y - 14, 20, 26, flash ? 0xe0c070 : 0xa8822a);
  R(x - 10, y - 14, 20, 1, 0xd4ab48);
  R(x - 10, y - 14, 1, 26, 0xc9a03a);
  R(x + 9, y - 14, 1, 26, 0x6a5218);
  R(x - 9, y - 14, 18, 6, 0x5a4a28);
  R(x - 9, y - 9, 18, 1, 0x3a2e18);
  C(x + 5, y + 6, 2, 0x6b3a1e);
  C(x - 6, y - 4, 1, 0x6b3a1e);
  P(x + 6, y - 7, 0x8a4f28);
  R(x - 5, y - 6, 10, 9, 0x2a2c2e);
  R(x - 8, y - 8, 16, 1, 0x3a3d40);
  R(x - 8, y + 3, 16, 1, 0x3a3d40);
  R(x - 8, y - 8, 1, 12, 0x3a3d40);
  R(x + 7, y - 8, 1, 12, 0x3a3d40);
  R(x - 7, y + 10, 14, 3, 0x3a3d40);
  R(x - 7, y + 10, 14, 1, 0x6b7075);
  R(x - 6, y + 13, 3, 12, 0x55585b);
  R(x + 3, y + 13, 3, 12, 0x55585b);
  R(x - 6, y + 13, 1, 12, 0x8a8f94);
  R(x + 3, y + 13, 1, 12, 0x8a8f94);
  nerve({{x, y - 2}, {x - 8, y + 4}, {x - 6, y + 12}}, t);
  nerve({{x, y - 2}, {x + 7, y + 6}, {x + 4, y + 12}}, t, 7);
  eye(x, y - 2, 2, t, flash, hurt);
  if (st == 0)
    for (int k = 0; k < 3; k++)
    {
      float p = fmodf(t * 0.08f + k / 3.0f, 1);
      C(x + 7 + p * 3, y - 16 - p * 10, 1 + p * 3, 0x3c3c3c, (int)(153 * (1 - p)));
    }
  if (st == 3 && crate > 0) // 爪で持ち上げた木箱
  {
    R(x - 11, y + 13, 22, 15, 0x8a6a3a);
    R(x - 11, y + 13, 22, 1, 0xc49a5a);
    R(x - 11, y + 27, 22, 1, 0x4a3420);
    for (int k = 0; k < 3; k++)
      R(x - 11, y + 17 + k * 4, 22, 1, 0x5a4428);
    L(x - 10, y + 14, x + 10, y + 26, 0x5a4428);
    if (crate <= 2)
      L(x - 4, y + 13, x - 1, y + 20, 0x2a1c10); // ひび
  }
}

static void drawSnake(const Enemy &e, int sc, int t)
{
  // 配管の口 (出る口と潜る口)
  for (int k = 0; k < 2; k++)
  {
    float hx = k ? e.ux : e.gx, hy = (k ? e.uy : e.gy) + sc;
    C(hx, hy, 8, 0x2e170d);
    C(hx, hy, 6, 0x080808);
    CR(hx, hy, 8, 0xa8683a);
    CR(hx, hy, 7, 0x6b3a1e);
  }
  for (int k = 7; k >= 0; k--)
  {
    float u = e.r - (k + 1) * SNAKE_DU, x, y;
    if (!e.seg[k] || u < 0 || u > 1)
      continue;
    snakeAt(e, u, sc, x, y);
    C(x + 2, y + 3, 5, 0, 77);
    C(x, y, 5, 0x2e170d);
    C(x - 1, y - 1, 4, e.seg[k] == 1 ? 0x8a4f28 : 0x6b3a1e);
    C(x - 2, y - 2, 2, 0xa8683a);
    P(x - 2, y - 3, 0xd09a6a);
    CR(x, y, 5, 0x1e0f08);
    float m = fmodf(k * 3 - t * 0.5f, 8);
    if (m < 0)
      m += 8;
    P(x, y, m < 2 ? RED : RED3);
  }
  if (e.hidden)
    return;
  float hx = e.x, hy = e.y;
  C(hx + 2, hy + 3, 6, 0, 77);
  C(hx, hy, 5, 0x3a1410);
  CR(hx, hy, 6, 0xb0302a);
  CR(hx, hy, 5, 0x7a2e22);
  for (int k = 0; k < 4; k++)
  {
    float a = t * 0.08f + k * PIf / 2;
    L(hx, hy, hx + cosf(a) * 5, hy + sinf(a) * 5, 0xb0302a);
  }
  eye(hx, hy, 2, t, e.hit > 0, e.st == 1);
}

static void drawEye(const Enemy &e, int t)
{
  float x = e.x, y = e.y, dx = cosf(e.a), dy = sinf(e.a);
  // 壁の金具
  R(x - 9, y - 14, 18, 7, 0x45423d);
  R(x - 9, y - 14, 18, 1, 0x6b6862);
  R(x - 9, y - 7, 18, 2, 0, 115);
  for (int i = 0; i < 3; i++)
    R(x - 7 + i * 6, y - 13, 1, 2 + i, 0x6b3a1e);
  if (e.st <= 1) // 見張っている向き
    for (float s : {-0.28f, 0.28f})
      for (int r = 14; r < 90; r += 4)
        P(x + cosf(e.a + s) * r, y + sinf(e.a + s) * r, RED, e.st == 1 ? 128 : 46);
  float ox = x + dx * 12, oy = y + dy * 12;
  L(x, y - 7, x, y, 0x3a3d40);
  C(x, y, 2, 0x55585b);
  LT(x + 3, y + 5, ox + 3, oy + 5, 6, 0, 77);
  LT(x, y, ox, oy, 6, e.hit ? 0xffffff : 0xcfcabd);
  L(x - 2, y - 1, ox - 2, oy - 1, 0xefebe0);
  C(ox, oy, 3, 0x141516);
  if (e.st == 1)
  {
    C(ox, oy, 4, RED, 56);
    P(ox, oy, (t >> 1) & 1 ? RED : RED2);
  }
  else
    P(ox, oy, 0x7a1a1a);
  nerve({{x, y - 14}, {x + 2, y - 10}, {x, y - 7}}, t);
}

static void drawScrap(const Enemy &e, int t)
{
  float x = e.x, y = e.y, r = e.r;
  static const uint32_t COLS[6] = {0x6b3a1e, 0x8a4f28, 0x7d8388, 0x4a2a1a, 0xa8683a, 0x55585b};
  C(x + 3, y + 4, r, 0, 89);
  C(x, y, r * 0.6f, RED3);
  C(x, y, r * 0.35f, e.hit ? 0xffffff : RED);
  int n = (int)(12 + r * 2.2f);
  for (int i = 0; i < n; i++)
  {
    float an = i * 2.39f + e.a, d = r * (0.3f + 0.7f * hs(i)), px = x + cosf(an) * d, py = y + sinf(an) * d, s = 2 + hs(i * 3) * 4;
    if (i % 5 == 0)
      L(px - s, py, px + s, py + s * 0.5f, COLS[i % 6]);
    else
      R(px - s / 2, py - s / 3, s, s * 0.6f, COLS[i % 6]);
  }
  for (int i = 0; i < 8; i++) // 吸い寄せられてくる鉄くず
  {
    float ph = fmodf(t * 0.01f + hs(i + 40), 1), dist = 40 * (1 - ph) + r, ang = hs(i + 50) * 6.28f;
    R(x + cosf(ang) * dist, y + sinf(ang) * dist, 2, 2, i % 2 ? 0x8a4f28 : 0x7d8388);
  }
}

static void drawMaint(const Enemy &e, const Enemy *tg, int t)
{
  float x = e.x, y = e.y;
  if (tg && e.st == 0 && hypotf(tg->x + 14 - x, tg->y - 16 - y) < 12) // 直している光
  {
    int n = 16;
    for (int k = 0; k < n; k++)
      if ((k + (t >> 1)) % 4 < 2)
        P(x + (tg->x - x) * k / n, y + 3 + (tg->y - y - 3) * k / n, RED);
    if (e.cnt > 0)
      for (int k = 0; k < 3; k++)
      {
        float px = tg->x - 6 + k * 6, py = tg->y - 6 - (10 - e.cnt) * 1.4f;
        R(px - 1, py, 3, 1, RED);
        R(px, py - 1, 1, 3, RED);
      }
  }
  C(x + 6, y + 12, 4, 0, 77);
  static const int ARM[4][2] = {{-6, -6}, {6, -6}, {-6, 6}, {6, 6}};
  for (auto &a : ARM)
  {
    L(x, y, x + a[0], y + a[1], 0x55585b);
    CR(x + a[0], y + a[1], 3, (t & 1) ? 0x8a8f94 : 0x55585b);
  }
  C(x, y, 4, e.hit ? 0xffffff : 0xd0ccc0);
  C(x - 1, y - 1, 2, 0xf2efe6);
  R(x - 2, y + 3, 5, 2, 0xc9a227);
  eye(x, y, 1, t, false, e.st == 1);
}

static void drawCore(const Enemy &e)
{
  float x = e.x, y = e.y;
  for (int k = 1; k < 10; k++)
    P(x - e.vx * k * 1.5f, y - e.vy * k * 1.5f, RED, (int)(204 * (1 - k / 10.0f)));
  C(x, y, 5, RED, 26);
  C(x, y, 3, RED, 56);
  C(x, y, 3, RED2);
  C(x, y, 2, RED);
  P(x - 1, y - 1, REDW);
}

// 強くなった (核が乗り移った) しるし: 脈打つ赤い輪と、段の数の点
static void lvlMark(float x, float y, float rad, int lvl, int t)
{
  CR(x, y, rad + sinf(t * 0.2f), RED, 90 + (int)(50 * sinf(t * 0.2f)));
  for (int i = 0; i < lvl; i++)
    R(x - lvl * 2 + i * 4, y - rad - 4, 2, 2, RED);
}

void drawEShot(int k, float x, float y, float dx, float dy, int life, int t)
{
  float l = hypotf(dx, dy);
  if (l == 0)
    l = 1;
  float ux = dx / l, uy = dy / l, nx = -uy, ny = ux;
  if (k == 1) // 溶接の火花 (ジグザグ)
  {
    float j = (t & 1 ? 1 : -1) * 1.5f;
    C(x, y, 3, 0xff3c28, 40);
    L(x, y, x + ux * 4 + nx * j, y + uy * 4 + ny * j, RED);
    L(x + ux * 4 + nx * j, y + uy * 4 + ny * j, x + ux * 7, y + uy * 7, RED);
    P(x, y, REDW);
  }
  else if (k == 2) // 橙の大きな弾 (ボス)
  {
    C(x, y, 6, 0xff7828, 64);
    C(x, y, 4, 0xff7a2a);
    C(x - 1, y - 1, 2, 0xffb060);
    P(x - 1, y - 1, 0xfff0d0);
  }
  else if (k == 3) // 蒸気
  {
    float q = 1 - life / 18.0f;
    C(x, y, 2 + q * 5, 0xd2dce1, (int)(150 * (1 - q)) + 30);
  }
  else
  {
    C(x, y, 3, 0xff3c28, 56);
    Pt d[4] = {{x + ux * 4, y + uy * 4}, {x + nx * 1.8f, y + ny * 1.8f}, {x - ux * 3, y - uy * 3}, {x - nx * 1.8f, y - ny * 1.8f}};
    poly(d, 4, RED);
    P(x + ux, y + uy, 0xffe0d8);
  }
}

// air: false なら床の敵 (戦車より下に描く)、true なら空の敵と壁のカメラ (手前の絵より上に描く)
void Game2::drawEnemies(int ltop, bool air)
{
  int sc = (int)scroll;
  for (auto &e : en)
  {
    bool up = isAir(e.k) || e.k == EYE;
    if (up != air)
      continue;
    if (e.k != SNAKE && (e.y < ltop - 40 || e.y > ltop + LSTRIP + 40))
      continue;
    int age = t - e.t0;
    bool flash = e.hit > 0, hurt = weak(e) && e.k != BUG && e.k != CORE;
    switch (e.k)
    {
    case TWIN:
      drawTwin(e.x, e.y, t, flash, hurt, e.st == 1);
      break;
    case WALKER:
      drawWalker(e.x, e.y, t, (e.st == 1 || age % 120 < 70) ? t : 0, flash, hurt);
      break;
    case BUG:
      drawBug(e.x, e.y, e.a, t, e.id, flash);
      break;
    case WELDER:
      drawWelder(e.x, e.y, e.a, e.a + 0.5f * sinf(age * 0.05f + 1), e.st == 1, e.st == 0 && e.tm > 0, t, flash, hurt);
      break;
    case FORK:
      drawFork(e.x, e.y, e.st, e.tm, e.cnt, t, flash, hurt);
      break;
    case SNAKE:
      drawSnake(e, sc, t);
      break;
    case EYE:
      drawEye(e, t);
      break;
    case SCRAP:
      drawScrap(e, t);
      break;
    case MAINT:
      drawMaint(e, findId(e.link), t);
      break;
    case CORE:
      drawCore(e);
      break;
    }
    if (hurt && !e.hidden && e.virus == 0)
      sparks(e.x, e.y, t, 4);
    if (e.virus > 0 && (e.virus > 60 || (t & 4))) // ウイルス: 青い輪と、ちらつく青いノイズ (終わりぎわは点滅)
    {
      float rad = e.k == SCRAP ? e.r + 3 : e.k == BUG ? 7 : 13;
      CR(e.x, e.y, rad, 0x3a7bff, 150);
      for (int i = 0; i < 7; i++)
        R(e.x + (hs(t * 1.3f + i + e.id) - 0.5f) * rad * 2, e.y + (hs(t * 2.1f + i * 3 + e.id) - 0.5f) * rad * 2, 1 + (i & 1) * 2, 1,
          i % 3 ? 0x6ab8ff : 0xe0f0ff);
      R(e.x - 1, e.y - rad - 5, 3, 3, 0x6ab8ff);
    }
    if (e.lvl > 0 && !e.hidden)
    {
      static const float RAD[] = {16, 15, 7, 12, 18, 9, 10, 0, 10, 0};
      lvlMark(e.x, e.y, e.k == SCRAP ? e.r + 4 : RAD[e.k], e.lvl, t);
    }
  }
}
