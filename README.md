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

## Quick start

```
# 1. build the candidate list from the CC0 Flipper-IRDB (optional but fast win)
git clone https://github.com/Lucaslhm/Flipper-IRDB ~/ir-databases/Flipper-IRDB
python3 tools/import_db.py --src ~/ir-databases/Flipper-IRDB/LED_Lighting \
    --out firmware/data/candidates.json

# 2. flash firmware + web UI
cd firmware
pio run -t upload      # firmware
pio run -t uploadfs    # web UI (data/ -> LittleFS)

# 3. connect to the WiFi "IRescue-XXXX" (password: irescue123)
#    open http://192.168.4.1/  -> calibrate the sensor -> start the search
```

## Repository layout

```
firmware/       PlatformIO project (ESP32, IRremoteESP8266, async web UI)
  src/          firmware source (engine, oracle, packs, web)
  data/         web UI served from LittleFS (+ candidates.json)
packs/          device packs (.ir, Flipper-compatible), one per remote
tools/          import_db.py: build a candidate list from a code database
docs/           concept and notes
```

## Databases & licenses

Flipper-IRDB is **CC0** and can be embedded/extended freely — it is the default
source. `irdb` and LIRC have restrictive/copyleft licenses and are only used
locally. Details and download locations: [packs/README.md](packs/README.md).

See [docs/KONZEPT.md](docs/KONZEPT.md) (German) for the detailed concept.
