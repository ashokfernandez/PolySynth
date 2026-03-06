# Sprint 4: Strengthen Test Pyramid — Filters, Boundaries, Stress & Unused Fields

## Goal

Close the most critical testing gaps: filter model behaviour, parameter boundary safety, voice stealing under stress, and regression guards for unused SynthState fields. Also update the Testing Guide to document the new test patterns.

## Why This Sprint

Without these tests:
- Filter model switching regressions go undetected
- Parameter extreme values (0, max) could produce NaN/Inf that silently corrupts output
- Voice stealing edge cases (rapid note bursts, unison overflow) could cause crashes
- Someone could wire a currently-unused SynthState field without realising they need to update golden masters

## Prerequisites

- **Sprint 1 must be complete** — We use isolated `Voice` for filter model and boundary tests.

## Background Reading

- `src/core/Voice.h` — FilterModel enum, SetFilter, Process (filter section)
- `src/core/SynthState.h` — All fields, especially unused ones: `unison`, `unisonDetune`, `oscASync`, `oscBLoFreq`, `oscBKeyboardPoints`, `mixNoise`, `filterKeyboardTrack`
- `src/core/VoiceManager.h` — OnNoteOn, voice allocation, polyphony enforcement
- `docs/architecture/TESTING_GUIDE.md` — Current test patterns and conventions

---

## Tasks

### Task 4.1: Create `tests/unit/Test_FilterModels.cpp`

**What to do:** Create a new test file with these test cases:

**File header:**
```cpp
// Tests for filter model behaviour. See TESTING_GUIDE.md for patterns.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 2048;
} // namespace
```

> **Note:** `TestHelpers.h` is created in Sprint 1 (Task 1.4b) and provides `processMaxAbs`, `processAllFinite`, `processRMS`, etc. All Sprint 4 tests should use these shared helpers.

**Test Case 1: Each filter model produces distinct spectral output**
- Setup: Create 4 Voice objects, each with a different FilterModel
- All other settings identical: Saw wave, cutoff=2000Hz, resonance=0.5, note=60, velocity=127
- Process `kRenderSamples` samples for each, compute RMS
- Assert: All 6 pairwise RMS comparisons show >1% relative difference
- Tag: `[Voice][Filter]`

**Test Case 2: Switching filter model mid-note doesn't crash**
- Setup: One Voice, NoteOn, process 256 samples with FilterModel::Classic
- Action: Switch to FilterModel::Ladder mid-note, process 256 more
- Switch to Cascade12, process 256 more
- Switch to Cascade24, process 256 more
- Assert: All output samples are finite (no NaN/Inf)
- Tag: `[Voice][Filter]`

**Test Case 3: All models stable at max resonance with rich input**
- For each of the 4 models:
  - Setup: Saw wave (harmonically rich), cutoff=1000Hz, resonance=1.0
  - NoteOn, process 4096 samples
  - Assert: All samples finite, no NaN/Inf
- Tag: `[Voice][Filter]`

**Test Case 4: Low cutoff attenuates signal for all models**
- For each model:
  - Render 2048 samples with cutoff=20Hz (min), note=60 (261Hz fundamental)
  - Render 2048 samples with cutoff=20000Hz (max)
  - Assert: RMS of low-cutoff render < RMS of high-cutoff render (use `CATCH_CHECK(rmsLow < rmsHigh * 0.9)` for robustness)
- Tag: `[Voice][Filter]`

**Test Case 5: High cutoff passes signal for all models**
- For each model:
  - Render 2048 samples with cutoff=20000Hz, note=60
  - Assert: RMS > 0.001 (measurable signal passes through) and all samples finite
- Tag: `[Voice][Filter]`

### Task 4.2: Create `tests/unit/Test_ParameterBoundaries.cpp`

**What to do:** Test extreme parameter values to ensure no NaN/Inf/crash.

**Test Case 1: All SynthState fields at minimum value**
- Setup: Engine, create SynthState with every numeric field at its minimum (0.0 for doubles, 0 for ints, false for bools)
- Override: polyphony=1 (can't be 0), lfoRate=0.01 (can't be 0)
- Action: UpdateState, NoteOn(60, 127), process 2048 samples
- Assert: All samples finite
- Tag: `[Engine][Boundaries]`

**Test Case 2: All SynthState fields at maximum value**
- Setup: Engine, create SynthState with every numeric field at reasonable max:
  - masterGain=1.0, polyphony=16, all envelopes at 10.0s, filterCutoff=20000, resonance=1.0, all mod amounts=1.0, etc.
- Action: UpdateState, NoteOn(60, 127), process 2048 samples
- Assert: All samples finite
- Tag: `[Engine][Boundaries]`

**Test Case 3: Rapid parameter changes every sample**
- Setup: Engine, NoteOn
- Action: For 1024 samples, change filterCutoff between 20 and 20000 every sample (alternating)
- Assert: All output finite
- Tag: `[Engine][Boundaries]`

**Test Case 4: Zero-duration envelope**
- Setup: Voice with attack=0, decay=0, sustain=0, release=0
- Action: NoteOn, process 512 samples
- Assert: No crash, all samples finite
- Tag: `[Voice][Boundaries]`

**Test Case 5: Max polyphony — 16 simultaneous notes**
- Setup: Engine with polyphony=16
- Action: NoteOn for notes 48-63 (all 16), process 2048 samples
- Assert: All output finite, no NaN/Inf
- Tag: `[Engine][Boundaries]`

**Test Case 6: MIDI boundary notes**
- Setup: Engine, default state
- Action: NoteOn(0, 127), process 256 samples, NoteOff(0)
- Action: NoteOn(127, 127), process 256 samples, NoteOff(127)
- Assert: All output finite
- Tag: `[Engine][Boundaries]`

### Task 4.3: Extend `tests/unit/Test_VoiceAllocation.cpp` — Stress Tests

**What to do:** Add these test cases to the existing file:

**Test Case 1: 32 rapid NoteOn into 8-voice polyphony**
- Setup: Engine, polyphony=8
- Action: Send 32 NoteOn events (notes 40-71) without any NoteOff
- Assert: Exactly 8 voices active (voice stealing handled), no crash
- Tag: `[VoiceAllocation][Stress]`

**Test Case 2: Unison overflow**
- Setup: Engine, polyphony=8, unisonCount=4
- Action: 3 NoteOn events (each allocates 4 voices = 12, but max is 8)
- Process 1024 samples
- Assert: No crash, output finite
- Tag: `[VoiceAllocation][Stress]`

**Test Case 3: Sustain pedal + rapid notes + release**
- Setup: Engine, polyphony=8
- Action: Sustain pedal ON, 8 NoteOn, 8 NoteOff (notes held by sustain)
- 4 more NoteOn (voice stealing while sustained)
- Sustain pedal OFF (release sustained notes)
- Process 4096 samples
- Assert: Eventually all voices become idle, no crash
- Tag: `[VoiceAllocation][Stress]`

**Test Case 4: NoteOn/NoteOff same note in quick succession**
- Setup: Engine
- Action: NoteOn(60), process 10 samples, NoteOff(60), NoteOn(60)
- Process 2048 samples
- Assert: Voice is active, output non-zero, no crash
- Tag: `[VoiceAllocation][Stress]`

### Task 4.4: Create `tests/unit/Test_UnusedFields.cpp`

**What to do:** For each SynthState field that is serialised/deserialised but NOT wired to DSP, write a "no-effect" test that verifies identical audio output regardless of the field's value.

**Unused fields** (verified by checking Engine::UpdateState which doesn't reference them):
1. `unison` (bool) — Unison on/off toggle, not wired (unisonCount handles this)
2. `unisonDetune` (double) — Not wired (unisonSpread handles detune via allocator)
3. `oscASync` (bool) — Hard sync not implemented
4. `oscBLoFreq` (bool) — LFO mode for OscB not implemented
5. `oscBKeyboardPoints` (bool) — Key tracking for OscB not implemented
6. `mixNoise` (double) — Noise oscillator not implemented
7. `filterKeyboardTrack` (bool) — Filter key tracking not implemented
8. `oscAFreq` (double) — Set in SynthState but oscillator frequency comes from MIDI NoteOn, not from this field
9. `oscBFreq` (double) — Same: oscillator frequency comes from MIDI NoteOn + detune, not this field

> **Why oscAFreq/oscBFreq are unused:** Engine::UpdateState never reads `state.oscAFreq` or `state.oscBFreq`. The oscillator frequencies are set exclusively by `Voice::NoteOn(note, velocity)` which converts the MIDI note number to Hz. These fields exist in SynthState for potential future "fixed frequency" modes but are currently dead.

**Pattern for each test:**
```cpp
CATCH_TEST_CASE("Unused field 'fieldName' has no audio effect", "[UnusedFields]") {
    PolySynthCore::Engine engineA;
    engineA.Init(kSampleRate);
    PolySynthCore::SynthState stateA;
    engineA.UpdateState(stateA);
    engineA.OnNoteOn(60, 100);

    PolySynthCore::Engine engineB;
    engineB.Init(kSampleRate);
    PolySynthCore::SynthState stateB;
    stateB.fieldName = extremeValue; // e.g., true, 1.0, 999.0
    engineB.UpdateState(stateB);
    engineB.OnNoteOn(60, 100);

    for (int i = 0; i < kRenderSamples; ++i) {
        sample_t lA, rA, lB, rB;
        engineA.Process(lA, rA);
        engineB.Process(lB, rB);
        CATCH_CHECK(lA == lB);
        CATCH_CHECK(rA == rB);
    }
}
```

> **Note on exact comparison:** For the UnusedFields tests, exact `==` comparison is correct and preferred. Both engines process identical state (the unused field has no effect), so the output must be bit-for-bit identical. Using `Approx` here would mask a real bug.

Write 9 test cases, one per unused field.

### Task 4.5: Update `tests/CMakeLists.txt`

Add all new test files to `UNIT_TEST_SOURCES`:
- `unit/Test_FilterModels.cpp`
- `unit/Test_ParameterBoundaries.cpp`
- `unit/Test_UnusedFields.cpp`

(Test_VoiceAllocation.cpp is already listed — just add cases to it.)

### Task 4.6: Update `docs/architecture/TESTING_GUIDE.md`

**What to update:**
1. Update the test pyramid diagram: change "22+ Catch2 test cases" to "45+ Catch2 test cases" (approximate after adding ~23 new tests)
2. Add a new section "### When Testing Filter Models" with the pattern used in Test_FilterModels.cpp
3. Add a new section "### When Testing Parameter Boundaries" with the pattern used in Test_ParameterBoundaries.cpp
4. Add the "No-Effect" test pattern to the existing section, referencing Test_UnusedFields.cpp as the canonical example
5. Add `Test_FilterModels.cpp`, `Test_ParameterBoundaries.cpp`, `Test_UnusedFields.cpp` to the file naming examples

---

## Cleanup & Style Improvements

In addition to writing new tests, apply these style cleanups to files you touch:

1. **Use shared test helpers** — Import `TestHelpers.h` (created in Sprint 1, Task 1.4b) for `processMaxAbs`, `processAllFinite`, `processRMS`. Do NOT duplicate these helpers in new test files.

2. **Consistent tag naming** — Use these standard tags across all test files:
   - `[Voice]`, `[Engine]`, `[Filter]`, `[Envelope]`, `[VoiceAllocation]`
   - `[Boundaries]`, `[Stress]`, `[UnusedFields]`, `[PolyMod]`

3. **Test file header comment** — Add a one-line comment at the top of each new test file:
   ```cpp
   // Tests for [description]. See TESTING_GUIDE.md for patterns.
   ```

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `tests/unit/Test_FilterModels.cpp` | **NEW** | 5 filter model test cases |
| `tests/unit/Test_ParameterBoundaries.cpp` | **NEW** | 6 boundary safety test cases |
| `tests/unit/Test_UnusedFields.cpp` | **NEW** | 9 no-effect regression tests |
| `tests/unit/Test_VoiceAllocation.cpp` | **MODIFIED** | 4 stress test cases added |
| `tests/CMakeLists.txt` | **MODIFIED** | 3 new test files registered |
| `docs/architecture/TESTING_GUIDE.md` | **MODIFIED** | Updated test count, new sections |

---

## Testing Instructions

```bash
just build                                    # All test files compile
just test                                     # All tests pass (old + new)
just asan                                     # No memory errors
just tsan                                     # No data races
python3 scripts/golden_master.py --verify     # Audio unchanged
```

### Expected test count
Before this sprint: ~32 test files
After this sprint: ~35 test files, ~55+ test cases total

---

## Definition of Done

- [ ] ≥5 test cases for filter model behaviour in `Test_FilterModels.cpp`
- [ ] ≥6 test cases for parameter boundary safety in `Test_ParameterBoundaries.cpp`
- [ ] ≥9 no-effect tests for unused SynthState fields (including oscAFreq, oscBFreq) in `Test_UnusedFields.cpp`
- [ ] ≥4 stress test cases added to `Test_VoiceAllocation.cpp`
- [ ] All new test files registered in `tests/CMakeLists.txt`
- [ ] `docs/architecture/TESTING_GUIDE.md` updated with new patterns and test count
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass (new + existing)
- [ ] `just asan` — no memory errors
- [ ] `just tsan` — no data races
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged
- [ ] Consistent test tags used across all new test files

---

## Common Pitfalls

1. **Comparing floating point with `==`** — For the UnusedFields tests, exact comparison (`==`) is correct because both engines process identical state. For other comparisons, use RMS with relative difference thresholds (>1% difference).
2. **Engine::Init resets voices** — When testing Engine, always call `Init()` before `UpdateState()`. The Init resets all voice states.
3. **Voice needs Init before NoteOn** — Always call `voice.Init(sampleRate)` before `voice.NoteOn()`. Uninitialized voice has undefined behaviour.
4. **Catch2 prefix** — Remember to use `CATCH_TEST_CASE`, `CATCH_CHECK`, `CATCH_REQUIRE` (not `TEST_CASE`, `CHECK`, `REQUIRE`). This project uses `CATCH_CONFIG_PREFIX_ALL`.
