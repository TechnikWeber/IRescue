**English** · [Deutsch](README.de.md)

# IRescue

Lost the infrared remote for your lamp, LED strip or fan? IRescue is an
ESP32-based tool that **finds the lost codes by brute force**, and later
**learns remotes and shares their codes as packs**, so others with the same
device can just load a pack.

> Status: concept / early prototype, nothing usable yet.

## How it works

1. **Known codes first:** replay code lists from public databases
   (Flipper-IRDB, IRDB, LIRC) that are likely to match (LED lamps, LED strips).
2. **Protocol sweep:** systematically send all combinations of the most common
   protocol (NEC: 256 addresses × 256 commands ≈ 2 hours).
3. **Automatic detection:** a light sensor watches the lamp. Many lamps briefly
   blink when they accept a command. IRescue notices this, narrows it down to
   the exact code and logs it.
4. **Learn & share (planned):** an IR receiver records existing remotes; the
   result is saved as a pack in a Flipper-compatible `.ir` format.

## Hardware (prototype)

| Part | Purpose |
|---|---|
| ESP32 dev board | Control, 38 kHz carrier via the RMT peripheral |
| IR LED 940 nm (e.g. TSAL6200) + NPN transistor/MOSFET + resistor | Sending |
| Light sensor (e.g. TEMT6000 or phototransistor) | Detects the lamp blinking |
| IR receiver TSOP38238 / VS1838B (optional) | Learning remotes |

## Repository layout (planned)

```
firmware/   ESP32 firmware (PlatformIO, IRremoteESP8266)
packs/      Code packs per device (.ir, Flipper-compatible)
tools/      Scripts: import/dedupe code databases, build candidate lists
docs/       Concept and notes
```

See [docs/KONZEPT.md](docs/KONZEPT.md) (German) for the detailed concept.
