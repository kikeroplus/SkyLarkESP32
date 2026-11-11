#pragma once
// 3 本目 (仮の名前 LONGSHOT) の景色と、景色の中で動くもの (人・車・鳩・旗・木) の描き方。
// 世界は 240x300 (見渡す画面の点)。屋上などの高い所から、向かいの建物と前の通りを見下ろす。
// ・動かない景色は 4 倍 (960x1200) で PSRAM に描いておく。スコープ (4 倍) の中はそれを写すだけ。
//   見渡す画面は、それを 4x4 の平均で縮めた絵 (240x300) を写す (縮めるので輪郭がなめらかになる)
// ・人や車は毎フレーム図形から描く。スコープでは 4 倍で直接、見渡す画面では 4 倍の小さな下書きに描いて縮めて重ねる
// ・昼・夕方・夜は、昼の色で描いて色を変換する (光る物の色は EMIT を付けて変換しない)
// ・シーン (ホテル前・駅前・港・雨の繁華街・公園) ごとに、動かない絵と、人の歩く所・車線・窓・明かり・木を Scene に持つ
#include "g2draw.h"

namespace w3
{
const int WW = 240, WH = 300, Z = 4, HW = WW * Z, HH = WH * Z;
enum Tod
{
  DAY,
  DUSK,
  NIGHT
};
extern int tod;
extern uint16_t *hi, *lo; // 4 倍の景色 (HW x HH)、縮めた景色 (WW x WH)
bool alloc();
void buildBegin(int scene, int tod);
bool buildStep(); // 少しずつ描く。できあがったら true
float buildProgress();
inline void buildAll(int sc, int t)
{
  buildBegin(sc, t);
  while (!buildStep())
  {
  }
}

// ---- 描く座標: 描く先の点 = (世界の座標 - O) * K ----
extern float K, OX, OY;
extern float lit; // 夜に、明かりの下にいる人を明るく描く (0〜1)
const uint32_t EMIT = 0x1000000; // 色に付けると、昼夜の色の変換をしない (光っている物)
uint32_t col(uint32_t c);
uint32_t rgb(int r, int g, int b);
uint32_t mix(uint32_t a, uint32_t b, float t);
uint32_t shade(uint32_t c, float k);
void R(float x, float y, float w, float h, uint32_t c, int a = 255);
void C(float x, float y, float r, uint32_t c, int a = 255);
void E(float x, float y, float rx, float ry, uint32_t c, int a = 255);
void L(float x0, float y0, float x1, float y1, float w, uint32_t c, int a = 255);
void T(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c, int a = 255);
void Q(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t c, int a = 255);
void ring(float x, float y, float r, float w, uint32_t c, int a = 255);
// 動かない景色を描くときだけ使うもの
void tex(float x, float y, float w, float h, uint32_t base, float amt, int cell, float seed, float fine = 0.05f);
void sign(float x, float y, float cell, const char *s, uint32_t c, int a = 255); // 5x7 の看板の文字 (A-Z 0-9)
float signW(const char *s, float cell);
void sky(float bottom = 48); // 空を bottom まで塗る (雲・夕日・月と星も)
void skyline(float bottom);
void lampPost(float x, float baseY, float h);
float hh(float a, float b = 0, float c = 0); // 決まった乱数 (0〜1)

// 見渡す画面用: 世界の (x0, y0, w, h) を 4 倍の下書きに描き (miniBegin〜miniEnd のあいだに描く)、縮めて今の描く先へ重ねる
void miniBegin(float x0, float y0, float w, float h);
void miniEnd();

// ---- 人の見た目 ----
enum Hat : uint8_t
{
  H_NONE,
  H_RED,
  H_NAVY,
  H_BLACK,
  H_BEIGE,
  H_WHITE,
  H_GREEN,
  H_N
};
enum Bag : uint8_t
{
  B_NONE,
  B_BLACK,
  B_BROWN,
  B_RED,
  B_N
};
enum Coat : uint8_t
{
  CO_GREY,
  CO_NAVY,
  CO_GREEN,
  CO_YELLOW,
  CO_WHITE,
  CO_BROWN,
  CO_BLUE,
  CO_MAROON,
  CO_N,          // ここまでが本当の服の色 (くじで選ぶ)
  CO_SIL = CO_N, // 指令の画面の、分からない服 (影の色)
};
extern const uint32_t HAT_C[H_N], BAG_C[B_N], COAT_C[CO_N + 1];
extern const char *const HAT_NAME[H_N], *const BAG_NAME[B_N], *const COAT_NAME[CO_N];

struct Look
{
  uint8_t hat = H_NONE, bag = B_NONE, coat = CO_GREY;
  bool glasses = false, skirt = false, longCoat = false;
  uint8_t hair = 0, hairKind = 0, skin = 0, pants = 0; // 色の番号
  uint8_t umbrella = 0;                                 // 傘 (0 = なし、1〜 色)
};

// 人を描く。足元が (x, y)。dir は向き (1 = 右)。ph は歩く足の位相。walk は歩いているか。
// down が 0 より大きいと倒れている途中 (1 で倒れきる)。倒れる向きは fallDir。shadow は足元の影を描くか
void drawPerson(float x, float y, int dir, float ph, bool walk, const Look &lk, float down, int fallDir, bool shadow = true);
// 窓の人 (胸から上)。窓の四角 (x, y, w, h) の中の位置 u (0〜1)、向き dir
void drawWinPerson(float x, float y, float w, float h, float u, int dir, const Look &lk, bool litWin);
inline float winPersonX(float x, float w, float u) { return x + w * 0.27f + u * w * 0.46f; }
float lightAt(float x, float y); // 夜に、その場所がどれだけ明かりに照らされているか (0〜1)

enum CarKind : uint8_t
{
  CAR_SEDAN,
  CAR_TAXI,
  CAR_VAN,
  CAR_HATCH,
  CAR_BUS,   // 駅前の路線バス (停留所で止まる)
  CAR_TRUCK, // 港のトラック
  CAR_BIKE,  // 公園の自転車 (乗っている人は人として当たる)
  CAR_N
};
float carLen(int kind);
float carTop(int kind); // 車線の y から見た車の上端
void drawCar(float x, float y, int dir, int kind, uint32_t color, float wheel, const Look *rider = nullptr, float ph = 0);
void drawPigeon(float x, float y, int dir, bool fly, float ph, bool gull);
void drawFlag(float px, float py, float len, float wind, int t, uint32_t c1, uint32_t c2, float seed);
void drawTree(float x, float cy, float s, float wind, int t, int i);

// ---- シーン ----
struct Box
{
  float x, y, w, h;
};
struct Lamp
{
  float x, y, r; // 照らす真ん中 (足元) と半径
};
struct Scene
{
  const char *name;           // 指令の画面の場所の名前
  void (*draw)(int r0, int r1); // 動かない絵 (4 倍の行 r0〜r1 を描くときに呼ばれる)
  void (*dyn)(int layer, float wind, int t); // 動く景色: 0 = 人より奥、1 = 奥の歩道の人のあと、2 = いちばん手前 (雨など)
  int nWalk;
  Box walk[3]; // 人が歩く所 (足元の y が y〜y+h)
  int nLane;
  float laneY[2];            // 車線 (車の基準の y。車輪はおよそ +9)
  int laneDir[2];
  float laneSpd[2];          // 点/秒
  uint8_t laneKinds[2];      // 走る車の種類 (ビットの組)
  float laneStop[2];         // 止まる所の x (バスの停留所。-1 はなし)
  int nWin;
  const Box *win;            // 人が顔を出す窓 (撃つとガラスが割れる)
  const uint8_t *winLitTab;  // 夜に明かりがついている窓 (1)
  int nLamp;
  Lamp lamp[6];
  int nBright;
  Box bright[4];             // 店の明かりが落ちている所 (夜に人が明るく見える)
  int nTree;
  float tree[4][3];          // 木の x、葉の真ん中の y、大きさ
  Box birdZone[2];           // 鳩 (港ではカモメ) のいる所
  bool gulls;
  float groundY;             // これより上に当たった弾は壁 (石のかけら)
  float d0, k1, yb, k2;      // 距離: y が yb より下は d0 + (300 - y) * k1、上は さらに (yb - y) * k2
  bool rain;
};
const int NSCENE = 5;
enum SceneId
{
  SC_HOTEL,
  SC_STATION,
  SC_HARBOR,
  SC_RAIN,
  SC_PARK
};
extern const Scene *const SCENES[NSCENE];
extern int sceneNo;
inline const Scene &SC() { return *SCENES[sceneNo]; }
inline int distAt(float y)
{
  const Scene &s = SC();
  float d = y > s.yb ? s.d0 + (300 - y) * s.k1 : s.d0 + (300 - s.yb) * s.k1 + (s.yb - y) * s.k2;
  return (int)lroundf(d);
}
extern float rainAmt; // 雨の強さ (0〜1)
void drawRain(int t, float wind, float amt);
} // namespace w3
