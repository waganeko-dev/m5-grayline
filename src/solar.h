#pragma once
// Sun position after the NOAA Solar Calculator (based on Meeus, "Astronomical
// Algorithms"). Accuracy is about 0.01 deg in declination and a few seconds
// in the equation of time for years 1800-2100.
//
// An earlier version used the shorter fractional-year Fourier series; the
// unit tests showed it drifts by up to ~0.4 deg in declination around the
// equinoxes (it ignores the leap-year cycle), so it was replaced.

#include <math.h>
#include <time.h>

struct SolarPosition {
  float declinationRad;      // solar declination
  float equationOfTimeMin;   // equation of time [minutes]
  double subsolarLonDeg;     // longitude where the sun is at the zenith [-180, 180)
};

inline SolarPosition computeSolarPosition(time_t utcEpoch) {
  constexpr double D2R = M_PI / 180.0;
  const double jd = utcEpoch / 86400.0 + 2440587.5;
  const double T = (jd - 2451545.0) / 36525.0;   // Julian centuries since J2000.0

  const double L0 = fmod(280.46646 + T * (36000.76983 + T * 0.0003032), 360.0);   // mean longitude
  const double M = 357.52911 + T * (35999.05029 - 0.0001537 * T);                 // mean anomaly
  const double e = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);            // eccentricity
  const double Mr = M * D2R;
  const double C = sin(Mr) * (1.914602 - T * (0.004817 + 0.000014 * T))
                   + sin(2 * Mr) * (0.019993 - 0.000101 * T) + sin(3 * Mr) * 0.000289;
  const double omega = (125.04 - 1934.136 * T) * D2R;
  const double lambda = (L0 + C - 0.00569 - 0.00478 * sin(omega)) * D2R;          // apparent longitude
  const double eps0 = 23.0 + (26.0 + (21.448 - T * (46.815 + T * (0.00059 - T * 0.001813))) / 60.0) / 60.0;
  const double eps = (eps0 + 0.00256 * cos(omega)) * D2R;                          // obliquity

  const double decl = asin(sin(eps) * sin(lambda));

  const double y = tan(eps / 2) * tan(eps / 2);
  const double L0r = L0 * D2R;
  const double eqtime = 4.0 / D2R * (y * sin(2 * L0r) - 2 * e * sin(Mr)
                                     + 4 * e * y * sin(Mr) * cos(2 * L0r)
                                     - 0.5 * y * y * sin(4 * L0r) - 1.25 * e * e * sin(2 * Mr));

  // The sun is overhead where apparent solar time is 12:00.
  struct tm utc;
  gmtime_r(&utcEpoch, &utc);
  const double hours = utc.tm_hour + utc.tm_min / 60.0 + utc.tm_sec / 3600.0;
  double lon = -15.0 * (hours - 12.0 + eqtime / 60.0);
  while (lon < -180.0) lon += 360.0;
  while (lon >= 180.0) lon -= 360.0;

  SolarPosition p;
  p.declinationRad = (float)decl;
  p.equationOfTimeMin = (float)eqtime;
  p.subsolarLonDeg = lon;
  return p;
}
