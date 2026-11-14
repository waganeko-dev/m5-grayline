#!/usr/bin/env python3
"""Logic of check-branch.sh. Reads the hook JSON on stdin; argv[1] = repo root."""
import json, re, shlex, subprocess, sys

root = sys.argv[1]
try:
    pattern = re.compile(open(f"{root}/.githooks/branch-name-pattern").readline().strip())
except OSError:
    sys.exit(0)  # rule file missing: do not block

try:
    cmd = json.load(sys.stdin).get("tool_input", {}).get("command", "")
except ValueError:
    sys.exit(0)

def current_branch():
    r = subprocess.run(["git", "-C", root, "branch", "--show-current"], capture_output=True, text=True)
    return r.stdout.strip()

def is_tag(name):
    return subprocess.run(["git", "-C", root, "show-ref", "--verify", "--quiet", f"refs/tags/{name}"]).returncode == 0

problems = []

def check_new(name):
    name = name.removeprefix("refs/heads/")
    if name == "main":
        problems.append("do not create or reset a branch named main")
    elif not pattern.match(name):
        problems.append(f"branch name '{name}' is not allowed")

def check_push_dest(name, deleting=False):
    if name.startswith("refs/tags/") or (not name.startswith("refs/heads/") and is_tag(name)):
        return
    name = name.removeprefix("refs/heads/")
    if name == "HEAD":
        name = current_branch()
    if name == "main":
        problems.append("do not push to main; open a pull request instead")
    elif not deleting and not pattern.match(name):
        problems.append(f"branch name '{name}' is not allowed")

try:
    lexer = shlex.shlex(cmd, posix=True, punctuation_chars=";&|")
    lexer.whitespace_split = True
    tokens = list(lexer)
except ValueError:
    sys.exit(0)

segments, seg = [], []
for t in tokens:
    if t and set(t) <= set(";&|"):
        segments.append(seg); seg = []
    else:
        seg.append(t)
segments.append(seg)

for seg in segments:
    if not seg or seg[0] != "git":
        continue
    args = seg[1:]
    while args and args[0].startswith("-"):          # git -C <dir>, -c k=v, --no-pager
        opt = args.pop(0)
        if opt in ("-C", "-c") and args:
            args.pop(0)
    if not args:
        continue
    sub, rest = args[0], args[1:]
    if sub == "checkout":
        for i, a in enumerate(rest):
            if a in ("-b", "-B") and i + 1 < len(rest):
                check_new(rest[i + 1])
    elif sub == "switch":
        for i, a in enumerate(rest):
            if a in ("-c", "-C", "--create", "--force-create") and i + 1 < len(rest):
                check_new(rest[i + 1])
            elif a.startswith(("--create=", "--force-create=")):
                check_new(a.split("=", 1)[1])
    elif sub == "branch":
        opts = [a for a in rest if a.startswith("-")]
        pos = [a for a in rest if not a.startswith("-")]
        if any(o in ("-m", "-M", "--move", "-c", "-C", "--copy") for o in opts):
            if len(pos) >= 2:
                check_new(pos[1])
            elif len(pos) == 1:
                check_new(pos[0])
        elif not opts and pos:
            check_new(pos[0])
        elif opts and pos and all(o in ("-f", "--force", "-t", "--track", "--no-track") for o in opts):
            check_new(pos[0])
    elif sub == "push":
        deleting = any(a in ("-d", "--delete") for a in rest)
        if any(a in ("--tags", "--all", "--mirror") for a in rest):
            if "--all" in rest or "--mirror" in rest:
                problems.append("git push --all / --mirror is not allowed; push one branch at a time")
            continue
        pos = [a for a in rest if not a.startswith("-")]
        refspecs = pos[1:]
        if not refspecs:
            if not deleting:
                check_push_dest(current_branch())
            continue
        for spec in refspecs:
            spec = spec.lstrip("+")
            if ":" in spec:
                src, dst = spec.split(":", 1)
                if src == "":
                    check_push_dest(dst, deleting=True)
                else:
                    check_push_dest(dst)
            else:
                check_push_dest(spec, deleting=deleting)

if problems:
    print("Blocked by .claude/hooks/check-branch.sh (see AGENTS.md):", file=sys.stderr)
    for p in dict.fromkeys(problems):
        print(f"  - {p}", file=sys.stderr)
    print("  Allowed branch names: feature/..., fix/..., docs/..., chore/... (lowercase, hyphens).", file=sys.stderr)
    sys.exit(2)
sys.exit(0)
