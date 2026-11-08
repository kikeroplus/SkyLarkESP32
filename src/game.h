#pragma once
// オリジナルの縦シューティング「SKYLARK」。イギリスの片田舎の上を、プロペラ機がのんびり飛ぶ。
// Windows 版 (WindowsApp/SkyLark) の Game*.cs を C++ に移したもの。中身 (地形、円盤、母船、自機、天気、遊びの流れ、物語) は同じ。
// 世界は 240x320 (地面 1 マス 8 点、30 列)。描くときは 1.5 倍の 360x480 で描き、液晶に出すときに 2/3 に縮める (gfx.h)。
// 2.5D 表示と案 B (絵本風) はまだ移していない
#include <Arduino.h>
#include "gfx.h"
#include "sound.h"

// Arduino.h のマクロを外す (PI は double なので計算が遅くなる。constrain は同じ名前の関数を下で定義する)
#undef PI
#undef constrain

// 1 フレームぶんの操作。main.cpp がジョイスティックとスイッチからまとめる
struct Input
{
  bool Left, Right, Up, Down, Fire, Slow, Bomb;
  float AX, AY; // アナログスティック (-1〜1。使っていなければ 0)
  bool FirePressed, StartPressed, PausePressed, QuitPressed, BackPressed, BombPressed;
  bool VolumePressed; // SW3 を短く押した (音量を 1 段上げる。いちばん上の次は OFF)。PausePressed は SW3 の長押し
  bool UpPressed, DownPressed, LeftPressed, RightPressed;
};

struct Rank
{
  int Score;
  char Name[4];
};

class Game
{
public:
  static constexpr int W = 240, H = 320;
  static constexpr int FPS = 30;

  enum class Mode
  {
    Attract,   // デモ (自動操縦)
    Play,
    Continue,  // 残機がなくなった。数えているあいだに押せば続けられる
    GameOver,
    NameEntry,
    Ranking,   // 遊び終わったあとのランキング
    Intro,     // 日の始まりの旅の絵はがき
    Ending,    // 最後の日のあとの結末
    Travel,    // 日の終わりに、地図の上を次の地方へ
  };
  Mode mode = Mode::Attract;
  bool Paused = false;
  Rank Table[5] = {{80000, "MOL"}, {60000, "BRK"}, {45000, "HAY"}, {30000, "OWL"}, {15000, "EWE"}};
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr; // ランキングが変わったら main.cpp が保存する
  Sound *Snd = nullptr;
  static constexpr int VOL_LEVELS = 5; // 音量の段階 (0 = OFF 〜 4)
  int VolumeLevel = 2;   // SW3 を短く押すたびに 1 段上げる。止めている画面では左右でも変えられる
  bool VolumeChanged = false;
  int volumeT = 0;       // 音量を変えてからしばらく、音量の札を出す

  Game();
  void Step(const Input &inp);
  void Draw(Gfx &g); // 帯 1 本ぶん (g.top から STRIP 行) を描く。帯ごとに呼ぶ
  int Score() const { return score; }

  // ---- 世界の大きさと流れ ----
  static constexpr int CY = (H - 320) / 2; // ESP32 版 (高さ 320) で決めていた文字や枠の高さを、真ん中へ寄せるずらし
  static constexpr int T = 8;                  // 地面の 1 マス
  static constexpr int COLS = (W + T - 1) / T; // 30
  // 地面は画面の上の端より FAR 行ぶん奥まで前もって作る (Windows 版は 2.5D のために 21)。
  // ESP32 版は、画面に入る前に行の絵を少しずつ描いておくために 3 行
  static constexpr int FAR = 3;
  static constexpr int ROWS = H / T + 2 + FAR;
  static constexpr float SPEED = 0.7f; // 地面が流れる速さ (px/フレーム)
  static constexpr int LEAD = (int)(FAR * T / SPEED);
  static constexpr int PY0 = H - 44;
  static constexpr int MOM_Y = 96; // 母船が止まる高さ
  static constexpr int MOM_STAY = 24 * FPS;

  // ---- 1 日の流れ (フレーム) ----
  static constexpr int TITLE_END = 7 * FPS;
  static constexpr int FELL_AT = 80 * FPS; // ここから 20 秒かけて山がちになる
  static constexpr int AREA2_AT = 92 * FPS;
  static constexpr int MOM_AT = 140 * FPS;
  static constexpr int CLEAR_AT = 178 * FPS;
  static constexpr int BONUS_AT = CLEAR_AT + 45;
  static constexpr int RANK_AT = CLEAR_AT + 4 * FPS;
  static constexpr int LOOP_AT = CLEAR_AT + 12 * FPS;
  static constexpr int SHUTTER = 20;
  static constexpr int RAIN_AT = 44 * FPS;    // ここから 6 秒かけて降りだす
  static constexpr int RAIN_END = 70 * FPS;   // ここから 6 秒かけて上がる
  static constexpr int SUNSET_AT = 100 * FPS; // ここから 35 秒かけて夕焼けに
  static constexpr int DUSK_AT = 150 * FPS;   // ここから 25 秒かけて夕暮れに
  static constexpr int STONE_AT = 76 * FPS;
  static constexpr int LOCH_AT = 106 * FPS;
  static constexpr int CROP_AT = 30 * FPS;  // ミステリーサークル (朝の谷の麦畑)
  static constexpr int BEAST_AT = 96 * FPS; // ボドミンの野獣 (AREA 2 の荒れ地)
  static constexpr int CROP_ROWS = 8;
  static constexpr int PLAIN_ROWS = 10, LOCH_ROWS = 26;
  static constexpr int CRASH_T = 150; // 母船が落とされてから地面に落ちるまでのフレーム数

  // 文字と飾り: クリームの紙に深緑とひなげしの赤、空色
  static constexpr uint32_t CREAM = 0xFFF4D8, PAPER = 0xF0E6C8, INKG = 0x1E2A14, DEEP = 0x3A6A2A, POPPY = 0xD84A3A, SKY = 0x8ACAE8,
                            GOLD = 0xF0C860, BROWN = 0x7A6A4A, PAPER_C = 0xF6F0DE;
  static constexpr uint32_t SHADOW = 0x34482A;
  static constexpr uint32_t HEDGE = 0x2F4E24, HEDGE_L = 0x46703A, WALL = 0x8E8A7C, WALL_D = 0x66625A;
  static constexpr uint32_t WATER_C = 0x4A7AA8, WATER_L = 0x7AA8D0, BANK_C = 0x6A9446, REED = 0x4A6A30;
  static constexpr uint32_t LANE_C = 0xB9A57E, LANE_D = 0x9A8A62, STONE = 0xA8A090, STONE_D = 0x6A665A;
  static constexpr uint32_t WOOD_C = 0x2A4A22, WOOD_L = 0x3E6A30, WOOD_H = 0x5A8A3E, GREEN_C = 0x7AA850;
  static const uint32_t MOOR_C[5];
  static constexpr uint32_t SILVER = 0xB8BCC8, SILVER_L = 0xDCE0E8, SILVER_D = 0x7A7E8A, UFO_K = 0x2E323C, DOME = 0x5AC8D8,
                            DOME_L = 0xC8F4F8, BEAM = 0xE8F8A0, PLASMA = 0xFF5AC8, PLASMA_D = 0x5A1040;
  static const uint32_t RIM[4];

  // 地面のマス
  static constexpr uint8_t FIELD = 0, WOOD = 1, WATER = 2, BANK = 3, LANE_V = 4, LANE_H = 5, BRIDGE = 6, MOOR = 7, VGREEN = 8, RAIL = 9,
                           RAILBRIDGE = 10, PLAIN = 11, SEA = 12, SAND = 13, CLIFF = 14;
  static constexpr uint8_t HEDGE_TOP = 1, HEDGE_LEFT = 2, STONEWALL = 4; // 下の縁に生け垣 / 左の縁に生け垣 / 生け垣のかわりに石垣
  struct Tile
  {
    uint8_t type, v, flags;
  };
  // 畑の作物。pat: 0 = 無地に点、1 = 横の畝、2 = 縦の畝、3 = 水田
  struct Crop
  {
    uint32_t b, line;
    int pat;
  };
  static const Crop CROPS[7];
  static constexpr int PASTURE = 0, MEADOW = 1, WHEAT = 2;

  static constexpr int SHEEP = 0, COW = 1, COTTAGE = 2, BARN = 3, CHURCH = 4, TREE = 5, BALE = 6, ROCK = 7, DEBRIS = 8, WRECK = 9;
  static constexpr int DOG = 10, DUCK = 11, PUB = 12, STONEHENGE = 13, NESSIE = 14, BEAST = 15, CROPCIRCLE = 16, RABBIT = 17, BOAT = 18;
  static constexpr int VIADUCT = 19, ABBEY = 20, LIGHTHOUSE = 21, MINE = 22, WHITEHORSE = 23, AAGUN = 24, TANK = 25;
  static constexpr int LAUNDRY = 2; // COTTAGE の v: 1 の位 = スレート屋根、2 = 洗濯物を干している

  // 動くものの位置。描くときは前のフレーム (ox, oy) とのあいだを補う (rx, ry = 描く位置)
  struct Body
  {
    float x = 0, y = 0, ox = 0, oy = 0, rx = 0, ry = 0;
  };
  struct Prop : Body
  {
    float lift = 0;              // 円盤の光線で持ち上げられている量 (0〜1)
    float vx = 0, vy = 0, ax = 0, ay = 0;
    int type = 0, v = 0, t = 0, hop = 0;
    int spawned = 0;             // 名所から湧いて出た円盤の数
    int kind = 0;                // 地方による違い
    bool on = false, held = false;
  };
  static constexpr int SCOUT = 0, TRIO = 1, ABDUCTOR = 2, CARRIER = 3, SPINNER = 4, SPLITTER = 5, MINI = 6, LANCER = 7;
  static const int UFO_HP[8], UFO_PTS[8], UFO_R[8];
  struct Ufo : Body
  {
    float vx = 0, vy = 0, home = 0, a = 0;
    int type = 0, t = 0, hp = 0, flash = 0, group = 0, state = 0, prop = 0;
    bool on = false;
    float rise = 1;   // 地面や湖から出てくる途中 (0〜1)
    int riseKind = 0; // 0 = ストーンヘンジから、1 = 湖から
  };
  struct Shot : Body
  {
    float vx = 0, vy = 0;
    int kind = 0, life = 0;
    float w = 0, tx = 0, ty = 0;
    bool on = false;
  };
  struct Cloud : Body
  {
    int r = 0;
  };
  struct Pop : Body // 「SAVED」「BAA...」などの浮かぶ文字
  {
    int t = 0;
    bool on = false;
    const char *text = "";
    uint32_t col = 0;
  };
  struct Item : Body
  {
    int t = 0;
    bool on = false;
  };
  struct Flock : Body
  {
    float vx = 0, vy = 0;
    int n = 0, t = 0;
    bool on = false;
  };
  struct Train : Body
  {
    int dir = 0, cars = 0;
    bool on = false;
  };
  struct Puff : Body
  {
    float vx = 0, r = 0;
    int t = 0, life = 0;
    bool smoke = false, on = false;
  };
  struct DebrisP : Body
  {
    float vx = 0, vy = 0, alt = 0, da = 0, ang = 0;
    bool on = false;
  };
  struct Drop : Body
  {
  };
  struct Mother : Body
  {
    bool on = false, dead = false;
    int t = 0, hp = 0, maxHp = 0, hitT = 0, deadT = 0;
  };
  struct Bomb : Body
  {
    float sx = 0, sy = 0, tx = 0, ty = 0;
    int t = 0;
    bool on = false;
  };
  struct Spark : Body
  {
    float vx = 0, vy = 0;
    int life = 0, max = 0;
    uint32_t rgb = 0;
  };

  static constexpr float PI = 3.14159265f;
  // atan2 の速い近似 (誤差 0.0001 ラジアンほど)。点ごとに陰影を計算する絵で使う
  static float fastAtan2(float y, float x)
  {
    float ax = fabsf(x), ay = fabsf(y), mx = ax > ay ? ax : ay;
    if (mx == 0)
      return 0;
    float a = (ax < ay ? ax : ay) / mx, s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax)
      r = 1.57079637f - r;
    if (x < 0)
      r = 3.14159274f - r;
    return y < 0 ? -r : r;
  }
  static float constrain(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
  static int constrain(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
  static uint32_t mix(uint32_t a, uint32_t b, float t);

  // 乱数 (xorshift)。毎日の始めに種を変えるので、地形も敵の出方も毎回違う
  uint32_t rngState = 1;
  uint32_t rnd()
  {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
  }
  float frand(float a, float b) { return a + (b - a) * (rnd() / 4294967295.0f); }
  int irand(int a, int b) { return a + (int)(rnd() % (uint32_t)(b - a + 1)); }

  // ---- 遊びの状態 ----
  bool demo = true;
  int day = 1;
  int openAt = 0;
  int modeT = 0;
  bool outOfLives = false, gone = false;
  int enterT = 0, fireCd = 0, continues = 0, nextExtend = 40000;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0;
  static constexpr const char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-! ";
  bool enemyShotSnd = false;
  bool railPending = false;
  float railY = 0;
  float enginePitch = 1;
  bool pubPlaced = false;
  bool worldMoved = false;
  float scrollAdv = 0, rscroll = 0, opx = 0, opy = 0, rpx = 0, rpy = 0;
  int thunderT = 0;

  Tile map[ROWS][COLS];
  int rowId[ROWS];
  float scrollY = 0;
  int rowCount = 0;
  uint32_t seed = 0;
  int segW[COLS], segLeft[COLS], segCrop[COLS];
  bool riverOn = false, laneOn = false;
  float riverC = 0, riverW = 0, riverDrift = 0;
  int riverLen = 0, laneLen = 0, laneCol = 0, villageLen = 0, churchAt = 0;
  bool railRow = false;
  bool stonePlanned = false, lochPlanned = false, stoneFound = false, nessieFound = false;
  int plainLen = 0, stoneCol = 0, lochLen = 0;
  bool cropPlanned = false, cropTopNext = false, cropFound = false, beastDone = false, beastFound = false;
  int lochTotal = LOCH_ROWS, lakeCool = 0;
  int cropLen = 0, cropC = 0;
  float lochC = 0, lochR = 0;

  static constexpr int NPROPS = 160, NUFOS = 28, NMINE = 80, NTHEIRS = 110, NCLOUDS = 5, NPUFFS = 32, NDEBRIS = 10, NDROPS = 64,
                       NPOPS = 4, NSPARKS = 200, NFLAK = 24, NBOMBS = 3;
  Prop props[NPROPS];
  Ufo ufos[NUFOS];
  Shot mine[NMINE], theirs[NTHEIRS], flak[NFLAK];
  Cloud clouds[NCLOUDS];
  Flock flock;
  Train train;
  Puff puffs[NPUFFS];
  DebrisP debris[NDEBRIS];
  Drop drops[NDROPS];
  int birdT = 0, flashT = 0;
  Pop pops[NPOPS];
  Item item;
  Mother mom;
  Bomb bombs[NBOMBS];
  Spark sparks[NSPARKS];
  int bombCd = 0;
  float px = 0, py = 0;
  int frame = 0, tapT = 0, taps = 0, tapsMax = 4, muzzleT = 0, waveT = 0, groupN = 0, invT = 0, deadT = 0;
  int lives = 3, power = 1, score = 0, saved = 0, extendT = 0, newRank = -1;

  // ---- 物語と地方 ----
  static constexpr int REGIONS = 5, REGIONS_READY = 5;
  static constexpr int INTRO_T = 7 * FPS, TRAVEL_T = 6 * FPS, ENDING_T = 16 * FPS;
  static constexpr int BAY_ROWS = 24;
  int region = 0;
  int travelFrom = 0, travelTo = 0;
  bool stonePending = false, abbeyPending = false, viaductPending = false, viaductRowMade = false, forceTrain = false,
       mountPending = false, castlePending = false, horsePending = false, nessiePending = false, morgawrPending = false;
  bool abbeyFound = false, viaductFound = false, horseFound = false;
  int bayLen = 0, bayTotal = 0, lightCool = 0;
  float bayW = 0;

  // ---- 手強さと時間 ----
  int area() const { return frame < AREA2_AT ? 1 : 2; }
  bool playing() const { return frame >= TITLE_END && frame < CLEAR_AT; }
  bool active() const { return frame > TITLE_END + 90 && frame < CLEAR_AT - 150 && !gone; }
  bool momAlive() const { return mom.on && !mom.dead; }
  float fellK() const { return constrain((frame + LEAD - FELL_AT) / (20.0f * FPS), 0.0f, 1.0f); }
  float ramp(int a, int b) const { return constrain((frame - a) / (float)(b - a), 0.0f, 1.0f); }
  float rainK() const;
  float sunsetK() const { return ramp(SUNSET_AT, SUNSET_AT + 35 * FPS); }
  float duskK() const { return ramp(DUSK_AT, DUSK_AT + 25 * FPS); }
  float bulletK() const { return min(1.6f, 1 + 0.12f * (day - 1)); }
  float waveK() const { return max(0.55f, 1 - 0.1f * (day - 1)); }
  float momAlt() const { return mom.dead ? (float)mom.deadT / CRASH_T : 1.0f; }

  void sfx(Sfx s, float vol = 1)
  {
    if (Snd)
      Snd->Play(s, demo ? vol * 0.5f : vol);
  }

  // ---- 地形 (game.cpp) ----
  float hash2(int x, int y) const;
  float noise(float x, float y) const;
  int pickCrop();
  void setSeg(int p, int w);
  void splitSegs(int c, int e);
  void fieldRow(Tile *row);
  void genRow(Tile *row, int n);
  Prop *addProp(float x, float y, int type, int v = 0);
  void newRow();
  bool hasProp(int type) const;
  int tileTypeAt(float x, float y) const;
  void addPop(float x, float y, const char *text, uint32_t col);
  int moorLevel(int r, int c) const;

  // ---- 弾と円盤と母船と自機 (game.cpp) ----
  void enemyShot(float x, float y, float vx, float vy);
  void shootAt(float x, float y, float ang, float speed) { enemyShot(x, y, cosf(ang) * speed, sinf(ang) * speed); }
  void aimed(float x, float y, float speed) { shootAt(x, y, atan2f(py - y, px - x), speed); }
  Ufo *addUfo(int type, float x, float y, int t);
  bool spawnAbductor();
  void spawnWave();
  void stepUfo(Ufo &u);
  void damageUfo(Ufo &u, int n);
  void startMom();
  void addPuff(float x, float y, bool smoke, float vx = 0);
  void damageMom(int n);
  void stepDebris();
  float portAngle(int i) const { return mom.t * 0.01f + i * (PI / 2); }
  void stepMom();
  void killPlayer();
  void shoot(float x, float y, float vx);
  void volley();
  void think();
  void trigger();
  void control(const Input &inp);
  void resetWorld();
  void StartDemo();
  void NewGame();
  void NextDay();
  void flyIn();
  void Continue();
  void EndGame();
  int RankOf(int s) const;
  void FinishEntry();
  void StepEntry(const Input &inp);
  void UpdateSound();
  void StepWorld(const Input *inp);
  static void keep(Body &b)
  {
    b.ox = b.x;
    b.oy = b.y;
  }
  static void lerp(Body &b, float a);
  void keepAll();
  void lerpAll(float a);
  void burst(float x, float y, uint32_t rgb, int n, float speed, int life); // 爆発などの火花

  // ---- 新しい円盤と弾、戦車と高射砲と爆弾 (game_arms.cpp) ----
  static constexpr int BOMB_T = 24;          // 爆弾が地面に届くまでのフレーム数
  static constexpr float BOMB_AHEAD = 46;    // 爆弾が落ちる、自機の前の距離
  void enemyShotK(float x, float y, float vx, float vy, int kind, float w);
  void shootAtK(float x, float y, float ang, float speed, int kind, float w) { enemyShotK(x, y, cosf(ang) * speed, sinf(ang) * speed, kind, w); }
  float angTo(float x, float y) const { return atan2f(py - y, px - x); }
  static float shotHitX(int kind) { return kind == 2 ? 2 : kind == 3 ? 4 : 3; }
  static float shotHitY(int kind) { return kind == 2 ? 2.5f : kind == 3 ? 4.5f : 4; }
  bool stepEnemyShot(Shot &s);
  bool carrierOn() const;
  int pickWave();
  void spawnArms(int k);
  bool stepUfoArms(Ufo &u);
  bool beamHits(const Ufo &u) const { return u.on && u.type == LANCER && u.state == 2 && fabsf(px - u.x) < 4 && py > u.y; }
  void onUfoDestroyed(Ufo &u);
  void stepTank(Prop &p);
  void stepAAGun(Prop &p);
  void stepFlak();
  void dropBomb();
  void stepBombs();
  void autoBomb();
  void drawSaucerStyle(Gfx &g, int x, int y, float R, bool flash, int phase, uint32_t body, uint32_t dome);
  void drawUfoArms(Gfx &g, Ufo &u, int x, int y, float R);
  void drawBeam(Gfx &g, Ufo &u, int x, int y);
  void drawEnemyShot(Gfx &g, Shot &s, int x, int y, float k);
  void drawTank(Gfx &g, Prop &p, int x, int y);
  void drawAAGun(Gfx &g, Prop &p, int x, int y);
  void drawArmsClassic(Gfx &g);

  // ---- 物語 (game_story.cpp) ----
  int regionOf(int d) const { return (d - 1) % REGIONS_READY; }
  void startIntro();
  void stepIntro(const Input &inp);
  void startTravel();
  void stepEnding(const Input &inp);
  void stepTravel(const Input &inp);
  int rainAt() const;
  int rainEnd() const;
  float rainbowK() const;
  void drawRainbow(Gfx &g, int cx, int cy, int r);
  const Pal *sheepPal() const;
  uint32_t moorCol(int lv) const;
  void drawBoat(Gfx &g, Prop &p, int x, int y);
  void castleStones(Gfx &g, int x, int y, bool shadow);
  void drawCastlerigg(Gfx &g, int x, int y, Prop &p);
  bool bownessieAt(float u, float v, float e) const;
  void drawBownessie(Gfx &g, Prop &p, int x, int y);
  void drawIntroCard(Gfx &g);
  void regionScene(Gfx &g, int reg, int x0, int y0, int w, int h);
  void lakesScene(Gfx &g, int x0, int y0, int w, int h);
  void drawTravelCard(Gfx &g);

  // ---- 地方 (game_regions.cpp) ----
  void resetRegionState();
  static bool isWater(int t) { return t == WATER || t == SEA; }
  int clearCol(int lo, int hi, int lim);
  void planLandmarks();
  void placeLandmarks(float y);
  void regionTerrain(Tile *row, int n);
  void regionProps(float y);
  void drawSeasonFx(Gfx &g, int skyBottom);
  uint32_t regionTilePixel(int type, int r, int c, int i, int j, uint32_t h, float m);
  void viaductShadow(Gfx &g, int y);
  void drawViaduct(Gfx &g, int y);
  void drawAbbey(Gfx &g, int x, int y);
  void drawLighthouse(Gfx &g, int x, int y);
  void drawMine(Gfx &g, int x, int y);
  void drawWhiteHorse(Gfx &g, int x, int y);
  void drawMount(Gfx &g, int x, int y, Prop &p, bool ground, bool building);
  void drawCastleIsland(Gfx &g, int x, int y, Prop &p, bool ground, bool building);
  void lighthouseBeam(Gfx &g, int x, int y);
  void shuckEyes(Gfx &g, int ex0, int ey, int gap);
  void drawStoneBarn(Gfx &g, int x, int y);
  void yorkshireScene(Gfx &g, int x0, int y0, int w, int h);
  void cornwallScene(Gfx &g, int x0, int y0, int w, int h);
  void highlandsScene(Gfx &g, int x0, int y0, int w, int h);
  void endingScene(Gfx &g, int x0, int y0, int w, int h);
  void drawEndingCard(Gfx &g);

  // ---- 描く (game_draw.cpp) ----
  static constexpr float K = 1.5f; // 世界 → 描く絵
  static constexpr int TS = 12;    // 地面の 1 マス (描く絵の点)
  static constexpr uint32_t SHADE = 0x1A2410;
  static int S(float v) { return (int)floorf(v * K + 0.5f); }
  static uint32_t mulc(uint32_t c, float f);
  static uint32_t litc(uint32_t c, float lit) { return lit >= 0 ? mix(c, 0xFFFFFF, lit * 0.32f) : mix(c, 0x10140C, -lit * 0.45f); }
  uint32_t ph(int x, int y) const;
  Gfx *cur = nullptr; // いま描いている帯 (visible 用)
  uint32_t prof[8] = {}, profMom = 0, profShots = 0, profPS = 0, profTint = 0, rowsN = 0, rowMax = 0; // 描くところの区間ごとの時間 (確かめる用)
  bool visible(int y0, int y1) const { return cur->visible(y0, y1); }

  uint16_t *rowCache[ROWS];
  bool rowDirty[ROWS];
  void invalidateRows();
  void shiftRowCache();
  int typeAt(int r, int c) const;
  static bool wet(int t) { return t == WATER || t == SEA || t == BRIDGE || t == RAILBRIDGE || t < 0; }
  void renderRow(int r);
  int workRow = -1, workCol = 0; // 少しずつ描いている行と、次に描く列
  void renderRowStep(int cols);
  void renderTile(uint16_t *buf, int r, int c);
  uint32_t fieldPixel(int crop, int ax, int ay, uint32_t h, float m);
  uint32_t hedgePixel(const Tile &t, int i, int j, int ax, int ay, uint32_t col);
  uint32_t moorPixel(int lv, int r, int c, int i, int j, uint32_t h, float m);
  float moorE[TS], moorCrack[TS]; // 山のマスの 1 行ぶんの陰影と割れ目の雑音 (renderTile が先に計算する)
  void tileExtras(uint16_t *buf, int r, int c);
  void propShadow(Gfx &g, Prop &p);
  void drawProp(Gfx &g, Prop &p, int idx);
  void drawCottage(Gfx &g, int x, int y, bool slate, int idx);
  void drawLaundry(Gfx &g, int x, int y, int idx);
  void drawPub(Gfx &g, int x, int y, bool right);
  void drawDuck(Gfx &g, Prop &p, int x, int y);
  template <typename F> void forEachStone(int x, int y, F f);
  void upright(Gfx &g, int bx, int by, int w, int h);
  void lintel(Gfx &g, int x0, int y0, int x1, int y1);
  void drawStones(Gfx &g, int x, int y, bool shadow);
  void drawStonehenge(Gfx &g, int x, int y, Prop &p);
  void drawHengeStones(Gfx &g, int x, int y);
  float nessU[17], nessV[17], nessR[17];
  bool nessieAt(float u, float v, float e) const;
  void drawNessie(Gfx &g, Prop &p, int x, int y);
  static bool cropShape(float dx, float dy);
  void drawCropCircle(Gfx &g, Prop &p, int x, int y);
  void drawCropSaucer(Gfx &g, Prop &p, int x, int y);
  void drawBarn(Gfx &g, int x, int y);
  void drawChurch(Gfx &g, int x, int y);
  void drawTree(Gfx &g, int x, int y, int r, int idx);
  void drawWreck(Gfx &g, int x, int y);
  void drawPlane(Gfx &g, int x, int y);
  void drawSaucer(Gfx &g, int x, int y, float R, bool flash, int phase);
  void drawUfo(Gfx &g, Ufo &u);
  void drawRising(Gfx &g, Ufo &u, int x, int y, float R);
  void drawMom(Gfx &g);
  void drawMomBody(Gfx &g, int cx, int cy, float rot);
  void drawDebris(Gfx &g);
  void drawTrain(Gfx &g);
  void cloudShape(Gfx &g, int x, int y, float R, int dx, int dy, float shrink, uint32_t c);
  uint8_t lr[256], lg[256], lb[256];
  void tint(Gfx &g);
  static void glow(Gfx &g, int x, int y, uint32_t c, int size);
  void drawLights(Gfx &g);
  void txt(Gfx &g, int x, int y, uint32_t c, const char *s, int datum = Datum::TL, int size = 1);
  void banner(Gfx &g, int y, uint32_t c, const char *s, int size = 3) { txt(g, SW / 2, y, c, s, Datum::TC, size); }
  void postcard(Gfx &g, int y, int h);
  void drawTitle(Gfx &g);
  void drawPostcardScene(Gfx &g, int x0, int y0, int w, int h);
  void drawRanking(Gfx &g);
  void drawOverlay(Gfx &g);
  void drawModeCard(Gfx &g);
  void drawVolumeBars(Gfx &g, int y);
  void drawGroundRows(Gfx &g);
  void drawWorldRest(Gfx &g);
  void drawFlock(Gfx &g, int dx, int dy, uint32_t c);
  void drawSparks(Gfx &g, uint32_t bg);

  // 点の絵 (game_draw.cpp)
  static const Sprite PLANE, LIFE, SHEEP_S, COW_S, DOG_A, DOG_B, DUCK_S, DUCKLING, BEAST_A, BEAST_B, BEAST_SIT, HEART, RABBIT_S,
      SHUCK_A, SHUCK_B, SHUCK_SIT;
  static const Pal LIFE_SHADOW, COW_BROWN, COW_DARK, HERDWICK, SWALEDALE, COW_HIGHLAND, LIFE_OLIVE;
};
