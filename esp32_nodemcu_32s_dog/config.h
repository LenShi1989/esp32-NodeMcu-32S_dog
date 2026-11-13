#pragma once

// ===========================================================================
// 開發板：ESP32 NodeMCU-32S（ESP32-WROOM-32 / 38-pin，兩側各 19 腳）
// ===========================================================================
// 腳位參考：docs/NodeMcu-32S_接腳圖-1.png、docs/NodeMcu-32S_接腳圖-2.png
// 排針編號由下往上 1～19；以下為「由上往下」順序
//
// 左排（3V3 在上、5V 在下）：
//   3V3, EN, GPIO36(SVP), GPIO39(SVN), GPIO34, GPIO35, GPIO32, GPIO33,
//   GPIO25, GPIO26, GPIO27, GPIO14, GPIO12, GND, GPIO13,
//   GPIO9(FLASH D2), GPIO10(FLASH D3), GPIO11(FLASH CMD), 5V
//
// 右排（GND 在上、GPIO6 在下）：
//   GND, GPIO23, GPIO22(SCL), GPIO1(TX0), GPIO3(RX0), GPIO21(SDA), GND,
//   GPIO19, GPIO18, GPIO5, GPIO17, GPIO16, GPIO4, GPIO0(BOOT), GPIO2,
//   GPIO15, GPIO8(FLASH D1), GPIO7(FLASH D0), GPIO6(FLASH SCK)
//
// 選用原則：
//   * 四顆 SG90 使用 GPIO25 / 26 / 27 / 14：在左排彼此相鄰、皆可輸出 PWM，
//     且不是 strapping 腳，接線與除錯都最單純。
//   * OLED 使用 GPIO21(SDA) / GPIO22(SCL)：NodeMCU-32S 的預設 I2C 腳位。
//   * GPIO34～39 只能輸入，絕對不可當伺服馬達訊號腳。
//   * GPIO0 / 2 / 5 / 12 / 15 為 strapping 腳，本專案只用 GPIO2 驅動板載 LED。
// ===========================================================================

// ---------------- 開發板 / 韌體資訊 ----------------
#define BOARD_NAME "ESP32 NodeMCU-32S"
#define FW_NAME    "DogBot"
#define FW_VERSION "1.6.0"

// ---------------- OLED SSD1306 (I2C) ----------------
// NodeMCU-32S 右排：GPIO22 = SCL（由上往下第 3 腳）、GPIO21 = SDA（由上往下第 6 腳）
#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1
#define OLED_ADDR   0x3C

// ---------------- SG90 伺服馬達 ----------------
// 編號：0=前左(M1) 1=前右(M2) 2=後左(M3) 3=後右(M4)
// NodeMCU-32S 左排由上往下第 9～12 腳：GPIO25、26、27、14
#define SERVO_PIN_FL 25
#define SERVO_PIN_FR 26
#define SERVO_PIN_RL 27
#define SERVO_PIN_RR 14
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400
#define SERVO_CENTER 90

// ---------------- 板載狀態燈 ----------------
// NodeMCU-32S 右排由上往下第 15 腳的 GPIO2 接藍色 LED，低電位點亮
#define STATUS_LED 2

// ---------------- WiFi ----------------
#define WIFI_CONFIG_FILE "/wifi.txt"
#define AP_SSID_PREFIX   "DogBot-"   // 後方自動接上 MAC 後 4 碼
#define AP_PASSWORD      "12345678"
#define WIFI_RETRY_MS    15000       // STA 斷線後重試間隔（AP 熱點常駐）

// ---------------- 伺服馬達校正 ----------------
#define SERVO_CONFIG_FILE "/servo.cfg"

// ---------------- 四足步態 ----------------
// 一次只擺動一隻腳的「側序步態」（後左 → 前左 → 後右 → 前右）。
// 每條腿只有一顆馬達，沒有膝關節可以抬腿，因此步態必須靠
// 「慢速支撐、快速回擺」讓擺動腳離地，並用左右差速產生轉向。
//
// GAIT_STRIDE_DEG：單邊最大擺幅（度）。太小抬不起腳，太大容易撞機構。
#define GAIT_STRIDE_DEG   24.0f
// 支撐相佔一個週期的比例。0.75 表示同一時間只有一隻腳在擺動，
// 另外三隻腳支撐；小於 0.75 會出現兩隻腳同時擺動、機身較不穩。
#define GAIT_STANCE_FRAC  0.75f
// 步態頻率（Hz）：越大走得越快，建議 1.0～2.0。
#define GAIT_FREQ_MIN     1.2f
#define GAIT_FREQ_MAX     1.6f
// 前進／後退方向相反時改為 -1.0f（左右轉方向也會一起修正）。
#define GAIT_DIR_SIGN     1.0f

// 每顆馬達在「方向控制（行走）」時的擺動幅度上限（度）。
// 可在網頁「SG90 測試」頁個別調整，設定會存到 SPIFFS；
// 預設值等於 GAIT_STRIDE_DEG，數值越大步伐越大。
#define SERVO_AMP_DEFAULT  ((int)(GAIT_STRIDE_DEG))
#define SERVO_AMP_MIN      2
#define SERVO_AMP_MAX      45
// 角度變化速度上限（度/秒）。行走時必須夠快，否則步態會被平滑掉。
#define SERVO_SLEW_WALK   900.0f
#define SERVO_SLEW_POSE   180.0f

// ---------------- 特殊動作 ----------------
// 撒尿：左後腿往前擺動的幅度（度）。90° 為待機，數值越大越往前；
// 若實際擺動方向相反，改成負值即可。
#define PEE_RL_SWING_DEG  32.0f
#define PEE_HOLD_MS       4000
// 自定義動作預設維持時間（毫秒）；0 表示持續保持姿勢。
#define CUSTOM_HOLD_MS    3000

// ===========================================================================
// 編譯期腳位檢查：接錯腳位會直接編譯失敗，而不是上電後才發現
// ===========================================================================
#define CONFIG_IS_INPUT_ONLY(p) ((p) >= 34 && (p) <= 39)

#if CONFIG_IS_INPUT_ONLY(SERVO_PIN_FL) || CONFIG_IS_INPUT_ONLY(SERVO_PIN_FR) || \
    CONFIG_IS_INPUT_ONLY(SERVO_PIN_RL) || CONFIG_IS_INPUT_ONLY(SERVO_PIN_RR)
#error "伺服馬達不可使用 GPIO34~39：NodeMCU-32S 的這些腳位僅能輸入。"
#endif

#if CONFIG_IS_INPUT_ONLY(OLED_SDA) || CONFIG_IS_INPUT_ONLY(OLED_SCL)
#error "OLED I2C 不可使用 GPIO34~39：NodeMCU-32S 的這些腳位僅能輸入。"
#endif

static_assert(SERVO_PIN_FL != SERVO_PIN_FR, "伺服馬達腳位重複：FL / FR");
static_assert(SERVO_PIN_FL != SERVO_PIN_RL, "伺服馬達腳位重複：FL / RL");
static_assert(SERVO_PIN_FL != SERVO_PIN_RR, "伺服馬達腳位重複：FL / RR");
static_assert(SERVO_PIN_FR != SERVO_PIN_RL, "伺服馬達腳位重複：FR / RL");
static_assert(SERVO_PIN_FR != SERVO_PIN_RR, "伺服馬達腳位重複：FR / RR");
static_assert(SERVO_PIN_RL != SERVO_PIN_RR, "伺服馬達腳位重複：RL / RR");
static_assert(OLED_SDA != OLED_SCL, "OLED SDA / SCL 不可使用同一腳位");

static_assert(GAIT_STRIDE_DEG >= 8.0f && GAIT_STRIDE_DEG <= 45.0f,
              "GAIT_STRIDE_DEG 建議設定在 8~45 度");
static_assert(GAIT_STANCE_FRAC > 0.5f && GAIT_STANCE_FRAC < 1.0f,
              "GAIT_STANCE_FRAC 必須介於 0.5 與 1.0 之間");
static_assert(GAIT_FREQ_MIN > 0.0f && GAIT_FREQ_MAX >= GAIT_FREQ_MIN,
              "GAIT_FREQ_MIN / GAIT_FREQ_MAX 設定錯誤");
