#pragma once
// 6 本目 (JUKEBOX): アメリカの田舎のダイナーにありそうな、60〜70 年代ふうのピンボール。ボール 3 個で点数を競う。
// 操作は ORBIT (5 本目) と同じ: SW1 = 左のフリッパー、SW2 = 右のフリッパー (下と真ん中の 2 組がいっしょに動く)。
//       スティックを後ろ (手前) に引いて離すと打ち出す。前へ押すと台を揺らす (押しすぎると TILT)。
//       SW3 短押しで音量・長押しで一時停止 (止めて SW2 でやめる)。
// 台: 上のレーン 3 本 (1・2・3)、左右の壁の的 (4・5)。1〜5 を全部点けるとインレーンに SPECIAL が点き、通るとエクストラボール。
//     ポップバンパー 3 個、キックアウトの穴 (ダブルボーナスを点ける)、ロールオーバーのボタン 4 個。
//     点数が入るたびに、古い台のようにチャイムが鳴る (10・100・1000)。点数は回るリールの窓に出す。
// 遊びの流れ: デモ (自動で遊ぶ) → 遊ぶ → ゲームオーバー → (よい記録なら) 名前 → ランキング → デモ
// 画面は液晶の座標 (240x320) でそのまま描く。台の絵は起動のときに PSRAM に描いておき、帯ごとに写す
#include "game.h"

class Game6
{
public:
  static constexpr int FPS = 30;
  static constexpr int NF = 4; // フリッパー (0, 1 = 下の左右、2, 3 = 真ん中の左右)

  bool begin();
  void Step(const Input &in);
  void Draw(Gfx &g);
  void debugDrain(); // 確かめる用: いまのボールを落とす
  bool autoPlay = false;

  Rank Table[5] = {{60000, "MEL"}, {40000, "FLO"}, {25000, "VIC"}, {15000, "JOE"}, {8000, "SUE"}};
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr;
  bool WantMenu = false;
  Sound *Snd = nullptr;
  int VolumeLevel = 2;
  bool VolumeChanged = false;
  uint32_t profDraw = 0;

private:
  enum class Mode
  {
    Attract,
    Play,
    Over,
    NameEntry,
    Ranking,
  };
  enum class BallSt
  {
    Plunger, // 打ち出す所 (打ち出してレーンを上っている間も)
    Live,
    Hold,  // キックアウトの穴に入っている
    Bonus, // 落ちた。ボーナスを数えている
  };
  struct Flip
  {
    float a, prevA;
    bool up;
  };

  Mode mode = Mode::Attract;
  BallSt st = BallSt::Plunger;
  int modeT = 0, t = 0;
  bool paused = false;
  int volT = 0;
  uint32_t seed = 1;
  int sndN = 0;

  float bx = 0, by = 0, bvx = 0, bvy = 0;
  int slowT = 0, holdT = 0, holeCool = 0;
  Flip fl[NF];
  // 打ち出し
  float plP = 0, plMax = 0, prevPull = 0;
  int plAge = 0, plShootT = 0;
  // 揺らす
  bool fwdWas = false;
  float tiltW = 0;
  bool tilted = false;
  int shakeT = 0;

  // 点数と台の状態
  int score = 0, ballNo = 1, extra = 0, bonus = 0, mult = 1;
  float reel[7] = {0, 0, 0, 0, 0, 0, 0}; // リールの見えている位置 (回って追いつく)
  int bonusTotal = 0, bonusShown = 0, bonusT = 0, bonusDone = 0;
  bool num[5] = {false, false, false, false, false}; // 1〜5
  int numFlashT = 0;
  bool special = false;
  int bumpT[3] = {0, 0, 0}, slingT[2] = {0, 0}, btnT[4] = {0, 0, 0, 0}, tgtT[2] = {0, 0};
  int msgT = 0, msgKind = 0, msgVal = 0;
  bool cheated = false;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  // 自動で遊ぶ
  int aiT = 0, aiFlipT[2] = {0, 0};
  float aiP = 0.9f;

  uint16_t *bg = nullptr;

  float rnd();
  void sfx(Sfx s, float vol = 1);
  void buildTable();
  void startDemo();
  void newGame();
  void serve();
  void add(int pts, int bon);
  void lightNum(int i);
  void message(int kind, int val = 0, int frames = 2 * FPS);
  void stepBall(const Input *in);
  void stepPlunger(float pull);
  void stepAI(bool &l, bool &r, float &pull);
  void physics();
  void collideSeg(float x0, float y0, float x1, float y1, float e, int kind);
  void collideFlipper(int k, float dt);
  void sensors(float px, float py);
  void lost();
  void stepBonus();
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();

  void drawDynamic(Gfx &g);
  void drawHud(Gfx &g);
  void drawOverlay(Gfx &g);
};
