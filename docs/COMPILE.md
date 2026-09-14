# Selbst kompilieren & ändern

Die kompletten Quellen liegen im Repo, du brauchst nur **PlatformIO** (baut auf
Python auf, keine schwere IDE nötig).

## Einmalig: PlatformIO installieren

```bash
python3 -m venv ~/.pio-venv
~/.pio-venv/bin/pip install -U platformio
```

(Alternativ die PlatformIO-Erweiterung in VS Code — aber CLI reicht völlig.)

## Bauen

```bash
cd firmware
~/.pio-venv/bin/pio run            # Firmware kompilieren
~/.pio-venv/bin/pio run -t buildfs # Web-UI aus data/ -> littlefs.bin
```

Ergebnis liegt in `firmware/.pio/build/esp32dev/`.

## Auf den ESP32 spielen

```bash
~/.pio-venv/bin/pio run -t upload    # Firmware
~/.pio-venv/bin/pio run -t uploadfs  # Web-UI
```

## Ein flashbares Gesamt-Abbild (wie im Web Flasher) erzeugen

```bash
cd firmware
B=.pio/build/esp32dev
BOOT=$(python3 -c "import glob;print(glob.glob('$HOME/.platformio/**/boot_app0.bin',recursive=True)[0])")
~/.pio-venv/bin/pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32 \
  merge_bin --fill-flash-size 4MB -o ../webflasher/firmware/irescue-full.bin \
  0x1000 $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 $BOOT \
  0x10000 $B/firmware.bin 0x290000 $B/littlefs.bin
```

## Was wo liegt

| Datei | Zweck |
|---|---|
| `firmware/src/config.h` | Pins, WLAN-Name, Timing |
| `firmware/src/engine.h` | Bruteforce-Engine (Modi, Ticks, Treffer) |
| `firmware/src/oracle.h` | Lichtsensor-Auswertung (Blink-Erkennung) |
| `firmware/src/packs.h` | Kandidatenliste laden, `.ir`-Packs schreiben |
| `firmware/src/main.cpp` | WLAN-AP, Captive Portal, Webserver, API |
| `firmware/data/` | Web-UI (wird ins LittleFS geflasht) |
| `tools/import_db.py` | Kandidatenliste aus Flipper-IRDB bauen |

## Reproduzierbar per CI

Jeder Push baut das Ganze auch in GitHub Actions
([`.github/workflows/build.yml`](../.github/workflows/build.yml)) und
veröffentlicht den Web Flasher auf GitHub Pages. Du kannst die gebaute
`irescue-full.bin` also auch direkt aus dem Actions-Artefakt ziehen.
