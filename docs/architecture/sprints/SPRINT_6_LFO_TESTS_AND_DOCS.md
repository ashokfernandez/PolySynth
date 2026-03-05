# Sprint 6: LFO Routing Tests, Documentation Pass & Architecture Doc Updates

## Goal

1. Add comprehensive LFO routing tests (pitch, filter, amp, pan — all currently untested)
2. Add signal-flow documentation comments to `Voice::Process()`
3. Update architecture documents to reflect the current state of the codebase after Sprints 1-5
4. Update `ARCHITECTURE_REVIEW_PROMPT.md` with resolution status for each issue

## Why This Sprint

LFO routing is one of the most complex modulation features with zero dedicated test coverage. The documentation has drifted from reality after 5 sprints of refactoring. This sprint closes both gaps in a single pass.

## Prerequisites

- **Sprint 1 must be complete** — We use isolated `Voice` for LFO tests.
- Ideally Sprints 2-5 are also complete so documentation reflects the final state.

## Background Reading

- `src/core/Voice.h` — SetLFORouting (line 341-347), LFO modulation in Process (lines 130, 147-155, 188, 218-223, 226, 272)
- `docs/architecture/OVERVIEW.md` — Layer diagram and file map
- `docs/architecture/ARCHITECTURE_REVIEW_PROMPT.md` — Original issues list
- `docs/architecture/TESTING_GUIDE.md` — Test conventions

---

## Tasks

### Task 6.1: Create `tests/unit/Test_LFO_Routing.cpp`

**File header:**
```cpp
// Tests for LFO routing to pitch, filter, amp, and pan targets.
// See TESTING_GUIDE.md for test patterns and conventions.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 4096; // ~85ms, enough for LFO to modulate
} // namespace
```

**Test Case 1: LFO pitch modulation varies frequency over time**
- Setup: Voice with LFO rate=5Hz, depth=1.0, shape=0 (Sine)
- Set LFO routing: pitch=1.0, filter=0.0, amp=0.0, pan=0.0
- NoteOn(60, 127)
- Process 4096 samples, count zero-crossings in first half vs second half
- Assert: Zero-crossing count differs between halves (frequency is varying)
- Tag: `[Voice][LFO]`

**Test Case 2: LFO filter modulation varies spectral content**
- Setup: Voice with LFO rate=5Hz, depth=1.0, shape=0, cutoff=1000Hz
- Set LFO routing: pitch=0.0, filter=1.0, amp=0.0, pan=0.0
- NoteOn(60, 127)
- Compare: RMS of first quarter of render vs third quarter
- Assert: RMS values differ (filter cutoff is sweeping)
- Tag: `[Voice][LFO]`

**Test Case 3: LFO amp modulation varies amplitude (tremolo)**
- Setup: Voice with LFO rate=5Hz, depth=1.0, shape=0
- Set LFO routing: pitch=0.0, filter=0.0, amp=1.0, pan=0.0
- NoteOn(60, 127)
- Compute max amplitude of consecutive 512-sample blocks
- Assert: Max amplitude varies across blocks (tremolo effect)
- Tag: `[Voice][LFO]`

**Test Case 4: LFO pan modulation varies pan position**
- Setup: Voice with LFO rate=5Hz, depth=1.0, shape=0
- Set LFO routing: pitch=0.0, filter=0.0, amp=0.0, pan=1.0
- NoteOn(60, 127)
- Sample `GetPanPosition()` at multiple points during render
- Assert: Pan position changes over time (not constant)
- Tag: `[Voice][LFO]`

**Test Case 5: LFO depth=0 produces no modulation**
- Setup: Voice with LFO rate=5Hz, depth=0.0 (all routing amounts at 1.0)
- Compare output to a Voice with LFO depth=0.0, routing=0.0
- Assert: Output is identical (LFO has no effect at zero depth)
- Tag: `[Voice][LFO]`

**Test Case 6: All LFO waveforms produce valid output**
- For each waveform (0=Sine, 1=Tri, 2=Square, 3=Saw):
  - Setup: Voice with that waveform, rate=5Hz, depth=0.5, pitch routing=1.0
  - NoteOn(60, 127), process 4096 samples
  - Assert: All samples finite (no NaN/Inf)
- Tag: `[Voice][LFO]`

**Test Case 7: LFO rate affects modulation speed**
- Setup: Two voices — one with rate=1Hz, one with rate=10Hz
- Both: depth=1.0, amp routing=1.0
- NoteOn, process 24000 samples (0.5 seconds)
- Count amplitude peaks (local maxima) for each
- Assert: Fast LFO has more peaks than slow LFO
- Tag: `[Voice][LFO]`

### Task 6.2: Add `tests/unit/Test_LFO_Routing.cpp` to CMakeLists.txt

Add `unit/Test_LFO_Routing.cpp` to the `UNIT_TEST_SOURCES` list.

### Task 6.3: Add Signal-Flow Comments to `Voice::Process()`

**What to do:** Add a comment block at the top of `Voice::Process()` and inline section comments. Do NOT change any code — only add comments.

```cpp
inline sample_t Process() {
    // ─── Voice Signal Flow ────────────────────────────────────────────
    // 1. Early exit if voice is idle or stolen-and-faded
    // 2. LFO + Filter Envelope generation
    // 3. Portamento: exponential glide toward target frequency
    // 4. Pitch modulation: base freq ← LFO vibrato + poly-mod (OscB→FreqA, FilterEnv→FreqA)
    // 5. OscB synthesis → used as poly-mod source
    // 6. OscA synthesis → modulated frequency, pulse width from poly-mod
    // 7. Mixer: OscA × mixA + OscB × mixB
    // 8. Filter: base cutoff + filter env + poly-mod + LFO modulation
    // 9. Amp envelope × velocity × tremolo (LFO amp mod)
    // 10. Voice stealing fade (if state == Stolen)
    // ──────────────────────────────────────────────────────────────────

    if (!mActive)
        return 0.0;
    // ... rest of Process unchanged
```

Also add inline section headers:
```cpp
    // ── Step 2: LFO & Filter Envelope ──
    sample_t lfoVal = mLfo.Process();
    sample_t filterEnvVal = mFilterEnv.Process();

    // ── Step 3: Portamento ──
    if (mGlideTime > 0.0 && ...) {
    ...

    // ── Step 4-6: Oscillator Synthesis & Modulation ──
    sample_t modFreqA = mFreq;
    ...

    // ── Step 7: Mixer ──
    sample_t mixed = (oscA * mMixA) + (oscB * mMixB);

    // ── Step 8: Filter (model dispatch) ──
    sample_t cutoff = mBaseCutoff;
    ...

    // ── Step 9: Amplitude Envelope & Tremolo ──
    sample_t ampEnvVal = mAmpEnv.Process();
    ...

    // ── Step 10: Voice Stealing Fade ──
    if (mVoiceState == VoiceState::Stolen) {
    ...
```

### Task 6.4: Document the Filter Model Dispatch

Add a comment above the filter switch in `Voice::Process()`:
```cpp
    // Filter model dispatch: each model uses a different topology.
    // Classic (Biquad): 2nd-order IIR, RBJ coefficients, gentle resonance
    // Ladder: 4-pole transistor ladder, TPT integrators, self-oscillates at high resonance
    // Cascade12: 2-pole cascaded biquad, 12dB/octave slope
    // Cascade24: 4-pole cascaded biquad, 24dB/octave slope (steepest rolloff)
    switch (mFilterModel) {
```

### Task 6.5: Update `docs/architecture/OVERVIEW.md`

**Changes needed after Sprints 1-5:**

1. **File Map — Core section:** Add `Voice.h` row:
   ```
   | `Voice.h` | Per-voice DSP chain: oscillators, filter, envelopes, LFO, modulation |
   ```
   Update `VoiceManager.h` description:
   ```
   | `VoiceManager.h` | Voice allocation, polyphony management, stereo mixing |
   ```
   Add `DspConstants.h` row:
   ```
   | `DspConstants.h` | Named constants for all DSP tuning parameters |
   ```

2. **File Map — Platform section:** Remove `PolySynth_DSP.h` row (deleted in Sprint 2). Update `PolySynth.cpp` description to mention it owns Engine directly.

3. **Dependency Rules — Rule 2:** Update:
   ```
   2. **Platform imports core.** `PolySynth.h` includes `SynthState.h` and (in DSP builds) `Engine.h`.
   ```
   (Remove reference to `PolySynth_DSP.h`.)

4. **Data Flow diagram:** Remove `PolySynthDSP` reference. Show `PolySynthPlugin → Engine` directly.

### Task 6.6: Update `docs/architecture/ARCHITECTURE_REVIEW_PROMPT.md`

Add a "Sprint Resolution Status" section at the top or bottom:

```markdown
## Issue Resolution Status

| Issue | Original Description | Status | Resolution |
|-------|---------------------|--------|------------|
| #1 | SynthState data race between UI/audio threads | RESOLVED | SPSCQueue<SynthState, 4> in PolySynth.h |
| #2 | PolySynth_DSP unnecessary indirection | RESOLVED (Sprint 2) | Deleted; plugin owns Engine directly |
| #3 | Engine::UpdateState missing | RESOLVED | Engine::UpdateState centralises parameter fan-out |
| #4 | Voice monolith in VoiceManager.h | RESOLVED (Sprint 1) | Voice extracted to Voice.h |
| #5 | Magic numbers in hot path | RESOLVED (Sprint 3) | All constants in DspConstants.h |
| #6 | Missing test coverage | RESOLVED (Sprints 1, 4, 6) | 55+ test cases covering Voice, filters, boundaries, LFO |
| #7 | Performance: per-sample pow/exp/sqrt | RESOLVED (Sprint 5) | Cached in setter methods |
```

### Task 6.7: Update `docs/architecture/TESTING_GUIDE.md`

Add LFO testing pattern:
```markdown
### When Testing LFO Routing

LFO modulation is time-varying, so tests must process enough samples for the LFO to complete at least one cycle. At rate=5Hz, one cycle = 200ms = 9600 samples at 48kHz. Common assertions:

- **Pitch modulation**: count zero-crossings in different time windows
- **Amplitude modulation**: compare max amplitude across time blocks
- **Filter modulation**: compare RMS across time blocks (spectral energy changes)
- **Pan modulation**: sample GetPanPosition() at multiple points

Example pattern: see `Test_LFO_Routing.cpp`.
```

---

## Cleanup & Style Improvements

1. **Consistent file map** — Verify all files listed in OVERVIEW.md actually exist. Remove any stale entries.
2. **Remove stale comments** — In Voice.h, remove any TODO comments that have been addressed by Sprints 1-5.
3. **Ensure all new test files have header comments** — Brief one-liner explaining what they test.
4. **Architecture review prompt** — Add a "Last reviewed" date at the top.

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `tests/unit/Test_LFO_Routing.cpp` | **NEW** | 7 LFO routing test cases |
| `tests/CMakeLists.txt` | **MODIFIED** | Add Test_LFO_Routing.cpp |
| `src/core/Voice.h` | **MODIFIED** | Documentation comments only (signal flow, filter dispatch) |
| `docs/architecture/OVERVIEW.md` | **MODIFIED** | Updated file map, dependency rules, data flow |
| `docs/architecture/ARCHITECTURE_REVIEW_PROMPT.md` | **MODIFIED** | Issue resolution status table |
| `docs/architecture/TESTING_GUIDE.md` | **MODIFIED** | LFO testing pattern, updated test count |

---

## Testing Instructions

```bash
just build                                    # Compiles cleanly
just test                                     # All tests pass (old + 7 new LFO tests)
just asan                                     # No memory errors
just tsan                                     # No data races
python3 scripts/golden_master.py --verify     # Audio unchanged (no code changes, only comments)
```

---

## Definition of Done

- [ ] ≥7 LFO routing test cases in `Test_LFO_Routing.cpp`
- [ ] All LFO modulation targets tested: pitch, filter, amp, pan
- [ ] LFO depth=0 "no effect" test passes
- [ ] All LFO waveforms (0-3) produce finite output
- [ ] `Voice::Process()` has a signal-flow comment block at the top
- [ ] `Voice::Process()` has inline section headers for each step
- [ ] Filter model dispatch has a comment explaining each model's topology
- [ ] `OVERVIEW.md` file map reflects current files (Voice.h, DspConstants.h added; PolySynth_DSP.h removed)
- [ ] `ARCHITECTURE_REVIEW_PROMPT.md` has issue resolution status table
- [ ] `TESTING_GUIDE.md` has LFO testing pattern section
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged
- [ ] No functional changes to any DSP code (comments only in Voice.h)

---

## Common Pitfalls

1. **LFO needs Init before use** — `mLfo.Init(sampleRate)` must be called in `Voice::Init()` before the LFO produces meaningful output.
2. **LFO phase is deterministic** — All voices start with the same LFO phase, so comparing two Voice instances at the same sample offset produces identical LFO values. Tests must account for this.
3. **Pan modulation is in GetPanPosition(), not Process()** — Pan modulation happens when VoiceManager reads `voice.GetPanPosition()` during `ProcessStereo()`, not during `voice.Process()`. To test it, sample `GetPanPosition()` between `Process()` calls.
4. **Documentation PRs should be reviewed for accuracy** — Double-check that OVERVIEW.md matches the actual file structure after all sprints.
