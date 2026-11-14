---
title: Data sources and licenses
description: The license of m5-grayline and the data, libraries and services it uses.
---

## License

m5-grayline is released under the [MIT License](https://github.com/atsuo-takahashi/m5-grayline/blob/main/LICENSE). Third-party libraries and data are under their own licenses; see [THIRD_PARTY_NOTICES.md](https://github.com/atsuo-takahashi/m5-grayline/blob/main/THIRD_PARTY_NOTICES.md).

## Map data

| Data | Used for | Source and license |
| --- | --- | --- |
| NASA Blue Marble | Photo-style map (day) | NASA, public domain |
| NASA Earth at Night | Photo-style map (night) | NASA, public domain |
| Natural Earth (1:50m land) | Land and sea (twilight zones style) | [Natural Earth](https://www.naturalearthdata.com/), public domain |

NASA does not endorse this project.

## Firmware libraries

| Library | License |
| --- | --- |
| [M5Unified](https://github.com/m5stack/M5Unified) | MIT |
| [M5GFX](https://github.com/m5stack/M5GFX) | MIT (bundled fonts are under their own licenses) |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | MIT |
| [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32) | LGPL-2.1 (most bundled ESP-IDF components are Apache-2.0) |

## Manual and Web Flasher

| Software | License |
| --- | --- |
| [Astro](https://astro.build/) / [Starlight](https://starlight.astro.build/) | MIT |
| [ESP Web Tools](https://github.com/esphome/esp-web-tools) | Apache-2.0 |

## External services

| Service | Used for |
| --- | --- |
| [ipinfo.io](https://ipinfo.io/) | Estimating your approximate location (GeoIP), only when `home.lat` and `home.lon` are not set. See [Configuration (config.json)](../config/#geoip-and-privacy). |
| NTP servers | Time synchronization. Defaults: `ntp.nict.jp`, `time.google.com`, `pool.ntp.org`. |
