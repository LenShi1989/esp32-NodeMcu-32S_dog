#include <ESP32Servo.h> // 載入 ESP32 專用的 Servo 函式庫

Servo myservo; // 建立 Servo 物件

int servoPin = 16; // 訊號線接到 GPIO 16

int pos = 0; // 定義舵機轉動位置

void setup()
{
  Serial.begin(115200); // 設定序列埠傳輸速率
  myservo.attach(servoPin);
}

void loop()
{
  // myservo.write(0); // 旋轉到 0 度（歸零）
  // delay(1000);
  // myservo.write(90); // 旋轉到 90 度
  // delay(1000);
  // myservo.write(180); // 旋轉到 180 度
  // delay(1000);
  // myservo.write(90); // 轉回 90 度
  // delay(1000);

  // 0到180旋轉舵機，每次延時15毫秒
  for (pos = 0; pos < 180; pos += 1)
  {
    myservo.write(pos);
    delay(15);
  }
  // 180到0旋轉舵機，每次延時15毫秒
  for (pos = 180; pos >= 1; pos -= 1)
  {
    myservo.write(pos);
    delay(15);
  }
}