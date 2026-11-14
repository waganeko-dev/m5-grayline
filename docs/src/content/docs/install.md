---
title: 書き込み方法
description: m5-grayline のファームウェアを M5Stack Basic に書き込む方法を説明します。
---

ファームウェアは次のいずれかの方法で書き込めます。

| 方法 | 必要なもの | 向いている人 |
| --- | --- | --- |
| [Web Flasher](../flasher/) | Google Chrome または Microsoft Edge | 手軽に書き込みたい人 |
| [esptool](#esptool-で書き込む) | Python と esptool | コマンドラインに慣れている人 |
| [PlatformIO](#platformio-でビルドして書き込む) | PlatformIO | ソースからビルドしたい人 |

:::note
M5Burner への対応は今後の予定です。
:::

## 準備: パソコンと本体の接続

M5Stack Basic V2.6 は、USB シリアル変換 IC に CH9102 を使っています。パソコンの OS によっては、CH9102 のドライバーのインストールが必要です。ドライバーは M5Stack の公式ドキュメントから入手してください。

本体をパソコンに接続すると、シリアルポートが現れます。ポートの名前は OS によって異なります。

- Windows: `COM3` など
- macOS: `/dev/cu.usbserial-XXXXXXXX` など
- Linux: `/dev/ttyUSB0` など

## Web Flasher で書き込む

[Web Flasher](../flasher/) のページを開き、"接続して書き込む" を押します。詳しくは Web Flasher のページを参照してください。

## esptool で書き込む

[GitHub Releases](https://github.com/waganeko-dev/m5-grayline/releases) には、リリースごとに次の 2 つのファイルがあります。

| ファイル | 書き込むアドレス | 内容 |
| --- | --- | --- |
| `m5-grayline-vX.Y.Z-full.bin` | `0x0` | ブートローダー、パーティションテーブル、アプリケーションを含む一括イメージ |
| `m5-grayline-vX.Y.Z-app.bin` | `0x10000` | アプリケーションだけのイメージ。m5-grayline が動いている本体の更新用 |

初めて書き込む場合は、一括イメージを使います。[esptool](https://github.com/espressif/esptool) をインストールし (`pip install esptool`)、次のコマンドを実行します。`<port>` は本体のシリアルポートに置き換えてください。

```bash
esptool --chip esp32 --port <port> --baud 921600 write-flash 0x0 m5-grayline-vX.Y.Z-full.bin
```

古い esptool (v4 以前) では、コマンド名が `esptool.py`、サブコマンドが `write_flash` です。

## PlatformIO でビルドして書き込む

[PlatformIO](https://platformio.org/) をインストールし、リポジトリを取得してから、リポジトリのフォルダで次のコマンドを実行します。

```bash
git clone https://github.com/waganeko-dev/m5-grayline.git
cd m5-grayline
pio run -t upload
```

ポートを指定する場合は、`--upload-port <port>` を付けます。ビルドの詳細は、リポジトリの [docs/DEVELOPMENT.md](https://github.com/waganeko-dev/m5-grayline/blob/main/docs/DEVELOPMENT.md) を参照してください。

## 書き込んだあと

書き込みが終わると、本体が再起動し、画面下部に起動のメッセージが表示されます。microSD を挿していれば、`/grayline/config.json` が自動で作られます。続いて [設定 (config.json)](../config/) を行ってください。
