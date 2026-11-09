#pragma once
// 2 本目 (仮の名前 FOUNDRY): 捨てられた工場を、六本脚の戦車が上へ登っていく縦シューティング。
// 敵は、工業機械に乗り移った AI。味方の弾は青、敵の目と弾は赤。
// 背景は g2bg (起動時に PSRAM に描いておく)、描く部品は g2draw (液晶の座標でそのまま描く)。
// 遊びの流れは SKYLARK と同じ: デモ (タイトル) → 遊ぶ → 残機がなくなったらコンティニューの秒読み → ゲームオーバー → 名前 → ランキング → デモ
// 操作: スティックで歩く (押し込むとゆっくり)、SW1 で撃つ、SW3 長押しで一時停止 (止めているあいだ SW2 でやめる)。
// デモの画面で SW3 を長押しすると、起動の選択画面へ戻る (WantMenu)
#include "game.h"
#include <vector>

class Game2
{
public:
  static constexpr int FPS = 30;
  bool begin(); // 背景を描いておく (1〜2 秒かかる)
  void Step(const Input &in);
  void Draw(Gfx &g); // 帯 1 本ぶん (main が帯ごとに呼ぶ)
  void skipZone();   // 確かめる用: 次の区画の始まりへ進める
  unsigned enemyCount() const { return en.size(); }
  bool autoPilot = false; // 確かめる用: 遊んでいるあいだも自動で歩いて撃ち、やられない

  Rank Table[5] = {{50000, "IRN"}, {40000, "LEG"}, {30000, "RST"}, {20000, "FOU"}, {10000, "NDR"}};
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr; // ランキングが変わったら呼ぶ (main が保存する)
  bool WantMenu = false;            // 選択画面へ戻りたい
  Sound *Snd = nullptr;
  int VolumeLevel = 2;        // SW3 を短く押すたびに 1 段上げる (0 = OFF〜4)。SKYLARK と同じ設定 (vol5)
  bool VolumeChanged = false; // main が音量を当てて保存する
  void sfx(Sfx s, float vol = 1);

  uint32_t buildMs = 0;
  uint32_t profBg = 0, profAnim = 0, profObj = 0, profFore = 0, profBoss = 0, profEn = 0, profHud = 0; // 描く時間の内訳 (確かめる用。us の合計)

  struct Shot
  {
    float x, y, dx, dy;
    bool dead;
    int k = 0;    // 敵の弾: 0 赤いひし形 / 1 溶接の火花 (ジグザグ) / 3 蒸気 (近くだけ)
    int life = 0; // 0 でなければ、この数のフレームで消える
  };
  // 敵の種類 (図鑑の順)。空の敵 (TWIN・MAINT・CORE) は床と一緒に流れない。ほかは床に乗っていて、gy に床の位置を持つ
  enum EK
  {
    TWIN,   // ツインファン: 2 つの羽根で浮いて 3 方向に撃つ。弱ると片方の羽根が止まり、端へ逃げる
    WALKER, // ウォーカー: 歩いて止まって 2 発ずつ撃つ。弱ると端へ歩いて逃げる
    BUG,    // ボルトムシ: 群れで這って寄ってくる。群れが半分になると散る
    WELDER, // ウェルダー: 床に固定。溶接の火花を飛ばす。続けて撃たれると腕をたたんで隠れる (弾が効かない)
    FORK,   // フォーク: ふかしてから突進する。弱ると木箱を盾にして下がる
    SNAKE,  // パイプヘビ: 配管の口から出て別の口へ潜る。節を壊すと頭だけで逃げる
    EYE,    // アイ: 壁の防犯カメラ。撃ってはこない。戦車を見つけると仲間を呼ぶ (ツインファンが来る)
    SCRAP,  // ダンゴ: 鉄くずを吸って大きくなる。大きくなると破片を飛ばす
    MAINT,  // メンテ: 傷ついた仲間を直す。狙われると逃げる
    CORE,   // 核: 壊した機械から出て、近くの機械へ逃げる。逃がすとその機械が強くなる
  };
  struct Enemy
  {
    int k, id, lvl = 0, st = 0; // lvl: 核が乗り移って強くなった段 (0〜3)。st: 動きの段階
    float x = 0, y = 0;         // 画面の位置
    float gx = 0, gy = 0;       // 床の敵は床の位置 (y = gy + scroll)。空の敵は動きの元の位置
    float vx = 0, vy = 0, a = 0, r = 0;
    float ux = 0, uy = 0;       // パイプヘビ: 潜る口 (床の位置)
    int hp = 1, maxhp = 1, t0 = 0, hit = 0, tm = 0, cnt = 0, grp = 0, link = -1;
    int virus = 0;              // 爆弾のウイルスで止まっている残りのフレーム (動かず、撃たず、触れてもやられない)
    uint8_t seg[8] = {};        // パイプヘビの節の耐久
    bool gone = false, hidden = false, alerted = false;
  };
  struct Fx
  {
    int k; // 0 爆発 / 2 戦車の大きな爆発 / 3 弾をはじいた火花 / 4 核が乗り移った輪 / 5 小さい爆発
    float x, y, s;
    int t, life;
    bool ground;
  };

  struct Boss
  {
    int k = -1;                          // -1 はいない
    int t = 0, st = 0, tm = 0, stun = 0; // stun: 爆弾のウイルスで止まっている残り
    int enterT = 0, dieT = 0, heals = 0, fireIdx = 0;
    float x = 0, y = 0, u = 0, elev = 0, ty = 0;
    int hp[9] = {}, maxhp[9] = {}, hit[9] = {}; // 部位ごとの耐久 (0 がいちばん大事な部位)
    float hookY = 0, hookOff = 0;
    float ringX = 0, ringY = 0, ringR = 0;
    bool ringOn = false, escaped = false;
  };
private:
  enum class Mode
  {
    Attract, // デモ (自動で歩いて撃つ。タイトルとランキングを交互に出す)
    Play,
    Continue, // 残機がなくなった。数えているあいだに SW1 で続ける
    GameOver,
    NameEntry,
    Ranking, // 遊び終わったあとのランキング
  };
  struct Foot
  {
    float x, y, sx, sy, p; // p: 足を上げている途中 (0〜1)。-1 は床に着いている
  };
  Mode mode = Mode::Attract;
  int modeT = 0;
  bool paused = false;
  float scroll = 0, tx = 120, ty = 250, ang = -1.5708f, rec = 0;
  int t = 0, fireCool = 0;
  int score = 0, lives = 2, nextExtend = 40000, continues = 0, extendT = 0;
  bool tankOn = true;         // 戦車が画面にいる (やられているあいだと遊び終わったあとは false)
  int deadT = 0, invT = 0, enterT = 0;
  int bannerT = 0, bannerZone = -1, lastZone = -1;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  Foot feet[6];
  std::vector<Shot> shots, eb;
  std::vector<Enemy> en, born; // born: このフレームに生まれた敵 (あとで en に足す)
  std::vector<Fx> fx;
  // 弾を強くする部品 (青く光るエネルギーの電池)。拾うと shotLv が上がる (0〜3)。やられると 1 段下がる
  struct Item
  {
    float x, y;
    int t;
  };
  std::vector<Item> items;
  int shotLv = 0, killCount = 0, powT = 0;
  void dropItem(float x, float y);
  // 爆弾 (SW2): 砲塔の向きの少し先へ放り投げ、落ちたところで爆発して、まわりの敵に少し傷を負わせる
  bool bombOn = false;
  float bombX0 = 0, bombG0 = 0, bombX1 = 0, bombG1 = 0; // 投げた所と落ちる所 (y は床の位置)
  int bombT = 0, bombCool = 0;
  void bombBlast(float x, float y);

  // ボス (game2_boss.cpp)。区画の決まった所で 1 体ずつ出る。出ているあいだは流れが止まる
  // 0 ガントリー (沈殿槽の天井クレーン) / 1 スタンパー (止まったプレス機) / 2 炉心 (溶鉱炉の跡、最後のボス)
  Boss boss;
  float spd = 0.55f;   // 今の流れる速さ
  int volT = 0;        // 音量の目盛りを出す残り
  int pileT = 40, clankT = 90; // 背景のくい打ち・大きな機械が次に鳴るまで
  void updateAmbience(); // 鳴り続ける音 (工場のうなり・溶鉱炉・小鳥・ボスのうなり)
  int bossBannerT = 0; // 「WARNING」や「倒した」を出す残り
  int bossResult = 0;  // 0 なし / 1 倒した / 2 逃げられた
  void startBoss(int k);
  void bossStep();
  bool bossShot(float x, float y); // 自分の弾が当たったら true
  void bossBomb(float x, float y);
  void bossHurt(int part, int dmg);
  void bossDraw(int ltop, bool air);
  float bossEyeX() const, bossEyeY() const;

  void resetWorld();
  void startDemo();
  void newGame();
  void enterTank(); // 画面の下から歩いて入ってくる
  void killTank();
  void endGame();
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();
  void stepWorld(const Input *in); // in が nullptr なら戦車は動かさない
  void stepTank(float ax, float ay, bool fire, bool slow);
  void initFeet();
  // 敵 (game2_enemy.cpp)
  int evIdx = 0, lap = 0, nextId = 1, nextGrp = 1, pendingLvl = 0;
  float diff = 0; // 難しさ (0〜1)。序盤ほど小さい
  void resetSpawns(bool skipVisible);
  void spawnEvents();
  Enemy &addEnemy(int k, float x, float y);
  void spawn(int k, int x, int n, float gyTop);
  void enemyStep();
  void stepOne(Enemy &e);
  void hitEnemy(Enemy &e, int part, float by);
  void killEnemy(Enemy &e);
  Enemy *findId(int id);
  bool touchesTank(const Enemy &e) const;
  void drawEnemies(int ltop, bool air);
  int zoneAt(int sy) const;
  void drawOverlay(Gfx &g, int ltop);
};
