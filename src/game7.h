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
  void debugEvent(char c); // 確かめる用: P = 百鬼夜行、B = 化け狸、W = 座敷わらし、T = 玉どろぼう、K = 猫又、E = 巣を 9 個に
  bool autoPlay = false;
  int statShots = 0, statWin = 0, statStolen = 0; // 確かめる用: 打った数と払い出しの合計 (割合を見る)、玉どろぼうに取られた数

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
    uint8_t st; // 0 = 無い、1 = 転がっている、2 = 穴に落ちている、3 = 10 の穴の妖怪がくわえている (このあと吐き出す)、4 = 玉どろぼうが持って逃げている
    uint8_t hole;
    int t;    // 落ちている時間 / 止まっている時間
    float rub; // 返しゴムに当たったとき、ゴムを立てる向きへ傾ける角度 (玉ごとにでたらめ)
    uint8_t nocap; // 吐き出された直後は、この回数のあいだ穴に入らない
  };
  struct Pop // 払い出しの数が浮かぶ
  {
    float x, y;
    int v, t;
    int kind; // 0 = 数、1 = LEAF (化け穴がはずれ)、2 = SPILL! (巣から玉があふれた)
  };

  struct Yokai // 台に出てくる小さな妖怪。玉が当たるとはじく
  {
    float x, y, k; // k: 0 = 隠れている、1 = 出ている
    int st, t, hitT;
    float ph;
  };

  struct Thief // 玉どろぼう (唐草のほっかむりをしたねずみの妖怪)。転がっている玉を追いかけて取り、壁の裏へ逃げる
  {
    float x, y, vx, vy;
    int st;   // 0 = 隠れている、1 = 玉を追いかけている、2 = 玉を持って逃げている、3 = 手ぶらで帰る
    int t;    // 隠れている時間 / 追いかける残り時間 / 取ってからにやりとする時間
    int face; // 1 = 右を向いている、-1 = 左
    int exit; // 逃げこむ壁: -1 = 左、1 = 右
    int ball; // 持っている玉
    int hitT;
    float ph; // 走る足の動き
    int wt;         // 玉を持ってうろうろする残り時間
    float tx, ty;   // うろうろする行き先
    int trip, tripCool; // 釘につまずいて転んでいる時間 / 次に転べるまで
    int fearCool;       // ほかの妖怪にびくっとしたあと
  };
  struct Neko // 猫又: まれに出てきて玉どろぼうを追いかけ回す
  {
    float x, y, vx, vy;
    int st; // 0 = 隠れている、1 = 追いかけている、2 = 満足して座っている、3 = 帰る
    int t, face, side, hitT;
    float ph;
  };
  struct Nest // 玉どろぼうの巣 (左右の壁の穴)。盗んだ玉がたまり、10 個であふれて台へ戻る
  {
    int n, spill, spillT;
  };
  struct Bake // 化け狸: 1 の穴がしばらく 10 に化ける (入ると 10 か、はずれの葉っぱ)
  {
    int hole, t, puffT, puffHole;
  };
  struct Warashi // 座敷わらし: とてもまれに台の隅に座り、いるあいだ払い出しが 2 倍
  {
    float x, y, k;
    int st, t;
  };
  struct Coin // 玉どろぼうが落とした小判
  {
    float x, y, vy;
    int t;
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

  int lastYkSnd = -1;
  int spitT = -1, spitBall = 0; // 10 の穴の妖怪: 顔を出してから数えたフレーム (-1 = 出ていない) と、くわえている玉
  Yokai yk[3]; // 0 = 上の提灯おばけ、1 = 左のから傘おばけ、2 = 右の一つ目小僧
  Thief th;
  Neko neko;
  Nest nest[2]; // 0 = 左、1 = 右
  Bake bake;
  Warashi zw;
  Coin coin;
  int paradeT = -1, nextParade = 100; // 百鬼夜行: 始まってからのフレーム (-1 = なし) と、次に始まる払い出しの合計
  uint16_t *bgDark[3] = {}; // 百鬼夜行の夜: だんだん暗くした台の絵

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
  void stepYokai();
  void resetYokai();
  void stepThief();
  void stepNeko();
  void stepExtras();
  void thiefDrop(bool koban, float nx);
  void pay(float x, float y, int v);
  void addPop(float x, float y, int v, int kind);
  float yokaiFear(float x, float y, float &fx, float &fy);
  void collideSeg(Ball &o, float x0, float y0, float x1, float y1, float e, bool gate, float rot = 0);
  int rankOf(int s) const;
  void stepEntry(const Input &in);
  void finishEntry();

  void drawDynamic();
  void drawHud(Gfx &g);
  void drawOverlay(Gfx &g);
};
