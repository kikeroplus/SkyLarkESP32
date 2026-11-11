#pragma once
// 3 本目 (仮の名前 LONGSHOT): 屋上から、ホテル前の通りの標的をスナイパーとして狙う。
// 面の流れ: 指令 (標的の手がかり 3 つ) → 見渡す → スコープで探す → 撃つ。
// 弾は遠いほど落ち、風で流れ、飛ぶのに時間がかかる。外すと通りの人が逃げまどい、標的も走って逃げる。
// 遊びの流れは SKYLARK と同じ: デモ → 遊ぶ → (失敗 3 回で) コンティニューの秒読み → ゲームオーバー → 名前 → ランキング → デモ
// 操作: スティックで照準、押し込みで息を止める、SW1 で撃つ、SW2 でスコープ、SW3 短押しで音量・長押しで一時停止 (止めて SW2 でやめる)。
// デモの画面で SW3 を長押しすると、起動の選択画面へ戻る (WantMenu)
#include "game.h"
#include "g3world.h"
#include <vector>

class Game3
{
public:
  static constexpr int FPS = 30;
  bool begin(); // 景色を描いておく
  void Step(const Input &in);
  void Draw(Gfx &g); // 帯 1 本ぶん (main が帯ごとに呼ぶ)
  void skipStage();  // 確かめる用: 今の面をクリアしたことにする
  void debugStage(int n); // 確かめる用: n 番目の面へ飛ぶ
  void debugScope();
  void debugAimTarget();

  Rank Table[5] = {{30000, "EYE"}, {24000, "OWL"}, {18000, "HWK"}, {12000, "FOX"}, {6000, "CAT"}};
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr;
  bool WantMenu = false;
  Sound *Snd = nullptr;
  int VolumeLevel = 2;
  bool VolumeChanged = false;
  uint32_t buildMs = 0;
  uint32_t profBg = 0, profObj = 0, profHud = 0;

  struct Ped
  {
    float x, y, spd, ph;
    int dir;
    int st; // 0 歩く / 1 立ち止まる / 2 逃げる
    int tm;
    w3::Look lk;
    int zone = 0; // 歩く所 (シーンの walk の番号)
    bool target = false, alive = true, gone = false;
    float down = 0; // 倒れる途中 (0〜1)
    int fd = 1;
  };
  struct WinP
  {
    int idx, tm, dir; // idx はシーンの窓の番号
    bool on;
    float u;
    w3::Look lk;
  };
  struct Car
  {
    float x, spd, wheel, ph = 0;
    int lane, dir, kind;
    uint32_t color;
    int stopT = 0;       // 停留所で止まっている残り
    bool stopped = false; // この周回で停留所に止まったか
    w3::Look rider;      // 自転車に乗っている人
  };
  struct Bird
  {
    float x, y, vx, vy, ph;
    int dir, tm;
    bool fly;
  };
  struct Bullet
  {
    float x0, y0, x, y; // 撃った照準の位置、当たる位置
    int t, total;
  };
  struct Hit
  {
    float x, y;
    int t, kind; // 0 石のかけら / 1 土ぼこり / 2 火花 / 3 ガラスのひび (残る)
  };
  struct Clue
  {
    int type, v; // 0 帽子 / 1 かばん / 2 眼鏡 / 3 服の色
  };

private:
  enum class Mode
  {
    Attract,
    Brief,   // 指令 (このあいだに景色を描く)
    Play,
    Clear,   // 標的をしとめた
    Failed,  // 失敗 (残りがあれば同じ面をやりなおす)
    Continue,
    GameOver,
    NameEntry,
    Ranking,
  };
  Mode mode = Mode::Attract;
  int modeT = 0, t = 0;
  bool paused = false;
  int score = 0, lives = 3, stage = 0, continues = 0;
  int volT = 0;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  uint32_t seed = 1;
  float rnd(); // 0〜1

  // 面
  int todNow = 0;
  float windBase = 2.5f, gustAmt = 0.8f;
  Clue clues[3];
  int timeLeft = 0, panicT = 0, panicMax = 0, shotsFired = 0, ammo = 5;
  bool panic = false;
  float panicX = 0;
  int failReason = 0; // 1 違う人 / 2 逃げられた / 3 弾切れ / 4 時間切れ / 5 窓の人
  int clearT = 0;     // 標的が倒れてからの時間
  bool headShot = false;
  int bonus[5] = {};  // 面の点の内訳 (基本・頭・時間・弾・一発)

  std::vector<Ped> peds;
  std::vector<int> order; // 奥から手前へ描く順
  std::vector<WinP> wins;
  std::vector<Car> cars;
  std::vector<Bird> birds;
  std::vector<Bullet> bullets;
  std::vector<Hit> hits;

  // 照準とスコープ
  float ax = 120, ay = 210;
  bool scope = false;
  float scopeK = 0, recoil = 0, amp = 1.2f, breath = 1;
  int reload = 0, tired = 0, flash = 0, heartT = 0;
  bool holding = false;
  // デモの自動の照準
  int demoTarget = 0;

  float wind() const;
  float swayX() const, swayY() const;
  static int distAt(float y) { return w3::distAt(y); }
  int sceneNow = 0;
  float rainNow = 0;
  void newCar(Car &c, int lane);
  void startDemo();
  void newGame();
  void startBrief();
  void setupStage();
  void makeCrowd(int n);
  w3::Look randomLook();
  void startPlay();
  void fire();
  void resolve(const Bullet &b);
  void startPanic(float x);
  void fail(int reason);
  void endGame();
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();
  void stepWorld();
  void stepAim(const Input *in);
  void stepAmbience();
  void sfx(Sfx s, float vol = 1);

  void drawScene(int ltop, bool scopeView);
  void drawWide(Gfx &g, int ltop);
  void drawScope(Gfx &g, int ltop);
  void drawBrief(Gfx &g, int ltop);
  void drawOverlay(Gfx &g, int ltop);
  void drawHud(Gfx &g, int ltop);
};
