// SKYLARK (ESP32 版): WindowsApp/SkyLark のゲームを、Livico と同じ機体 (縦置きの液晶 240x320) で遊ぶ。
// 操作: ジョイスティックで移動 (押し込むとゆっくり)、SW1 撃つ、SW2 爆弾、SW3 一時停止。止めているあいだ左右で音量
// 1 秒 30 回ゲームを進め、そのたびに 1 枚描く (描く絵は 1.5 倍の 360x480 を 96 行ずつ 5 本の帯に分け、縮めて液晶へ送る)。
// ランキングと名前と音量は Preferences (名前空間 skylark) に残す
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "config.h"
#include "game.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *panel = new Arduino_ST7789(bus, TFT_RST, TFT_ROTATION, TFT_IPS, 240, 320);
Gfx gfx;
Game *game = nullptr;
Sound snd;
Preferences prefs;

// ---- ジョイスティック (試験で確かめた読み方。joytest と同じ) ----
const float DEAD_ZONE = 0.12f; // これより小さい傾きは 0 (中央のぶれを消す)
const float DIR_ON = 0.35f;    // これより倒すと、その向きのキーを押したことにする
const int ADC_MAX = 4095;

struct Axis
{
  int pin;
  bool invert;
  int center = ADC_MAX / 2;
  float v = 0; // -1〜1 (右・上が正。不感帯を除いたもの)

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
    float n = raw >= center ? (float)(raw - center) / max(1, ADC_MAX - center) : (float)(raw - center) / max(1, center);
    if (invert)
      n = -n;
    float a = fabsf(n);
    v = a < DEAD_ZONE ? 0 : copysignf(min(1.0f, (a - DEAD_ZONE) / (1 - DEAD_ZONE)), n);
  }
};
// 上を正にそろえる: ふつうのモジュールは上に倒すと VRY が減るので、反転の指定を逆にして渡す
Axis ax{JOY_X_PIN, JOY_X_INVERT}, ay{JOY_Y_PIN, !JOY_Y_INVERT};

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
Key kSw1, kSw2, kSw3, kStick, kUp, kDown, kLeft, kRight;

// ゲームで使う左右 (右が正) と上下 (上が正)。取り付けの向きで入れ替える (JOY_SWAP_XY)
float stickX() { return JOY_SWAP_XY ? ay.v : ax.v; }
float stickY() { return JOY_SWAP_XY ? ax.v : ay.v; }

void pollInput()
{
  ax.read();
  ay.read();
  kSw1.set(digitalRead(SW1_PIN) == LOW);
  kSw2.set(digitalRead(SW2_PIN) == LOW);
  kSw3.set(digitalRead(SW3_PIN) == LOW);
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
  inp.PausePressed = kSw3.take();
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
  game->VolumeLevel = prefs.getInt("vol", 2);
}

void applyVolume()
{
  static const float MASTER[] = {0, 0.3f, 0.6f, 1.0f}; // OFF / LOW / MID / HIGH
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

// 確かめる用のコマンド: s = 画面を送る、f = 10 秒先へ進める、n = SW1 を押したことにする、m = 母船の直前へ
void serialCommands()
{
  while (Serial.available())
  {
    int c = Serial.read();
    if (c == 's')
      snapshot = true;
    else if (c == 'f')
    {
      Input none = {};
      for (int i = 0; i < 10 * Game::FPS; i++)
        game->Step(none);
    }
    else if (c == 'n')
      kSw1.hit = true;
    else if (c == 'm') // 母船が来る直前へ
      game->frame = Game::MOM_AT - 2 * Game::FPS;
  }
}

void setup()
{
  Serial.begin(921600);
  if (!panel->begin(80000000))
    Serial.println("panel begin failed");
  panel->fillScreen(0x0000);
  panel->setTextColor(to565(0xF0C860));
  panel->setTextSize(3);
  panel->setCursor(LW / 2 - 63, LH / 2 - 20);
  panel->print("SKYLARK");
  panel->setTextSize(1);
  panel->setTextColor(to565(0xFFF4D8));
  panel->setCursor(LW / 2 - 30, LH / 2 + 16);
  panel->print("LOADING...");
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

  if (!gfx.begin())
    Serial.println("gfx begin failed (memory)");
  disableCore0WDT(); // コア 0 は音を作るのと液晶へ送るので埋まるので、IDLE0 の見張りは外す
  snd.begin();
  game = new Game();
  game->Snd = &snd;
  prefs.begin("skylark", false);
  loadPrefs();
  game->TableChanged = saveTable;
  applyVolume();
  Serial.printf("heap %u  psram %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
}

void loop()
{
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
    prefs.putInt("vol", game->VolumeLevel);
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
