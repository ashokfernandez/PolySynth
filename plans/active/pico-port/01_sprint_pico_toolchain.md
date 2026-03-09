# Sprint Pico-1: RP2350 Toolchain & Basic Hello World

**Depends on:** Nothing (greenfield Pico platform)
**Blocks:** Sprints Pico-2, Pico-3, Pico-4
**Approach:** Incremental build-up — each task adds one working layer, verified before moving on
**Estimated effort:** 1–2 days

---

## Goal

Set up Pico SDK cross-compilation for RP2350, create CMake infrastructure that coexists with the desktop build, and flash a blink + serial hello world. This sprint proves the toolchain works end-to-end before any DSP code enters the picture.

---

## Prerequisites

Before starting, ensure:

1. **Pico SDK v2.1.0+** is installed locally (v1.x only supports RP2040 — will NOT work)
2. **ARM GCC 12+** is installed (`arm-none-eabi-gcc --version` should show 12.x or later)
3. **CMake 3.15+** and **Ninja** (or Make) are available
4. `PICO_SDK_PATH` environment variable points to your Pico SDK install

```bash
# Verify prerequisites
arm-none-eabi-gcc --version    # Must be 12.x+ for Cortex-M33 C++17 support
cmake --version                # Must be 3.15+
echo $PICO_SDK_PATH            # Must point to Pico SDK v2.1.0+
ls $PICO_SDK_PATH/src/rp2350   # Must exist (proves v2.1.0+, not v1.x)
```

**If `PICO_SDK_PATH` is not set or SDK is not installed, stop here and set it up first.** Each developer points to their own SDK install — we do NOT bundle the SDK in the repo.

---

## Task 1.1: Create `pico_sdk_import.cmake` (Standard SDK Bootstrap)

### What

This is the standard Pico SDK bootstrap file that every Pico project needs. It locates the SDK via `PICO_SDK_PATH` and sets up the CMake toolchain for cross-compilation. This file is provided by the Pico SDK itself — we copy it verbatim.

### File to Create

**`/src/platform/pico/pico_sdk_import.cmake`**

Copy this file from `$PICO_SDK_PATH/external/pico_sdk_import.cmake`. This is the canonical way to bootstrap the Pico SDK in out-of-tree projects. Do NOT hand-write this file — always copy from the SDK.

```bash
cp $PICO_SDK_PATH/external/pico_sdk_import.cmake src/platform/pico/pico_sdk_import.cmake
```

### Why a Copy Instead of a Reference?

The Pico SDK docs recommend copying this file into your project so that the CMake configuration works even if `PICO_SDK_PATH` is not set (the file will search common locations). This is standard practice for all Pico projects.

### Verification

```bash
# File should contain pico_sdk_init() and reference PICO_SDK_PATH
head -20 src/platform/pico/pico_sdk_import.cmake
```

### Acceptance Criteria

- [ ] File exists at `/src/platform/pico/pico_sdk_import.cmake`
- [ ] File is the unmodified copy from the Pico SDK
- [ ] File contains `pico_sdk_init` function

---

## Task 1.2: Create Pico CMakeLists.txt (Top-Level Build for Pico Target)

### What

Create the CMake build file for the Pico target. This is the top-level entry point for building the Pico firmware. It:
- Sets the board to `pico2_w` (RP2350 with CYW43 WiFi chip)
- Sets the platform to `rp2350-arm-s` (secure ARM mode)
- Includes SEA_DSP and SEA_Util as subdirectories (same pattern as `/tests/CMakeLists.txt`)
- Adds the correct compile definitions for embedded mode

### File to Create

**`/src/platform/pico/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.15)

# ── Pico SDK bootstrap (must come before project()) ──────────────────────
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
include(pico_sdk_import.cmake)

project(polysynth_pico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Initialize the Pico SDK (must come after project())
pico_sdk_init()

# ── Embedded compile definitions ─────────────────────────────────────────
# These propagate to ALL targets in this build tree via SEA_DSP INTERFACE.
# POLYSYNTH_USE_FLOAT:       sample_t = float (not double)
# SEA_PLATFORM_EMBEDDED:     sea::Real = float, enables embedded codepaths
# POLYSYNTH_MAX_VOICES=4:    4 voices to fit in 520KB SRAM
set(SEA_PLATFORM_EMBEDDED ON CACHE BOOL "Embedded platform" FORCE)

# ── SEA_DSP library ──────────────────────────────────────────────────────
# Same pattern as /tests/CMakeLists.txt
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_DSP"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_DSP")

# ── SEA_Util library ─────────────────────────────────────────────────────
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_Util"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_Util")

# ── Firmware executable ──────────────────────────────────────────────────
add_executable(polysynth_pico
    main.cpp
)

target_compile_definitions(polysynth_pico PRIVATE
    POLYSYNTH_USE_FLOAT
    POLYSYNTH_MAX_VOICES=4
    PICO_AUDIO_SAMPLE_RATE=48000
    PICO_AUDIO_BUFFER_FRAMES=256
)

# Catch float-to-double promotions in hot paths (these cost ~10x on Cortex-M33)
target_compile_options(polysynth_pico PRIVATE
    -Wall -Wextra
    -Wdouble-promotion
    -fno-exceptions
    -fno-rtti
)

target_include_directories(polysynth_pico PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/core"
)

target_link_libraries(polysynth_pico PRIVATE
    pico_stdlib
    pico_cyw43_arch_none    # CYW43 for onboard LED (no WiFi stack)
    SEA_DSP
    SEA_Util
)

# USB serial output (not UART) — Pico 2 W connects via USB
pico_enable_stdio_usb(polysynth_pico 1)
pico_enable_stdio_uart(polysynth_pico 0)

# Generate .uf2 for drag-and-drop flashing
pico_add_extra_outputs(polysynth_pico)
```

### Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| `PICO_BOARD=pico2_w` | Pico 2 W has CYW43 chip — onboard LED requires WiFi driver init |
| `PICO_PLATFORM=rp2350-arm-s` | Secure ARM mode (not RISC-V) for Cortex-M33 with FPU |
| `-fno-exceptions -fno-rtti` | Saves ~20-40KB flash, required for RT-safe DSP code |
| `-Wdouble-promotion` | Catches accidental float→double promotion (10x penalty on M33) |
| USB stdio, not UART | Pico 2 W connects via USB — simpler for developers |
| `pico_cyw43_arch_none` | Minimal CYW43 init for LED — no WiFi/BT stack overhead |

### Review Guardrails

- The relative paths to `libs/SEA_DSP` and `libs/SEA_Util` use `../../../` because this CMakeLists.txt lives at `src/platform/pico/`. Verify these paths resolve correctly from that location.
- `SEA_PLATFORM_EMBEDDED` is set as a cache variable, which propagates to the SEA_DSP subdirectory and triggers `sea::Real = float` in `sea_platform.h`.
- Do NOT add `hardware_i2s` or `hardware_dma` here — those come in Sprint 2.

### Verification

```bash
# Configure (should succeed without errors)
cmake -S src/platform/pico -B build/pico -DPICO_SDK_PATH=$PICO_SDK_PATH

# Check that SEA_PLATFORM_EMBEDDED is set
grep -r "SEA_PLATFORM_EMBEDDED" build/pico/CMakeCache.txt
```

### Acceptance Criteria

- [ ] `cmake -S src/platform/pico -B build/pico` succeeds with zero errors
- [ ] CMakeCache.txt shows `SEA_PLATFORM_EMBEDDED:BOOL=ON`
- [ ] SEA_DSP and SEA_Util are found and configured
- [ ] Target `polysynth_pico` is created
- [ ] Compiler is `arm-none-eabi-gcc` (not host gcc)

---

## Task 1.3: Create `main.cpp` — LED Blink + Serial Hello World

### What

Create the entry point for the Pico firmware. This is a minimal "hello world" that:
1. Initializes the CYW43 WiFi chip (required for onboard LED on Pico 2 W)
2. Prints version info and SRAM usage over USB serial
3. Blinks the onboard LED in a loop

### File to Create

**`/src/platform/pico/main.cpp`**

```cpp
// PolySynth Pico — Sprint 1: Hello World
// Validates toolchain, USB serial, and onboard LED on Pico 2 W (RP2350).

#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Linker-provided symbols for SRAM usage reporting
extern "C" {
    extern uint8_t __StackLimit;
    extern uint8_t __bss_end__;
    extern uint8_t __end__;
}

static void print_sram_report()
{
    // Static allocation: data + bss sections
    const uint32_t static_used = reinterpret_cast<uint32_t>(&__bss_end__);
    const uint32_t stack_limit = reinterpret_cast<uint32_t>(&__StackLimit);
    const uint32_t total_sram = 520 * 1024; // RP2350: 520KB

    printf("=== SRAM Report ===\n");
    printf("  Static (data+bss):  %lu bytes\n",
           static_cast<unsigned long>(static_used));
    printf("  Stack limit addr:   0x%08lx\n",
           static_cast<unsigned long>(stack_limit));
    printf("  Total SRAM:         %lu bytes (%lu KB)\n",
           static_cast<unsigned long>(total_sram),
           static_cast<unsigned long>(total_sram / 1024));
    printf("===================\n");
}

int main()
{
    // Initialize USB serial (stdio)
    stdio_init_all();

    // Initialize CYW43 — required for onboard LED on Pico 2 W.
    // pico_cyw43_arch_none: minimal init, no WiFi/BT stack.
    if (cyw43_arch_init()) {
        printf("ERROR: CYW43 init failed\n");
        return 1;
    }

    // Wait for USB serial to connect (with timeout for headless operation)
    for (int i = 0; i < 30; i++) {
        if (stdio_usb_connected()) break;
        sleep_ms(100);
    }

    printf("\n");
    printf("========================================\n");
    printf("  PolySynth Pico v0.1\n");
    printf("  Board:    Pico 2 W (RP2350)\n");
    printf("  Voices:   %d\n", POLYSYNTH_MAX_VOICES);
    printf("  Precision: float (single)\n");
    printf("========================================\n");
    printf("\n");

    print_sram_report();

    printf("\nBlink loop started — LED toggles every 500ms\n");

    // Main loop: blink LED to prove firmware is running
    bool led_on = false;
    while (true) {
        led_on = !led_on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        sleep_ms(500);
    }

    return 0;
}
```

### Implementation Notes for Juniors

**Why `cyw43_arch_init()` for an LED?**
The Pico 2 W's onboard LED is connected through the CYW43 WiFi chip, not a standard GPIO pin. This means you must initialize the CYW43 driver even if you don't use WiFi. The `pico_cyw43_arch_none` library provides a minimal driver without the full network stack. If you use a regular Pico 2 (non-W), you can use `gpio_put()` instead.

**Why the USB serial wait loop?**
When the Pico first boots, the USB CDC (serial) interface takes a moment to enumerate on the host. Without this wait, the first few `printf()` calls are lost. The 3-second timeout (30 × 100ms) ensures we don't block forever in headless operation.

**SRAM report linker symbols:**
The ARM linker places special symbols at section boundaries. `__bss_end__` marks the end of the BSS (zero-initialized data) section, which tells us how much static memory is allocated. In Sprint 3, we'll use this to verify the DSP engine's memory footprint.

### Verification

```bash
# Build
cmake --build build/pico

# Check for .uf2 output
ls -la build/pico/polysynth_pico.uf2

# Flash: hold BOOTSEL, connect USB, release BOOTSEL
# Then:
cp build/pico/polysynth_pico.uf2 /media/$USER/RP2350/

# Monitor serial output:
minicom -D /dev/ttyACM0 -b 115200
```

### Expected Serial Output

```
========================================
  PolySynth Pico v0.1
  Board:    Pico 2 W (RP2350)
  Voices:   4
  Precision: float (single)
========================================

=== SRAM Report ===
  Static (data+bss):  XXXXX bytes
  Stack limit addr:   0xXXXXXXXX
  Total SRAM:         532480 bytes (520 KB)
===================

Blink loop started — LED toggles every 500ms
```

### Acceptance Criteria

- [ ] Compiles with zero errors and zero warnings (with `-Wall -Wextra -Wdouble-promotion`)
- [ ] Produces `polysynth_pico.uf2` file
- [ ] LED blinks on Pico 2 W after flashing
- [ ] USB serial shows "PolySynth Pico v0.1"
- [ ] USB serial shows SRAM usage report
- [ ] `POLYSYNTH_MAX_VOICES` prints as 4

---

## Task 1.4: Add Justfile Targets

### What

Add `pico-build` and `pico-flash` targets to the Justfile so developers can build with one command. These follow the existing Justfile conventions (shell commands, no external scripts needed for simple operations).

### File to Modify

**`/Justfile`** — Add after the existing build targets (after the `build-make` target, around line 63):

```just
# ── Pico RP2350 targets ──────────────────────────────────────────────────

# Configure and build Pico firmware (.uf2).
# Requires PICO_SDK_PATH environment variable pointing to Pico SDK v2.1.0+.
pico-build:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ -z "${PICO_SDK_PATH:-}" ]; then
        echo "ERROR: PICO_SDK_PATH not set. Point it to your Pico SDK v2.1.0+ install."
        exit 1
    fi
    cmake -S src/platform/pico -B build/pico -DPICO_SDK_PATH="$PICO_SDK_PATH"
    cmake --build build/pico --parallel

# Flash Pico firmware via picotool (device must be in BOOTSEL mode).
pico-flash: pico-build
    picotool load build/pico/polysynth_pico.uf2 -f
    picotool reboot

# Remove Pico build directory.
pico-clean:
    rm -rf build/pico
```

### Implementation Notes for Juniors

**Why check `PICO_SDK_PATH`?**
Unlike the desktop build which uses system-installed tools, the Pico build requires the SDK at a developer-specified path. A clear error message saves 15 minutes of debugging cryptic CMake failures.

**Why `picotool` for flashing?**
`picotool` is the official Pico flashing utility. It's faster than drag-and-drop and can be automated. If `picotool` is not installed, developers can still flash manually by copying the `.uf2` file to the Pico's mass storage device.

**Manual flash alternative:**
```bash
# Hold BOOTSEL button while plugging in Pico
cp build/pico/polysynth_pico.uf2 /media/$USER/RP2350/
```

### Verification

```bash
# Should show the new targets
just --list | grep pico

# Should build (or fail with clear PICO_SDK_PATH error if not set)
just pico-build
```

### Acceptance Criteria

- [ ] `just --list` shows `pico-build`, `pico-flash`, `pico-clean`
- [ ] `just pico-build` produces `build/pico/polysynth_pico.uf2`
- [ ] `just pico-build` fails with clear error if `PICO_SDK_PATH` is not set
- [ ] `just pico-clean` removes `build/pico/`
- [ ] Existing `just build` and `just test` are unaffected

---

## Task 1.5: Add CI Pico Compile Check

### What

Add a GitHub Actions job that compiles the Pico firmware on every PR. This is a compile-only check — no flashing, no hardware. It proves the Pico build doesn't break when desktop code changes.

### File to Modify

**`/.github/workflows/ci.yml`** — Add a new job after the existing lint job:

```yaml
  # ---------------------------------------------------------------------------
  # Job: Pico RP2350 Compile Check
  # Ensures Pico firmware compiles after every change — no hardware needed.
  # ---------------------------------------------------------------------------
  pico-compile-check:
    name: Pico / Compile Check
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Install ARM toolchain
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi

      - name: Clone Pico SDK v2.1.0
        run: |
          git clone --depth 1 --branch 2.1.0 https://github.com/raspberrypi/pico-sdk.git /tmp/pico-sdk
          cd /tmp/pico-sdk && git submodule update --init --depth 1

      - name: Build Pico firmware
        env:
          PICO_SDK_PATH: /tmp/pico-sdk
        run: |
          cmake -S src/platform/pico -B build/pico \
            -DPICO_SDK_PATH=$PICO_SDK_PATH \
            -G Ninja
          cmake --build build/pico

      - name: Verify .uf2 output
        run: |
          test -f build/pico/polysynth_pico.uf2
          echo "✓ polysynth_pico.uf2 generated successfully"
          ls -lh build/pico/polysynth_pico.uf2
```

### Design Decision: Independent Job

This job runs in parallel with lint and tests — it does NOT block the native test gate. Rationale: a Pico compile failure should not block desktop development, and vice versa. The jobs are independent.

### Review Guardrails

- The Pico SDK version is pinned to `2.1.0` — update this when we move to a newer SDK
- `gcc-arm-none-eabi` from Ubuntu's apt is typically GCC 12.x — sufficient for C++17 on Cortex-M33
- The `git submodule update --init --depth 1` is required for the SDK's TinyUSB dependency

### Acceptance Criteria

- [ ] CI job passes on PR (compile-only, no flash)
- [ ] Job runs in parallel with existing lint/test jobs
- [ ] `.uf2` file is verified to exist
- [ ] Job completes in < 5 minutes

---

## Task 1.6: Desktop Regression Check

### What

Verify that the new Pico files and Justfile changes don't break the existing desktop build. This is not a new task — it's a verification step.

### Verification

```bash
# Desktop build + test must still pass
just build
just test

# Lint must still pass (new files should not introduce warnings in desktop build)
just lint
```

### Acceptance Criteria

- [ ] `just build` succeeds
- [ ] `just test` passes with zero failures
- [ ] `just lint` passes (no new warnings from Pico files — they're not in the desktop build)

---

## Task Execution Order

Execute tasks in this order — each depends on the previous:

1. **T1.1** — Copy `pico_sdk_import.cmake` from SDK
2. **T1.2** — Create `CMakeLists.txt` — verify `cmake -S ... -B ...` configures successfully
3. **T1.3** — Create `main.cpp` — verify `cmake --build build/pico` produces `.uf2`
4. **T1.4** — Add Justfile targets — verify `just pico-build` works
5. **T1.5** — Add CI job — verify via local `act` or push to PR
6. **T1.6** — Desktop regression — verify `just build && just test`

### TDD Note

This sprint is infrastructure-focused — there are no unit tests to write (the "tests" are: does it compile? does the LED blink? does serial output appear?). Sprint 2 introduces the first hardware-testable behavior (audio output), and Sprint 3 introduces the first unit-testable DSP integration.

---

## Definition of Done

- [ ] `just pico-build` produces a valid `polysynth_pico.uf2`
- [ ] LED blinks on Pico 2 W after flashing
- [ ] USB serial shows "PolySynth Pico v0.1" and SRAM report
- [ ] `just build && just test` (desktop) still passes — zero regressions
- [ ] SEA_DSP compiles with `SEA_PLATFORM_EMBEDDED=ON` for ARM target
- [ ] CI pico-compile-check job passes
- [ ] No new files outside `/src/platform/pico/` except Justfile and CI modifications

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Create | `/src/platform/pico/pico_sdk_import.cmake` | Standard SDK bootstrap (copy from SDK) |
| Create | `/src/platform/pico/CMakeLists.txt` | Top-level Pico build |
| Create | `/src/platform/pico/main.cpp` | LED blink + serial hello world |
| Modify | `/Justfile` | Add `pico-build`, `pico-flash`, `pico-clean` |
| Modify | `/.github/workflows/ci.yml` | Add `pico-compile-check` job |

---

## Troubleshooting Guide

### "CMake Error: PICO_PLATFORM rp2350 not supported"
→ You're using Pico SDK v1.x. Install v2.1.0+ and update `PICO_SDK_PATH`.

### "No rule to make target 'pico_cyw43_arch_none'"
→ The SDK's CYW43 driver submodule is not initialized. Run `cd $PICO_SDK_PATH && git submodule update --init`.

### "arm-none-eabi-gcc: command not found"
→ Install the ARM toolchain: `sudo apt install gcc-arm-none-eabi` (Linux) or `brew install arm-none-eabi-gcc` (macOS).

### USB serial shows nothing
→ Wait 3 seconds after boot for USB enumeration. Check with `ls /dev/ttyACM*`. Try `minicom -D /dev/ttyACM0 -b 115200`.

### LED doesn't blink
→ If using Pico 2 (non-W), the LED is on GPIO25, not CYW43. Check your board variant.

---

## Review Checklist for Reviewers

- [ ] `pico_sdk_import.cmake` is an unmodified copy from the SDK (diff against `$PICO_SDK_PATH/external/pico_sdk_import.cmake`)
- [ ] Relative paths in CMakeLists.txt correctly resolve from `src/platform/pico/`
- [ ] No iPlug2 headers or dependencies in any Pico file
- [ ] `-Wdouble-promotion` flag is present (critical for FPU performance)
- [ ] `-fno-exceptions -fno-rtti` flags are present (saves flash, prevents RT-unsafe code)
- [ ] USB serial enabled, UART disabled (correct for Pico 2 W)
- [ ] CYW43 init present (required for onboard LED on Pico 2 W)
- [ ] Desktop build and tests are unaffected (`just build && just test`)
- [ ] CI job pins SDK version (not `main` branch)
