# Build and modify it yourself

The complete sources are in the repo. All you need is **PlatformIO** (it runs on
Python, no heavy IDE required).

## Once: install PlatformIO

```bash
python3 -m venv ~/.pio-venv
~/.pio-venv/bin/pip install -U platformio
```

(The PlatformIO extension for VS Code works too, but the CLI is all you need.)

## Build

```bash
cd firmware
~/.pio-venv/bin/pio run            # compile the firmware
~/.pio-venv/bin/pio run -t buildfs # web UI from data/ -> littlefs.bin
```

The output is in `firmware/.pio/build/esp32dev/`.

## Flash it to the ESP32

```bash
~/.pio-venv/bin/pio run -t upload    # firmware
~/.pio-venv/bin/pio run -t uploadfs  # web UI
```

## Build one flashable image (as used by the web flasher)

```bash
cd firmware
B=.pio/build/esp32dev
BOOT=$(python3 -c "import glob;print(glob.glob('$HOME/.platformio/**/boot_app0.bin',recursive=True)[0])")
~/.pio-venv/bin/pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32 \
  merge_bin --fill-flash-size 4MB -o ../webflasher/firmware/irescue-full.bin \
  0x1000 $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 $BOOT \
  0x10000 $B/firmware.bin 0x290000 $B/littlefs.bin
```

## Where things are

| File | Purpose |
|---|---|
| `firmware/src/config.h` | Pins, Wi-Fi name, timing |
| `firmware/src/engine.h` | Brute-force engine (modes, ticks, hits) |
| `firmware/src/oracle.h` | Light sensor evaluation (blink detection) |
| `firmware/src/packs.h` | Load the candidate list, write `.ir` packs |
| `firmware/src/main.cpp` | Wi-Fi AP, captive portal, web server, API |
| `firmware/data/` | Web UI (flashed into LittleFS) |
| `tools/import_db.py` | Build the candidate list from Flipper-IRDB |

## Reproducible in CI

Every push also builds everything in GitHub Actions
([`.github/workflows/build.yml`](../.github/workflows/build.yml)) and publishes
the web flasher on GitHub Pages. So you can also take the built
`irescue-full.bin` straight from the Actions artifact.
