// 地方ごとの中身 (Windows 版の GameRegions.cs): DAY 3 ヨークシャー、DAY 4 コーンウォール、DAY 5 スコットランド高地、DAY 1 の白馬、結末。
// ・名所を置く: 時刻 (朝 / 中ごろ / 夕方) ごとに、地方に合った名所の地形を作りはじめ、その真ん中の行に名所を置く
// ・地形: コーンウォールは左に海 (浜と崖)、高地は湖が点々。ヨークシャーの谷には高架橋
// ・天気: ヨークシャーは秋 (落ち葉)、コーンウォールは晴れ、高地は夕方に雪とオーロラ
#include "game.h"

void Game::resetRegionState()
{
  stonePending = abbeyPending = viaductPending = viaductRowMade = forceTrain = mountPending = castlePending = horsePending = nessiePending =
      morgawrPending = false;
  abbeyFound = viaductFound = horseFound = false;
  bayLen = 0;
  lightCool = 10;
}

// 川と小道を避けた、真ん中寄りの列
int Game::clearCol(int lo, int hi, int lim)
{
  int best = COLS / 2, bestD = -1;
  for (int c = lo; c <= hi; c++)
  {
    int dd = min(riverOn ? (int)fabsf(c - riverC) : 99, laneOn ? abs(c - laneCol) : 99);
    dd = min(dd, lim) * 4 - abs(c - COLS / 2);
    if (dd > bestD)
    {
      bestD = dd;
      best = c;
    }
  }
  return best;
}

// ---- 名所の地形を作りはじめる (新しい行を作る前に) ----
void Game::planLandmarks()
{
  int f = frame + LEAD;
  if (!cropPlanned && f >= CROP_AT) // 朝の名所
  {
    cropPlanned = true;
    if (region == 0) // ミステリーサークルの麦畑
    {
      cropLen = CROP_ROWS;
      cropC = clearCol(7, COLS - 8, 9);
    }
    else if (region == 2) // 丘の上のウィットビー修道院
    {
      plainLen = 8;
      stoneCol = clearCol(6, COLS - 7, 8);
      abbeyPending = true;
    }
  }
  if (!stonePlanned && f >= STONE_AT) // 中ごろの名所
  {
    stonePlanned = true;
    if (region <= 2) // 草原にストーンヘンジ / キャッスルリッグ、ヨークシャーは谷に高架橋
    {
      plainLen = PLAIN_ROWS;
      stoneCol = clearCol(5, COLS - 6, 8);
      if (region == 2)
        viaductPending = true;
      else
        stonePending = true;
    }
    else if (region == 3) // 入り江に浮かぶセント・マイケルズ・マウント
    {
      bayTotal = bayLen = BAY_ROWS;
      bayW = 13;
      mountPending = true;
    }
    else // 湖の小島のアイリーン・ドナン城
    {
      lochTotal = lochLen = 18;
      lochR = 5.5f;
      lochC = frand(lochR + 3, COLS - lochR - 4);
      laneOn = false;
      castlePending = true;
    }
  }
  if (!lochPlanned && f >= LOCH_AT) // 夕方の名所
  {
    lochPlanned = true;
    if (region == 0) // 白亜の丘の白馬
    {
      plainLen = PLAIN_ROWS;
      stoneCol = clearCol(7, COLS - 8, 8);
      horsePending = true;
    }
    else if (region == 1 || region == 4) // 大きな湖と、ボウネッシー / ネッシー
    {
      lochTotal = lochLen = LOCH_ROWS;
      lochR = 7.0f;
      lochC = frand(lochR + 2, COLS - lochR - 3);
      laneOn = false;
      nessiePending = true;
    }
    else if (region == 3) // 入り江と、海の大蛇モーガー
    {
      bayTotal = bayLen = BAY_ROWS + 4;
      bayW = 14;
      morgawrPending = true;
    }
  }
}

// ---- 名所を置く (新しい行を作ったあと、その行の y に) ----
void Game::placeLandmarks(float y)
{
  if (cropPlanned && region == 0 && cropLen == CROP_ROWS / 2 && !hasProp(CROPCIRCLE))
    addProp(cropC * T + 4, y, CROPCIRCLE);
  if (abbeyPending && plainLen == 3)
  {
    abbeyPending = false;
    addProp(stoneCol * T + 4, y, ABBEY);
  }
  if (stonePending && plainLen == PLAIN_ROWS / 2 - 1)
  {
    stonePending = false;
    Prop *sh = addProp(stoneCol * T + 4, y, STONEHENGE);
    if (sh)
      sh->kind = region == 1 ? 1 : 0;
  }
  if (viaductPending && viaductRowMade)
  {
    viaductPending = false;
    addProp(W / 2.0f, y, VIADUCT);
  }
  if (horsePending && plainLen == PLAIN_ROWS / 2 - 1)
  {
    horsePending = false;
    addProp(stoneCol * T + 4, y, WHITEHORSE);
  }
  if (mountPending && bayLen == BAY_ROWS / 2)
  {
    mountPending = false;
    Prop *sh = addProp(bayW * T * 0.45f, y, STONEHENGE);
    if (sh)
      sh->kind = 2;
  }
  if (castlePending && lochLen == 9)
  {
    castlePending = false;
    Prop *sh = addProp(lochC * T, y, STONEHENGE);
    if (sh)
      sh->kind = 3;
  }
  if (nessiePending && lochLen == LOCH_ROWS / 2)
  {
    nessiePending = false;
    Prop *n = addProp(lochC * T, y, NESSIE);
    if (n)
    {
      n->ax = lochC * T;
      n->kind = region == 1 ? 1 : 0;
    }
  }
  if (morgawrPending && bayLen == (BAY_ROWS + 4) / 2)
  {
    morgawrPending = false;
    float x = bayW * T * 0.42f;
    Prop *n = addProp(x, y, NESSIE);
    if (n)
    {
      n->ax = x;
      n->kind = 2;
    }
  }
}

// ---- 地方の地形 (行を作るいちばん最後に) ----
void Game::regionTerrain(Tile *row, int n)
{
  if (region == 2 && viaductPending && !viaductRowMade && plainLen == PLAIN_ROWS / 2 - 1) // 谷を渡る高架橋 (石の上の線路)
  {
    for (int c = 0; c < COLS; c++)
      row[c] = {RAILBRIDGE, 0, 0};
    railRow = true;
    forceTrain = true;
    viaductRowMade = true;
  }
  if (region == 3) // コーンウォール: 左は海。岸は浜か崖。入り江のあいだは海が広がる
  {
    float bw = 3.2f + 3.2f * noise(n * 0.07f, 11.3f) - fellK() * 1.2f;
    if (bayLen > 0)
    {
      float tt = (bayTotal - bayLen + 0.5f) / bayTotal;
      bw += (bayW - bw) * sqrtf(max(0.0f, 1 - (2 * tt - 1) * (2 * tt - 1)));
      bayLen--;
    }
    int cl = max(1, (int)bw);
    for (int c = 0; c < cl && c < COLS; c++)
      row[c] = {SEA, 0, 0};
    if (cl < COLS)
      row[cl] = {noise(cl * 0.5f, n * 0.2f) > 0.52f ? CLIFF : SAND, 0, 0};
  }
}

// ---- 地方の置き物 (行を作ったあと) ----
void Game::regionProps(float y)
{
  for (int c = 0; c < COLS; c++)
  {
    Tile t = map[0][c];
    float x = c * T + 4;
    if (region == 2 && t.type == FIELD && (t.flags & STONEWALL) != 0 && frand(0, 1) < 0.006f) // 谷の石の納屋
    {
      Prop *b = addProp(x, y, BARN);
      if (b)
        b->kind = 1;
    }
    else if (region == 3 && t.type == CLIFF && --lightCool <= 0) // 崖の上の灯台
    {
      addProp(x + 2, y, LIGHTHOUSE);
      lightCool = irand(60, 110);
    }
    else if (region == 3 && t.type == MOOR && t.v <= 2 && frand(0, 1) < 0.008f) // 廃坑の機関小屋
      addProp(x, y, MINE);
    else if (region == 4 && t.type == MOOR && t.v == 1 && frand(0, 1) < 0.01f) // 荒れ地のハイランド牛
      for (int i = 0, k = irand(2, 3); i < k; i++)
        addProp(x + frand(-8, 8), y + frand(-6, 6), COW, irand(0, 1));
  }
  if (region == 3) // 海の漁船
  {
    int sea = 0;
    while (sea < COLS && map[0][sea].type == SEA)
      sea++;
    if (sea >= 2 && irand(0, 12) == 0)
    {
      Prop *b = addProp(frand(4, sea * T - 4), y + 2, BOAT, 2);
      if (b)
        b->vx = (irand(0, 1) != 0 ? 1 : -1) * frand(0.05f, 0.1f);
    }
  }
}

// ---- 地方の天気の飾り: 秋の落ち葉、高地のオーロラ (色掛けのあとに描く) ----
void Game::drawSeasonFx(Gfx &g, int skyBottom)
{
  if (region == 2) // 落ち葉: ひらひらと横へ流れる
    for (int i = 0; i < 26; i++)
    {
      uint32_t h = (uint32_t)(i * 2654435761u);
      float fx2 = fmodf((h % 997) / 997.0f * (SW + 40) + frame * (0.5f + ((h >> 10) & 7) * 0.08f), SW + 40) - 20;
      float fy2 = fmodf(((h >> 4) % 991) / 991.0f * SH + frame * (0.8f + ((h >> 14) & 3) * 0.2f), SH);
      fx2 += sinf(frame * 0.05f + i) * 6;
      uint32_t c = (h & 3) == 0 ? 0xD8583Au : (h & 3) == 1 ? 0xE8A040u : (h & 3) == 2 ? 0xC8782Au : 0xF0C850u;
      if (((frame / 6) + i) % 2 == 0)
        g.fillRect((int)fx2, (int)fy2, 2, 1, c);
      else
        g.fillRect((int)fx2, (int)fy2, 1, 2, c);
    }
  if (region == 4) // オーロラ: 夕暮れの空に、緑と紫のカーテン
  {
    float k = duskK();
    int h = (int)(skyBottom * 0.55f);
    if (k <= 0 || !g.visible(0, h))
      return;
    for (int x = 0; x < SW; x++)
    {
      float c1 = 20 + 14 * sinf(x * 0.025f + frame * 0.01f) + 6 * sinf(x * 0.07f - frame * 0.02f);
      float band = 0.5f + 0.5f * sinf(x * 0.11f + frame * 0.03f);
      for (int y = max(0, g.top); y < min(h, g.top + STRIP); y++)
      {
        float t = (y - c1) / h;
        if (t < 0 || t > 1)
          continue;
        int a = (int)(170 * min(1.0f, k * 1.6f) * band * (1 - t) * min(1.0f, t * 6));
        g.blendPixel(x, y, t < 0.5f ? 0x6AFFA8u : 0xB878F0u, a);
      }
    }
  }
}

// ---- 描く: 地形のマス ----
uint32_t Game::regionTilePixel(int type, int r, int c, int i, int j, uint32_t h, float m)
{
  switch (type)
  {
  case SEA:
  {
    uint32_t col = mix(0x2A5E8E, 0x3A74A6, m);
    if ((h % 37) == 0)
      col = 0x6A9ACA;
    if (!isWater(typeAt(r, c + 1)) && typeAt(r, c + 1) >= 0 && i >= TS - 3)
      col = i == TS - 1 ? 0xF0F8FCu : 0xB8D8EAu; // 岸に寄せる白い波
    return col;
  }
  case SAND:
  {
    uint32_t col = (h & 7) == 0 ? 0xD4C090u : (h & 7) == 1 ? 0xF0E4C0u : 0xE4D4A4u;
    if (typeAt(r, c - 1) == SEA && i < 3)
      col = mix(0xC4B488, col, i / 3.0f); // ぬれた砂
    return col;
  }
  default: // 崖: 海の側は岩の面の影、陸の側は草
  {
    uint32_t col = (h & 7) == 0 ? 0x7A766Au : (h & 7) == 1 ? 0x9E9A8Eu : 0x8A867Au;
    if (i < 3)
      col = mulc(col, 0.6f + i * 0.12f);
    if (i > 8)
      col = (h & 1) != 0 ? 0x6E9A48u : 0x5E8A3Cu;
    return col;
  }
  }
}

// ---- 描く: 置き物 (真上から) ----
// 高架橋: 石のアーチの影が下 (南) の地面に落ちる。上は線路 (マスで描く)。橋の両側に欄干
void Game::viaductShadow(Gfx &g, int y)
{
  for (int yy = y + 6; yy < y + 22; yy++)
    for (int x = 0; x < SW; x++)
    {
      int ph3 = (x + 6) % 26, d = yy - (y + 6);
      bool pier = ph3 < 7;
      float ax = (ph3 - 16.5f) / 9.5f;
      bool arch = !pier && d > 3 + (int)(6 * (1 - min(1.0f, ax * ax)));
      if (!arch)
        g.drawPixel(x + d / 2, yy, 0);
    }
}
void Game::drawViaduct(Gfx &g, int y)
{
  g.fillRect(0, y - 7, SW, 2, 0xB8B0A0);
  g.drawFastHLine(0, y - 7, SW, 0xD8D0C0);
  g.fillRect(0, y + 5, SW, 2, 0x8A8476);
  g.drawFastHLine(0, y + 6, SW, 0x5E5A50);
}

// ウィットビー修道院の廃墟: 屋根のない石の壁、東の高い切妻壁と尖ったアーチの窓、柱の跡、墓地
void Game::drawAbbey(Gfx &g, int x, int y)
{
  int x0 = x - 24, y0 = y - 12;
  for (int k = 0; k < 2; k++) // 長い壁 2 本 (ところどころ崩れている)
    for (int i = 0; i < 48; i++)
      if ((i + k * 5) % 13 != 0)
      {
        g.fillRect(x0 + i, y0 + k * 22, 1, 3, (i & 1) == 0 ? 0xA8A294u : 0x968F82u);
        g.drawPixel(x0 + i, y0 + k * 22, 0xC8C2B4);
      }
  g.fillRect(x0, y0, 3, 25, 0x968F82);           // 西の壁
  g.fillRect(x0 + 45, y0 - 2, 5, 29, 0xB0AA9C); // 東の切妻壁 (高い)
  g.drawFastVLine(x0 + 45, y0 - 2, 29, 0xC8C2B4);
  for (int w = 0; w < 3; w++) // 尖ったアーチの窓
    g.fillRect(x0 + 46, y0 + 3 + w * 7, 3, 4, 0x6A8A5A);
  for (int i = 0; i < 5; i++) // 柱の跡
  {
    g.fillRect(x0 + 8 + i * 8, y0 + 7, 2, 2, 0x8A8476);
    g.fillRect(x0 + 8 + i * 8, y0 + 15, 2, 2, 0x8A8476);
  }
  for (int k = 0; k < 7; k++) // 墓地
  {
    int gx = x0 - 6 + (int)(ph(k, 4) % 8), gy = y0 + 30 + k * 2 - (k % 3) * 3;
    g.fillRect(gx + k * 8, gy, 2, 3, 0xB8B8B0);
    g.drawPixel(gx + k * 8 + 2, gy + 2, 0x4A4A40);
  }
}

// 灯台: 白い塔に赤い帯、ガラスの灯室。夜は光が回る
void Game::drawLighthouse(Gfx &g, int x, int y)
{
  g.fillCircle(x, y, 6, 0xF4F2EA);
  g.drawCircle(x, y, 6, 0xA8A8A0);
  g.drawCircle(x, y, 4, POPPY);
  g.fillCircle(x, y, 3, 0x2A2A2A);
  g.fillCircle(x, y, 2, 0xF8E890);
  g.drawPixel(x - 1, y - 1, 0xFFFFFF);
}

// 廃坑の機関小屋: 屋根のない石の小屋と、角に高い煙突
void Game::drawMine(Gfx &g, int x, int y)
{
  g.fillRect(x - 7, y - 5, 14, 10, 0x8A8476);
  g.fillRect(x - 5, y - 3, 10, 6, 0x6E7A4A);
  g.drawFastHLine(x - 7, y - 5, 14, 0xB0AA9C);
  g.drawFastVLine(x - 7, y - 5, 10, 0xB0AA9C);
  g.fillCircle(x + 6, y - 6, 3, 0x7A746A);
  g.fillCircle(x + 6, y - 6, 1, 0x1A1A1A);
}

// 白亜の丘の白馬 (ウェストベリー風): 緑の丘に白い線で描いた馬 (地面の模様)
void Game::drawWhiteHorse(Gfx &g, int x, int y)
{
  static const int lx0[] = {-15, -7, 10, 16}, lx1[] = {-18, -7, 11, 20};
  for (int pass = 0; pass < 2; pass++)
  {
    uint32_t c = pass == 0 ? 0xB8BEA0u : 0xF4F4EAu;
    int o = pass == 0 ? 1 : 0;
    g.fillEllipse(x + o, y + o, 22, 8, c);
    for (int t = 0; t <= 10; t++) // 首
      g.fillCircle(x + 16 + t + o, y - 3 - t + o, 3, c);
    g.fillEllipse(x + 30 + o, y - 14 + o, 6, 3, c);
    for (int k = 0; k < 4; k++) // 脚
    {
      g.drawLine(x + lx0[k] + o, y + 5 + o, x + lx1[k] + o, y + 20 + o, c);
      g.drawLine(x + lx0[k] + 1 + o, y + 5 + o, x + lx1[k] + 1 + o, y + 20 + o, c);
    }
    g.drawLine(x - 21 + o, y - 2 + o, x - 29 + o, y + 9 + o, c); // しっぽ
    g.drawLine(x - 22 + o, y - 2 + o, x - 30 + o, y + 9 + o, c);
  }
  g.drawPixel(x + 32, y - 15, 0x6E9A48); // 目
}

// セント・マイケルズ・マウント: 入り江の岩の小島、緑の丘と木、てっぺんに城。干潮の道が岸へ
void Game::drawMount(Gfx &g, int x, int y, Prop &p, bool ground, bool building)
{
  if (ground)
  {
    for (int i = 0; i < 70; i++) // 干潮の道 (岸へ)
      g.blendPixel(x + 24 + i, y + 6 + (i / 12) % 2, 0xD8C898, 170);
    g.fillEllipse(x + 2, y + 3, 30, 21, 0x5E5A50);
    g.fillEllipse(x, y, 29, 20, 0x8A867A);
    g.fillEllipse(x - 2, y - 2, 23, 15, 0x6E9A48);
    g.fillEllipse(x - 5, y - 5, 12, 8, 0x86AE58);
    for (int k = 0; k < 9; k++)
    {
      uint32_t h = ph(k, 7);
      g.fillCircle(x - 18 + (int)(h % 30), y + 2 + (int)((h >> 5) % 10), 2, 0x3E6A30);
    }
  }
  if (!building)
    return;
  g.fillRect(x - 9, y - 9, 16, 10, 0x9A968A); // 城
  g.fillRect(x - 9, y - 9, 16, 4, 0xB8B4A8);
  g.fillRect(x + 4, y - 13, 5, 6, 0xA8A498);
  g.fillRect(x - 11, y - 12, 4, 5, 0xA8A498);
  g.drawFastVLine(x + 6, y - 18, 5, 0x5A4030);
  g.fillRect(x + 7, y - 18, 3, 2, POPPY);
  int ph2 = p.t % 80;
  float glowK = p.spawned < 6 && ph2 >= 18 && ph2 <= 70 ? 1 - abs(ph2 - 40) / 30.0f : 0;
  if (glowK > 0 && !ground)
    for (int j = -8; j <= 8; j++)
      for (int i = -14; i <= 14; i++)
        if (i * i / 196.0f + j * j / 64.0f < 1)
          g.blendPixel(x + i, y - 4 + j, 0xB8FFD0, (int)(200 * glowK));
}

// アイリーン・ドナン城: 湖の小島に四角い石の天守と胸壁、岸へ続くアーチの石橋
void Game::drawCastleIsland(Gfx &g, int x, int y, Prop &p, bool ground, bool building)
{
  if (ground)
  {
    for (int i = 0; i < 46; i++) // 石橋
    {
      g.fillRect(x + 12 + i, y + 2, 1, 4, (i % 7) < 2 ? 0x6A665Au : 0xA8A294u);
      g.drawPixel(x + 12 + i, y + 2, 0xC8C2B4);
    }
    g.fillEllipse(x + 1, y + 2, 17, 12, 0x4E4A42);
    g.fillEllipse(x, y, 16, 11, 0x7A766A);
    g.fillEllipse(x - 2, y - 2, 11, 7, 0x6E8A50);
  }
  if (!building)
    return;
  g.fillRect(x - 8, y - 9, 16, 15, 0x8E8A7E); // 天守
  g.fillRect(x - 6, y - 7, 12, 11, 0x6E6A60);
  for (int i = 0; i < 16; i += 2) // 胸壁
  {
    g.drawPixel(x - 8 + i, y - 9, 0xC8C4B6);
    g.drawPixel(x - 8 + i, y + 5, 0xC8C4B6);
  }
  g.fillRect(x + 4, y - 12, 5, 5, 0x9A968A);
  int ph2 = p.t % 80;
  float glowK = p.spawned < 6 && ph2 >= 18 && ph2 <= 70 ? 1 - abs(ph2 - 40) / 30.0f : 0;
  if (glowK > 0 && !ground)
    for (int j = -6; j <= 6; j++)
      for (int i = -8; i <= 8; i++)
        g.blendPixel(x + i, y - 2 + j, 0xB8FFD0, (int)(180 * glowK * max(0.0f, 1 - (i * i + j * j) / 100.0f)));
}

// 灯台の光: 回る 2 本の光の帯
void Game::lighthouseBeam(Gfx &g, int x, int y)
{
  for (int b = 0; b < 2; b++)
  {
    float a = frame * 0.05f + b * PI, ca = cosf(a), sa = sinf(a);
    for (int d = 4; d < 70; d++)
      for (int s = -d / 8; s <= d / 8; s++)
      {
        float px2 = x + ca * d - sa * s, py2 = y + sa * d + ca * s;
        g.blendPixel((int)px2, (int)py2, 0xFFF4C0, max(0, 90 - d));
      }
  }
  glow(g, x - 1, y - 1, 0xFFF4C0, 2);
}

// 座ったブラック・シャックの赤く光る目
void Game::shuckEyes(Gfx &g, int ex0, int ey, int gap)
{
  for (int s = 0; s < 2; s++)
  {
    int ex = ex0 + s * gap;
    for (int j = -2; j <= 2; j++)
      for (int i = -2; i <= 3; i++)
        g.blendPixel(ex + i, ey + j, 0xFF4030, max(0, 110 - (i * i + j * j) * 20));
    g.fillRect(ex, ey, 2, 1, 0xFFB0A0);
  }
}

// 石の納屋 (ヨークシャーの谷): 灰色の石の壁にスレート屋根
void Game::drawStoneBarn(Gfx &g, int x, int y)
{
  int x0 = x - 9, y0 = y - 6;
  for (int j = 0; j < 10; j++)
    for (int i = 0; i < 18; i++)
    {
      uint32_t c = j < 5 ? 0x7A8090u : 0x5A6070u;
      if (j % 3 == 2)
        c = mulc(c, 0.85f);
      if (j == 5)
        c = 0x8A90A0;
      if (j == 9 || i == 17)
        c = mulc(c, 0.72f);
      g.drawPixel(x0 + i, y0 + j, c);
    }
  g.fillRect(x0, y0 + 10, 18, 3, 0x9A968A);
  g.drawFastHLine(x0, y0 + 12, 18, 0x76726A);
  g.fillRect(x0 + 7, y0 + 10, 4, 3, 0x5A3A22);
}

// ---- 絵はがきの風景 (ヨークシャー、コーンウォール、高地、結末) ----
void Game::yorkshireScene(Gfx &g, int x0, int y0, int w, int h)
{
  g.setClipRect(x0, y0, w, h);
  for (int j = 0; j < h; j++)
    g.drawFastHLine(x0, y0 + j, w, mix(0xB8C8D8, 0xEADCC8, (float)j / h));
  for (int x = 0; x < w; x++) // 紫の荒れ野の丘
  {
    int top = y0 + 30 - (int)(14 * (0.5f + 0.5f * sinf(x * 0.03f + 1)));
    g.drawFastVLine(x0 + x, top, 30, (x * 7 % 5) == 0 ? 0x9A6A92u : 0x8A5A82u);
  }
  for (int x = 0; x < w; x++) // 緑の谷
  {
    int top = y0 + 52 - (int)(10 * sinf(x * 0.022f));
    g.drawFastVLine(x0 + x, top, h, mix(0x7AA050, 0x5E8A3C, (x % 40) / 40.0f));
  }
  for (int k = 0; k < 6; k++) // 石垣
    for (int x = 0; x < w; x += 2)
      g.drawPixel(x0 + x, y0 + 60 + k * 9 + (int)(3 * sinf(x * 0.04f + k)), 0xA8A498);
  int vy = y0 + 46; // 遠くの高架橋と汽車
  for (int x = 30; x < w - 30; x++)
  {
    int ph3 = x % 14;
    g.drawPixel(x0 + x, vy, 0xA89E8E);
    if (ph3 < 3)
      g.drawFastVLine(x0 + x, vy, 12, 0x968C7C);
    else if (ph3 == 3 || ph3 == 13)
      g.drawPixel(x0 + x, vy + 1, 0x968C7C);
  }
  int tx = x0 + 40 + (frame / 3) % (w - 100);
  g.fillRect(tx, vy - 4, 18, 4, 0x2A5A3A);
  g.fillRect(tx + 18, vy - 4, 12, 4, 0x7A2A2A);
  g.fillCircle(tx + 2, vy - 8, 3, 0xF0F0F0);
  g.fillCircle(x0 + 30, y0 + 70, 9, 0xD8883A); // 紅葉の木
  g.fillCircle(x0 + 28, y0 + 68, 6, 0xF0B850);
  g.fillRect(x0 + 29, y0 + 78, 2, 8, 0x5A3A22);
  g.fillRect(x0 + w - 60, y0 + 70, 14, 8, 0x9A968A); // 石の納屋
  g.fillTriangle(x0 + w - 62, y0 + 70, x0 + w - 44, y0 + 70, x0 + w - 53, y0 + 64, 0x5E6474);
  g.clearClipRect();
  g.drawRect(x0 - 1, y0 - 1, w + 2, h + 2, 0x8A7A5A);
}

void Game::cornwallScene(Gfx &g, int x0, int y0, int w, int h)
{
  g.setClipRect(x0, y0, w, h);
  for (int j = 0; j < h; j++)
    g.drawFastHLine(x0, y0 + j, w, mix(0x7AB8E8, 0xE0F0F8, (float)j / h));
  int seaY = y0 + 52;
  for (int j = seaY; j < y0 + h; j++) // 海
    g.drawFastHLine(x0, j, w, mix(0x4A8AC0, 0x2A5E8E, (j - seaY) / (float)(h - 52)));
  for (int k = 0; k < 30; k++)
    g.drawFastHLine(x0 + (int)(ph(k, 3) % (uint32_t)w), seaY + 4 + (int)(ph(k, 5) % 50), 4, 0xC8E4F4);
  // 入り江のセント・マイケルズ・マウント
  g.fillTriangle(x0 + 60, seaY + 2, x0 + 110, seaY + 2, x0 + 85, seaY - 22, 0x5E8A3C);
  g.fillRect(x0 + 78, seaY - 28, 14, 8, 0x9A968A);
  g.fillRect(x0 + 88, seaY - 32, 4, 6, 0xA8A498);
  // 右の崖と灯台
  g.fillTriangle(x0 + w - 90, y0 + h, x0 + w, y0 + 30, x0 + w, y0 + h, 0x6E9A48);
  g.fillTriangle(x0 + w - 90, y0 + h, x0 + w - 60, y0 + 50, x0 + w, y0 + 30, 0x8A867A);
  int lx = x0 + w - 40;
  for (int y = y0 + 12; y < y0 + 40; y++)
    g.drawFastHLine(lx - 3, y, 7, ((y - y0) / 5) % 2 == 0 ? 0xF4F2EAu : POPPY);
  g.fillRect(lx - 3, y0 + 8, 7, 4, 0xF8E890);
  for (int k = 0; k < 4; k++) // カモメ
  {
    int gx = x0 + 30 + k * 30 + (frame / 2 + k * 20) % 40, gy = y0 + 20 + k * 5;
    g.drawPixel(gx, gy, 0xFFFFFF);
    g.drawPixel(gx - 1, gy - 1, 0xFFFFFF);
    g.drawPixel(gx + 1, gy - 1, 0xFFFFFF);
  }
  g.clearClipRect();
  g.drawRect(x0 - 1, y0 - 1, w + 2, h + 2, 0x8A7A5A);
}

void Game::highlandsScene(Gfx &g, int x0, int y0, int w, int h)
{
  g.setClipRect(x0, y0, w, h);
  for (int j = 0; j < h; j++)
    g.drawFastHLine(x0, y0 + j, w, mix(0x8AA8C8, 0xDCE4EC, (float)j / h));
  for (int layer = 0; layer < 2; layer++) // 雪の峰
    for (int x = 0; x < w; x++)
    {
      float fx = x * (0.035f + layer * 0.015f) + layer * 3;
      int top = y0 + 50 - (int)((40 - layer * 14) * (0.5f + 0.4f * sinf(fx) + 0.1f * sinf(fx * 3)));
      int baseY = y0 + 62;
      for (int y = top; y < baseY; y++)
        g.drawPixel(x0 + x, y, y < top + 6 - layer * 2 ? 0xF4F6F8u : (layer == 0 ? 0x7E8A9Au : 0x6A7A5Au));
    }
  for (int j = 0; j < 20; j++) // 湖
    g.drawFastHLine(x0, y0 + 62 + j, w, mix(0x5A7AA0, 0x3E5E86, j / 20.0f));
  int cx = x0 + 90; // 小島の城
  g.fillEllipse(cx, y0 + 66, 16, 4, 0x6E6A60);
  g.fillRect(cx - 8, y0 + 50, 16, 14, 0x8E8A7E);
  for (int x = cx - 8; x < cx + 8; x += 3)
    g.fillRect(x, y0 + 48, 2, 2, 0x8E8A7E);
  for (int i = 0; i < 40; i++)
    g.fillRect(cx + 10 + i, y0 + 62, 1, 2, (i % 8) < 2 ? 0x5E5A50u : 0x9A968Au);
  for (int j = 0; j < h - 82; j++) // 手前の荒れ野
    g.drawFastHLine(x0, y0 + 82 + j, w, mix(0x7A6A6A, 0x6E7A4A, j / (float)(h - 82)));
  int hx = x0 + w - 70, hy = y0 + 96; // ハイランド牛
  g.fillEllipse(hx, hy, 9, 5, 0xC2703A);
  g.fillEllipse(hx + 9, hy - 2, 4, 4, 0xB0602E);
  g.drawLine(hx + 6, hy - 6, hx + 3, hy - 9, 0xF4ECD8);
  g.drawLine(hx + 12, hy - 6, hx + 15, hy - 9, 0xF4ECD8);
  g.clearClipRect();
  g.drawRect(x0 - 1, y0 - 1, w + 2, h + 2, 0x8A7A5A);
}

// 結末: 星空の下、手編みの赤いセーターを着た宇宙人が円盤から手を振って帰っていく。丘には羊と、手を振るおばあさんの家
void Game::endingScene(Gfx &g, int x0, int y0, int w, int h)
{
  g.setClipRect(x0, y0, w, h);
  for (int j = 0; j < h; j++)
    g.drawFastHLine(x0, y0 + j, w, mix(0x1A2A4A, 0x4A5A7A, (float)j / h));
  for (int k = 0; k < 40; k++)
    g.drawPixel(x0 + (int)(ph(k, 1) % (uint32_t)w), y0 + (int)(ph(k, 2) % 70), ((frame / 10 + k) % 5) == 0 ? 0xFFFFFFu : 0xC8D0E0u);
  for (int x = 0; x < w; x++) // オーロラ
    for (int y = 0; y < 30; y++)
      g.blendPixel(x0 + x, y0 + 10 + y + (int)(6 * sinf(x * 0.05f + frame * 0.02f)), 0x6AFFA8, (int)(60 * (1 - y / 30.0f)));
  for (int x = 0; x < w; x++) // 丘
  {
    int top = y0 + 80 - (int)(12 * sinf(x * 0.02f + 0.5f));
    g.drawFastVLine(x0 + x, top, h, 0x3E5E3A);
  }
  int sx = x0 + w / 2 - 10 + (int)(10 * sinf(frame * 0.03f)), sy = y0 + 36 - (int)fmodf(frame * 0.15f, 20); // 円盤
  g.fillEllipse(sx, sy, 22, 7, 0xB8BCC8);
  g.fillEllipse(sx, sy - 2, 18, 4, 0xDCE0E8);
  g.fillEllipse(sx, sy - 7, 9, 7, 0x8ADCE8);
  g.fillCircle(sx, sy - 8, 3, 0x7ACC6A); // 宇宙人の顔
  g.drawPixel(sx - 1, sy - 9, 0x101010);
  g.drawPixel(sx + 1, sy - 9, 0x101010);
  g.drawLine(sx - 2, sy - 11, sx - 3, sy - 14, 0x7ACC6A);
  g.drawLine(sx + 2, sy - 11, sx + 3, sy - 14, 0x7ACC6A);
  g.fillRect(sx - 4, sy - 5, 9, 4, 0xD83A3A); // 赤いセーター
  g.drawFastHLine(sx - 4, sy - 4, 9, 0xF0E8D8);
  int wave = (frame / 8) % 2;
  g.drawLine(sx + 4, sy - 4, sx + 8, sy - 8 - wave * 2, 0xD83A3A); // 手を振る
  for (int k = 0; k < 7; k++)
    g.drawPixel(sx - 18 + k * 6, sy + 2, RIM[(k + frame / 6) % 4]);
  for (int k = 0; k < 6; k++) // 羊
  {
    int lx = x0 + 30 + k * 18 + (k & 1) * 5, ly = y0 + 92 + (k % 3) * 3;
    g.fillRect(lx, ly, 5, 3, 0xF4F2EA);
    g.drawPixel(lx + 5, ly, 0x2A2A2A);
  }
  int hx = x0 + w - 60, hy = y0 + 80; // おばあさんの家 (窓に灯り)
  g.fillRect(hx, hy, 18, 10, 0xEDE3C8);
  g.fillTriangle(hx - 2, hy, hx + 20, hy, hx + 9, hy - 8, 0xB08A4A);
  g.fillRect(hx + 3, hy + 3, 3, 3, 0xFFD070);
  g.fillRect(hx + 12, hy + 3, 3, 3, 0xFFD070);
  g.clearClipRect();
  g.drawRect(x0 - 1, y0 - 1, w + 2, h + 2, 0x8A7A5A);
}

static const char *const ENDING[] = {
    "THE ALIENS ONLY WANTED TO BE WARM.",
    "GRANNY KNITTED THEM A JUMPER,",
    "AND THEY FLEW HOME HAPPY.",
    "THE SHEEP ARE SAFE AGAIN.",
};
void Game::drawEndingCard(Gfx &g)
{
  int y0 = 60;
  postcard(g, y0, 380);
  txt(g, SW / 2, y0 + 18, GOLD, "THE END", Datum::TC, 3);
  if (g.visible(y0 + 56, y0 + 190))
    endingScene(g, 26, y0 + 58, SW - 52, 130);
  for (int i = 0; i < 4; i++)
    g.text(SW / 2, y0 + 206 + i * 16, i == 3 ? DEEP : BROWN, ENDING[i], Datum::TC);
  g.text(SW / 2, y0 + 290, POPPY, "THANK YOU FOR FLYING, SKYLARK!", Datum::TC);
  if ((frame / 15) % 2 != 0)
  {
    char s[32];
    snprintf(s, sizeof(s), "ROUND %d - PUSH SW1", (day - 1) / REGIONS + 2);
    g.text(SW / 2, y0 + 340, DEEP, s, Datum::TC);
  }
}
