// ゲームの中身 (地形、円盤、母船、自機、天気、遊びの流れ)。Windows 版の Game.cs を移したもの
#include "game.h"

const uint32_t Game::MOOR_C[5] = {0, 0x7A8A4A, 0x86707A, 0x9A9488, 0xB8B4AA}; // 山の高さ 1〜4 (荒れ地 → ヒース → 岩 → 頂)
const uint32_t Game::RIM[4] = {0xFF5050, 0xFFD040, 0x60FF80, 0x60C0FF};
const Game::Crop Game::CROPS[7] = {
    {0x6E9A48, 0x5E8A3C, 0}, // 牧草地 (羊がいる)
    {0x9AB257, 0xE8E0A0, 0}, // 花の咲く草地 (牛がいる)
    {0xC8B060, 0xA89048, 1}, // 麦 (干し草ロール)
    {0x8A6A44, 0x6E5232, 1}, // 耕した畑
    {0xD8C840, 0xB8A830, 0}, // 菜の花
    {0x4E7A3A, 0x3E6A2E, 2}, // 野菜
    {0x6A9AA8, 0x4E8A3A, 3}, // 水田
};
const int Game::UFO_HP[8] = {2, 2, 6, 12, 2, 4, 1, 5}; // 3 以降は game_arms.cpp の円盤
const int Game::UFO_PTS[8] = {200, 300, 1500, 2000, 250, 400, 100, 800};
const int Game::UFO_R[8] = {6, 6, 9, 13, 5, 8, 4, 8};

uint32_t Game::mix(uint32_t a, uint32_t b, float t) // a と b を t (0〜1) で混ぜる (整数で計算する)
{
  if (t <= 0)
    return a;
  if (t >= 1)
    return b;
  int k = (int)(t * 256);
  int ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
  int r = ar + ((((int)(b >> 16) & 255) - ar) * k >> 8), g = ag + ((((int)(b >> 8) & 255) - ag) * k >> 8), bb = ab + ((((int)b & 255) - ab) * k >> 8);
  return (r << 16) | (g << 8) | bb;
}

Game::Game()
{
  for (int r = 0; r < ROWS; r++)
    rowCache[r] = nullptr;
  rngState = esp_random() | 1;
  StartDemo();
}

float Game::rainK() const
{
  float k = ramp(rainAt(), rainAt() + 6 * FPS) - ramp(rainEnd(), rainEnd() + 6 * FPS);
  return region == 1 ? k * 0.75f : region == 3 ? 0 : region == 4 ? k * 0.7f : k; // 湖水地方は霧雨、コーンウォールは晴れ、高地は雪
}

// ---- 火花 ----
void Game::burst(float x, float y, uint32_t rgb, int n, float speed, int life)
{
  for (Spark &q : sparks)
  {
    if (n <= 0)
      break;
    if (q.life > 0)
      continue;
    float a = frand(0, 2 * PI), s = frand(0.3f, 1.0f) * speed;
    q.x = q.ox = x;
    q.y = q.oy = y;
    q.vx = cosf(a) * s;
    q.vy = sinf(a) * s;
    q.life = irand(life / 2, life);
    q.max = life;
    q.rgb = rgb;
    n--;
  }
}

// ---- 地形 ----
float Game::hash2(int x, int y) const
{
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed;
  h = (h ^ (h >> 13)) * 1274126177u;
  return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}
float Game::noise(float x, float y) const
{
  int ix = (int)floorf(x), iy = (int)floorf(y);
  float fx = x - ix, fy = y - iy;
  fx = fx * fx * (3 - 2 * fx);
  fy = fy * fy * (3 - 2 * fy);
  float a = hash2(ix, iy), b = hash2(ix + 1, iy), c = hash2(ix, iy + 1), d = hash2(ix + 1, iy + 1);
  return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

static const uint8_t VALE[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
static const uint8_t FELL[] = {0, 0, 0, 0, 0, 0, 1, 1, 2, 3};
static const uint8_t LAKES[] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 2};
static const uint8_t YORKS[] = {0, 0, 0, 0, 2, 2, 3, 3, 3, 1}; // 秋: 刈り取ったあとの麦と耕した畑
static const uint8_t CORNW[] = {0, 0, 0, 1, 1, 1, 2, 4, 4, 5};
static const uint8_t HIGHL[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 3};
int Game::pickCrop()
{
  // 山のほうは牧草地ばかり。谷は色とりどり。地方ごとに作物が違う
  if (region == 1)
    return LAKES[irand(0, 9)];
  if (region == 2)
    return YORKS[irand(0, 9)];
  if (region == 3)
    return CORNW[irand(0, 9)];
  if (region == 4)
    return HIGHL[irand(0, 9)];
  return fellK() > 0.5f ? FELL[irand(0, 9)] : VALE[irand(0, 19)];
}

void Game::setSeg(int p, int w)
{
  segW[p] = w;
  segLeft[p] = irand(3, 9);
  segCrop[p] = pickCrop();
  for (int k = p + 1; k < p + w; k++)
    segW[k] = 0;
}
// [c, e) を幅 3〜8 の区画に分け直す
void Game::splitSegs(int c, int e)
{
  for (int p = c; p < e;)
  {
    int w = min(irand(3, 8), e - p);
    if (e - (p + w) < 3)
      w = e - p;
    setSeg(p, w);
    p += w;
  }
}

// 畑の 1 行。区画が終わったら、ときどき右どなりとまとめて分け直すので、境の生け垣の位置が動いていく
void Game::fieldRow(Tile *row)
{
  int wall = fellK() > 0.5f || region == 1 || region == 2 || region == 4 ? STONEWALL : 0; // 湖水地方とヨークシャーと高地はどこも石垣
  for (int c = 0; c < COLS;)
  {
    int e = c + segW[c];
    bool top = false;
    if (--segLeft[c] < 0)
    {
      if (e < COLS && irand(0, 1) != 0)
        e += segW[e];
      splitSegs(c, e);
      top = true;
    }
    for (int p = c; p < e; p += segW[p])
      for (int k = p; k < p + segW[p]; k++)
        row[k] = {FIELD, (uint8_t)segCrop[p], (uint8_t)((top ? HEDGE_TOP : 0) | (k == p && k > 0 ? HEDGE_LEFT : 0) | wall)};
    c = e;
  }
}

void Game::genRow(Tile *row, int n)
{
  float f = fellK();
  fieldRow(row);
  for (int c = 0; c < COLS; c++)
  {
    // 雑木林
    if (noise(c * 0.25f, n * 0.08f + 77) > (region == 1 ? 0.71f : region == 2 ? 0.74f : region == 4 ? 0.8f : 0.76f))
      row[c] = {WOOD, 0, 0};
    // 山: 高さが閾値を超えたところ。山がちになるほど閾値が下がる
    float h = noise(c * 0.16f, n * 0.05f + 300);
    float thr = region == 1 ? 0.80f - 0.30f * f : region == 4 ? 0.72f - 0.30f * f : region == 3 ? 0.88f - 0.30f * f : 0.86f - 0.36f * f;
    if (h > thr)
      row[c] = {MOOR, (uint8_t)constrain(1 + (int)((h - thr) / 0.06f), 1, 4), 0};
  }
  // 川: くねりながら流れ、両岸は葦の生えた土手
  if (riverOn)
  {
    riverDrift = constrain(riverDrift + frand(-0.06f, 0.06f), -0.3f, 0.3f);
    riverC += riverDrift;
    if (riverC < 2 || riverC > COLS - 3)
      riverDrift = -riverDrift;
    riverC = constrain(riverC, 2.0f, COLS - 3.0f);
    riverW = constrain(riverW + frand(-0.12f, 0.12f), 1.4f, 3.2f);
    for (int c = 0; c < COLS; c++)
    {
      float d = fabsf(c + 0.5f - riverC);
      if (d < riverW / 2)
        row[c] = {WATER, 0, 0};
      else if (d < riverW / 2 + 1)
        row[c] = {BANK, 0, 0};
    }
    if (--riverLen <= 0)
      riverOn = false;
  }
  else if (irand(0, 70) == 0)
  {
    riverOn = true;
    riverC = frand(3, COLS - 4);
    riverW = frand(1.6f, 2.8f);
    riverDrift = 0;
    riverLen = irand(60, 140);
  }
  // 小道: ときどき 1 列ずれながら続く。川の上は石の橋
  if (laneOn)
  {
    int old = laneCol;
    if (irand(0, 11) == 0)
      laneCol = constrain(laneCol + (irand(0, 1) != 0 ? 1 : -1), 2, COLS - 4);
    for (int c = min(old, laneCol); c <= max(old, laneCol); c++)
      row[c] = {row[c].type == WATER ? BRIDGE : LANE_V, 0, 0};
    if (--laneLen <= 0)
      laneOn = false;
  }
  else if (lochLen == 0 && irand(0, 60) == 0)
  {
    laneOn = true;
    laneCol = irand(3, COLS - 5);
    laneLen = irand(40, 100);
  }
  if (irand(0, 90) == 0) // 横切る小道
  {
    for (int c = 0; c < COLS; c++)
      if (row[c].type != LANE_V)
        row[c] = {row[c].type == WATER ? BRIDGE : LANE_H, 0, 0};
  }
  else if (villageLen == 0 && plainLen == 0 && lochLen == 0 && irand(0, 110) == 0) // 横切る線路。小道とは踏切で交わる
  {
    for (int c = 0; c < COLS; c++)
      row[c] = {row[c].type == WATER ? RAILBRIDGE : RAIL, 0, 0};
    railRow = true;
  }
  // 村: 小道の両側を芝にして小屋を並べる。真ん中あたりに教会
  if (villageLen > 0 && laneOn)
  {
    for (int c = laneCol - 2; c <= laneCol + 3; c++)
      if (c >= 0 && c < COLS && (row[c].type == FIELD || row[c].type == WOOD))
        row[c] = {VGREEN, 0, 0};
    villageLen--;
  }
  else if (laneOn && plainLen == 0 && lochLen == 0 && irand(0, f < 0.7f ? 50 : 110) == 0) // 山あいにも、ときどき小さな集落
  {
    villageLen = irand(7, 12);
    churchAt = villageLen / 2;
    pubPlaced = false;
  }
  // ミステリーサークルの麦畑: 生け垣で囲った大きな麦畑 (川や小道のマスはそのまま)
  if (cropLen > 0)
  {
    for (int c = cropC - 6; c <= cropC + 6; c++)
      if (c >= 0 && c < COLS)
      {
        uint8_t t = row[c].type;
        if (t == FIELD || t == WOOD || t == MOOR || t == VGREEN || t == PLAIN)
          row[c] = {FIELD, WHEAT, (uint8_t)((c == cropC - 6 ? HEDGE_LEFT : 0) | (cropLen == CROP_ROWS ? HEDGE_TOP : 0))};
      }
    if (cropC + 7 < COLS && row[cropC + 7].type == FIELD)
      row[cropC + 7].flags |= HEDGE_LEFT;
    if (--cropLen == 0)
      cropTopNext = true;
  }
  else if (cropTopNext) // 麦畑の上の縁にも生け垣
  {
    for (int c = max(0, cropC - 6); c <= min(COLS - 1, cropC + 6); c++)
      if (row[c].type == FIELD)
        row[c].flags |= HEDGE_TOP;
    cropTopNext = false;
  }
  // 白亜の草原: 畑も生け垣も林も消えて、一面のなだらかな草地 (小道と川はそのまま)
  if (plainLen > 0)
  {
    for (int c = 0; c < COLS; c++)
    {
      uint8_t t = row[c].type;
      if (t == FIELD || t == WOOD || t == MOOR || t == VGREEN)
        row[c] = {PLAIN, 0, 0};
    }
    plainLen--;
  }
  // 湖水地方と高地: ときどき湖ができる
  if ((region == 1 || region == 4) && lochLen == 0 && plainLen == 0 && --lakeCool <= 0)
  {
    lochTotal = lochLen = irand(14, 22);
    lochR = frand(3.5f, 6.0f);
    lochC = frand(lochR + 2, COLS - lochR - 3);
    laneOn = false;
    lakeCool = region == 1 ? irand(40, 90) : irand(70, 130);
  }
  // 湖 (ロッホ): 上下にふくらんだ楕円の水。まわりは葦の土手
  if (lochLen > 0)
  {
    float tt = (lochTotal - lochLen + 0.5f) / lochTotal, hw = lochR * sqrtf(max(0.0f, 1 - (2 * tt - 1) * (2 * tt - 1)));
    lochC = constrain(lochC + frand(-0.15f, 0.15f), lochR + 1, COLS - lochR - 2);
    for (int c = 0; c < COLS; c++)
    {
      float dd = fabsf(c + 0.5f - lochC);
      if (dd < hw)
        row[c] = {WATER, 0, 0};
      else if (dd < hw + 1.2f)
        row[c] = {BANK, 0, 0};
    }
    lochLen--;
  }
  regionTerrain(row, n);
}

Game::Prop *Game::addProp(float x, float y, int type, int v)
{
  for (Prop &p : props)
    if (!p.on)
    {
      p.x = p.ox = x;
      p.y = p.oy = y;
      p.lift = p.vx = p.vy = 0;
      p.ax = x;
      p.ay = y;
      p.type = type;
      p.v = v;
      p.t = p.hop = p.spawned = p.kind = 0;
      p.on = true;
      p.held = false;
      return &p;
    }
  return nullptr;
}

void Game::newRow()
{
  // 行をひとつずつ下へずらす (いちばん下の行は上に回して作り直す)
  Tile last[COLS];
  memcpy(last, map[ROWS - 1], sizeof(last));
  memmove(map[1], map[0], sizeof(Tile) * COLS * (ROWS - 1));
  memcpy(map[0], last, sizeof(last));
  memmove(rowId + 1, rowId, sizeof(int) * (ROWS - 1));
  shiftRowCache();
  rowId[0] = rowCount++;
  planLandmarks(); // 名所の地形を作りはじめる (地方ごと)
  genRow(map[0], rowId[0]);
  // 畑や村の上に置くもの
  float y = scrollY - T + 4 - FAR * T;
  if (railRow)
  {
    railRow = false;
    if (!train.on && !railPending && (forceTrain || irand(0, 9) < 8)) // 線路ができたら、たいてい汽車が来る (線路が画面の上の端まで来たら走りだす)
    {
      forceTrain = false;
      railPending = true;
      railY = y;
    }
  }
  bool village = villageLen > 0 && laneOn;
  if (village && (rowId[0] & 1) != 0)
  {
    int sd = irand(0, 1) != 0 ? 1 : -1;
    float x = (laneCol + 0.5f + sd * 2.0f) * T + (sd > 0 ? 4 : 0);
    if (villageLen == churchAt)
      addProp((laneCol + 0.5f - sd * 3.5f) * T, y, CHURCH);
    if (!pubPlaced && villageLen <= 3 && villageLen != churchAt) // 村のはずれにパブ
    {
      addProp(x + sd * 2, y, PUB, sd > 0 ? 1 : 0);
      pubPlaced = true;
    }
    else
      addProp(x, y, COTTAGE, (region == 1 || irand(0, 2) == 0 ? 1 : 0) | (irand(0, 2) == 0 ? LAUNDRY : 0));
  }
  for (int c = 0; c < COLS; c++)
  {
    Tile t = map[0][c];
    float x = c * T + 4, r = frand(0, 1);
    if (t.type == FIELD)
    {
      if (t.v == PASTURE && r < 0.02f + 0.02f * fellK())
      {
        bool cows = region == 4 && irand(0, 9) < 6; // 高地はハイランド牛が多い
        for (int i = 0, n = irand(3, 5); i < n; i++)
          addProp(x + frand(-8, 8), y + frand(-6, 6), cows ? COW : SHEEP, irand(0, 1));
        if (irand(0, 4) == 0) // ときどき牧羊犬がついている
        {
          Prop *d = addProp(x, y, DOG);
          if (d)
            d->t = irand(0, 300);
        }
      }
      else if (t.v == MEADOW && r < 0.012f)
        for (int i = 0, n = irand(2, 3); i < n; i++)
          addProp(x + frand(-8, 8), y + frand(-6, 6), COW, irand(0, 1));
      else if (t.v == WHEAT && r < 0.015f)
        for (int i = 0, n = irand(2, 4); i < n; i++)
          addProp(x - 6 + i * 6, y, BALE);
      else if ((t.flags & (HEDGE_TOP | HEDGE_LEFT)) != 0 && (t.flags & STONEWALL) == 0 && r < 0.05f)
        addProp(x - 3 + irand(0, 6), y + ((t.flags & HEDGE_TOP) != 0 ? 3 : 0), TREE, irand(0, 1));
      else if (laneOn && !village && abs(c - laneCol) == 2 && r < 0.012f) // 小道ぞいの一軒家と納屋
      {
        addProp(x, y, COTTAGE, (region == 1 ? 1 : irand(0, 1)) | (irand(0, 1) != 0 ? LAUNDRY : 0));
        addProp(x + (c > laneCol ? 14 : -14), y - 4, BARN);
      }
    }
    else if (t.type == MOOR && t.v >= 3 && r < 0.05f)
      addProp(x + frand(-3, 3), y, ROCK);
    if (t.type == FIELD && (t.v == PASTURE || t.v == MEADOW) && frand(0, 1) < (region == 1 ? 0.012f : 0.003f)) // 野うさぎ
    {
      Prop *rb = addProp(x + frand(-4, 4), y + frand(-4, 4), RABBIT, irand(0, 1));
      if (rb)
      {
        rb->ay = 0;
        rb->t = irand(0, 69);
      }
    }
  }
  if (frand(0, 1) < 0.03f) // 村人の高射砲 (土のうの陣地か、大砲を積んだトラクター)
  {
    int c = irand(2, COLS - 3);
    if (map[0][c].type == FIELD || map[0][c].type == PLAIN)
    {
      Prop *aa = addProp(c * T + 4, y, AAGUN, map[0][c].type == FIELD && map[0][c].v == WHEAT ? 1 : irand(0, 1));
      if (aa)
      {
        aa->ax = -PI / 2;
        aa->t = irand(20, 60);
      }
    }
  }
  placeLandmarks(y);
  regionProps(y);
  for (int c = 0; c < COLS; c++) // 草原にも羊
    if (map[0][c].type == PLAIN && frand(0, 1) < 0.012f && abs(c - stoneCol) > 3)
      for (int i = 0, n = irand(3, 6); i < n; i++)
        addProp(c * T + 4 + frand(-8, 8), y + frand(-6, 6), SHEEP, irand(0, 1));
  // カモの親子: 川の岸から向こう岸へ、親を先頭に一列で泳いで渡る
  int wl = -1, wr = -1;
  for (int c = 0; c < COLS; c++)
    if (map[0][c].type == WATER)
    {
      if (wl < 0)
        wl = c;
      wr = c;
    }
  if (region == 1 && wr - wl >= 3 && irand(0, 10) == 0) // 湖水地方: 湖に手こぎボートやヨット
  {
    Prop *b = addProp((wl + 1 + frand(0, wr - wl - 1)) * T, y + 2, BOAT, irand(0, 1));
    if (b)
      b->vx = (irand(0, 1) != 0 ? 1 : -1) * frand(0.06f, 0.14f);
  }
  if (wl >= 0 && wr > wl && lochLen == 0 && irand(0, 40) == 0)
  {
    int dir = irand(0, 1) != 0 ? 1 : -1;
    float x0 = dir > 0 ? wl * T - 2 : (wr + 1) * T + 2;
    for (int i = 0, n = irand(3, 5); i <= n; i++)
    {
      Prop *d = addProp(x0 - dir * i * 5, y + 3, DUCK, i);
      if (d)
        d->vx = dir * 0.16f;
    }
  }
}

bool Game::hasProp(int type) const
{
  for (const Prop &p : props)
    if (p.on && p.type == type)
      return true;
  return false;
}

// 世界の座標の地面のマス (なければ -1)
int Game::tileTypeAt(float x, float y) const
{
  int r = (int)floorf((y - scrollY) / T) + 1 + FAR, c = (int)floorf(x / T);
  if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
    return -1;
  return map[r][c].type;
}

void Game::addPop(float x, float y, const char *text, uint32_t col)
{
  for (Pop &q : pops)
    if (!q.on)
    {
      q.x = q.ox = x;
      q.y = q.oy = y;
      q.t = 0;
      q.text = text;
      q.col = col;
      q.on = true;
      return;
    }
}

int Game::moorLevel(int r, int c) const
{
  if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
    return -1;
  return map[r][c].type == MOOR ? map[r][c].v : 0;
}

// ---- 弾 ----
void Game::enemyShot(float x, float y, float vx, float vy)
{
  float k = bulletK();
  for (Shot &s : theirs)
    if (!s.on)
    {
      s.x = s.ox = x;
      s.y = s.oy = y;
      s.vx = vx * k;
      s.vy = vy * k;
      s.kind = 0;
      s.life = 0;
      s.on = true;
      enemyShotSnd = true;
      return;
    }
}

// ---- 円盤 ----
Game::Ufo *Game::addUfo(int type, float x, float y, int t)
{
  for (Ufo &u : ufos)
    if (!u.on)
    {
      u.x = u.ox = x;
      u.y = u.oy = y;
      u.vx = u.vy = 0;
      u.home = x;
      u.a = 0;
      u.type = type;
      u.t = t;
      u.hp = UFO_HP[type];
      u.flash = 0;
      u.group = -1;
      u.state = 0;
      u.prop = -1;
      u.rise = 1;
      u.riseKind = 0;
      u.on = true;
      return &u;
    }
  return nullptr;
}

bool Game::spawnAbductor()
{
  int best = -1;
  for (int i = 0; i < NPROPS; i++)
  {
    Prop &p = props[i];
    if (p.on && !p.held && (p.type == SHEEP || p.type == COW) && p.y > 30 && p.y < 130 && p.x > 20 && p.x < W - 20)
    {
      best = i;
      break;
    }
  }
  if (best < 0)
    return false;
  Ufo *u = addUfo(ABDUCTOR, props[best].x < W / 2 ? W + 14 : -14, 30, 0);
  if (u)
  {
    u->prop = best;
    props[best].held = true;
    return true;
  }
  return false;
}

void Game::spawnWave()
{
  int kind = pickWave();
  if (kind >= 3)
  {
    spawnArms(kind);
    return;
  }
  if (kind == 2 && spawnAbductor())
    return;
  if (kind == 1 || kind == 2)
  {
    // 3 機の輪
    float cx = frand(50, W - 50), home = frand(70, 140);
    groupN++;
    for (int i = 0; i < 3; i++)
    {
      Ufo *u = addUfo(TRIO, cx, -20, 0);
      if (u)
      {
        u->vx = cx;
        u->home = home;
        u->a = i * (2 * PI / 3);
        u->group = groupN;
      }
    }
    return;
  }
  // 偵察機が 2〜4 機 (日がたつと増える)、少しずつ遅れて
  for (int i = 0, n = irand(2, 3 + area() - 1) + min(day - 1, 2); i < n; i++)
  {
    Ufo *u = addUfo(SCOUT, 0, -12, -i * 22);
    if (u)
    {
      u->home = frand(30, W - 30);
      u->x = u->home;
      u->vy = frand(1.1f, 1.5f);
      u->a = frand(0, 2 * PI);
    }
  }
}

void Game::stepUfo(Ufo &u)
{
  if (++u.t < 0)
    return;
  if (u.flash > 0)
    u.flash--;
  if (u.rise < 1) // 地面 (湖) から出てくるところ: 地面と一緒に流れながら浮かびあがる
  {
    u.rise = min(1.0f, u.rise + 1.0f / (u.riseKind == 1 ? 60 : 45));
    u.y += SPEED;
    if (u.rise >= 1) // 出きったら、ふつうの偵察機として蛇行しながら降りてくる
    {
      u.home = u.x;
      u.a = -(u.t + 1) * 0.05f;
    }
    return;
  }
  if (!stepUfoArms(u))
    switch (u.type)
    {
    case SCOUT:
      u.y += u.vy;
      u.x = u.home + 28 * sinf(u.t * 0.05f + u.a);
      if (u.t % 70 == 40 && u.y > 20 && u.y < H - 120)
        aimed(u.x, u.y, 1.8f);
      break;
    case TRIO:
    {
      float cy = u.t < 120 ? -20 + (u.home + 20) * (u.t / 120.0f) : u.t < 360 ? u.home : u.home - (u.t - 360) * 1.5f;
      float cx = u.vx + 30 * sinf(u.t * 0.012f);
      float ang = u.t * 0.04f + u.a;
      u.x = cx + 20 * cosf(ang);
      u.y = cy + 20 * sinf(ang);
      if (u.t > 120 && u.t < 360 && (u.t + (int)(u.a * 30)) % 90 == 0)
        aimed(u.x, u.y, 1.7f);
      break;
    }
    case ABDUCTOR:
    {
      Prop *p = u.prop >= 0 && props[u.prop].on ? &props[u.prop] : nullptr;
      if (u.state < 2 && !p)
        u.state = 2;
      if (u.state == 0)
      {
        // 羊の少し上 (画面では上) へ近づく
        float dx = p->x - u.x, dy = p->y - 26 - u.y, d = sqrtf(dx * dx + dy * dy) + 0.01f;
        if (d < 2.5f)
          u.state = 1;
        else
        {
          u.x += dx / d * 1.6f;
          u.y += dy / d * 1.6f + SPEED;
        }
        if (u.t % 80 == 60 && u.y > 20)
          aimed(u.x, u.y, 1.6f);
      }
      else if (u.state == 1)
      {
        // 光線で持ち上げる。上がりきったら連れ去る
        u.x = p->x;
        u.y = p->y - 26;
        if ((p->lift += 1.0f / 150) >= 1)
        {
          p->on = false;
          u.state = 2;
          // 連れ去られる: 遠ざかる鳴き声
          addPop(p->x, p->y - 22, p->type == SHEEP ? "BAA..." : "MOO...", 0xD8DCE0);
          sfx(p->type == SHEEP ? Sfx::BaaAway : Sfx::MooAway);
        }
      }
      else
      {
        u.vy = max(u.vy - 0.08f, -3.0f);
        u.y += u.vy;
      }
      break;
    }
    }
  if (u.y > H + 20 || u.y < -40 || u.x < -30 || u.x > W + 30)
  {
    u.on = false;
    if (u.type == ABDUCTOR && u.prop >= 0)
      props[u.prop].held = false;
  }
}

void Game::damageUfo(Ufo &u, int n)
{
  u.flash = 3;
  if ((u.hp -= n) > 0)
  {
    sfx(Sfx::Hit);
    return;
  }
  u.on = false;
  score += UFO_PTS[u.type];
  onUfoDestroyed(u);
  sfx(Sfx::Boom, u.type == ABDUCTOR ? 1 : 0.7f);
  burst(u.x, u.y, SILVER_L, 12, 1.6f, 18);
  burst(u.x, u.y, RIM[irand(0, 3)], 6, 1.0f, 14);
  if (u.type == ABDUCTOR && u.prop >= 0)
  {
    Prop &p = props[u.prop];
    p.held = false; // 光線が切れて、そっと地面に戻る
    if (u.state == 1 && p.on)
    {
      saved++;
      score += 1000;
      sfx(Sfx::Saved);
      addPop(p.x, p.y - 10, "SAVED", GOLD);
      // うれしそうに跳ねながら、いちばん近い仲間のところへ戻る
      float best = 60 * 60, tx = p.x, ty = p.y;
      for (Prop &o : props)
        if (&o != &p && o.on && o.type == p.type && !o.held)
        {
          float d2 = (o.x - p.x) * (o.x - p.x) + (o.y - p.y) * (o.y - p.y);
          if (d2 < best)
          {
            best = d2;
            tx = o.x;
            ty = o.y;
          }
        }
      p.hop = 80;
      p.vx = constrain((tx - p.x) / 70, -0.4f, 0.4f);
      p.vy = constrain((ty - p.y) / 70, -0.3f, 0.3f);
      p.v = tx >= p.x ? 1 : 0;
    }
  }
  if (u.type == TRIO)
  {
    for (Ufo &o : ufos)
      if (o.on && o.group == u.group)
        return;
    if (!item.on) // 輪を全部落とすと P
    {
      item.x = item.ox = u.x;
      item.y = item.oy = u.y;
      item.t = 0;
      item.on = true;
    }
  }
}

// ---- 母船 ----
void Game::startMom()
{
  mom.on = true;
  mom.dead = false;
  mom.x = W / 2.0f;
  mom.y = -50;
  mom.t = -1;
  mom.maxHp = mom.hp = (160 + 50 * (day - 1)) * (region == 4 ? 3 : 2) / 2; // 最後の日は大母船
  mom.hitT = mom.deadT = 0;
  sfx(Sfx::Warning);
}

void Game::addPuff(float x, float y, bool smoke, float vx)
{
  for (Puff &q : puffs)
    if (!q.on)
    {
      q.x = q.ox = x;
      q.y = q.oy = y;
      q.vx = vx;
      q.r = 1.5f;
      q.t = 0;
      q.life = smoke ? irand(50, 70) : irand(30, 45);
      q.smoke = smoke;
      q.on = true;
      return;
    }
}

void Game::damageMom(int n)
{
  mom.hitT = 2;
  if ((mom.hp -= n) > 0)
  {
    sfx(Sfx::MomHit);
    return;
  }
  // 落とした: 火を噴き、かけらを撒きながら地面へ落ちていく
  mom.dead = true;
  mom.deadT = CRASH_T;
  score += region == 4 ? 50000 : 30000;
  sfx(Sfx::BigBoom);
  for (Shot &s : theirs)
    s.on = false;
  burst(mom.x, mom.y, 0xFFFFFF, 20, 2.4f, 26);
  for (DebrisP &d : debris)
  {
    d.x = mom.x + frand(-20, 20);
    d.y = mom.y + frand(-16, 16);
    d.vx = frand(-1.6f, 1.6f);
    d.vy = frand(-1.2f, 1.2f);
    d.alt = 1;
    d.da = 1.0f / irand(40, 100);
    d.ang = frand(0, 2 * PI);
    d.on = true;
    keep(d);
  }
}

void Game::stepDebris()
{
  for (DebrisP &d : debris)
  {
    if (!d.on)
      continue;
    d.x += d.vx;
    d.y += d.vy + SPEED * (1 - d.alt); // 低くなるほど地面と一緒に流れる
    d.vx *= 0.98f;
    d.vy *= 0.98f;
    d.ang += 0.3f;
    if (frame % 4 == 0)
      addPuff(d.x, d.y, true);
    if ((d.alt -= d.da) <= 0)
    {
      d.on = false;
      addProp(d.x, d.y, DEBRIS);
      burst(d.x, d.y, 0xFF8A3A, 5, 0.8f, 12);
      sfx(Sfx::Boom, 0.35f);
    }
  }
}

void Game::stepMom()
{
  if (mom.hitT > 0)
    mom.hitT--;
  if (mom.dead)
  {
    // 傾いてぐるぐる回りながら、煙の尾を引いて落ちる。低くなるほど地面と一緒に流れる
    float alt = (float)mom.deadT / CRASH_T;
    mom.y += 0.25f + SPEED * (1 - alt);
    mom.x += mom.x < W / 2 ? -0.15f : 0.15f;
    mom.t += 3;
    if (mom.deadT % 3 == 0)
      addPuff(mom.x + frand(-20, 20), mom.y + frand(-14, 14), true);
    if (mom.deadT % 6 == 0)
    {
      burst(mom.x + frand(-28, 28), mom.y + frand(-20, 20), mom.deadT % 12 != 0 ? GOLD : 0xFF6A30, 6, 1.2f, 18);
      if (mom.deadT % 24 == 0)
        sfx(Sfx::Boom, 0.5f);
    }
    if (--mom.deadT <= 0)
    {
      // 地面に落ちた: 大きな爆発と光。残骸が残って煙を上げる
      burst(mom.x, mom.y, 0xFFFFFF, 30, 3.0f, 40);
      burst(mom.x, mom.y, 0xFF8A3A, 30, 2.2f, 36);
      burst(mom.x, mom.y, 0x6A5232, 20, 1.4f, 30);
      flashT = 4;
      addProp(mom.x, mom.y, WRECK);
      for (int i = 0; i < 8; i++)
        addPuff(mom.x + frand(-20, 20), mom.y + frand(-12, 12), true);
      sfx(Sfx::Crash);
      mom.on = false;
    }
    return;
  }
  if (mom.t < 0)
  {
    mom.y += 1.0f;
    if (mom.y >= MOM_Y)
      mom.t = 0;
    return;
  }
  if (++mom.t > MOM_STAY || frame > CLEAR_AT - 4 * FPS)
  {
    mom.y -= 1.2f;
    if (mom.y < -70)
      mom.on = false;
    return;
  }
  mom.x = W / 2 + 26 * sinf(mom.t * 0.012f);
  bool angry = mom.hp < mom.maxHp / 2;
  // 弾の数は Windows 版の 6 割ほど (画面が小さく、弾が多いと見づらくて重いので): 輪 14 → 8 発 (3 日目から 18 → 11)、
  // 狙い撃ち 5 → 3 発、曲がる弾 3 → 2 組
  int u = mom.t % 300, ring = day >= 3 ? 11 : 8;
  if (u % (angry ? 36 : 50) == 0) // 輪
    for (int i = 0; i < ring; i++)
      shootAt(mom.x, mom.y, i * (2 * PI / ring) + mom.t * 0.05f, 1.2f);
  if (u >= 100 && u < 170 && u % 24 == 4) // 回る砲口から狙い撃ち
  {
    float a = portAngle((u / 24) % 4);
    aimed(mom.x + cosf(a) * 22, mom.y + sinf(a) * 22, 2.0f);
  }
  if (angry && u >= 180 && u < 240 && u % 30 == 0) // 怒ると、左右へ曲がる弾も混ぜる
    for (int sd = -1; sd <= 1; sd += 2)
      shootAtK(mom.x + sd * 26, mom.y + 6, angTo(mom.x + sd * 26, mom.y + 6) - sd * 0.6f, 1.3f, 1, sd * 0.016f);
  if (u == 220 || u == 260) // ハッチから偵察機を出す
    for (int sd = -1; sd <= 1; sd += 2)
    {
      Ufo *s = addUfo(SCOUT, mom.x + sd * 22, mom.y + 10, 0);
      if (s)
      {
        s->home = mom.x + sd * 22;
        s->vy = 1.3f;
        s->a = sd > 0 ? PI : 0;
      }
    }
}

// ---- 自機 ----
void Game::killPlayer()
{
  burst(px, py, GOLD, 30, 2.0f, 30);
  burst(px, py, 0xFFFFFF, 12, 1.0f, 20);
  burst(px, py, 0x6A6A6A, 10, 0.6f, 30); // 煙
  sfx(Sfx::Death);
  deadT = 60;
  if (demo) // デモは残りがなくてもゲームオーバーにはしない
    lives = max(0, lives - 1);
  else if (lives == 0)
    outOfLives = true;
  else
    lives--;
  power = max(1, power - 1);
}

void Game::shoot(float x, float y, float vx)
{
  for (Shot &s : mine)
    if (!s.on)
    {
      s.x = s.ox = x;
      s.y = s.oy = y;
      s.vx = vx;
      s.vy = -7;
      s.on = true;
      return;
    }
}

// 1 回押したぶんの弾。強くなるほど本数が増える
void Game::volley()
{
  shoot(px - 5, py - 3, 0);
  shoot(px + 5, py - 3, 0);
  if (power >= 2)
  {
    shoot(px - 8, py - 1, -0.6f);
    shoot(px + 8, py - 1, 0.6f);
  }
  if (power >= 3)
    shoot(px, py - 9, 0);
  muzzleT = 2;
  sfx(Sfx::Shot);
}

void Game::think()
{
  // 行きたいところ: P があれば取りに行く。なければ羊をさらっている円盤、母船、一番下の円盤の真下
  float tx = W / 2, ty = PY0;
  bool urgent = false;
  if (item.on && item.y > 30 && item.y < H - 20)
  {
    tx = item.x;
    ty = constrain(item.y + 10, PY0 - 70.0f, (float)PY0);
  }
  else
  {
    float lowest = -1e9f;
    for (Ufo &u : ufos)
      if (u.on && u.t >= 0 && u.type == ABDUCTOR && u.state < 2 && u.y > 0)
      {
        tx = u.x;
        urgent = true;
      }
    if (!urgent)
    {
      if (momAlive() && mom.t >= 0)
      {
        tx = mom.x;
        urgent = true;
      }
      else
        for (Ufo &u : ufos)
          if (u.on && u.t >= 0 && u.y > 0 && u.y < py - 40 && u.y > lowest)
          {
            lowest = u.y;
            tx = u.x;
          }
    }
  }

  // 候補の位置ごとに、そこへ動くあいだ 1.3 秒先まで弾や円盤に当たらないかを見て、危なさを点数にする。
  // 横と縦は動ける速さが違うので、軸ごとに t フレームでどこまで行けるかを見る
  float best = 1e9f, bx = px, by = py;
  float tw = urgent ? 0.07f : 0.03f; // 羊を助けるときと母船のときは、真下に付くのを強く意識する
  for (int j = -2; j <= 2; j++)
    for (int k = -6; k <= 6; k++)
    {
      float cx = constrain(px + k * 6, 10.0f, W - 10.0f), cy = constrain(py + j * 6, H / 2.0f, H - 18.0f);
      float dx = cx - px, dy = cy - py, danger = 0;
      float cost = fabsf(cx - tx) * tw + fabsf(cy - ty) * 0.02f + (abs(k) + abs(j)) * 0.04f;
      for (Shot &s : theirs)
      {
        if (!s.on || fabsf(s.x - cx) > 90 || (s.y > cy + 20 && s.vy >= 0))
          continue;
        for (int t = 2; t <= 40; t += 2)
        {
          float mx = px + constrain(dx, -2.2f * t, 2.2f * t), my = py + constrain(dy, -1.8f * t, 1.8f * t);
          float ex = s.x + s.vx * t - mx, ey = s.y + s.vy * t - my;
          if (ex * ex + ey * ey < 100)
          {
            danger += 20.0f / (1 + t * 0.15f);
            break;
          }
        }
        if (cost + danger >= best)
          break;
      }
      for (Ufo &u : ufos) // 体当たりも先読みして避ける (動きはおおよそ)
      {
        if (!u.on || u.t < 0 || cost + danger >= best)
          continue;
        if (u.type == LANCER && (u.state == 2 || (u.state == 1 && u.a > 20)) && fabsf(cx - u.x) < 9 && cy > u.y) // 光線の下は避ける
          danger += u.state == 2 ? 40 : 25;
        float vy = u.type == SCOUT ? u.vy : u.type == ABDUCTOR ? (u.state == 2 ? u.vy : SPEED) : 0;
        for (int t = 0; t <= 30; t += 10)
        {
          float mx = px + constrain(dx, -2.2f * t, 2.2f * t), my = py + constrain(dy, -1.8f * t, 1.8f * t);
          if (fabsf(u.x - mx) < 16 && fabsf(u.y + vy * t - my) < 18)
          {
            danger += 12.0f / (1 + t * 0.1f);
            break;
          }
        }
      }
      if (cost + danger < best)
      {
        best = cost + danger;
        bx = cx;
        by = cy;
      }
    }
  px += constrain(bx - px, -2.2f, 2.2f);
  py += constrain(by - py, -1.8f, 1.8f);
}

// ボタンを押して撃っているように: 3〜6 回連打しては少し指を休める。撃つ相手がいないときは押さない
void Game::trigger()
{
  if (tapT > 0)
  {
    tapT--;
    return;
  }
  bool target = momAlive() && mom.t >= 0;
  for (Ufo &u : ufos)
    if (u.on && u.t >= 0 && u.y > 0 && u.y < py)
      target = true;
  if (!target)
    return;
  volley();
  if (++taps >= tapsMax)
  {
    taps = 0;
    tapsMax = irand(3, 6);
    tapT = irand(12, 24);
  }
  else
    tapT = irand(4, 7);
}

// 人の操作: スティックで動かし (押し込むとゆっくり)、SW1 を押しているあいだ撃ちつづける。押した瞬間はすぐ撃つ。SW2 で爆弾
void Game::control(const Input &inp)
{
  // スティックは傾けた量で速さが変わる。傾けていなければ上下左右 (遊びの判定) を使う
  float mx = inp.AX, my = inp.AY;
  if (mx == 0 && my == 0)
  {
    mx = (inp.Right ? 1 : 0) - (inp.Left ? 1 : 0);
    my = (inp.Down ? 1 : 0) - (inp.Up ? 1 : 0);
  }
  float len = sqrtf(mx * mx + my * my);
  if (len > 1)
  {
    mx /= len;
    my /= len;
  }
  float sp = inp.Slow ? 1.4f : 2.9f;
  px = constrain(px + mx * sp, 8.0f, W - 8.0f);
  py = constrain(py + my * sp * 0.9f, 40.0f, H - 14.0f);
  if (fireCd > 0)
    fireCd--;
  if (inp.FirePressed && fireCd <= 2)
    fireCd = 0;
  if (inp.Fire && fireCd == 0)
  {
    volley();
    fireCd = 4;
  }
  if (inp.BombPressed)
    dropBomb();
}

// ---- 遊びの切り替え ----
// 世界を作り直す (地形、空のもの、天気)。自機の点や残機はそのまま
void Game::resetWorld()
{
  seed = rnd() * 2654435761u;
  frame = 0;
  rowCount = 0;
  riverOn = laneOn = false;
  villageLen = 0;
  stonePlanned = lochPlanned = stoneFound = nessieFound = false;
  cropPlanned = cropTopNext = cropFound = beastDone = beastFound = false;
  cropLen = 0;
  lakeCool = 20;
  lochTotal = LOCH_ROWS;
  resetRegionState();
  plainLen = lochLen = 0;
  splitSegs(0, COLS);
  for (Prop &p : props)
    p.on = false;
  for (Ufo &u : ufos)
    u.on = false;
  for (Shot &s : mine)
    s.on = false;
  for (Shot &s : theirs)
    s.on = false;
  for (Shot &s : flak)
    s.on = false;
  for (Bomb &b : bombs)
    b.on = false;
  bombCd = 0;
  for (Pop &q : pops)
    q.on = false;
  for (int i = 0; i < NCLOUDS; i++)
  {
    clouds[i].x = frand(0, W);
    clouds[i].y = frand(-H, H);
    clouds[i].r = i < 3 ? irand(8, 14) : 0;
  }
  railRow = railPending = false;
  flock.on = train.on = false;
  for (Puff &q : puffs)
    q.on = false;
  for (DebrisP &d : debris)
    d.on = false;
  for (Drop &d : drops)
  {
    d.x = frand(0, W + 60);
    d.y = frand(0, H);
  }
  birdT = 12 * FPS;
  flashT = thunderT = 0;
  item.on = false;
  mom.on = false;
  scrollY = 0;
  for (int r = ROWS - 1; r >= 0; r--) // 下の行から作る (上が新しい)
  {
    rowId[r] = rowCount++;
    genRow(map[r], rowId[r]);
  }
  railRow = false; // 作りなおしのときにできた線路では、汽車を呼ばない
  tapT = taps = muzzleT = 0;
  tapsMax = 4;
  waveT = 4 * FPS;
  groupN = 0;
  saved = 0;
  extendT = 0;
  newRank = -1;
  for (Spark &q : sparks)
    q.life = 0;
  openAt = 0;
  invalidateRows();
}

void Game::StartDemo()
{
  mode = Mode::Attract;
  demo = true;
  Paused = false;
  gone = outOfLives = false;
  day = 1;
  region = 0;
  rngState ^= millis() * 2654435761u;
  if (rngState == 0)
    rngState = 1;
  resetWorld();
  px = W / 2;
  py = H + 30;
  invT = deadT = enterT = 0;
  lives = 3;
  power = 1;
  score = 0;
  nextExtend = 40000;
}

void Game::NewGame()
{
  mode = Mode::Play;
  demo = false;
  Paused = false;
  gone = outOfLives = false;
  day = 1;
  region = regionOf(day);
  continues = 0;
  resetWorld();
  frame = TITLE_END; // タイトルは飛ばして、旅の絵はがきのあと幕を開けて AREA 1 から
  lives = 2;
  power = 1;
  score = 0;
  nextExtend = 40000;
  deadT = invT = 0;
  fireCd = 0;
  px = W / 2;
  py = H + 20;
  startIntro();
}

// 次の日: 次の地方へ。地形も天気も新しくして、旅の絵はがきのあと朝の AREA 1 から。点と残機と強さは持ちこす
void Game::NextDay()
{
  day++;
  region = regionOf(day);
  resetWorld();
  frame = TITLE_END;
  deadT = 0;
  px = W / 2;
  py = H + 20;
  startIntro();
}

// 画面の下から飛んで入ってくる
void Game::flyIn()
{
  px = W / 2;
  py = H + 20;
  enterT = 40;
  invT = 40 + 60;
}

void Game::Continue()
{
  mode = Mode::Play;
  outOfLives = false;
  lives = 2;
  continues++;
  if (score % 10 < 9) // 点のいちばん下の桁は、コンティニューした回数
    score++;
  flyIn();
  sfx(Sfx::Start);
}

void Game::EndGame()
{
  gone = true;
  mode = Mode::GameOver;
  modeT = 4 * FPS;
  sfx(Sfx::GameOver);
}

int Game::RankOf(int s) const
{
  for (int i = 0; i < 5; i++)
    if (s > Table[i].Score)
      return i;
  return -1;
}

void Game::FinishEntry()
{
  char name[4];
  memcpy(name, entry, 4);
  for (int i = 2; i >= 0 && name[i] == ' '; i--) // うしろの空白を取る
    name[i] = 0;
  if (!name[0])
    strcpy(name, "---");
  memcpy(LastName, entry, 4);
  int i = RankOf(score);
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
  sfx(Sfx::Extend);
}

// ---- 1 フレーム進める (1 秒 30 回) ----
void Game::Step(const Input &inp)
{
  enemyShotSnd = false;
  worldMoved = false;
  switch (mode)
  {
  case Mode::Attract:
    if (inp.StartPressed || inp.FirePressed)
    {
      NewGame();
      break;
    }
    StepWorld(nullptr);
    if (frame >= LOOP_AT)
      StartDemo();
    break;
  case Mode::Play:
    if (inp.PausePressed)
    {
      Paused = !Paused;
      sfx(Sfx::Pause);
    }
    if (Paused)
    {
      if (inp.QuitPressed)
        StartDemo();
      else if (inp.LeftPressed || inp.RightPressed) // 音量 (OFF 〜 3)
      {
        VolumeLevel = constrain(VolumeLevel + (inp.RightPressed ? 1 : -1), 0, 3);
        VolumeChanged = true;
        sfx(Sfx::Select);
      }
      break;
    }
    StepWorld(&inp);
    if (frame == RANK_AT) // 遊んでいるときはランキングを出さずに、幕を閉じて次の日へ
      frame = LOOP_AT - SHUTTER;
    if (frame >= LOOP_AT) // 地図の上を次の地方へ
      startTravel();
    break;
  case Mode::Intro:
    stepIntro(inp);
    break;
  case Mode::Travel:
    stepTravel(inp);
    break;
  case Mode::Ending:
    stepEnding(inp);
    break;
  case Mode::Continue:
    if (inp.FirePressed || inp.StartPressed)
    {
      Continue();
      break;
    }
    if (inp.PausePressed || inp.QuitPressed) // やめる
      modeT = 1;
    if (modeT % FPS == 0 && modeT > 0)
      sfx(Sfx::Tick);
    if (--modeT <= 0)
      EndGame();
    break;
  case Mode::GameOver:
    StepWorld(nullptr);
    if (--modeT <= 0 || (modeT < 3 * FPS && (inp.FirePressed || inp.StartPressed)))
    {
      if (RankOf(score) >= 0)
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
    StepWorld(nullptr);
    StepEntry(inp);
    break;
  case Mode::Ranking:
    StepWorld(nullptr);
    if (--modeT <= 0 || (modeT < 7 * FPS && (inp.FirePressed || inp.StartPressed)))
      StartDemo();
    break;
  }
  if (!worldMoved) // 止まっているあいだは、補わずにそのまま描く
  {
    keepAll();
    scrollAdv = 0;
  }
  UpdateSound();
}

// ---- 動くもののあいだを補う ----
// 描く位置: 前と今のあいだを a (0〜1) で補う。大きく飛んだとき (出てきた、上へ回りこんだ) は今の位置
void Game::lerp(Body &b, float a)
{
  if (fabsf(b.x - b.ox) < 24 && fabsf(b.y - b.oy) < 24)
  {
    b.rx = b.ox + (b.x - b.ox) * a;
    b.ry = b.oy + (b.y - b.oy) * a;
  }
  else
  {
    b.rx = b.x;
    b.ry = b.y;
  }
}
template <typename B, size_t N> static void keepArr(B (&a)[N])
{
  for (B &b : a)
    Game::keep(b);
}
template <typename B, size_t N> static void lerpArr(B (&a)[N], float t)
{
  for (B &b : a)
    Game::lerp(b, t);
}
void Game::keepAll()
{
  keepArr(props);
  keepArr(ufos);
  keepArr(mine);
  keepArr(theirs);
  keepArr(clouds);
  keepArr(pops);
  keepArr(puffs);
  keepArr(debris);
  keepArr(drops);
  keepArr(flak);
  keepArr(bombs);
  keepArr(sparks);
  keep(item);
  keep(flock);
  keep(train);
  keep(mom);
  opx = px;
  opy = py;
}
void Game::lerpAll(float a)
{
  lerpArr(props, a);
  lerpArr(ufos, a);
  lerpArr(mine, a);
  lerpArr(theirs, a);
  lerpArr(clouds, a);
  lerpArr(pops, a);
  lerpArr(puffs, a);
  lerpArr(debris, a);
  lerpArr(drops, a);
  lerpArr(flak, a);
  lerpArr(bombs, a);
  lerpArr(sparks, a);
  lerp(item, a);
  lerp(flock, a);
  lerp(train, a);
  lerp(mom, a);
  bool near = fabsf(px - opx) < 24 && fabsf(py - opy) < 24;
  rpx = near ? opx + (px - opx) * a : px;
  rpy = near ? opy + (py - opy) * a : py;
  rscroll = scrollY - scrollAdv * (1 - a);
}

// 名前の 3 文字: 上下で文字を選び、SW1 で決めて次の文字へ (SW2 で戻る)
void Game::StepEntry(const Input &inp)
{
  if (--modeT <= 0)
  {
    FinishEntry();
    return;
  }
  int dir = 0;
  if (inp.UpPressed)
    dir = 1;
  else if (inp.DownPressed)
    dir = -1;
  else if (inp.Up || inp.Down)
  {
    if (++entryRepeat > 8 && entryRepeat % 3 == 0)
      dir = inp.Up ? 1 : -1;
  }
  if (!inp.Up && !inp.Down)
    entryRepeat = 0;
  if (dir != 0)
  {
    int n = strlen(LETTERS), k = strchr(LETTERS, entry[entryPos]) - LETTERS;
    entry[entryPos] = LETTERS[(k + dir + n) % n];
    sfx(Sfx::Select);
  }
  if (inp.LeftPressed || inp.BackPressed)
  {
    entryPos = max(0, entryPos - 1);
    sfx(Sfx::Select);
  }
  if (inp.RightPressed && entryPos < 2)
  {
    entryPos++;
    sfx(Sfx::Select);
  }
  if (inp.FirePressed)
  {
    sfx(Sfx::Select);
    if (++entryPos > 2)
      FinishEntry();
  }
}

// 鳴りつづける音の大きさを決める
void Game::UpdateSound()
{
  if (!Snd)
    return;
  bool frozen = Paused || mode == Mode::Continue || mode == Mode::Intro || mode == Mode::Travel || mode == Mode::Ending;
  float k = frozen ? 0 : demo ? 0.5f : 1;
  bool plane = !gone && deadT == 0 && py < H + 8 && (mode == Mode::Play || mode == Mode::Attract);
  Snd->Engine = plane ? k : 0;
  // プロペラの音程: 前へ出ると上がり、うしろへ下がると下がり、横へ旋回すると少し上ずる
  if (plane && !frozen)
  {
    float mx = px - opx, my = py - opy;
    float target = 1 - constrain(my, -3.0f, 3.0f) * 0.075f + min(3.0f, fabsf(mx)) * 0.03f;
    enginePitch += (target - enginePitch) * 0.15f;
    Snd->EnginePitch = enginePitch;
    Snd->EngineBank = min(1.0f, fabsf(mx) / 2.9f);
  }
  Snd->Rain = rainK() * k;
  int n = 0;
  bool beam = false;
  for (Ufo &u : ufos)
    if (u.on && u.t >= 0 && u.y > -10 && u.y < H)
    {
      n++;
      if (u.type == ABDUCTOR && u.state == 1)
        beam = true;
    }
  Snd->UfoHum = min(1.0f, n / 3.0f) * k;
  Snd->Beam = beam ? k : 0;
  Snd->MomHum = (momAlive() ? 1 : mom.on && mom.dead ? momAlt() : 0) * k;
  Snd->Birds = (1 - rainK()) * (1 - duskK()) * (1 - 0.6f * sunsetK()) * (momAlive() ? 0.2f : 1) * k;
  if (frozen)
    return;
  if (enemyShotSnd)
    sfx(Sfx::EnemyShot);
  if (thunderT > 0 && --thunderT == 0)
    sfx(Sfx::Thunder);
}

// 世界を 1 フレーム進める。inp が nullptr のときは自動操縦 (デモ) か、自機のいない世界
void Game::StepWorld(const Input *inp)
{
  if (frame >= LOOP_AT || ++frame >= LOOP_AT) // 1 日の終わり。次への切り替えは Step がする
    return;
  keepAll();
  scrollAdv = SPEED;
  worldMoved = true;
  for (Spark &q : sparks)
    if (q.life > 0)
    {
      q.x += q.vx;
      q.y += q.vy;
      q.life--;
    }
  if (extendT > 0)
    extendT--;
  if (muzzleT > 0)
    muzzleT--;
  scrollY += SPEED;
  if (scrollY >= T)
  {
    scrollY -= T;
    newRow();
  }
  for (Prop &p : props)
  {
    if (!p.on)
      continue;
    p.y += SPEED;
    if (!p.held && p.lift > 0)
      p.lift = max(0.0f, p.lift - 0.03f);
    if (p.hop > 0) // 跳ねながら仲間のところへ
    {
      p.hop--;
      if (p.lift == 0)
      {
        p.x += p.vx;
        p.y += p.vy;
      }
    }
    else if ((p.type == SHEEP || p.type == COW) && !p.held) // のんびり歩く
      p.x += 0.06f * sinf((frame + p.v * 97 + (int)p.y * 13) * 0.01f);
    if (p.type == TANK)
      stepTank(p);
    else if (p.type == AAGUN)
      stepAAGun(p);
    if (p.type == DOG) // 群れのまわりを 8 の字に走りまわる
    {
      p.ay += SPEED;
      p.t++;
      float nx = p.ax + 17 * cosf(p.t * 0.045f), ny = p.ay + 8 * sinf(p.t * 0.09f);
      p.v = nx >= p.x ? 1 : 0;
      p.x = nx;
      p.y = ny;
      if (irand(0, 170) == 0 && p.y > 0 && p.y < H)
        sfx(Sfx::Woof, 0.6f);
    }
    else if (p.type == BEAST) // ボドミンの野獣: 駆けてきて、途中で座ってこちらを見て、また走り去る
    {
      p.t++;
      if (p.t < 60 || p.t >= 130)
        p.x += (p.v != 0 ? 1 : -1) * 1.6f;
      if (p.t == 60 && p.y > 0 && p.y < H)
        sfx(Sfx::Growl);
    }
    else if (p.type == CROPCIRCLE && p.y > -20) // 画面の上の端まで来てから描きはじめる
      p.t++;
    else if (p.type == NESSIE) // 湖をゆっくり回りながら、ときどき水から出て、また沈む
    {
      p.ay += SPEED;
      p.t++;
      float ang = p.t * 0.009f, nx = p.ax + 30 * cosf(ang), ny = p.ay + 26 * sinf(ang);
      p.v = nx >= p.x ? 1 : 0;
      p.x = nx;
      p.y = ny;
      int ph_ = p.t % 560;
      p.lift = ph_ < 50 ? ph_ / 50.0f : ph_ < 360 ? 1 : ph_ < 420 ? 1 - (ph_ - 360) / 60.0f : 0;
      if (ph_ == 1 && p.y > 0 && p.y < H)
        sfx(Sfx::Splash);
    }
    else if (p.type == RABBIT && p.y > -10) // 野うさぎ: ときどきぴょんと跳ねる
    {
      int ph3 = ++p.t % 70;
      if (ph3 < 12)
      {
        if (ph3 == 0)
        {
          p.vx = frand(-0.5f, 0.5f);
          p.vy = frand(-0.3f, 0.3f);
          p.v = p.vx >= 0 ? 1 : 0;
        }
        p.x += p.vx;
        p.y += p.vy;
        p.ay = sinf(ph3 / 12.0f * PI) * 4;
      }
      else
        p.ay = 0;
    }
    else if (p.type == BOAT && p.y > -10) // ボート: 湖をゆっくり進み、岸に近づくと向きを変える
    {
      p.x += p.vx;
      if (!isWater(tileTypeAt(p.x + (p.vx > 0 ? 7 : -7), p.y)))
        p.vx = -p.vx;
    }
    else if (p.type == DUCK)
    {
      if (p.y > -10) // 画面の上の端まで来てから泳ぎだす
        p.x += p.vx;
      if (p.v == 0 && irand(0, 220) == 0 && p.y > 0 && p.y < H)
        sfx(Sfx::Quack, 0.6f);
    }
    if (p.y > H + (p.type == STONEHENGE ? 40 : 16) || p.x < -30 || p.x > W + 30)
      p.on = false;
  }
  if (!beastDone && (region == 2 || region == 3) && frame >= BEAST_AT && !gone) // ヨークシャーはブラック・シャック、コーンウォールはボドミンの野獣
  {
    beastDone = true;
    int dir = irand(0, 1) != 0 ? 1 : -1;
    Prop *b = addProp(dir > 0 ? -12 : W + 12, frand(50, 110), BEAST, dir > 0 ? 1 : 0);
    if (b)
    {
      b->ox = b->x;
      b->kind = region == 2 ? 1 : 0;
    }
  }
  for (Prop &p : props) // 名所から円盤が湧いてくる: ストーンヘンジは真ん中が光って地面から、湖は水の下から浮上
  {
    if (!p.on || (p.type != STONEHENGE && p.type != NESSIE))
      continue;
    if (p.type == STONEHENGE)
      p.t++;
    bool canSpawn = active() && !momAlive() && p.y > 30 && p.y < H - 130 && p.spawned < (p.type == STONEHENGE ? 6 : 5);
    int period = p.type == STONEHENGE ? 80 : 95;
    if (canSpawn && (p.type == STONEHENGE ? p.t : p.t + 30) % period == period / 2)
    {
      float sx = p.type == STONEHENGE ? p.x : p.ax + frand(-26, 26), sy = p.type == STONEHENGE ? p.y - 2 : p.ay + frand(-30, 30);
      if (p.type == NESSIE && !isWater(tileTypeAt(sx, sy)))
        continue;
      Ufo *u = addUfo(SCOUT, sx, sy, 0);
      if (u)
      {
        u->rise = 0;
        u->riseKind = p.type == STONEHENGE ? 0 : 1;
        u->vy = frand(0.8f, 1.1f);
        u->home = sx;
        p.spawned++;
        sfx(p.type == STONEHENGE ? Sfx::Emerge : Sfx::Splash, 0.8f);
      }
    }
  }
  if (!gone && deadT == 0 && playing()) // 名所の真上を飛ぶと、見つけたボーナス (1 日 1 回ずつ)
    for (Prop &p : props)
    {
      if (!p.on)
        continue;
      if (p.type == STONEHENGE && !stoneFound && fabsf(px - p.x) < 14 && fabsf(py - p.y) < 14)
      {
        stoneFound = true;
        score += 3000;
        addPop(p.x, p.y - 24, p.kind == 1 ? "CASTLERIGG" : p.kind == 2 ? "ST MICHAEL'S MOUNT" : p.kind == 3 ? "EILEAN DONAN" : "STONEHENGE", GOLD);
        sfx(Sfx::Discover);
      }
      else if (p.type == BEAST && !beastFound && fabsf(px - p.x) < 16 && fabsf(py - p.y) < 16)
      {
        beastFound = true;
        score += 4000;
        addPop(p.x, p.y - 16, p.kind == 1 ? "BLACK SHUCK" : "BODMIN BEAST", p.kind == 1 ? 0xF06040u : 0xE8D040u);
        sfx(Sfx::Discover);
      }
      else if (((p.type == ABBEY && !abbeyFound) || (p.type == VIADUCT && !viaductFound) || (p.type == WHITEHORSE && !horseFound)) &&
               fabsf(py - p.y) < 16 && (p.type == VIADUCT || fabsf(px - p.x) < 20))
      {
        if (p.type == ABBEY)
          abbeyFound = true;
        else if (p.type == VIADUCT)
          viaductFound = true;
        else
          horseFound = true;
        score += 3000;
        addPop(p.type == VIADUCT ? px : p.x, p.y - 22, p.type == ABBEY ? "WHITBY ABBEY" : p.type == VIADUCT ? "RIBBLEHEAD" : "WHITE HORSE", GOLD);
        sfx(Sfx::Discover);
      }
      else if (p.type == CROPCIRCLE && !cropFound && p.t > 50 && fabsf(px - p.x) < 20 && fabsf(py - p.y) < 20)
      {
        cropFound = true;
        score += 3000;
        addPop(p.x, p.y - 26, "CROP CIRCLE", GOLD);
        sfx(Sfx::Discover);
      }
      else if (p.type == NESSIE && !nessieFound && p.lift > 0.6f && fabsf(px - p.x) < 18 && fabsf(py - p.y) < 22)
      {
        nessieFound = true;
        score += 5000;
        addPop(p.x, p.y - 20, p.kind == 1 ? "BOWNESSIE!" : p.kind == 2 ? "MORGAWR!" : "NESSIE!", 0x9AF0B0);
        sfx(Sfx::Discover);
      }
    }
  for (Prop &d : props) // 牧羊犬や野獣が近づくと、羊は逃げる (野獣からは遠くから、速く)
    if (d.on && (d.type == DOG || d.type == BEAST))
      for (Prop &sh : props)
        if (sh.on && sh.type == SHEEP && !sh.held && sh.hop == 0)
        {
          float dx = sh.x - d.x, dy = sh.y - d.y, d2 = dx * dx + dy * dy;
          float far = d.type == BEAST ? 26 : 12, push = d.type == BEAST ? 0.7f : 0.35f;
          if (d2 < far * far && d2 > 0.01f)
          {
            float dist = sqrtf(d2);
            sh.x += dx / dist * push;
            sh.y += dy / dist * push * 0.7f;
            sh.v = dx > 0 ? 1 : 0;
          }
        }
  float rk = rainK();
  for (int i = 0; i < NCLOUDS; i++) // 雲は地面より速く流れる。雨のときは雲が増え、大きくなる (r = 0 は出ていない雲)
  {
    Cloud &c = clouds[i];
    if ((c.y += 1.3f + rk * 0.5f) > H + 30)
    {
      c.x = frand(10, W - 10);
      c.y = -30 - frand(0, rk > 0.3f ? 60 : 200);
      c.r = i < 3 || rk > 0.3f ? irand(8, 14) + (int)(rk * 6) : 0;
    }
  }
  if (flashT > 0)
    flashT--;
  if (rk > 0.8f && irand(0, 140) == 0) // 稲光。少し遅れて雷が鳴る
  {
    flashT = 3;
    thunderT = irand(8, 40);
  }
  if (rk > 0)
    for (Drop &d : drops)
    {
      d.x -= region == 4 ? 0.5f + 0.4f * sinf((frame + d.y) * 0.05f) : 1.5f; // 高地は雪がふわふわ
      if ((d.y += region == 4 ? 1.6f : 9) > H || d.x < -4)
      {
        d.x = frand(0, W + 60);
        d.y = frand(-20, 0);
      }
    }

  // 渡り鳥: ときどき群れが横切る
  if (!flock.on && frame > TITLE_END && --birdT <= 0)
  {
    int dir = irand(0, 1) != 0 ? 1 : -1;
    flock.x = dir > 0 ? -30.0f : W + 30.0f;
    flock.y = frand(40, H - 120);
    flock.vx = dir * frand(0.8f, 1.1f);
    flock.vy = frand(-0.2f, 0.4f);
    flock.n = irand(5, 9);
    flock.t = 0;
    flock.on = true;
    birdT = irand(15 * FPS, 25 * FPS);
  }
  if (flock.on)
  {
    flock.x += flock.vx;
    flock.y += flock.vy;
    flock.t++;
    if (flock.x < -90 || flock.x > W + 90)
      flock.on = false;
  }

  if (railPending && (railY += SPEED) >= -2)
  {
    railPending = false;
    int dir = irand(0, 1) != 0 ? 1 : -1;
    train.x = train.ox = dir > 0 ? -4.0f : W + 4.0f;
    train.y = train.oy = railY;
    train.dir = dir;
    train.cars = irand(2, 4);
    train.on = true;
    sfx(Sfx::Whistle);
  }
  // 蒸気機関車
  if (train.on)
  {
    train.y += SPEED;
    train.x += train.dir * 1.1f;
    if (frame % 5 == 0)
      addPuff(train.x - train.dir * 4, train.y, false, -train.dir * 0.3f);
    if (train.y > H + 10 || (train.dir > 0 ? train.x > W + 100 : train.x < -100))
      train.on = false;
  }
  for (Puff &q : puffs)
    if (q.on)
    {
      q.y += SPEED - (q.smoke ? 0.15f : 0);
      q.x += q.vx;
      q.r += q.smoke ? 0.1f : 0.12f;
      if (++q.t > q.life)
        q.on = false;
    }
  stepDebris();
  for (Prop &p : props) // 残骸から煙
    if (p.on && p.type == WRECK && frame % 5 == 0)
      addPuff(p.x + frand(-14, 14), p.y + frand(-8, 8), true);
  for (Pop &q : pops)
    if (q.on)
    {
      q.y -= 0.4f;
      if (++q.t > 50)
        q.on = false;
    }
  if (frame == BONUS_AT && !gone)
    score += saved * 1000;
  if (frame == RANK_AT)
    newRank = -1;

  // 円盤: ときどき来る。母船のあいだは来ない
  if (!mom.on && frame == MOM_AT && !gone)
    startMom();
  if (mom.on)
    stepMom();
  if (active() && !momAlive() && --waveT <= 0)
  {
    spawnWave();
    waveT = (int)((area() == 1 ? irand(5 * FPS, 9 * FPS) : irand(4 * FPS, 7 * FPS)) * waveK());
  }
  for (Ufo &u : ufos)
    if (u.on)
      stepUfo(u);

  bool alive = !gone && deadT == 0 && enterT == 0 && playing();
  if (item.on)
  {
    item.t++;
    item.y += 0.6f;
    item.x += sinf(item.t * 0.08f) * 0.5f;
    if (item.y > H + 10)
      item.on = false;
    else if (alive && fabsf(item.x - px) < 11 && fabsf(item.y - py) < 11)
    {
      item.on = false;
      if (power < 3)
        power++;
      else
        score += 2000;
      burst(item.x, item.y, GOLD, 8, 1.2f, 12);
      sfx(Sfx::Power);
    }
  }

  for (Shot &s : theirs)
    if (s.on)
    {
      if (!stepEnemyShot(s))
        s.on = false;
      else if (s.kind != 4 && alive && invT == 0 && fabsf(s.x - px) < shotHitX(s.kind) && fabsf(s.y - py) < shotHitY(s.kind))
      {
        s.on = false;
        killPlayer();
        alive = false;
      }
    }
  for (Ufo &u : ufos) // 光線円盤の光線
    if (alive && invT == 0 && beamHits(u))
    {
      killPlayer();
      alive = false;
    }
  stepFlak();
  stepBombs();
  for (Shot &s : mine)
  {
    if (!s.on)
      continue;
    s.x += s.vx;
    s.y += s.vy;
    if (s.y < -6 || s.x < -4 || s.x > W + 4)
    {
      s.on = false;
      continue;
    }
    for (Ufo &u : ufos)
      if (u.on && u.t >= 0 && u.rise >= 0.7f && fabsf(s.x - u.x) < UFO_R[u.type] && fabsf(s.y - u.y) < UFO_R[u.type])
      {
        s.on = false;
        damageUfo(u, 1);
        break;
      }
    if (s.on && momAlive() && mom.t >= 0)
    {
      float dx = s.x - mom.x, dy = s.y - mom.y;
      if (dx * dx + dy * dy < 34 * 34)
      {
        s.on = false;
        damageMom(1);
        if ((mom.hp & 3) == 0)
          burst(s.x, s.y, GOLD, 1, 0.8f, 6);
      }
    }
  }
  if (!gone && score >= nextExtend) // 4 万点と、そのあと 10 万点ごとに 1 機ふえる
  {
    nextExtend += 100000;
    lives++;
    extendT = 60;
    sfx(Sfx::Extend);
  }

  // 自機
  if (gone)
    return;
  if (deadT > 0)
  {
    if (--deadT == 0)
    {
      if (outOfLives)
      {
        mode = Mode::Continue;
        modeT = 10 * FPS - 1;
        return;
      }
      if (demo)
      {
        px = W / 2;
        py = PY0;
        invT = 100;
      }
      else
        flyIn();
    }
    return;
  }
  if (invT > 0)
    invT--;
  if (frame < TITLE_END)
  {
    if (frame > 110)
      py = max((float)PY0, py - 1.0f);
    return;
  }
  if (frame >= CLEAR_AT)
  {
    // 真ん中に寄って、前へ飛び去る
    px += constrain(W / 2 - px, -1.0f, 1.0f);
    if (frame > CLEAR_AT + 45)
      py -= min(5.0f, (frame - CLEAR_AT - 45) * 0.12f);
    return;
  }
  if (enterT > 0)
  {
    enterT--;
    py = max((float)PY0, py - 3.0f);
    if (py <= PY0)
      enterT = 0;
    return;
  }
  if (!inp)
  {
    think();
    trigger();
    autoBomb();
  }
  else
    control(*inp);
  for (Ufo &u : ufos) // 体当たり
    if (u.on && u.t >= 0 && u.rise >= 1 && invT == 0 && fabsf(u.x - px) < 8 && fabsf(u.y - py) < 8)
    {
      damageUfo(u, 10);
      killPlayer();
      break;
    }
}
