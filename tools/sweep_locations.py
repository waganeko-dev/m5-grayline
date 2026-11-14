#!/usr/bin/env python3
"""Screen check on the real device: show the map and info screens for a list of
test locations and combine all screenshots into one contact sheet PNG.

Usage:
  python tools/sweep_locations.py <port> <out_dir>

Needs pyserial (bundled with PlatformIO; use the Python that PlatformIO runs on,
or `pip install pyserial`).

Each location is applied with the serial "L" command (temporary, not saved).
Opening the port may reboot the device, so every shot waits for the first map.
Look through <out_dir>/sheet.png for clipped text, leftovers and wrong
"--:--" / Polar night / Midnight sun states.
"""
import os
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))

# key, lat, lon, tz offset [min], tz label, name  -- edge cases on purpose
LOCATIONS = [
    ("01_tokyo", 35.6895, 139.6917, 540, "JST", "Tokyo"),
    ("02_london", 51.5074, -0.1278, 0, "GMT", "London"),
    ("03_newyork", 40.7128, -74.0060, -300, "EST", "New York"),
    ("04_capetown", -33.9249, 18.4241, 120, "SAST", "Cape Town"),
    ("05_quito", -0.1807, -78.4678, -300, "ECT", "Quito"),
    ("06_kathmandu", 27.7172, 85.3240, 345, "NPT", "Kathmandu"),
    ("07_kiritimati", 1.8721, -157.4278, 840, "LINT", "Kiritimati"),
    ("08_syowa", -69.0042, 39.5817, 180, "SYOT", "Syowa Station"),
    ("09_longyearbyen", 78.2232, 15.6267, 60, "CET", "Longyearbyen"),
    ("10_mcmurdo", -77.8463, 166.6682, 780, "NZDT", "McMurdo Station"),
    ("11_longname", 64.1466, -21.9426, 0, "GMT", "Reykjavik Capital Region Very Long Name"),
]


def read_png(path):
    b = open(path, "rb").read()
    i, idat, w, h = 8, b"", 0, 0
    while i < len(b):
        n = struct.unpack(">I", b[i:i + 4])[0]
        t, c = b[i + 4:i + 8], b[i + 8:i + 8 + n]
        if t == b"IHDR":
            w, h = struct.unpack(">II", c[:8])
        elif t == b"IDAT":
            idat += c
        i += 12 + n
    raw = zlib.decompress(idat)   # written by screenshot.py: RGB, filter 0
    return w, h, [raw[y * (w * 3 + 1) + 1:(y + 1) * (w * 3 + 1)] for y in range(h)]


def write_sheet(out_dir, keys, path):
    gap, w, h = 8, 320, 240
    W, H = w * 2 + gap * 3, (h + gap) * len(keys) + gap
    canvas = [bytearray(b"\x30\x30\x30" * W) for _ in range(H)]
    for k, key in enumerate(keys):
        for j, kind in enumerate(("map", "info")):
            _, _, rows = read_png(os.path.join(out_dir, f"{key}_{kind}.png"))
            x0, y0 = gap + j * (w + gap), gap + k * (h + gap)
            for y, row in enumerate(rows):
                canvas[y0 + y][x0 * 3:(x0 + w) * 3] = row
    raw = b"".join(b"\x00" + bytes(r) for r in canvas)
    chunk = lambda t, c: struct.pack(">I", len(c)) + t + c + struct.pack(">I", zlib.crc32(t + c) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def main():
    port, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    shot = [sys.executable, os.path.join(HERE, "screenshot.py"), port]
    for key, lat, lon, tz, label, name in LOCATIONS:
        cmd = f"L{lat},{lon},{tz},{label},{name}\n"
        subprocess.run(shot + [os.path.join(out_dir, f"{key}_map.png"), cmd], check=True)
        subprocess.run(shot + [os.path.join(out_dir, f"{key}_info.png"), cmd + "c"], check=True)
    sheet = os.path.join(out_dir, "sheet.png")
    write_sheet(out_dir, [l[0] for l in LOCATIONS], sheet)
    print("sheet:", sheet)


if __name__ == "__main__":
    main()
