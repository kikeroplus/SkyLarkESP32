#pragma once
// 2 本目 (廃工場) の背景。下から順に 8 つの区画を積む。動かない部分は起動時に一度だけ PSRAM に描いておき
// (下の絵と、戦車より手前に出す絵の 2 枚)、毎フレームはそれを写して、動くもの (anim) だけを描き足す
#include "g2draw.h"

namespace bg2
{
struct Zone
{
  const char *name;
  int h;
  void (*stat)(int h);        // 動かない部分 (下の絵)
  void (*anim)(int h, int t); // 毎フレーム描き足すもの
  void (*fore)(int h);        // 戦車より手前の絵 (頭上の配管や梁)。なければ nullptr
};
extern const Zone ZONES[];
extern const int NZ;
extern int STARTS[]; // 区画の下端 (下から数えた位置)
extern int TOT;      // 全部の高さ

// 絵の行 r は、いちばん上の区画の上端から数えた行 (区画 i のローカルの y は TOT - STARTS[i] - h + y)
extern uint16_t *bgBuf, *foreBuf;
extern uint8_t *foreRow; // 手前の絵がある行は 1
const uint16_t KEY = 0x0821; // 手前の絵の抜き色

bool build(); // PSRAM を取って描く (起動時)
} // namespace bg2
