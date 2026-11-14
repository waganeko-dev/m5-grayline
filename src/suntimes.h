#pragma once
// Sunrise / sunset / twilight times for one location, built on solar.h.
//
// The hour angle at the observer is H(t) = lon - subsolarLon(t). An event
// with solar altitude h happens when cos H = (sin h - sin lat sin dec) /
// (cos lat cos dec). Times are refined a few times because the declination
// and the equation of time change during the day.

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "solar.h"

enum SunEvent {
  EV_ASTRO_DAWN, EV_NAUT_DAWN, EV_CIVIL_DAWN, EV_SUNRISE,
  EV_NOON,
  EV_SUNSET, EV_CIVIL_DUSK, EV_NAUT_DUSK, EV_ASTRO_DUSK,
  EV_COUNT
};

constexpr time_t SUN_NEVER = 0;   // event does not happen that day (polar day/night)

struct SunTimes {
  time_t t[EV_COUNT];   // UTC epoch, or SUN_NEVER
};

inline double wrap180(double d) {
  while (d < -180.0) d += 360.0;
  while (d >= 180.0) d -= 360.0;
  return d;
}

// Hour angle of the sun at the observer [deg], -180..180, 0 at solar noon.
inline double hourAngleDeg(time_t t, double lonDeg) {
  return wrap180(lonDeg - computeSolarPosition(t).subsolarLonDeg);
}

inline time_t solarNoon(time_t guess, double lonDeg) {
  time_t t = guess;
  for (int i = 0; i < 3; ++i) t -= (time_t)(hourAngleDeg(t, lonDeg) / 15.0 * 3600.0);
  return t;
}

// rising: before noon. Returns SUN_NEVER if the altitude is never crossed.
inline time_t sunEventTime(time_t noon, double latDeg, double lonDeg, double altDeg, bool rising) {
  const double lat = latDeg * M_PI / 180.0;
  time_t t = noon;
  for (int i = 0; i < 4; ++i) {
    const double dec = computeSolarPosition(t).declinationRad;
    const double c = (sin(altDeg * M_PI / 180.0) - sin(lat) * sin(dec)) / (cos(lat) * cos(dec));
    if (c < -1.0 || c > 1.0) return SUN_NEVER;
    const double h0 = acos(c) * 180.0 / M_PI;
    const double target = rising ? -h0 : h0;
    t += (time_t)(wrap180(target - hourAngleDeg(t, lonDeg)) / 15.0 * 3600.0);
  }
  return t;
}

// localMidnightUtc: UTC epoch of 00:00 local time of the wanted day.
inline SunTimes computeSunTimes(time_t localMidnightUtc, double latDeg, double lonDeg) {
  static const double ALT[4] = { -18.0, -12.0, -6.0, -0.833 };
  SunTimes s;
  const time_t noon = solarNoon(localMidnightUtc + 12 * 3600, lonDeg);
  s.t[EV_NOON] = noon;
  for (int i = 0; i < 4; ++i) {
    s.t[EV_ASTRO_DAWN + i] = sunEventTime(noon, latDeg, lonDeg, ALT[i], true);
    s.t[EV_ASTRO_DUSK - i] = sunEventTime(noon, latDeg, lonDeg, ALT[i], false);
  }
  return s;
}

// UTC epoch of local midnight for the local day containing `utc`.
inline time_t localMidnightUtc(time_t utc, int tzOffsetMin) {
  const time_t local = utc + (time_t)tzOffsetMin * 60;
  return utc - (time_t)((local % 86400 + 86400) % 86400);
}

// Next sunrise or sunset after `now` (searching today and the next two days).
// Returns false during polar day / polar night.
inline bool nextRiseSet(time_t now, double latDeg, double lonDeg, int tzOffsetMin,
                        time_t& when, bool& isSunrise) {
  const time_t mid = localMidnightUtc(now, tzOffsetMin);
  for (int d = 0; d < 3; ++d) {
    const SunTimes st = computeSunTimes(mid + d * 86400, latDeg, lonDeg);
    const time_t cand[2] = { st.t[EV_SUNRISE], st.t[EV_SUNSET] };
    time_t best = 0;
    int bestIdx = -1;
    for (int i = 0; i < 2; ++i) {
      if (cand[i] != SUN_NEVER && cand[i] > now && (bestIdx < 0 || cand[i] < best)) {
        best = cand[i];
        bestIdx = i;
      }
    }
    if (bestIdx >= 0) { when = best; isSunrise = bestIdx == 0; return true; }
  }
  return false;
}

// "HH:MM" in local time, rounded to the nearest minute like an almanac.
// SUN_NEVER gives "--:--".
inline void formatLocalHm(time_t utc, int tzOffsetMin, char* buf, size_t n) {
  if (utc == SUN_NEVER) { snprintf(buf, n, "--:--"); return; }
  const time_t local = utc + (time_t)tzOffsetMin * 60 + 30;
  struct tm t;
  gmtime_r(&local, &t);
  snprintf(buf, n, "%02d:%02d", t.tm_hour, t.tm_min);
}

// Current altitude and azimuth (north = 0, east = 90) in degrees.
inline void sunAltAz(time_t t, double latDeg, double lonDeg, double& altDeg, double& azDeg) {
  const SolarPosition p = computeSolarPosition(t);
  const double lat = latDeg * M_PI / 180.0;
  const double dec = p.declinationRad;
  const double h = hourAngleDeg(t, lonDeg) * M_PI / 180.0;
  const double sinAlt = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(h);
  altDeg = asin(sinAlt) * 180.0 / M_PI;
  double az = atan2(sin(h), cos(h) * sin(lat) - tan(dec) * cos(lat)) * 180.0 / M_PI + 180.0;
  azDeg = fmod(az + 360.0, 360.0);
}
