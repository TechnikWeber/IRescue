# Flashen — ohne IDE

Der ESP32 nutzt `.bin`-Dateien (kein `.hex` wie AVR/Arduino-Uno). Die fertigen
Binärdateien liegen im Repo unter [`webflasher/firmware/`](../webflasher/firmware/).

## Weg 1 — Web Flasher im Browser (am einfachsten)

Nichts installieren. Nur **Chrome** oder **Edge** am Desktop (Web Serial).

1. Seite öffnen: **https://technikweber.github.io/IRescue/**
2. ESP32 per **USB-Datenkabel** anstecken.
3. „IRescue installieren“ → seriellen Port wählen → „Install“.
4. Klemmt es? Beim „Install“ die **BOOT**-Taste am ESP32 gedrückt halten.

Danach: mit dem offenen WLAN **IRescue-XXXX** verbinden — die Oberfläche öffnet
sich automatisch (Captive Portal), sonst `http://192.168.4.1/`.

## Weg 2 — Kommandozeile mit esptool (auch ohne IDE)

Nur Python nötig:

```bash
pip install esptool
# ganze 4-MB-Abbild in einem Rutsch (enthält Firmware + Web-UI):
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
    write_flash 0x0 webflasher/firmware/irescue-full.bin
```

Unter Fedora ist der Port meist `/dev/ttyUSB0` (dein KDE-System). Fehlt die
Berechtigung: `sudo usermod -aG dialout $USER` und neu einloggen.

Oder die Einzelteile (falls du nur die Firmware ohne Web-UI neu willst):

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash \
  0x1000   webflasher/firmware/bootloader.bin \
  0x8000   webflasher/firmware/partitions.bin \
  0xe000   webflasher/firmware/boot_app0.bin \
  0x10000  webflasher/firmware/firmware.bin \
  0x290000 webflasher/firmware/littlefs.bin
```

## Weg 3 — selbst kompilieren

Siehe [COMPILE.md](COMPILE.md).
