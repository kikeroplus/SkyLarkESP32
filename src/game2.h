#pragma once
// 2 本目 (仮の名前 FOUNDRY): 捨てられた工場を、六本脚の戦車が上へ登っていく縦シューティング。
// 敵は、工業機械に乗り移った AI。味方の弾は青、敵の目と弾は赤。
// 背景は g2bg (起動時に PSRAM に描いておく)、描く部品は g2draw (液晶の座標でそのまま描く)。
// いまは試作: 背景・戦車の歩行・撃つ・ウォーカーとツインファン・爆発と逃げる核まで。やられても止まらない (当たった数を数えるだけ)
#include "game.h"
#include <vector>

class Game2
{
public:
  static constexpr int FPS = 30;
  bool begin(); // 背景を描いておく (1〜2 秒かかる)
  void Step(const Input &in);
  void Draw(Gfx &g); // 帯 1 本ぶん (main が帯ごとに呼ぶ)

  uint32_t buildMs = 0;
  int score = 0, hits = 0;
  uint32_t profBg = 0, profAnim = 0, profObj = 0, profFore = 0; // 描く時間の内訳 (確かめる用。us の合計)

private:
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
    int k; // 0 爆発 / 1 逃げる核
    float x, y, s;
    int t, life;
    bool ground;
  };

  float scroll = 0, tx = 120, ty = 250, ang = -1.5708f, rec = 0;
  int t = 0, fireCool = 0, hitFlash = 0;
  Foot feet[6];
  std::vector<Shot> shots, eb;
  std::vector<Enemy> en;
  std::vector<Fx> fx;

  void initFeet();
  void enemyStep();
  int zoneAt(int sy) const;
};
