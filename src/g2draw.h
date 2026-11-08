#pragma once
// 2 本目 (廃工場) の描く部品。液晶の座標 (240x320) でそのまま描く。色は 0xRRGGBB、a は不透明さ (0〜255。255 で上書き)。
// 描く先 (Surf) は、液晶の帯 (64 行) でも、PSRAM に取っておく背景の絵でも同じように扱う。
// 形の描き方はデザイン案のページ (HTML) と同じにしてある
#include <Arduino.h>
#include <vector>

struct Surf
{
  uint16_t *buf = nullptr; // 行 0 が座標 y0 の行
  int w = 240, y0 = 0, h = 0;
  int ox = 0, oy = 0;                         // 描くときに足すずらし
  int cx0 = 0, cy0 = -100000, cx1 = 239, cy1 = 100000; // 描ける範囲 (ずらしたあとの座標。端も含む)
  uint16_t key = 0;                           // 0 でなければ抜き色 (この色は塗らない)
  Surf *alt = nullptr;                        // 半透明の描き込みはこちらへ回す (手前の絵の影を床に落とすため)
};

struct Pt
{
  float x, y;
};
typedef std::vector<Pt> Pts;

namespace d2
{
extern Surf *S;
const float PIf = 3.14159265f;

uint16_t c565(uint32_t c);
uint16_t blend(uint16_t v, uint32_t c, int a);
void span(int x0, int x1, int y, uint32_t c, int a = 255);
void R(float x, float y, float w, float h, uint32_t c, int a = 255);
void P(float x, float y, uint32_t c, int a = 255);
void L(float x0, float y0, float x1, float y1, uint32_t c, int a = 255);
void LT(float x0, float y0, float x1, float y1, int t, uint32_t c, int a = 255);
void C(float cx, float cy, float r, uint32_t c, int a = 255);
void CR(float cx, float cy, float r, uint32_t c, int a = 255);
void poly(const Pt *p, int n, uint32_t c, int a = 255); // 凸な多角形
void T(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c, int a = 255);

float hs(float n);          // 0〜1 の決まった乱数
float h2(float x, float y); // 2 つの値から
float vn(float x, float y); // なめらかなノイズ (0〜1)

Pts pathPts(const Pts &segs, float step = 1.5f);
void arc(Pts &out, float cx, float cy, float r, float a0, float a1, int n = 24);
} // namespace d2
