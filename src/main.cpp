// SKYLARK (ESP32 版): WindowsApp/SkyLark のゲームを、Livico と同じ機体 (縦置きの液晶 240x320) で遊ぶ。
// 操作: ジョイスティックで移動 (押し込むとゆっくり)、SW1 撃つ、SW2 爆弾、SW3 短く押すと音量 (5 段階)、長押しで一時停止。止めているあいだ左右でも音量
// 1 秒 30 回ゲームを進め、そのたびに 1 枚描く (描く絵は 1.5 倍の 360x480 を 96 行ずつ 5 本の帯に分け、縮めて液晶へ送る)。
// ランキングと名前と音量は Preferences (名前空間 skylark) に残す
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "config.h"
#include "game.h"
#include "padmode.h"
#include "game2.h"
#include "game3.h"
#include "game4.h"
#include "game5.h"
#include "game6.h"
#include "game7.h"
#include <algorithm>

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *panel = new Arduino_ST7789(bus, TFT_RST, TFT_ROTATION, TFT_IPS, 240, 320);
Gfx gfx;
Game *game = nullptr;
Game2 *game2 = nullptr; // 2 本目を選んだときだけ作る
Game3 *game3 = nullptr; // 3 本目を選んだときだけ作る
Game4 *game4 = nullptr; // 4 本目を選んだときだけ作る
Game5 *game5 = nullptr; // 5 本目を選んだときだけ作る
Game6 *game6 = nullptr; // 6 本目を選んだときだけ作る
Game7 *game7 = nullptr; // 7 本目を選んだときだけ作る
Sound snd;
Preferences prefs;

// ---- ジョイスティック (試験で確かめた読み方。joytest と同じ) ----
const float JUMP = 0.085f;     // 軸ごとに、中央から跳ぶ分 (Axis::read で差し引く)
const float DEAD_ZONE = 0.04f; // 跳ぶ分を差し引いたあとの倒れ (縦横あわせた長さ) がこれより小さいと 0 (中央のぶれを消す)
const float DIR_ON = 0.35f;    // これより倒すと、その向きのキーを押したことにする
const int ADC_MAX = 4095;

struct Axis
{
  int pin;
  bool invert;
  int center = ADC_MAX / 2;
  int last = 0; // 最後に読んだ ADC の値 (確かめる用)
  float n = 0; // -1〜1 (右・上が正。中央からの傾きそのまま。少しならして揺れを消す)
  float v = 0; // -1〜1 (不感帯を除いたもの。readStick() で縦横いっしょに決める)

  Axis(int p, bool inv) : pin(p), invert(inv) {}
  void calibrate()
  {
    long sum = 0;
    for (int i = 0; i < 64; i++)
      sum += analogRead(pin);
    center = sum / 64;
  }
  void read()
  {
    long sum = 0;
    for (int i = 0; i < 4; i++)
      sum += analogRead(pin);
    int raw = sum / 4;
    last = raw;
    float m = raw >= center ? (float)(raw - center) / max(1, ADC_MAX - center) : (float)(raw - center) / max(1, center);
    if (invert)
      m = -m;
    // このスティックは中央の近くに値の変わらない幅があり、そこを越えると約 0.09 へ急に跳ぶ
    // (2026-11-12 に実機で測った)。跳ぶ分を差し引いて 0 からつながるようにする。
    // 差し引かないと、小さいほうの軸が 0 か 0.09 しか取れず、十字の向きに吸いつく
    float am = fabsf(m);
    m = am <= JUMP ? 0 : copysignf((am - JUMP) / (1 - JUMP), m);
    n += (m - n) * 0.6f; // 軽くならす (遅れは 1 回分ほど)
  }
};
// 上を正にそろえる: ふつうのモジュールは上に倒すと VRY が減るので、反転の指定を逆にして渡す
Axis ax{JOY_X_PIN, JOY_X_INVERT}, ay{JOY_Y_PIN, !JOY_Y_INVERT};

// 不感帯を軸ごとでなく円で取る。軸ごとだと、少し斜めに倒しても小さいほうの軸が 0 に切られて
// 十字の向きに吸いつき、いびつに動く。円で取れば向きはそのまま、長さだけを詰める。
// 長さは 1 で止める (四角く動くスティックで斜めだけ速くならないように)
void readStick()
{
  ax.read();
  ay.read();
  float r = sqrtf(ax.n * ax.n + ay.n * ay.n);
  if (r < DEAD_ZONE)
  {
    ax.v = ay.v = 0;
    return;
  }
  float k = min(1.0f, (r - DEAD_ZONE) / (1 - DEAD_ZONE)) / r;
  ax.v = ax.n * k;
  ay.v = ay.n * k;
}

// 押した瞬間を、次にゲームを進めるときまで覚えておく (短く押しても取りこぼさない)
struct Key
{
  bool down = false, hit = false;
  void set(bool d)
  {
    if (d && !down)
      hit = true;
    down = d;
  }
  bool take()
  {
    bool h = hit;
    hit = false;
    return h;
  }
};
Key kSw1, kSw2, kStick, kUp, kDown, kLeft, kRight;

// SW3: 短く押して離すと音量、長く押すと (離すのを待たずに) 一時停止
const uint32_t PAUSE_HOLD_MS = 600;
struct HoldKey
{
  bool down = false, longFired = false, tap = false, hold = false;
  uint32_t since = 0;
  void set(bool d)
  {
    uint32_t now = millis();
    if (d && !down)
    {
      since = now;
      longFired = false;
    }
    else if (d && !longFired && now - since >= PAUSE_HOLD_MS)
    {
      longFired = true;
      hold = true;
    }
    else if (!d && down && !longFired && now - since >= 20) // 20ms より短いのはチャタリングとして捨てる
      tap = true;
    down = d;
  }
  bool takeTap()
  {
    bool t = tap;
    tap = false;
    return t;
  }
  bool takeHold()
  {
    bool h = hold;
    hold = false;
    return h;
  }
};
HoldKey kSw3Hold;

// ゲームで使う左右 (右が正) と上下 (上が正)。取り付けの向きで入れ替える (JOY_SWAP_XY)
float stickX() { return JOY_SWAP_XY ? ay.v : ax.v; }
float stickY() { return JOY_SWAP_XY ? ax.v : ay.v; }

void pollInput()
{
  readStick();
  kSw1.set(digitalRead(SW1_PIN) == LOW);
  kSw2.set(digitalRead(SW2_PIN) == LOW);
  kSw3Hold.set(digitalRead(SW3_PIN) == LOW);
  kStick.set(digitalRead(JOY_SW_PIN) == LOW);
  kRight.set(stickX() >= DIR_ON);
  kLeft.set(stickX() <= -DIR_ON);
  kUp.set(stickY() >= DIR_ON);
  kDown.set(stickY() <= -DIR_ON);
}

Input readInput()
{
  Input inp = {};
  inp.Left = kLeft.down;
  inp.Right = kRight.down;
  inp.Up = kUp.down;
  inp.Down = kDown.down;
  inp.AX = stickX();
  inp.AY = -stickY(); // ゲームは下が正
  inp.Fire = kSw1.down;
  inp.Bomb = kSw2.down;
  inp.Slow = kStick.down;
  inp.FirePressed = kSw1.take();
  bool sw2 = kSw2.take();
  inp.BombPressed = inp.BackPressed = inp.QuitPressed = sw2;
  inp.PausePressed = kSw3Hold.takeHold();
  inp.VolumePressed = kSw3Hold.takeTap();
  inp.UpPressed = kUp.take();
  inp.DownPressed = kDown.take();
  inp.LeftPressed = kLeft.take();
  inp.RightPressed = kRight.take();
  kStick.take();
  return inp;
}

// ---- 保存 ----
void saveTable()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs.putInt(k, game->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs.putString(k, game->Table[i].Name);
  }
  prefs.putString("name", game->LastName);
}

void loadPrefs()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs.isKey(k))
      continue;
    game->Table[i].Score = prefs.getInt(k, game->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs.getString(k, game->Table[i].Name);
    strncpy(game->Table[i].Name, n.c_str(), 3);
    game->Table[i].Name[3] = 0;
  }
  String n = prefs.isKey("name") ? prefs.getString("name", "AAA") : String("AAA");
  strncpy(game->LastName, n.c_str(), 3);
  game->LastName[3] = 0;
  game->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), Game::VOL_LEVELS - 1); // 4 段階のころの "vol" は使わない
}

// 2 本目のランキングと名前は、別の名前空間 (foundry) に残す
Preferences prefs2;
void saveTable2()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs2.putInt(k, game2->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs2.putString(k, game2->Table[i].Name);
  }
  prefs2.putString("name", game2->LastName);
}

void loadPrefs2()
{
  prefs2.begin("foundry", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs2.isKey(k))
      continue;
    game2->Table[i].Score = prefs2.getInt(k, game2->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs2.getString(k, game2->Table[i].Name);
    strncpy(game2->Table[i].Name, n.c_str(), 3);
    game2->Table[i].Name[3] = 0;
  }
  if (prefs2.isKey("name"))
  {
    String n = prefs2.getString("name", "AAA");
    strncpy(game2->LastName, n.c_str(), 3);
    game2->LastName[3] = 0;
  }
}

// 3 本目のランキングと名前は、名前空間 longshot に残す
Preferences prefs3;
void saveTable3()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs3.putInt(k, game3->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs3.putString(k, game3->Table[i].Name);
  }
  prefs3.putString("name", game3->LastName);
}

void loadPrefs3()
{
  prefs3.begin("longshot", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs3.isKey(k))
      continue;
    game3->Table[i].Score = prefs3.getInt(k, game3->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs3.getString(k, game3->Table[i].Name);
    strncpy(game3->Table[i].Name, n.c_str(), 3);
    game3->Table[i].Name[3] = 0;
  }
  if (prefs3.isKey("name"))
  {
    String n = prefs3.getString("name", "AAA");
    strncpy(game3->LastName, n.c_str(), 3);
    game3->LastName[3] = 0;
  }
}

// 4 本目のランキング (打数) と名前は、名前空間 corners に残す
Preferences prefs4;
void saveTable4()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs4.putInt(k, game4->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs4.putString(k, game4->Table[i].Name);
  }
  prefs4.putString("name", game4->LastName);
}

void loadPrefs4()
{
  prefs4.begin("corners", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs4.isKey(k))
      continue;
    game4->Table[i].Score = prefs4.getInt(k, game4->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs4.getString(k, game4->Table[i].Name);
    strncpy(game4->Table[i].Name, n.c_str(), 3);
    game4->Table[i].Name[3] = 0;
  }
  if (prefs4.isKey("name"))
  {
    String n = prefs4.getString("name", "AAA");
    strncpy(game4->LastName, n.c_str(), 3);
    game4->LastName[3] = 0;
  }
}

// 5 本目のランキング (点数) と名前は、名前空間 orbit に残す
Preferences prefs5;
void saveTable5()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs5.putInt(k, game5->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs5.putString(k, game5->Table[i].Name);
  }
  prefs5.putString("name", game5->LastName);
}

void loadPrefs5()
{
  prefs5.begin("orbit", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs5.isKey(k))
      continue;
    game5->Table[i].Score = prefs5.getInt(k, game5->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs5.getString(k, game5->Table[i].Name);
    strncpy(game5->Table[i].Name, n.c_str(), 3);
    game5->Table[i].Name[3] = 0;
  }
  if (prefs5.isKey("name"))
  {
    String n = prefs5.getString("name", "AAA");
    strncpy(game5->LastName, n.c_str(), 3);
    game5->LastName[3] = 0;
  }
}

// 6 本目のランキング (点数) と名前は、名前空間 jukebox に残す
Preferences prefs6;
void saveTable6()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs6.putInt(k, game6->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs6.putString(k, game6->Table[i].Name);
  }
  prefs6.putString("name", game6->LastName);
}

void loadPrefs6()
{
  prefs6.begin("jukebox", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs6.isKey(k))
      continue;
    game6->Table[i].Score = prefs6.getInt(k, game6->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs6.getString(k, game6->Table[i].Name);
    strncpy(game6->Table[i].Name, n.c_str(), 3);
    game6->Table[i].Name[3] = 0;
  }
  if (prefs6.isKey("name"))
  {
    String n = prefs6.getString("name", "AAA");
    strncpy(game6->LastName, n.c_str(), 3);
    game6->LastName[3] = 0;
  }
}

// 7 本目のランキング (払い出しの合計) と名前は、名前空間 smartball に残す
Preferences prefs7;
void saveTable7()
{
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    prefs7.putInt(k, game7->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    prefs7.putString(k, game7->Table[i].Name);
  }
  prefs7.putString("name", game7->LastName);
}

void loadPrefs7()
{
  prefs7.begin("smartball", false);
  char k[4];
  for (int i = 0; i < 5; i++)
  {
    snprintf(k, sizeof(k), "s%d", i);
    if (!prefs7.isKey(k))
      continue;
    game7->Table[i].Score = prefs7.getInt(k, game7->Table[i].Score);
    snprintf(k, sizeof(k), "n%d", i);
    String n = prefs7.getString(k, game7->Table[i].Name);
    strncpy(game7->Table[i].Name, n.c_str(), 3);
    game7->Table[i].Name[3] = 0;
  }
  if (prefs7.isKey("name"))
  {
    String n = prefs7.getString("name", "AAA");
    strncpy(game7->LastName, n.c_str(), 3);
    game7->LastName[3] = 0;
  }
}

void applyVolume7() // 7 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game7->VolumeLevel == 0;
  snd.Master = MASTER[game7->VolumeLevel];
}

void applyVolume6() // 6 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game6->VolumeLevel == 0;
  snd.Master = MASTER[game6->VolumeLevel];
}

void applyVolume5() // 5 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game5->VolumeLevel == 0;
  snd.Master = MASTER[game5->VolumeLevel];
}

void applyVolume4() // 4 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game4->VolumeLevel == 0;
  snd.Master = MASTER[game4->VolumeLevel];
}

void applyVolume3() // 3 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game3->VolumeLevel == 0;
  snd.Master = MASTER[game3->VolumeLevel];
}

void applyVolume2() // 2 本目
{
  static const float MASTER[5] = {0, 0.12f, 0.25f, 0.5f, 1.0f};
  snd.Muted = game2->VolumeLevel == 0;
  snd.Master = MASTER[game2->VolumeLevel];
}

void applyVolume()
{
  static const float MASTER[Game::VOL_LEVELS] = {0, 0.12f, 0.25f, 0.5f, 1.0f}; // OFF / 1〜4 (1 段ごとにおよそ倍)
  snd.Muted = game->VolumeLevel == 0;
  snd.Master = MASTER[game->VolumeLevel];
}

// ---- 描く ----
bool snapshot = false; // シリアルに 's' が来たら、次の 1 枚を送る (確かめる用)
uint32_t drawUs, drawMax, stepUs, frames;

void drawFrame()
{
  uint32_t t0 = micros();
  if (snapshot)
    Serial.printf("FRAME %d %d\n", LW, LH);
  for (int s = 0; s < STRIPS; s++)
  {
    gfx.startStrip(s);
    if (game7)
      game7->Draw(gfx);
    else if (game6)
      game6->Draw(gfx);
    else if (game5)
      game5->Draw(gfx);
    else if (game4)
      game4->Draw(gfx);
    else if (game3)
      game3->Draw(gfx);
    else if (game2)
      game2->Draw(gfx);
    else
      game->Draw(gfx);
    if (snapshot)
      Serial.write((const uint8_t *)gfx.buf, LW * LSTRIP * 2);
    gfx.finishStrip(panel); // 送るのは裏のタスク。そのあいだに次の帯を描く
  }
  if (snapshot)
  {
    Serial.println("\nEND");
    snapshot = false;
  }
  uint32_t dt = micros() - t0;
  drawUs += dt;
  drawMax = max(drawMax, dt);
  frames++;
}

// 確かめる用のコマンド: s = 画面を送る、f = 10 秒先へ進める、n = SW1 を押したことにする、m = 母船の直前へ、z = (2 本目) 次の区画へ、a = (2 本目) 自動で遊ぶ
bool joyLog = false; // 確かめる用: 1 フレームごとにスティックの値を出す ('j' で切り替え)
void serialCommands()
{
  while (Serial.available())
  {
    int c = Serial.read();
    if (c == 's')
      snapshot = true;
    else if (c == 'j')
      joyLog = !joyLog;
    else if (c == 'n')
      kSw1.hit = true;
    else if (c == 'z' && game7) // 7 本目: 持ち玉を 0 にする
      game7->debugEnd();
    else if (strchr("PBWTKE", c) && c && game7) // 7 本目: 妖怪の出来事をすぐ起こす
      game7->debugEvent(c);
    else if (c == 'a' && game7) // 7 本目: 自動で遊ぶを切り替える
      game7->autoPlay = !game7->autoPlay;
    else if (c == 'z' && game6) // 6 本目: いまのボールを落とす
      game6->debugDrain();
    else if (c == 'a' && game6) // 6 本目: 自動で遊ぶを切り替える
      game6->autoPlay = !game6->autoPlay;
    else if (c == 'z' && game5) // 5 本目: いまのボールを落とす
      game5->debugDrain();
    else if (c == 'a' && game5) // 5 本目: 自動で遊ぶを切り替える
      game5->autoPlay = !game5->autoPlay;
    else if (c == 'z' && game4) // 4 本目: 的玉を 1 個だけ残す
      game4->debugClear();
    else if (c == 'a' && game4) // 4 本目: 自動で打つを切り替える
      game4->autoPlay = !game4->autoPlay;
    else if (c == 'z' && game2) // 2 本目: 次の区画へ
      game2->skipZone();
    else if (c == 'z' && game3) // 3 本目: 今の面をしとめたことにする
      game3->skipStage();
    else if (c >= '0' && c <= '9' && game3) // 3 本目: その番号の面へ飛ぶ
      game3->debugStage(c - '0');
    else if (c == 'o' && game3) // 3 本目: スコープ
      game3->debugScope();
    else if (c == 't' && game3) // 3 本目: 照準を標的へ
      game3->debugAimTarget();
    else if (c == 'a' && game2) // 2 本目: 自動で遊ぶ (やられない) を切り替える
      game2->autoPilot = !game2->autoPilot;
    else if (!game)
      continue;
    else if (c == 'f')
    {
      Input none = {};
      for (int i = 0; i < 10 * Game::FPS; i++)
        game->Step(none);
    }
    else if (c == 'm') // 母船が来る直前へ
      game->frame = Game::MOM_AT - 2 * Game::FPS;
  }
}

// ---- 起動の選択画面: SKYLARK か、2 本目 (FOUNDRY) か、3 本目 (LONGSHOT) か、4 本目 (CORNERS) か、5 本目 (ORBIT) か、6 本目 (JUKEBOX) か、7 本目 (SMARTBALL) か、ジョイスティック (PC の USB ゲームパッド) か ----
// 前回選んだものを選んでおき、触らなければ 3 秒で始める。スティックの上下 (SW2 でも) で選び、SW1 か押し込みで決める
// 保存する番号は前からのまま (0 = SKYLARK、1 = ジョイスティック)。2 本目は 2、3 本目は 3、4 本目は 4、5 本目は 5、6 本目は 6、7 本目は 7。画面の並びは MENU_ORDER
const int MODE_GAME = 0, MODE_PAD = 1, MODE_G2 = 2, MODE_G3 = 3, MODE_G4 = 4, MODE_G5 = 5, MODE_G6 = 6, MODE_G7 = 7;
const int MENU_N = 8;
const int MENU_ORDER[MENU_N] = {MODE_GAME, MODE_G2, MODE_G3, MODE_G4, MODE_G5, MODE_G6, MODE_G7, MODE_PAD};
const int MENU_AUTO_MS = 3000;

void drawMenu(int sel)
{
  panel->fillScreen(0x0000);
  panel->setTextSize(3);
  panel->setTextColor(to565(0xF0C860));
  panel->setCursor(LW / 2 - 63, 40);
  panel->print("SKYLARK");
  static const char *NAME[] = {"SKYLARK", "FOUNDRY", "LONGSHOT", "CORNERS", "ORBIT", "JUKEBOX", "SMARTBALL", "JOYSTICK"};
  static const char *SUB[] = {"PLAY SKYLARK", "HEXAPOD TANK", "SNIPER (WIP)", "POOL: CORNER POCKETS", "SPACE PINBALL", "RETRO DINER PINBALL", "OLD SHOP SMART BALL", "USB GAMEPAD FOR PC"};
  for (int i = 0; i < MENU_N; i++)
  {
    int y = 67 + i * 27;
    bool on = i == sel;
    panel->fillRoundRect(24, y, LW - 48, 25, 6, to565(on ? 0xF0C860 : 0x0A111C));
    if (!on)
      panel->drawRoundRect(24, y, LW - 48, 25, 6, to565(0x1C3448));
    panel->setTextSize(2);
    panel->setTextColor(to565(on ? 0x04070C : 0xD8E6F0));
    panel->setCursor(LW / 2 - (int)strlen(NAME[i]) * 6, y + 1);
    panel->print(NAME[i]);
    panel->setTextSize(1);
    panel->setTextColor(to565(on ? 0x04070C : 0x5A7084));
    panel->setCursor(LW / 2 - (int)strlen(SUB[i]) * 3, y + 16);
    panel->print(SUB[i]);
  }
  panel->setTextSize(1);
  panel->setTextColor(to565(0x5A7084));
  panel->setCursor(LW / 2 - 63, 286);
  panel->print("STICK: SELECT  SW1: OK");
}

// 自動で始めるまでの秒読み (0 で消す)
void drawCountdown(int left)
{
  panel->fillRect(0, 298, LW, 12, 0x0000);
  if (left <= 0)
    return;
  char t[16];
  snprintf(t, sizeof(t), "AUTO START %d", (left + 999) / 1000);
  panel->setTextSize(1);
  panel->setTextColor(to565(0xD8E6F0));
  panel->setCursor(LW / 2 - (int)strlen(t) * 3, 300);
  panel->print(t);
}

int chooseMode()
{
  int saved = prefs.getInt("mode", MODE_GAME), sel = 0; // sel は画面の並びの番号
  for (int i = 0; i < MENU_N; i++)
    if (MENU_ORDER[i] == saved)
      sel = i;
  // 電源を入れたときに押していたボタンを、選んだことにしない (いったん離すまで待つ)
  bool armed = false, wasUp = false, wasDown = false, wasSw2 = false;
  bool autoOn = true;
  uint32_t t0 = millis();
  int shown = -1, shownLeft = -1;
  for (;;)
  {
    readStick();
    bool sw1 = digitalRead(SW1_PIN) == LOW, sw2 = digitalRead(SW2_PIN) == LOW, stick = digitalRead(JOY_SW_PIN) == LOW;
    bool up = stickY() >= DIR_ON, down = stickY() <= -DIR_ON;
    if (!sw1 && !stick)
      armed = true;
    else if (armed)
      break;
    int sc = Serial.available() ? Serial.read() : -1; // 確かめる用: シリアルの 1〜5 で、画面の並びのその項目を選ぶ
    if (sc >= '1' && sc < '1' + MENU_N)
    {
      sel = sc - '1';
      break;
    }
    if ((up && !wasUp) || (down && !wasDown) || (sw2 && !wasSw2))
    {
      sel = up ? (sel + MENU_N - 1) % MENU_N : (sel + 1) % MENU_N;
      autoOn = false; // 触ったら自動では始めない
    }
    wasUp = up;
    wasDown = down;
    wasSw2 = sw2;
    int left = autoOn ? MENU_AUTO_MS - (int)(millis() - t0) : 0;
    if (autoOn && left <= 0)
      break;
    if (sel != shown)
    {
      drawMenu(sel);
      shown = sel;
      shownLeft = -1;
    }
    int secs = left > 0 ? (left + 999) / 1000 : 0;
    if (secs != shownLeft)
    {
      drawCountdown(left);
      shownLeft = secs;
    }
    delay(10);
  }
  int mode = MENU_ORDER[sel];
  if (prefs.getInt("mode", -1) != mode)
    prefs.putInt("mode", mode);
  panel->fillScreen(0x0000);
  // 決めたボタンを離すまで待つ (ゲームの最初に撃ったことにならないように)
  while (digitalRead(SW1_PIN) == LOW || digitalRead(JOY_SW_PIN) == LOW)
    delay(10);
  return mode;
}

void setup()
{
  Serial.begin(921600);
  if (TFT_BL >= 0)
  {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // バックライトを点ける
  }
  if (!panel->begin(TFT_SPI_HZ))
    Serial.println("panel begin failed");
  panel->fillScreen(0x0000);
  panel->setTextColor(to565(0xF0C860));
  panel->setTextSize(3);
  panel->setCursor(LW / 2 - 63, LH / 2 - 20);
  panel->print("SKYLARK");
  // GPIO13 は SPI2 の MISO の決まったピンなので、スイッチの pinMode は液晶の begin のあとでする
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(JOY_SW_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);
  delay(50);
  ax.calibrate(); // 起動時はスティックに触らない前提で、その位置を中央にする
  ay.calibrate();

  prefs.begin("skylark", false);
  int mode = chooseMode();
  if (mode == MODE_PAD)
    runPadMode(panel); // 戻らない
  if (mode == MODE_G7)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0xC8A860));
    panel->setCursor(LW / 2 - 81, LH / 2 - 20);
    panel->print("SMARTBALL");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xE8DFC4));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game7 = new Game7();
    game7->Snd = &snd;
    game7->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume7();
    uint32_t b0 = millis();
    if (!game7->begin())
      Serial.println("game7 begin failed (psram)");
    loadPrefs7();
    game7->TableChanged = saveTable7;
    Serial.printf("game7 table built in %lu ms  heap %u  psram %u\n", millis() - b0, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  if (mode == MODE_G6)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0xE8323A));
    panel->setCursor(LW / 2 - 63, LH / 2 - 20);
    panel->print("JUKEBOX");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xFFF4DC));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game6 = new Game6();
    game6->Snd = &snd;
    game6->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume6();
    uint32_t b0 = millis();
    if (!game6->begin())
      Serial.println("game6 begin failed (psram)");
    loadPrefs6();
    game6->TableChanged = saveTable6;
    Serial.printf("game6 table built in %lu ms  heap %u  psram %u\n", millis() - b0, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  if (mode == MODE_G5)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0x60E0F0));
    panel->setCursor(LW / 2 - 45, LH / 2 - 20);
    panel->print("ORBIT");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xDCD8CF));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game5 = new Game5();
    game5->Snd = &snd;
    game5->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume5();
    uint32_t b0 = millis();
    if (!game5->begin())
      Serial.println("game5 begin failed (psram)");
    loadPrefs5();
    game5->TableChanged = saveTable5;
    Serial.printf("game5 table built in %lu ms  heap %u  psram %u\n", millis() - b0, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  if (mode == MODE_G4)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0xE0B050));
    panel->setCursor(LW / 2 - 63, LH / 2 - 20);
    panel->print("CORNERS");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xDCD8CF));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game4 = new Game4();
    game4->Snd = &snd;
    game4->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume4();
    uint32_t b0 = millis();
    if (!game4->begin())
      Serial.println("game4 begin failed (psram)");
    loadPrefs4();
    game4->TableChanged = saveTable4;
    Serial.printf("game4 table built in %lu ms  heap %u  psram %u\n", millis() - b0, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  if (mode == MODE_G3)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0xD9A441));
    panel->setCursor(LW / 2 - 72, LH / 2 - 20);
    panel->print("LONGSHOT");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xDCD8CF));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game3 = new Game3();
    game3->Snd = &snd;
    game3->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume3();
    if (!game3->begin())
      Serial.println("game3 begin failed (psram)");
    loadPrefs3();
    game3->TableChanged = saveTable3;
    Serial.printf("game3 world built in %lu ms  heap %u  psram %u\n", game3->buildMs, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  if (mode == MODE_G2)
  {
    panel->setTextSize(3);
    panel->setTextColor(to565(0xC9A227));
    panel->setCursor(LW / 2 - 63, LH / 2 - 20);
    panel->print("FOUNDRY");
    panel->setTextSize(1);
    panel->setTextColor(to565(0xDCD8CF));
    panel->setCursor(LW / 2 - 30, LH / 2 + 16);
    panel->print("LOADING...");
    if (!gfx.begin())
      Serial.println("gfx begin failed (memory)");
    disableCore0WDT();
    snd.begin();
    game2 = new Game2();
    game2->Snd = &snd;
    game2->VolumeLevel = min(max((int)prefs.getInt("vol5", 2), 0), 4); // SKYLARK と同じ設定
    applyVolume2();
    if (!game2->begin())
      Serial.println("game2 begin failed (psram)");
    loadPrefs2();
    game2->TableChanged = saveTable2;
    Serial.printf("game2 bg built in %lu ms  heap %u  psram %u\n", game2->buildMs, ESP.getFreeHeap(), ESP.getFreePsram());
    return;
  }
  panel->setTextSize(3);
  panel->setTextColor(to565(0xF0C860));
  panel->setCursor(LW / 2 - 63, LH / 2 - 20);
  panel->print("SKYLARK");
  panel->setTextSize(1);
  panel->setTextColor(to565(0xFFF4D8));
  panel->setCursor(LW / 2 - 30, LH / 2 + 16);
  panel->print("LOADING...");

  if (!gfx.begin())
    Serial.println("gfx begin failed (memory)");
  disableCore0WDT(); // コア 0 は音を作るのと液晶へ送るので埋まるので、IDLE0 の見張りは外す
  snd.begin();
  game = new Game();
  game->Snd = &snd;
  loadPrefs();
  game->TableChanged = saveTable;
  applyVolume();
  Serial.printf("heap %u  psram %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
}

// 2 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop2()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game2->Step(readInput());
    if (game2->WantMenu)
      ESP.restart();
    if (game2->VolumeChanged)
    {
      game2->VolumeChanged = false;
      applyVolume2();
      prefs.putInt("vol5", game2->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game2::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f\n", drawUs / f, drawMax, gfx.usPush / f, stepUs / f,
                  f * 1000.0f / (millis() - lastLog));
    Serial.printf("  bg %lu  anim %lu  objects %lu (boss %lu  enemies %lu  hud %lu)  fore %lu  en %u\n", game2->profBg / f, game2->profAnim / f,
                  game2->profObj / f, game2->profBoss / f, game2->profEn / f, game2->profHud / f, game2->profFore / f, game2->enemyCount());
    Serial.printf("  audio %lu us/s (loops %lu)\n", snd.usMix * 1000 / (millis() - lastLog), snd.usLoop * 1000 / (millis() - lastLog));
    snd.usMix = snd.usLoop = 0;
    game2->profBg = game2->profAnim = game2->profObj = game2->profFore = game2->profBoss = game2->profEn = game2->profHud = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

// 3 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop3()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game3->Step(readInput());
    if (game3->WantMenu)
      ESP.restart();
    if (game3->VolumeChanged)
    {
      game3->VolumeChanged = false;
      applyVolume3();
      prefs.putInt("vol5", game3->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game3::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f  (scene %lu hud %lu)\n", drawUs / f, drawMax, gfx.usPush / f,
                  stepUs / f, f * 1000.0f / (millis() - lastLog), game3->profObj / f, game3->profHud / f);
    Serial.printf("  audio %lu us/s (loops %lu)\n", snd.usMix * 1000 / (millis() - lastLog), snd.usLoop * 1000 / (millis() - lastLog));
    snd.usMix = snd.usLoop = 0;
    game3->profObj = game3->profHud = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

// 4 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop4()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game4->Step(readInput());
    if (joyLog)
      Serial.printf("J %d %d %d %d %.3f %.3f\n", ax.last, ay.last, ax.center, ay.center, ax.n, ay.n);
    if (game4->WantMenu)
      ESP.restart();
    if (game4->VolumeChanged)
    {
      game4->VolumeChanged = false;
      applyVolume4();
      prefs.putInt("vol5", game4->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game4::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f  (game draw %lu)\n", drawUs / f, drawMax, gfx.usPush / f,
                  stepUs / f, f * 1000.0f / (millis() - lastLog), game4->profDraw / f);
    game4->profDraw = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

// 5 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop5()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game5->Step(readInput());
    if (game5->WantMenu)
      ESP.restart();
    if (game5->VolumeChanged)
    {
      game5->VolumeChanged = false;
      applyVolume5();
      prefs.putInt("vol5", game5->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game5::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f  (game draw %lu)\n", drawUs / f, drawMax, gfx.usPush / f,
                  stepUs / f, f * 1000.0f / (millis() - lastLog), game5->profDraw / f);
    game5->profDraw = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

// 6 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop6()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game6->Step(readInput());
    if (game6->WantMenu)
      ESP.restart();
    if (game6->VolumeChanged)
    {
      game6->VolumeChanged = false;
      applyVolume6();
      prefs.putInt("vol5", game6->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game6::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f  (game draw %lu)\n", drawUs / f, drawMax, gfx.usPush / f,
                  stepUs / f, f * 1000.0f / (millis() - lastLog), game6->profDraw / f);
    game6->profDraw = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

// 7 本目: 1 秒 30 回進めて描く。デモの画面で SW3 を長く押すと選択画面へ戻る (再起動)
void loop7()
{
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2)
  {
    uint32_t t0 = micros();
    game7->Step(readInput());
    if (game7->WantMenu)
      ESP.restart();
    if (game7->VolumeChanged)
    {
      game7->VolumeChanged = false;
      applyVolume7();
      prefs.putInt("vol5", game7->VolumeLevel);
    }
    stepUs += micros() - t0;
    next += 1000 / Game7::FPS + ((next % 3) == 0 ? 1 : 0);
    n++;
  }
  if ((int32_t)(now - next) >= 0)
    next = now + 33;
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (push wait %lu)  step %lu us  fps %.1f  (game draw %lu)  shots %d win %d stolen %d\n", drawUs / f, drawMax,
                  gfx.usPush / f, stepUs / f, f * 1000.0f / (millis() - lastLog), game7->profDraw / f, game7->statShots, game7->statWin, game7->statStolen);
    game7->profDraw = 0;
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    lastLog = millis();
  }
}

void loop()
{
  if (game7)
  {
    loop7();
    return;
  }
  if (game6)
  {
    loop6();
    return;
  }
  if (game5)
  {
    loop5();
    return;
  }
  if (game4)
  {
    loop4();
    return;
  }
  if (game3)
  {
    loop3();
    return;
  }
  if (game2)
  {
    loop2();
    return;
  }
  static uint32_t next = millis();
  static uint32_t lastLog = millis();
  pollInput();
  serialCommands();
  uint32_t now = millis();
  if ((int32_t)(now - next) < 0)
    return;
  int n = 0;
  while ((int32_t)(now - next) >= 0 && n < 2) // 描くのが間に合わないときは、進めるだけ先にする (2 回まで)
  {
    uint32_t t0 = micros();
    game->Step(readInput());
    stepUs += micros() - t0;
    next += 1000 / Game::FPS + ((next % 3) == 0 ? 1 : 0); // 33.3ms
    n++;
  }
  if ((int32_t)(now - next) >= 0) // 大きく遅れたら追いかけない
    next = now + 33;
  if (game->VolumeChanged)
  {
    game->VolumeChanged = false;
    applyVolume();
    prefs.putInt("vol5", game->VolumeLevel);
  }
  drawFrame();
  if (millis() - lastLog >= 5000 && frames > 0)
  {
    uint32_t f = frames;
    Serial.printf("draw avg %lu us max %lu us (shrink %lu push %lu)  step %lu us  fps %.1f\n", drawUs / frames, drawMax,
                  0u, gfx.usPush / frames, stepUs / frames, frames * 1000.0f / (millis() - lastLog));
    drawUs = drawMax = stepUs = frames = 0;
    gfx.usPush = 0;
    Serial.printf("  ground %lu  shadow %lu  props %lu  sky %lu  air %lu  tint %lu  overlay %lu  rows %lu\n", game->prof[0] / f, game->prof[1] / f,
                  game->prof[2] / f, game->prof[3] / f, game->prof[4] / f, game->prof[5] / f, game->prof[6] / f, game->prof[7] / f);
    Serial.printf("  mom %lu  shots %lu  propShadow %lu  tint %lu  rows %lu (max %lu us)\n", game->profMom / f, game->profShots / f,
                  game->profPS / f, game->profTint / f, game->rowsN, game->rowMax);
    game->profMom = game->profShots = game->profPS = game->profTint = game->rowsN = game->rowMax = 0;
    memset(game->prof, 0, sizeof(game->prof));
    lastLog = millis();
  }
}
