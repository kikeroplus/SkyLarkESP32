#pragma once
// 2 本目 (廃工場) の敵とボスで使う描く部品 (game2_enemy.cpp)
#include "g2draw.h"

namespace ep
{
const uint32_t RED = 0xff3b2a, RED2 = 0xb0201a, RED3 = 0x4a1410, REDW = 0xffe8e0;
void eye(float x, float y, float r, int t, bool flash, bool weakFlicker); // 赤く光る目 (乗り移ったしるし)
void nerve(std::initializer_list<Pt> pts, int t, float ph = 0);           // 機械に這う赤い配線
void sparks(float x, float y, int t, int n);                               // 弱ったときの火花
void block(float x, float gy, int w, int d, float hh, uint32_t top, uint32_t front); // 斜め上から見た箱
} // namespace ep
