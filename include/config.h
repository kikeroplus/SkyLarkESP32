#pragma once
// ピンと液晶の設定。液晶・スイッチ・アンプは Livico と同じ配線

// ---- 液晶 (ST7789 240x320) ----
#define TFT_SCK  12 // SCL
#define TFT_MOSI 11 // SDA
#define TFT_MISO -1
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define TFT_IPS      true
#define TFT_ROTATION 0 // 縦置き 240x320。上下が逆なら 2

// ---- スイッチ (片側 GND、押すと LOW) ----
// GPIO13 は SPI2 の MISO の決まったピンなので、pinMode は液晶の begin のあとでする
#define SW1_PIN 13
#define SW2_PIN 14
#define SW3_PIN 1

// ---- アンプ (MAX98357A。Vin 5V、SD・GAIN は未接続) ----
#define AMP_BCLK 15
#define AMP_LRC  16
#define AMP_DIN  17

// ---- ジョイスティック (GND / 5V / VRX / VRY / SW の 5 端子のもの) ----
// 「5V」の端子には 3.3V をつなぐ。5V をつなぐと VRX・VRY に 5V まで出て、ESP32 の入力 (3.3V まで) を壊す
// VRX・VRY は ADC1 (GPIO1〜10) につなぐ。ADC2 は WiFi を使うと読めなくなる
#define JOY_X_PIN  4 // VRX
#define JOY_Y_PIN  5 // VRY
#define JOY_SW_PIN 6 // SW。押すと GND につながる。内部プルアップで押すと LOW
#define JOY_X_INVERT true // 右に倒して値が減るなら true
#define JOY_Y_INVERT false // 上に倒して値が増えるなら true (上を正にそろえる)
// 取り付けの向きで上下と左右が入れ替わるときは true (ゲームだけ。試験 joytest はそのままの値を出す)。
// 入れ替えたあと向きが逆なら、ゲームの左右は JOY_Y_INVERT、上下は JOY_X_INVERT で直す
#define JOY_SWAP_XY true
