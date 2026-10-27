# Flashing — without an IDE

The ESP32 uses `.bin` files (not `.hex` like AVR/Arduino Uno). The prebuilt
binaries are in the repo under [`webflasher/firmware/`](../webflasher/firmware/).

## Option 1 — web flasher in the browser (easiest)

Nothing to install. Just **Chrome** or **Edge** on a desktop (Web Serial).

1. Open the page: **https://technikweber.github.io/IRescue/**
2. Connect the ESP32 with a **USB data cable**.
3. "Install IRescue" → choose the serial port → "Install".
4. Stuck? Hold the **BOOT** button on the ESP32 while clicking "Install".

Then connect to the open Wi-Fi **IRescue-XXXX** — the interface opens
automatically (captive portal), otherwise go to `http://192.168.4.1/`.

## Option 2 — command line with esptool (also without an IDE)

Only Python is needed:

```bash
pip install esptool
# the whole 4 MB image in one go (firmware + web UI):
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
    write_flash 0x0 webflasher/firmware/irescue-full.bin
```

On Linux the port is usually `/dev/ttyUSB0`. If permission is denied:
`sudo usermod -aG dialout $USER`, then log out and back in.

Or the individual parts (if you only want new firmware without the web UI):

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash \
  0x1000   webflasher/firmware/bootloader.bin \
  0x8000   webflasher/firmware/partitions.bin \
  0xe000   webflasher/firmware/boot_app0.bin \
  0x10000  webflasher/firmware/firmware.bin \
  0x290000 webflasher/firmware/littlefs.bin
```

## Option 3 — build it yourself

See [BUILDING.md](BUILDING.md).
