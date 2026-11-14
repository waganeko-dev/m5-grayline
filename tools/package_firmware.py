#!/usr/bin/env python3
"""Package the PlatformIO build into release images and Web Flasher files.

Usage (after `pio run`):
  python tools/package_firmware.py [--version vX.Y.Z] [--out out] [--web docs/public]

Writes:
  <out>/m5-grayline-<version>-full.bin   bootloader + partitions + app, flashed at 0x0
  <out>/m5-grayline-<version>-app.bin    application only, flashed at 0x10000
  <web>/firmware/<version>/m5-grayline-<version>-full.bin   (with --web)
  <web>/firmware/manifest.json                              (with --web, for ESP Web Tools)
With --web, <web>/firmware/ is recreated, so it holds only this version.

The flash layout (offsets of the bootloader, partition table and boot_app0)
is read from `pio project metadata`, so it follows platformio.ini.
The version defaults to `git describe --tags --always --dirty`.
esptool is taken from PATH, then `python -m esptool`, then PlatformIO's
tool-esptool package.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys

ENV = "m5stack-core-16m"
NAME = "m5-grayline"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run(cmd, **kw):
    return subprocess.run(cmd, check=True, cwd=ROOT, **kw)


def git_version():
    try:
        return subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"],
                                       cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return "dev"


def esptool_cmd():
    exe = shutil.which("esptool") or shutil.which("esptool.py")
    if exe:
        return [exe]
    try:
        subprocess.run([sys.executable, "-m", "esptool", "version"], check=True,
                       capture_output=True)
        return [sys.executable, "-m", "esptool"]
    except Exception:
        pass
    pkg = os.path.expanduser("~/.platformio/packages/tool-esptool/esptool")
    if os.path.exists(pkg):
        return [pkg]
    sys.exit("esptool not found (pip install esptool)")


def flash_layout():
    out = subprocess.check_output(["pio", "project", "metadata", "-e", ENV, "--json-output"],
                                  cwd=ROOT, text=True)
    # pio may print other lines before the JSON document
    meta = json.loads(out[out.index("{"):])[ENV]
    extra = meta["extra"]
    images = [(int(i["offset"], 16), i["path"]) for i in extra["flash_images"]]
    app_offset = int(extra.get("application_offset", "0x10000"), 16)
    build_dir = os.path.dirname(meta["prog_path"])
    return images, app_offset, os.path.join(build_dir, "firmware.bin")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default=None)
    ap.add_argument("--out", default="out")
    ap.add_argument("--web", default=None, help="Starlight public dir, e.g. docs/public")
    args = ap.parse_args()
    version = args.version or git_version()

    images, app_offset, app = flash_layout()
    if not os.path.exists(app):
        sys.exit(f"{app} not found; run `pio run` first")
    out_dir = os.path.join(ROOT, args.out)
    os.makedirs(out_dir, exist_ok=True)
    full = os.path.join(out_dir, f"{NAME}-{version}-full.bin")
    app_out = os.path.join(out_dir, f"{NAME}-{version}-app.bin")

    parts = sorted(images + [(app_offset, app)])
    cmd = esptool_cmd() + ["--chip", "esp32", "merge-bin", "-o", full]
    for off, path in parts:
        cmd += [hex(off), path]
    run(cmd)
    shutil.copyfile(app, app_out)
    print("wrote", full)
    print("wrote", app_out)

    if args.web:
        fw_root = os.path.join(ROOT, args.web, "firmware")
        shutil.rmtree(fw_root, ignore_errors=True)   # generated; keep only this version
        ver_dir = os.path.join(fw_root, version)
        os.makedirs(ver_dir, exist_ok=True)
        shutil.copyfile(full, os.path.join(ver_dir, os.path.basename(full)))
        manifest = {
            "name": "m5-grayline",
            "version": version,
            "new_install_prompt_erase": True,
            "builds": [{
                "chipFamily": "ESP32",
                "parts": [{"path": f"{version}/{os.path.basename(full)}", "offset": 0}],
            }],
        }
        with open(os.path.join(fw_root, "manifest.json"), "w") as f:
            json.dump(manifest, f, indent=2)
            f.write("\n")
        print("wrote", os.path.join(fw_root, "manifest.json"))


if __name__ == "__main__":
    main()
