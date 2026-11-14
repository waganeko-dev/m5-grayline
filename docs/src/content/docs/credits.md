---
title: データ出典とライセンス
description: m5-grayline のライセンスと、使用しているデータ、ライブラリ、サービスを説明します。
---

## ライセンス

m5-grayline は [MIT ライセンス](https://github.com/atsuo-takahashi/m5-grayline/blob/main/LICENSE) で公開しています。サードパーティのライブラリとデータには、それぞれのライセンスが適用されます。詳しくは [THIRD_PARTY_NOTICES.md](https://github.com/atsuo-takahashi/m5-grayline/blob/main/THIRD_PARTY_NOTICES.md) を参照してください。

## 地図のデータ

| データ | 用途 | 出典とライセンス |
| --- | --- | --- |
| NASA Blue Marble | 写真風の地図 (昼) | NASA、パブリックドメイン |
| NASA Earth at Night | 写真風の地図 (夜) | NASA、パブリックドメイン |
| Natural Earth (1:50m land) | 陸地と海の判定 (薄明の塗り分け) | [Natural Earth](https://www.naturalearthdata.com/)、パブリックドメイン |

NASA は本プロジェクトを推奨・保証するものではありません。

## ファームウェアのライブラリ

| ライブラリ | ライセンス |
| --- | --- |
| [M5Unified](https://github.com/m5stack/M5Unified) | MIT |
| [M5GFX](https://github.com/m5stack/M5GFX) | MIT (同梱のフォントにはそれぞれのライセンスが適用されます) |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | MIT |
| [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32) | LGPL-2.1 (同梱の ESP-IDF の多くは Apache-2.0) |

## マニュアルと Web Flasher

| ソフトウェア | ライセンス |
| --- | --- |
| [Astro](https://astro.build/) / [Starlight](https://starlight.astro.build/) | MIT |
| [ESP Web Tools](https://github.com/esphome/esp-web-tools) | Apache-2.0 |

## 外部のサービス

| サービス | 用途 |
| --- | --- |
| [ipinfo.io](https://ipinfo.io/) | 自局のおおよその位置の推定 (GeoIP)。`home.lat` と `home.lon` を設定していない場合だけ使います。詳しくは [設定 (config.json)](../config/#geoip-とプライバシー) を参照してください。 |
| NTP サーバー | 時刻の同期。既定は `ntp.nict.jp`、`time.google.com`、`pool.ntp.org` です。 |
