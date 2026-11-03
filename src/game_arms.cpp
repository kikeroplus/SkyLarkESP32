// 円盤の種類と弾の種類を増やし、村人の高射砲と、円盤が下ろす戦車、自機の爆弾を足す (Windows 版の GameArms.cs)。
// ・円盤: 戦車運び (戦車を光線で地上へ下ろす)、こま円盤 (曲がる弾)、分かれ円盤 (ばらまき弾。落とすとちび円盤 3 機に)、
//   光線円盤 (力をためて太い光線を真下へ)
// ・弾: いつもの弾 (0)、曲がる弾 (1)、ばらまきの小粒 (2)、ゆっくり大きな玉 (3)、戦車が高射砲へ撃つ地上の砲弾 (4)
// ・村人の高射砲とトラクターの大砲が円盤を撃ち上げる。戦車は自機と高射砲をねらう。戦車は爆弾 (SW2) で倒す
#include "game.h"

// ---- 円盤の弾 ----
void Game::enemyShotK(float x, float y, float vx, float vy, int kind, float w)
{
  float k = kind == 4 ? 1 : bulletK();
  for (Shot &s : theirs)
    if (!s.on)
    {
      s.x = s.ox = x;
      s.y = s.oy = y;
      s.vx = vx * k;
      s.vy = vy * k;
      s.kind = kind;
      s.w = w;
      s.life = 0;
      s.on = true;
      if (kind != 4)
        enemyShotSnd = true;
      return;
    }
}

// 弾を 1 フレーム動かす (曲がる弾は向きが回り、地上の砲弾は的に着いたら高射砲を壊す)。消えたら false
bool Game::stepEnemyShot(Shot &s)
{
  s.life++;
  if (s.kind == 1 && s.life < 50) // 曲がるのは撃ってしばらくのあいだだけ
  {
    float c = cosf(s.w), sn = sinf(s.w), vx = s.vx * c - s.vy * sn;
    s.vy = s.vx * sn + s.vy * c;
    s.vx = vx;
  }
  s.x += s.vx;
  s.y += s.vy;
  if (s.kind == 4)
  {
    s.y += SPEED;
    if (s.life >= (int)s.tx)
    {
      for (Prop &p : props)
        if (p.on && p.type == AAGUN && p.kind == 0 && fabsf(p.x - s.x) < 9 && fabsf(p.y - s.y) < 9)
        {
          p.kind = 1; // 壊れた
          burst(p.x, p.y, 0xFF8A3A, 10, 1.2f, 16);
          addPuff(p.x, p.y, true);
          sfx(Sfx::Boom, 0.4f);
        }
      return false;
    }
  }
  return !(s.y > H + 4 || s.y < -4 || s.x < -4 || s.x > W + 4);
}

// ---- 新しい円盤の出しかた ----
bool Game::carrierOn() const
{
  for (const Ufo &u : ufos)
    if (u.on && u.type == CARRIER)
      return true;
  return false;
}

// 波の種類を選ぶ: 時間がたつほど種類が増える (0 偵察機、1 輪、2 羊さらい、3 こま、4 分かれ、5 光線、6 戦車運び)
int Game::pickWave()
{
  int mx = frame < 30 * FPS ? 3 : frame < 45 * FPS ? 5 : 6;
  int k = max(0, irand(-1, mx)); // 偵察機は少し多め
  if (k == 6 && carrierOn())
    k = irand(0, 5);
  return k;
}

void Game::spawnArms(int k)
{
  switch (k)
  {
  case 3: // こま円盤が 2 機、同じ側から少しずつ遅れて
  {
    int dir = irand(0, 1) != 0 ? 1 : -1;
    float home = frand(40, 120);
    for (int i = 0; i < 2; i++)
    {
      Ufo *u = addUfo(SPINNER, dir > 0 ? -16 : W + 16, home, -i * 26);
      if (u)
      {
        u->vx = dir * 1.4f;
        u->home = home + i * 14;
      }
    }
    break;
  }
  case 4: // 分かれ円盤
    for (int i = 0, n = day >= 3 ? 2 : 1; i < n; i++)
    {
      Ufo *u = addUfo(SPLITTER, 0, -14, -i * 40);
      if (u)
      {
        u->home = frand(40, W - 40);
        u->x = u->home;
        u->vy = 0.7f;
      }
    }
    break;
  case 5: // 光線円盤
  {
    Ufo *u = addUfo(LANCER, frand(30, W - 30), -16, 0);
    if (u)
      u->home = frand(60, 110);
    break;
  }
  case 6: // 戦車運び
  {
    Ufo *u = addUfo(CARRIER, frand(50, W - 50), -30, 0);
    if (u)
      u->home = frand(70, 100);
    break;
  }
  }
}

// 新しい円盤の動き。それ以外の円盤なら false
bool Game::stepUfoArms(Ufo &u)
{
  switch (u.type)
  {
  case CARRIER:
    if (u.state == 0) // 降りてきて止まる
    {
      u.y += 0.8f;
      if (u.y >= u.home)
      {
        u.state = 1;
        u.a = 30;
        u.prop = 0;
      }
    }
    else if (u.state == 1) // 止まって、光線で戦車を 3 台下ろす
    {
      u.x += sinf(u.t * 0.02f) * 0.3f;
      if (--u.a <= 0)
      {
        if (u.prop < 3)
        {
          Prop *tk = addProp(u.x, u.y + 2, TANK, irand(0, 1));
          if (tk)
          {
            tk->lift = 1;
            tk->vx = (irand(0, 1) != 0 ? 1 : -1) * 0.25f;
            tk->ax = PI / 2;
          }
          u.prop++;
          u.a = 75;
        }
        else
          u.state = 2;
      }
      if (u.t % 110 == 60)
        for (int k = -1; k <= 1; k++)
          shootAtK(u.x, u.y, angTo(u.x, u.y) + k * 0.35f, 1.0f, 3, 0);
    }
    else
      u.y -= 1.0f;
    return true;
  case SPINNER: // 横から入り、波打ちながら曲がる弾を撃つ
    u.x += u.vx;
    u.y = u.home + 18 * sinf(u.t * 0.06f);
    u.a += 0.3f;
    if (u.t % 50 == 25 && u.x > 6 && u.x < W - 6)
    {
      float a0 = angTo(u.x, u.y);
      shootAtK(u.x, u.y, a0 - 0.5f, 1.4f, 1, 0.018f);
      shootAtK(u.x, u.y, a0 + 0.5f, 1.4f, 1, -0.018f);
    }
    return true;
  case SPLITTER: // ゆっくり降りながら、5 方向にばらまく
    u.y += u.vy;
    u.x = u.home + 14 * sinf(u.t * 0.04f);
    if (u.t % 80 == 40 && u.y > 20 && u.y < H - 100)
    {
      float a0 = angTo(u.x, u.y);
      for (int k = -2; k <= 2; k++)
        shootAtK(u.x, u.y, a0 + k * 0.28f, 1.6f, 2, 0);
    }
    return true;
  case MINI: // ちび円盤: 散ってから降下し、1 発だけ撃つ
    u.x += u.vx;
    u.y += u.vy;
    u.vx *= 0.97f;
    u.vy = min(u.vy + 0.04f, 2.2f);
    if (u.t == 22)
      shootAtK(u.x, u.y, angTo(u.x, u.y), 1.8f, 2, 0);
    return true;
  case LANCER: // 止まって力をため (予告の線)、太い光線を真下へ。2 回撃ったら去る
    if (u.state == 0)
    {
      u.y += 1.2f;
      if (u.y >= u.home)
      {
        u.state = 1;
        u.a = 0;
      }
    }
    else if (u.state == 1)
    {
      if (++u.a >= 45)
      {
        u.state = 2;
        u.a = 0;
        sfx(Sfx::BeamFire);
      }
    }
    else if (u.state == 2)
    {
      if (++u.a >= 34)
      {
        u.state = 3;
        u.a = 0;
      }
    }
    else if (u.state == 3) // 自機のほうへ少し寄ってから、もう一度
    {
      u.x += constrain(px - u.x, -0.6f, 0.6f);
      if (++u.a >= 60)
      {
        u.group++;
        u.state = u.group >= 1 ? 4 : 1;
        u.a = 0;
      }
    }
    else
      u.y -= 1.4f;
    return true;
  }
  return false;
}

void Game::onUfoDestroyed(Ufo &u)
{
  if (u.type == SPLITTER) // 3 機のちび円盤に分かれる
    for (int k = -1; k <= 1; k++)
    {
      Ufo *m = addUfo(MINI, u.x + k * 4, u.y, 0);
      if (m)
      {
        m->vx = k * 1.3f;
        m->vy = -0.6f;
      }
    }
  if (u.type == CARRIER)
    burst(u.x, u.y, 0xC8A8F0, 16, 2.0f, 22);
}

// ---- 戦車と高射砲 (地上のもの。置き物として地面と一緒に流れる) ----
void Game::stepTank(Prop &p)
{
  if (p.lift > 0) // まだ光線で下ろされている
    return;
  if (p.t == 0) // 着地
  {
    p.t = 1;
    burst(p.x, p.y, 0xB8A880, 8, 0.8f, 14);
    sfx(Sfx::TankDrop, 0.6f);
  }
  p.t++;
  if (p.y > 0)
  {
    p.x += p.vx;
    if (p.x < 10 || p.x > W - 10)
      p.vx = -p.vx;
    p.v = p.vx > 0 ? 1 : 0;
  }
  // ねらう相手: 近くの高射砲があればそれを、なければ自機
  Prop *aa = nullptr;
  float best = 70 * 70;
  for (Prop &q : props)
    if (q.on && q.type == AAGUN && q.kind == 0)
    {
      float d2 = (q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y);
      if (d2 < best)
      {
        best = d2;
        aa = &q;
      }
    }
  float tx = aa ? aa->x : px, ty = aa ? aa->y : py;
  float want = atan2f(ty - p.y, tx - p.x), diff = want - p.ax;
  while (diff > PI)
    diff -= 2 * PI;
  while (diff < -PI)
    diff += 2 * PI;
  p.ax += constrain(diff, -0.05f, 0.05f);
  if (p.t % 100 == 50 && p.y > 10 && p.y < H - 20)
  {
    if (aa) // 地上の砲弾で高射砲を壊しにいく
    {
      float d = sqrtf(best), sp = 1.6f;
      Shot *s = nullptr;
      enemyShotK(p.x, p.y, (tx - p.x) / d * sp, (ty - p.y) / d * sp, 4, 0);
      for (Shot &q : theirs)
        if (q.on && q.kind == 4 && q.life == 0 && q.x == p.x && q.y == p.y)
          s = &q;
      if (s)
        s->tx = d / sp;
    }
    else if (!gone && deadT == 0)
      shootAtK(p.x, p.y, want, 1.0f, 3, 0);
    sfx(Sfx::Flak, 0.5f);
  }
}

void Game::stepAAGun(Prop &p)
{
  if (p.kind != 0 || p.y < 0 || p.y > H - 8)
    return;
  Ufo *tgt = nullptr;
  float best = 120 * 120;
  for (Ufo &u : ufos)
    if (u.on && u.t >= 0 && u.rise >= 1)
    {
      float d2 = (u.x - p.x) * (u.x - p.x) + (u.y - p.y) * (u.y - p.y);
      if (d2 < best)
      {
        best = d2;
        tgt = &u;
      }
    }
  bool momT = momAlive() && mom.t >= 0;
  float tx = tgt ? tgt->x : (momT ? mom.x : p.x), ty = tgt ? tgt->y : (momT ? mom.y : p.y - 40);
  bool haveTarget = tgt || (momT && fabsf(mom.y - p.y) < 160);
  float want = atan2f(ty - p.y, tx - p.x), diff = want - p.ax;
  while (diff > PI)
    diff -= 2 * PI;
  while (diff < -PI)
    diff += 2 * PI;
  p.ax += constrain(diff, -0.08f, 0.08f);
  if (haveTarget && --p.t <= 0) // 撃ち上げる: 円盤の少し先で炸裂する
  {
    p.t = irand(40, 70);
    float lead = tgt ? 12 : 0, ex = tx + (tgt ? tgt->vx * lead * (tgt->type == SPINNER ? 1 : 0.2f) : 0), ey = ty;
    float d = sqrtf((ex - p.x) * (ex - p.x) + (ey - p.y) * (ey - p.y)) + 0.01f;
    for (Shot &f : flak)
      if (!f.on)
      {
        f.x = f.ox = p.x;
        f.y = f.oy = p.y;
        f.tx = ex;
        f.ty = ey;
        f.vx = (ex - p.x) / d * 3.2f;
        f.vy = (ey - p.y) / d * 3.2f;
        f.life = (int)(d / 3.2f) + 1;
        f.kind = 0;
        f.on = true;
        break;
      }
    p.hop = 0;
    p.vy = 3; // 砲口の光る残り (vy を使う)
    sfx(Sfx::Flak, 0.35f);
  }
  if (p.vy > 0)
    p.vy--;
}

void Game::stepFlak()
{
  for (Shot &f : flak)
  {
    if (!f.on)
      continue;
    f.x += f.vx;
    f.y += f.vy;
    if (--f.life > 0)
      continue;
    f.on = false; // 炸裂: 近くの円盤と母船に当たる
    burst(f.x, f.y, 0xD8D8D0, 6, 1.0f, 12);
    burst(f.x, f.y, 0xF0C860, 3, 0.6f, 8);
    for (Ufo &u : ufos)
      if (u.on && u.t >= 0 && u.rise >= 0.7f && fabsf(u.x - f.x) < UFO_R[u.type] + 6 && fabsf(u.y - f.y) < UFO_R[u.type] + 6)
        damageUfo(u, 1);
    if (momAlive() && mom.t >= 0 && (mom.x - f.x) * (mom.x - f.x) + (mom.y - f.y) * (mom.y - f.y) < 36 * 36)
      damageMom(1);
  }
}

// ---- 自機の爆弾: 前の地上に落ち、戦車を壊す ----
void Game::dropBomb()
{
  if (bombCd > 0)
    return;
  int n = 0;
  for (Bomb &b : bombs)
    if (b.on)
      n++;
  if (n >= 2)
    return;
  for (Bomb &b : bombs)
    if (!b.on)
    {
      b.sx = b.x = b.ox = px;
      b.sy = b.y = b.oy = py;
      b.tx = px;
      b.ty = py - BOMB_AHEAD;
      b.t = 0;
      b.on = true;
      bombCd = 14;
      sfx(Sfx::Bomb);
      return;
    }
}

void Game::stepBombs()
{
  if (bombCd > 0)
    bombCd--;
  for (Bomb &b : bombs)
  {
    if (!b.on)
      continue;
    b.t++;
    b.ty += SPEED;
    float k = (float)b.t / BOMB_T;
    b.x = b.sx + (b.tx - b.sx) * k;
    b.y = b.sy + (b.ty - b.sy) * k;
    if (b.t < BOMB_T)
      continue;
    b.on = false; // 地面で爆発
    burst(b.tx, b.ty, 0xFF8A3A, 12, 1.4f, 18);
    burst(b.tx, b.ty, 0x8A7A5A, 8, 1.0f, 22);
    addPuff(b.tx, b.ty, true);
    sfx(Sfx::Boom, 0.6f);
    for (Prop &p : props)
      if (p.on && p.type == TANK && p.lift <= 0.2f && fabsf(p.x - b.tx) < 12 && fabsf(p.y - b.ty) < 12)
      {
        p.on = false;
        score += 500;
        burst(p.x, p.y, 0xC8A8F0, 12, 1.6f, 20);
        burst(p.x, p.y, GOLD, 8, 1.2f, 16);
        addPuff(p.x, p.y, true);
        addPop(p.x, p.y - 12, "500", CREAM);
      }
  }
}

// 自動操縦: 前の地上に戦車がいたら爆弾を落とす
void Game::autoBomb()
{
  for (Prop &p : props)
    if (p.on && p.type == TANK && p.lift <= 0 && fabsf(p.x - px) < 9 && py - p.y > BOMB_AHEAD - 10 && py - p.y < BOMB_AHEAD + 10)
    {
      dropBomb();
      return;
    }
}

// ---- 描く (真上から) ----
// 色を選べる円盤 (いつもの円盤と同じ描き方)
void Game::drawSaucerStyle(Gfx &g, int x, int y, float R, bool flash, int phase, uint32_t body, uint32_t dome)
{
  int ri = (int)ceilf(R);
  if (!g.visible(y - ri, y + ri))
    return;
  float rot = frame * 0.02f + phase;
  for (int dy = -ri; dy <= ri; dy++)
    for (int dx = -ri; dx <= ri; dx++)
    {
      if (!Gfx::rep(x + dx, y + dy))
        continue;
      float d = sqrtf(dx * dx + dy * dy);
      if (d > R + 0.3f)
        continue;
      float t = d / R, lit = (-dx * 0.6f - dy * 0.8f) / R;
      uint32_t c;
      if (t > 0.94f)
        c = UFO_K;
      else if (t > 0.84f)
        c = litc(mulc(body, 0.65f), lit);
      else if (t > 0.79f)
        c = 0x3A3E48;
      else if (t > 0.42f)
      {
        float a = (fastAtan2(dy, dx) - rot) / (2 * PI) * 8;
        c = litc(body, lit * 1.4f);
        if (a - floorf(a) < 0.08f)
          c = mulc(c, 0.75f);
      }
      else if (t > 0.34f)
        c = 0x4A505C;
      else
      {
        c = mix(mulc(dome, 0.55f), mix(dome, 0xFFFFFF, 0.6f), 0.45f + 0.5f * lit);
        if (dx < 0 && dy < 0 && dx * dx + dy * dy < R * R * 0.04f)
          c = 0xFFFFFF;
      }
      if (flash)
        c = mix(c, 0xFFFFFF, 0.75f);
      g.drawPixel(x + dx, y + dy, c);
    }
  for (int i = 0; i < 6; i++)
  {
    float a = frame * 0.15f + phase + i * (PI / 3);
    int lx = x + (int)roundf(cosf(a) * R * 0.89f), ly = y + (int)roundf(sinf(a) * R * 0.89f);
    g.drawPixel(lx, ly, mix(RIM[(i + frame / 8) % 4], 0xFFFFFF, 0.5f));
  }
}

void Game::drawUfoArms(Gfx &g, Ufo &u, int x, int y, float R)
{
  bool fl = u.flash > 0;
  switch (u.type)
  {
  case CARRIER:
    drawSaucerStyle(g, x, y, R, fl, 0, 0xA898C0, 0xC888F0);
    g.drawCircle(x, y, (int)(R * 0.6f), 0x5A4A70);
    if (u.state == 1 && u.a > 50) // ハッチが光る
      g.fillCircle(x, y + (int)(R * 0.55f), 3, (frame / 3) % 2 != 0 ? 0xF0D8FFu : 0xB070E0u);
    break;
  case SPINNER:
    drawSaucerStyle(g, x, y, R, fl, (int)u.a, 0xD8C0A8, 0xF0A040);
    for (int k = 0; k < 3; k++) // 回る羽根
    {
      float a = u.a + k * (2 * PI / 3);
      g.drawLine(x, y, x + (int)(cosf(a) * (R + 3)), y + (int)(sinf(a) * (R + 3)), 0xF08A30);
    }
    break;
  case SPLITTER:
    drawSaucerStyle(g, x, y, R, fl, 1, 0xA8C8A0, 0x60E080);
    for (int k = 0; k < 3; k++) // 3 つのドーム
    {
      float a = frame * 0.03f + k * (2 * PI / 3);
      g.fillCircle(x + (int)(cosf(a) * R * 0.55f), y + (int)(sinf(a) * R * 0.55f), 2, 0x60E080);
    }
    break;
  case MINI:
    drawSaucerStyle(g, x, y, R, fl, 2, 0xA8C8A0, 0x60E080);
    break;
  case LANCER:
  {
    drawSaucerStyle(g, x, y, R, fl, 3, 0xC8A8A8, 0xE84A4A);
    float ch = u.state == 1 ? u.a / 45.0f : u.state == 2 ? 1 : 0; // 力をためると赤いレンズが明るく大きく
    if (ch > 0)
      glow(g, x - 1, y - 1, mix(0xE84A4A, 0xFFFFFF, ch), 2 + (int)(ch * 2));
    break;
  }
  }
}

// 光線円盤の予告の線と、太い光線 (画面の下まで)
void Game::drawBeam(Gfx &g, Ufo &u, int x, int y)
{
  if (u.type != LANCER || u.state < 1 || u.state > 2)
    return;
  int y0 = max(y + 6, g.top), y1 = min(SH, g.top + STRIP); // この帯にかかるところだけ
  if (u.state == 1)
  {
    if ((frame / 2) % 2 == 0)
      for (int yy = y + 8; yy < SH; yy += 3)
        if (yy >= y0 && yy < y1)
          g.blendPixel(x, yy, 0xFF8080, 120);
    return;
  }
  int w = 6 + (int)(2 * sinf(frame * 0.8f));
  for (int yy = y0; yy < y1; yy++)
    for (int i = -w - 3; i <= w + 3; i++)
    {
      int a = abs(i) <= w / 2 ? 255 : abs(i) <= w ? 180 : 70;
      g.blendPixel(x + i, yy, abs(i) <= w / 2 ? 0xFFFFFFu : 0xFF6AB8u, a);
    }
}

void Game::drawEnemyShot(Gfx &g, Shot &s, int x, int y, float k)
{
  if (!g.visible(y - 10, y + 10))
    return;
  bool blink = (frame / 2) % 2 != 0;
  int r;
  switch (s.kind)
  {
  case 1: // 曲がる弾: 水色の玉に尾
    r = max(2, (int)(3 * k));
    g.blendPixel(x - (int)(s.vx * 3 * k), y - (int)(s.vy * 3 * k), 0x60D8F0, 140);
    g.fillCircle(x, y, r, 0x1A4A6A);
    g.fillCircle(x, y, r - 1, blink ? 0xFFFFFFu : 0x60D8F0u);
    break;
  case 2: // ばらまきの小粒: 黄色
    r = max(1, (int)(2 * k));
    g.fillCircle(x, y, r, 0x6A4A10);
    g.drawPixel(x, y, blink ? 0xFFFFFFu : 0xF0D040u);
    if (r > 1)
      g.fillRect(x - r / 2, y - r / 2, r, r, blink ? 0xFFF8C0u : 0xF0D040u);
    break;
  case 3: // ゆっくり大きな玉: 紫
    r = max(3, (int)(5 * k));
    g.fillCircle(x, y, r, 0x3A1A5A);
    g.fillCircle(x, y, r - 1, 0xA060E0);
    g.fillCircle(x - 1, y - 1, max(1, r / 2), blink ? 0xFFFFFFu : 0xE0C0FFu);
    break;
  case 4: // 戦車の地上の砲弾
    g.fillRect(x - 1, y - 1, 2, 2, 0x2A2A2A);
    break;
  default:
    r = max(2, (int)(4 * k));
    g.fillCircle(x, y, r, PLASMA_D);
    g.fillCircle(x, y, r - 1, blink ? 0xFFFFFFu : PLASMA);
    g.drawPixel(x - r / 2, y - r / 2, 0xFFFFFF);
    break;
  }
}

// 戦車 (真上から): 銀紫の車体に緑の灯り、両側に履帯、丸い砲塔と砲身。下ろされているあいだは光線の中
void Game::drawTank(Gfx &g, Prop &p, int x, int y)
{
  int ly = y - (int)(p.lift * 30);
  if (p.lift > 0)
  {
    for (int yy = ly - 30; yy < y; yy++)
      for (int i = -6; i <= 6; i++)
        g.blendPixel(x + i, yy, 0xE0C8FF, 60 - abs(i) * 6);
    g.fillEllipse(x, y + 1, 6, 2, 0x2A3A20);
  }
  g.fillRect(x - 8, ly - 6, 16, 3, 0x2A2A30); // 履帯
  g.fillRect(x - 8, ly + 3, 16, 3, 0x2A2A30);
  for (int i = -8 + (p.t / 4) % 3; i < 8; i += 3)
  {
    g.drawPixel(x + i, ly - 5, 0x5A5A60);
    g.drawPixel(x + i, ly + 4, 0x5A5A60);
  }
  g.fillRect(x - 7, ly - 4, 14, 8, 0x8A80A0); // 車体
  g.drawFastHLine(x - 7, ly - 4, 14, 0xB8B0D0);
  g.drawPixel(x - 5, ly + 2, (frame / 6) % 2 != 0 ? 0x60FF90u : 0x30A060u);
  g.drawPixel(x + 5, ly + 2, (frame / 6) % 2 == 0 ? 0x60FF90u : 0x30A060u);
  g.fillCircle(x, ly, 3, 0x6A6080); // 砲塔と砲身
  g.drawLine(x, ly, x + (int)(cosf(p.ax) * 8), ly + (int)(sinf(p.ax) * 8), 0x3A3448);
  g.drawPixel(x - 1, ly - 1, 0xC8C0E0);
}

// 高射砲: 土のうの輪に砲身 (トラクターは赤い車体に大きな後輪と砲)。壊れると焦げて煙
void Game::drawAAGun(Gfx &g, Prop &p, int x, int y)
{
  if (p.kind == 1)
  {
    g.fillEllipse(x, y, 7, 5, 0x3A2E1E);
    g.fillRect(x - 3, y - 1, 6, 2, 0x4A4A4A);
    return;
  }
  if (p.v == 0)
  {
    for (int k = 0; k < 10; k++) // 土のう
    {
      float a = k * (2 * PI / 10);
      g.fillEllipse(x + (int)(cosf(a) * 6), y + (int)(sinf(a) * 5), 2, 1, (k & 1) != 0 ? 0xC8B488u : 0xB09C70u);
    }
    g.fillCircle(x, y, 2, 0x5A5A50);
  }
  else
  {
    g.fillRect(x - 5, y - 3, 10, 6, 0xC83A2A); // トラクター
    g.fillRect(x - 6, y - 5, 4, 3, 0x1A1A1A);
    g.fillRect(x - 6, y + 2, 4, 3, 0x1A1A1A);
    g.fillRect(x + 3, y - 3, 2, 2, 0x1A1A1A);
    g.fillRect(x + 3, y + 1, 2, 2, 0x1A1A1A);
    g.drawFastHLine(x - 5, y - 3, 10, 0xE85A4A);
  }
  g.drawLine(x, y, x + (int)(cosf(p.ax) * 9), y + (int)(sinf(p.ax) * 9), 0x3A3A36);
  g.drawLine(x + 1, y, x + 1 + (int)(cosf(p.ax) * 9), y + (int)(sinf(p.ax) * 9), 0x5A5A54);
  if (p.vy > 0)
    glow(g, x + (int)(cosf(p.ax) * 10) - 1, y + (int)(sinf(p.ax) * 10) - 1, 0xFFF0A0, 2);
  g.drawPixel(x - 3, y + 3, 0x3A4A6A); // 村人
  g.drawPixel(x - 3, y + 2, 0xE8C8A8);
}

// 高射砲の弾、自機の爆弾、狙いの印 (真上から)
void Game::drawArmsClassic(Gfx &g)
{
  for (Shot &f : flak)
    if (f.on)
    {
      g.fillRect(S(f.rx), S(f.ry), 2, 2, 0xFFE890);
      g.blendPixel(S(f.rx - f.vx), S(f.ry - f.vy), 0xF0C860, 120);
    }
  for (Bomb &b : bombs)
    if (b.on)
    {
      float k = 1 - (float)b.t / BOMB_T;
      int r = max(1, (int)(1 + 2 * k));
      g.fillCircle(S(b.rx), S(b.ry), r, 0x2A2A2A);
      g.drawPixel(S(b.rx) - 1, S(b.ry) - 1, 0x8A8A8A);
    }
  if (mode == Mode::Play && !gone && deadT == 0 && playing() && enterT == 0) // 狙いの印
  {
    int x = S(rpx), y = S(rpy - BOMB_AHEAD);
    uint32_t c = (frame / 8) % 2 != 0 ? 0xFFF0B0u : 0xE8C060u;
    g.drawCircle(x, y, 7, 0x3A3020);
    g.drawCircle(x, y, 6, c);
    g.drawFastHLine(x - 5, y, 3, c);
    g.drawFastHLine(x + 3, y, 3, c);
    g.drawFastVLine(x, y - 5, 3, c);
    g.drawFastVLine(x, y + 3, 3, c);
  }
}
