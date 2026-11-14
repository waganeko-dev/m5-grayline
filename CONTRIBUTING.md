# Contributing

Thank you for your interest in m5-grayline.

- Development rules (branches, pull requests, releases, secrets) are in
  [AGENTS.md](AGENTS.md). They apply to humans and AI agents alike.
- Build, test, serial commands, tools and the manual are described in
  [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) (Japanese).

## Quick start

```bash
git config core.hooksPath .githooks   # enable the pre-push branch check
git switch -c feature/short-description
pio test -e native
pio run
```

Open a pull request against `main`. It is merged with squash after the CI
checks pass.

Bug reports and ideas are welcome as GitHub issues. Please do not include
your Wi-Fi password, ipinfo.io token or exact home location in issues or
logs.
