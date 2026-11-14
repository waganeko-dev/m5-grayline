#include "config.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>


static bool mountSd() {
  // M5Stack Basic: microSD shares the LCD SPI bus, CS = GPIO4.
  return SD.begin(GPIO_NUM_4, SPI, 25000000);
}

static void toJson(const Config& cfg, JsonDocument& doc) {
  doc["version"] = CONFIG_VERSION;
  doc["wifi"]["ssid"] = cfg.wifiSsid;
  doc["wifi"]["password"] = cfg.wifiPassword;
  if (cfg.homeFromConfig) {
    doc["home"]["name"] = cfg.homeName;
    doc["home"]["lat"] = cfg.homeLat;
    doc["home"]["lon"] = cfg.homeLon;
  }
  doc["home"]["geoip"] = cfg.geoipEnabled;
  doc["home"]["ipinfo_token"] = cfg.ipinfoToken;
  if (cfg.centerFromConfig) doc["map"]["center_lon"] = cfg.centerLon;
  doc["time"]["tz_label"] = cfg.tzLabel;
  doc["time"]["tz_offset_min"] = cfg.tzOffsetMin;
  JsonArray ntp = doc["time"]["ntp"].to<JsonArray>();
  for (const auto& s : cfg.ntp) {
    if (s.length()) ntp.add(s);
  }
  doc["time"]["ntp_interval_min"] = cfg.ntpIntervalMin;
  doc["display"]["brightness"] = cfg.brightness;
  doc["display"]["dim_brightness"] = cfg.dimBrightness;
  doc["display"]["wake_sec"] = cfg.wakeSec;
  doc["display"]["day_alt_deg"] = cfg.dayAltDeg;
}

static void fromJson(const JsonDocument& doc, Config& cfg) {
  cfg.wifiSsid = doc["wifi"]["ssid"] | cfg.wifiSsid.c_str();
  cfg.wifiPassword = doc["wifi"]["password"] | cfg.wifiPassword.c_str();
  cfg.homeFromConfig = doc["home"]["lat"].is<double>() && doc["home"]["lon"].is<double>();
  if (cfg.homeFromConfig) {
    cfg.homeName = doc["home"]["name"] | "Home";
    cfg.homeLat = doc["home"]["lat"];
    cfg.homeLon = doc["home"]["lon"];
  }
  cfg.geoipEnabled = doc["home"]["geoip"] | true;
  cfg.ipinfoToken = doc["home"]["ipinfo_token"] | cfg.ipinfoToken.c_str();
  cfg.centerFromConfig = doc["map"]["center_lon"].is<double>();
  cfg.centerLon = cfg.centerFromConfig ? doc["map"]["center_lon"].as<double>() : cfg.homeLon;
  cfg.tzLabel = doc["time"]["tz_label"] | cfg.tzLabel.c_str();
  cfg.tzOffsetMin = doc["time"]["tz_offset_min"] | cfg.tzOffsetMin;
  cfg.ntpIntervalMin = doc["time"]["ntp_interval_min"] | cfg.ntpIntervalMin;
  cfg.brightness = doc["display"]["brightness"] | cfg.brightness;
  cfg.dimBrightness = doc["display"]["dim_brightness"] | cfg.dimBrightness;
  cfg.wakeSec = doc["display"]["wake_sec"] | cfg.wakeSec;
  cfg.dayAltDeg = doc["display"]["day_alt_deg"] | cfg.dayAltDeg;
  JsonArrayConst ntp = doc["time"]["ntp"];
  if (!ntp.isNull() && ntp.size() > 0) {
    for (int i = 0; i < 3; ++i) {
      cfg.ntp[i] = i < (int)ntp.size() ? String((const char*)(ntp[i] | "")) : String();
    }
  }
}

bool writeConfig(const Config& cfg) {
  if (!SD.exists(CONFIG_DIR)) SD.mkdir(CONFIG_DIR);
  File f = SD.open(CONFIG_PATH, FILE_WRITE);
  if (!f) return false;
  JsonDocument doc;
  toJson(cfg, doc);
  serializeJsonPretty(doc, f);
  f.close();
  return true;
}

ConfigSource loadConfig(Config& cfg, String& message) {
  if (!mountSd()) {
    message = "SD not found: using defaults";
    return ConfigSource::Defaults;
  }

  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (f) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      message = String("config.json error: ") + err.c_str();
      return ConfigSource::Defaults;
    }
    fromJson(doc, cfg);
    if (doc["display"].isNull() || !doc["version"].is<int>()) {
      // Older file: add the new keys with their defaults so they can be edited.
      writeConfig(cfg);
      message = "Updated " + String(CONFIG_PATH);
      return ConfigSource::SdCard;
    }
    message = "Loaded " + String(CONFIG_PATH);
    return ConfigSource::SdCard;
  }

  if (writeConfig(cfg)) {
    message = "Created " + String(CONFIG_PATH);
    return ConfigSource::CreatedOnSd;
  }
  message = "Cannot write config: using defaults";
  return ConfigSource::Defaults;
}

void printConfig(const Config& cfg) {
  Serial.printf("wifi.ssid=%s wifi.password=%s\n", cfg.wifiSsid.c_str(),
                cfg.wifiPassword.length() ? "(set)" : "(empty)");
  Serial.printf("home=%s lat=%.4f lon=%.4f (%s) center_lon=%.4f (%s) geoip=%d ipinfo_token=%s\n",
                cfg.homeName.c_str(), cfg.homeLat, cfg.homeLon,
                cfg.homeFromConfig ? "config" : "auto", cfg.centerLon,
                cfg.centerFromConfig ? "config" : "home", cfg.geoipEnabled,
                cfg.ipinfoToken.length() ? "(set)" : "(none)");
  Serial.printf("tz=%s %+d min ntp=%s,%s,%s every %d min\n", cfg.tzLabel.c_str(), cfg.tzOffsetMin,
                cfg.ntp[0].c_str(), cfg.ntp[1].c_str(), cfg.ntp[2].c_str(), cfg.ntpIntervalMin);
  Serial.printf("display brightness=%d dim=%d wake=%ds day_alt=%.1f\n", cfg.brightness,
                cfg.dimBrightness, cfg.wakeSec, cfg.dayAltDeg);
}
