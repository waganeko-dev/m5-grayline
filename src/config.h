#pragma once
// Runtime settings stored on the microSD card as /grayline/config.json.
//
// {
//   "version": 1,
//   "wifi": { "ssid": "...", "password": "..." },
//   "home": { "name": "Tokyo", "lat": 35.6895, "lon": 139.6917, "geoip": true,
//             "ipinfo_token": "" },
//   "map":  { "center_lon": 139.6917 },
//
// Home location priority: "lat"/"lon" in this file -> GeoIP (looked up while
// Wi-Fi is on, last result kept in NVS) -> built-in default (Tokyo).
// GeoIP sends the device's public IP address to ipinfo.io; it is never called
// when "lat"/"lon" are set. "ipinfo_token" is optional (free tier works without
// one); when set, it is sent as a Bearer token.
// "center_lon" is optional; without it the map is centered on home.
//   "time": { "tz_label": "JST", "tz_offset_min": 540,
//             "ntp": ["ntp.nict.jp", "time.google.com", "pool.ntp.org"],
//             "ntp_interval_min": 360 },
//   "display": { "brightness": 160, "dim_brightness": 16, "wake_sec": 30,
//                "day_alt_deg": -6 }
// }
//
// When the file does not exist, it is created with defaults; fill in the
// Wi-Fi SSID and password on a PC.

#include <Arduino.h>

constexpr const char* CONFIG_DIR = "/grayline";
constexpr const char* CONFIG_PATH = "/grayline/config.json";

// Format version of config.json. Bump it on incompatible changes and migrate
// older files in loadConfig().
constexpr int CONFIG_VERSION = 1;

struct Config {
  String wifiSsid;
  String wifiPassword;
  String homeName = "Tokyo";
  double homeLat = 35.6895;
  double homeLon = 139.6917;
  double centerLon = 139.6917;
  bool homeFromConfig = false;     // lat/lon given in config.json
  bool centerFromConfig = false;   // center_lon given in config.json
  bool geoipEnabled = true;
  String ipinfoToken;              // optional ipinfo.io access token
  String tzLabel = "JST";
  int tzOffsetMin = 9 * 60;
  String ntp[3] = { "ntp.nict.jp", "time.google.com", "pool.ntp.org" };
  int ntpIntervalMin = 360;     // resync interval; Wi-Fi is off in between
  // Backlight: bright in daytime on USB power, dim at night or on battery.
  int brightness = 160;         // 0-255
  int dimBrightness = 16;
  int wakeSec = 30;             // a button press while dimmed lights it up this long
  float dayAltDeg = -6.0f;      // home sun altitude above which it counts as daytime
};

enum class ConfigSource { SdCard, CreatedOnSd, Defaults };

// Mounts the SD card and loads (or creates) the config file.
ConfigSource loadConfig(Config& cfg, String& message);

// Writes the config back to the SD card.
bool writeConfig(const Config& cfg);

// Prints the config to Serial with the password and token masked.
void printConfig(const Config& cfg);
