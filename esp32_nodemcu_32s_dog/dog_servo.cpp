#include "dog_servo.h"
#include "config.h"

// ESP32 core 2.x 使用 ledcSetup + ledcAttachPin（以 channel 控制）
// ESP32 core 3.x 使用 ledcAttach + ledcWrite（以 pin 控制）
#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

namespace {

// 0=FL 1=FR 2=RL 3=RR
const uint8_t LEG_PINS[4] = {SERVO_PIN_FL, SERVO_PIN_FR, SERVO_PIN_RL, SERVO_PIN_RR};
const bool    LEFT_LEG[4] = {true, false, true, false};

// 側序步態相位：後左 → 前左 → 後右 → 前右（一次只擺動一隻腳）。
// 相位差 0.25，搭配 GAIT_STANCE_FRAC = 0.75 時，任一時刻剛好一隻腳在擺動。
const float LEG_PHASE[4] = {0.50f, 0.00f, 0.75f, 0.25f};  // FL, FR, RL, RR

// 姿勢角度（可依實際機構調整）
const float POSE_STAND[4] = {90, 90, 90, 90};
const float POSE_SIT[4]   = {112, 112, 68, 68};
const float POSE_LIE[4]   = {136, 136, 44, 44};

inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// 0..1 的平滑曲線（兩端速度為 0）
inline float smoothstep01(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// 非對稱步態波形：
//   支撐相（0 ~ GAIT_STANCE_FRAC）：+1 → -1，腳著地慢慢往後推，推動身體前進
//   擺動相（其餘時間）：-1 → +1，腳快速回到前方
// 若改用對稱的 sin()，前後擺動的位移會互相抵消，狗狗只會原地抖動。
inline float gaitWave(float p) {
  if (p < GAIT_STANCE_FRAC) {
    return 1.0f - 2.0f * (p / GAIT_STANCE_FRAC);
  }
  return -1.0f + 2.0f * smoothstep01((p - GAIT_STANCE_FRAC) /
                                     (1.0f - GAIT_STANCE_FRAC));
}

void servoAttach(uint8_t index, uint8_t pin) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)index;
  ledcAttach(pin, 50, 16);
#else
  (void)pin;
  ledcSetup(index, 50, 16);
  ledcAttachPin(LEG_PINS[index], index);
#endif
}

void servoWrite(uint8_t index, uint8_t pin, uint16_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)index;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(index, duty);
#endif
}

uint16_t angleToDuty(float angle) {
  angle = clampf(angle, 0.0f, 180.0f);
  float pulse = SERVO_MIN_US + (angle / 180.0f) * (float)(SERVO_MAX_US - SERVO_MIN_US);
  return (uint16_t)(pulse * 65535.0f / 20000.0f + 0.5f);
}

}  // namespace

void DogServo::begin() {
  for (uint8_t i = 0; i < 4; i++) {
    servoAttach(i, LEG_PINS[i]);
    servoWrite(i, LEG_PINS[i], angleToDuty(SERVO_CENTER));
    _current[i] = SERVO_CENTER;
    _target[i]  = SERVO_CENTER;
    _written[i] = SERVO_CENTER;
  }
  _mode = MODE_STAND;
  _phase = 0.0f;
  _walkGain = 0.0f;
  _lastUpdate = millis();
}

void DogServo::update() {
  uint32_t now = millis();
  uint32_t elapsed = now - _lastUpdate;
  if (elapsed < 8) return;
  _lastUpdate = now;

  float dt = elapsed / 1000.0f;
  if (dt > 0.12f) dt = 0.12f;

  // 舉手／撒嬌時間到後自動回待機
  if ((_mode == MODE_WAVE || _mode == MODE_WAG) &&
      _holdUntil && (int32_t)(now - _holdUntil) >= 0) {
    _mode = MODE_STAND;
    _holdUntil = 0;
  }

  // 只有行走模式需要起步淡入
  if (_mode != MODE_WALK) _walkGain = 0.0f;

  switch (_mode) {
    case MODE_WALK: {
      float mag = sqrtf(_forward * _forward + _turn * _turn);
      mag = clampf(mag, 0.0f, 1.0f);

      float freq = GAIT_FREQ_MIN + (GAIT_FREQ_MAX - GAIT_FREQ_MIN) * mag;
      _phase += freq * dt;
      _phase -= floorf(_phase);

      _walkGain = clampf(_walkGain + dt * 4.0f, 0.0f, 1.0f);

      // 前進／後退：四隻腳往同一個方向擺動。
      // 左右轉：左腳與右腳的擺幅相反（差速），讓機身原地旋轉。
      // 搖桿過半即達到最大擺幅，避免小幅度擺動抬不起腳。
      float fwdAmp  = GAIT_STRIDE_DEG * GAIT_DIR_SIGN *
                      clampf(_forward * 1.5f, -1.0f, 1.0f);
      float turnAmp = GAIT_STRIDE_DEG * GAIT_DIR_SIGN *
                      clampf(_turn * 1.5f, -1.0f, 1.0f);

      for (uint8_t i = 0; i < 4; i++) {
        float p = _phase + LEG_PHASE[i];
        p -= floorf(p);

        float amp = fwdAmp + (LEFT_LEG[i] ? turnAmp : -turnAmp);
        amp = clampf(amp, -GAIT_STRIDE_DEG, GAIT_STRIDE_DEG);

        _target[i] = SERVO_CENTER + amp * _walkGain * gaitWave(p);
      }
      break;
    }

    case MODE_WAVE: {
      float w = sinf(now * 0.012f) * 22.0f;
      _target[0] = SERVO_CENTER;
      _target[2] = SERVO_CENTER;
      _target[3] = SERVO_CENTER;
      _target[1] = 122.0f + w;                     // 前右腳舉起揮動
      break;
    }

    case MODE_WAG: {
      float w = sinf(now * 0.014f);
      _target[0] = SERVO_CENTER + 8.0f * w;
      _target[1] = SERVO_CENTER - 8.0f * w;
      _target[2] = SERVO_CENTER - 14.0f * w;
      _target[3] = SERVO_CENTER + 14.0f * w;
      break;
    }

    case MODE_SIT:
      for (uint8_t i = 0; i < 4; i++) _target[i] = POSE_SIT[i];
      break;

    case MODE_LIE:
      for (uint8_t i = 0; i < 4; i++) _target[i] = POSE_LIE[i];
      break;

    case MODE_MANUAL:
      // 由 setManual() / centerAll() 直接指定 _target
      break;

    default:
      for (uint8_t i = 0; i < 4; i++) _target[i] = POSE_STAND[i];
      break;
  }

  // 限速平滑移動，避免 SG90 瞬間跳動。
  // 行走時上限必須遠高於步態峰值速度，否則非對稱步態會被壓成小幅度擺動。
  float maxStep = ((_mode == MODE_WALK) ? SERVO_SLEW_WALK : SERVO_SLEW_POSE) * dt;
  for (uint8_t i = 0; i < 4; i++) {
    _target[i] = clampf(_target[i], 0.0f, 180.0f);
    float diff = _target[i] - _current[i];
    if (fabsf(diff) <= maxStep) {
      _current[i] = _target[i];
    } else {
      _current[i] += (diff > 0.0f) ? maxStep : -maxStep;
    }

    int value = (int)(_current[i] + 0.5f);
    if (value != _written[i]) {
      // 套用校正：以 90° 為中心反轉，再加上微調
      float physical = 90.0f + ((float)value - 90.0f) * (_rev[i] ? -1.0f : 1.0f) + (float)_trim[i];
      servoWrite(i, LEG_PINS[i], angleToDuty(physical));
      _written[i] = value;
    }
  }
}

void DogServo::setWalk(float forward, float turn) {
  _forward = clampf(forward, -1.0f, 1.0f);
  _turn    = clampf(turn, -1.0f, 1.0f);

  if (fabsf(_forward) < 0.08f && fabsf(_turn) < 0.08f) {
    _forward = 0.0f;
    _turn = 0.0f;
    if (_mode == MODE_WALK) _mode = MODE_STAND;
    return;
  }

  // 從待機／坐姿進入行走時，重新從相位 0 起步並淡入
  if (_mode != MODE_WALK) {
    _phase = 0.0f;
    _walkGain = 0.0f;
  }

  _mode = MODE_WALK;
  _holdUntil = 0;
}

bool DogServo::action(const String &name) {
  String a = name;
  a.trim();
  a.toLowerCase();

  _forward = 0.0f;
  _turn = 0.0f;
  _holdUntil = 0;

  if (a == "stop" || a == "stand") {
    _mode = MODE_STAND;
  } else if (a == "sit") {
    _mode = MODE_SIT;
  } else if (a == "lie" || a == "down") {
    _mode = MODE_LIE;
  } else if (a == "wave" || a == "hand") {
    _mode = MODE_WAVE;
    _holdUntil = millis() + 3000;
  } else if (a == "wag" || a == "cute") {
    _mode = MODE_WAG;
    _holdUntil = millis() + 4000;
  } else {
    return false;
  }
  return true;
}

void DogServo::setManual(uint8_t channel, int angle) {
  if (channel > 3) return;
  _mode = MODE_MANUAL;
  _holdUntil = 0;
  _forward = 0.0f;
  _turn = 0.0f;
  _target[channel] = clampf((float)angle, 0.0f, 180.0f);
}

void DogServo::centerAll() {
  _mode = MODE_MANUAL;
  _holdUntil = 0;
  _forward = 0.0f;
  _turn = 0.0f;
  for (uint8_t i = 0; i < 4; i++) _target[i] = SERVO_CENTER;
}

void DogServo::setCalibration(uint8_t channel, int trim, bool reverse) {
  if (channel > 3) return;
  _trim[channel] = constrain(trim, -45, 45);
  _rev[channel] = reverse;
  _written[channel] = -1;  // 強制下一次 update 重新輸出
}

int DogServo::trim(uint8_t channel) const {
  return channel > 3 ? 0 : _trim[channel];
}

bool DogServo::reversed(uint8_t channel) const {
  return channel > 3 ? false : _rev[channel];
}

int DogServo::angle(uint8_t channel) const {
  if (channel > 3) return SERVO_CENTER;
  return (int)(_current[channel] + 0.5f);
}

const char *DogServo::modeName() const {
  switch (_mode) {
    case MODE_WALK:   return "walk";
    case MODE_SIT:    return "sit";
    case MODE_LIE:    return "lie";
    case MODE_WAVE:   return "wave";
    case MODE_WAG:    return "wag";
    case MODE_MANUAL: return "manual";
    default:          return "stand";
  }
}
