// 2 本目 (廃工場) のボス 3 体。形と動きは敵の図鑑のページ (ボスの欄) と同じ。
// 区画の決まった所で出て、出ているあいだは流れが止まる (spd)。上から降りてきて (enterT)、倒すと大きな爆発 (dieT)。
//   0 ガントリー: 沈殿槽の天井クレーン。台車が左右に動き、影の所へフックを落とす。弱ると桁の端の制御盤へ逃げて直す
//   1 スタンパー: プレス機が油圧の脚で歩く。ヘッドを落として衝撃波の輪。ヘッドが上がると下の核が撃てる。脚を全部壊すと這って逃げる
//   2 炉心: 溶鉱炉。周りの羽口 8 つが順に橙の大きな弾を撃つ。追い詰めると、核を呼び集めて直そうとする
#include "game2.h"
#include "g2bg.h"
#include "g2draw.h"
#include "g2parts.h"

using namespace d2;
using namespace ep;

static const int W = 240, H = 320;
static const int ENTER = 90;
static const char *BOSS_NAME[3] = {"GANTRY", "STAMPER", "HEARTH"};
static const int BONUS[3] = {10000, 15000, 30000};

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float angDiff(float a, float b) { return fmodf(a - b + PIf * 3, PIf * 2) - PIf; }

const char *bossName(int k) { return k >= 0 && k < 3 ? BOSS_NAME[k] : ""; }

// スタンパーの脚 k (0〜3) の付け根・ひざ・足
static void stampLeg(const Game2::Boss &B, int k, int t, float &hx, float &hy, float &kx, float &ky, float &fx, float &fy)
{
  static const int SX[4] = {-1, 1, -1, 1}, SY[4] = {-1, -1, 1, 1};
  float sx = SX[k], sy = SY[k];
  float step = B.st == 0 && B.stun == 0 ? sinf(t * 0.06f + (k == 0 || k == 3 ? 0 : PIf)) * 5 : 0;
  hx = B.x + sx * 32;
  hy = B.y + sy * 14;
  fx = hx + sx * 16;
  fy = hy + sy * 12 + step;
  kx = hx + sx * 12;
  ky = hy - 10 + sy * 2;
}

// 炉心の羽口 k (0〜7) の位置と向き
static void hearthTur(const Game2::Boss &B, int k, float tx, float ty, float &bx, float &by, float &aim)
{
  float a = k / 8.0f * PIf * 2 + 0.2f;
  bx = B.x + cosf(a) * 52;
  by = B.y + sinf(a) * 52;
  aim = a + clampf(angDiff(atan2f(ty - by, tx - bx), a), -0.9f, 0.9f);
}

float Game2::bossEyeX() const { return boss.x + 5 * sinf(boss.t * 0.023f); }
float Game2::bossEyeY() const { return boss.y + 3 + 3 * sinf(boss.t * 0.031f); }

void Game2::startBoss(int k)
{
  boss = Boss();
  boss.k = k;
  boss.enterT = ENTER;
  boss.x = W / 2;
  boss.y = -90;
  bossBannerT = 3 * FPS;
  bossResult = 0;
  float m = 1 + 0.5f * min(lap, 2); // 2 周目からは硬い
  auto set = [&](int i, int hp) { boss.hp[i] = boss.maxhp[i] = (int)(hp * m); };
  if (k == 0)
  {
    boss.ty = 56;
    set(0, 120);
    set(1, 30);
    set(2, 30);
  }
  else if (k == 1)
  {
    boss.ty = 92;
    set(0, 70);
    for (int i = 1; i <= 4; i++)
      set(i, 32);
  }
  else
  {
    boss.ty = 80;
    set(0, 150);
    for (int i = 1; i <= 8; i++)
      set(i, 20);
  }
}

// 部位の位置 (爆発と爆弾に使う)
static void partPos(const Game2::Boss &B, int part, int t, float tx, float ty, float &x, float &y)
{
  x = B.x;
  y = B.y;
  if (B.k == 0)
  {
    if (part == 1 || part == 2)
    {
      x = part == 1 ? 26 : W - 26;
      y = B.y + 7;
    }
  }
  else if (B.k == 1)
  {
    if (part == 0)
      y = B.y + 2;
    else
    {
      float hx, hy, kx, ky;
      stampLeg(B, part - 1, t, hx, hy, kx, ky, x, y);
      x = kx;
      y = ky;
    }
  }
  else if (part > 0)
  {
    float a;
    hearthTur(B, part - 1, tx, ty, x, y, a);
  }
}

void Game2::bossHurt(int part, int dmg)
{
  Boss &B = boss;
  if (B.k < 0 || B.dieT > 0 || B.enterT > 0 || B.hp[part] <= 0)
    return;
  B.hp[part] -= dmg;
  B.hit[part] = 3;
  if (B.hp[part] > 0)
    return;
  B.hp[part] = 0;
  float x, y;
  partPos(B, part, t, tx, ty, x, y);
  fx.push_back({0, x, y, hs(t) * 6, 0, 26, false});
  if (part > 0)
  {
    if (mode == Mode::Play)
      score += 1000;
    return;
  }
  // 倒した
  B.dieT = 1;
  bossResult = 1;
  bossBannerT = 4 * FPS;
  if (mode == Mode::Play)
    score += BONUS[B.k] * (1 + min(lap, 2));
  dropItem(B.x - 20, B.y);
  dropItem(B.x + 20, B.y);
  eb.clear();
  for (auto &e : en)
    if (e.k == CORE && e.link == -2) // 呼び集めていた核も消える
      e.gone = true;
}

void Game2::bossStep()
{
  Boss &B = boss;
  if (B.k < 0)
    return;
  B.t++;
  for (auto &h : B.hit)
    if (h > 0)
      h--;
  if (bossBannerT > 0)
    bossBannerT--;

  if (B.dieT > 0) // 大きな爆発が続いて、消える
  {
    int len = B.k == 2 ? 120 : 90;
    if (B.dieT % 5 == 1)
    {
      float r = B.k == 0 ? 110 : 45;
      fx.push_back({0, B.x + (hs(t) - 0.5f) * r * 2, B.y + (hs(t * 1.7f) - 0.5f) * (B.k == 0 ? 30 : r * 1.4f), hs(t + 2) * 6, 0, 26, false});
    }
    if (++B.dieT > len)
    {
      fx.push_back({2, B.x, B.y, hs(t) * 6, 0, 40, false});
      B.k = -1;
    }
    return;
  }
  if (B.enterT > 0) // 上から降りてくる
  {
    B.enterT--;
    B.y = B.ty - (float)B.enterT / ENTER * (B.ty + 90);
    return;
  }

  bool hittable = mode == Mode::Play && tankOn && invT == 0 && !autoPilot;
  bool canShoot = tankOn && enterT == 0;
  auto shoot = [&](float x, float y, float a, float s, int k) {
    if (eb.size() < 30)
      eb.push_back({x, y, cosf(a) * s, sinf(a) * s, false, k, 0});
  };
  float bs = 1.6f + 0.3f * min(lap, 2); // 弾の速さ
  auto per = [&](int base) { return lap > 0 ? base * 3 / 4 : base; };

  // 衝撃波の輪 (止まっていても広がる)
  if (B.ringOn)
  {
    B.ringR += 3;
    if (hittable && fabsf(hypotf(tx - B.ringX, ty - B.ringY) - B.ringR) < 5)
      killTank();
    if (B.ringR > 85)
      B.ringOn = false;
  }
  if (B.stun > 0) // ウイルスで止まっている
  {
    B.stun--;
    return;
  }

  if (B.k == 0) // ---- ガントリー ----
  {
    float gy = B.y;
    if (B.st == 0)
    {
      int c = B.tm % 110;
      B.tm++;
      if (c < 40 || c >= 90)
      {
        B.u += 0.018f;
        B.x = W / 2 + 84 * sinf(B.u);
      }
      if (c == 0)
        B.hookY = clampf(ty, 150, H - 30); // 戦車のいる高さを狙う
      float full = B.hookY - (gy + 18);
      B.hookOff = c < 40 ? 0 : c < 50 ? (c - 40) / 10.0f * full : c < 80 ? full : c < 100 ? full * (1 - (c - 80) / 20.0f) : 0;
      B.elev = c; // 描くとき用 (影とフックの段階)
      if (c == 50)
      {
        fx.push_back({5, B.x, B.hookY, hs(t) * 6, 0, 16, false});
        if (hittable && hypotf(tx - B.x, ty - B.hookY) < 16)
          killTank();
      }
      if (c == 20 && canShoot) // 台車から 5 方向 (1 回りに 1 度)
      {
        float a0 = clampf(atan2f(ty - gy - 14, tx - B.x), 0.4f, PIf - 0.4f);
        for (float d : {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f})
          shoot(B.x, gy + 14, a0 + d, bs, 0);
      }
      for (int i = 1; i <= 2; i++) // 桁の目は橙の大きな弾
        if (B.hp[i] > 0 && canShoot && (B.t + i * 60) % per(120) == 0)
        {
          float ex = i == 1 ? 26 : W - 26, ey = gy + 7;
          shoot(ex, ey, atan2f(ty - ey, tx - ex), bs * 0.85f, 2);
        }
      if (B.hp[0] * 10 < B.maxhp[0] * 3 && B.heals < 2) // 弱ったので、制御盤へ逃げ込む
      {
        B.st = 1;
        B.hookOff = 0;
        B.elev = 100;
      }
    }
    else if (B.st == 1)
    {
      float gx = B.x < W / 2 ? 20 : W - 20;
      B.x += clampf(gx - B.x, -2.2f, 2.2f);
      if (fabsf(gx - B.x) < 0.5f)
      {
        B.st = 2;
        B.tm = 0;
      }
    }
    else // 2: 制御盤で直している
    {
      if (++B.tm % 3 == 0)
        B.hp[0] = min(B.maxhp[0], B.hp[0] + 1);
      if (B.tm >= 75)
      {
        B.heals++;
        B.st = 0;
        B.tm = 0;
        B.u = asinf(clampf((B.x - W / 2) / 84.0f, -1, 1));
      }
    }
  }
  else if (B.k == 1) // ---- スタンパー ----
  {
    int legs = 0;
    for (int i = 1; i <= 4; i++)
      if (B.hp[i] > 0)
        legs++;
    if (B.st == 0)
    {
      if (legs == 0)
        B.st = 1;
      B.u += 0.01f * (0.4f + 0.6f * legs / 4);
      B.x = W / 2 + 50 * sinf(B.u);
      B.y = B.ty + 10 * sinf(B.u * 1.3f);
    }
    else // 脚がない: 這って上へ逃げる
    {
      B.y -= 0.35f;
      B.x += (W / 2 - B.x) * 0.01f;
      if (B.y < -70)
      {
        B.k = -1;
        B.escaped = true;
        bossResult = 2;
        bossBannerT = 3 * FPS;
        return;
      }
    }
    int c = B.tm % 120;
    B.tm++;
    B.elev = c < 60 ? c / 60.0f * 26 : c < 68 ? 26 * (1 - (c - 60) / 8.0f) : 0;
    if (c == 68) // ヘッドを落とす: 衝撃波
    {
      B.ringOn = true;
      B.ringR = 8;
      B.ringX = B.x;
      B.ringY = B.y + 6;
      if (legs <= 2 && canShoot)
        for (int k = 0; k < 8; k++)
          shoot(B.x, B.y + 6, k * PIf / 4 + 0.2f, bs * 0.8f, 0);
    }
    if (canShoot && B.t % per(80) == 0) // 柱から狙う
    {
      float px = B.x + (B.fireIdx++ % 2 ? 32 : -32), py = B.y - 18;
      shoot(px, py, atan2f(ty - py, tx - px), bs, 0);
    }
    if (hittable)
    {
      bool hit = fabsf(tx - B.x) < 34 && fabsf(ty - B.y) < 22;
      for (int k = 0; k < 4 && !hit; k++)
        if (B.hp[k + 1] > 0)
        {
          float hx, hy, kx, ky, fx_, fy_;
          stampLeg(B, k, t, hx, hy, kx, ky, fx_, fy_);
          hit = hypotf(tx - fx_, ty - fy_) < 9;
        }
      if (hit)
        killTank();
    }
  }
  else // ---- 炉心 ----
  {
    B.x = W / 2;
    if (canShoot && B.t % per(26) == 0) // 生きている羽口が順に撃つ
      for (int n = 0; n < 8; n++)
      {
        int k = (B.fireIdx++) % 8;
        if (B.hp[k + 1] <= 0)
          continue;
        float bx, by, aim;
        hearthTur(B, k, tx, ty, bx, by, aim);
        shoot(bx + cosf(aim) * 8, by + sinf(aim) * 8, aim, bs * 0.95f, 2);
        break;
      }
    if (B.st == 0 && ((B.heals == 0 && B.hp[0] * 2 < B.maxhp[0]) || (B.heals == 1 && B.hp[0] * 5 < B.maxhp[0])))
    {
      B.st = 1; // 追い詰められたので、工場じゅうの核を呼び集める
      B.tm = 0;
      B.heals++;
    }
    if (B.st == 1)
    {
      B.tm++;
      if (B.tm % 20 == 0 && B.tm <= 160)
      {
        int side = (B.tm / 20) % 4;
        float x = side == 0 ? -8 : side == 1 ? W + 8 : side == 2 ? 30 + hs(t) * 60 : W - 30 - hs(t) * 60;
        float y = side < 2 ? 150 + hs(t + 1) * 120 : H + 8;
        Enemy &c = addEnemy(CORE, x, y);
        c.link = -2; // 炉心の目へ向かう
        c.lvl = 0;
      }
      if (B.tm > 220)
        B.st = 0;
    }
    if (hittable && hypotf(tx - B.x, ty - B.y) < 50)
      killTank();
  }
}

// 自分の弾が当たったか。当たったら傷を負わせて true (硬い所に当たったら、はじいて true)
bool Game2::bossShot(float x, float y)
{
  Boss &B = boss;
  if (B.k < 0 || B.dieT > 0)
    return false;
  auto spark = [&]() { fx.push_back({3, x, y, hs(t) * 6, 0, 5, false}); };
  if (B.enterT > 0)
    return false;
  if (B.k == 0)
  {
    float gy = B.y;
    if (fabsf(x - B.x) < 14 && y > gy - 16 && y < gy + 16)
    {
      bossHurt(0, 1);
      return true;
    }
    for (int i = 1; i <= 2; i++)
      if (B.hp[i] > 0 && hypotf(x - (i == 1 ? 26 : W - 26), y - (gy + 7)) < 9)
      {
        bossHurt(i, 1);
        return true;
      }
    return false;
  }
  if (B.k == 1)
  {
    for (int k = 0; k < 4; k++)
      if (B.hp[k + 1] > 0)
      {
        float hx, hy, kx, ky, fx_, fy_;
        stampLeg(B, k, t, hx, hy, kx, ky, fx_, fy_);
        if (hypotf(x - kx, y - ky) < 7 || hypotf(x - fx_, y - fy_) < 7 || hypotf(x - (hx + kx) / 2, y - (hy + ky) / 2) < 6)
        {
          bossHurt(k + 1, 1);
          return true;
        }
      }
    if (B.elev > 14 && hypotf(x - B.x, y - (B.y + 2)) < 9) // ヘッドが上がると核が見える
    {
      bossHurt(0, 1);
      return true;
    }
    // ヘッドが下りているときは、ヘッドが核を覆っていて弾をはじく。上がっているときは弾が体の上を越えて核に届く
    float hy = B.y - 20 - B.elev;
    if (B.elev <= 14 && fabsf(x - B.x) < 32 && y > hy - 14 && y < hy + 28)
    {
      spark();
      return true;
    }
    return false;
  }
  for (int k = 0; k < 8; k++)
    if (B.hp[k + 1] > 0)
    {
      float bx, by, a;
      hearthTur(B, k, tx, ty, bx, by, a);
      if (hypotf(x - bx, y - by) < 7)
      {
        bossHurt(k + 1, 1);
        return true;
      }
    }
  // 弾は炉の上を越えて飛ぶ。真ん中の目に届いたら当たる (まばたきしているあいだは、はじく)
  if (hypotf(x - bossEyeX(), y - bossEyeY()) < 11)
  {
    if ((B.t % 140) < 6)
      spark();
    else
      bossHurt(0, 1);
    return true;
  }
  return false;
}

// 爆弾: 近くの部位に 2 発ぶん。本体の近くならウイルスで 3 秒止める
void Game2::bossBomb(float x, float y)
{
  Boss &B = boss;
  if (B.k < 0 || B.dieT > 0 || B.enterT > 0)
    return;
  int n = B.k == 0 ? 3 : B.k == 1 ? 5 : 9;
  for (int i = 0; i < n; i++)
  {
    if (B.hp[i] <= 0)
      continue;
    if (i == 0 && B.k == 1 && B.elev <= 14) // スタンパーの核はヘッドの下
      continue;
    float px, py;
    partPos(B, i, t, tx, ty, px, py);
    if (hypotf(px - x, py - y) < 42)
      bossHurt(i, 2);
  }
  if (B.k >= 0 && B.dieT == 0)
  {
    float d = B.k == 0 ? fabsf(y - B.y) : hypotf(x - B.x, y - B.y);
    if (d < (B.k == 0 ? 40 : 70))
      B.stun = 3 * FPS;
  }
}

// ---- 描く ----
static void stunNoise(float x, float y, float rx, float ry, int t)
{
  for (int i = 0; i < 16; i++)
    R(x + (hs(t * 1.3f + i) - 0.5f) * rx * 2, y + (hs(t * 2.1f + i * 3) - 0.5f) * ry * 2, 1 + (i & 1) * 3, 1, i % 3 ? 0x6ab8ff : 0xe0f0ff);
}

static void drawGantry(const Game2::Boss &B, int t)
{
  float gy = B.y, x = B.x;
  int c = (int)B.elev;
  // フックの落ちる所の影と、落ちたときの輪
  if (B.st == 0 && c < 50)
  {
    float r = 2 + min(1.0f, c / 40.0f) * 9;
    C(x, B.hookY, r, 0, 40 + c * 2);
  }
  if (B.st == 0 && c >= 50 && c < 64)
  {
    float q = (c - 50) / 14.0f;
    CR(x, B.hookY, 4 + q * 16, 0xc8f0eb, (int)(180 * (1 - q)));
    CR(x, B.hookY, 2 + q * 9, 0xc8f0eb, (int)(130 * (1 - q)));
  }
  R(0, gy + 26, W, 13, 0, 77);
  R(0, gy, W, 16, 0x2f6b52);
  R(0, gy, W, 1, 0x6fb595);
  R(0, gy + 15, W, 1, 0x1d3f31);
  R(0, gy + 2, W, 11, 0x24473a);
  for (int i = 0; i < W; i += 10)
  {
    L(i, gy + 2, i + 5, gy + 12, 0x4f9a78);
    L(i + 5, gy + 12, i + 10, gy + 2, 0x4f9a78);
  }
  block(0, gy - 4, 12, 24, 6, 0x3f8a6a, 0x24473a);
  block(W - 12, gy - 4, 12, 24, 6, 0x3f8a6a, 0x24473a);
  for (int s = 0; s < 2; s++) // 制御盤のランプ (直しているあいだは速く点滅)
  {
    float px = s ? W - 8 : 4;
    bool fast = B.st == 2 && (x < W / 2) == (s == 0);
    P(px, gy - 6, (fast ? (t >> 1) & 1 : (t >> 4) & 1) ? 0x7cff8a : 0x1d3f31);
    P(px + 3, gy - 6, (fast ? (t >> 1) & 1 : (t >> 5) & 1) ? RED : 0x3a1410);
  }
  nerve({{0, gy + 14}, {(float)W, gy + 14}}, t);
  nerve({{0, gy + 1}, {(float)W, gy + 1}}, t, 6);
  for (int i = 1; i <= 2; i++)
  {
    float ex = i == 1 ? 26 : W - 26;
    if (B.hp[i] > 0)
      eye(ex, gy + 7, 3, t, B.hit[i] > 0, B.hp[i] * 3 <= B.maxhp[i]);
    else
    {
      C(ex, gy + 7, 4, 0x141516);
      sparks(ex, gy + 7, t, 2);
    }
  }
  L(x, gy + 12, x, gy + 16 + B.hookOff, 0x2a2b2c);
  float hy = gy + 18 + B.hookOff;
  C(x + 2, hy + 3, 3, 0, 77);
  CR(x, hy, 3, 0x55585b);
  R(x - 1, hy - 4, 3, 3, 0xc9a227);
  block(x - 13, gy - 6, 26, 22, 8, B.hit[0] ? 0xf0d878 : 0xc9a227, 0x8a7020);
  R(x - 9, gy - 12, 18, 2, 0x141516);
  bool weak = B.hp[0] * 10 < B.maxhp[0] * 3;
  eye(x, gy - 4, 3, t, B.hit[0] > 0, weak);
  if (weak)
    sparks(x, gy - 2, t, 4);
}

static void drawStamper(const Game2::Boss &B, int t)
{
  float cx = B.x, cy = B.y, elev = B.elev;
  for (int k = 0; k < 4; k++)
  {
    float hx, hy, kx, ky, fx, fy;
    stampLeg(B, k, t, hx, hy, kx, ky, fx, fy);
    if (B.hp[k + 1] <= 0) // 壊れた脚の付け根
    {
      LT(hx, hy, hx + (fx - hx) * 0.3f, hy + 3, 5, 0x2a2420);
      sparks(hx, hy, t + k, 2);
      continue;
    }
    bool fl = B.hit[k + 1] > 0;
    C(fx + 2, fy + 3, 4, 0, 77);
    LT(hx, hy, kx, ky, 5, fl ? 0xa08070 : 0x55402f);
    L(hx - 1, hy - 2, kx - 1, ky - 2, 0x7a6656);
    LT(kx, ky, fx, fy, 3, fl ? 0xffffff : 0x9aa0a5);
    L(kx - 1, ky, fx - 1, fy, 0xe3e8ec);
    C(kx, ky, 3, 0x3a3d40);
    P(kx - 1, ky - 1, 0x8a8f94);
    R(fx - 4, fy - 2, 8, 4, 0x2a2c2e);
    R(fx - 4, fy - 2, 8, 1, 0x55585b);
  }
  block(cx - 34, cy - 18, 68, 36, 8, 0x5b5650, 0x3e3a36);
  nerve({{cx - 30, cy - 20}, {cx - 10, cy}, {cx + 10, cy}, {cx + 30, cy - 20}}, t);
  eye(cx, cy + 2, 5, t, B.hit[0] > 0, B.hp[0] * 3 <= B.maxhp[0]);
  for (int dx : {-36, 28})
    block(cx + dx, cy - 22, 8, 8, 54, 0x7a6a5c, 0x56463a);
  float hy = cy - 20 - elev;
  R(cx - 30 + elev * 0.3f, cy - 14 + elev * 0.3f, 60, 28, 0, 77);
  block(cx - 32, hy, 64, 28, 14, 0x7a6656, 0x55402f);
  for (int k = 1; k < 5; k++)
    R(cx - 32 + k * 13, hy - 12, 2, 24, 0x5f4d40);
  for (int i = 0; i < 6; i++)
    R(cx - 30 + hs(i) * 60, hy + 14, 1, 3 + hs(i + 2) * 6, 0x6b3a1e);
  eye(cx - 18, hy - 2, 2, t, false, false);
  eye(cx + 18, hy - 2, 2, t, false, false);
  for (int dx : {-36, 28})
    block(cx + dx, cy + 14, 8, 8, 54, 0x7a6a5c, 0x56463a);
  nerve({{cx - 32, cy - 34}, {cx - 32, cy + 10}}, t, 3);
  nerve({{cx + 32, cy - 34}, {cx + 32, cy + 10}}, t, 9);
}

static void drawHearth(const Game2::Boss &B, int t, float tx, float ty, float px, float py)
{
  float cx = B.x, cy = B.y;
  C(cx + 5, cy + 7, 50, 0, 102);
  float bx[8], by[8], aim[8];
  for (int k = 0; k < 8; k++)
  {
    hearthTur(B, k, tx, ty, bx[k], by[k], aim[k]);
    float a = k / 8.0f * PIf * 2 + 0.2f;
    LT(cx + cosf(a) * 42, cy + sinf(a) * 42, bx[k], by[k], 4, 0x5a3422);
  }
  for (int k = 0; k < 8; k++)
  {
    if (B.hp[k + 1] <= 0)
    {
      C(bx[k], by[k], 4, 0x1a1210);
      sparks(bx[k], by[k], t + k, 1);
      continue;
    }
    bool fl = B.hit[k + 1] > 0;
    C(bx[k], by[k], 5, fl ? 0xffffff : 0x6b3a1e);
    CR(bx[k], by[k], 5, 0x2e170d);
    LT(bx[k], by[k], bx[k] + cosf(aim[k]) * 8, by[k] + sinf(aim[k]) * 8, 3, 0x3a2216);
    P(bx[k] - 1, by[k] - 1, RED);
  }
  C(cx, cy, 46, 0x5a3422);
  CR(cx, cy, 46, 0x8a4f28);
  CR(cx, cy, 43, 0x3a2216);
  C(cx, cy, 40, 0x3d2a22);
  CR(cx, cy, 36, 0x2a1c16);
  float pul = sinf(t * 0.09f);
  C(cx, cy, 32, 0x7a1d05);
  C(cx, cy, 27, 0xc2410c);
  C(cx, cy, 21, pul > 0 ? 0xf26b1d : 0xe85d12);
  for (int k = 0; k < 7; k++)
  {
    float a = t * 0.03f + k * 0.9f, rr = 22 + 4 * sinf(t * 0.05f + k);
    C(cx + cosf(a) * rr, cy + sinf(a) * rr, 2, 0xffb23e);
  }
  for (int k = 0; k < 8; k++)
    if (B.hp[k + 1] > 0)
      nerve({{cx, cy}, {bx[k], by[k]}}, t, k * 3);
  C(cx, cy, 15, 0x1f0806);
  CR(cx, cy, 15, RED2);
  CR(cx, cy, 13, RED3);
  bool fl = B.hit[0] > 0;
  C(px, py, 9, RED, 30);
  C(px, py, 7, fl ? 0xffffff : RED2);
  C(px, py, 5, fl ? 0xffffff : RED);
  C(px, py, 2, 0x200404);
  P(px - 3, py - 3, REDW);
  P(px - 2, py - 3, REDW);
  if ((B.t % 140) < 6) // まばたき
  {
    float q = (B.t % 140) / 6.0f;
    R(cx - 15, cy - 15, 31, 30 * sinf(q * PIf), 0x3d2a22);
  }
  if (B.hp[0] * 3 <= B.maxhp[0])
    sparks(cx, cy, t, 6);
}

// air: ガントリーは頭上なので手前に、ほかは床なので戦車より下に描く
void Game2::bossDraw(int ltop, bool air)
{
  const Boss &B = boss;
  if (B.k < 0)
    return;
  if (!air && B.ringOn)
  {
    float q = B.ringR / 85;
    CR(B.ringX, B.ringY, B.ringR, 0xe8fffb, (int)(200 * (1 - q)));
    CR(B.ringX, B.ringY, B.ringR - 3, 0xe8fffb, (int)(110 * (1 - q)));
  }
  if ((B.k == 0) != air)
    return;
  // この帯にかからなければ描かない (ボスは大きいので、帯ごとに全部描くと遅い)
  float y0 = B.k == 0 ? B.y - 16 : B.y - (B.k == 1 ? 80 : 62), y1 = B.k == 0 ? max(B.y + 42, B.hookY + 24) : B.y + (B.k == 1 ? 44 : 62);
  if (y1 < ltop || y0 > ltop + LSTRIP)
    return;
  if (B.dieT > 60 && (t & 2)) // 消える前に点滅
    return;
  if (B.k == 0)
    drawGantry(B, t);
  else if (B.k == 1)
    drawStamper(B, t);
  else
    drawHearth(B, t, tx, ty, bossEyeX(), bossEyeY());
  if (B.stun > 0 && (B.stun > 30 || (t & 4)))
  {
    if (B.k == 0)
      stunNoise(B.x, B.y, 30, 20, t);
    else
      stunNoise(B.x, B.y, 44, 36, t);
  }
}
