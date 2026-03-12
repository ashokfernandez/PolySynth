# PolySynth Testing Guide

## Test Pyramid

```
                  ┌────────────┐
                  │  Visual    │  Skia pixel-diff screenshots
                  │ Regression │  (ComponentGallery, macOS)
                 ┌┴────────────┴┐
                 │  Integration  │  WAM Emscripten build, ARM cross-compile,
                 │    Builds     │  Wokwi emulation, desktop smoke test
                ┌┴──────────────┴┐
                │  Golden Master  │  19 WAV files, RMS + FFT comparison
                │  Audio Tests    │  (catches signal path regressions)
               ┌┴────────────────┴┐
               │  Sanitizer Tests  │  ASan + UBSan (memory), TSan (races)
               │  (same unit tests)│  (catches undefined behaviour)
              ┌┴──────────────────┴┐
              │   Embedded Config   │  float precision, 4-voice limit,
              │   Tests (Pico)      │  -Wdouble-promotion (Layer 1)
             ┌┴────────────────────┴┐
             │     Unit Tests        │  55+ Catch2 test cases
             │   (src/core only)     │  (catches logic errors)
             └──────────────────────┘
```

Each layer catches different classes of bugs. All layers run in CI on every PR.

---

## Running Tests Locally

### Prerequisites

```bash
# Download external dependencies (iPlug2, Catch2, DaisySP)
./scripts/download_dependencies.sh
```

### Unit Tests

```bash
cd tests
mkdir -p build && cd build
cmake ..
make -j$(nproc)    # Linux
make -j$(sysctl -n hw.ncpu)  # macOS

# Run all tests
./run_tests

# Run specific test by name
./run_tests "SynthState Reset restores all fields"

# Run tests matching a tag
./run_tests "[SynthState]"
./run_tests "[preset]"
./run_tests "[Engine]"
```

### Golden Master Audio Tests

```bash
# Verify current audio matches golden references
python3 scripts/golden_master.py --verify

# Regenerate golden references (after intentional audio changes)
python3 scripts/golden_master.py --generate
```

### SEA_DSP Library Tests

```bash
cmake -S libs/SEA_DSP -B build/sea_dsp_tests -DSEA_BUILD_TESTS=ON
cmake --build build/sea_dsp_tests
./build/sea_dsp_tests/tests/sea_tests
```

---

## Conventions

### File Naming

- Test files: `Test_ClassName.cpp` (e.g., `Test_SynthState.cpp`, `Test_Engine_Audio.cpp`, `Test_FilterModels.cpp`, `Test_ParameterBoundaries.cpp`, `Test_UnusedFields.cpp`)
- Demo programs: `demo_feature_name.cpp` (e.g., `demo_filter_sweep.cpp`)
- One test file per class or concern

### Catch2 Macros

This project uses Catch2 v2.13.9 with the `CATCH_CONFIG_PREFIX_ALL` option. This means all Catch2 macros are prefixed with `CATCH_`:

```cpp
#define CATCH_CONFIG_PREFIX_ALL
#include "catch.hpp"

CATCH_TEST_CASE("Description of what's being tested", "[Tag]") {
    // Setup
    PolySynthCore::SynthState state;

    CATCH_SECTION("specific scenario") {
        state.masterGain = 0.5;
        CATCH_CHECK(state.masterGain == 0.5);
        CATCH_REQUIRE(state.masterGain > 0.0);  // Aborts section on failure
    }
}
```

- Use `CATCH_CHECK` for assertions that should continue on failure (see all failures at once).
- Use `CATCH_REQUIRE` for assertions where continuing would crash or be meaningless.
- Use `Approx` for floating-point comparisons: `CATCH_CHECK(value == Approx(0.5).margin(0.001))`.

### Test Structure

```cpp
CATCH_TEST_CASE("ClassName behaves correctly", "[ClassName]") {
    // Common setup for all sections
    PolySynthCore::Engine engine;
    engine.Init(48000.0);

    CATCH_SECTION("specific behavior A") {
        // Test one thing
    }

    CATCH_SECTION("specific behavior B") {
        // Test another thing — setup is re-run fresh
    }
}
```

### Registration

Every new test file must be added to `tests/CMakeLists.txt`:

```cmake
set(UNIT_TEST_SOURCES
    main_test.cpp
    # ...existing tests...
    unit/Test_YourNewTest.cpp   # <-- Add here
    ../src/core/PresetManager.cpp
)
```

---

## What to Test

### When Adding a SynthState Field

The existing tests in `Test_SynthState.cpp` enforce:
1. **Reset equivalence**: `Reset()` restores the field to its default-constructed value
2. **Serialization round-trip**: The field survives JSON serialize/deserialize

You must update both test cases with the new field. See [ADDING_A_PARAMETER.md](ADDING_A_PARAMETER.md) Step 9.

### When Adding a DSP Feature

1. **Unit test** the feature in isolation (e.g., `Test_Voice.cpp` for a new modulation source)
2. **Add a demo program** that produces audible output of the feature
3. **Add the demo to golden master** generation (`scripts/golden_master.py`)
4. **Generate new golden references**: `python3 scripts/golden_master.py --generate`

### When Fixing a Bug

1. Write a test that **fails before the fix** and **passes after**
2. If the bug is in the signal path, verify golden masters still pass (or regenerate if the fix intentionally changes audio)

### When Testing Filter Models

Test each `Voice::FilterModel` variant in isolation. See `Test_FilterModels.cpp` for the canonical examples:

- **Distinct output**: Render the same input through each model and verify RMS values differ by >1%
- **Mid-note switching**: Change the filter model during a note and verify no crash/NaN
- **Max resonance stability**: All models must remain finite at resonance=1.0 with rich (Saw) input
- **Cutoff response**: Low cutoff should attenuate, high cutoff should pass signal through

```cpp
Voice v;
v.Init(kSampleRate);
v.SetFilterModel(Voice::FilterModel::Ladder);
v.SetFilter(2000.0, 0.5, 0.0);
v.SetADSR(0.001, 0.1, 1.0, 0.2);
v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
v.NoteOn(60, 127);
double rms = processRMS(v, kRenderSamples);
```

### When Testing Parameter Boundaries

Test extreme values (min, max, zero, rapid changes) to ensure no NaN/Inf. See `Test_ParameterBoundaries.cpp`:

- Set every numeric field to its minimum or maximum reasonable value
- Alternate parameters every sample to stress smoothing/interpolation
- Test zero-duration envelopes (attack=decay=sustain=release=0)
- Test MIDI boundary notes (0 and 127)

Use the local helper pattern for Engine-level boundary tests:
```cpp
bool engineProcessAllFinite(Engine& engine, int n) {
    for (int i = 0; i < n; ++i) {
        sample_t left = 0.0, right = 0.0;
        engine.Process(left, right);
        if (!std::isfinite(left) || !std::isfinite(right)) return false;
    }
    return true;
}
```

---

## Test Patterns

### Audio Output Comparison

For testing that audio processing produces expected results:

```cpp
CATCH_TEST_CASE("Engine produces non-silent output", "[Engine]") {
    PolySynthCore::Engine engine;
    engine.Init(48000.0);
    engine.OnNoteOn(60, 100);

    // Render some samples
    double maxAbs = 0.0;
    for (int i = 0; i < 4800; i++) {  // 100ms at 48kHz
        PolySynthCore::sample_t left, right;
        engine.Process(left, right);
        maxAbs = std::max(maxAbs, std::abs(left));
    }

    CATCH_CHECK(maxAbs > 0.01);  // Should produce audible output
}
```

### Parameter Round-Trip

For verifying state survives serialization:

```cpp
SynthState original;
original.myField = 0.42;  // Non-default

std::string json = PresetManager::Serialize(original);
SynthState loaded;
CATCH_REQUIRE(PresetManager::Deserialize(json, loaded));
CATCH_CHECK(loaded.myField == original.myField);
```

### RMS Comparison Between Renders

For verifying two audio renders are identical (or different):

```cpp
// Render with default state
std::vector<double> renderA = RenderAudio(engine, defaultState, numSamples);

// Render with modified state
std::vector<double> renderB = RenderAudio(engine, modifiedState, numSamples);

// Compare
double rms = ComputeRmsDiff(renderA, renderB);
CATCH_CHECK(rms < 0.001);  // Should be identical
// OR
CATCH_CHECK(rms > 0.01);   // Should be audibly different
```

### The "No-Effect" Test Pattern

For verifying that unimplemented SynthState fields have no audio effect. See `Test_UnusedFields.cpp` for the canonical examples (9 tests covering all unused fields).

Use two separate Engine instances to guarantee identical initial state, and compare output sample-by-sample with exact `==` (not `Approx`) since both engines process identical state:

```cpp
void verifyIdenticalOutput(const SynthState& stateA, const SynthState& stateB) {
    Engine engineA;
    engineA.Init(kSampleRate);
    engineA.UpdateState(stateA);
    engineA.OnNoteOn(60, 100);

    Engine engineB;
    engineB.Init(kSampleRate);
    engineB.UpdateState(stateB);
    engineB.OnNoteOn(60, 100);

    for (int i = 0; i < kRenderSamples; ++i) {
        sample_t lA = 0.0, rA = 0.0, lB = 0.0, rB = 0.0;
        engineA.Process(lA, rA);
        engineB.Process(lB, rB);
        CATCH_CHECK(lA == lB);
        CATCH_CHECK(rA == rB);
    }
}

CATCH_TEST_CASE("Unused field 'mixNoise' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.mixNoise = 1.0;
    verifyIdenticalOutput(stateA, stateB);
}
```

When someone wires up the feature, this test fails — reminding them to add proper tests for the new behaviour.

### When Testing LFO Routing

LFO modulation is time-varying, so tests must process enough samples for the LFO to complete at least one cycle. At rate=5Hz, one cycle = 200ms = 9600 samples at 48kHz. Common assertions:

- **Pitch modulation**: count zero-crossings in different time windows
- **Amplitude modulation**: compare max amplitude across time blocks
- **Filter modulation**: compare RMS across time blocks (spectral energy changes)
- **Pan modulation**: sample GetPanPosition() at multiple points (must call Process() between reads)

Use relative difference thresholds (>1%) rather than bare `!=` for robustness.

Example pattern: see `Test_LFO_Routing.cpp`.

---

## Golden Master Reference

### Overview

Golden master tests verify DSP determinism by comparing audio output against
committed reference files. Any signal path change — intentional or accidental —
shows up as an RMS diff exceeding the 0.001 tolerance.

Reference files are stored in Git LFS under `tests/golden/` with one
subdirectory per target platform:

```
tests/golden/
├── desktop-x86_64/      19 WAV files (double precision, all effects)
├── embedded-x86_64/     15 WAV files + pico_signal_chain.bin/.wav
└── embedded-armv7/      15 WAV files (ARM 32-bit via QEMU)
```

### What Each Target Tests

| Target | Build flags | What it proves |
|---|---|---|
| **desktop-x86_64** | Default (double, all FX) | Full-fidelity signal path on x86-64 |
| **embedded-x86_64** | `POLYSYNTH_USE_FLOAT`, `MAX_VOICES=4`, FX off | Pico-equivalent DSP on host — catches float precision and compile-flag regressions |
| **embedded-armv7** | ARM cross-compile + VFPv4 FPU, run under QEMU | ARM 32-bit single-precision float behavior — catches FMA fusion, rounding differences vs x86 |

The **embedded-x86_64** target also produces a binary golden master
(`pico_signal_chain.bin`) — an exact I2S-packed buffer from the full signal
chain (Engine → gain → fast_tanh → int16 saturation → I2S pack). This catches
post-processing regressions that RMS comparison alone might miss.

### Wokwi CRC (Firmware-Level Verification)

The RP2350 firmware includes an on-device golden master check
(`src/platform/pico/pico_self_test.cpp`). At boot it renders 2048 stereo
frames through the complete signal chain and computes a CRC32 of the packed I2S
buffer. The expected CRC is compiled into the binary via `golden_crc.h`.

This is the only test that exercises the *actual* firmware code path including
ARM `SSAT` instructions, Pico SDK math, and the real `PackI2S` implementation.
It runs in the Wokwi RP2040 emulator in CI (Layer 2b) and on real hardware
during development.

**Bootstrap flow:** On first run (CRC placeholder = `0x00000000`), the test
prints the generated CRC to serial and passes. Update `golden_crc.h` with the
printed value to lock in the reference.

### How It Works

1. Demo programs in `tests/demos/` each produce a WAV file
2. `golden_master.py --generate` runs all demos and stores outputs in the target directory
3. `golden_master.py --verify` re-runs demos and compares RMS difference against stored files
4. Tolerance: 0.001 RMS (very strict — catches any signal path change)
5. The embedded target additionally verifies exact binary match for `pico_signal_chain.bin`

### Commands

```bash
# Desktop
just golden-generate           # Write tests/golden/desktop-x86_64/*.wav
just golden-verify             # Verify against committed desktop references

# Embedded (Pico-equivalent, host-compiled)
just golden-generate-embedded  # Write tests/golden/embedded-x86_64/*.wav + .bin
just golden-verify-embedded    # Verify against committed embedded references

# ARM (QEMU cross-compiled)
just golden-generate-arm       # Write tests/golden/embedded-armv7/*.wav
just golden-verify-arm         # Verify against committed ARM references
```

### Current Golden Master Files

**Desktop (19 WAVs):**
```
demo_engine_poly.wav     demo_lfo_filter_wobble.wav
demo_engine_saw.wav      demo_lfo_tremolo.wav
demo_factory_presets.wav  demo_lfo_vibrato.wav
demo_filter_envelope.wav  demo_osc_mix.wav
demo_filter_resonance.wav demo_poly_chords.wav
demo_filter_sweep.wav    demo_polymod.wav
demo_fx_chorus.wav       demo_prophet_filter.wav
demo_fx_delay.wav        demo_pulse_width.wav
demo_fx_limiter.wav      demo_render_wav.wav
demo_waveforms.wav
```

**Embedded (15 WAVs + binary):** Same as desktop minus `demo_fx_chorus`,
`demo_fx_delay`, `demo_fx_limiter` (effects compile-guarded), plus
`pico_signal_chain.bin` and `pico_signal_chain.wav`.

**ARM:** Same set as embedded, cross-compiled for ARM 32-bit.

### Adding a New Demo

1. Create `tests/demos/demo_your_feature.cpp`
2. Use the `WavWriter` utility from `tests/utils/WavWriter.h`
3. Add build targets to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(demo_your_feature demos/demo_your_feature.cpp)
   target_link_libraries(demo_your_feature PRIVATE SEA_DSP)
   ```
4. Register in `scripts/golden_master.py` demo list
5. Generate references for all targets:
   ```bash
   just golden-generate
   just golden-generate-embedded
   just golden-generate-arm  # if applicable
   ```
6. Commit the new files (stored in Git LFS automatically)

---

## CI Pipeline

The CI pipeline (`.github/workflows/ci.yml`) uses a safety-gate pattern: fast analysis jobs run first in parallel, and downstream jobs only start after all gates pass.

### Job Dependency Graph

```
Parallel start (safety gates):
  lint-static-analysis
  build-asan-tests
  build-tsan-tests
  pico-native-embedded-tests     (independent)
  pico-arm-compile-check         (independent)

After safety gates pass (parallel):
  native-dsp-tests               (golden masters, frequency analysis)
  native-ui-tests                (VRT + desktop smoke, macOS)
  build-wam-demo                 (Emscripten WAM build)

Tag-only (release):
  build-macos / build-windows    (needs: dsp-tests + ui-tests)
  publish-release                (needs: both platform builds)

Main-only (deploy):
  package-demo-site -> publish-pages
```

### Job Reference

| Job | What it does | Catches |
|---|---|---|
| `lint-static-analysis` | Cppcheck, Clang-Tidy, platform API guard checks | Logic errors, style violations, unguarded `BundleResourcePath` calls |
| `build-asan-tests` | Unit tests with AddressSanitizer + UBSan (Clang) | Memory corruption, buffer overflows, undefined behaviour |
| `build-tsan-tests` | Unit tests with ThreadSanitizer (Clang) | Data races in concurrent audio processing |
| `native-dsp-tests` | Clean release build + unit tests + golden masters + frequency analysis | Audio regressions, signal path changes |
| `native-ui-tests` | VRT (Skia pixel-diff) + desktop app smoke test (macOS) | Visual regressions, startup crashes |
| `build-wam-demo` | Emscripten WAM build (processor + controller) | Conditional compilation errors, `#if IPLUG_EDITOR` guard issues |
| `pico-native-embedded-tests` | Unit tests with `POLYSYNTH_USE_FLOAT`, `MAX_VOICES=4`, `-Wdouble-promotion` | Float precision regressions, voice-count assumptions |
| `pico-arm-compile-check` | ARM cross-compile for RP2350 + binary size report | Linker errors, missing symbols on embedded target |
| `pico-emulation-test` | Wokwi RP2040 emulator runs firmware, checks `[TEST:ALL_PASSED]` serial output | Runtime failures on actual (emulated) hardware. Optional — requires `WOKWI_CLI_TOKEN` |
| `package-demo-site` | Assembles GitHub Pages deployment + validates HTML references | Artifact packaging errors |

All safety-gate jobs must pass before downstream jobs run. The WAM build is particularly valuable as an integration test — it compiles the plugin with `IPLUG_EDITOR` only (no `IPLUG_DSP`), catching any code that accidentally references DSP-only members in shared code paths.

### Embedded Testing Layers (Pico)

The Pico CI uses a four-layer testing approach, progressively closer to real
hardware:

| Layer | Environment | Golden dir | What it proves |
|---|---|---|---|
| **Layer 1** | Native host (x86) + embedded compile flags | `embedded-x86_64/` | DSP logic correct under `float` precision and 4-voice limit |
| **Layer 1b** | ARM cross-compile + QEMU user-mode | `embedded-armv7/` | ARM 32-bit float behavior (FMA, rounding) matches reference |
| **Layer 2a** | ARM bare-metal cross-compile (RP2350) | — | Full firmware links for target hardware |
| **Layer 2b** | Wokwi emulator (RP2040 proxy) | CRC in `golden_crc.h` | Firmware boots, runs self-tests, signal chain CRC matches |

**Layer 1** catches the most common regressions: float precision changes,
voice-count assumptions, and accidentally enabled effects. **Layer 1b** catches
ARM-specific issues that x86 cannot detect (different FPU rounding, FMA
fusion). **Layer 2b** is the only layer that exercises the actual firmware code
path including ARM SSAT instructions.

Layer 2b uses RP2040 as a proxy because Wokwi does not yet support RP2350
natively. The code is identical — only the board target differs.
