// SkyLarkESP32 ジョイスティックの試験 (pio run -e joytest -t upload)
// VRX・VRY の値、倒した位置、ゲームで使う 8 方向、押し込み (SW) と SW1〜SW3 を画面とシリアルに出す
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

#define SCREEN_W 320
#define SCREEN_H 240

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *panel = new Arduino_ST7789(bus, TFT_RST, 3, TFT_IPS, 240, 320); // 試験は横置き
// 画面全体のキャンバス (PSRAM に取る) に描いてから一度に送る
Arduino_Canvas *cv = new Arduino_Canvas(SCREEN_W, SCREEN_H, panel);

// ---- 色 (Livico と同じ) ----
uint16_t c565(uint32_t rgb) { return ((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F); }
const uint32_t C_BG = 0x04070C, C_PANEL = 0x0A111C, C_LINE = 0x1C3448,
               C_DIM = 0x5A7084, C_TEXT = 0xD8E6F0, C_CYAN = 0x00D0FF,
               C_GREEN = 0x30E080, C_YELLOW = 0xF0C020, C_RED = 0xFF4050;

// ---- 判定のしきい値 (倒した量 0〜1 に対して) ----
const float DEAD_ZONE = 0.12f; // これより小さい傾きは 0 とみなす (中央のぶれを消す)
const float DIR_ON = 0.35f;    // これより倒すと、その向きのキーを押したことにする (矢印キーの代わり)

const int ADC_MAX = 4095;
const int SAMPLES = 8; // 1 回の読み取りで平均する数

struct Axis
{
  int pin;
  bool invert;
  int raw = 0, mv = 0;
  int center = ADC_MAX / 2;
  int lo = ADC_MAX, hi = 0; // 動かした範囲
  int hist[30] = {0};       // ぶれを見るための最近の値
  int histPos = 0;
  float v = 0;              // 倒した量 -1〜1 (右・上が正。不感帯を除いたもの)

  Axis(int p, bool inv) : pin(p), invert(inv) {}

  void read()
  {
    long sum = 0, sumMv = 0;
    for (int i = 0; i < SAMPLES; i++)
    {
      sum += analogRead(pin);
      sumMv += analogReadMilliVolts(pin);
    }
    raw = sum / SAMPLES;
    mv = sumMv / SAMPLES;
    lo = min(lo, raw);
    hi = max(hi, raw);
    hist[histPos] = raw;
    histPos = (histPos + 1) % 30;

    // 中央から端までの長さで割る (中央が真ん中にないこともあるので左右別々に)
    float n = raw >= center ? (float)(raw - center) / max(1, ADC_MAX - center)
                            : (float)(raw - center) / max(1, center);
    if (invert)
      n = -n;
    float a = fabsf(n);
    v = a < DEAD_ZONE ? 0 : copysignf(min(1.0f, (a - DEAD_ZONE) / (1 - DEAD_ZONE)), n);
  }
  int noise() const
  {
    int a = ADC_MAX, b = 0;
    for (int h : hist)
    {
      a = min(a, h);
      b = max(b, h);
    }
    return b - a;
  }
  void calibrate()
  {
    long sum = 0;
    for (int i = 0; i < 64; i++)
      sum += analogRead(pin);
    center = sum / 64;
    lo = hi = center;
  }
};

// 上を正にそろえる: ふつうのモジュールは上に倒すと VRY が減るので、反転の指定を逆にして渡す
Axis ax{JOY_X_PIN, JOY_X_INVERT}, ay{JOY_Y_PIN, !JOY_Y_INVERT};

struct Button
{
  int pin;
  const char *name;
  bool down = false;
  int count = 0;

  Button(int p, const char *n) : pin(p), name(n) {}
};
Button buttons[] = {{JOY_SW_PIN, "STICK"}, {SW1_PIN, "SW1"}, {SW2_PIN, "SW2"}, {SW3_PIN, "SW3"}};
Button &stick = buttons[0], &sw1 = buttons[1], &sw2 = buttons[2];
const int BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

bool up, down, left, right; // ゲームで使う 8 方向
int pressX = -1, pressY = -1; // 押し込んだときの点 (押し込みで値がずれないか見る)

// 押した瞬間に true を返す
bool updateButton(Button &b)
{
  bool d = digitalRead(b.pin) == LOW;
  bool pressed = d && !b.down;
  b.down = d;
  if (pressed)
    b.count++;
  return pressed;
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

// 矢印 (向き dx, dy の三角)
void arrow(int cx, int cy, int dx, int dy, bool on)
{
  uint16_t col = c565(on ? C_CYAN : C_LINE);
  int s = 9;
  if (dx)
    cv->fillTriangle(cx + dx * s, cy, cx - dx * s, cy - s, cx - dx * s, cy + s, col);
  else
    cv->fillTriangle(cx, cy + dy * s, cx - s, cy - dy * s, cx + s, cy - dy * s, col);
}

void draw()
{
  cv->fillScreen(c565(C_BG));
  cv->setTextSize(1);

  // ---- 左: 倒した位置 ----
  const int R = 100, CX = 4 + R, CY = 20 + R;
  text(4, 4, C_DIM, "JOYSTICK TEST");
  cv->fillCircle(CX, CY, R, c565(C_PANEL));
  cv->drawCircle(CX, CY, R, c565(C_LINE));
  cv->drawCircle(CX, CY, R * DIR_ON, c565(C_LINE));
  cv->drawCircle(CX, CY, max(1, (int)(R * DEAD_ZONE)), c565(C_DIM));
  cv->drawFastHLine(CX - R, CY, 2 * R, c565(C_LINE));
  cv->drawFastVLine(CX, CY - R, 2 * R, c565(C_LINE));

  // 生の値を -1〜1 にしたもの (不感帯の前) の点を細かく、不感帯を除いた値を大きな点で
  auto rawN = [](const Axis &a, int r)
  {
    float n = r >= a.center ? (float)(r - a.center) / max(1, ADC_MAX - a.center)
                            : (float)(r - a.center) / max(1, a.center);
    return a.invert ? -n : n;
  };
  // 動かした範囲の四角
  int x0 = CX + rawN(ax, ax.invert ? ax.hi : ax.lo) * R, x1 = CX + rawN(ax, ax.invert ? ax.lo : ax.hi) * R;
  int y0 = CY - rawN(ay, ay.invert ? ay.lo : ay.hi) * R, y1 = CY - rawN(ay, ay.invert ? ay.hi : ay.lo) * R;
  cv->drawRect(x0, y0, max(1, x1 - x0 + 1), max(1, y1 - y0 + 1), c565(C_DIM));
  if (pressX >= 0)
    cv->drawCircle(pressX, pressY, 4, c565(C_YELLOW));
  cv->drawPixel(CX + rawN(ax, ax.raw) * R, CY - rawN(ay, ay.raw) * R, c565(C_TEXT));
  int px = CX + ax.v * R, py = CY - ay.v * R;
  cv->fillCircle(px, py, 6, c565(stick.down ? C_YELLOW : C_CYAN));

  // ---- 右: 値 ----
  int x = 214, y = 4;
  text(x, y, C_DIM, "     X      Y");
  y += 12;
  text(x, y, C_DIM, "RAW");
  text(x + 24, y, C_TEXT, "%4d   %4d", ax.raw, ay.raw);
  y += 11;
  text(x, y, C_DIM, "mV");
  text(x + 24, y, ax.mv > 3200 || ay.mv > 3200 ? C_RED : C_TEXT, "%4d   %4d", ax.mv, ay.mv);
  y += 11;
  text(x, y, C_DIM, "CEN");
  text(x + 24, y, C_TEXT, "%4d   %4d", ax.center, ay.center);
  y += 11;
  text(x, y, C_DIM, "MIN");
  text(x + 24, y, C_TEXT, "%4d   %4d", ax.lo, ay.lo);
  y += 11;
  text(x, y, C_DIM, "MAX");
  text(x + 24, y, C_TEXT, "%4d   %4d", ax.hi, ay.hi);
  y += 11;
  text(x, y, C_DIM, "NOIS");
  text(x + 24, y, C_TEXT, "%4d   %4d", ax.noise(), ay.noise());
  y += 11;
  text(x, y, C_DIM, "VAL");
  text(x + 18, y, C_CYAN, "%+5.2f  %+5.2f", ax.v, ay.v);

  // 8 方向 (ゲームの矢印キーの代わり)
  int ac = 262, acy = 136;
  text(x, 106, C_DIM, "DIRECTION");
  arrow(ac, acy - 18, 0, -1, up);
  arrow(ac, acy + 18, 0, 1, down);
  arrow(ac - 18, acy, -1, 0, left);
  arrow(ac + 18, acy, 1, 0, right);
  cv->fillCircle(ac, acy, 4, c565(stick.down ? C_YELLOW : C_LINE));

  // ボタン
  y = 168;
  for (int i = 0; i < BUTTON_COUNT; i++, y += 11)
  {
    Button &b = buttons[i];
    text(x, y, C_DIM, "%s", b.name);
    text(x + 36, y, b.down ? C_GREEN : C_LINE, b.down ? "ON " : "off");
    text(x + 60, y, C_TEXT, "%4d", b.count);
  }

  text(4, 228, C_DIM, "SW1:CENTER  SW2:CLEAR MIN/MAX");
  cv->flush();
}

void setup()
{
  Serial.begin(115200);
  if (!cv->begin(40000000))
    Serial.println("canvas begin failed");
  for (Button &b : buttons)
    pinMode(b.pin, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db); // 0〜約 3.1V を読む
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);
  delay(50);
  // 起動時はスティックに触らない前提で、その位置を中央にする
  ax.calibrate();
  ay.calibrate();
}

void loop()
{
  ax.read();
  ay.read();
  // 斜めでも両方のキーが入るよう、軸ごとに判定する
  right = ax.v >= DIR_ON;
  left = ax.v <= -DIR_ON;
  up = ay.v >= DIR_ON;
  down = ay.v <= -DIR_ON;

  if (updateButton(stick))
  {
    pressX = 4 + 100 + ax.v * 100;
    pressY = 20 + 100 - ay.v * 100;
  }
  if (updateButton(sw1))
  {
    ax.calibrate();
    ay.calibrate();
  }
  if (updateButton(sw2))
  {
    ax.lo = ax.hi = ax.raw;
    ay.lo = ay.hi = ay.raw;
    pressX = pressY = -1;
  }
  updateButton(buttons[3]);

  static uint32_t lastLog = 0;
  if (millis() - lastLog >= 100)
  {
    lastLog = millis();
    Serial.printf("X raw=%4d mv=%4d v=%+.2f | Y raw=%4d mv=%4d v=%+.2f | %c%c%c%c | STICK=%d SW1=%d SW2=%d SW3=%d\n",
                  ax.raw, ax.mv, ax.v, ay.raw, ay.mv, ay.v,
                  up ? 'U' : '-', down ? 'D' : '-', left ? 'L' : '-', right ? 'R' : '-',
                  stick.down, sw1.down, sw2.down, buttons[3].down);
  }

  draw();
}
