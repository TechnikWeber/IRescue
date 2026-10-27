# Parts list

All standard hobby parts, roughly **€10–20** in total (without the ESP32).
Much of it often comes with Arduino starter kits.

## Required

| Qty | Part | Purpose | approx. price | Note |
|---|---|---|---|---|
| 1 | **ESP32 DevKit-C** | Controller | – | |
| 1 | **IR LED 940 nm** (e.g. TSAL6200, or any 5 mm 940 nm) | Transmit | ~€0.30 | 940 nm, **not** 850 nm (the TSOP receiver sees that less well) |
| 1 | **NPN transistor BC337** (or 2N2222) | Switches the LED | ~€0.10 | Alternative: logic-level MOSFET 2N7000 |
| 1 | **Resistor 1 kΩ** | Base resistor | ~€0.02 | Between GPIO and transistor base |
| 1 | **Resistor ~33 Ω** (¼ W) | Limits the LED current | ~€0.02 | ~110 mA pulsed → good range |
| 1 | **Light sensor: TEMT6000 breakout** | Detects the blink | ~€1–2 | Fast enough; alternatives below |
| – | **Breadboard + jumper wires** | Assembly | ~€5 | |
| 1 | **USB data cable** | Flashing/power | – | Must carry data, not just charge |

## Optional

| Qty | Part | Purpose | approx. price | Note |
|---|---|---|---|---|
| 1 | **IR receiver TSOP38238** (or VS1838B) | Learn remotes | ~€0.50 | Only needed for learn mode |

## Sensor alternatives (instead of the TEMT6000)

- **Phototransistor** (e.g. BPW40) + 10 kΩ to GND, signal to GPIO34 — cheap and fast.
- **LDR/photoresistor** + 10 kΩ voltage divider — works, but may be **too slow**
  for the short blink. Only use it if nothing else is at hand.

## Important

- **Never** connect the IR LED straight to a GPIO — always through the transistor
  (a GPIO delivers too little current, the range would be poor).
- Light sensor on **GPIO34** (ADC1). ADC2 pins do not work while Wi-Fi is active.

Wiring: see [HARDWARE.md](HARDWARE.md).
