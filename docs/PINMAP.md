# ESP32 NodeMCU-32S 腳位對照

本專案目標板為 **ESP32 NodeMCU-32S（ESP32-WROOM-32 / 38-pin）**，
兩側各 19 支排針。腳位圖請見：

- [`NodeMcu-32S_接腳圖-1.png`](NodeMcu-32S_接腳圖-1.png) — 38-pin NodeMCU-32S 腳位圖（**本專案依此圖**）
- [`NodeMcu-32S_接腳圖-2.png`](NodeMcu-32S_接腳圖-2.png) — 同一塊 38-pin 板，以顏色標示 GPIO 可用性
- [`NodeMcu-32S_接腳說明.pdf`](NodeMcu-32S_接腳說明.pdf)
- [`pinmap.svg`](pinmap.svg) — **本專案腳位標示圖**（板子正視圖，標出 M1～M4 與各接線腳位）

> 圖 1 的排針編號是**由下往上 1～19**；本文件同時標出「由上往下第 N 腳」，
> 方便直接對照實體排針。

---

## 本專案使用腳位

| 功能 | 常數 | GPIO | 左/右排 | 由上往下 | 排針編號 | 備註 |
| --- | --- | --- | --- | --- | --- | --- |
| SG90 M1 前左 | `SERVO_PIN_FL` | **25** | 左排 | 第 9 腳 | #11 | DAC1 / ADC18 |
| SG90 M2 前右 | `SERVO_PIN_FR` | **26** | 左排 | 第 10 腳 | #10 | DAC2 / ADC19 |
| SG90 M3 後左 | `SERVO_PIN_RL` | **27** | 左排 | 第 11 腳 | #9 | ADC17 / TOUCH7 |
| SG90 M4 後右 | `SERVO_PIN_RR` | **14** | 左排 | 第 12 腳 | #8 | ADC16 / TOUCH6 / HSPI SCK |
| OLED SDA | `OLED_SDA` | **21** | 右排 | 第 6 腳 | #14 | Wire SDA |
| OLED SCL | `OLED_SCL` | **22** | 右排 | 第 3 腳 | #17 | Wire SCL |
| 板載狀態燈 | `STATUS_LED` | **2** | 右排 | 第 15 腳 | #5 | 藍色 LED，低電位點亮 |

四顆伺服馬達刻意選在左排 **由上往下第 9～12 腳（GPIO25、26、27、14）**：
位置相鄰、皆可輸出 PWM、不是 strapping 腳，也沒有任何開機特殊狀態。

---

## 左排完整腳位（3V3 在上、5V 在下，共 19 腳）

| 由上往下 | 排針編號 | 標籤 | GPIO | 本專案用途 | 備註 |
| --- | --- | --- | --- | --- | --- |
| 1 | #19 | 3V3 | — | **OLED VCC** | 3.3V 輸出，不可接 5V 負載 |
| 2 | #18 | EN | — | — | 重置致能（高電位工作） |
| 3 | #17 | SVP | 36 | — | **僅輸入** / ADC0 |
| 4 | #16 | SVN | 39 | — | **僅輸入** / ADC3 |
| 5 | #15 | D34 | 34 | — | **僅輸入** / ADC6 |
| 6 | #14 | D35 | 35 | — | **僅輸入** / ADC7 |
| 7 | #13 | D32 | 32 | 預留 | ADC4 / TOUCH9 |
| 8 | #12 | D33 | 33 | 預留 | ADC5 / TOUCH8 |
| 9 | #11 | D25 | 25 | **SG90 M1 前左** | DAC1 / ADC18 |
| 10 | #10 | D26 | 26 | **SG90 M2 前右** | DAC2 / ADC19 |
| 11 | #9 | D27 | 27 | **SG90 M3 後左** | ADC17 / TOUCH7 |
| 12 | #8 | D14 | 14 | **SG90 M4 後右** | ADC16 / TOUCH6 / TMS / HSPI SCK |
| 13 | #7 | D12 | 12 | 預留（避免輸出） | TDI / strapping / HSPI MISO |
| 14 | #6 | GND | — | 共地（OLED GND） | |
| 15 | #5 | D13 | 13 | 預留 | TCK / ADC14 / TOUCH4 / HSPI MOSI |
| 16 | #4 | D9 | 9 | 不可使用 | FLASH D2 |
| 17 | #3 | D10 | 10 | 不可使用 | FLASH D3 |
| 18 | #2 | D11 | 11 | 不可使用 | FLASH CMD |
| 19 | #1 | 5V | — | 不接馬達電源 | 5V 輸入 / 輸出視供電方式而定 |

---

## 右排完整腳位（GND 在上、GPIO6 在下，共 19 腳）

| 由上往下 | 排針編號 | 標籤 | GPIO | 本專案用途 | 備註 |
| --- | --- | --- | --- | --- | --- |
| 1 | #19 | GND | — | 共地 | |
| 2 | #18 | D23 | 23 | 預留 | VSPI MOSI |
| 3 | #17 | SCL | 22 | **OLED SCL** | Wire SCL（預設 I2C 時鐘） |
| 4 | #16 | TX0 | 1 | 序列監控 | UART0 TX |
| 5 | #15 | RX0 | 3 | 序列監控 | UART0 RX |
| 6 | #14 | SDA | 21 | **OLED SDA** | Wire SDA（預設 I2C 資料） |
| 7 | #13 | GND | — | 共地 | |
| 8 | #12 | D19 | 19 | 預留 | VSPI MISO |
| 9 | #11 | D18 | 18 | 預留 | VSPI SCK |
| 10 | #10 | D5 | 5 | 預留（避免輸出） | VSPI SS / strapping / 內部上拉 |
| 11 | #9 | D17 | 17 | 預留 | — |
| 12 | #8 | D16 | 16 | 預留 | — |
| 13 | #7 | D4 | 4 | 預留 | ADC10 / TOUCH0 / 內部下拉 |
| 14 | #6 | D0 | 0 | 預留（避免輸出） | BOOT / ADC11 / TOUCH1 / strapping |
| 15 | #5 | D2 | 2 | **板載 LED** | ADC12 / TOUCH2 / 內部下拉 / strapping |
| 16 | #4 | D15 | 15 | 預留（避免輸出） | HSPI SS / ADC13 / TOUCH3 / TDO / strapping |
| 17 | #3 | D8 | 8 | 不可使用 | FLASH D1 |
| 18 | #2 | D7 | 7 | 不可使用 | FLASH D0 |
| 19 | #1 | D6 | 6 | 不可使用 | FLASH SCK |

---

## 使用限制

| 腳位 | 限制 |
| --- | --- |
| **GPIO34 / 35 / 36 / 39** | **僅能輸入**，沒有內部上拉／下拉，不可驅動伺服馬達或 LED。`config.h` 會以 `#error` 阻擋。 |
| **GPIO6～11** | 連接到內部 SPI Flash，不可使用（圖 1 標為 FLASH SCK / D0~D3 / CMD）。 |
| **GPIO0** | strapping（BOOT），開機時低電位會進入下載模式。 |
| **GPIO2** | strapping，且連接板載 LED；本專案僅作為狀態燈。 |
| **GPIO5 / 12 / 15** | strapping，開機瞬間狀態會影響 Flash 電壓／開機模式，避免當輸出。 |
| **GPIO1 / 3** | UART0，保留給序列埠監控與燒錄。 |
| **GPIO16 / 17** | 可自由使用（本專案預留）。 |
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

若手邊是 30-pin 版本的 NodeMCU-32S（每側 15 腳，不引出 GPIO6～11），
GPIO 編號完全相同，本專案的腳位設定可直接沿用；只有排針的實體位置需要改看該板的圖。
