#pragma once
// Low-precision Moon position (Astronomical Almanac, "low precision formulae
// for the Moon", about 0.3 deg) and illuminated fraction. Parallax is ignored,
// so the sublunar point can be off by up to ~1 deg, which is about one map pixel.

#include <math.h>
#include <time.h>

struct MoonPosition {
  double sublunarLatDeg;   // = geocentric declination
  double sublunarLonDeg;   // [-180, 180)
  double illumination;     // 0 (new) .. 1 (full)
  bool waxing;
};

inline double moonDegSin(double d) { return sin(d * M_PI / 180.0); }

inline MoonPosition computeMoonPosition(time_t utcEpoch) {
  const double d = utcEpoch / 86400.0 + 2440587.5 - 2451545.0;   // days since J2000.0
  const double T = d / 36525.0;

  // Ecliptic longitude / latitude of the Moon [deg].
  const double L = 218.32 + 481267.881 * T
                   + 6.29 * moonDegSin(135.0 + 477198.87 * T)
                   - 1.27 * moonDegSin(259.3 - 413335.36 * T)
                   + 0.66 * moonDegSin(235.7 + 890534.22 * T)
                   + 0.21 * moonDegSin(269.9 + 954397.74 * T)
                   - 0.19 * moonDegSin(357.5 + 35999.05 * T)
                   - 0.11 * moonDegSin(186.5 + 966404.03 * T);
  const double B = 5.13 * moonDegSin(93.3 + 483202.02 * T)
                   + 0.28 * moonDegSin(228.2 + 960400.89 * T)
                   - 0.28 * moonDegSin(318.3 + 6003.15 * T)
                   - 0.17 * moonDegSin(217.6 - 407332.21 * T);

  const double l = L * M_PI / 180.0, b = B * M_PI / 180.0;
  const double x = cos(b) * cos(l);
  const double y = 0.9175 * cos(b) * sin(l) - 0.3978 * sin(b);
  const double z = 0.3978 * cos(b) * sin(l) + 0.9175 * sin(b);
  const double ra = atan2(y, x) * 180.0 / M_PI;
  const double dec = asin(z) * 180.0 / M_PI;

  // Greenwich mean sidereal time [deg].
  const double gmst = 280.46061837 + 360.98564736629 * d;
  double lon = fmod(ra - gmst, 360.0);
  if (lon < -180.0) lon += 360.0;
  if (lon >= 180.0) lon -= 360.0;

  // Sun's ecliptic longitude for the phase.
  const double g = 357.528 + 0.9856003 * d;
  const double sunL = 280.460 + 0.9856474 * d + 1.915 * moonDegSin(g) + 0.020 * moonDegSin(2 * g);
  const double dl = (L - sunL) * M_PI / 180.0;
  const double cosPsi = cos(b) * cos(dl);   // cos of Sun-Moon elongation

  MoonPosition m;
  m.sublunarLatDeg = dec;
  m.sublunarLonDeg = lon;
  m.illumination = (1.0 - cosPsi) / 2.0;
  m.waxing = sin(dl) > 0;
  return m;
}

// Maidenhead locator (6 characters) for a position.
inline void maidenhead(double latDeg, double lonDeg, char out[7]) {
  double lon = lonDeg + 180.0, lat = latDeg + 90.0;
  if (lon >= 360.0) lon -= 360.0;
  if (lat >= 180.0) lat = 179.9999;
  out[0] = 'A' + (int)(lon / 20);
  out[1] = 'A' + (int)(lat / 10);
  out[2] = '0' + (int)(fmod(lon, 20) / 2);
  out[3] = '0' + (int)(fmod(lat, 10) / 1);
  out[4] = 'a' + (int)(fmod(lon, 2) * 12);
  out[5] = 'a' + (int)(fmod(lat, 1) * 24);
  out[6] = 0;
}
