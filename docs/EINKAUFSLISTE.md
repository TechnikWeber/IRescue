# Einkaufsliste

Alles Standard-Bastelteile, zusammen grob **10–20 €** (ohne den ESP32).
Vieles ist oft in Arduino-Startersets schon dabei.

## Pflicht

| Menge | Teil | Zweck | ca. Preis | Hinweis |
|---|---|---|---|---|
| 1 | **ESP32 DevKit-C** | Steuerung | – | schon vorhanden |
| 1 | **IR-LED 940 nm** (z. B. TSAL6200, oder 5 mm 940 nm) | Senden | ~0,30 € | 940 nm, **nicht** 850 nm (das sieht der TSOP-Empfänger schlechter) |
| 1 | **NPN-Transistor BC337** (oder 2N2222) | schaltet die LED | ~0,10 € | Alternative: Logic-Level-MOSFET 2N7000 |
| 1 | **Widerstand 1 kΩ** | Basisvorwiderstand | ~0,02 € | zwischen GPIO und Transistor-Basis |
| 1 | **Widerstand ~33 Ω** (¼ W) | begrenzt den LED-Strom | ~0,02 € | ~110 mA gepulst → gute Reichweite |
| 1 | **Lichtsensor: TEMT6000-Breakout** | erkennt das Blinken | ~1–2 € | schnell genug; Alternative unten |
| – | **Breadboard + Jumperkabel** | Aufbau | ~5 € | Steckbrett |
| 1 | **USB-Datenkabel** | Flashen/Strom | – | muss Daten können, nicht nur laden |

## Optional

| Menge | Teil | Zweck | ca. Preis | Hinweis |
|---|---|---|---|---|
| 1 | **IR-Empfänger TSOP38238** (oder VS1838B) | Fernbedienungen anlernen | ~0,50 € | nur für den Anlern-Modus nötig |

## Sensor-Alternativen (statt TEMT6000)

- **Fototransistor** (z. B. BPW40) + 10 kΩ nach GND, Signal an GPIO34 — günstig, schnell.
- **LDR/Fotowiderstand** + 10 kΩ Spannungsteiler — geht, ist aber evtl. **zu träge**
  für das kurze Blinken. Nur nehmen, wenn nichts anderes da ist.

## Wichtig

- Die IR-LED **nie direkt** an einen GPIO — immer über den Transistor
  (GPIO liefert zu wenig Strom, Reichweite wäre mies).
- Lichtsensor an **GPIO34** (ADC1). ADC2-Pins funktionieren mit aktivem WLAN nicht.

Verdrahtung: siehe [HARDWARE.md](HARDWARE.md).
