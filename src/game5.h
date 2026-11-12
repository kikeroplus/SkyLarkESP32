#pragma once
// 5 本目 (ORBIT): 宇宙のピンボール。ボール 3 個で点数を競う。
// 操作: SW1 = 左のフリッパー、SW2 = 右のフリッパー。
//       スティックを後ろ (手前) に引いて離すと、引いた量の強さでボールを打ち出す (ゆっくり戻すと打たない)。
//       ボールが台の上にあるときにスティックを前へ押すと台を揺らす (押しすぎると TILT)。
//       SW3 短押しで音量・長押しで一時停止 (止めて SW2 でやめる)。
// 台: 上のレーン 3 本 (全部点けると倍率が上がる。フリッパーで点いているレーンを左右へずらせる)、
//     惑星のバンパー 3 個、スリングショット 2 個、左の彗星ターゲット 3 枚 (全部倒すとジャックポットが上がる)、
//     右のブラックホール (入るとジャックポット)。打ち出してしばらくはボールセーブ。
// 遊びの流れ: デモ (自動で遊ぶ) → 遊ぶ → ゲームオーバー → (よい記録なら) 名前 → ランキング → デモ
// デモの画面で SW3 を長押しすると、起動の選択画面へ戻る (WantMenu)
// 画面は液晶の座標 (240x320) でそのまま描く。台の絵は起動のときに PSRAM に描いておき、帯ごとに写す
#include "game.h"

class Game5
{
public:
  static constexpr int FPS = 30;

  bool begin(); // 台の絵を描いておく
  void Step(const Input &in);
  void Draw(Gfx &g); // 帯 1 本ぶん (main が帯ごとに呼ぶ)
  void debugDrain(); // 確かめる用: いまのボールを落とす
  bool autoPlay = false; // 確かめる用: 遊ぶときも自動で打つ (記録には残さない)

  Rank Table[5] = {{40000, "ORB"}, {25000, "SUN"}, {15000, "MAR"}, {8000, "VEN"}, {4000, "LUN"}};
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
    Over, // ゲームオーバーの表示
    NameEntry,
    Ranking,
  };
  enum class BallSt
  {
    Plunger, // 打ち出す所で待っている (打ち出したあと、レーンを上っている間も含む)
    Live,    // 台の上
    Hold,    // ブラックホールにつかまっている
    Bonus,   // 落ちた。ボーナスを数えている
  };
  struct Flip
  {
    float a, prevA; // 向き (画面の角度。ラジアン)
    bool up;
  };

  Mode mode = Mode::Attract;
  BallSt st = BallSt::Plunger;
  int modeT = 0, t = 0;
  bool paused = false;
  int volT = 0;
  uint32_t seed = 1;
  int sndN = 0;

  // ボール
  float bx = 0, by = 0, bvx = 0, bvy = 0;
  int slowT = 0;     // ほとんど動いていないフレーム数 (引っかかったら少し押す)
  int holdT = 0;     // ブラックホールにつかまっている時間
  int holeCool = 0;  // はき出したあと、しばらくはつかまえない
  int saveT = 0;     // ボールセーブの残り
  bool saveUsed = false; // このボールでボールセーブを使った
  // フリッパー
  Flip fl[2];
  // 打ち出し
  float plP = 0;     // いまの引き (絵)
  float plMax = 0;   // 引いた量 (覚えておく)
  float prevPull = 0;
  int plAge = 0;
  int plShootT = 0;  // 打ち出しの絵 (戻っていく)
  // 揺らす
  bool fwdWas = false;
  float tiltW = 0;
  bool tilted = false;
  int shakeT = 0;

  // 点数と台の状態
  int score = 0, ballNo = 1, mult = 1, bonus = 0, jackpot = 2000;
  int bonusTotal = 0, bonusShown = 0, bonusT = 0, bonusDone = 0;
  bool lane[3] = {false, false, false};
  int laneFlashT = 0;
  bool drop[3] = {true, true, true}; // 立っている
  int dropResetT = 0;
  int bumpT[3] = {0, 0, 0}, slingT[2] = {0, 0};
  int msgT = 0, msgKind = 0, msgVal = 0;
  bool cheated = false;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  // 自動で遊ぶ
  int aiT = 0, aiFlipT[2] = {0, 0};
  float aiP = 0.9f;

  uint16_t *bg = nullptr; // 台の絵 (240x320)

  float rnd();
  void sfx(Sfx s, float vol = 1);
  void buildTable();
  void startDemo();
  void newGame();
  void serve();
  void add(int pts, int bon);
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
