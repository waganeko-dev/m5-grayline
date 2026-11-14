# AGENTS.md

Rules for AI coding agents (Claude Code, Codex, Copilot, Cursor, ...) working
in this repository. This file is the source of truth; `CLAUDE.md` only imports
it. Humans: see [CONTRIBUTING.md](CONTRIBUTING.md) and
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) (Japanese).

## Project

m5-grayline: firmware for M5Stack Basic V2.6 (ESP32, Arduino framework,
PlatformIO) that draws a grayline map, plus its user manual and Web Flasher
(Astro Starlight in `docs/`).

## Branches and pull requests

- Never commit or push to `main` directly. Every change goes through a pull
  request.
- Branch names must start with one of these prefixes, followed by a short
  lowercase, hyphenated description:
  - `feature/` new functionality
  - `fix/` bug fixes
  - `docs/` documentation and the manual
  - `chore/` tooling, CI, dependencies, release preparation
  - Examples: `feature/moon-overlay`, `fix/polar-night-countdown`,
    `chore/release-v0.3.0`.
  - The exact rule is the regular expression in `.githooks/branch-name-pattern`.
- Pull requests are merged with **squash** only. Do not merge pull requests
  yourself unless a human asks you to.
- The CI checks `Firmware tests and build` and `Manual build` must pass.
- Commit messages: English, imperative mood, a short subject line
  (e.g. "Add Moon overlay to the map"). Conventional Commits are not required.
- Record user-visible changes in `CHANGELOG.md` under `[Unreleased]`
  (Japanese, Keep a Changelog style).

## Tags and releases

- Create or push `v*` tags **only when a human explicitly asks for a release**.
  Pushing a tag starts `.github/workflows/release.yml`, which publishes a
  GitHub Release and deploys the manual to GitHub Pages.
- Release steps:
  1. On a `chore/release-vX.Y.Z` branch, rename `[Unreleased]` to
     `[X.Y.Z] - YYYY-MM-DD` in `CHANGELOG.md` (add a new empty
     `[Unreleased]` and the link at the bottom), open a pull request, and get
     it merged.
  2. On the updated `main`: `git tag -a vX.Y.Z -m "m5-grayline vX.Y.Z"` and
     `git push origin vX.Y.Z`.
- Versioning: Semantic Versioning, `0.x` until the requirements are settled.
  MAJOR for incompatible `config.json` changes or a change of target hardware.

## Build and test

```bash
pio test -e native            # unit tests on the PC
pio run                       # firmware build (env m5stack-core-16m)
cd docs && npm ci && npm run build   # manual
```

Run the relevant commands before opening a pull request. Do not flash a
device or open a serial port unless a human asks you to.

## Secrets and personal data

- Never commit `config.json` (Wi-Fi password, ipinfo.io token, home
  latitude / longitude). Use `config.example.json` with sample values.
- Never commit tokens, keys, serial port names that contain device serial
  numbers, or other personal data.

## Guard rails

Branch rules are enforced in three layers:

1. **Claude Code hook**: `.claude/settings.json` runs
   `.claude/hooks/check-branch.sh` before Bash commands and blocks
   `git checkout -b`, `git switch -c`, `git branch` and `git push` with a
   disallowed branch name or a push to `main`. If it blocks you, rename the
   branch; do not try to get around the hook.
2. **Git hook**: `.githooks/pre-push` rejects the same pushes for any tool.
   Enable it once per clone: `git config core.hooksPath .githooks`.
3. **GitHub rulesets** (the last line of defense): `main` requires a pull
   request with passing checks and squash merge, branch creation is limited to
   the prefixes above, and `v*` tags cannot be moved or deleted.
   Repository admins can bypass the rulesets; that is for emergencies by
   humans only.
