# PlatformIO pre-build script: embeds the firmware version from Git.
#   APP_VERSION        full `git describe`, e.g. v0.1.0 or v0.1.0-3-gabc1234-dirty
#   APP_VERSION_SHORT  last tag, with "+" when the build is not exactly that tag
import subprocess

Import("env")  # noqa: F821  (provided by PlatformIO / SCons)


def git(*args):
    try:
        return subprocess.check_output(["git", *args], stderr=subprocess.DEVNULL, text=True).strip()
    except Exception:
        return ""


full = git("describe", "--tags", "--always", "--dirty") or "unknown"
tag = git("describe", "--tags", "--abbrev=0") or "v0.0.0"
short = tag if full == tag else tag + "+"

env.Append(CPPDEFINES=[  # noqa: F821
    ("APP_VERSION", env.StringifyMacro(full)),  # noqa: F821
    ("APP_VERSION_SHORT", env.StringifyMacro(short)),  # noqa: F821
])
print(f"Firmware version: {full}")
