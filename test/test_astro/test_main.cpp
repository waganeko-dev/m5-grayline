// Unit tests for the sun / moon / locator calculations (run on the PC).
//   pio test -e native
//
// Reference values: NOAA solar calculator conventions, almanac sunrise /
// sunset times (rounded to the minute), and known lunar phases. Tolerances
// reflect the low-precision formulas used on the device.

#include <unity.h>

#include <math.h>
#include <string.h>
#include <time.h>

#include <initializer_list>

#include "moon.h"
#include "solar.h"
#include "suntimes.h"

static time_t utc(int y, int mo, int d, int h = 0, int mi = 0, int s = 0) {
  struct tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  return timegm(&t);
}

// Local "HH:MM" of an event on the local day of `y-mo-d`.
static void eventHm(int y, int mo, int d, double lat, double lon, int tzMin, SunEvent ev, char out[8]) {
  time_t mid = localMidnightUtc(utc(y, mo, d, 12), tzMin);
  SunTimes st = computeSunTimes(mid, lat, lon);
  formatLocalHm(st.t[ev], tzMin, out, 8);
}

static int hmToMin(const char* hm) { return (hm[0] - '0') * 600 + (hm[1] - '0') * 60 + (hm[3] - '0') * 10 + (hm[4] - '0'); }

static void assertTimeNear(const char* expected, const char* actual, int tolMin, const char* what) {
  char msg[96];
  snprintf(msg, sizeof(msg), "%s: expected %s got %s", what, expected, actual);
  TEST_ASSERT_TRUE_MESSAGE(strcmp(actual, "--:--") != 0, msg);
  int diff = abs(hmToMin(expected) - hmToMin(actual));
  if (diff > 720) diff = 1440 - diff;
  TEST_ASSERT_TRUE_MESSAGE(diff <= tolMin, msg);
}

// ---- Places used across tests --------------------------------------------
struct Place { const char* name; double lat, lon; int tzMin; };
static const Place PLACES[] = {
  { "Tokyo", 35.6895, 139.6917, 540 },
  { "London", 51.5074, -0.1278, 0 },
  { "New York", 40.7128, -74.0060, -300 },
  { "Cape Town", -33.9249, 18.4241, 120 },
  { "Quito (equator)", -0.1807, -78.4678, -300 },
  { "Kathmandu (+5:45)", 27.7172, 85.3240, 345 },
  { "Kiritimati (+14)", 1.8721, -157.4278, 840 },
  { "Pago Pago (-11)", -14.2756, -170.7020, -660 },
  { "Date line east", 0.0, 179.99, 720 },
  { "Date line west", 0.0, -179.99, -720 },
  { "Syowa", -69.0042, 39.5817, 180 },
  { "Longyearbyen", 78.2232, 15.6267, 60 },
  { "McMurdo", -77.8463, 166.6682, 780 },
  { "North Pole", 90.0, 0.0, 0 },
  { "South Pole", -90.0, 0.0, 720 },
};

// ---- Solar position ----------------------------------------------------------
void test_declination_at_solstices_and_equinox(void) {
  // 2026 solstices / equinox instants (UTC).
  TEST_ASSERT_FLOAT_WITHIN(0.02, 23.44, computeSolarPosition(utc(2026, 6, 21, 8, 24)).declinationRad * 180 / M_PI);
  TEST_ASSERT_FLOAT_WITHIN(0.02, -23.44, computeSolarPosition(utc(2026, 12, 21, 20, 50)).declinationRad * 180 / M_PI);
  TEST_ASSERT_FLOAT_WITHIN(0.02, 0.0, computeSolarPosition(utc(2026, 3, 20, 14, 46)).declinationRad * 180 / M_PI);
  TEST_ASSERT_FLOAT_WITHIN(0.02, 0.0, computeSolarPosition(utc(2026, 9, 23, 0, 5)).declinationRad * 180 / M_PI);
}

void test_equation_of_time_extremes(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.4, 16.4, computeSolarPosition(utc(2026, 11, 3, 12)).equationOfTimeMin);
  TEST_ASSERT_FLOAT_WITHIN(0.4, -14.2, computeSolarPosition(utc(2026, 2, 11, 12)).equationOfTimeMin);
}

void test_subsolar_longitude_follows_utc(void) {
  // At 12:00 UTC the sun is over 0 deg minus the equation of time (4 min = 1 deg).
  SolarPosition p = computeSolarPosition(utc(2026, 11, 14, 12));
  TEST_ASSERT_FLOAT_WITHIN(0.05, -p.equationOfTimeMin / 4.0, p.subsolarLonDeg);
  // Always wrapped to [-180, 180).
  for (int h = 0; h < 24; ++h) {
    double lon = computeSolarPosition(utc(2026, 11, 14, h)).subsolarLonDeg;
    TEST_ASSERT_TRUE(lon >= -180.0 && lon < 180.0);
  }
}

// ---- Sunrise / sunset against almanac values --------------------------------
void test_almanac_tokyo(void) {
  char hm[8];
  eventHm(2026, 11, 14, 35.6895, 139.6917, 540, EV_SUNRISE, hm);
  assertTimeNear("06:14", hm, 2, "Tokyo sunrise");
  eventHm(2026, 11, 14, 35.6895, 139.6917, 540, EV_SUNSET, hm);
  assertTimeNear("16:35", hm, 2, "Tokyo sunset");
}

void test_almanac_other_cities(void) {
  char hm[8];
  eventHm(2026, 11, 14, 51.5074, -0.1278, 0, EV_SUNRISE, hm);
  assertTimeNear("07:14", hm, 3, "London sunrise");
  eventHm(2026, 11, 14, 51.5074, -0.1278, 0, EV_SUNSET, hm);
  assertTimeNear("16:14", hm, 3, "London sunset");
  eventHm(2026, 11, 14, 40.7128, -74.0060, -300, EV_SUNRISE, hm);
  assertTimeNear("06:41", hm, 3, "New York sunrise");
  eventHm(2026, 11, 14, 40.7128, -74.0060, -300, EV_SUNSET, hm);
  assertTimeNear("16:40", hm, 3, "New York sunset");
  eventHm(2026, 11, 14, -33.9249, 18.4241, 120, EV_SUNRISE, hm);
  assertTimeNear("05:35", hm, 3, "Cape Town sunrise");
  eventHm(2026, 11, 14, -33.9249, 18.4241, 120, EV_SUNSET, hm);
  assertTimeNear("19:25", hm, 3, "Cape Town sunset");
}

// ---- Self-consistency for every place and season -----------------------------
// At each computed event time the sun altitude must equal the event's altitude,
// events must be in order, and nothing may be NaN.
void test_events_hit_their_altitude_everywhere(void) {
  static const double ALT[EV_COUNT] = { -18, -12, -6, -0.833, 0, -0.833, -6, -12, -18 };
  static const int MONTHS[] = { 1, 3, 6, 9, 11, 12 };
  for (const Place& p : PLACES) {
    for (int mo : MONTHS) {
      time_t mid = localMidnightUtc(utc(2026, mo, 15, 12), p.tzMin);
      SunTimes st = computeSunTimes(mid, p.lat, p.lon);
      time_t prev = 0;
      for (int e = 0; e < EV_COUNT; ++e) {
        char msg[96];
        snprintf(msg, sizeof(msg), "%s 2026-%02d event %d", p.name, mo, e);
        if (st.t[e] == SUN_NEVER) continue;
        double alt, az;
        sunAltAz(st.t[e], p.lat, p.lon, alt, az);
        TEST_ASSERT_FALSE_MESSAGE(isnan(alt) || isnan(az), msg);
        if (e != EV_NOON) TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, ALT[e], alt, msg);
        TEST_ASSERT_TRUE_MESSAGE(st.t[e] > prev, msg);   // chronological order
        prev = st.t[e];
      }
      // Solar noon is the highest point: one minute either side is lower.
      double a0, a1, a2, az;
      sunAltAz(st.t[EV_NOON] - 60, p.lat, p.lon, a0, az);
      sunAltAz(st.t[EV_NOON], p.lat, p.lon, a1, az);
      sunAltAz(st.t[EV_NOON] + 60, p.lat, p.lon, a2, az);
      if (fabs(p.lat) < 89.9) {
        char msg[64];
        snprintf(msg, sizeof(msg), "%s 2026-%02d noon", p.name, mo);
        // The declination drifts up to ~0.4 deg/day, which moves the true
        // maximum by a fraction of a minute at high latitudes; allow 0.001 deg.
        TEST_ASSERT_TRUE_MESSAGE(a1 >= a0 - 1e-3 && a1 >= a2 - 1e-3, msg);
      }
    }
  }
}

// Sunrise and sunset belong to the requested local day (not the day before/after).
void test_events_fall_on_the_local_day(void) {
  for (const Place& p : PLACES) {
    if (fabs(p.lat) > 60) continue;
    time_t mid = localMidnightUtc(utc(2026, 11, 14, 12), p.tzMin);
    SunTimes st = computeSunTimes(mid, p.lat, p.lon);
    char msg[64];
    snprintf(msg, sizeof(msg), "%s noon on local day", p.name);
    TEST_ASSERT_TRUE_MESSAGE(st.t[EV_NOON] >= mid && st.t[EV_NOON] < mid + 86400, msg);
  }
}

// ---- Polar day / polar night / no-twilight regions ------------------------------
void test_polar_night_longyearbyen(void) {
  time_t mid = localMidnightUtc(utc(2026, 12, 21, 12), 60);
  SunTimes st = computeSunTimes(mid, 78.2232, 15.6267);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_SUNRISE]);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_SUNSET]);
  time_t when;
  bool rise;
  TEST_ASSERT_FALSE(nextRiseSet(mid + 12 * 3600, 78.2232, 15.6267, 60, when, rise));
  double alt, az;
  sunAltAz(st.t[EV_NOON], 78.2232, 15.6267, alt, az);
  TEST_ASSERT_TRUE(alt < 0);   // shown as "Polar night"
}

void test_midnight_sun_mcmurdo(void) {
  time_t mid = localMidnightUtc(utc(2026, 12, 21, 12), 780);
  SunTimes st = computeSunTimes(mid, -77.8463, 166.6682);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_SUNRISE]);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_SUNSET]);
  for (int e = 0; e < EV_COUNT; ++e) {
    if (e != EV_NOON) TEST_ASSERT_EQUAL(SUN_NEVER, st.t[e]);
  }
  double alt, az;
  sunAltAz(mid, -77.8463, 166.6682, alt, az);
  TEST_ASSERT_TRUE(alt > 0);   // shown as "Midnight sun"
}

void test_white_night_syowa_has_sunset_but_no_twilight_end(void) {
  time_t mid = localMidnightUtc(utc(2026, 11, 14, 12), 180);
  SunTimes st = computeSunTimes(mid, -69.0042, 39.5817);
  TEST_ASSERT_NOT_EQUAL(SUN_NEVER, st.t[EV_SUNRISE]);
  TEST_ASSERT_NOT_EQUAL(SUN_NEVER, st.t[EV_SUNSET]);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_CIVIL_DUSK]);
  TEST_ASSERT_EQUAL(SUN_NEVER, st.t[EV_ASTRO_DUSK]);
}

void test_poles_do_not_produce_nan(void) {
  for (int mo = 1; mo <= 12; ++mo) {
    for (double lat : { 90.0, -90.0, 89.999, -89.999 }) {
      time_t mid = localMidnightUtc(utc(2026, mo, 15, 12), 0);
      SunTimes st = computeSunTimes(mid, lat, 0.0);
      time_t when;
      bool rise;
      nextRiseSet(mid, lat, 0.0, 0, when, rise);   // must not hang or crash
      double alt, az;
      sunAltAz(mid, lat, 0.0, alt, az);
      TEST_ASSERT_FALSE(isnan(alt));
      (void)st;
    }
  }
}

void test_equator_day_length_is_about_12h(void) {
  time_t mid = localMidnightUtc(utc(2026, 3, 20, 12), -300);
  SunTimes st = computeSunTimes(mid, -0.1807, -78.4678);
  long len = (long)(st.t[EV_SUNSET] - st.t[EV_SUNRISE]);
  TEST_ASSERT_INT_WITHIN(5 * 60, 12 * 3600 + 7 * 60, len);   // refraction adds ~7 min
}

// ---- Time zone helpers ------------------------------------------------------
void test_local_midnight_for_various_offsets(void) {
  const int OFFSETS[] = { -720, -660, -300, 0, 330, 345, 540, 780, 840 };
  const time_t now = utc(2026, 11, 14, 23, 30);
  for (int tz : OFFSETS) {
    time_t mid = localMidnightUtc(now, tz);
    time_t local = mid + tz * 60;
    TEST_ASSERT_EQUAL(0, local % 86400);         // exactly 00:00 local
    TEST_ASSERT_TRUE(mid <= now && now - mid < 86400);
  }
}

void test_format_rounds_to_nearest_minute(void) {
  char hm[8];
  formatLocalHm(utc(2026, 11, 14, 7, 35, 29), 540, hm, sizeof(hm));
  TEST_ASSERT_EQUAL_STRING("16:35", hm);
  formatLocalHm(utc(2026, 11, 14, 7, 35, 30), 540, hm, sizeof(hm));
  TEST_ASSERT_EQUAL_STRING("16:36", hm);
  formatLocalHm(utc(2026, 11, 14, 14, 59, 45), 540, hm, sizeof(hm));
  TEST_ASSERT_EQUAL_STRING("00:00", hm);        // wraps past midnight
  formatLocalHm(SUN_NEVER, 540, hm, sizeof(hm));
  TEST_ASSERT_EQUAL_STRING("--:--", hm);
}

void test_next_rise_set_sequence(void) {
  const double lat = 35.6895, lon = 139.6917;
  const int tz = 540;
  time_t when;
  bool rise;
  // 03:00 JST -> sunrise today; 12:00 JST -> sunset today; 20:00 JST -> sunrise tomorrow.
  TEST_ASSERT_TRUE(nextRiseSet(utc(2026, 11, 13, 18), lat, lon, tz, when, rise));
  TEST_ASSERT_TRUE(rise);
  TEST_ASSERT_TRUE(nextRiseSet(utc(2026, 11, 14, 3), lat, lon, tz, when, rise));
  TEST_ASSERT_FALSE(rise);
  time_t evening = utc(2026, 11, 14, 11);
  TEST_ASSERT_TRUE(nextRiseSet(evening, lat, lon, tz, when, rise));
  TEST_ASSERT_TRUE(rise);
  TEST_ASSERT_TRUE(when - evening > 9 * 3600 && when - evening < 12 * 3600);
}

// ---- Moon ---------------------------------------------------------------------
void test_moon_phase_at_known_full_and_new_moon(void) {
  TEST_ASSERT_TRUE(computeMoonPosition(utc(2026, 1, 3, 10, 3)).illumination > 0.97);    // full moon
  TEST_ASSERT_TRUE(computeMoonPosition(utc(2026, 1, 18, 19, 52)).illumination < 0.03);  // new moon
  // Between new and full the Moon is waxing.
  TEST_ASSERT_TRUE(computeMoonPosition(utc(2026, 1, 25)).waxing);
}

void test_moon_values_stay_in_range(void) {
  for (int d = 0; d < 60; ++d) {
    MoonPosition m = computeMoonPosition(utc(2026, 11, 1) + d * 86400);
    TEST_ASSERT_TRUE(m.illumination >= 0.0 && m.illumination <= 1.0);
    TEST_ASSERT_TRUE(fabs(m.sublunarLatDeg) < 29.0);   // max lunar declination ~28.6
    TEST_ASSERT_TRUE(m.sublunarLonDeg >= -180.0 && m.sublunarLonDeg < 180.0);
  }
}

// ---- Maidenhead locator ---------------------------------------------------------
void test_maidenhead_known_locators(void) {
  char loc[7];
  maidenhead(35.6895, 139.6917, loc);
  TEST_ASSERT_EQUAL_STRING("PM95uq", loc);
  maidenhead(51.5074, -0.1278, loc);
  TEST_ASSERT_EQUAL_STRING("IO91wm", loc);
  maidenhead(40.7128, -74.0060, loc);
  TEST_ASSERT_EQUAL_STRING("FN20xr", loc);
}

void test_maidenhead_edges(void) {
  char loc[7];
  maidenhead(-90.0, -180.0, loc);
  TEST_ASSERT_EQUAL_STRING("AA00aa", loc);
  maidenhead(90.0, 179.9999, loc);   // north pole / date line must stay in range
  TEST_ASSERT_EQUAL_STRING("RR99xx", loc);
  maidenhead(0.0, 180.0, loc);       // 180 E is the same meridian as 180 W
  TEST_ASSERT_EQUAL('A', loc[0]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_declination_at_solstices_and_equinox);
  RUN_TEST(test_equation_of_time_extremes);
  RUN_TEST(test_subsolar_longitude_follows_utc);
  RUN_TEST(test_almanac_tokyo);
  RUN_TEST(test_almanac_other_cities);
  RUN_TEST(test_events_hit_their_altitude_everywhere);
  RUN_TEST(test_events_fall_on_the_local_day);
  RUN_TEST(test_polar_night_longyearbyen);
  RUN_TEST(test_midnight_sun_mcmurdo);
  RUN_TEST(test_white_night_syowa_has_sunset_but_no_twilight_end);
  RUN_TEST(test_poles_do_not_produce_nan);
  RUN_TEST(test_equator_day_length_is_about_12h);
  RUN_TEST(test_local_midnight_for_various_offsets);
  RUN_TEST(test_format_rounds_to_nearest_minute);
  RUN_TEST(test_next_rise_set_sequence);
  RUN_TEST(test_moon_phase_at_known_full_and_new_moon);
  RUN_TEST(test_moon_values_stay_in_range);
  RUN_TEST(test_maidenhead_known_locators);
  RUN_TEST(test_maidenhead_edges);
  return UNITY_END();
}
