#!/usr/bin/env python3
"""Grab a screenshot from the device over serial and save it as PNG.

Usage: python tools/screenshot.py <port> out.png [keys]
  port: serial port, e.g. /dev/ttyUSB0 (Linux), /dev/cu.usbserial-XXXX (macOS), COM3 (Windows)
  keys: optional serial commands sent first, e.g. "c" to press button C.
Needs pyserial (bundled with PlatformIO). The PNG is written without extra packages.
"""
import struct
import sys
import time
import zlib

import serial

W, H = 320, 240


def write_png(path, rgb):
    raw = b"".join(b"\x00" + rgb[y * W * 3:(y + 1) * W * 3] for y in range(H))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    port, out = sys.argv[1], sys.argv[2]
    # The CH9102 auto-reset circuit may reboot the device when the port is
    # opened, so wait until the first map has been drawn after boot.
    s = serial.Serial(port, 115200, timeout=0.5)
    s.dtr = False
    s.rts = False
    log = b""
    boot_deadline = time.time() + 40
    while b"map redraw" not in log and time.time() < boot_deadline:
        log += s.read(256)
    time.sleep(0.5)
    if len(sys.argv) > 3:
        s.write(sys.argv[3].encode())
        time.sleep(1.5)
    need = W * H * 2
    for attempt in range(3):   # bytes are occasionally lost on the USB serial link
        s.reset_input_buffer()
        s.write(b"S")
        buf = b""
        deadline = time.time() + 30
        while b"SCRN" not in buf and time.time() < deadline:
            buf += s.read(64)
        if b"SCRN" not in buf:
            continue
        buf = buf[buf.index(b"SCRN") + 4:]
        while len(buf) < need + 4 and time.time() < deadline:
            buf += s.read(need + 4 - len(buf))
        if len(buf) >= need + 4 and sum(buf[:need]) & 0xFFFFFFFF == struct.unpack("<I", buf[need:need + 4])[0]:
            break
        print(f"checksum mismatch, retrying ({attempt + 1})", file=sys.stderr)
        time.sleep(1)
    else:
        sys.exit("screenshot failed")
    rgb = bytearray()
    for i in range(0, need, 2):
        v = (buf[i + 1] << 8) | buf[i]  # readRect returns byte-swapped RGB565
        rgb += bytes(((v >> 11 & 0x1F) * 255 // 31, (v >> 5 & 0x3F) * 255 // 63, (v & 0x1F) * 255 // 31))
    write_png(out, bytes(rgb))
    print("saved", out)


if __name__ == "__main__":
    main()
