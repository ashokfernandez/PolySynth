#!/usr/bin/env bash
# find_arm_toolchain.sh — Discover ARM GNU Toolchain for Pico builds.
# Prints the bin/ directory containing arm-none-eabi-gcc on success.
# Exits non-zero with a diagnostic message on failure.
#
# Search order:
#   1. ARM_GNU_TOOLCHAIN_PATH env var (explicit override)
#   2. /Applications/ArmGNUToolchain/*/arm-none-eabi/bin (macOS, newest first)
#   3. /Applications/ARM/bin (legacy macOS location)
#   4. arm-none-eabi-gcc already on PATH (Linux apt-installed)
set -euo pipefail

MIN_MAJOR=12  # C++17 on Cortex-M33 requires GCC 12+

# ── Helpers ──────────────────────────────────────────────────────────────

die() { echo "ERROR: $*" >&2; exit 1; }

# Extract major version number from gcc --version output
gcc_major() {
    local gcc="$1"
    "$gcc" --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 | cut -d. -f1
}

validate() {
    local bin_dir="$1"
    local gcc="$bin_dir/arm-none-eabi-gcc"
    [[ -x "$gcc" ]] || die "arm-none-eabi-gcc not found at $bin_dir"

    local major
    major=$(gcc_major "$gcc")
    [[ -n "$major" ]] || die "Could not determine GCC version from $gcc"
    (( major >= MIN_MAJOR )) || die "GCC $major found at $gcc — version $MIN_MAJOR+ required (C++17 for Cortex-M33)"

    local version_line
    version_line=$("$gcc" --version 2>/dev/null | head -1)
    echo "ARM toolchain: $version_line" >&2
    echo "  Path: $bin_dir" >&2
    echo "$bin_dir"
}

# ── 1. Explicit env var ──────────────────────────────────────────────────
if [[ -n "${ARM_GNU_TOOLCHAIN_PATH:-}" ]]; then
    validate "$ARM_GNU_TOOLCHAIN_PATH"
    exit 0
fi

# ── 2. macOS /Applications/ArmGNUToolchain (newest first) ───────────────
if [[ -d /Applications/ArmGNUToolchain ]]; then
    # Sort version dirs in reverse so newest comes first
    for dir in $(ls -d /Applications/ArmGNUToolchain/*/arm-none-eabi/bin 2>/dev/null | sort -rV); do
        if [[ -x "$dir/arm-none-eabi-gcc" ]]; then
            validate "$dir"
            exit 0
        fi
    done
fi

# ── 3. Legacy macOS location ────────────────────────────────────────────
if [[ -x /Applications/ARM/bin/arm-none-eabi-gcc ]]; then
    validate "/Applications/ARM/bin"
    exit 0
fi

# ── 4. Already on PATH (Linux / Homebrew / apt) ─────────────────────────
if command -v arm-none-eabi-gcc &>/dev/null; then
    local_bin=$(dirname "$(command -v arm-none-eabi-gcc)")
    validate "$local_bin"
    exit 0
fi

# ── Nothing found ────────────────────────────────────────────────────────
die "ARM GNU Toolchain not found.
Install from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
Or set ARM_GNU_TOOLCHAIN_PATH to the bin/ directory containing arm-none-eabi-gcc."
