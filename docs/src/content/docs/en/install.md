---
title: Installing the firmware
description: How to install the m5-grayline firmware on M5Stack Basic.
---

You can install the firmware in one of the following ways.

| Method | Requirements | Best for |
| --- | --- | --- |
| [Web Flasher](../flasher/) | Google Chrome or Microsoft Edge | The easiest way |
| [esptool](#installing-with-esptool) | Python and esptool | Command-line users |
| [PlatformIO](#building-and-uploading-with-platformio) | PlatformIO | Building from source |

:::note
Support for M5Burner is planned.
:::

## Preparation: connecting the device

M5Stack Basic V2.6 uses a CH9102 USB-to-serial chip. Depending on your operating system, you may need to install the CH9102 driver. Get it from the official M5Stack documentation.

When the device is connected, a serial port appears. Its name depends on the operating system:

- Windows: `COM3`, etc.
- macOS: `/dev/cu.usbserial-XXXXXXXX`, etc.
- Linux: `/dev/ttyUSB0`, etc.

## Installing with the Web Flasher

Open the [Web Flasher](../flasher/) page and click "Connect and install". See that page for details.

## Installing with esptool

Each release on [GitHub Releases](https://github.com/waganeko-dev/m5-grayline/releases) has two files:

| File | Address | Contents |
| --- | --- | --- |
| `m5-grayline-vX.Y.Z-full.bin` | `0x0` | Full image: bootloader, partition table and application |
| `m5-grayline-vX.Y.Z-app.bin` | `0x10000` | Application only, for updating a device that already runs m5-grayline |

For the first installation, use the full image. Install [esptool](https://github.com/espressif/esptool) (`pip install esptool`) and run the following command, replacing `<port>` with the serial port of the device:

```bash
esptool --chip esp32 --port <port> --baud 921600 write-flash 0x0 m5-grayline-vX.Y.Z-full.bin
```

With older esptool versions (v4 and earlier), the command is `esptool.py` and the subcommand is `write_flash`.

## Building and uploading with PlatformIO

Install [PlatformIO](https://platformio.org/), get the repository, and run the following commands:

```bash
git clone https://github.com/waganeko-dev/m5-grayline.git
cd m5-grayline
pio run -t upload
```

To choose the port, add `--upload-port <port>`. For more about building, see [docs/DEVELOPMENT.md](https://github.com/waganeko-dev/m5-grayline/blob/main/docs/DEVELOPMENT.md) in the repository (in Japanese).

## After installing

The device restarts and shows a startup message at the bottom of the screen. If a microSD card is inserted, `/grayline/config.json` is created automatically. Continue with [Configuration (config.json)](../config/).
