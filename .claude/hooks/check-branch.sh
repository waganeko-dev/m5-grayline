#!/usr/bin/env bash
# Claude Code PreToolUse hook (Bash): blocks git commands that create or push
# a branch whose name breaks the rule in .githooks/branch-name-pattern, and
# pushes to main. Exit code 2 blocks the command and shows the message to Claude.
# Checked: git checkout -b/-B, git switch -c/-C/--create, git branch <new>,
# git branch -m/-c <old> <new>, git push [remote] [refspec...].
set -euo pipefail
root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
exec python3 "$(dirname "$0")/check_branch.py" "$root"
