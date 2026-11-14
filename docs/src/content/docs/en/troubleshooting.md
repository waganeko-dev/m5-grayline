---
title: Troubleshooting
description: Startup messages and solutions to common problems.
---

## Startup messages

During startup and time synchronization, messages are shown at the bottom of the screen.

| Message | Meaning and solution |
| --- | --- |
| `SD not found: using defaults` | The microSD card is not recognized. The device runs with default values (no Wi-Fi, Tokyo as the location, and so on). Check that the card is fully inserted and formatted as FAT32. |
| `config.json error: ...` | `config.json` is not valid JSON. The device runs with default values. Check for missing or extra commas and quotes. |
| `Cannot write config: using defaults` | `config.json` could not be created. Check that the card is not write-protected. |
| `Created /grayline/config.json` | A new `config.json` was created. Fill in your Wi-Fi settings. |
| `Updated /grayline/config.json` | New settings were added with default values to an older `config.json`. |
| `No Wi-Fi in config.json` | `wifi.ssid` is empty. Fill it in; see [Configuration (config.json)](../config/). |
| `Wi-Fi failed (st=..., ssid found)` | The SSID was found, but the connection failed. Check the password. |
| `Wi-Fi failed (st=..., ssid not found - 2.4GHz?)` | The SSID was not found. Check its spelling. The device supports 2.4 GHz networks only and cannot connect to 5 GHz networks. |
| `NTP failed` | Wi-Fi is connected, but the time could not be obtained. Check that NTP (UDP port 123) is allowed on your network, or set other servers in `time.ntp`. |

## The time is wrong, or UTC is shown in red

The time has not been synchronized with NTP. Until it is, the build time of the firmware is used. Check the messages in the table above and your Wi-Fi settings. Press button A to resynchronize right away. After a failed sync, the device retries every 30 minutes.

If only the local time is off, check `time.tz_offset_min` (offset from UTC in minutes). Daylight saving time is not applied automatically.

## The location is wrong

If the info screen shows `GeoIP` at the top right, the location was estimated from your global IP address. Depending on your internet provider, it can be far from where you are. Set `home.lat` and `home.lon`.

If it shows `default`, no latitude / longitude is set and GeoIP was not available, so Tokyo is used.

## The screen is dark

The screen is dimmed at night (sun altitude at your location at or below `display.day_alt_deg`) and on battery. A button press lights it up for `display.wake_sec` seconds. Change the dimmed level with `display.dim_brightness`.

## A button does nothing

When the screen is dimmed, a button press only lights up the screen. Press the button again once the screen is bright.

## Cannot install the firmware, or no serial port

- Make sure the USB cable supports data, not only charging.
- Depending on your operating system, you may need the driver for the CH9102 USB-to-serial chip. Get it from the official M5Stack documentation.
- The Web Flasher works only in Google Chrome and Microsoft Edge on a computer.
- Close other applications (such as a serial monitor) that have the port open.

## Starting over with the settings

Delete `/grayline/config.json` from the microSD card and restart the device. A new file with default values is created. The Wi-Fi settings are also cleared, so fill them in again.
