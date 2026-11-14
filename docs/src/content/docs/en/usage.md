---
title: Screens and buttons
description: How to read the map and info screens, and how to use the buttons.
---

## Buttons

The three buttons on the front are called A, B and C from left to right.

| Button | Action | Result |
| --- | --- | --- |
| A | Press | Resynchronizes the time with NTP. |
| B | Press | Switches the map style between "photo" and "twilight zones". |
| B | Hold (about 0.7 s) | Cycles the overlay: none → Maidenhead grid → grid and Moon. |
| C | Press | Switches between the map screen and the info screen. |

The map style and overlay are stored on the device and kept when the power is off.

When the screen is dimmed, a button press only lights up the screen and does nothing else. Press the button again once the screen is bright.

## Map screen

![Map screen](../../../assets/images/map-photo.png)

### Top

The local time, as "time zone name, year-month-day hour:minute:second". Set the time zone name and offset with `time.tz_label` and `time.tz_offset_min` in `config.json`.

### Map

A world map in equirectangular projection. Its center is your longitude unless `map.center_lon` is set. The map is redrawn every minute.

| Mark | Meaning |
| --- | --- |
| Orange line | Grayline (sunrise / sunset line, sun altitude -0.833°) |
| Yellow circle | Subsolar point |
| Red dot | Your location |
| Gray dotted horizontal line | Equator |

Button B switches the map style:

- Photo style (default): NASA Blue Marble by day and NASA Earth at Night (city lights) by night, blended smoothly between sun altitudes of -12° and +2°.
- Twilight zones: day, civil twilight (sun altitude 0° to -6°), nautical twilight (-6° to -12°), astronomical twilight (-12° to -18°) and night in steps of brightness.

Holding button B cycles the overlays:

- Maidenhead grid: dotted lines for each field (20° longitude × 10° latitude), with field letters along the bottom and left edges.
- Moon: the Moon at the sublunar point, drawn with its current phase.

![Maidenhead grid and Moon overlay](../../../assets/images/map-overlay.png)

### Bottom

| Item | Meaning |
| --- | --- |
| `UTC hh:mm:ss` | Coordinated Universal Time. Cyan when synchronized with NTP, red when not. |
| Battery icon | Battery level in 25% steps (red at 25% or less). A green frame means USB power. The text below shows `CHG` while charging, `USB` on USB power, or the level (%) on battery. |
| `Sunrise` / `Sunset` | Time of your next sunrise or sunset (local time) and the time left until then. |
| `Polar night` / `Midnight sun` | Shown when there is no sunrise or sunset within two days. The second line shows the current sun altitude. |

## Info screen

Press button C to switch.

![Info screen](../../../assets/images/info-screen.png)

| Line | Contents |
| --- | --- |
| First line | Your location name, latitude / longitude and 6-character grid locator. At the top right: the location source (`config`, `GeoIP` or `default`) and the reason for the last restart (`rst:`). |
| Twilight, sunrise, sunset | Morning on the left (start of astronomical, nautical and civil twilight, sunrise) and evening on the right (end of astronomical, nautical and civil twilight, sunset), in local time. `--:--` means the event does not happen that day. |
| `Solar noon` / `Day length` | Time of solar noon and length of the day. The firmware version is shown at the right. |
| `Sun now` | Current sun altitude (`Alt`) and azimuth (`Az`, clockwise from north). |
| `Dec` / `EoT` / `Moon` | Solar declination (degrees), equation of time (minutes), Moon illumination (%) and whether it is waxing (`wax`) or waning (`wan`). |
| Last line | UTC, NTP status (`NTP OK` or `NO SYNC`), battery level and power source. |

## Screen brightness

The screen is bright (`display.brightness`) when it is daytime at your location (sun altitude above `display.day_alt_deg`) and the device is on USB power. At night or on battery, it is dimmed (`display.dim_brightness`). A button press lights up a dimmed screen for `display.wake_sec` seconds.

## Time synchronization

At startup, the device connects to Wi-Fi and synchronizes the time with NTP. It then disconnects from Wi-Fi and resynchronizes every `time.ntp_interval_min` minutes (6 hours by default). After a failed sync, it retries every 30 minutes. Press button A to resynchronize right away.

Until the first successful sync, the build time of the firmware is used as a provisional time. In this state, UTC on the map screen is shown in red and the info screen shows `NO SYNC`.
