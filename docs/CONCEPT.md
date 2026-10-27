# IRescue – Concept

## 0. Check first: is it infrared at all?

Many ceiling lamps with a remote use **2.4 GHz** radio (e.g. MiLight-style) or
**433 MHz** instead of infrared. Then every IR search comes up empty.

Signs of IR:
- Did you have to *point* the remote at the lamp? → IR
- A visible dark/red window or "bump" on the lamp rim or the power supply → IR receiver
- Did the remote work through walls or from another room? → radio
- Did the remote have to be "paired"? → almost certainly radio

It is also worth checking the rating plate/model number: if the model is known,
the code may already be in a database.

## 1. Time budget: why pure brute force only works for NEC

Cheap LED lamps overwhelmingly use the **NEC protocol** at 38 kHz.

One NEC frame: 9 ms burst + 4.5 ms space + 32 bits + stop bit.
Standard NEC sends `address, ~address, command, ~command`, so always 16 ones
and 16 zeros → a constant **≈ 67.5 ms**. With the gap to the next frame we count
**≈ 110 ms per attempt**.

| Search space | Combinations | Duration (sent once) |
|---|---|---|
| Commands for a known address | 256 | ≈ 30 s |
| Standard NEC (8-bit address) | 65,536 | ≈ 2 h |
| Extended NEC (16-bit address) | 16.7 million | ≈ 3 weeks ❌ |
| Other protocols (RC5, RC6, Sony, …) | depends on protocol | candidate list instead |

→ **Trying all of standard NEC is realistic (one night).**
Extended NEC and exotic protocols only via lists of known addresses.

## 2. Search strategy (in this order)

1. **Candidate list from databases** (minutes)
   - [Flipper-IRDB](https://github.com/Lucaslhm/Flipper-IRDB): **CC0/public
     domain** → may be embedded, reworked and published with the project.
     This is the default source. Categories LED_Lighting, Fans.
   - [irdb](https://github.com/probonopd/irdb): **own license with conditions**
     (up to 3 devices free, open a GitHub issue, include a notice) →
     **not shipped**, only downloaded and used locally.
   - [LIRC remotes](https://sourceforge.net/projects/lirc-remotes/): GPL
     (copyleft) → only under GPL terms, downloaded locally.
   - `tools/import_db.py` deduplicates Flipper-IRDB into a list of
     `(protocol, address, command)`, sorted by frequency → `candidates.json`.
     Details and download locations: [../packs/README.md](../packs/README.md).
2. **Common NEC addresses × all 256 commands** (minutes)
   - Step 1 yields the typical addresses (for LED strips e.g. `0x00`,
     `0xEF00`/`0x00EF` variants); each address costs only ≈ 30 s.
3. **Full standard NEC sweep** (≈ 2 h, overnight)
4. **Extended NEC / other protocols** only with addresses from the databases.

You know very early whether there is a hit: as soon as **one** command blinks,
the address is known, and the remaining buttons are found in 30 seconds.

## 3. Automatic hit detection

The lamp briefly goes off when it recognises a command — that is the oracle.

- **Sensor:** TEMT6000 (analog) or a phototransistor on the ESP32's ADC,
  sampled at ~1 kHz. An LDR may be too slow for a short flicker.
- **Filter:** lamps often flicker at 100 Hz (mains/PWM ripple) → moving average
  over 10–20 ms, then detect a dip against a baseline.
- **Shielding:** keep the sensor out of the IR beam (silicon sees 940 nm);
  best put the sensor in a small tube right against the lamp shade.
- **Latency:** the lamp reacts with a delay (decoding + reaction, an estimated
  50–300 ms). Therefore:
  - fast pass; on a blink event remember the last *N* codes
  - then repeat those *N* codes one by one, slowly → exact code
  - confirm every hit at least twice before it is saved
- **Side effects:** you will inevitably also hit "off", "night light",
  "dim to minimum". Staying dark ≠ blinking → count it as an "off" hit,
  then search specifically for the on command (often the same toggle).

## 4. Hardware

```
ESP32 GPIO ──[1 kΩ]── base NPN (BC337 / 2N2222)
                      collector ── IR LED cathode
                      emitter   ── GND
5 V ──[≈ 33 Ω]── IR LED anode           (≈ 100 mA pulse current)

TEMT6000 SIG ── ESP32 ADC1 pin (e.g. GPIO34), VCC 3.3 V
TSOP38238 OUT ── ESP32 GPIO (optional, for learning)
```

Exact values depend on the LED and transistor and are set during assembly.
Use ADC1 pins, because ADC2 cannot be used while Wi-Fi is active.

## 5. Software

- **PlatformIO + Arduino framework**, library **IRremoteESP8266**
  (supports the ESP32, sending and receiving many protocols).
- **Web UI from the start** (decided): the ESP32 (DevKit-C) opens its own
  access point `IRescue-XXXX`, so the interface is reachable without any Wi-Fi
  setup. The UI shows the live sensor, progress, pause/resume, testing and saving
  hits, and learning.
  Implementation: ESPAsyncWebServer + WebSocket, UI in LittleFS.
- Save progress in NVS/flash so a sweep continues after a power cut. *(still TODO)*

## 6. Packs (learning & sharing)

**Decided:** learned remotes are saved **as one pack per device** (e.g.
`treadmill-remote.ir`, `living-room-lamp.ir`), not mixed into one big shared
database. These device packs are what gets shared on GitHub later. Format: the
**Flipper `.ir` format**. It is widespread, human-readable, and packs work
directly with a Flipper Zero:

```
Filetype: IR signals file
Version: 1
#
name: Power
type: parsed
protocol: NEC
address: 00 00 00 00
command: 45 00 00 00
```

Extra metadata (manufacturer, model, photo of the remote, who tested it) as
comment lines or in a `pack.json` next to it. Raw signals (`type: raw`) for
unknown protocols.

Your own brute-force hits are saved automatically as a draft pack, so every
find helps others later.

## Decisions (2026-09-14)

- The lamp is IR. ✔
- Board: ESP32 **DevKit-C**. ✔
- **Web UI from the start** (AP mode). ✔
- Database: embed/extend **Flipper-IRDB (CC0)**; irdb & LIRC only locally. ✔
- **One pack per device** (e.g. "treadmill remote"). ✔

## Open questions / TODO

- Save progress across power cuts in NVS.
- Extended NEC byte layout in pack export.
- Export non-NEC protocols cleanly when learning (currently raw only).
