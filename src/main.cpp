// Grayline Map for M5Stack Basic
//
// Draws a world map in equirectangular projection, shades day / civil /
// nautical / astronomical twilight / night from the sun position, and shows
// local time, UTC and the next sunrise / sunset at the home location.
// Settings come from /grayline/config.json on the microSD card.
//
// The backlight is bright in daytime on USB power and dimmed at night or on
// battery; while dimmed, the first button press only lights it up.
// Wi-Fi is switched off between NTP syncs to save power.
//
// Buttons: A = NTP resync,
//          B = map style (photo / twilight zones), long press = overlays
//              (none -> Maidenhead grid -> grid + Moon),
//          C = map / sun info screen.

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>

#include "config.h"
#include <Preferences.h>
#include <esp_system.h>
#include <esp_sntp.h>

#include "landmask.h"
#include "moon.h"
#include "photomaps.h"
#include "solar.h"
#include "suntimes.h"
#include "textfit.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#define APP_VERSION_SHORT "dev"
#endif

// ---- Layout -----------------------------------------------------------------
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int HEADER_H = 40;               // local clock
constexpr int MAP_Y = HEADER_H;
constexpr int MAP_W = 320;
constexpr int MAP_H = 160;                 // 2:1 equirectangular
constexpr int FOOTER_Y = MAP_Y + MAP_H;    // UTC clock and next sun event (40 px)

static Config config;

// ---- Colors (RGB888, converted when drawing) --------------------------------
struct Rgb { uint8_t r, g, b; };
constexpr Rgb SEA_DAY    = {  40, 110, 190 };
constexpr Rgb SEA_NIGHT  = {   6,  14,  40 };
constexpr Rgb LAND_DAY   = { 110, 170,  80 };
constexpr Rgb LAND_NIGHT = {  22,  34,  26 };
constexpr Rgb TERMINATOR = { 255, 170,  40 };

// Brightness for each zone: day, civil, nautical, astronomical, night.
constexpr float ZONE_LEVEL[5] = { 1.00f, 0.62f, 0.38f, 0.18f, 0.0f };

static uint16_t palette[2][5];   // [sea/land][zone] in RGB565
static uint16_t lineBuf[MAP_W];
static bool timeSynced = false;
static int lastMapMinute = -1;
static int lastSecond = -1;

enum class MapStyle : uint8_t { Photo = 0, Zones = 1 };
static MapStyle mapStyle = MapStyle::Photo;
enum class Overlay : uint8_t { None = 0, Grid = 1, GridMoon = 2 };
static Overlay overlay = Overlay::None;
static Preferences prefs;   // NVS: remembers the map style and overlay across reboots

// Power state (IP5306 gives only a 4-step battery level and charger status).
struct PowerState {
  int batteryLevel = -1;   // 0, 25, 50, 75, 100 or -1
  bool external = false;   // USB power present
  bool charging = false;
};
static PowerState power;
static uint32_t wakeUntilMs = 0;   // button-wake period end
static bool backlightBright = true;
static time_t lastSyncAttempt = 0;
static const char* resetReason = "?";

enum class HomeSource { Config, GeoIP, Default, Test };
static HomeSource homeSource = HomeSource::Default;

enum class Screen { Map, Info };
static Screen screen = Screen::Map;

static time_t toLocal(time_t utc) { return utc + config.tzOffsetMin * 60; }

static uint16_t blend565(const Rgb& night, const Rgb& day, float t) {
  auto mix = [t](uint8_t a, uint8_t b) { return (uint8_t)(a + (b - a) * t + 0.5f); };
  return lgfx::color565(mix(night.r, day.r), mix(night.g, day.g), mix(night.b, day.b));
}

static void buildPalette() {
  for (int z = 0; z < 5; ++z) {
    palette[0][z] = blend565(SEA_NIGHT, SEA_DAY, ZONE_LEVEL[z]);
    palette[1][z] = blend565(LAND_NIGHT, LAND_DAY, ZONE_LEVEL[z]);
  }
}

// Mix two RGB565 colors, t = 0 -> a, t = 1 -> b.
static inline uint16_t blendPixel(uint16_t a, uint16_t b, float t) {
  if (t <= 0.0f) return a;
  if (t >= 1.0f) return b;
  const int k = (int)(t * 32.0f);   // 0..32
  int r = ((a >> 11) * (32 - k) + (b >> 11) * k) >> 5;
  int g = (((a >> 5) & 0x3F) * (32 - k) + ((b >> 5) & 0x3F) * k) >> 5;
  int bl = ((a & 0x1F) * (32 - k) + (b & 0x1F) * k) >> 5;
  return (uint16_t)(r << 11 | g << 5 | bl);
}

static inline bool isLand(int col, int row) {
  return (LANDMASK[row][col >> 3] >> (7 - (col & 7))) & 1;
}

static double wrapLon(double lon) {
  while (lon < -180.0) lon += 360.0;
  while (lon >= 180.0) lon -= 360.0;
  return lon;
}

// Screen x (0..MAP_W-1) for a longitude.
static int lonToX(double lon) {
  double rel = wrapLon(lon - config.centerLon) + 180.0;   // 0..360
  return (int)(rel / 360.0 * MAP_W);
}

static int latToY(double lat) {
  return MAP_Y + (int)((90.0 - lat) / 180.0 * MAP_H);
}

// Maidenhead fields (20 x 10 deg) as dotted lines, field letters on the edges.
static void drawGrid() {
  const uint16_t c = lgfx::color565(120, 120, 120);
  for (int i = 0; i < 18; ++i) {           // meridians every 20 deg
    int x = lonToX(-180.0 + i * 20.0);
    for (int y = MAP_Y; y < MAP_Y + MAP_H; y += 3) M5.Display.drawPixel(x, y, c);
  }
  for (int j = 1; j < 18; ++j) {           // parallels every 10 deg
    int y = latToY(-90.0 + j * 10.0);
    for (int x = 0; x < MAP_W; x += 3) M5.Display.drawPixel(x, y, c);
  }
  M5.Display.setFont(&fonts::Font0);
  // Labels with a dark outline so they stay readable on ice and on night areas.
  auto label = [](const char* t, int x, int y) {
    M5.Display.setTextColor(TFT_BLACK);
    for (int k = 0; k < 4; ++k) M5.Display.drawString(t, x + (k == 0) - (k == 1), y + (k == 2) - (k == 3));
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString(t, x, y);
  };
  M5.Display.setTextDatum(textdatum_t::top_center);
  char s[2] = { 0, 0 };
  for (int i = 0; i < 18; ++i) {           // longitude letters along the bottom
    s[0] = 'A' + i;
    int x = lonToX(-180.0 + i * 20.0 + 10.0);
    label(s, x, MAP_Y + MAP_H - 9);
  }
  M5.Display.setTextDatum(textdatum_t::middle_left);
  for (int j = 0; j < 18; ++j) {           // latitude letters along the left edge
    s[0] = 'A' + j;
    label(s, 2, latToY(-90.0 + j * 10.0 + 5.0));
  }
}

// Moon at the sublunar point, drawn with its current phase.
static void drawMoon(time_t now) {
  MoonPosition m = computeMoonPosition(now);
  const int cx = lonToX(m.sublunarLonDeg), cy = latToY(m.sublunarLatDeg);
  const int r = 5;
  const double cosPsi = 1.0 - 2.0 * m.illumination;   // cos of elongation
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      const double nx = dx / (double)r, ny = dy / (double)r;
      if (nx * nx + ny * ny > 1.0) continue;
      const double edge = sqrt(1.0 - ny * ny) * cosPsi;   // terminator ellipse
      const bool lit = m.waxing ? (nx > edge) : (nx < -edge);
      M5.Display.drawPixel(cx + dx, cy + dy, lit ? lgfx::color565(240, 240, 220) : lgfx::color565(70, 70, 80));
    }
  }
  M5.Display.drawCircle(cx, cy, r + 1, TFT_DARKGREY);
}

// Render the map one row at a time (no full frame buffer: Basic has no PSRAM).
static void drawMap(time_t now) {
  SolarPosition sun = computeSolarPosition(now);

  const float sinDec = sinf(sun.declinationRad);
  const float cosDec = cosf(sun.declinationRad);

  // Thresholds on sin(altitude).
  const float sinHorizon = sinf(-0.833f * DEG_TO_RAD);   // refraction + solar radius
  const float sinCivil   = sinf(-6.0f * DEG_TO_RAD);
  const float sinNaut    = sinf(-12.0f * DEG_TO_RAD);
  const float sinAstro   = sinf(-18.0f * DEG_TO_RAD);
  const float lineHalf   = 0.6f * (180.0f / MAP_H) * DEG_TO_RAD;  // ~0.6 px
  // Photo style: day weight goes 0 -> 1 between these altitudes (smoothstep).
  const float sinBlend0  = sinf(-12.0f * DEG_TO_RAD);
  const float sinBlend1  = sinf(2.0f * DEG_TO_RAD);
  const bool photo = mapStyle == MapStyle::Photo;

  // Per-column hour angle term and land mask column.
  static float cosH[MAP_W];
  static uint16_t maskCol[MAP_W];
  const double leftLon = config.centerLon - 180.0;
  for (int x = 0; x < MAP_W; ++x) {
    double lon = wrapLon(leftLon + (x + 0.5) * 360.0 / MAP_W);
    cosH[x] = cosf((float)((lon - sun.subsolarLonDeg) * DEG_TO_RAD));
    maskCol[x] = (uint16_t)((lon + 180.0) / 360.0 * LANDMASK_W) % LANDMASK_W;
  }

  M5.Display.startWrite();
  for (int y = 0; y < MAP_H; ++y) {
    const float lat = (90.0f - (y + 0.5f) * 180.0f / MAP_H) * DEG_TO_RAD;
    const float a = sinf(lat) * sinDec;
    const float b = cosf(lat) * cosDec;
    const int maskRow = y * LANDMASK_H / MAP_H;

    for (int x = 0; x < MAP_W; ++x) {
      const float sinAlt = a + b * cosH[x];
      int zone;
      if (sinAlt > sinHorizon)      zone = 0;
      else if (sinAlt > sinCivil)   zone = 1;
      else if (sinAlt > sinNaut)    zone = 2;
      else if (sinAlt > sinAstro)   zone = 3;
      else                          zone = 4;

      // Grayline (sunrise / sunset terminator).
      // Near the horizon d(sin alt)/d(alt) ~= 1, so compare on sin(alt) directly.
      if (fabsf(sinAlt - sinHorizon) < lineHalf) {
        lineBuf[x] = lgfx::color565(TERMINATOR.r, TERMINATOR.g, TERMINATOR.b);
        continue;
      }
      if (photo) {
        float t = (sinAlt - sinBlend0) / (sinBlend1 - sinBlend0);
        t = t < 0 ? 0 : (t > 1 ? 1 : t);
        t = t * t * (3 - 2 * t);
        lineBuf[x] = blendPixel(PHOTO_NIGHT[maskRow][maskCol[x]], PHOTO_DAY[maskRow][maskCol[x]], t);
      } else {
        lineBuf[x] = palette[isLand(maskCol[x], maskRow)][zone];
      }
    }

    // Equator as a dotted line.
    if (y == MAP_H / 2) {
      for (int x = 0; x < MAP_W; x += 4) lineBuf[x] = TFT_DARKGREY;
    }
    // lineBuf holds native RGB565; a plain uint16_t* would be taken as byte-swapped.
    M5.Display.pushImage(0, MAP_Y + y, MAP_W, 1, (const lgfx::rgb565_t*)lineBuf);
  }

  if (overlay != Overlay::None) drawGrid();
  if (overlay == Overlay::GridMoon) drawMoon(now);

  // Home marker.
  int hx = lonToX(config.homeLon), hy = latToY(config.homeLat);
  M5.Display.fillCircle(hx, hy, 3, TFT_RED);
  M5.Display.drawCircle(hx, hy, 3, TFT_WHITE);

  // Subsolar point marker.
  int sx = lonToX(sun.subsolarLonDeg), sy = latToY(sun.declinationRad * RAD_TO_DEG);
  M5.Display.fillCircle(sx, sy, 5, TFT_YELLOW);
  M5.Display.drawCircle(sx, sy, 6, TFT_ORANGE);
  M5.Display.endWrite();
}

static time_t localMidnightUtc(time_t utc) { return localMidnightUtc(utc, config.tzOffsetMin); }

static void formatLocalHm(time_t utc, char* buf, size_t n) {
  formatLocalHm(utc, config.tzOffsetMin, buf, n);
}

// Cached: the calculation takes ~35 ms on the ESP32, and the footer asks every
// second. Recomputed once a minute, when the event has passed, or when the
// home location / time zone changes.
static bool nextRiseSet(time_t now, time_t& when, bool& isSunrise) {
  static bool valid = false, has = false, rise = false;
  static time_t cachedWhen = 0, computedAt = 0;
  static double lat = NAN, lon = NAN;
  static int tz = 0;
  if (!valid || now - computedAt >= 60 || (has && now >= cachedWhen) || lat != config.homeLat ||
      lon != config.homeLon || tz != config.tzOffsetMin) {
    has = nextRiseSet(now, config.homeLat, config.homeLon, config.tzOffsetMin, cachedWhen, rise);
    computedAt = now;
    lat = config.homeLat;
    lon = config.homeLon;
    tz = config.tzOffsetMin;
    valid = true;
  }
  when = cachedWhen;
  isSunrise = rise;
  return has;
}

static void drawHeader(time_t now) {
  struct tm lt;
  time_t local = toLocal(now);
  gmtime_r(&local, &lt);
  char buf[64];
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(textdatum_t::middle_left);
  M5.Display.setFont(&fonts::Font4);
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d ", config.tzLabel.c_str(),
           lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  M5.Display.drawString(buf, 6, HEADER_H / 2);
}

static void drawBattery(int x, int y) {
  // Body 22x11 + terminal, level in 25% steps; text below.
  const uint16_t frame = power.external ? TFT_GREEN : TFT_LIGHTGREY;
  M5.Display.fillRect(x, y, 26, 28, TFT_BLACK);
  M5.Display.drawRect(x, y, 22, 11, frame);
  M5.Display.fillRect(x + 22, y + 3, 2, 5, frame);
  if (power.batteryLevel >= 0) {
    int w = 18 * power.batteryLevel / 100;
    uint16_t c = power.batteryLevel <= 25 ? TFT_RED : TFT_GREEN;
    if (w > 0) M5.Display.fillRect(x + 2, y + 2, w, 7, c);
  }
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextDatum(textdatum_t::top_center);
  M5.Display.setTextColor(frame, TFT_BLACK);
  char buf[8];
  if (power.charging) snprintf(buf, sizeof(buf), "CHG");
  else if (power.external) snprintf(buf, sizeof(buf), "USB");
  else if (power.batteryLevel >= 0) snprintf(buf, sizeof(buf), "%d%%", power.batteryLevel);
  else snprintf(buf, sizeof(buf), "--");
  M5.Display.drawString(buf, x + 12, y + 16);
}

static void drawFooter(time_t now) {
  struct tm utc;
  gmtime_r(&now, &utc);
  char buf[64];

  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextColor(timeSynced ? TFT_CYAN : TFT_RED, TFT_BLACK);   // red: not synced
  M5.Display.setTextDatum(textdatum_t::middle_left);
  snprintf(buf, sizeof(buf), "UTC %02d:%02d:%02d", utc.tm_hour, utc.tm_min, utc.tm_sec);
  M5.Display.drawString(buf, 6, FOOTER_Y + 20);
  drawBattery(166, FOOTER_Y + 6);

  // Next sunrise / sunset with countdown.
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextDatum(textdatum_t::top_right);
  time_t when;
  bool rise;
  const bool hasEvent = nextRiseSet(now, when, rise);
  // Clear the right block when its layout changes (event <-> polar day/night).
  static int lastLayout = -1;
  if ((int)hasEvent != lastLayout) {
    lastLayout = hasEvent;
    M5.Display.fillRect(196, FOOTER_Y, SCREEN_W - 196, SCREEN_H - FOOTER_Y, TFT_BLACK);
  }
  if (hasEvent) {
    char hm[8];
    formatLocalHm(when, hm, sizeof(hm));
    long left = (long)(when - now);
    M5.Display.setTextColor(rise ? TFT_YELLOW : TFT_ORANGE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "  %s %s", rise ? "Sunrise" : "Sunset", hm);
    M5.Display.drawString(buf, SCREEN_W - 4, FOOTER_Y + 4);
    snprintf(buf, sizeof(buf), "  in %ld:%02ld:%02ld", left / 3600, left / 60 % 60, left % 60);
    M5.Display.drawString(buf, SCREEN_W - 4, FOOTER_Y + 21);
  } else {
    // No sunrise or sunset within two days: polar night or midnight sun.
    double alt, az;
    sunAltAz(now, config.homeLat, config.homeLon, alt, az);
    M5.Display.setTextColor(alt > 0 ? TFT_YELLOW : TFT_SKYBLUE, TFT_BLACK);
    M5.Display.drawString(alt > 0 ? "Midnight sun" : "Polar night", SCREEN_W - 4, FOOTER_Y + 4);
    M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "  Sun alt %+.1f", alt);
    M5.Display.drawString(buf, SCREEN_W - 4, FOOTER_Y + 21);
  }

}

// Detail screen: today's sun events at home, current sun position.
static void drawInfo(time_t now, bool full) {
  constexpr int TOP = HEADER_H + 4;
  constexpr int LH = 19;
  char buf[64], hm[8];
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextDatum(textdatum_t::top_left);

  if (full) {
    M5.Display.fillRect(0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H, TFT_BLACK);
    M5.Display.drawFastHLine(0, HEADER_H, SCREEN_W, TFT_DARKGREY);

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    char loc[7];
    maidenhead(config.homeLat, config.homeLon, loc);
    // Shorten a long name so the line does not run into the source / reset labels.
    auto titleLine = [&](const std::string& name) {
      snprintf(buf, sizeof(buf), "%s %.2f%c %.2f%c %s", name.c_str(),
               fabs(config.homeLat), config.homeLat >= 0 ? 'N' : 'S',
               fabs(config.homeLon), config.homeLon >= 0 ? 'E' : 'W', loc);
    };
    std::string name = shortenToFit(config.homeName.c_str(), 3, [&](const std::string& n) {
      titleLine(n);
      return M5.Display.textWidth(buf) <= 244;
    });
    titleLine(name);
    M5.Display.drawString(buf, 6, TOP);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextDatum(textdatum_t::top_right);
    M5.Display.setTextColor(homeSource == HomeSource::GeoIP ? TFT_GREENYELLOW
                            : homeSource == HomeSource::Config ? TFT_LIGHTGREY : TFT_ORANGE, TFT_BLACK);
    M5.Display.drawString(homeSource == HomeSource::GeoIP ? "GeoIP"
                          : homeSource == HomeSource::Config ? "config"
                          : homeSource == HomeSource::Test ? "test" : "default", SCREEN_W - 4, TOP);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "rst:%s", resetReason);
    M5.Display.drawString(buf, SCREEN_W - 4, TOP + 9);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextDatum(textdatum_t::top_left);

    SunTimes st = computeSunTimes(localMidnightUtc(now), config.homeLat, config.homeLon);
    static const char* names[EV_COUNT] = {
      "Astro dawn", "Naut dawn", "Civil dawn", "Sunrise", "Solar noon",
      "Sunset", "Civil dusk", "Naut dusk", "Astro dusk" };
    static const uint16_t colors[EV_COUNT] = {
      TFT_DARKGREY, TFT_LIGHTGREY, TFT_SKYBLUE, TFT_YELLOW, TFT_WHITE,
      TFT_ORANGE, TFT_SKYBLUE, TFT_LIGHTGREY, TFT_DARKGREY };
    // Morning events on the left, evening events on the right.
    for (int i = 0; i < 4; ++i) {
      int y = TOP + LH * (i + 1) + 4;
      M5.Display.setTextColor(colors[EV_ASTRO_DAWN + i], TFT_BLACK);
      formatLocalHm(st.t[EV_ASTRO_DAWN + i], hm, sizeof(hm));
      M5.Display.drawString(names[EV_ASTRO_DAWN + i], 6, y);
      M5.Display.drawString(hm, 96, y);
      M5.Display.setTextColor(colors[EV_ASTRO_DUSK - i], TFT_BLACK);
      formatLocalHm(st.t[EV_ASTRO_DUSK - i], hm, sizeof(hm));
      M5.Display.drawString(names[EV_ASTRO_DUSK - i], 166, y);
      M5.Display.drawString(hm, 256, y);
    }

    int y = TOP + LH * 5 + 8;
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    formatLocalHm(st.t[EV_NOON], hm, sizeof(hm));
    long dayLen = (st.t[EV_SUNRISE] != SUN_NEVER && st.t[EV_SUNSET] != SUN_NEVER)
                      ? (long)(st.t[EV_SUNSET] - st.t[EV_SUNRISE]) : -1;
    if (dayLen >= 0) {
      snprintf(buf, sizeof(buf), "Solar noon %s   Day length %ld:%02ld", hm, dayLen / 3600, dayLen / 60 % 60);
    } else {
      snprintf(buf, sizeof(buf), "Solar noon %s", hm);
    }
    M5.Display.drawString(buf, 6, y);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextDatum(textdatum_t::top_right);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.drawString(APP_VERSION_SHORT, SCREEN_W - 4, y + 4);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextDatum(textdatum_t::top_left);
  }

  // Live values (every second).
  int y = TOP + LH * 7 + 4;
  double alt, az;
  sunAltAz(now, config.homeLat, config.homeLon, alt, az);
  SolarPosition sun = computeSolarPosition(now);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  snprintf(buf, sizeof(buf), "Sun now  Alt %+6.2f   Az %6.2f   ", alt, az);
  M5.Display.drawString(buf, 6, y);
  M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  MoonPosition moon = computeMoonPosition(now);
  snprintf(buf, sizeof(buf), "Dec %+6.2f  EoT %+5.1fm  Moon %2.0f%% %s  ",
           sun.declinationRad * RAD_TO_DEG, sun.equationOfTimeMin, moon.illumination * 100.0,
           moon.waxing ? "wax" : "wan");
  M5.Display.drawString(buf, 6, y + LH);

  struct tm utc;
  gmtime_r(&now, &utc);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  char bat[24];
  if (power.batteryLevel >= 0) {
    snprintf(bat, sizeof(bat), "Bat %d%% %s", power.batteryLevel,
             power.charging ? "CHG" : (power.external ? "USB" : "   "));
  } else {
    snprintf(bat, sizeof(bat), "Bat --");
  }
  snprintf(buf, sizeof(buf), "UTC %02d:%02d:%02d  %s  %s   ", utc.tm_hour, utc.tm_min, utc.tm_sec,
           timeSynced ? "NTP OK " : "NO SYNC", bat);
  M5.Display.drawString(buf, 6, y + LH * 2);
}

static void drawClock(time_t now) {
  M5.Display.startWrite();
  drawHeader(now);
  if (screen == Screen::Map) drawFooter(now);
  else drawInfo(now, false);
  M5.Display.endWrite();
}

static void status(const char* msg) {
  M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, 240 - FOOTER_Y, TFT_BLACK);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(textdatum_t::middle_left);
  M5.Display.drawString(msg, 6, FOOTER_Y + 20);
  Serial.println(msg);
}

static void setHome(double lat, double lon, const String& name, HomeSource src) {
  config.homeLat = lat;
  config.homeLon = lon;
  config.homeName = name;
  if (!config.centerFromConfig) config.centerLon = lon;
  homeSource = src;
}

// Approximate location from the public IP (ipinfo.io). City level at best;
// it shows where the ISP routes the line, which is usually close enough for
// a grayline map. Only called while Wi-Fi is up for the NTP sync, and never
// when config.json has home lat/lon. The request reveals the public IP address
// to ipinfo.io. An optional access token (config "ipinfo_token") is sent as a
// Bearer header so that it does not appear in the URL.
static bool lookupGeoIp() {
  WiFiClientSecure client;
  client.setInsecure();   // no CA bundle on the device; the data is not sensitive
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, "https://ipinfo.io/json")) return false;
  if (config.ipinfoToken.length()) {
    http.addHeader("Authorization", "Bearer " + config.ipinfoToken);
  }
  int code = http.GET();
  if (code != 200) {
    Serial.printf("GeoIP HTTP %d\n", code);
    http.end();
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  const char* loc = doc["loc"] | "";
  double lat, lon;
  if (err || sscanf(loc, "%lf,%lf", &lat, &lon) != 2) {
    Serial.println("GeoIP parse error");
    return false;
  }
  String city = doc["city"] | "GeoIP";
  setHome(lat, lon, city, HomeSource::GeoIP);
  prefs.putDouble("geoLat", lat);
  prefs.putDouble("geoLon", lon);
  prefs.putString("geoCity", city);
  Serial.printf("GeoIP: %s %.4f,%.4f\n", city.c_str(), lat, lon);
  return true;
}

static bool syncTime() {
  if (config.wifiSsid.isEmpty()) {
    status("No Wi-Fi in config.json");
    delay(1500);
    return false;
  }
  status("Connecting Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(200);
  if (WiFi.status() != WL_CONNECTED) {
    int st = WiFi.status();
    // Diagnose: is the configured SSID visible on 2.4 GHz?
    int n = WiFi.scanNetworks();
    bool found = false;
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == config.wifiSsid) { found = true; break; }
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "Wi-Fi failed (st=%d, ssid %s)", st,
             found ? "found" : "not found - 2.4GHz?");
    status(msg);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(3000);
    return false;
  }
  status("Syncing NTP...");
  // Keep the system clock in UTC; local time is derived when drawing.
  configTzTime("UTC0", config.ntp[0].c_str(),
               config.ntp[1].length() ? config.ntp[1].c_str() : nullptr,
               config.ntp[2].length() ? config.ntp[2].c_str() : nullptr);
  start = millis();
  while (time(nullptr) < 1700000000 && millis() - start < 15000) delay(200);
  if (time(nullptr) < 1700000000) {
    status("NTP failed");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }
  Serial.printf("NTP synced: %ld\n", (long)time(nullptr));
  if (!config.homeFromConfig && config.geoipEnabled) {
    status("Looking up location (GeoIP)...");
    if (lookupGeoIp()) lastMapMinute = -1;
  }
  // Wi-Fi is only needed for the sync; switch it off to save power.
  // (SNTP's own periodic resync is not used; loop() resyncs on an interval.)
  esp_sntp_stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return true;
}

// Fallback when NTP is unavailable: build time of the firmware (assumed local time).
static void setBuildTime() {
  static const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {0};
  int day, year, hh, mm, ss;
  sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = (strstr(months, mon) - months) / 3;
  t.tm_mday = day;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  time_t local = mktime(&t);          // TZ is UTC, so this is local seconds as if UTC
  struct timeval tv = { local - config.tzOffsetMin * 60, 0 };
  settimeofday(&tv, nullptr);
}

static const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT";
    case ESP_RST_SW:        return "SW";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    default:                return "OTHER";
  }
}

static void updatePower() {
  power.batteryLevel = M5.Power.getBatteryLevel();
  bool active = false;
  M5.Power.Ip5306.readChargeActive(&active);   // set while USB (VIN) is connected
  power.external = active;
  power.charging = M5.Power.isCharging();
}

// Bright: daytime at home on USB power, or within the wake period after a press.
static void updateBacklight(time_t now) {
  double alt, az;
  sunAltAz(now, config.homeLat, config.homeLon, alt, az);
  const bool day = alt > config.dayAltDeg;
  const bool woken = (int32_t)(wakeUntilMs - millis()) > 0;
  const bool bright = woken || (day && power.external);
  if (bright != backlightBright) {
    backlightBright = bright;
    M5.Display.setBrightness(bright ? config.brightness : config.dimBrightness);
    Serial.printf("backlight %s (sun alt %.1f, %s, %s)\n", bright ? "bright" : "dim", alt,
                  power.external ? "USB" : "battery", woken ? "woken" : "auto");
  }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  setenv("TZ", "UTC0", 1);
  tzset();

  M5.Display.fillScreen(TFT_BLACK);
  buildPalette();
  prefs.begin("grayline", false);
  mapStyle = (MapStyle)prefs.getUChar("style", (uint8_t)MapStyle::Photo);
  overlay = (Overlay)prefs.getUChar("overlay", (uint8_t)Overlay::None);
  M5.BtnB.setHoldThresh(700);

  Serial.printf("m5-grayline %s\n", APP_VERSION);
  status("m5-grayline " APP_VERSION);
  delay(800);

  String msg;
  loadConfig(config, msg);
  // Home location: config.json -> last GeoIP result -> default (Tokyo).
  if (config.homeFromConfig) {
    homeSource = HomeSource::Config;
  } else if (config.geoipEnabled && prefs.isKey("geoLat")) {
    setHome(prefs.getDouble("geoLat"), prefs.getDouble("geoLon"),
            prefs.getString("geoCity", "GeoIP"), HomeSource::GeoIP);
  } else {
    setHome(35.6895, 139.6917, "Tokyo", HomeSource::Default);
  }
  status(msg.c_str());
  printConfig(config);
  delay(1000);

  resetReason = resetReasonName(esp_reset_reason());
  Serial.printf("reset reason: %s\n", resetReason);
  // Keep the boost converter on under light load (dimmed, Wi-Fi off);
  // otherwise the IP5306 cuts power on battery after a while.
  M5.Power.Ip5306.setPowerBoostKeepOn(true);
  updatePower();
  M5.Display.setBrightness(config.brightness);

  lastSyncAttempt = time(nullptr);
  timeSynced = syncTime();
  if (!timeSynced) setBuildTime();
  lastSyncAttempt = time(nullptr);
  M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, 240 - FOOTER_Y, TFT_BLACK);
}

void loop() {
  M5.update();

  // While dimmed, the first press of any button only lights up the screen.
  // The press is ignored until all buttons are released (so a click or hold
  // detected later on release does not act either).
  static bool swallowing = false;
  const bool anyPress = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed();
  if (anyPress) {
    if (!backlightBright) swallowing = true;
    wakeUntilMs = millis() + config.wakeSec * 1000u;
  }
  const bool swallow = swallowing;
  if (swallowing && !M5.BtnA.isPressed() && !M5.BtnB.isPressed() && !M5.BtnC.isPressed() &&
      !M5.BtnA.wasReleased() && !M5.BtnB.wasReleased() && !M5.BtnC.wasReleased()) {
    swallowing = false;
  }

  time_t now = time(nullptr);
  struct tm utc;
  gmtime_r(&now, &utc);

  if (screen == Screen::Map && utc.tm_min != lastMapMinute) {
    lastMapMinute = utc.tm_min;
    uint32_t t0 = millis();
    drawMap(now);
    Serial.printf("map redraw %lu ms\n", (unsigned long)(millis() - t0));
  }
  if (utc.tm_sec != lastSecond) {
    lastSecond = utc.tm_sec;
    if (utc.tm_sec % 5 == 0) updatePower();
    drawClock(now);
  }
  updateBacklight(now);

  // Periodic NTP resync (Wi-Fi is off in between). Retry every 30 min on failure.
  const long interval = timeSynced ? config.ntpIntervalMin * 60L : 30 * 60L;
  if (now - lastSyncAttempt >= interval) {
    lastSyncAttempt = now;
    bool ok = syncTime();
    timeSynced = timeSynced || ok;
    if (screen == Screen::Map) {
      M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, TFT_BLACK);
    } else {
      drawInfo(time(nullptr), true);
    }
    return;
  }

  // Debug: send 'S' over serial to receive a raw RGB565 screenshot
  // (header "SCRN" + 320*240*2 bytes, big-endian), see tools/screenshot.py.
  // Serial commands: 'C' prints the config (password and token masked), 'S' screenshot,
  // 'P' prints the time taken by the sun calculations,
  // 'H' switches home to auto (GeoIP) in config.json,
  // 'L...' sets a temporary test location (see below),
  // 'b' / 'c' act as buttons B / C, 'B' as a long press of B, 'w' as a wake press.
  int cmd = Serial.available() ? Serial.read() : -1;
  if (cmd == 'C') printConfig(config);
  if (cmd == 'P') {   // timing of the per-second sun calculations
    uint32_t t0 = micros();
    time_t w;
    bool r;
    nextRiseSet(now, config.homeLat, config.homeLon, config.tzOffsetMin, w, r);   // uncached
    uint32_t t1 = micros();
    computeSunTimes(localMidnightUtc(now), config.homeLat, config.homeLon);
    Serial.printf("nextRiseSet %lu us, computeSunTimes %lu us\n", (unsigned long)(t1 - t0),
                  (unsigned long)(micros() - t1));
  }
  if (cmd == 'L') {
    // Test: "L<lat>,<lon>,<tz offset min>,<tz label>,<name>\n" sets home, map
    // center and time zone until the next reboot (nothing is saved).
    String line = Serial.readStringUntil('\n');
    double lat, lon;
    int tz;
    char label[16] = "", name[32] = "";
    if (sscanf(line.c_str(), "%lf,%lf,%d,%15[^,],%31[^\r\n]", &lat, &lon, &tz, label, name) == 5) {
      setHome(lat, lon, name, HomeSource::Test);
      config.centerLon = lon;
      config.tzOffsetMin = tz;
      config.tzLabel = label;
      M5.Display.fillRect(0, 0, SCREEN_W, HEADER_H, TFT_BLACK);
      M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, TFT_BLACK);
      lastMapMinute = -1;
      if (screen == Screen::Info) drawInfo(now, true);
      Serial.printf("test home: %s %.4f,%.4f %s%+d\n", name, lat, lon, label, tz);
    } else {
      Serial.println("usage: L<lat>,<lon>,<tz_min>,<label>,<name>");
    }
  }
  if (cmd == 'H') {
    // Drop home / center from config.json so the location comes from GeoIP.
    config.homeFromConfig = false;
    config.centerFromConfig = false;
    config.geoipEnabled = true;
    Serial.println(writeConfig(config) ? "config: home set to auto (GeoIP)" : "config write failed");
  }
  if (cmd == 'w') wakeUntilMs = millis() + config.wakeSec * 1000u;   // test: button wake
  if (cmd == 'S') {
    static uint16_t row[320];
    uint32_t sum = 0;
    Serial.write((const uint8_t*)"SCRN", 4);
    for (int y = 0; y < 240; ++y) {
      M5.Display.readRect(0, y, 320, 1, row);
      for (int x = 0; x < 320; ++x) {
        uint8_t b[2] = { (uint8_t)(row[x] >> 8), (uint8_t)row[x] };
        sum += b[0] + b[1];
        Serial.write(b, 2);
      }
    }
    Serial.write((const uint8_t*)&sum, 4);   // little-endian byte sum, checked by screenshot.py
    Serial.flush();
  }

  // Button A: retry NTP sync.
  if (!swallow && M5.BtnA.wasPressed()) {
    lastSyncAttempt = now;
    timeSynced = syncTime() || timeSynced;
    M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, 240 - FOOTER_Y, TFT_BLACK);
    lastMapMinute = -1;
    if (screen == Screen::Info) drawInfo(now, true);
  }

  // Button B: toggle map style (remembered in NVS).
  // Short click: map style. Long press: overlays.
  if ((!swallow && M5.BtnB.wasHold()) || cmd == 'B') {
    overlay = (Overlay)(((uint8_t)overlay + 1) % 3);
    prefs.putUChar("overlay", (uint8_t)overlay);
    if (screen == Screen::Map) lastMapMinute = -1;
  }
  if ((!swallow && M5.BtnB.wasClicked()) || cmd == 'b') {   // 'b' over serial for testing
    mapStyle = mapStyle == MapStyle::Photo ? MapStyle::Zones : MapStyle::Photo;
    prefs.putUChar("style", (uint8_t)mapStyle);
    if (screen == Screen::Map) lastMapMinute = -1;
  }

  // Button C: toggle map / sun info screen.
  if ((!swallow && M5.BtnC.wasPressed()) || cmd == 'c') {   // 'c' over serial for testing
    if (screen == Screen::Map) {
      screen = Screen::Info;
      drawInfo(now, true);
    } else {
      screen = Screen::Map;
      M5.Display.fillRect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, TFT_BLACK);
      lastMapMinute = -1;
    }
  }

  // Refresh the info screen when the local date changes.
  static time_t lastInfoDay = 0;
  time_t day = localMidnightUtc(now);
  if (screen == Screen::Info && day != lastInfoDay) drawInfo(now, true);
  lastInfoDay = day;
  delay(20);
}
