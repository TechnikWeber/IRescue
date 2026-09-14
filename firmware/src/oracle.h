#pragma once
#include <Arduino.h>

// Watches the lamp through a light sensor and reports when it briefly blinks
// (dips in brightness) — that is how the lamp signals "I understood a command".
class Oracle {
 public:
  void begin(int pin) {
    pin_ = pin;
    analogReadResolution(12);                 // 0..4095
    analogSetPinAttenuation(pin_, ADC_11db);  // full ~0..3.3V range
  }

  int read() { last_ = analogRead(pin_); return last_; }
  int last() const { return last_; }
  float baseline() const { return baseline_; }
  bool calibrated() const { return calibrated_; }
  void setSensitivity(float s) { sensitivity_ = constrain(s, 0.05f, 0.9f); }
  float sensitivity() const { return sensitivity_; }

  // Sample the ambient (lamp-on) level so we know what "normal" looks like.
  void calibrate(uint32_t ms = 600) {
    double sum = 0; uint32_t n = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < ms) { sum += analogRead(pin_); n++; delayMicroseconds(500); }
    baseline_ = n ? (float)(sum / n) : 0;
    calibrated_ = baseline_ > 0;
  }

  // Watch for a brightness dip for up to windowMs. Returns true on a dip and
  // reports how deep it went. Non-async code: the AsyncWebServer keeps running
  // on its own task, so blocking here is fine.
  bool watch(uint32_t windowMs, int &minValOut) {
    const float threshold = baseline_ * (1.0f - sensitivity_);
    int minVal = 4095;
    uint32_t t0 = millis();
    while (millis() - t0 < windowMs) {
      int v = analogRead(pin_);
      last_ = v;
      if (v < minVal) minVal = v;
      if (calibrated_ && v < threshold) { minValOut = v; return true; }
      delayMicroseconds(300);  // ~2-3 kHz sampling
    }
    minValOut = minVal;
    return false;
  }

 private:
  int   pin_ = -1;
  int   last_ = 0;
  float baseline_ = 0;
  float sensitivity_ = 0.30f;  // dip must be >=30% below baseline
  bool  calibrated_ = false;
};
