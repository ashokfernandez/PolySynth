#!/usr/bin/env python3
"""
Interactive version manager for PolySynth.

Usage:
  python3 scripts/version.py          # interactive TUI
  python3 scripts/version.py --bump patch   # non-interactive bump
  python3 scripts/version.py --set 1.2.3    # non-interactive set
"""

import os
import re
import subprocess
import sys
from pathlib import Path

# ── ANSI colours (no deps) ────────────────────────────────────────────────────
RESET  = "\033[0m"
BOLD   = "\033[1m"
DIM    = "\033[2m"
CYAN   = "\033[36m"
GREEN  = "\033[32m"
YELLOW = "\033[33m"
RED    = "\033[31m"
BLUE   = "\033[34m"
MAGENTA= "\033[35m"
WHITE  = "\033[97m"

def c(colour: str, text: str) -> str:
    return f"{colour}{text}{RESET}"

def header(text: str) -> str:
    return f"\n{BOLD}{CYAN}{text}{RESET}"

# ── Paths ─────────────────────────────────────────────────────────────────────
ROOT = Path(__file__).parent.parent
VERSION_FILE = ROOT / "VERSION"
SET_VERSION_SCRIPT = ROOT / "scripts" / "tasks" / "set_version.sh"

# ── Version helpers ───────────────────────────────────────────────────────────
def read_current() -> str:
    return VERSION_FILE.read_text().strip()

def parse(v: str) -> tuple[int, int, int]:
    m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", v)
    if not m:
        raise ValueError(f"Not a valid semver: {v!r}")
    return int(m[1]), int(m[2]), int(m[3])

def bump(v: str, part: str) -> str:
    major, minor, patch = parse(v)
    if part == "major":
        return f"{major + 1}.0.0"
    if part == "minor":
        return f"{major}.{minor + 1}.0"
    if part == "patch":
        return f"{major}.{minor}.{patch + 1}"
    raise ValueError(f"Unknown bump type: {part!r}")

# ── Git helpers ───────────────────────────────────────────────────────────────
def git(*args: str, check=True) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if check and result.returncode != 0:
        raise RuntimeError(result.stderr.strip())
    return result.stdout.strip()

def recent_tags(n: int = 8) -> list[str]:
    out = git("tag", "--sort=-v:refname", check=False)
    tags = [t for t in out.splitlines() if re.match(r"v\d+\.\d+\.\d+", t)]
    return tags[:n]

def uncommitted_changes() -> bool:
    return bool(git("status", "--porcelain", check=False))

def tag_exists(tag: str) -> bool:
    return git("tag", "-l", tag, check=False) == tag

def branch_name() -> str:
    return git("rev-parse", "--abbrev-ref", "HEAD", check=False)

# ── Core action ───────────────────────────────────────────────────────────────
def apply_version(new_version: str, tag: bool = True, push: bool = True) -> None:
    """Run set_version.sh, commit, optionally tag and push."""

    # 1. Update all files via existing script
    print(f"\n  {c(CYAN, '→')} Updating version files to {c(BOLD, new_version)}...")
    result = subprocess.run(
        ["bash", str(SET_VERSION_SCRIPT), new_version],
        cwd=ROOT,
        capture_output=False,
    )
    if result.returncode != 0:
        print(c(RED, "  ✗ set_version.sh failed"), file=sys.stderr)
        sys.exit(1)

    # 2. Regenerate plists from .in templates via cmake
    build_desktop = ROOT / "build_desktop"
    if build_desktop.exists():
        print(f"  {c(CYAN, '→')} Regenerating Info.plists via cmake...")
        subprocess.run(
            ["cmake", "-S", "src/platform/desktop", "-B", "build_desktop", "-G", "Xcode"],
            cwd=ROOT,
            capture_output=True,  # suppress verbose cmake output
        )

    # 3. Stage everything that set_version touches
    files_to_stage = [
        "VERSION",
        "src/platform/desktop/config.h",
        "src/platform/desktop/CMakeLists.txt",
        "src/platform/desktop/resources/main.rc",
        "src/platform/desktop/resources/PolySynth-AU-Info.plist",
        "src/platform/desktop/resources/PolySynth-VST3-Info.plist",
        "src/platform/desktop/resources/PolySynth-macOS-Info.plist",
        "src/platform/desktop/resources/PolySynth-AAX-Info.plist",
        "src/platform/desktop/resources/PolySynth-CLAP-Info.plist",
    ]
    existing = [f for f in files_to_stage if (ROOT / f).exists()]

    print(f"  {c(CYAN, '→')} Staging version files...")
    # Use --sparse to handle worktree sparse-checkout setups
    git("add", "--sparse", *existing)

    # 4. Commit
    print(f"  {c(CYAN, '→')} Committing...")
    git("commit", "-m", f"chore: release v{new_version}")

    # 5. Tag
    tag_name = f"v{new_version}"
    if tag:
        if tag_exists(tag_name):
            print(c(YELLOW, f"  ⚠  Tag {tag_name} already exists — skipping tag creation"))
        else:
            print(f"  {c(CYAN, '→')} Creating tag {c(BOLD, tag_name)}...")
            git("tag", tag_name)

    # 6. Push
    if push:
        branch = branch_name()
        print(f"  {c(CYAN, '→')} Pushing {c(BOLD, branch)} + tag to origin...")
        if tag and not tag_exists(tag_name):
            git("push", "origin", branch)
        else:
            git("push", "origin", branch, tag_name)

    print(f"\n  {c(GREEN, '✓')} Released {c(BOLD + GREEN, tag_name)} {'and pushed to origin' if push else ''}")
    print(f"  {c(DIM, 'CI will build macOS + Windows installers and publish to GitHub Releases.')}\n")


# ── Display helpers ───────────────────────────────────────────────────────────
def print_status(current: str, tags: list[str]) -> None:
    major, minor, patch = parse(current)

    print(header(" PolySynth Version Manager"))
    print()

    # Current version box
    print(f"  {c(BOLD, 'Current version')}   {c(BOLD + GREEN, current)}")
    print()

    # Next-version suggestions
    suggestions = [
        ("patch", bump(current, "patch"), "bug fixes"),
        ("minor", bump(current, "minor"), "new features, backwards-compatible"),
        ("major", bump(current, "major"), "breaking changes"),
    ]
    print(f"  {c(BOLD, 'Suggested next versions')}")
    for kind, ver, desc in suggestions:
        label = c(YELLOW, f"{kind:6s}")
        print(f"    {label}  {c(BOLD, ver):30s}  {c(DIM, desc)}")
    print()

    # Recent release history
    if tags:
        print(f"  {c(BOLD, 'Recent releases')}")
        for i, tag in enumerate(tags):
            marker = c(GREEN, "●") if i == 0 else c(DIM, "○")
            print(f"    {marker}  {tag}")
    else:
        print(f"  {c(DIM, 'No releases yet')}")
    print()


def prompt_new_version(current: str) -> str | None:
    """Ask user for a version string, accepting bump shortcuts."""
    shortcuts = {"patch": bump(current, "patch"),
                 "minor": bump(current, "minor"),
                 "major": bump(current, "major")}

    hint = c(DIM, "  [patch / minor / major / x.y.z / enter to cancel]")
    print(hint)
    try:
        raw = input(f"  {c(CYAN, '→')} New version: ").strip()
    except (KeyboardInterrupt, EOFError):
        return None

    if not raw:
        return None
    if raw in shortcuts:
        return shortcuts[raw]
    try:
        parse(raw)
        return raw
    except ValueError:
        print(c(RED, f"  ✗ Invalid version: {raw!r}"))
        return None


def confirm(question: str) -> bool:
    try:
        ans = input(f"  {c(YELLOW, '?')} {question} {c(DIM, '[y/N]')} ").strip().lower()
        return ans in ("y", "yes")
    except (KeyboardInterrupt, EOFError):
        return False


# ── Interactive flow ──────────────────────────────────────────────────────────
def interactive() -> None:
    current = read_current()
    tags = recent_tags()

    print_status(current, tags)

    new_version = prompt_new_version(current)
    if not new_version:
        print(c(DIM, "\n  Cancelled.\n"))
        return

    if new_version == current:
        print(c(YELLOW, f"\n  ⚠  Already at {current} — nothing to do.\n"))
        return

    print()
    print(f"  {c(BOLD, 'About to release:')}  {c(DIM, current)} → {c(BOLD + GREEN, new_version)}")

    do_tag  = confirm("Create and push git tag?")
    do_push = confirm("Push to origin?") if do_tag else confirm("Push commit to origin?")

    if not confirm(f"Confirm release of {c(BOLD, f'v{new_version}')}?"):
        print(c(DIM, "\n  Cancelled.\n"))
        return

    # Guard: warn on dirty working tree
    if uncommitted_changes():
        print(c(YELLOW, "\n  ⚠  You have uncommitted changes. They will NOT be included in the release commit."))
        if not confirm("Continue anyway?"):
            print(c(DIM, "\n  Cancelled.\n"))
            return

    apply_version(new_version, tag=do_tag, push=do_push)


# ── Non-interactive ───────────────────────────────────────────────────────────
def non_interactive(args: list[str]) -> None:
    if len(args) >= 2 and args[0] == "--bump":
        current = read_current()
        new_version = bump(current, args[1])
    elif len(args) >= 2 and args[0] == "--set":
        parse(args[1])  # validate
        new_version = args[1]
    elif len(args) >= 1 and args[0] == "--show":
        current = read_current()
        tags = recent_tags()
        print_status(current, tags)
        return
    else:
        print(f"Usage: {sys.argv[0]} [--show | --bump patch|minor|major | --set x.y.z]")
        sys.exit(1)

    apply_version(new_version, tag=True, push=True)


# ── Entry point ───────────────────────────────────────────────────────────────
if __name__ == "__main__":
    args = sys.argv[1:]
    if args:
        non_interactive(args)
    else:
        interactive()
