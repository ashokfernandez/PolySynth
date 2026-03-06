# Sprint 1: Extract Voice to Own Header & Add Isolated Voice Tests

## Goal

Move the `Voice` class out of `VoiceManager.h` into its own `Voice.h` header so it can be tested in isolation. Then write comprehensive unit tests for Voice that are currently impossible because Voice is coupled to VoiceManager.

## Why This Sprint Is First

Every subsequent testing sprint (3, 4, 5, 6) needs to instantiate a `Voice` object in isolation. Currently, `#include "VoiceManager.h"` is the only way to access `Voice`, which drags in the entire VoiceManager and allocator machinery. This sprint is a pure structural refactor — zero audio behaviour changes — that unblocks all future work.

## Prerequisites

- None. This is the first sprint.

## Background Reading

- `src/core/VoiceManager.h` — Lines 17-417 define `Voice`, lines 419-729 define `VoiceManager`
- `docs/architecture/DESIGN_PRINCIPLES.md` — Principle #8: header-only core code for inlining
- `tests/unit/Test_VoiceAllocation.cpp` — Existing tests that exercise VoiceManager (not Voice directly)

---

## Tasks

### Task 1.1: Create `src/core/Voice.h`

**What to do:**
1. Create a new file `src/core/Voice.h`
2. Add `#pragma once` at the top
3. Copy the following from `VoiceManager.h`:
   - All `#include` directives needed by Voice (lines 3-12 of VoiceManager.h):
     ```cpp
     #include "types.h"
     #include <algorithm>
     #include <cmath>
     #include <sea_dsp/sea_adsr.h>
     #include <sea_dsp/sea_biquad_filter.h>
     #include <sea_dsp/sea_cascade_filter.h>
     #include <sea_dsp/sea_ladder_filter.h>
     #include <sea_dsp/sea_lfo.h>
     #include <sea_dsp/sea_oscillator.h>
     ```
   - The `namespace PolySynthCore {` wrapper
   - The entire `Voice` class (lines 17-417)
   - Closing `}` for namespace
4. Do NOT include `<array>` or `<sea_util/sea_voice_allocator.h>` — those are VoiceManager dependencies only.

**Exact source range to extract:** `VoiceManager.h` lines 17 (`class Voice {`) through 417 (`};` closing brace of Voice).

**Verification:** The file should compile standalone:
```bash
echo '#include "Voice.h"' > /tmp/test_voice_include.cpp
# Should compile with the existing CMake include paths
```

### Task 1.2: Update `src/core/VoiceManager.h`

**What to do:**
1. Remove the Voice class definition (lines 17-417)
2. Add `#include "Voice.h"` after the existing includes (after line 13)
3. Keep ALL existing `#include` directives that VoiceManager still needs:
   - `#include "types.h"` — still needed for `kMaxVoices`, `VoiceState`, `VoiceRenderState`
   - `#include <algorithm>` — still needed for `std::clamp` in `ProcessStereo`
   - `#include <array>` — still needed for `std::array<Voice, kNumVoices>`
   - `#include <cmath>` — still needed for `std::cos`, `std::sin`, `std::sqrt`
   - `#include <sea_util/sea_voice_allocator.h>` — still needed for allocator
4. Remove includes that Voice.h now handles and VoiceManager doesn't directly use:
   - `<sea_dsp/sea_adsr.h>`
   - `<sea_dsp/sea_biquad_filter.h>`
   - `<sea_dsp/sea_cascade_filter.h>`
   - `<sea_dsp/sea_ladder_filter.h>`
   - `<sea_dsp/sea_lfo.h>`
   - `<sea_dsp/sea_oscillator.h>`

**After this change, VoiceManager.h should look like:**
```cpp
#pragma once

#include "Voice.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sea_util/sea_voice_allocator.h>

namespace PolySynthCore {

class VoiceManager {
  // ... unchanged from line 419 onwards ...
};

} // namespace PolySynthCore
```

### Task 1.3: Verify existing tests still pass

**What to do:**
```bash
just build   # CMake test build compiles
just test    # All existing tests pass
```

**Expected result:** Zero test failures, zero compilation errors. This is a pure move refactor.

### Task 1.4: Add `tests/unit/Test_Voice.cpp` to CMakeLists.txt

**What to do:**
1. Open `tests/CMakeLists.txt`
2. Find the `set(UNIT_TEST_SOURCES ...)` list
3. Add `unit/Test_Voice.cpp` in alphabetical order

### Task 1.4b: Create `tests/utils/TestHelpers.h` — Shared Test Utilities

**What to do:** Create a shared header for test helpers that will be reused across multiple test files (Sprints 1, 4, 5, 6).

```cpp
#pragma once
// Shared test helpers for Voice and Engine unit tests.
// Used by Test_Voice.cpp, Test_FilterModels.cpp, Test_ParameterBoundaries.cpp, etc.

#include "Voice.h"
#include <cmath>

namespace TestHelpers {

// Process N samples, return the last output value.
inline double processNSamples(PolySynthCore::Voice& voice, int n) {
    double last = 0.0;
    for (int i = 0; i < n; ++i) {
        last = voice.Process();
    }
    return last;
}

// Process N samples, return the maximum absolute output value.
inline double processMaxAbs(PolySynthCore::Voice& voice, int n) {
    double maxAbs = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        double a = std::abs(s);
        if (a > maxAbs) maxAbs = a;
    }
    return maxAbs;
}

// Process N samples, return true if all output values are finite (no NaN or Inf).
inline bool processAllFinite(PolySynthCore::Voice& voice, int n) {
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        if (!std::isfinite(s)) return false;
    }
    return true;
}

// Compute RMS of N samples from a Voice.
inline double processRMS(PolySynthCore::Voice& voice, int n) {
    double sumSq = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        sumSq += s * s;
    }
    return std::sqrt(sumSq / n);
}

} // namespace TestHelpers
```

**Why a shared header:** These helpers are needed by Test_Voice.cpp (Sprint 1), Test_FilterModels.cpp (Sprint 4), Test_ParameterBoundaries.cpp (Sprint 4), and Test_LFO_Routing.cpp (Sprint 6). Duplicating them would violate DRY and create maintenance burden.

**CMakeLists.txt:** The `tests/utils/` directory is already on the include path (verify in `tests/CMakeLists.txt`). If not, add it:
```cmake
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utils)
```

### Task 1.5: Create `tests/unit/Test_Voice.cpp` — Core Tests

**What to do:** Create the test file with the following test cases. Each test case is fully specified below with exact setup and assertions.

**File header:**
```cpp
// Isolated Voice tests. See TESTING_GUIDE.md for patterns.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 256;
} // namespace
```

> **Important:** The test helpers (`processNSamples`, `processMaxAbs`, `processAllFinite`) live in `tests/utils/TestHelpers.h` — see Task 1.4b below. Do NOT define them inline in this file.

**Test Case 1: Idle voice produces silence**
```cpp
CATCH_TEST_CASE("Voice idle produces silence", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    // Voice starts inactive — Process() should return 0.0
    for (int i = 0; i < kBlockSize; ++i) {
        CATCH_REQUIRE(v.Process() == 0.0);
    }
}
```

**Test Case 2: NoteOn triggers audible output**
```cpp
CATCH_TEST_CASE("Voice NoteOn produces non-zero output", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2); // Near-zero attack
    v.NoteOn(60, 127);
    // After a few samples of attack, output should be non-zero
    double maxVal = processMaxAbs(v, kBlockSize);
    CATCH_REQUIRE(maxVal > 0.0);
}
```

**Test Case 3: NoteOff followed by release returns to silence**
```cpp
CATCH_TEST_CASE("Voice returns to silence after NoteOff + release", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.05, 0.8, 0.01); // Very fast release (10ms)
    v.NoteOn(60, 127);
    processNSamples(v, kBlockSize); // Let attack/decay happen
    v.NoteOff();
    // Process enough samples for release to complete (10ms = 480 samples @ 48kHz)
    processNSamples(v, 1000);
    // Voice should now be idle
    CATCH_CHECK(!v.IsActive());
    CATCH_CHECK(v.Process() == 0.0);
}
```

**Test Case 4: All four filter models produce distinct output**
```cpp
CATCH_TEST_CASE("Filter models produce distinct output", "[Voice][Filter]") {
    // Sum of absolute values for each model should differ
    double sums[4] = {};
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(2000.0, 0.5, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        for (int i = 0; i < kBlockSize * 4; ++i) {
            sums[model] += std::abs(v.Process());
        }
    }
    // Each model should produce a different sum (different filter characteristics)
    CATCH_CHECK(sums[0] != sums[1]);
    CATCH_CHECK(sums[0] != sums[2]);
    CATCH_CHECK(sums[0] != sums[3]);
}
```

**Test Case 5: Extreme resonance produces finite output**
```cpp
CATCH_TEST_CASE("All filter models stable at extreme resonance", "[Voice][Filter]") {
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(1000.0, 1.0, 0.0); // Max resonance
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        CATCH_CHECK(processAllFinite(v, kBlockSize * 4));
    }
}
```

**Test Case 6: Zero attack produces immediate onset**
```cpp
CATCH_TEST_CASE("Zero attack time produces immediate output", "[Voice][Envelope]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.0, 0.1, 1.0, 0.2); // Zero attack
    v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
    v.NoteOn(60, 127);
    // First sample after NoteOn should already have signal
    double first = std::abs(v.Process());
    // With zero attack + sustain=1.0, first sample should be close to full amplitude
    CATCH_CHECK(first > 0.0);
}
```

**Test Case 7: Velocity scaling**
```cpp
CATCH_TEST_CASE("Velocity scales output amplitude", "[Voice]") {
    // Low velocity should produce quieter output than high velocity
    double maxLow = 0.0, maxHigh = 0.0;

    Voice vLow;
    vLow.Init(kSampleRate);
    vLow.SetADSR(0.001, 0.1, 1.0, 0.2);
    vLow.NoteOn(60, 10); // Low velocity
    maxLow = processMaxAbs(vLow, kBlockSize);

    Voice vHigh;
    vHigh.Init(kSampleRate);
    vHigh.SetADSR(0.001, 0.1, 1.0, 0.2);
    vHigh.NoteOn(60, 127); // Max velocity
    maxHigh = processMaxAbs(vHigh, kBlockSize);

    CATCH_CHECK(maxHigh > maxLow);
}
```

**Test Case 8: Voice stealing fade**
```cpp
CATCH_TEST_CASE("StartSteal fades voice to zero within 25ms", "[Voice][Stealing]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 5.0); // Long release so voice stays active
    v.NoteOn(60, 127);
    processNSamples(v, kBlockSize); // Let it sustain

    v.StartSteal();
    CATCH_CHECK(v.GetVoiceState() == VoiceState::Stolen);

    // Process 25ms worth of samples (20ms fade + 5ms margin)
    int samples25ms = static_cast<int>(0.025 * kSampleRate);
    processNSamples(v, samples25ms);

    // Voice should be idle after fade completes
    CATCH_CHECK(!v.IsActive());
    CATCH_CHECK(v.GetVoiceState() == VoiceState::Idle);
}
```

**Test Case 9: Poly-mod OscB→FreqA produces measurable frequency change**
```cpp
CATCH_TEST_CASE("PolyMod OscB to FreqA modulates output", "[Voice][PolyMod]") {
    // Compare output with and without FM modulation
    double sumNoMod = 0.0, sumWithMod = 0.0;

    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetMixer(1.0, 1.0, 0.0);
    v1.SetPolyModOscBToFreqA(0.0);
    v1.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumNoMod += std::abs(v1.Process());

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetMixer(1.0, 1.0, 0.0);
    v2.SetPolyModOscBToFreqA(0.8); // Strong FM
    v2.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumWithMod += std::abs(v2.Process());

    CATCH_CHECK(sumNoMod != sumWithMod);
}
```

**Test Case 10: Poly-mod OscB→PWM modulates pulse width**
```cpp
CATCH_TEST_CASE("PolyMod OscB to PWM modulates output", "[Voice][PolyMod]") {
    double sumNoMod = 0.0, sumWithMod = 0.0;

    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetWaveformA(sea::Oscillator::WaveformType::Square);
    v1.SetMixer(1.0, 1.0, 0.0);
    v1.SetPolyModOscBToPWM(0.0);
    v1.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumNoMod += std::abs(v1.Process());

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetWaveformA(sea::Oscillator::WaveformType::Square);
    v2.SetMixer(1.0, 1.0, 0.0);
    v2.SetPolyModOscBToPWM(0.8);
    v2.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumWithMod += std::abs(v2.Process());

    CATCH_CHECK(sumNoMod != sumWithMod);
}
```

**Test Case 11: Poly-mod OscB→Filter modulates filter cutoff**
```cpp
CATCH_TEST_CASE("PolyMod OscB to Filter modulates output", "[Voice][PolyMod]") {
    double sumNoMod = 0.0, sumWithMod = 0.0;

    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetFilter(1000.0, 0.3, 0.0);
    v1.SetMixer(1.0, 1.0, 0.0);
    v1.SetPolyModOscBToFilter(0.0);
    v1.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumNoMod += std::abs(v1.Process());

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetFilter(1000.0, 0.3, 0.0);
    v2.SetMixer(1.0, 1.0, 0.0);
    v2.SetPolyModOscBToFilter(0.8);
    v2.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 2; ++i) sumWithMod += std::abs(v2.Process());

    CATCH_CHECK(sumNoMod != sumWithMod);
}
```

**Test Case 12: Filter envelope modulates cutoff**
```cpp
CATCH_TEST_CASE("Filter envelope modulates filter cutoff", "[Voice][Filter]") {
    double sumNoEnv = 0.0, sumWithEnv = 0.0;

    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetFilterEnv(0.01, 0.1, 0.5, 0.2);
    v1.SetFilter(1000.0, 0.3, 0.0); // No env amount
    v1.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 4; ++i) sumNoEnv += std::abs(v1.Process());

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetFilterEnv(0.01, 0.1, 0.5, 0.2);
    v2.SetFilter(1000.0, 0.3, 0.8); // Strong env amount
    v2.NoteOn(60, 127);
    for (int i = 0; i < kBlockSize * 4; ++i) sumWithEnv += std::abs(v2.Process());

    CATCH_CHECK(sumNoEnv != sumWithEnv);
}
```

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `src/core/Voice.h` | **NEW** | Voice class extracted from VoiceManager.h |
| `src/core/VoiceManager.h` | **MODIFIED** | Remove Voice class, add `#include "Voice.h"` |
| `tests/utils/TestHelpers.h` | **NEW** | Shared test helpers (processMaxAbs, processAllFinite, etc.) |
| `tests/unit/Test_Voice.cpp` | **NEW** | 12 isolated Voice test cases |
| `tests/CMakeLists.txt` | **MODIFIED** | Add Test_Voice.cpp to UNIT_TEST_SOURCES, ensure utils/ is on include path |

---

## Testing Instructions

### Run after Task 1.2 (before writing new tests)
```bash
just build    # Must compile cleanly
just test     # All EXISTING tests must pass — zero regressions
```

### Run after Task 1.5 (after writing new tests)
```bash
just build                                          # Compiles with new test file
just test                                           # All tests pass (old + new)
just asan                                           # No memory errors
just tsan                                           # No data races
python3 scripts/golden_master.py --verify           # Audio unchanged
```

### What a test failure means
- **Compilation error in Voice.h** → Missing include. Check which `sea_dsp` headers Voice needs.
- **Compilation error in VoiceManager.h** → Removed an include that VoiceManager still needs. Add it back.
- **Existing test failure** → The refactor changed something it shouldn't have. Diff VoiceManager.h carefully.
- **New test failure** → Voice behaves differently than expected. Read the failing assertion and check the Voice::Process() logic.

---

## Definition of Done

- [ ] `src/core/Voice.h` exists and the Voice class compiles standalone (no VoiceManager dependency)
- [ ] `src/core/VoiceManager.h` includes `Voice.h` and contains only VoiceManager code
- [ ] All 12 new test cases in `Test_Voice.cpp` pass
- [ ] All pre-existing tests pass with zero changes
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass
- [ ] `just check` — lint + tests pass
- [ ] `just asan` — no memory errors detected
- [ ] `just tsan` — no data races detected
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged
- [ ] No functional changes to any DSP code (pure structural move)

---

## Common Pitfalls

1. **Forgetting `#include <cmath>` in Voice.h** — Voice::Process() uses `std::pow`, `std::exp`, `std::abs`, `std::clamp`, `std::cos`, `std::sin`. All require `<cmath>`.
2. **Removing `<algorithm>` from Voice.h** — `std::clamp` and `std::max` require `<algorithm>`.
3. **Circular include** — Voice.h must NOT include VoiceManager.h. The dependency is one-way: VoiceManager.h → Voice.h.
4. **Namespace mismatch** — Voice.h must wrap the class in `namespace PolySynthCore { ... }`.
5. **Missing `types.h` in Voice.h** — Voice uses `sample_t`, `VoiceState`, `VoiceRenderState`, `kPi` from types.h.
