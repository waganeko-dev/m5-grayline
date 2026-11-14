---
title: 設定 (config.json)
description: microSD の config.json で設定できる項目を説明します。
---

m5-grayline の設定は、microSD カードの `/grayline/config.json` に書きます。

## 設定ファイルの作り方

1. microSD カードを本体に挿して起動します。`/grayline/config.json` がなければ、既定値で自動作成されます。
2. 本体の電源を切り、microSD カードをパソコンに挿します。
3. `/grayline/config.json` をテキストエディターで開き、`wifi.ssid` と `wifi.password` を記入します。必要に応じて、自局の位置などを設定します。
4. microSD カードを本体に戻して起動します。

[config.example.json](https://github.com/waganeko-dev/m5-grayline/blob/main/config.example.json) を `/grayline/config.json` として microSD にコピーし、書き換えて使うこともできます。

:::note
自動作成された `config.json` には、自局の緯度経度 (`home.lat` と `home.lon`) が含まれていません。そのまま使うと、Wi-Fi に接続して時刻を同期したときに GeoIP で位置を推定します。GeoIP を使わない場合は、緯度経度を記入してください (後述の "[GeoIP とプライバシー](#geoip-とプライバシー)" を参照)。
:::

## 設定の例

```json
{
  "version": 1,
  "wifi": { "ssid": "your-ssid", "password": "your-password" },
  "home": {
    "name": "Tokyo",
    "lat": 35.6895,
    "lon": 139.6917,
    "geoip": true,
    "ipinfo_token": ""
  },
  "map": { "center_lon": 139.6917 },
  "time": {
    "tz_label": "JST",
    "tz_offset_min": 540,
    "ntp": ["ntp.nict.jp", "time.google.com", "pool.ntp.org"],
    "ntp_interval_min": 360
  },
  "display": { "brightness": 160, "dim_brightness": 16, "wake_sec": 30, "day_alt_deg": -6 }
}
```

## 設定項目

省略した項目には既定値を使います。

| キー | 型 | 既定値 | 説明 |
| --- | --- | --- | --- |
| `version` | 整数 | `1` | 設定ファイルの形式のバージョンです。変更しないでください。 |
| `wifi.ssid` | 文字列 | `""` | Wi-Fi の SSID です。2.4GHz 帯のネットワークを指定します。 |
| `wifi.password` | 文字列 | `""` | Wi-Fi のパスワードです。 |
| `home.name` | 文字列 | `"Home"` | 自局の名前です。詳細画面に表示します。`home.lat` と `home.lon` を設定したときだけ使います。 |
| `home.lat` | 数値 | なし | 自局の緯度 (度) です。北緯が正、南緯が負です。 |
| `home.lon` | 数値 | なし | 自局の経度 (度) です。東経が正、西経が負です。 |
| `home.geoip` | 真偽値 | `true` | `home.lat` と `home.lon` がないときに、GeoIP (ipinfo.io) で位置を推定するかどうかです。 |
| `home.ipinfo_token` | 文字列 | `""` | ipinfo.io のアクセストークンです (任意)。トークンなしでも無料枠で利用できます。 |
| `map.center_lon` | 数値 | 自局の経度 | 地図の中心の経度 (度) です。 |
| `time.tz_label` | 文字列 | `"JST"` | 画面上部の現地時刻の前に表示するタイムゾーン名です。 |
| `time.tz_offset_min` | 整数 | `540` | UTC との時差 (分) です。例: 日本は `540`、インドは `330`、ニューヨークの標準時は `-300`。 |
| `time.ntp` | 文字列の配列 | `["ntp.nict.jp", "time.google.com", "pool.ntp.org"]` | NTP サーバーです。3 つまで指定できます。 |
| `time.ntp_interval_min` | 整数 | `360` | NTP で再同期する間隔 (分) です。同期に失敗した場合は、30 分ごとに再試行します。 |
| `display.brightness` | 整数 (0〜255) | `160` | 画面が明るいときのバックライトの明るさです。 |
| `display.dim_brightness` | 整数 (0〜255) | `16` | 画面を暗くしたときのバックライトの明るさです。 |
| `display.wake_sec` | 整数 | `30` | 暗いときにボタンを押して明るくする時間 (秒) です。 |
| `display.day_alt_deg` | 数値 | `-6` | 自局の太陽高度がこの値 (度) より高いときを "昼" とみなします。画面の明るさの切り替えに使います。 |

:::caution[夏時間]
夏時間には自動で切り替わりません。夏時間の期間は、`time.tz_offset_min` と `time.tz_label` を書き換えてください。
:::

## 自局の位置の決め方

自局の位置は、次の優先順位で決めます。詳細画面の右上に、どれを使っているか ("config"、"GeoIP"、"default") を表示します。

1. `home.lat` と `home.lon` があれば、その値を使います。
2. なければ (かつ `home.geoip` が `true` なら)、Wi-Fi に接続して時刻を同期したときに GeoIP (ipinfo.io) でおおよその位置を推定します。結果は本体に記憶し、次回の起動時はまずその値を使います。
3. どちらもなければ、東京 (北緯 35.6895°、東経 139.6917°) を使います。

GeoIP で分かるのは、回線のグローバル IP アドレスから推定したおおよその位置 (多くの場合は都市単位) です。プロバイダーによっては、実際の場所から離れた位置になることがあります。正確な位置を使いたい場合は、`home.lat` と `home.lon` を設定してください。

## GeoIP とプライバシー

`home.lat` と `home.lon` を設定していない場合、本体は起動時と NTP 同期のたび (既定では 6 時間ごと) に ipinfo.io へ問い合わせます。このとき、**お使いの回線のグローバル IP アドレスが ipinfo.io に送られます**。

次のどちらかを設定すると、ipinfo.io には問い合わせません。

- `home.lat` と `home.lon` を設定する
- `home.geoip` を `false` にする (この場合、緯度経度がなければ東京を使います)

`home.ipinfo_token` を設定すると、問い合わせにアクセストークンを付けます。トークンは URL ではなく HTTP のヘッダーで送ります。
