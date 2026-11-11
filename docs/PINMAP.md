# ESP32 NodeMCU-32S 腳位對照

本專案目標板為 **ESP32 NodeMCU-32S（ESP32-WROOM-32 / 30-pin）**。
腳位圖請見：

- [`NodeMcu-32S_接腳圖-2.png`](NodeMcu-32S_接腳圖-2.png) — 30-pin NodeMCU-32S（本專案採用）
- [`NodeMcu-32S_接腳圖-1.png`](NodeMcu-32S_接腳圖-1.png) — 36 GPIO DevKit 版本
- [`NodeMcu-32S_接腳說明.pdf`](NodeMcu-32S_接腳說明.pdf)

---

## 本專案使用腳位

| 功能 | 常數 | GPIO | 排針位置 | 備註 |
| --- | --- | --- | --- | --- |
| SG90 M1 前左 | `SERVO_PIN_FL` | **25** | 左排第 8 腳 | LEDC PWM 50 Hz |
| SG90 M2 前右 | `SERVO_PIN_FR` | **26** | 左排第 9 腳 | LEDC PWM 50 Hz |
| SG90 M3 後左 | `SERVO_PIN_RL` | **27** | 左排第 10 腳 | LEDC PWM 50 Hz |
| SG90 M4 後右 | `SERVO_PIN_RR` | **14** | 左排第 11 腳 | LEDC PWM 50 Hz |
| OLED SDA | `OLED_SDA` | **21** | 右排第 5 腳 | I2C 資料 |
| OLED SCL | `OLED_SCL` | **22** | 右排第 2 腳 | I2C 時鐘 |
| 板載狀態燈 | `STATUS_LED` | **2** | 右排第 12 腳 | 藍色 LED，低電位點亮 |

> 左排由 **EN = 第 1 腳**往下數；右排由 **GPIO23 = 第 1 腳**往下數。

四顆伺服馬達刻意選在左排 **第 8～11 腳（GPIO25、26、27、14）**：
位置相鄰、皆可輸出 PWM、不是 strapping 腳，也沒有任何開機特殊狀態。

---

## 左排完整腳位（EN 在上）

| # | 標籤 | GPIO | 本專案用途 | 備註 |
| --- | --- | --- | --- | --- |
| 1 | EN | — | — | 重置致能（高電位工作） |
| 2 | VP | 36 | — | **僅輸入** |
| 3 | VN | 39 | — | **僅輸入** |
| 4 | D34 | 34 | — | **僅輸入** |
| 5 | D35 | 35 | — | **僅輸入** |
| 6 | D32 | 32 | 預留 | 可輸出 / ADC1 / Touch |
| 7 | D33 | 33 | 預留 | 可輸出 / ADC1 / Touch |
| 8 | D25 | 25 | **SG90 M1 前左** | DAC1 / ADC2 |
| 9 | D26 | 26 | **SG90 M2 前右** | DAC2 / ADC2 |
| 10 | D27 | 27 | **SG90 M3 後左** | ADC2 / Touch7 |
| 11 | D14 | 14 | **SG90 M4 後右** | ADC2 / Touch6 / HSPI CLK |
| 12 | D12 | 12 | 預留（避免輸出） | strapping（MTDI） |
| 13 | D13 | 13 | 預留 | ADC2 / Touch4 / HSPI MOSI |
| 14 | GND | — | 共地 | |
| 15 | VIN | — | 不接馬達電源 | 5V 輸入 / 輸出視供電方式而定 |

---

## 右排完整腳位（GPIO23 在上）

| # | 標籤 | GPIO | 本專案用途 | 備註 |
| --- | --- | --- | --- | --- |
| 1 | D23 | 23 | 預留 | VSPI MOSI |
| 2 | SCL | 22 | **OLED SCL** | 預設 I2C 時鐘 |
| 3 | TX0 | 1 | 序列監控 | UART0 TX |
| 4 | RX0 | 3 | 序列監控 | UART0 RX |
| 5 | SDA | 21 | **OLED SDA** | 預設 I2C 資料 |
| 6 | D19 | 19 | 預留 | VSPI MISO |
| 7 | D18 | 18 | 預留 | VSPI CLK |
| 8 | D5 | 5 | 預留（避免輸出） | strapping / VSPI CS0 |
| 9 | TX2 | 17 | 預留 | UART2 TX |
| 10 | RX2 | 16 | 預留 | UART2 RX |
| 11 | D4 | 4 | 預留 | ADC2 / Touch0 |
| 12 | D2 | 2 | **板載 LED** | strapping / ADC2 / Touch2 |
| 13 | D15 | 15 | 預留（避免輸出） | strapping / HSPI CS0 |
| 14 | GND | — | 共地 | |
| 15 | 3V3 | — | **OLED VCC** | 3.3V 輸出，不可接 5V 負載 |

---

## 使用限制

| 腳位 | 限制 |
| --- | --- |
| **GPIO34 / 35 / 36 / 39** | **僅能輸入**，沒有內部上拉／下拉，不可驅動伺服馬達或 LED。`config.h` 會以 `#error` 阻擋。 |
| **GPIO6～11** | 連接到內部 SPI Flash，不可使用。 |
| **GPIO0** | strapping（BOOT），開機時低電位會進入下載模式。 |
| **GPIO2** | strapping，且連接板載 LED；本專案僅作為狀態燈。 |
| **GPIO5 / 12 / 15** | strapping，開機瞬間狀態會影響 Flash 電壓／開機模式，避免當輸出。 |
| **GPIO1 / 3** | UART0，保留給序列埠監控與燒錄。 |
| **GPIO16 / 17** | UART2，可自由使用。 |
| ADC2 腳位 | WiFi 開啟時 ADC2 無法使用；本專案未使用 ADC。 |

---

## 變更腳位

編輯 `esp32_nodemcu_32s_dog/config.h`：

```cpp
#define SERVO_PIN_FL 25
#define SERVO_PIN_FR 26
#define SERVO_PIN_RL 27
#define SERVO_PIN_RR 14

#define OLED_SDA 21
#define OLED_SCL 22
#define STATUS_LED 2
```

可用的替代輸出腳位：**GPIO4、16、17、18、19、23、32、33**。
變更後 `config.h` 的編譯期檢查會自動確認：

1. 沒有使用 GPIO34～39（僅輸入）。
2. 四顆伺服馬達與 I2C 腳位沒有重複。

若使用 38-pin 版本的 NodeMCU-32S，GPIO 編號與 30-pin 版本相同，
本專案的腳位設定可直接沿用。
