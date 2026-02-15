# PolySynth Testing Guide

## Test Pyramid

```
                    ┌──────────┐
                    │ Visual   │  Playwright screenshots
                    │Regression│  (component gallery)
                   ┌┴──────────┴┐
                   │ CI Build   │  WAM Emscripten build
                   │Integration │  (catches #if guard issues)
                  ┌┴────────────┴┐
                  │ Golden Master │  19 WAV files, RMS comparison
                  │ Audio Tests   │  (catches signal path regressions)
                 ┌┴──────────────┴┐
                 │   Unit Tests    │  22+ Catch2 test cases
                 │ (src/core only) │  (catches logic errors)
                 └────────────────┘
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

- Test files: `Test_ClassName.cpp` (e.g., `Test_SynthState.cpp`, `Test_Engine_Audio.cpp`)
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

For verifying that unimplemented SynthState fields have no audio effect:

```cpp
CATCH_TEST_CASE("Unimplemented field has no audio effect", "[SynthState]") {
    PolySynthCore::Engine engine;
    engine.Init(48000.0);

    PolySynthCore::SynthState stateA;
    PolySynthCore::SynthState stateB;
    stateB.unimplementedField = 999.0;  // Extreme value

    engine.UpdateState(stateA);
    engine.OnNoteOn(60, 100);
    // ... render samples into bufferA ...

    engine.Reset();
    engine.UpdateState(stateB);
    engine.OnNoteOn(60, 100);
    // ... render samples into bufferB ...

    // Buffers should be identical — field has no DSP effect
    CATCH_CHECK(memcmp(bufferA, bufferB, sizeof(bufferA)) == 0);
}
```

When someone wires up the feature, this test fails — reminding them to add proper tests for the new behaviour.

---

## Golden Master Reference

### How It Works

1. Demo programs in `tests/demos/` each produce a WAV file
2. `golden_master.py --generate` runs all demos and stores outputs in `tests/golden/`
3. `golden_master.py --verify` re-runs demos and compares RMS difference against stored files
4. Tolerance: 0.001 RMS (very strict — catches any signal path change)

### Current Golden Master Files (19)

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

### Adding a New Demo

1. Create `tests/demos/demo_your_feature.cpp`
2. Use the `WavWriter` utility from `tests/utils/WavWriter.h`
3. Add build targets to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(demo_your_feature demos/demo_your_feature.cpp)
   target_link_libraries(demo_your_feature PRIVATE SEA_DSP)
   ```
4. Register in `scripts/golden_master.py` demo list
5. Generate reference: `python3 scripts/golden_master.py --generate`
6. Commit the new `.wav` file in `tests/golden/`

---

## CI Pipeline

The CI pipeline (`.github/workflows/ci.yml`) runs these jobs:

| Job | What it does | Catches |
|---|---|---|
| `native-dsp-tests` | Builds + runs unit tests, golden masters, SEA_DSP tests | Logic bugs, audio regressions |
| `build-wam-demo` | Emscripten WAM build (processor + controller) | Conditional compilation errors |
| `build-storybook-site` | Component gallery + Storybook static site | UI build errors |
| `package-demo-site` | Assembles GitHub Pages deployment | Artifact packaging errors |
| Visual Regression | Playwright screenshot comparisons | UI rendering regressions |

All jobs must pass before merging. The WAM build is particularly valuable as an integration test — it compiles the plugin with `IPLUG_EDITOR` only (no `IPLUG_DSP`), catching any code that accidentally references DSP-only members in shared code paths.
