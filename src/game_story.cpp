// 物語「ひばり号と羊どろぼうの円盤」: 1 日ごとにイギリスの別の地方へ、円盤の母船を追って旅をする (Windows 版の GameStory.cs)。
// 日の始まりに旅の絵はがき (地方の絵と短い便り)、日の終わりにイギリスの地図の上を次の地方へ飛ぶ場面。
// 地方ごとに地形の作り方、羊の種類、天気、名所が変わる
#include "game.h"

static const char *const REGION_NAME[] = {"WILTSHIRE", "THE LAKES", "YORKSHIRE", "CORNWALL", "HIGHLANDS"};
extern const char *const AREA1_NAME[];
extern const char *const AREA2_NAME[];
const char *const AREA1_NAME[] = {"MEADOW VALE", "WINDERMERE", "THE DALES", "THE COAST", "THE GLENS"};
const char *const AREA2_NAME[] = {"THE DOWNS", "HELVELLYN", "THE MOORS", "BODMIN MOOR", "LOCH NESS"};
static const char *const STORY[][3] = {
    {"SAUCERS OVER WILTSHIRE!", "THEY ARE AFTER OUR SHEEP.", "UP YOU GO, SKYLARK!"},
    {"THE MOTHERSHIP FLED NORTH", "TO THE MISTY LAKES.", "MIND THE BOATS - AND BOWNESSIE!"},
    {"ON THE PURPLE MOORS", "THE SAUCERS LIE IN WAIT.", "FOLLOW THE OLD RAILWAY."},
    {"DOWN TO THE SEA!", "THE SAUCERS HIDE BY THE CLIFFS.", "WATCH OUT FOR THE BEAST."},
    {"THE LAST STAND IN THE GLENS.", "THEY WANT EVERY SHEEP -", "AND NESSIE TOO!"},
};
// 地図の上の地方の場所 (地図の幅と高さに対する割合)
static const float REGION_MX[] = {0.57f, 0.48f, 0.64f, 0.37f, 0.49f}, REGION_MY[] = {0.80f, 0.49f, 0.53f, 0.88f, 0.21f};

// ---- 旅の流れ ----
void Game::startIntro()
{
  mode = Mode::Intro;
  modeT = INTRO_T;
  openAt = -9999;
  sfx(Sfx::Start);
}
void Game::stepIntro(const Input &inp)
{
  if (--modeT <= 0 || (modeT < INTRO_T - 30 && (inp.FirePressed || inp.StartPressed)))
  {
    mode = Mode::Play; // 幕を開けて飛び立つ
    openAt = frame;
    flyIn();
  }
}
void Game::startTravel()
{
  if (region == 4) // 最後の日: 結末の絵はがき
  {
    mode = Mode::Ending;
    modeT = ENDING_T;
    sfx(Sfx::Extend);
    return;
  }
  mode = Mode::Travel;
  modeT = TRAVEL_T;
  travelFrom = region;
  travelTo = regionOf(day + 1);
  sfx(Sfx::Discover);
}
void Game::stepEnding(const Input &inp)
{
  if (--modeT <= 0 || (modeT < ENDING_T - 60 && (inp.FirePressed || inp.StartPressed)))
    NextDay();
}
void Game::stepTravel(const Input &inp)
{
  if (--modeT <= 0 || (modeT < TRAVEL_T - 30 && (inp.FirePressed || inp.StartPressed)))
    NextDay();
}

// ---- 天気: 湖水地方は霧雨 (早めに降りだして長く、弱い)、そのあと虹 ----
int Game::rainAt() const { return region == 1 ? 30 * FPS : region == 4 ? 118 * FPS : RAIN_AT; } // 高地は夕方から雪
int Game::rainEnd() const { return region == 1 ? 64 * FPS : region == 4 ? 160 * FPS : RAIN_END; }
float Game::rainbowK() const
{
  if (region != 1)
    return 0;
  int s = rainEnd() + 8 * FPS;
  return ramp(s, s + 4 * FPS) - ramp(s + 16 * FPS, s + 22 * FPS);
}

static const uint32_t RAINBOW[] = {0xE84A4A, 0xF0943A, 0xF4E04A, 0x6AC85A, 0x4A9AE0, 0x5A5AC8, 0x9A5AC0};
void Game::drawRainbow(Gfx &g, int cx, int cy, int r)
{
  float k = rainbowK();
  if (k <= 0)
    return;
  int top = max(max(0, cy - r - 2), g.top), bot = min(min(SH - 1, cy), g.top + STRIP - 1);
  float ri = r - 7 * 3.2f;
  for (int y = top; y <= bot; y++)
  {
    float dy = y - cy, xo2 = r * r - dy * dy;
    if (xo2 <= 0)
      continue;
    int xo = (int)sqrtf(xo2) + 1, xi = (int)sqrtf(max(0.0f, ri * ri - dy * dy)) - 1;
    for (int s = -1; s <= 1; s += 2)
      for (int ax = max(0, xi); ax <= xo; ax++) // 帯がかかる横の範囲だけ
      {
        int x = cx + s * ax;
        if (x < 0 || x >= SW)
          continue;
        float d = sqrtf((x - cx) * (x - cx) + dy * dy), b = (r - d) / 3.2f;
        if (b < 0 || b >= 7)
          continue;
        int band = (int)b;
        float edge = min(1.0f, min(b, 7 - b) * 1.5f);
        float fade = min(1.0f, (cy - y) / 120.0f); // 地面に近いところは薄く
        g.blendPixel(x, y, RAINBOW[band], (int)(85 * k * edge * fade));
      }
  }
}

// ---- 湖水地方のもの ----
const Pal *Game::sheepPal() const { return region == 1 ? &HERDWICK : region == 2 ? &SWALEDALE : &SHEEP_S.pal; }

static const uint32_t MOOR_LAKES[] = {0, 0x86904E, 0x9A7440, 0x8E8E86, 0xBCBCB8}; // 荒れ地、さび色のシダ、岩、頂
static const uint32_t MOOR_YORKS[] = {0, 0x7A7A48, 0x8A5A82, 0x8E8A80, 0xA8A49C};
static const uint32_t MOOR_CORN[] = {0, 0x8A8E50, 0x8A9040, 0x9A968E, 0xB4B0A8};
static const uint32_t MOOR_HIGH[] = {0, 0x6E7A4A, 0x7A6074, 0x8C8C8A, 0xEEF2F4};
uint32_t Game::moorCol(int lv) const
{
  return region == 1 ? MOOR_LAKES[lv] : region == 2 ? MOOR_YORKS[lv] : region == 3 ? MOOR_CORN[lv] : region == 4 ? MOOR_HIGH[lv] : MOOR_C[lv];
}

// ボート (真上から): 手こぎは茶色の舟に漕ぎ手とオール、ヨットは白い舟に帆。うしろに V 字の波
void Game::drawBoat(Gfx &g, Prop &p, int x, int y)
{
  int dir = p.vx >= 0 ? 1 : -1;
  for (int k = 3; k <= 9; k++)
  {
    int a = 110 - k * 11;
    g.blendPixel(x - dir * (k + 4), y - k / 2, 0xD8ECF8, a);
    g.blendPixel(x - dir * (k + 4), y + k / 2, 0xD8ECF8, a);
  }
  if (p.v == 2) // 漁船: 赤い舟に白い操舵室とマスト
  {
    g.fillEllipse(x, y, 8, 3, 0x6A1E14);
    g.fillEllipse(x, y, 7, 2, 0xB83A2A);
    g.drawFastHLine(x - 6, y, 13, 0xF0E8D8);
    g.fillRect(x - dir * 3 - 2, y - 2, 4, 3, 0xF4F2EA);
    g.drawPixel(x + dir * 3, y - 3, 0x5A4030);
    g.drawPixel(x + dir * 3, y - 4, 0x5A4030);
  }
  else if (p.v == 0)
  {
    g.fillEllipse(x, y, 7, 3, 0x5A3A22);
    g.fillEllipse(x, y, 6, 2, 0x9A6A3E);
    g.drawFastVLine(x - 2, y - 2, 5, 0x6A4A2A);
    g.fillCircle(x + 1, y, 1, 0x3A4A6A); // 漕ぎ手
    g.drawPixel(x + 1, y - 1, 0xE8C8A8);
    int st = (frame / 8) % 2;
    g.drawLine(x + 1, y - 2, x + 1 - dir * (2 + st * 2), y - 6, 0xC8A878); // オール
    g.drawLine(x + 1, y + 2, x + 1 - dir * (2 + st * 2), y + 6, 0xC8A878);
  }
  else
  {
    for (int j = 0; j < 4; j++) // 帆の影
      g.blendPixel(x + 3 + j, y + 3 + j / 2, 0x1A2A3A, 70);
    g.fillEllipse(x, y, 7, 3, 0x8A8A84);
    g.fillEllipse(x, y, 6, 2, 0xF4F2EA);
    g.fillTriangle(x - dir * 4, y, x + dir * 3, y, x - dir * 1, y - 9, 0xFFFFFF); // 帆
    g.drawLine(x - dir * 1, y - 9, x - dir * 1, y, 0x8A6A44);
    g.drawPixel(x - dir * 1, y - 10, POPPY);
  }
}

// キャッスルリッグの石の環 (斜め上から): 不ぞろいな 38 個の石が楕円に並び、北が入り口。内側の東に小さな四角の石組み
// 石は奥 (上) から手前 (下) の順に描く
void Game::castleStones(Gfx &g, int x, int y, bool shadow)
{
  struct St
  {
    int bx, by, w, hh;
    uint32_t h;
  };
  St items[48];
  int n = 0;
  for (int i = 0; i < 38; i++)
  {
    float a = i * (2 * PI / 38) + 0.05f;
    if (fabsf(a - 3 * PI / 2) < 0.18f) // 北の入り口
      continue;
    uint32_t h = ph(i * 13 + 7, 91);
    items[n++] = {x + (int)roundf(cosf(a) * 32), y + (int)roundf(sinf(a) * 16), 3 + (int)(h & 1), 5 + (int)(h % 6), h};
  }
  for (int i = 0; i < 10; i++) // サンクチュアリ
    items[n++] = {x + 8 + (i < 5 ? i * 3 : (i - 5) * 3), y + (i < 5 ? -4 : 5), 2, 4, ph(i, 33)};
  std::stable_sort(items, items + n, [](const St &a, const St &b) { return a.by < b.by; });
  bool keepMask = g.maskMode;
  if (shadow)
    g.maskMode = true;
  for (int k = 0; k < n; k++)
  {
    const St &s = items[k];
    int x0 = s.bx - s.w / 2, by = s.by, w = s.w, hh = s.hh;
    if (shadow)
    {
      int sx = hh * 7 / 10, sy = hh / 4;
      g.fillTriangle(x0, by, x0 + w, by, x0 + w + sx, by + sy, 0);
      g.fillTriangle(x0, by, x0 + w + sx, by + sy, x0 + sx, by + sy, 0);
      continue;
    }
    uint32_t face = mulc(0xA8A498, 0.9f + ((s.h >> 8) & 3) * 0.05f);
    g.fillRect(x0, by - hh, w, hh, face);
    g.drawFastVLine(x0, by - hh + 1, hh - 1, mix(face, 0xFFFFFF, 0.25f));
    g.drawFastVLine(x0 + w - 1, by - hh + 1, hh - 1, mulc(face, 0.72f));
    g.drawFastHLine(x0 + 1, by - hh, max(1, w - 2), mix(face, 0xFFFFFF, 0.35f)); // 丸い頭
    g.drawPixel(x0, by - hh, (s.h & 2) != 0 ? 0x8A9A6Au : mulc(face, 0.8f));     // 苔
    g.drawFastHLine(x0, by - 1, w, 0x4E4A42);
  }
  g.maskMode = keepMask;
}

void Game::drawCastlerigg(Gfx &g, int x, int y, Prop &p)
{
  for (int j = -26; j <= 26; j++) // 踏まれて薄くなった芝の輪
    for (int i = -48; i <= 48; i++)
    {
      float q = sqrtf((i / 44.0f) * (i / 44.0f) + (j / 23.0f) * (j / 23.0f));
      if (q > 0.86f && q < 1.04f)
        g.blendPixel(x + i, y + j, 0xD8E8B0, 60);
    }
  int ph2 = p.t % 80; // 円盤が湧いてくるとき、真ん中が緑に光る
  float glowK = p.spawned < 6 && ph2 >= 18 && ph2 <= 70 ? 1 - abs(ph2 - 40) / 30.0f : 0;
  if (glowK > 0)
    for (int j = -10; j <= 10; j++)
      for (int i = -20; i <= 20; i++)
      {
        float q = i * i / 400.0f + j * j / 100.0f;
        if (q < 1)
          g.blendPixel(x + i, y + j, 0xB8FFD0, (int)(230 * glowK * (1 - q * q)));
      }
  castleStones(g, x, y, false);
}

// ボウネッシー (ウィンダミア湖の怪物): ネッシーよりずっと低く、水面に 3 つのこぶと小さな頭が並ぶ。緑がかった灰色
bool Game::bownessieAt(float u, float v, float e) const
{
  if (v < 0)
    return false;
  static const float hu[] = {-17, -9, -1}, hw[] = {4.5f, 5.5f, 5.5f}, hv[] = {3.5f, 5, 5.5f};
  for (int k = 0; k < 3; k++)
  {
    float t = (u - hu[k]) / hw[k];
    if (t > -1 && t < 1 && v <= hv[k] * e * sqrtf(1 - t * t))
      return true;
  }
  if (u > 3.2f && u < 5.8f && v <= 6.2f * e) // 短い首
    return true;
  float qu = (u - 7.5f) / 3.2f, qv = (v - 6.5f * e) / 1.9f; // 頭
  return qu * qu + qv * qv <= 1;
}

void Game::drawBownessie(Gfx &g, Prop &p, int x, int y)
{
  float e = p.lift, sc = p.kind == 2 ? 1.45f : 1; // モーガー (海の大蛇) は大きい
  int dir = p.v != 0 ? 1 : -1;
  if (e <= 0.02f)
  {
    for (int j = -4; j <= 4; j++)
      for (int i = -16; i <= 16; i++)
        if (i * i / 256.0f + j * j / 16.0f < 1)
          g.blendPixel(x + i, y + j, 0x1E2A30, 55);
    return;
  }
  uint32_t DARK = p.kind == 2 ? 0x1A2A2Au : 0x222A20u, BODY = p.kind == 2 ? 0x2A4A4Cu : 0x3A4636u, RIMC = p.kind == 2 ? 0x5E9A90u : 0x6E7E62u;
  for (int r = 0; r < 4; r++) // こぶのまわりの波紋
  {
    float cu = r == 3 ? 7 : -17 + r * 8;
    for (int w = 0; w < 2; w++)
    {
      float rx_ = 6 + w * 4 + ((frame / 6 + r) % 4), ry_ = rx_ * 0.28f;
      for (int s = 0; s < 32; s++)
      {
        float a = s * (2 * PI / 32);
        g.blendPixel(x + (int)((dir * cu + cosf(a) * rx_) * sc), y + 1 + (int)(sinf(a) * ry_ * sc), 0xE0F0FA, 110 - w * 40);
      }
    }
  }
  for (int v = 1; v <= (int)(12 * sc); v++) // 映り込み
  {
    float wave = sinf(v * 0.9f + frame * 0.25f) * 1.3f;
    int a = (int)(140 * (1 - v / (13.0f * sc)));
    for (int u = (int)(-22 * sc); u <= (int)(12 * sc); u++)
      if (bownessieAt((u + wave) / sc, v / sc, e))
        g.blendPixel(x + dir * u, y + v, 0x141A12, a);
  }
  for (int v = 0; v <= (int)(10 * sc); v++)
    for (int u = (int)(-22 * sc); u <= (int)(12 * sc); u++)
    {
      if (!bownessieAt(u / sc, v / sc, e))
        continue;
      uint32_t c = v <= 1 ? DARK : BODY;
      if (!bownessieAt(u / sc, (v + 1) / sc, e) || !bownessieAt((u - 1) / sc, (v + 1) / sc, e))
        c = RIMC;
      g.drawPixel(x + dir * u, y - v, c);
    }
  if (e > 0.85f)
    g.drawPixel(x + dir * (int)(9 * sc), y - (int)(7 * e * sc), 0xD8E0B0);
}

// ---- 絵はがき (日の始まり) ----
void Game::drawIntroCard(Gfx &g)
{
  int y0 = 70;
  postcard(g, y0, 340);
  char s[32];
  if (day > REGIONS)
    snprintf(s, sizeof(s), "DAY %d  ROUND %d", day, (day - 1) / REGIONS + 1);
  else
    snprintf(s, sizeof(s), "DAY %d", day);
  txt(g, SW / 2, y0 + 18, GOLD, s, Datum::TC, 2);
  txt(g, SW / 2, y0 + 40, POPPY, REGION_NAME[region], Datum::TC, 3);
  regionScene(g, region, 26, y0 + 76, SW - 52, 112);
  for (int i = 0; i < 3; i++)
    g.text(SW / 2, y0 + 208 + i * 16, i == 2 ? DEEP : BROWN, STORY[region][i], Datum::TC);
  if ((frame / 15) % 2 != 0 || modeT % 30 < 18)
    g.text(SW / 2, y0 + 300, DEEP, "PUSH SW1 TO FLY", Datum::TC);
}

void Game::regionScene(Gfx &g, int reg, int x0, int y0, int w, int h)
{
  if (!g.visible(y0 - 2, y0 + h + 2))
    return;
  if (reg == 1)
    lakesScene(g, x0, y0, w, h);
  else if (reg == 2)
    yorkshireScene(g, x0, y0, w, h);
  else if (reg == 3)
    cornwallScene(g, x0, y0, w, h);
  else if (reg == 4)
    highlandsScene(g, x0, y0, w, h);
  else
    drawPostcardScene(g, x0, y0, w, h);
}

// 湖水地方の小さな風景: 霧の空、重なる山 (さび色のシダ)、湖と映り込み、ヨット、白い家、石垣と灰色の羊
void Game::lakesScene(Gfx &g, int x0, int y0, int w, int h)
{
  g.setClipRect(x0, y0, w, h);
  for (int j = 0; j < h; j++)
    g.drawFastHLine(x0, y0 + j, w, mix(0xA8C4D8, 0xE4ECEE, (float)j / h));
  int lakeY = y0 + 62;
  static const uint32_t fellC[] = {0x9AAEA8, 0x7E9A7A, 0x6A8A5E};
  for (int layer = 0; layer < 3; layer++) // 山
  {
    for (int x = 0; x < w; x++)
    {
      float fx = x * (0.03f + layer * 0.012f) + layer * 2.1f;
      int top = lakeY - 8 - (int)((26 - layer * 7) * (0.5f + 0.35f * sinf(fx) + 0.15f * sinf(fx * 2.3f + 1)));
      g.drawFastVLine(x0 + x, top, lakeY - top, fellC[layer]);
      if (layer == 2 && ((x / 6 + (x * 7 % 5)) % 4 == 0))
        g.drawFastVLine(x0 + x, top + 3, max(1, (lakeY - top) / 3), 0xA07840); // シダ
      if (layer == 0)
        g.drawPixel(x0 + x, top, 0xC8D4D0);
    }
  }
  for (int j = 0; j < 22; j++) // 湖と、山の映り込み
    for (int x = 0; x < w; x++)
    {
      uint32_t c = mix(0x6A94BC, 0x4E7AA6, j / 22.0f);
      if (j < 8 && ((x * 3 + j) % 9) < 4)
        c = mix(c, 0x6A8A5E, 0.35f);
      if ((x + j * 5 + frame / 6) % 23 == 0)
        c = 0xD8ECF8;
      g.drawPixel(x0 + x, lakeY + j, c);
    }
  int bx = x0 + 60 + (frame / 4) % (w - 120); // ヨット
  g.fillTriangle(bx, lakeY + 6, bx + 7, lakeY + 6, bx + 3, lakeY - 6, 0xFFFFFF);
  g.fillRect(bx - 1, lakeY + 6, 10, 2, 0xF0EEE6);
  g.drawFastHLine(bx, lakeY + 9, 8, 0x8AB0D0);
  int hx = x0 + w - 70; // 白い家
  g.fillRect(hx, lakeY - 9, 14, 8, 0xF4F2EA);
  g.fillTriangle(hx - 2, lakeY - 9, hx + 16, lakeY - 9, hx + 7, lakeY - 15, 0x5E6474);
  g.fillRect(hx + 6, lakeY - 6, 2, 5, 0x3A4A5A);
  for (int j = 0; j < h - 84; j++) // 手前の牧草地と石垣
    g.drawFastHLine(x0, lakeY + 22 + j, w, mix(0x7AA850, 0x6A9446, j / (float)(h - 84)));
  for (int x = 0; x < w; x += 2)
  {
    g.drawPixel(x0 + x, lakeY + 30 + (x / 40) % 2, 0xA8A498);
    g.drawPixel(x0 + x + 1, lakeY + 30 + (x / 40) % 2, 0x86827A);
  }
  for (int k = 0; k < 5; k++)
  {
    int sx = x0 + 40 + k * 15 + (k & 1) * 4, sy = lakeY + 38 + (k % 3) * 3;
    g.fillRect(sx, sy, 4, 2, 0x948E82);
    g.drawPixel(sx + 4, sy, 0xF2F0E8);
  }
  g.clearClipRect();
  g.drawRect(x0 - 1, y0 - 1, w + 2, h + 2, 0x8A7A5A);
}

// ---- 地図 (日の終わり): イギリスの上を、次の地方へ飛ぶ ----
static const float GB_X[] = {0.50f, 0.60f, 0.56f, 0.47f, 0.58f, 0.66f, 0.66f, 0.60f, 0.57f, 0.62f, 0.66f, 0.68f, 0.72f, 0.76f, 0.80f, 0.88f,
                             0.90f, 0.85f, 0.92f, 0.82f, 0.70f, 0.60f, 0.50f, 0.40f, 0.30f, 0.28f, 0.38f, 0.48f, 0.40f, 0.30f, 0.34f, 0.36f,
                             0.30f, 0.38f, 0.46f, 0.46f, 0.42f, 0.46f, 0.36f, 0.40f, 0.36f, 0.34f, 0.38f, 0.30f, 0.36f, 0.32f, 0.38f};
static const float GB_Y[] = {0.03f, 0.04f, 0.10f, 0.13f, 0.15f, 0.17f, 0.22f, 0.28f, 0.31f, 0.33f, 0.38f, 0.44f, 0.51f, 0.58f, 0.63f, 0.66f,
                             0.71f, 0.75f, 0.79f, 0.83f, 0.84f, 0.86f, 0.87f, 0.89f, 0.93f, 0.90f, 0.84f, 0.80f, 0.79f, 0.78f, 0.73f, 0.67f,
                             0.62f, 0.60f, 0.60f, 0.53f, 0.50f, 0.45f, 0.44f, 0.40f, 0.36f, 0.31f, 0.27f, 0.22f, 0.17f, 0.12f, 0.06f};
static const int GB_N = sizeof(GB_X) / sizeof(GB_X[0]);

// 多角形の塗り (上から 1 行ずつ、辺との交わりのあいだを塗る)
static void fillPoly(Gfx &g, const float *xs, const float *ys, int n, uint32_t c)
{
  float minY = 1e9f, maxY = -1e9f;
  for (int i = 0; i < n; i++)
  {
    minY = min(minY, ys[i]);
    maxY = max(maxY, ys[i]);
  }
  float hits[64];
  for (int y = max((int)minY, g.top); y <= min((int)maxY, g.top + STRIP - 1); y++)
  {
    float yc = y + 0.5f;
    int m = 0;
    for (int i = 0, j = n - 1; i < n; j = i++)
      if ((ys[i] <= yc) != (ys[j] <= yc))
        hits[m++] = xs[i] + (yc - ys[i]) / (ys[j] - ys[i]) * (xs[j] - xs[i]);
    std::sort(hits, hits + m);
    for (int k = 0; k + 1 < m; k += 2)
      g.drawFastHLine((int)hits[k], y, (int)hits[k + 1] - (int)hits[k] + 1, c);
  }
}

void Game::drawTravelCard(Gfx &g)
{
  int y0 = 40;
  postcard(g, y0, 436);
  char title[40];
  if (travelTo == 0)
    strcpy(title, "BACK TO WILTSHIRE");
  else
    snprintf(title, sizeof(title), "ON TO %s", REGION_NAME[travelTo]);
  txt(g, SW / 2, y0 + 20, POPPY, title, Datum::TC, 2);
  int mw = 200, mh = 330, mx = (SW - mw) / 2, my = y0 + 52;
  // 海
  for (int j = max(0, g.top - my); j < min(mh, g.top + STRIP - my); j++)
    for (int i = 0; i < mw; i++)
      g.drawPixel(mx + i, my + j, ((i + j * 3 + frame / 4) % 29 == 0) ? 0xC8E4F4u : 0x8AC0E0u);
  g.drawRect(mx - 1, my - 1, mw + 2, mh + 2, 0x8A7A5A);
  // 島: 海に落ちる影 → 陸 → 海岸の線
  float xs[GB_N], ys[GB_N];
  for (int i = 0; i < GB_N; i++)
  {
    xs[i] = mx + GB_X[i] * mw;
    ys[i] = my + 8 + GB_Y[i] * (mh - 16);
  }
  for (int i = 0, j = GB_N - 1; i < GB_N; j = i++)
    g.drawLine((int)xs[j], (int)ys[j] + 2, (int)xs[i], (int)ys[i] + 2, 0x5E8AA8);
  const uint32_t LAND = 0x86BC5C;
  fillPoly(g, xs, ys, GB_N, LAND);
  int land = to888(to565(LAND));
  for (int k = 0; k < 40; k++) // 山や森の点
  {
    uint32_t h = ph(k * 17, 5);
    float px2 = GB_X[h % GB_N] * 0.6f + 0.2f + ((h >> 8) % 20) / 100.0f, py2 = ((h >> 16) % 90) / 100.0f;
    int ix = (int)(mx + px2 * mw), iy = (int)(my + 8 + py2 * (mh - 16));
    if (g.get(ix, iy) == land)
      g.drawPixel(ix, iy, py2 < 0.35f ? 0x6A8A5Eu : 0x6EA04Au);
  }
  for (int i = 0, j = GB_N - 1; i < GB_N; j = i++)
    g.drawLine((int)xs[j], (int)ys[j], (int)xs[i], (int)ys[i], PAPER_C);
  // 地方の点と、たどった道
  auto RX = [&](int r) { return (int)(mx + REGION_MX[r] * mw); };
  auto RY = [&](int r) { return (int)(my + 8 + REGION_MY[r] * (mh - 16)); };
  for (int r = 0; r + 1 < REGIONS; r++)
    for (int s = 0; s <= 20; s += 2)
    {
      float t = s / 20.0f;
      g.drawPixel((int)(RX(r) + (RX(r + 1) - RX(r)) * t), (int)(RY(r) + (RY(r + 1) - RY(r)) * t), 0xA89A78);
    }
  for (int r = 0; r < REGIONS; r++)
  {
    bool visited = r <= travelFrom || (day >= REGIONS);
    uint32_t c = r == travelTo && (frame / 8) % 2 != 0 ? GOLD : visited ? POPPY : 0xA89A78;
    g.fillCircle(RX(r), RY(r), 3, PAPER_C);
    g.fillCircle(RX(r), RY(r), 2, c);
  }
  g.text(RX(travelFrom) + 6, RY(travelFrom) - 3, DEEP, REGION_NAME[travelFrom]);
  g.text(RX(travelTo) + 6, RY(travelTo) - 3, POPPY, REGION_NAME[travelTo]);
  // 飛んでいくスカイラーク号
  float pr = constrain(1 - (modeT - 20) / (float)(TRAVEL_T - 50), 0.0f, 1.0f);
  pr = pr * pr * (3 - 2 * pr);
  int fx2 = (int)(RX(travelFrom) + (RX(travelTo) - RX(travelFrom)) * pr), fy2 = (int)(RY(travelFrom) + (RY(travelTo) - RY(travelFrom)) * pr) - 6;
  g.blit(LIFE, fx2 + 1, fy2 + 1, false, &LIFE_SHADOW);
  g.blit(LIFE, fx2, fy2, false, &LIFE_OLIVE);
  g.text(SW / 2, y0 + 400, BROWN, "PUSH SW1 TO GO", Datum::TC);
}
