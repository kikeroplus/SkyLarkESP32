// ジョイスティックのモード: この機体を PC の USB ゲームパッドにする (ESP32/JoyStick と同じ動き)。
// スティック → 左スティック (X/Y 軸)、SW1 → ボタン 1、SW2 → ボタン 2、SW3 → ボタン 3、押し込み → ボタン 4。
// 向きはゲームと同じ (縦置きで持ったときの上下左右)。SW3 を 2 秒押し続けると中央を取り直す。
// SW1 と SW3 を 2 秒押し続けると再起動して、起動の選択画面に戻る。
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "USB.h"
#include "USBHIDGamepad.h"
#include "config.h"
#include "padmode.h"

// ゲームパッドは USB が始まる前 (setup より前) に登録しておく必要があるので、いつも置いておく。
// ゲームのモードでは何も送らない
static USBHIDGamepad pad;

namespace
{
const int W = 240, H = 320;

uint16_t c565(uint32_t rgb) { return ((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F); }
const uint32_t C_BG = 0x04070C, C_PANEL = 0x0A111C, C_LINE = 0x1C3448,
               C_DIM = 0x5A7084, C_TEXT = 0xD8E6F0, C_CYAN = 0x00D0FF,
               C_GREEN = 0x30E080, C_YELLOW = 0xF0C020, C_RED = 0xFF4050;

const float JUMP = 0.085f;     // 軸ごとに、中央から跳ぶ分 (Axis::read で差し引く)
const float DEAD_ZONE = 0.03f; // 跳ぶ分を差し引いたあとの倒れ (縦横あわせた長さ) がこれより小さいと 0 (中央のぶれを消す)
const int ADC_MAX = 4095;

struct Axis
{
  int pin;
  bool invert;
  int center = ADC_MAX / 2;
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
float stickX() { return JOY_SWAP_XY ? ay.v : ax.v; }
float stickY() { return JOY_SWAP_XY ? ax.v : ay.v; }

struct Button
{
  int pin;
  const char *name;
  bool down = false;
};
Button buttons[] = {{SW1_PIN, "SW1"}, {SW2_PIN, "SW2"}, {SW3_PIN, "SW3"}, {JOY_SW_PIN, "STICK"}};
const int BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

const int HOLD_MS = 2000;        // 長押しとみなす長さ
uint32_t calSince = 0, exitSince = 0; // 長押しが始まった時刻 (0 = 押していない)
bool calDone = false;
uint32_t calShownUntil = 0;

volatile int8_t sentX = 0, sentY = 0;
uint32_t sentButtons = 0;
bool sentOnce = false;
volatile uint32_t reports = 0;

Arduino_Canvas *cv = nullptr;

int8_t toAxis(float v) { return (int8_t)lroundf(constrain(v, -1.0f, 1.0f) * 127); }

void calibrate()
{
  ax.calibrate();
  ay.calibrate();
  calShownUntil = millis() + 1000;
}

// 長押しを数える。押し続けて HOLD_MS たったら 1 回だけ true
bool held(bool down, uint32_t &since)
{
  if (!down)
  {
    since = 0;
    return false;
  }
  if (!since)
    since = millis() | 1;
  return millis() - since >= HOLD_MS;
}

void poll()
{
  readStick();
  uint32_t bits = 0;
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    buttons[i].down = digitalRead(buttons[i].pin) == LOW;
    if (buttons[i].down)
      bits |= 1u << i;
  }

  // SW1 + SW3 長押しで、選択画面に戻る
  if (held(buttons[0].down && buttons[2].down, exitSince))
  {
    pad.send(0, 0, 0, 0, 0, 0, HAT_CENTER, 0); // 押したままにならないように離してから
    delay(50);
    ESP.restart();
  }
  // SW3 だけの長押しで中央を取り直す (スティックから手を離して押す)
  bool sw3Only = buttons[2].down && !buttons[0].down && !buttons[1].down;
  if (held(sw3Only, calSince))
  {
    if (!calDone)
      calibrate();
    calDone = true;
  }
  else if (!sw3Only)
    calDone = false;

  // HID の Y は下が正
  int8_t x = toAxis(stickX()), y = toAxis(-stickY());
  if (!sentOnce || x != sentX || y != sentY || bits != sentButtons)
  {
    if (pad.send(x, y, 0, 0, 0, 0, HAT_CENTER, bits))
    {
      sentX = x;
      sentY = y;
      sentButtons = bits;
      sentOnce = true;
      reports++;
    }
  }
}

void text(int x, int y, uint32_t col, const char *fmt, ...)
{
  char s[64];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(s, sizeof(s), fmt, ap);
  va_end(ap);
  cv->setTextColor(c565(col));
  cv->setCursor(x, y);
  cv->print(s);
}

void draw()
{
  cv->fillScreen(c565(C_BG));
  cv->setTextSize(2);
  text(12, 12, C_TEXT, "JOYSTICK");
  cv->setTextSize(1);
  bool on = (bool)USB;
  cv->fillCircle(W - 66, 19, 4, c565(on ? C_GREEN : C_RED));
  text(W - 56, 16, on ? C_GREEN : C_DIM, on ? "USB OK" : "NO USB");

  // スティックの位置 (枠の中の点)
  const int cx = W / 2, cy = 130, r = 80;
  cv->fillRoundRect(cx - r - 6, cy - r - 6, 2 * r + 12, 2 * r + 12, 8, c565(C_PANEL));
  cv->drawCircle(cx, cy, r, c565(C_LINE));
  cv->drawFastHLine(cx - r, cy, 2 * r, c565(C_LINE));
  cv->drawFastVLine(cx, cy - r, 2 * r, c565(C_LINE));
  int8_t x = sentX, y = sentY;
  int px = cx + x * r / 127, py = cy + y * r / 127;
  cv->fillCircle(px, py, 9, c565(x || y ? C_CYAN : C_DIM));

  cv->setTextSize(2);
  text(16, 226, C_TEXT, "X %4d", x);
  text(W / 2 + 8, 226, C_TEXT, "Y %4d", y);

  // ボタン
  cv->setTextSize(1);
  const int bw = 52, gap = 4, bx0 = (W - (bw * BUTTON_COUNT + gap * (BUTTON_COUNT - 1))) / 2, by = 254;
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    int bx = bx0 + i * (bw + gap);
    bool d = buttons[i].down;
    cv->fillRoundRect(bx, by, bw, 34, 6, c565(d ? C_YELLOW : C_PANEL));
    uint32_t col = d ? C_BG : C_DIM;
    text(bx + (bw - 6 * (int)strlen(buttons[i].name)) / 2, by + 7, col, "%s", buttons[i].name);
    text(bx + (bw - 12) / 2, by + 21, col, "B%d", i + 1);
  }

  uint32_t now = millis();
  if (now < calShownUntil)
    text(12, H - 16, C_GREEN, "CENTER SET");
  else if (exitSince)
    text(12, H - 16, C_YELLOW, "HOLD SW1+SW3: MENU %d", (int)(HOLD_MS - (now - exitSince) + 999) / 1000);
  else if (calSince && !calDone)
    text(12, H - 16, C_YELLOW, "HOLD SW3: CENTER %d", (int)(HOLD_MS - (now - calSince) + 999) / 1000);
  else
  {
    text(12, H - 28, C_DIM, "SW3 HOLD 2s = CENTER");
    text(12, H - 16, C_DIM, "SW1+SW3 HOLD 2s = MENU");
  }
  text(W - 48, H - 16, C_DIM, "%7lu", (unsigned long)reports);
  cv->flush();
}

void drawTask(void *)
{
  for (;;)
  {
    uint32_t t = millis();
    draw();
    uint32_t used = millis() - t;
    vTaskDelay(pdMS_TO_TICKS(used < 33 ? 33 - used : 1)); // 液晶は 1 秒 30 回ほど
  }
}
} // namespace

void runPadMode(Arduino_GFX *panel)
{
  pad.begin();
  cv = new Arduino_Canvas(W, H, panel);
  if (!cv->begin(GFX_SKIP_OUTPUT_BEGIN)) // 液晶はもう始めてある
    Serial.println("canvas begin failed");
  calibrate(); // スティックに触っていない前提で、その位置を中央にする
  // 液晶へ送るのは 1 回十数 ms かかるので、コア 0 の別のタスクで描く (読み取りを止めない)
  xTaskCreatePinnedToCore(drawTask, "draw", 4096, nullptr, 1, nullptr, 0);
  for (;;)
  {
    poll(); // 1 秒 500 回読む
    delay(2);
  }
}
