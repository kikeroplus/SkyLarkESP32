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

  Rank Table[5] = {{50000, "IRN"}, {40000, "LEG"}, {30000, "RST"}, {20000, "FOU"}, {10000, "NDR"}};
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr; // ランキングが変わったら呼ぶ (main が保存する)
  bool WantMenu = false;            // 選択画面へ戻りたい

  uint32_t buildMs = 0;
  uint32_t profBg = 0, profAnim = 0, profObj = 0, profFore = 0; // 描く時間の内訳 (確かめる用。us の合計)

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
  struct Shot
  {
    float x, y, dx, dy;
    bool dead;
  };
  struct Enemy
  {
    int k; // 0 ツインファン (空) / 1 ウォーカー (地上)
    float bx, x, y, wy;
    int hp, t0, hit;
    bool walking;
  };
  struct Fx
  {
    int k; // 0 爆発 / 1 逃げる核 / 2 戦車の大きな爆発
    float x, y, s;
    int t, life;
    bool ground;
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
  std::vector<Enemy> en;
  std::vector<Fx> fx;

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
  void enemyStep();
  int zoneAt(int sy) const;
  void drawOverlay(Gfx &g, int ltop);
};
