set shell := ["bash", "-eu", "-o", "pipefail", "-c"]
set positional-arguments := true

# Show command help by default.
default:
    @just --list

# Show all available commands and their descriptions.
help:
    @just --list

# Core developer workflow (delegates to scripts/dev.sh)
# Download/refresh external dependencies (iPlug2, DaisySP, SDK assets).
deps *args:
    bash ./scripts/cli.sh run deps -- ./scripts/dev.sh deps {{args}}

# Configure and build C++ test targets (no sanitizers).
build *args:
    bash ./scripts/cli.sh run build -- ./scripts/dev.sh build {{args}}

# Force Ninja generator for test/sanitizer tasks.
build-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run build-ninja -- ./scripts/dev.sh build {{args}}

# Run unit tests. Pass-through example: `just test -- --filter "[engine]"`.
test *args:
    bash ./scripts/cli.sh run test -- ./scripts/dev.sh test {{args}}

# Force Ninja generator for unit tests.
test-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run test-ninja -- ./scripts/dev.sh test {{args}}

# Run static analysis (Cppcheck/Clang-Tidy based on local availability).
lint *args:
    bash ./scripts/cli.sh run lint -- ./scripts/dev.sh lint {{args}}

# Force Ninja generator for static analysis configure/build graph.
lint-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run lint-ninja -- ./scripts/dev.sh lint {{args}}

# Build and run tests under AddressSanitizer + UBSan.
asan *args:
    bash ./scripts/cli.sh run asan -- ./scripts/dev.sh asan {{args}}

# Force Ninja generator for ASan pipeline.
asan-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run asan-ninja -- ./scripts/dev.sh asan {{args}}

# Build and run tests under ThreadSanitizer.
tsan *args:
    bash ./scripts/cli.sh run tsan -- ./scripts/dev.sh tsan {{args}}

# Force Ninja generator for TSan pipeline.
tsan-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run tsan-ninja -- ./scripts/dev.sh tsan {{args}}

# Remove test build directories managed by scripts/dev.sh.
clean *args:
    bash ./scripts/cli.sh run clean -- ./scripts/dev.sh clean {{args}}

# Force Unix Makefiles generator (compat/debug fallback).
build-make *args:
    POLYSYNTH_CMAKE_GENERATOR="Unix Makefiles" bash ./scripts/cli.sh run build-make -- ./scripts/dev.sh build {{args}}

# Quick path: build + unit tests (fast local confidence check).
quick:
    just build
    just test

# Quick path: run the full local PR gate.
quick-pr:
    just ci-pr

# Quick path: rebuild and launch desktop app.
quick-desktop:
    just desktop-rebuild

# Quick path: build and open native sandbox.
quick-ui:
    just sandbox-build
    just sandbox-run

# Quick path: inspect latest failure logs.
quick-logs:
    just logs-latest

# Quick local gate: lint + unit tests.
check:
    just lint
    just test

# Run both sanitizer pipelines locally.
check-sanitizers:
    just asan
    just tsan

# Approximate PR gate: lint + ASan + TSan + unit tests + desktop startup smoke + VRT.
ci-pr:
    just lint
    just asan
    just tsan
    just test
    just desktop-smoke
    just sandbox-build
    just vrt-run

# Desktop app workflow
# Build the standalone desktop app (default config: Debug).
desktop-build config="Debug":
    bash ./scripts/cli.sh run desktop-build -- ./scripts/tasks/build_desktop.sh {{config}}

# Build and run a startup smoke test for the standalone desktop app.
desktop-smoke config="Debug":
    bash ./scripts/cli.sh run desktop-build -- ./scripts/tasks/build_desktop.sh {{config}}
    bash ./scripts/cli.sh run desktop-smoke -- bash ./scripts/tasks/test_desktop_startup.sh {{config}}

# Launch latest built desktop app bundle.
desktop-run:
    bash ./scripts/cli.sh run desktop-run -- ./scripts/tasks/run_desktop.sh

# Rebuild then launch desktop app.
desktop-rebuild config="Debug":
    bash ./scripts/cli.sh run desktop-build -- ./scripts/tasks/build_desktop.sh {{config}}
    bash ./scripts/cli.sh run desktop-run -- ./scripts/tasks/run_desktop.sh

# Install built AU/VST3 into local macOS plugin folders.
install-local config="Debug":
    bash ./scripts/cli.sh run install-local -- ./scripts/tasks/install_local_macos.sh {{config}}

# Alias for install-local (kept for explicitness).
install-local-macos config="Debug":
    bash ./scripts/cli.sh run install-local-macos -- ./scripts/tasks/install_local_macos.sh {{config}}

# Build unsigned release-ready macOS AU + VST3 bundles into .artifacts/releases/macos.
plugin-release-mac:
    bash ./scripts/cli.sh run plugin-release-mac -- ./scripts/tasks/plugin_release_mac.sh

# Build release Windows VST3 bundle and zip into .artifacts/releases/windows.
plugin-release-win:
    bash ./scripts/cli.sh run plugin-release-win -- ./scripts/tasks/plugin_release_windows.sh

# Create unsigned macOS installer pkg from staged release bundles.
plugin-package-mac:
    bash ./scripts/cli.sh run plugin-package-mac -- ./scripts/tasks/create_pkg.sh

# UI Sandbox workflow
# Build the native PolySynthGallery standalone app.
sandbox-build config="Debug":
    bash ./scripts/cli.sh run sandbox-build -- ./scripts/tasks/build_sandbox.sh {{config}}

# Launch the native PolySynthGallery sandbox for interactive development.
sandbox-run *args:
    bash ./scripts/cli.sh run sandbox-run -- ./scripts/tasks/run_sandbox.sh {{args}}

# PolySynth web demo workflow
# Build and serve web demo locally (opens browser when possible).
web-demo:
    bash ./scripts/cli.sh run web-demo -- ./scripts/tasks/run_web.sh

# Build WAM demo bundle only (no local server).
wam-demo-build:
    bash ./scripts/cli.sh run wam-demo-build -- bash -lc 'cd src/platform/desktop && chmod +x scripts/makedist-web.sh && ./scripts/makedist-web.sh off'

# Visual regression testing workflow

# Run gallery visual regression test pipeline.
# UPDATED: now builds sandbox and runs VRT.
gallery-test:
    just sandbox-build
    just vrt-run

# Run gallery visual regression test pipeline (baseline mode).
vrt-baseline:
    bash ./scripts/cli.sh run vrt-baseline -- ./scripts/tasks/run_vrt.sh --generate

# Run gallery visual regression test pipeline (run mode).
vrt-run:
    bash ./scripts/cli.sh run vrt-run -- ./scripts/tasks/run_vrt.sh --verify

# Show latest run summary and log file locations.
logs-latest:
    bash ./scripts/cli.sh latest

# Interactive version manager: shows current version, history, and prompts for a bump.
version:
    python3 scripts/version.py

# Show current version and recent release history (non-interactive).
version-show:
    python3 scripts/version.py --show

# Bump and release a specific part non-interactively: just version-bump patch|minor|major
version-bump part:
    python3 scripts/version.py --bump {{part}}

# Set an explicit version non-interactively: just version-set 1.2.3
version-set version:
    python3 scripts/version.py --set {{version}}

# Set project version across VERSION + desktop platform metadata (low-level).
set-version version:
    bash ./scripts/cli.sh run set-version -- ./scripts/tasks/set_version.sh {{version}}

# ── Pico RP2350 targets ──────────────────────────────────────────────────

# Download Pico SDK to external/pico-sdk/ (idempotent).
pico-deps:
    bash ./scripts/download_pico_sdk.sh

# Configure and build Pico firmware (.uf2).
# Auto-downloads SDK if not present; discovers ARM toolchain automatically.
pico-build:
    #!/usr/bin/env bash
    set -euo pipefail
    bash ./scripts/download_pico_sdk.sh
    export PICO_SDK_PATH="$(pwd)/external/pico-sdk"
    # Discover ARM toolchain (validates version ≥ 12 for C++17)
    ARM_BIN=$(bash ./scripts/find_arm_toolchain.sh)
    export PATH="$ARM_BIN:$PATH"
    cmake -S src/platform/pico -B build/pico -G Ninja -DPICO_SDK_PATH="$PICO_SDK_PATH"
    cmake --build build/pico --parallel

# Flash Pico firmware via picotool (device must be in BOOTSEL mode).
pico-flash: pico-build
    picotool load build/pico/polysynth_pico.uf2 -f
    picotool reboot

# Remove Pico build directory.
pico-clean:
    rm -rf build/pico build/pico-emu

# Run unit tests with embedded (Pico-equivalent) compile flags.
# Layer 1 of the Pico CI pipeline — catches float and voice-count regressions.
test-embedded *args:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build_embedded \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug
    cmake --build tests/build_embedded --target run_tests_embedded --parallel
    tests/build_embedded/run_tests_embedded {{args}}

# Build Pico firmware for Wokwi emulation (RP2040 proxy).
# Used by CI Layer 2b. Not for flashing to real hardware.
pico-build-emu:
    #!/usr/bin/env bash
    set -euo pipefail
    bash ./scripts/download_pico_sdk.sh
    export PICO_SDK_PATH="$(pwd)/external/pico-sdk"
    ARM_BIN=$(bash ./scripts/find_arm_toolchain.sh)
    export PATH="$ARM_BIN:$PATH"
    cmake -S src/platform/pico -B build/pico-emu \
        -G Ninja \
        -DPICO_SDK_PATH="$PICO_SDK_PATH" \
        -DPICO_BOARD=pico_w
    cmake --build build/pico-emu --parallel

# Run Wokwi emulation locally (requires wokwi-cli and WOKWI_CLI_TOKEN).
pico-emu-test:
    #!/usr/bin/env bash
    set -euo pipefail
    just pico-build-emu
    cd src/platform/pico/wokwi
    wokwi-cli --timeout 30000 \
        --expect-text "[TEST:ALL_PASSED]" \
        --fail-text "[TEST:FAIL]" \
        --fail-text "Hard fault"

# ── Golden master workflows ────────────────────────────────────────────

# Generate desktop golden master reference WAVs (commit tests/golden/).
golden-generate:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build tests/build --parallel
    python3 scripts/golden_master.py --generate
    echo "Desktop golden masters written to tests/golden/desktop-x86_64/"
    echo "Review and commit: git add tests/golden/desktop-x86_64/*.wav"

# Generate embedded golden master reference WAVs (commit tests/golden_embedded/).
golden-generate-embedded:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build tests/build --parallel
    python3 scripts/golden_master.py --generate --embedded
    echo "Embedded golden masters written to tests/golden/embedded-x86_64/"
    echo "Review and commit: git add tests/golden/embedded-x86_64/*.wav"

# Generate ARM golden master reference WAVs (commit tests/golden_arm/).
# Requires: apt install qemu-user-static g++-arm-linux-gnueabihf
golden-generate-arm:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build_arm \
        -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake \
        -DCMAKE_BUILD_TYPE=Release -G Ninja
    cmake --build tests/build_arm --parallel
    QEMU_LD_PREFIX=/usr/arm-linux-gnueabihf \
        python3 scripts/golden_master.py --generate --arm
    echo "ARM golden masters written to tests/golden/embedded-armv7/"
    echo "Review and commit: git add tests/golden/embedded-armv7/*.wav"

# Verify desktop golden masters against committed references.
golden-verify:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build tests/build --parallel
    python3 scripts/golden_master.py --verify

# Verify embedded golden masters against committed references.
golden-verify-embedded:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build tests/build --parallel
    python3 scripts/golden_master.py --verify --embedded

# Verify ARM golden masters against committed references.
golden-verify-arm:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build_arm \
        -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake \
        -DCMAKE_BUILD_TYPE=Release -G Ninja
    cmake --build tests/build_arm --parallel
    QEMU_LD_PREFIX=/usr/arm-linux-gnueabihf \
        python3 scripts/golden_master.py --verify --arm

# Docs workflow
# Validate docs structure (mirrors CI smoke test).
docs-check:
    @echo "Checking docs structure..."
    @test -f docs/index.html && echo "  ok  index.html" || (echo "FAIL: docs/index.html missing" && exit 1)
    @test -f docs/design-system.css && echo "  ok  design-system.css" || (echo "FAIL: docs/design-system.css missing" && exit 1)
    @test -f docs/web-demo/index.html && echo "  ok  web-demo/index.html" || (echo "FAIL: docs/web-demo/index.html missing" && exit 1)
    @grep -q "web-demo/scripts/audioworklet.js" docs/index.html && echo "  ok  audioworklet.js referenced" || (echo "FAIL: audioworklet.js not referenced in index.html" && exit 1)
    @grep -q "web-demo/scripts/PolySynth-web.js" docs/index.html && echo "  ok  PolySynth-web.js referenced" || (echo "FAIL: PolySynth-web.js not referenced in index.html" && exit 1)
    @echo "Docs check passed. Run 'just docs-serve' to preview locally."

# Serve docs locally for manual preview (http://localhost:8080).
docs-serve:
    @echo "Serving docs at http://localhost:8080 — Ctrl+C to stop"
    python3 -m http.server 8080 --directory docs/
