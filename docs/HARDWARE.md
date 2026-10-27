# Hardware & wiring

Parts list: [PARTS.md](PARTS.md).

![Wiring](wiring.svg)

## Wiring as text

```
IR transmitter:
  ESP32 5V ───[ 33 Ω ]─── IR LED anode
  IR LED cathode ──────── collector  (BC337)
  ESP32 GPIO4 ──[ 1 kΩ ]── base      (BC337)
  emitter (BC337) ─────── ESP32 GND

Light sensor (TEMT6000):
  VCC ── ESP32 3V3
  GND ── ESP32 GND
  SIG ── ESP32 GPIO34      (ADC1, input)

IR receiver (TSOP38238, optional, learn mode only):
  VCC ── ESP32 3V3
  GND ── ESP32 GND
  OUT ── ESP32 GPIO27
```

## Why this way?

- **Transistor instead of the LED straight on the pin:** a GPIO delivers only
  ~20 mA. Through the BC337 and the 33 Ω resistor, ~110 mA flow through the LED in
  pulses → much more range. The 1 kΩ resistor limits the base current.
- **GPIO34 for the sensor:** it is an ADC1 pin. ADC2 pins are locked while Wi-Fi is
  active — and Wi-Fi is always on here (access point).
- **Alternative transistor:** a logic-level MOSFET (e.g. 2N7000), then gate instead
  of base; the 1 kΩ resistor can stay.

## Changing pins

All pins are in [`firmware/src/config.h`](../firmware/src/config.h):
`PIN_IR_SEND`, `PIN_LIGHT`, `PIN_IR_RECV`.
