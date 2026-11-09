#pragma once
// 音。素材のファイルは使わず、全部その場で合成する (Windows 版の Sound.cs と同じ音)。
// 鳴らしきりの音 (機銃、爆発、汽笛など) は起動したあと音のタスクが波形を作って PSRAM に置き (50 秒ほど。できた音から鳴る。作った音はフラッシュに保存し、次からは読むだけ)、鳴りつづける音 (プロペラ、雨、円盤のうなり、
// 小鳥のさえずり) はゲームが毎フレーム音量を決めて、混ぜるときに作る。混ぜて MAX98357A へ送るのはコア 0 のタスク
#include <Arduino.h>

enum class Sfx
{
  Shot,
  Hit,
  MomHit,
  Boom,
  BigBoom,
  Crash,
  Death,
  Power,
  Saved,
  Extend,
  Warning,
  Whistle,
  Thunder,
  EnemyShot,
  Start,
  GameOver,
  Tick,
  Select,
  Pause,
  BaaAway,
  MooAway,
  Woof,
  Quack,
  Discover,
  Splash,
  Emerge,
  Growl,
  Bomb,
  Flak,
  BeamFire,
  TankDrop,
  // 2 本目 (FOUNDRY)
  G2Shot,   // 戦車の青い弾: ピュッ
  G2Boom,   // 機械が壊れる: 爆発に金属のガシャン
  CoreFree, // 核が飛び出す: 上ずる電子音
  CoreKill, // 核を撃ち落とした: ガラスのようなチン
  Possess,  // 核が乗り移った: 下がっていく不気味なうなり
  Virus,    // ウイルス: デジタルのノイズ
  Ping,     // 弾をはじいた: キン
  Alarm,    // 防犯カメラに見つかった: ピピッ
  Steam,    // 蒸気: シュー
  Step,     // 戦車の足が着く: カシャ
  Slam,     // フックやプレスが落ちる: ズシン
  G2Start,  // 出発: 工場らしい短い節
  Count
};

class Sound
{
public:
  // 鳴りつづける音の大きさ (0〜1)。ゲームが書き、音のタスクが読む
  volatile float Engine = 0, Rain = 0, UfoHum = 0, Beam = 0, MomHum = 0, Birds = 0;
  volatile float Hum = 0, Roar = 0; // 2 本目: 工場の低いうなり、溶鉱炉のごうごう
  volatile float EnginePitch = 1, EngineBank = 0; // プロペラの音程 (1 = ふつう) と旋回の度合い (0〜1)
  volatile float Master = 1;
  volatile bool Muted = false;
  volatile uint32_t usMix = 0, usLoop = 0; // 確かめる用: 混ぜるのにかかった時間の合計 (us)

  void begin();
  void Play(Sfx s, float vol = 1);
};
