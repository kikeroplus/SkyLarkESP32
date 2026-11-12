#pragma once
// 7 本目 (SMARTBALL): 温泉街や縁日の古い店にあるスマートボール。色あせて、さびた台。
// 遊び方: 持ち玉 30 個から始める。スティックを後ろ (手前) に引いて離すと、引いた量の強さで玉を打ち上げる (ゆっくり戻すと打たない)。
//         玉は釘のあいだを落ちていき、穴に入ると書いてある数だけ玉が出てくる (払い出し)。入らなかった玉は下の樋へ落ちて終わり。
//         打った玉が打ち出す所を離れると、次の玉がすぐ転がってくる (何個も続けて打てる)。持ち玉がなくなり、台の玉もなくなったら終わり。
//         SW1 / SW2 で台を左 / 右へそっと揺らす。揺らしすぎると、しばらく打てない。
//         SW3 短押しで音量・長押しで一時停止 (止めて SW2 でやめる)。
// 記録: 払い出しの合計 (WIN)。ランキングは多い順
// 画面は液晶の座標 (240x320) でそのまま描く。台の絵 (釘・穴・色あせ) は起動のときに PSRAM に描いておき、帯ごとに写す
#include "game.h"

class Game7
{
public:
  static constexpr int FPS = 30;
  static constexpr int NB = 8;        // 台の上に同時に出せる玉
  static constexpr int START_BALLS = 30;

  bool begin();
  void Step(const Input &in);
  void Draw(Gfx &g);
  void debugEnd(); // 確かめる用: 持ち玉を 0 にする
  bool autoPlay = false;
  int statShots = 0, statWin = 0; // 確かめる用: 打った数と払い出しの合計 (割合を見る)

  Rank Table[5] = {{220, "UME"}, {170, "MAT"}, {130, "TAK"}, {90, "KAM"}, {50, "SAK"}};
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
  struct Ball
  {
    float x, y, vx, vy;
    uint8_t st; // 0 = 無い、1 = 転がっている、2 = 穴に落ちている
    uint8_t hole;
    int t;    // 落ちている時間 / 止まっている時間
  };
  struct Pop // 払い出しの数が浮かぶ
  {
    float x, y;
    int v, t;
  };

  Mode mode = Mode::Attract;
  int modeT = 0, t = 0;
  bool paused = false;
  int volT = 0;
  uint32_t seed = 1;
  int sndN = 0, nailN = 0;

  Ball b[NB];
  Pop pops[6];
  int pool = START_BALLS, win = 0;
  int feedT = 0;
  // 打ち出し
  float plP = 0, plMax = 0, prevPull = 0;
  int plAge = 0, plShootT = 0;
  // 揺らす
  float tiltW = 0;
  int lockT = 0, shakeT = 0, shakeDir = 0;
  bool cheated = false;
  char entry[4] = "AAA";
  int entryPos = 0, entryRepeat = 0, newRank = -1;
  // 自動で遊ぶ
  int aiT = 0, aiNudgeT = 0;
  float aiP = 0.7f;

  uint16_t *bg = nullptr;

  float rnd();
  void sfx(Sfx s, float vol = 1);
  void buildTable();
  void startDemo();
  void newGame();
  bool laneBusy() const;
  void stepBoard(const Input *in);
  void stepPlunger(float pull);
  void nudge(int dir);
  void physics();
  void collideSeg(Ball &o, float x0, float y0, float x1, float y1, float e, bool gate);
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();

  void drawDynamic();
  void drawHud(Gfx &g);
  void drawOverlay(Gfx &g);
};
