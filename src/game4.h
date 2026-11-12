#pragma once
// 4 本目 (CORNERS): 四隅のポケットだけの玉突き。10 個の玉を全部落とすまでの打数を競う (少ないほどよい)。
// 操作: スティックを倒した向きの反対へ打つ (倒した量が強さ)。手を離してスティックが中央へはね戻った瞬間に打つ。
//       倒したまま SW1 を押しても打てる (打ったあとはスティックをいったん中央へ戻すまで構えない)。
//       ゆっくり中央へ戻すと打たない (やめる)。SW3 短押しで音量・長押しで一時停止 (止めて SW2 でやめる)。
// 手玉を落とすと 1 打足して、手玉は元の位置へ戻す。
// 遊びの流れ: デモ (自動で打つ) → 遊ぶ → 全部落とす → (よい記録なら) 名前 → ランキング → デモ
// デモの画面で SW3 を長押しすると、起動の選択画面へ戻る (WantMenu)
// 画面は液晶の座標 (240x320) でそのまま描く。台の絵は起動のときに PSRAM に描いておき、帯ごとに写す
#include "game.h"

class Game4
{
public:
  static constexpr int FPS = 30;
  static constexpr int NB = 11; // 玉の数 (0 が手玉、1〜10 が的玉)

  bool begin(); // 台の絵を描いておく
  void Step(const Input &in);
  void Draw(Gfx &g); // 帯 1 本ぶん (main が帯ごとに呼ぶ)
  void debugClear(); // 確かめる用: 的玉を 1 個だけ残す
  bool autoPlay = false; // 確かめる用: 遊ぶときも自動で打つ (記録には残さない)

  Rank Table[5] = {{12, "ACE"}, {15, "PRO"}, {18, "CUE"}, {22, "RAK"}, {26, "CHK"}}; // 打数 (少ないほど上)
  char LastName[4] = "AAA";
  void (*TableChanged)() = nullptr;
  bool WantMenu = false;
  Sound *Snd = nullptr;
  int VolumeLevel = 2;
  bool VolumeChanged = false;
  uint32_t profDraw = 0;

private:
  struct Ball
  {
    float x, y, vx, vy;
    bool on;    // 台の上にある (落ちる途中も含む)
    int fall;   // 0 = 台の上、1〜 = ポケットへ落ちる途中
    int pocket; // 落ちたポケット
    float fx, fy; // 落ちはじめた位置
  };
  enum class Mode
  {
    Attract,
    Play,
    Clear,
    NameEntry,
    Ranking,
  };
  enum class Phase
  {
    Aim,    // 止まっていて、狙っている
    Strike, // キューが手玉へ進む (数フレーム)
    Roll,   // 玉が動いている
  };
  Mode mode = Mode::Attract;
  Phase phase = Phase::Aim;
  int modeT = 0, t = 0;
  bool paused = false;
  int volT = 0, msgT = 0, msgKind = 0;
  int strokes = 0;
  bool scratched = false;
  bool cheated = false; // 確かめる用の操作を使った (記録には残さない)
  int potThisShot = 0;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  uint32_t seed = 1;
  int sndN = 0; // このフレームに鳴らした音の数 (ブレイクで鳴らしすぎないように)

  Ball b[NB];
  uint16_t *bg = nullptr; // 台の絵 (240x320)

  // 狙い
  bool aiming = false;
  float smx = 0, smy = 0, prevM = 0;
  float aimDx = 0, aimDy = -1, aimP = 0; // 打つ向き (手玉から見た向き) と強さ (0〜1)
  int heldAge = 0;
  bool needCenter = false; // SW1 で打ったあと、スティックが中央へ戻るのを待っている
  float shotDx = 0, shotDy = -1, shotP = 0;
  int strikeT = 0;
  // 自動で打つ
  int aiT = 0;
  float aiDx = 0, aiDy = -1, aiP = 0;

  float rnd();
  void sfx(Sfx s, float vol = 1);
  void buildTable();
  void rack();
  void respotCue();
  void startDemo();
  void newGame();
  int left() const;
  void stepTable(const Input *in);
  void stepAimInput(const Input &in);
  void stepAI();
  bool plan(float &dx, float &dy, float &p);
  void strike(float dx, float dy, float p);
  void physics();
  void endShot();
  float cast(float ox, float oy, float dx, float dy, int &hit) const;
  bool pathClear(float x0, float y0, float x1, float y1, int skipA, int skipB) const;
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();

  void drawBalls(Gfx &g);
  void drawGuide();
  void drawCue();
  void drawHud(Gfx &g);
  void drawOverlay(Gfx &g);
};
