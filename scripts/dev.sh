#!/usr/bin/env bash
# scripts/dev.sh — PolySynth developer task runner
#
# Usage:
#   ./scripts/dev.sh <command> [options]
#
# Commands:
#   build          Configure + build tests (clean/release mode, no sanitizers)
#   test           Run unit tests (builds first if needed)
#   lint           Run Cppcheck + Clang-Tidy static analysis
#   asan           Build + run tests under AddressSanitizer + UBSan
#   tsan           Build + run tests under ThreadSanitizer
#   clean          Remove all build directories
#   deps           Download external dependencies
#   help           Show this message
#
# Options:
#   -j N           Parallel jobs (default: number of CPU cores)
#   -v             Verbose output
#   --filter EXPR  Catch2 tag/name filter passed to run_tests (e.g. "[smoke]")
#
# Environment:
#   POLYSYNTH_CMAKE_GENERATOR
#                  Force CMake generator. Defaults to Ninja when available.
#
# Examples:
#   ./scripts/dev.sh build
#   ./scripts/dev.sh test --filter "[engine]"
#   ./scripts/dev.sh asan --filter "[voice]"
#   ./scripts/dev.sh lint
#   ./scripts/dev.sh clean

set -euo pipefail

# ---------------------------------------------------------------------------
# Paths (always relative to repo root, regardless of where script is called from)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TESTS_DIR="$ROOT/tests"

# ---------------------------------------------------------------------------
# Colours (disabled when not a TTY, e.g. in CI log files)
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
    RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
    CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; CYAN=''; BOLD=''; RESET=''
fi

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
info()  { echo -e "${CYAN}[dev]${RESET} $*"; }
ok()    { echo -e "${GREEN}[ok]${RESET}  $*"; }
warn()  { echo -e "${YELLOW}[warn]${RESET} $*"; }
die()   { echo -e "${RED}[fail]${RESET} $*" >&2; exit 1; }
hr()    { echo -e "${BOLD}──────────────────────────────────────────────${RESET}"; }

require() {
    local tool="$1"; shift
    if ! command -v "$tool" &>/dev/null; then
        die "'$tool' not found. $*"
    fi
}

# Detect CPU count portably (macOS + Linux)
cpu_count() {
    if command -v nproc &>/dev/null; then nproc
    elif command -v sysctl &>/dev/null; then sysctl -n hw.logicalcpu
    else echo 4; fi
}

asan_options() {
    local opts="halt_on_error=1:print_stacktrace=1"
    if [[ "$(uname -s)" == "Linux" ]]; then
        opts="detect_leaks=1:${opts}"
    fi
    echo "$opts"
}

pick_cmake_generator() {
    if [[ -n "${POLYSYNTH_CMAKE_GENERATOR:-}" ]]; then
        echo "${POLYSYNTH_CMAKE_GENERATOR}"
        return
    fi

    if command -v ninja &>/dev/null; then
        echo "Ninja"
    else
        echo "Unix Makefiles"
    fi
}

generator_slug() {
    case "$1" in
        Ninja) echo "ninja" ;;
        "Unix Makefiles") echo "make" ;;
        *)
            echo "$1" \
                | tr '[:upper:]' '[:lower:]' \
                | sed -E 's/[^a-z0-9]+/_/g; s/^_+//; s/_+$//'
            ;;
    esac
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
COMMAND="${1:-help}"; shift || true
JOBS="$(cpu_count)"
VERBOSE=0
FILTER=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j)       JOBS="$2"; shift 2 ;;
        -j*)      JOBS="${1#-j}"; shift ;;
        -v)       VERBOSE=1; shift ;;
        --filter) FILTER="$2"; shift 2 ;;
        *)        die "Unknown option: $1. Run './scripts/dev.sh help' for usage." ;;
    esac
done

CMAKE_COMMON=(
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)
[[ "$VERBOSE" == "1" ]] && CMAKE_COMMON+=(-DCMAKE_VERBOSE_MAKEFILE=ON)

CMAKE_GENERATOR="$(pick_cmake_generator)"
GENERATOR_SLUG="$(generator_slug "$CMAKE_GENERATOR")"

BUILD_DIR="$TESTS_DIR/build_${GENERATOR_SLUG}"
BUILD_ASAN="$TESTS_DIR/build_asan_${GENERATOR_SLUG}"
BUILD_TSAN="$TESTS_DIR/build_tsan_${GENERATOR_SLUG}"

if command -v ccache &>/dev/null; then
    CMAKE_COMMON+=(
        -DCMAKE_C_COMPILER_LAUNCHER=ccache
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    )
fi

# ---------------------------------------------------------------------------
# deps – download external dependencies
# ---------------------------------------------------------------------------
cmd_deps() {
    hr; info "Downloading external dependencies..."
    require git "Install git."
    require cmake "Install cmake: brew install cmake"
    chmod +x "$ROOT/scripts/download_dependencies.sh"
    "$ROOT/scripts/download_dependencies.sh"

    # In a git worktree the external/ directory is not checked out.
    # If the main repo already has it, symlink the heavy subdirs rather
    # than re-downloading hundreds of MB.
    local main_ext
    main_ext="$(git -C "$ROOT" rev-parse --show-toplevel 2>/dev/null)/external"
    local wt_ext="$ROOT/external"
    mkdir -p "$wt_ext"
    for dep in iPlug2 daisysp; do
        if [[ -d "$main_ext/$dep" && ! -e "$wt_ext/$dep" ]]; then
            ln -sf "$main_ext/$dep" "$wt_ext/$dep"
            info "  Symlinked external/$dep from main repo"
        fi
    done

    ok "Dependencies ready."
}

# ---------------------------------------------------------------------------
# build – configure + build normal test binaries (no sanitizers)
# ---------------------------------------------------------------------------
cmd_build() {
    hr; info "Build: normal (no sanitizers, generator: $CMAKE_GENERATOR)  →  $BUILD_DIR"
    require cmake "Install cmake: brew install cmake"

    cmake -S "$TESTS_DIR" -B "$BUILD_DIR" \
        -G "$CMAKE_GENERATOR" \
        "${CMAKE_COMMON[@]}" \
        -DCMAKE_BUILD_TYPE=Debug

    cmake --build "$BUILD_DIR" --parallel "$JOBS"
    ok "Build complete."
}

# ---------------------------------------------------------------------------
# test – run unit tests (build first if binary is missing)
# ---------------------------------------------------------------------------
cmd_test() {
    hr; info "Running unit tests..."

    if [[ ! -f "$BUILD_DIR/run_tests" ]]; then
        warn "run_tests binary not found — building first."
        cmd_build
    fi

    if [[ -n "$FILTER" ]]; then
        "$BUILD_DIR/run_tests" "$FILTER"
    else
        "$BUILD_DIR/run_tests"
    fi
    ok "All tests passed."
}

# ---------------------------------------------------------------------------
# lint – run Cppcheck + Clang-Tidy via run_analysis.py
# ---------------------------------------------------------------------------
cmd_lint() {
    hr; info "Running static analysis..."
    require cmake  "Install cmake: brew install cmake"
    require python3 "Install python3."

    # Check at least one analysis tool is available; warn about the other.
    local tools_found=0
    if command -v cppcheck &>/dev/null; then
        info "  Cppcheck: $(cppcheck --version)"
        (( tools_found++ )) || true
    else
        warn "cppcheck not found (brew install cppcheck). Skipping Cppcheck."
    fi

    if command -v clang-tidy &>/dev/null; then
        info "  clang-tidy: $(clang-tidy --version | head -1)"
        (( tools_found++ )) || true
    else
        warn "clang-tidy not found. Install via: brew install llvm  then add to PATH."
        warn "On macOS: export PATH=\"\$(brew --prefix llvm)/bin:\$PATH\""
    fi

    [[ "$tools_found" -eq 0 ]] && die "No analysis tools found. Install cppcheck or clang-tidy."

    # Configure to generate compile_commands.json
    info "Configuring CMake to generate compile_commands.json..."
    cmake -S "$TESTS_DIR" -B "$BUILD_DIR" \
        -G "$CMAKE_GENERATOR" \
        "${CMAKE_COMMON[@]}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DPOLYSYNTH_ENABLE_STATIC_ANALYSIS=ON

    local analysis_args=("--build-dir" "$BUILD_DIR")
    [[ "$VERBOSE" == "1" ]] && analysis_args+=("--verbose")

    # Run whichever tools are installed
    if command -v cppcheck &>/dev/null && command -v clang-tidy &>/dev/null; then
        python3 "$ROOT/scripts/run_analysis.py" "${analysis_args[@]}" --tool all
    elif command -v cppcheck &>/dev/null; then
        python3 "$ROOT/scripts/run_analysis.py" "${analysis_args[@]}" --tool cppcheck
    else
        python3 "$ROOT/scripts/run_analysis.py" "${analysis_args[@]}" --tool clang-tidy
    fi

    ok "Static analysis complete."
}

# ---------------------------------------------------------------------------
# asan – build + test under AddressSanitizer + UBSan
# ---------------------------------------------------------------------------
cmd_asan() {
    hr; info "Build + test: ASan + UBSan (generator: $CMAKE_GENERATOR)  →  $BUILD_ASAN"
    require cmake "Install cmake: brew install cmake"

    # Prefer clang for best ASan stack traces; fall back to system cc
    local CC CXX
    if command -v clang &>/dev/null; then CC=clang; CXX=clang++
    else warn "clang not found, using system compiler (stack traces may be less readable)."
         CC="${CC:-cc}"; CXX="${CXX:-c++}"
    fi
    info "  Compiler: $CC / $CXX"

    cmake -S "$TESTS_DIR" -B "$BUILD_ASAN" \
        -G "$CMAKE_GENERATOR" \
        "${CMAKE_COMMON[@]}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DUSE_SANITIZER=Address

    cmake --build "$BUILD_ASAN" --parallel "$JOBS"

    local ASAN_RUNTIME_OPTIONS
    ASAN_RUNTIME_OPTIONS="$(asan_options)"

    hr; info "Running tests under ASan + UBSan..."
    if [[ -n "$FILTER" ]]; then
        ASAN_OPTIONS="$ASAN_RUNTIME_OPTIONS" \
        UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
            "$BUILD_ASAN/run_tests" "$FILTER"
    else
        ASAN_OPTIONS="$ASAN_RUNTIME_OPTIONS" \
        UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
            "$BUILD_ASAN/run_tests"
    fi

    ok "ASan + UBSan clean."
}

# ---------------------------------------------------------------------------
# tsan – build + test under ThreadSanitizer
# ---------------------------------------------------------------------------
cmd_tsan() {
    hr; info "Build + test: TSan (generator: $CMAKE_GENERATOR)  →  $BUILD_TSAN"
    require cmake "Install cmake: brew install cmake"

    local CC CXX
    if command -v clang &>/dev/null; then CC=clang; CXX=clang++
    else CC="${CC:-cc}"; CXX="${CXX:-c++}"; fi
    info "  Compiler: $CC / $CXX"

    cmake -S "$TESTS_DIR" -B "$BUILD_TSAN" \
        -G "$CMAKE_GENERATOR" \
        "${CMAKE_COMMON[@]}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DUSE_SANITIZER=Thread

    cmake --build "$BUILD_TSAN" --parallel "$JOBS"

    hr; info "Running tests under TSan..."
    if [[ -n "$FILTER" ]]; then
        TSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
            "$BUILD_TSAN/run_tests" "$FILTER"
    else
        TSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
            "$BUILD_TSAN/run_tests"
    fi

    ok "TSan clean."
}

# ---------------------------------------------------------------------------
# clean – remove all build artifacts
# ---------------------------------------------------------------------------
cmd_clean() {
    hr; info "Cleaning build directories..."
    for d in \
        "$BUILD_DIR" "$BUILD_ASAN" "$BUILD_TSAN" \
        "$TESTS_DIR/build" "$TESTS_DIR/build_asan" "$TESTS_DIR/build_tsan"
    do
        if [[ -d "$d" ]]; then
            rm -rf "$d"
            info "  Removed: $d"
        fi
    done
    ok "Clean complete."
}

# ---------------------------------------------------------------------------
# help
# ---------------------------------------------------------------------------
cmd_help() {
    sed -n '2,/^set -/p' "${BASH_SOURCE[0]}" | grep -E '^#' | sed 's/^# \?//'
}

# ---------------------------------------------------------------------------
# Dispatch
# ---------------------------------------------------------------------------
case "$COMMAND" in
    build)  cmd_build  ;;
    test)   cmd_test   ;;
    lint)   cmd_lint   ;;
    asan)   cmd_asan   ;;
    tsan)   cmd_tsan   ;;
    clean)  cmd_clean  ;;
    deps)   cmd_deps   ;;
    help|-h|--help) cmd_help ;;
    *) die "Unknown command: '$COMMAND'. Run './scripts/dev.sh help' for usage." ;;
esac
