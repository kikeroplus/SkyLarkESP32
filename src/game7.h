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
  void debugEvent(char c); // 確かめる用: P = 百鬼夜行、B = 化け狸、W = 座敷わらし、T = 玉どろぼう、K = 猫又、E = 巣を 9 個に、G = おばけホール、L = 提灯おばけ、M = 蹴鞠
  bool autoPlay = false;
  int statShots = 0, statWin = 0, statStolen = 0; // 確かめる用: 打った数と払い出しの合計 (割合を見る)、玉どろぼうに取られた数
  int statGulp = 0, statSuck = 0, statKemari = 0; // 確かめる用: 提灯おばけに飲まれた数、おばけホールに吸われた数、蹴鞠の回数

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
    uint8_t st; // 0 = 無い、1 = 転がっている、2 = 穴に落ちている、3 = 10 の穴の妖怪がくわえている (このあと吐き出す)、4 = 玉どろぼうが持って逃げている、
                // 5 = 提灯おばけが飲みこんでいる、6 = 小鬼が蹴鞠をしている、7 = おばけホールに吸いこまれている
    uint8_t hole;
    int t;    // 落ちている時間 / 止まっている時間
    float rub; // 返しゴムに当たったとき、ゴムを立てる向きへ傾ける角度 (玉ごとにでたらめ)
    uint8_t nocap; // 吐き出された直後は、この回数のあいだ穴に入らない
  };
  struct Pop // 払い出しの数が浮かぶ
  {
    float x, y;
    int v, t;
    int kind; // 0 = 数、1 = LEAF (化け穴がはずれ)、2 = SPILL! (巣から玉があふれた)、3 = GULP! (提灯おばけ)、4 = BOO! (おばけホール)、5 = KEMARI!
  };

  struct Yokai // 台に出てくる小さな妖怪。玉が当たるとはじく
  {
    float x, y, k; // k: 0 = 隠れている、1 = 出ている
    int st, t, hitT;
    float ph;
    // 提灯おばけだけ: 前を向いている / お腹がすいて光っている (このときだけ玉を飲む) / 後ろを向いている (飲めない)
    bool back, hungry;
    int moodT, turnT; // 今の様子の残りフレーム、振り向いている残りフレーム
    float glow;       // 光り具合 (0〜1)
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
  struct Ghost // おばけホール: 台の空いた所にしばらく開き、まわりの玉を吸い寄せて飲みこむ (払い出しはない)
  {
    float x, y, k; // k: 開いている量 (0〜1)
    int st, t;     // 0 = 閉じている、1 = 開く、2 = 開いている、3 = 閉じる
  };
  struct Kemari // 蹴鞠: 烏帽子の小鬼が 3 匹、転がっている玉を取って蹴りあい、飽きたら玉を置いて帰る
  {
    int st, t;        // 0 = いない、1 = どろんと出てくる、2 = 蹴りあっている、3 = 最後の蹴りで真ん中へ、4 = 帰っていく
    int ball, holder; // 蹴っている玉と、いま玉を持っている小鬼 (-1 = 玉は宙を真ん中へ)
    int kicks;        // 残りの蹴る回数
    int fly, flyN;    // 玉が宙を飛んでいるフレームと、着くまでのフレーム
    float fx, fy, tx, ty, hop; // 飛んでいる玉の出た所・着く所・高さ
    float cx, cy;     // 輪の真ん中
    float x[3], y[3], hx[3], hy[3]; // 小鬼の場所と、輪の中の持ち場
    int face[3], kickT[3], hitT[3];
    float ph;
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
  Ghost gh;
  Kemari km;
  int gulpT = 0, gulpBall = 0; // 提灯おばけが玉を飲みこんでからのフレーム (0 = 飲んでいない) と、その玉
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
  void stepLantern(Yokai &y);
  void resetYokai();
  void stepThief();
  void stepNeko();
  void stepExtras();
  void stepGhost();
  void stepKemari();
  void kemariKick(int from, int to, float hop, int n);
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
