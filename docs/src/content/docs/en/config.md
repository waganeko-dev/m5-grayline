---
title: Configuration (config.json)
description: Settings in config.json on the microSD card.
---

m5-grayline reads its settings from `/grayline/config.json` on the microSD card.

## Creating the settings file

1. Insert a microSD card into the device and turn it on. If `/grayline/config.json` does not exist, it is created with default values.
2. Turn the device off and put the microSD card in your computer.
3. Open `/grayline/config.json` in a text editor and fill in `wifi.ssid` and `wifi.password`. Change other settings, such as your location, as needed.
4. Put the card back in the device and turn it on.

You can also copy [config.example.json](https://github.com/atsuo-takahashi/m5-grayline/blob/main/config.example.json) to the card as `/grayline/config.json` and edit it.

:::note
The automatically created `config.json` has no latitude / longitude (`home.lat` and `home.lon`). If you keep it that way, the device estimates its location with GeoIP after connecting to Wi-Fi and synchronizing the time. To avoid GeoIP, fill in your latitude and longitude (see [GeoIP and privacy](#geoip-and-privacy)).
:::

## Example

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

## Settings

Omitted keys use their default values.

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | integer | `1` | Format version of the file. Do not change it. |
| `wifi.ssid` | string | `""` | Wi-Fi SSID. Use a 2.4 GHz network. |
| `wifi.password` | string | `""` | Wi-Fi password. |
| `home.name` | string | `"Home"` | Name of your location, shown on the info screen. Used only when `home.lat` and `home.lon` are set. |
| `home.lat` | number | none | Latitude in degrees, positive north. |
| `home.lon` | number | none | Longitude in degrees, positive east. |
| `home.geoip` | boolean | `true` | Whether to estimate the location with GeoIP (ipinfo.io) when `home.lat` / `home.lon` are not set. |
| `home.ipinfo_token` | string | `""` | ipinfo.io access token (optional). The free tier works without a token. |
| `map.center_lon` | number | your longitude | Longitude of the map center in degrees. |
| `time.tz_label` | string | `"JST"` | Time zone name shown before the local time at the top. |
| `time.tz_offset_min` | integer | `540` | Offset from UTC in minutes, e.g. `540` for Japan, `330` for India, `-300` for New York (standard time). |
| `time.ntp` | array of strings | `["ntp.nict.jp", "time.google.com", "pool.ntp.org"]` | NTP servers, up to three. |
| `time.ntp_interval_min` | integer | `360` | Interval between NTP syncs in minutes. After a failed sync, the device retries every 30 minutes. |
| `display.brightness` | integer (0-255) | `160` | Backlight level when the screen is bright. |
| `display.dim_brightness` | integer (0-255) | `16` | Backlight level when the screen is dimmed. |
| `display.wake_sec` | integer | `30` | How long (seconds) a button press lights up a dimmed screen. |
| `display.day_alt_deg` | number | `-6` | Sun altitude (degrees) at your location above which it counts as daytime, for the backlight. |

:::caution[Daylight saving time]
Daylight saving time is not applied automatically. During DST, change `time.tz_offset_min` and `time.tz_label`.
:::

## How your location is chosen

Your location is chosen in the following order. The info screen shows the source (`config`, `GeoIP` or `default`) at the top right.

1. `home.lat` and `home.lon`, if set.
2. Otherwise (and if `home.geoip` is `true`), an estimate by GeoIP (ipinfo.io) after connecting to Wi-Fi and synchronizing the time. The result is stored on the device and used first at the next startup.
3. Otherwise, Tokyo (35.6895° N, 139.6917° E).

GeoIP gives an approximate location (often city level) estimated from your global IP address. Depending on your internet provider, it can be far from where you are. For an exact location, set `home.lat` and `home.lon`.

## GeoIP and privacy

If `home.lat` and `home.lon` are not set, the device queries ipinfo.io at startup and at every NTP sync (every 6 hours by default). **This sends your global IP address to ipinfo.io.**

The device does not contact ipinfo.io if you do either of the following:

- Set `home.lat` and `home.lon`.
- Set `home.geoip` to `false` (Tokyo is used if no latitude / longitude is set).

If `home.ipinfo_token` is set, the token is sent with the request in an HTTP header, not in the URL.
