#pragma once
// ---- Pin map (ESP32 DevKit-C) --------------------------------------------
// IR LED: drive via transistor, NOT straight from the pin. See docs/KONZEPT.md.
#define PIN_IR_SEND   4     // -> 1k -> base of NPN (or gate of logic-level MOSFET)
#define PIN_LIGHT     34    // ADC1 (input-only) light sensor. ADC2 is unusable with WiFi.
#define PIN_IR_RECV   27    // TSOP38238 OUT (only needed for learning remotes)

// ---- Access point (open, always on) --------------------------------------
// No password: connect and a captive portal pops the UI up automatically.
#define AP_SSID_PREFIX "IRescue-"

// ---- IR timing -----------------------------------------------------------
// Time we watch the lamp after each frame. The lamp reacts with a delay, so
// this also has to cover that latency. Bigger = safer detection but slower.
#define DEFAULT_POST_GAP_MS  110

// How many recent codes to keep around a detected blink, for the slow re-test.
#define HIT_WINDOW  8
