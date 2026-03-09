# Sprint Pico-2: CI/CD Pipeline & Emulated Testing

**Depends on:** Sprint Pico-1 (toolchain compiles, blink works)
**Blocks:** All subsequent sprints (this is the testing foundation)
**Approach:** Strict TDD — every subsequent sprint's Definition of Done requires CI green
**Estimated effort:** 2–3 days

---

## Goal

Establish a rigorous two-layer automated testing pipeline so that every PR is verified without physical hardware. No code reaches `main` without passing both layers:

1. **Layer 1 — Native Host DSP Tests (x86/ARM64):** Compile and run the core DSP engine with embedded configuration flags (`POLYSYNTH_USE_FLOAT`, `POLYSYNTH_MAX_VOICES=4`, `SEA_PLATFORM_EMBEDDED`) on the host machine. Proves audio math correctness in milliseconds using Catch2.

2. **Layer 2 — Emulated Firmware (Wokwi RP2040 proxy):** Run the actual compiled firmware inside [Wokwi's RP2040 simulator](https://docs.wokwi.com/wokwi-ci/getting-started) via [GitHub Actions](https://docs.wokwi.com/wokwi-ci/github-actions). Verify that firmware boots without hard-fault, serial output is correct, and the audio callback is invoked.

### Critical Emulation Landscape Note

> **Neither Wokwi nor Renode officially supports the RP2350 as of March 2026.**
>
> - **Wokwi:** Full RP2040 support. RP2350 requested in [wokwi/rp2040js#142](https://github.com/wokwi/rp2040js/issues/142) — not yet implemented. A community fork ([c1570/rp2040js](https://github.com/c1570/rp2040js)) adds limited RISC-V-only RP2350 support.
> - **Renode:** Community RP2040 support via [matgla/Renode_RP2040](https://github.com/matgla/Renode_RP2040) (no DMA, no IRQ). No RP2350 support exists.
>
> **Our approach:** Use Wokwi's RP2040 simulation as a **behavioral proxy** for the RP2350. The Pico SDK abstracts most hardware differences — GPIO, PIO, DMA, and USB CDC APIs are source-compatible between RP2040 and RP2350. We compile a separate `pico_w` (RP2040) build specifically for Wokwi emulation, while the primary build targets `pico2_w` (RP2350). When RP2350 emulation becomes available, we swap the board target — no code changes needed.

---

## Prerequisites

- Sprint Pico-1 complete (`just pico-build` produces `.uf2`)
- Wokwi account and API token (free tier available at [wokwi.com/ci](https://wokwi.com/ci))
- Understanding of the existing CI pipeline in `/.github/workflows/ci.yml`

---

## Testing Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CI Pipeline                                  │
│                                                                     │
│  ┌──────────────────────────┐  ┌────────────────────────────────┐  │
│  │  Layer 1: Native Host    │  │  Layer 2a: ARM Cross-Compile   │  │
│  │  (x86, milliseconds)     │  │  (RP2350, compile-only)        │  │
│  │                          │  │                                │  │
│  │  • Catch2 unit tests     │  │  • cmake --build for ARM      │  │
│  │  • Embedded config flags │  │  • -Wdouble-promotion clean   │  │
│  │  • Float precision DSP   │  │  • .uf2 produced              │  │
│  │  • Deploy flag combos    │  │  • .elf map verified           │  │
│  │  • Command parser tests  │  │                                │  │
│  └──────────────────────────┘  └────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Layer 2b: Wokwi Emulated Firmware (RP2040 proxy)           │  │
│  │  (10-30 seconds per scenario)                                │  │
│  │                                                              │  │
│  │  • Firmware boots without hard-fault                         │  │
│  │  • Serial output: version string, SRAM report               │  │
│  │  • I2S PIO initialized (GPIO pin state via logic analyzer)  │  │
│  │  • Serial command protocol: NOTE_ON → OK response           │  │
│  │  • No watchdog timeout / hang detection                      │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Task 2.1: Create Layer 1 — Native Embedded Configuration Test Target

### What

Add a new CMake build configuration that compiles the PolySynth DSP engine on the host machine (x86/ARM64) but with the exact same compile definitions that the Pico build uses. This catches float precision issues, `POLYSYNTH_MAX_VOICES=4` regressions, and deploy-flag incompatibilities without cross-compilation.

### File to Modify

**`/tests/CMakeLists.txt`** — Add a new test target:

```cmake
# ── Embedded-configuration test target ───────────────────────────────────
# Compiles the same DSP code with Pico-equivalent flags on the host machine.
# This is Layer 1 of the Pico CI pipeline — catches float precision and
# voice-count regressions without needing ARM cross-compilation.
add_executable(run_tests_embedded
    ${UNIT_TEST_SOURCES}
    ${CATCH2_DIR}/catch.hpp
)

target_compile_definitions(run_tests_embedded PRIVATE
    POLYSYNTH_USE_FLOAT
    SEA_PLATFORM_EMBEDDED
    POLYSYNTH_MAX_VOICES=4
    # All deploy flags OFF — matches default Pico configuration
    # (Do NOT define POLYSYNTH_DEPLOY_CHORUS, _DELAY, _LIMITER)
)

target_include_directories(run_tests_embedded PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/pico
    ${CATCH2_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(run_tests_embedded PRIVATE SEA_DSP SEA_Util)

target_compile_options(run_tests_embedded PRIVATE
    -Wall -Wextra
    -Wdouble-promotion    # Catch float→double promotion (10x penalty on Cortex-M33)
)

# Apply sanitizers if available
if(COMMAND polysynth_apply_sanitizers)
    polysynth_apply_sanitizers(run_tests_embedded)
endif()
```

### What This Catches That Desktop Tests Don't

| Issue | Desktop Build | Embedded Config Build |
|-------|--------------|----------------------|
| `double` literal in hot path (`2.0` vs `2.0f`) | Silent | `-Wdouble-promotion` warning/error |
| `sample_t = double` vs `float` precision | Tests pass with double | Tests reveal float precision failures |
| 16-voice test assumptions | Pass (16 voices) | Fail (only 4 voices available) |
| `std::atomic<uint64_t>` lock-free | Lock-free on x86 | Compiled out by `SEA_PLATFORM_EMBEDDED` |
| Deploy-flag-guarded code paths | All effects ON | All effects OFF — exercises stripped path |

### TDD: Tests That Must Pass Under Embedded Config

Some existing tests will need `#if` guards or tolerance adjustments for the embedded configuration. Document which tests need modification:

```cpp
// Example: Test that uses kMaxVoices
TEST_CASE("VoiceManager handles max voices", "[VoiceManager]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    // Fill all available voices
    for (int i = 0; i < PolySynthCore::kMaxVoices; i++) {
        vm.OnNoteOn(60 + i, 100);
    }
    REQUIRE(vm.GetActiveVoiceCount() == PolySynthCore::kMaxVoices);

    // Next note triggers stealing
    vm.OnNoteOn(60 + PolySynthCore::kMaxVoices, 100);
    REQUIRE(vm.GetActiveVoiceCount() == PolySynthCore::kMaxVoices);
}
```

Tests that hard-code `16` or `8` for voice count must use `kMaxVoices` instead.

### File to Create

**`/tests/unit/Test_EmbeddedConfig.cpp`**

```cpp
#include "../../src/core/types.h"
#include "../../src/core/Engine.h"
#include "../../src/core/SynthState.h"
#include "catch.hpp"
#include <cmath>
#include <type_traits>

// ── Compile-time configuration verification ─────────────────────────────

#if defined(SEA_PLATFORM_EMBEDDED)

TEST_CASE("Embedded: sample_t is float", "[EmbeddedConfig]") {
    STATIC_REQUIRE(std::is_same_v<PolySynthCore::sample_t, float>);
}

TEST_CASE("Embedded: sea::Real is float", "[EmbeddedConfig]") {
    STATIC_REQUIRE(std::is_same_v<sea::Real, float>);
}

TEST_CASE("Embedded: kMaxVoices is 4", "[EmbeddedConfig]") {
    REQUIRE(PolySynthCore::kMaxVoices == 4);
}

TEST_CASE("Embedded: Engine produces audio in float mode", "[EmbeddedConfig]") {
    PolySynthCore::Engine engine;
    PolySynthCore::SynthState state;
    state.Reset();

    engine.Init(48000.0);
    engine.UpdateState(state);
    engine.OnNoteOn(60, 100);

    // Process one buffer worth of samples
    float left = 0.0f, right = 0.0f;
    float energy = 0.0f;
    for (int i = 0; i < 256; i++) {
        left = 0.0f;
        right = 0.0f;
        engine.Process(left, right);
        energy += left * left + right * right;
    }

    // Should produce non-zero audio
    REQUIRE(energy > 0.0f);

    // Output should be finite (no NaN, no inf)
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
}

TEST_CASE("Embedded: 4-voice polyphony works", "[EmbeddedConfig]") {
    PolySynthCore::Engine engine;
    PolySynthCore::SynthState state;
    state.Reset();

    engine.Init(48000.0);
    engine.UpdateState(state);

    // Play exactly 4 notes (our max)
    engine.OnNoteOn(60, 100);  // C4
    engine.OnNoteOn(64, 90);   // E4
    engine.OnNoteOn(67, 80);   // G4
    engine.OnNoteOn(72, 70);   // C5

    // Process some samples
    float left, right;
    float energy = 0.0f;
    for (int i = 0; i < 256; i++) {
        left = 0.0f;
        right = 0.0f;
        engine.Process(left, right);
        energy += left * left + right * right;
    }

    // All 4 voices contributing — energy should be significant
    REQUIRE(energy > 0.1f);
}

TEST_CASE("Embedded: float precision sufficient for pitch", "[EmbeddedConfig]") {
    // Middle C (261.63 Hz) — verify float precision doesn't ruin pitch
    // At 48kHz, one period = 48000/261.63 ≈ 183.5 samples
    // Float has ~7 decimal digits of precision — more than enough
    float freq = 261.63f;
    float period_samples = 48000.0f / freq;
    REQUIRE(period_samples == Approx(183.5f).margin(0.5f));

    // Phase increment per sample
    float phase_inc = 2.0f * 3.14159265f * freq / 48000.0f;
    REQUIRE(std::isfinite(phase_inc));
    REQUIRE(phase_inc > 0.0f);
    REQUIRE(phase_inc < 1.0f);  // Less than pi (Nyquist)
}

TEST_CASE("Embedded: SynthState Reset works with all field types", "[EmbeddedConfig]") {
    PolySynthCore::SynthState state;
    state.Reset();

    // Verify defaults are sensible
    REQUIRE(state.masterGain == Approx(0.75));
    REQUIRE(state.filterCutoff == Approx(2000.0));
    REQUIRE(state.ampSustain == Approx(1.0));
}

#endif // SEA_PLATFORM_EMBEDDED
```

### Add to `tests/CMakeLists.txt`

Add `unit/Test_EmbeddedConfig.cpp` to `UNIT_TEST_SOURCES`.

### Justfile Target

**`/Justfile`** — Add after the existing test targets:

```just
# Run unit tests with embedded (Pico-equivalent) compile flags.
# Layer 1 of the Pico CI pipeline — catches float and voice-count regressions.
test-embedded *args:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tests -B tests/build_embedded \
        -DCMAKE_BUILD_TYPE=Debug
    cmake --build tests/build_embedded --target run_tests_embedded --parallel
    tests/build_embedded/run_tests_embedded {{args}}
```

### Acceptance Criteria

- [ ] `run_tests_embedded` binary builds with zero warnings (including `-Wdouble-promotion`)
- [ ] All tests tagged `[EmbeddedConfig]` pass
- [ ] Existing tests that use `kMaxVoices` pass with value 4
- [ ] `just test-embedded` works from the repo root
- [ ] `just test` (desktop, 16 voices) still passes — no regressions

---

## Task 2.2: Create Wokwi Emulation Configuration (RP2040 Proxy Build)

### What

Create a separate CMake build target that compiles the firmware for RP2040 (`pico_w`) instead of RP2350 (`pico2_w`). This is used exclusively for Wokwi CI emulation. The DSP code is identical — only the board init and LED GPIO differ slightly.

### Why a Separate Build?

The RP2350 and RP2040 share the Pico SDK's HAL (Hardware Abstraction Layer). The same C++ code compiles for both targets. The differences are:

| Aspect | RP2350 (pico2_w) | RP2040 (pico_w) |
|--------|-------------------|-----------------|
| CPU | Cortex-M33 @ 150MHz | Cortex-M0+ @ 133MHz |
| SRAM | 520KB | 264KB |
| FPU | Hardware float | Software float |
| PIO | PIO v2 (4 blocks) | PIO v1 (2 blocks) |
| LED | CYW43 (same) | CYW43 (same) |
| I2S API | Same SDK API | Same SDK API |
| USB CDC | Same SDK API | Same SDK API |

For emulation testing purposes, the behavioral differences don't matter — we're testing:
- Does the firmware boot?
- Does it print the expected serial output?
- Does the audio callback get invoked?
- Does the serial command protocol work?

### Files to Create

**`/src/platform/pico/wokwi/diagram.json`**

```json
{
  "version": 1,
  "author": "PolySynth",
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-pi-pico-w",
      "id": "pico",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-logic-analyzer",
      "id": "logic1",
      "top": 150,
      "left": 0,
      "attrs": {
        "bufferSize": "1000000"
      }
    }
  ],
  "connections": [
    ["pico:GP26", "logic1:D0", "green", ["v:20"]],
    ["pico:GP27", "logic1:D1", "blue", ["v:30"]],
    ["pico:GP28", "logic1:D2", "purple", ["v:40"]],
    ["pico:GND.1", "logic1:GND", "black", ["v:50"]]
  ]
}
```

**`/src/platform/pico/wokwi/wokwi.toml`**

```toml
[wokwi]
version = 1
firmware = "../../../build/pico-emu/polysynth_pico.uf2"
elf = "../../../build/pico-emu/polysynth_pico.elf"
```

### Implementation Notes for Juniors

**What is `diagram.json`?**
This file describes the virtual hardware setup in Wokwi. We create a Pico W board and connect a [Logic Analyzer](https://docs.wokwi.com/parts/wokwi-logic-analyzer) to the I2S GPIO pins. The logic analyzer captures pin state changes and exports them as VCD (Value Change Dump) files for analysis.

**What is `wokwi.toml`?**
This is the [Wokwi CI configuration file](https://docs.wokwi.com/wokwi-ci/getting-started) that tells the simulator where to find the firmware binary. The `firmware` path points to the `.uf2` file produced by our emulation build.

**Why `board-pi-pico-w` and not `board-pi-pico-2-w`?**
Wokwi does not support the Pico 2 W yet. The Pico W (RP2040) serves as a behavioral proxy — the SDK APIs and GPIO mappings are identical. See the [RP2350 support tracking issue](https://github.com/wokwi/rp2040js/issues/142).

### Acceptance Criteria

- [ ] `diagram.json` is valid (can be loaded in Wokwi web editor for manual testing)
- [ ] Logic analyzer connected to I2S pins (GPIO 26, 27, 28)
- [ ] `wokwi.toml` points to the emulation build output

---

## Task 2.3: Create Emulation Build Target

### What

Add a CMake preset that builds the firmware for RP2040 (Wokwi emulation). This is a thin wrapper around the existing Pico CMakeLists.txt with the board overridden.

### File to Modify

**`/src/platform/pico/CMakeLists.txt`** — Add conditional board selection:

Replace the hardcoded board setting:
```cmake
# BEFORE:
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)

# AFTER:
# Board selection: pico2_w (RP2350, default) or pico_w (RP2040, for Wokwi emulation)
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2_w)
endif()
if(NOT DEFINED PICO_PLATFORM)
    if(PICO_BOARD STREQUAL "pico2_w")
        set(PICO_PLATFORM rp2350-arm-s)
    else()
        # RP2040 — no PICO_PLATFORM override needed (SDK defaults to rp2040)
    endif()
endif()
```

### Justfile Targets

**`/Justfile`** — Add emulation build:

```just
# Build Pico firmware for Wokwi emulation (RP2040 proxy).
# Used by CI Layer 2b. Not for flashing to real hardware.
pico-build-emu:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ -z "${PICO_SDK_PATH:-}" ]; then
        echo "ERROR: PICO_SDK_PATH not set."
        exit 1
    fi
    cmake -S src/platform/pico -B build/pico-emu \
        -DPICO_SDK_PATH="$PICO_SDK_PATH" \
        -DPICO_BOARD=pico_w
    cmake --build build/pico-emu --parallel

# Run Wokwi emulation locally (requires wokwi-cli and WOKWI_CLI_TOKEN).
pico-emu-test:
    #!/usr/bin/env bash
    set -euo pipefail
    just pico-build-emu
    cd src/platform/pico/wokwi
    wokwi-cli --timeout 15000 \
        --expect-text "PolySynth Pico" \
        --fail-text "ERROR:" \
        --fail-text "Hard fault"
```

### Acceptance Criteria

- [ ] `just pico-build-emu` produces `build/pico-emu/polysynth_pico.uf2`
- [ ] Build uses `PICO_BOARD=pico_w` (RP2040)
- [ ] `just pico-build` still uses `pico2_w` (RP2350) — default unchanged
- [ ] Both builds share the same source files

---

## Task 2.4: Create Firmware Test Harness in main.cpp

### What

Add a self-test mode to `main.cpp` that runs automatically on boot and prints structured pass/fail results over serial. This is what Wokwi's `expect_text` checks against.

### Design: Test Protocol

The firmware prints structured test markers that the CI can detect:

```
[TEST:BEGIN]
[TEST:PASS] boot_init
[TEST:PASS] serial_output
[TEST:PASS] sram_report
[TEST:PASS] engine_init
[TEST:PASS] audio_callback_invoked
[TEST:PASS] note_on_produces_audio
[TEST:ALL_PASSED]
```

If any test fails:
```
[TEST:FAIL] audio_callback_invoked: DMA underrun within 1 second
```

Wokwi CI config:
- `expect_text: "[TEST:ALL_PASSED]"` — test passes if this string appears in serial output
- `fail_text: "[TEST:FAIL]"` — test fails immediately if any test fails
- `fail_text: "Hard fault"` — catches hardware faults
- `timeout: 30000` — 30 second timeout catches hangs

### File to Modify

**`/src/platform/pico/main.cpp`** — Add self-test preamble before the main loop:

```cpp
// ── Self-test sequence (runs on every boot) ─────────────────────────────
// Results are printed as structured markers for CI detection.
// Wokwi CI checks for "[TEST:ALL_PASSED]" in serial output.

static void run_self_tests()
{
    int pass_count = 0;
    int fail_count = 0;

    printf("[TEST:BEGIN]\n");

    // Test 1: Boot init completed (if we got here, it passed)
    printf("[TEST:PASS] boot_init\n");
    pass_count++;

    // Test 2: Serial output works (if you can read this, it passed)
    printf("[TEST:PASS] serial_output\n");
    pass_count++;

    // Test 3: SRAM report
    print_sram_report();
    printf("[TEST:PASS] sram_report\n");
    pass_count++;

    // Test 4: Engine init
    s_state.Reset();
    s_engine.Init(static_cast<double>(pico_audio::kSampleRate));
    s_engine.UpdateState(s_state);
    printf("[TEST:PASS] engine_init\n");
    pass_count++;

    // Test 5: Engine produces non-zero audio
    {
        float left = 0.0f, right = 0.0f;
        float energy = 0.0f;
        s_engine.OnNoteOn(60, 100);
        for (int i = 0; i < 256; i++) {
            left = 0.0f;
            right = 0.0f;
            s_engine.Process(left, right);
            energy += left * left + right * right;
        }
        s_engine.OnNoteOff(60);

        if (energy > 0.0001f) {
            printf("[TEST:PASS] engine_produces_audio (energy=%.4f)\n",
                   static_cast<double>(energy));
            pass_count++;
        } else {
            printf("[TEST:FAIL] engine_produces_audio: energy=%.4f (expected > 0.0001)\n",
                   static_cast<double>(energy));
            fail_count++;
        }
    }

    // Test 6: Audio I2S init
    if (pico_audio::Init(audio_callback)) {
        printf("[TEST:PASS] audio_i2s_init\n");
        pass_count++;
    } else {
        printf("[TEST:FAIL] audio_i2s_init\n");
        fail_count++;
    }

    // Summary
    printf("\n[TEST:SUMMARY] %d passed, %d failed\n", pass_count, fail_count);
    if (fail_count == 0) {
        printf("[TEST:ALL_PASSED]\n");
    } else {
        printf("[TEST:SOME_FAILED]\n");
    }
}
```

Call `run_self_tests()` in `main()` after serial initialization but before the main loop.

### Acceptance Criteria

- [ ] Self-test runs on every boot
- [ ] `[TEST:ALL_PASSED]` printed when all tests pass
- [ ] `[TEST:FAIL]` printed with details when any test fails
- [ ] Self-test completes in < 5 seconds (within Wokwi timeout)
- [ ] Normal operation continues after self-test (LED blink or serial REPL)

---

## Task 2.5: Create GitHub Actions CI Jobs

### What

Add two new CI jobs to `/.github/workflows/ci.yml`:

1. **`pico-native-embedded-tests`** — Layer 1: runs `run_tests_embedded` on x86
2. **`pico-emulation-test`** — Layer 2b: runs firmware in Wokwi emulator

### File to Modify

**`/.github/workflows/ci.yml`** — Add after the existing `pico-compile-check` job (from Sprint Pico-1). Replace that simple compile-check with these more comprehensive jobs:

```yaml
  # ---------------------------------------------------------------------------
  # Pico Layer 1: Native Host Tests with Embedded Configuration
  # Runs DSP unit tests with POLYSYNTH_USE_FLOAT, MAX_VOICES=4, effects OFF.
  # Catches float precision, voice-count, and deploy-flag regressions.
  # ---------------------------------------------------------------------------
  pico-native-embedded-tests:
    name: Pico / Layer 1 – Native Embedded Tests
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build ccache

      - name: Restore ccache
        uses: actions/cache@v4
        with:
          path: ~/.cache/ccache
          key: ${{ runner.os }}-ccache-pico-embedded-${{ hashFiles('tests/CMakeLists.txt') }}
          restore-keys: |
            ${{ runner.os }}-ccache-pico-embedded-

      - name: Download project external dependencies
        run: |
          chmod +x scripts/download_dependencies.sh
          ./scripts/download_dependencies.sh

      - name: Configure and build embedded test target
        run: |
          cmake -S tests -B tests/build_embedded \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build tests/build_embedded --target run_tests_embedded --parallel

      - name: Run embedded-config unit tests
        run: |
          cd tests/build_embedded
          ./run_tests_embedded -r compact

  # ---------------------------------------------------------------------------
  # Pico Layer 2a: ARM Cross-Compile Check (RP2350)
  # Proves firmware compiles for the actual target hardware.
  # ---------------------------------------------------------------------------
  pico-arm-compile-check:
    name: Pico / Layer 2a – ARM Compile (RP2350)
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

      - name: Build Pico firmware (RP2350)
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

      - name: Check binary size
        run: |
          arm-none-eabi-size build/pico/polysynth_pico.elf

  # ---------------------------------------------------------------------------
  # Pico Layer 2b: Wokwi Emulated Firmware Test (RP2040 proxy)
  # Runs actual firmware in Wokwi simulator, checks serial output.
  # ---------------------------------------------------------------------------
  pico-emulation-test:
    name: Pico / Layer 2b – Wokwi Emulation
    runs-on: ubuntu-latest
    needs: pico-arm-compile-check
    # Only run if Wokwi token is configured (optional for forks)
    if: ${{ secrets.WOKWI_CLI_TOKEN != '' }}

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

      - name: Build Pico firmware for emulation (RP2040 proxy)
        env:
          PICO_SDK_PATH: /tmp/pico-sdk
        run: |
          cmake -S src/platform/pico -B build/pico-emu \
            -DPICO_SDK_PATH=$PICO_SDK_PATH \
            -DPICO_BOARD=pico_w \
            -G Ninja
          cmake --build build/pico-emu

      - name: Run firmware in Wokwi emulator
        uses: wokwi/wokwi-ci-action@v1
        with:
          token: ${{ secrets.WOKWI_CLI_TOKEN }}
          path: src/platform/pico/wokwi
          timeout: 30000
          expect_text: "[TEST:ALL_PASSED]"
          fail_text: "[TEST:FAIL]"
          serial_log_file: wokwi-serial.log

      - name: Upload serial log
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: wokwi-serial-log
          path: wokwi-serial.log
          if-no-files-found: ignore

      - name: Upload logic analyzer trace
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: wokwi-logic-trace
          path: src/platform/pico/wokwi/wokwi-logic.vcd
          if-no-files-found: ignore
```

### Design Decision: Wokwi Job is Optional

The Wokwi emulation job uses `if: ${{ secrets.WOKWI_CLI_TOKEN != '' }}`. This means:
- **Main repo:** Runs (token configured in repo secrets)
- **Forks/PRs from forks:** Skips gracefully (no token available)
- **Local development:** Use `just pico-emu-test` with your own token

This follows the [Wokwi CI docs](https://docs.wokwi.com/wokwi-ci/github-actions) recommendation for open-source projects.

### Acceptance Criteria

- [ ] Layer 1 job runs and passes on PR
- [ ] Layer 2a job compiles RP2350 firmware on PR
- [ ] Layer 2b job runs firmware in Wokwi (when token available)
- [ ] `[TEST:ALL_PASSED]` detected in serial output
- [ ] Serial log uploaded as CI artifact
- [ ] Logic analyzer VCD trace uploaded as CI artifact
- [ ] Jobs complete in < 5 minutes total

---

## Task 2.6: Add Wokwi Automation Scenario (Stretch Goal)

### What

Create a [Wokwi Automation Scenario](https://docs.wokwi.com/wokwi-ci/automation-scenarios) YAML file that performs more sophisticated testing — e.g., sending serial commands and verifying responses.

### File to Create

**`/src/platform/pico/wokwi/test_boot.yaml`**

```yaml
# Wokwi Automation Scenario: Boot + Serial Protocol Test
# Tests that firmware boots, self-tests pass, and serial commands work.

name: Boot and Serial Protocol Test

steps:
  # Wait for self-test to complete
  - wait-serial: "[TEST:ALL_PASSED]"
    timeout: 15000

  # Test serial command protocol
  - serial-type: "STATUS\n"
  - wait-serial: "STATUS:"
    timeout: 5000

  # Test note on/off
  - serial-type: "NOTE_ON 60 100\n"
  - wait-serial: "OK: NOTE_ON"
    timeout: 2000

  - serial-type: "NOTE_OFF 60\n"
  - wait-serial: "OK: NOTE_OFF"
    timeout: 2000

  # Test parameter set
  - serial-type: "SET filterCutoff 1000.0\n"
  - wait-serial: "OK: SET"
    timeout: 2000

  # Test error handling
  - serial-type: "INVALID_COMMAND\n"
  - wait-serial: "ERR:"
    timeout: 2000

  # All tests passed
  - serial-type: "STATUS\n"
  - wait-serial: "STATUS:"
    timeout: 2000
```

This scenario is referenced in the CI action via the `scenario` parameter:

```yaml
      - name: Run Wokwi automation scenario
        uses: wokwi/wokwi-ci-action@v1
        with:
          token: ${{ secrets.WOKWI_CLI_TOKEN }}
          path: src/platform/pico/wokwi
          timeout: 60000
          scenario: test_boot.yaml
          serial_log_file: wokwi-serial-scenario.log
```

### Acceptance Criteria

- [ ] Automation scenario runs without timeout
- [ ] Serial command responses match expected patterns
- [ ] Error handling produces `ERR:` response

---

## Definition of Done

- [ ] **Layer 1:** `run_tests_embedded` binary builds and all tests pass on x86
- [ ] **Layer 1:** `-Wdouble-promotion` produces zero warnings in test build
- [ ] **Layer 1:** Tests exercise `POLYSYNTH_MAX_VOICES=4` and `sample_t=float`
- [ ] **Layer 2a:** RP2350 ARM cross-compile succeeds and produces `.uf2`
- [ ] **Layer 2b:** Wokwi emulation runs firmware and detects `[TEST:ALL_PASSED]`
- [ ] CI jobs added to `.github/workflows/ci.yml` and pass on PR
- [ ] `just test` (desktop) still passes — zero regressions
- [ ] `just test-embedded` works locally
- [ ] `just pico-build-emu` produces RP2040 proxy `.uf2`
- [ ] Firmware self-test prints structured `[TEST:PASS/FAIL]` markers
- [ ] Wokwi serial log and logic analyzer VCD uploaded as CI artifacts

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Modify | `/tests/CMakeLists.txt` | Add `run_tests_embedded` target |
| Create | `/tests/unit/Test_EmbeddedConfig.cpp` | Embedded-specific unit tests |
| Create | `/src/platform/pico/wokwi/diagram.json` | Wokwi virtual hardware layout |
| Create | `/src/platform/pico/wokwi/wokwi.toml` | Wokwi CI configuration |
| Create | `/src/platform/pico/wokwi/test_boot.yaml` | Wokwi automation scenario (stretch) |
| Modify | `/src/platform/pico/CMakeLists.txt` | Conditional board selection |
| Modify | `/src/platform/pico/main.cpp` | Self-test harness |
| Modify | `/.github/workflows/ci.yml` | Three new Pico CI jobs |
| Modify | `/Justfile` | `test-embedded`, `pico-build-emu`, `pico-emu-test` |

---

## Troubleshooting Guide

### "run_tests_embedded fails but run_tests passes"
→ This is the intended behavior — the embedded build catches issues the desktop build doesn't. Check:
1. Are you using `float` literals in per-sample code? (`2.0` → `2.0f`)
2. Are tests hardcoding voice count `16` instead of using `kMaxVoices`?
3. Are tests accessing `#if defined(POLYSYNTH_DEPLOY_CHORUS)` guarded code?

### "Wokwi timeout — [TEST:ALL_PASSED] not found"
→ Check the serial log artifact. Common causes:
1. Firmware hangs during CYW43 init (different between Pico W and Pico 2 W)
2. Self-test fails and prints `[TEST:FAIL]` instead
3. USB serial not initialized before test output starts

### "pico-build-emu fails but pico-build succeeds"
→ The RP2040 has different peripheral register maps. Check if any RP2350-specific registers are used. Stick to SDK HAL functions, not direct register access.

### "WOKWI_CLI_TOKEN not set"
→ The Wokwi emulation job is optional. Add your token to repo secrets under `WOKWI_CLI_TOKEN`. Get a token from [wokwi.com/ci](https://wokwi.com/ci).

---

## Testing Mandate for Subsequent Sprints

**Every subsequent sprint's Definition of Done must include:**

1. `just test` — Desktop tests pass (existing requirement)
2. `just test-embedded` — Layer 1 embedded-config tests pass (**NEW**)
3. `just pico-build` — RP2350 ARM cross-compile succeeds (**existing**)
4. CI green — All three Pico CI jobs pass (**NEW**)

**Every new Pico feature must include:**

1. A Catch2 unit test in `Test_EmbeddedConfig.cpp` (or a new test file)
2. A corresponding `[TEST:PASS]` marker in the firmware self-test
3. For serial protocol features: a step in the Wokwi automation scenario

---

## Review Checklist for Reviewers

- [ ] `run_tests_embedded` target has `-Wdouble-promotion` flag
- [ ] Embedded test target defines match Pico CMake: `POLYSYNTH_USE_FLOAT`, `SEA_PLATFORM_EMBEDDED`, `POLYSYNTH_MAX_VOICES=4`
- [ ] Desktop `run_tests` still defines `POLYSYNTH_DEPLOY_*` flags — unaffected
- [ ] Wokwi `diagram.json` uses `board-pi-pico-w` (RP2040), NOT `board-pi-pico-2-w`
- [ ] `wokwi.toml` firmware path resolves correctly from the wokwi/ directory
- [ ] Self-test markers follow exact format: `[TEST:PASS]`, `[TEST:FAIL]`, `[TEST:ALL_PASSED]`
- [ ] Wokwi CI job has `if: ${{ secrets.WOKWI_CLI_TOKEN != '' }}` guard
- [ ] Serial log and VCD artifacts uploaded on both success and failure (`if: always()`)
- [ ] No changes to desktop build behavior
