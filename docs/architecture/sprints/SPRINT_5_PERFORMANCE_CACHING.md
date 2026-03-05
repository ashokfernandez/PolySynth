# Sprint 5: Performance — Cache Expensive Math in DSP Hot Path

## Goal

Pre-compute expensive math operations (`std::pow`, `std::exp`, `std::sqrt`) that are currently repeated per-sample in `Voice::Process()` and `VoiceManager::ProcessStereo()`. Cache the results in member variables and only recalculate when the underlying parameters change.

## Why This Sprint

On desktop, these per-sample `std::pow`/`std::exp`/`std::sqrt` calls are measurable but tolerable. On embedded targets (Cortex-M33 @ 150MHz), they are a significant performance concern — each `std::pow(2.0, x)` call can take hundreds of clock cycles. Caching them is a standard DSP optimisation that makes embedded portability viable.

## Prerequisites

- **Sprint 3 must be complete** — We use the named constants from `DspConstants.h` when implementing the cached values.

## Background Reading

- `src/core/Voice.h` — Lines 27, 76, 86, 110, 114, 135, 149 (expensive math calls)
- `src/core/VoiceManager.h` — Lines 525, 544 (sqrt in ProcessStereo)
- `src/core/DspConstants.h` (from Sprint 3) — Named constants referenced

---

## Tasks

### Task 5.1: Cache OscB Detune Factor in Voice

**Problem:** `std::pow(2.0, mDetuneB / 1200.0)` is computed every sample on line 149 of `Voice::Process()`, and also during `NoteOn()` (line 27, 86) and `ApplyDetuneCents()` (line 114).

**What to do:**

1. Add a new member variable to Voice's private section:
   ```cpp
   sample_t mDetuneFactor = 1.0; // Cached: pow(2.0, mDetuneB / 1200.0)
   ```

2. Update `Voice::SetMixer()` to recompute the factor when detune changes:
   ```cpp
   void SetMixer(sample_t mixA, sample_t mixB, sample_t detuneB) {
       mMixA = std::clamp(mixA, sample_t(0.0), sample_t(1.0));
       mMixB = std::clamp(mixB, sample_t(0.0), sample_t(1.0));
       mDetuneB = detuneB;
       mDetuneFactor = std::pow(2.0, mDetuneB / 1200.0);
   }
   ```

3. Update `Voice::Init()` — after setting mDetuneB, compute:
   ```cpp
   mDetuneFactor = std::pow(2.0, mDetuneB / 1200.0);
   ```

4. Replace all `std::pow(2.0, mDetuneB / 1200.0)` in Process, NoteOn, ApplyDetuneCents with `mDetuneFactor`:
   - Line 27 (Init): `mOscB.SetFrequency(440.0 * mDetuneFactor);`
   - Line 86 (NoteOn): `mOscB.SetFrequency(mFreq * mDetuneFactor);`
   - Line 114 (ApplyDetuneCents): `mOscB.SetFrequency(mFreq * mDetuneFactor);`
   - Line 149 (Process): `sample_t modFreqB = mFreq * mDetuneFactor;`

### Task 5.2: Cache Glide Coefficient in Voice

**Problem:** `std::exp(-kGlideTimeConstant / (mGlideTime * mSampleRate))` is computed every sample during portamento (line 135).

**What to do:**

1. Add a new member variable:
   ```cpp
   sample_t mGlideAlpha = 0.0; // Cached: 1.0 - exp(-kGlideTimeConstant / (mGlideTime * mSampleRate))
   ```

2. Update `Voice::SetGlideTime()`:
   ```cpp
   void SetGlideTime(sample_t seconds) {
       mGlideTime = std::max(sample_t(0.0), seconds);
       if (mGlideTime > 0.0) {
           mGlideAlpha = 1.0 - std::exp(-kGlideTimeConstant / (mGlideTime * mSampleRate));
       } else {
           mGlideAlpha = 1.0; // Instant snap
       }
   }
   ```

3. Replace the per-sample computation in `Voice::Process()`:

   **Before (line 135):**
   ```cpp
   sample_t alpha = 1.0 - std::exp(-kGlideTimeConstant / (mGlideTime * mSampleRate));
   mFreq += (mTargetFreq - mFreq) * alpha;
   ```

   **After:**
   ```cpp
   mFreq += (mTargetFreq - mFreq) * mGlideAlpha;
   ```

4. Also initialise `mGlideAlpha` in `Voice::Init()`:
   ```cpp
   mGlideAlpha = 1.0; // No glide initially
   ```

### Task 5.3: Cache Headroom Scale in VoiceManager

**Problem:** `1.0 / std::sqrt(static_cast<sample_t>(kNumVoices))` is computed every `Process()` and `ProcessStereo()` call (lines 525, 544). `kNumVoices` is a compile-time constant, so this never changes.

**What to do:**

1. Add a `static constexpr` or member constant to VoiceManager:
   ```cpp
   // Pre-computed headroom scaling: 1/sqrt(kNumVoices)
   // This is constant because kNumVoices is a compile-time constant.
   static inline const sample_t kHeadroomScale =
       1.0 / std::sqrt(static_cast<sample_t>(kNumVoices));
   ```

   Note: `constexpr` with `std::sqrt` may not work in C++17 depending on the compiler. Use `static inline const` as a portable alternative.

2. Replace in `Process()` (line 525):
   ```cpp
   return sum * kHeadroomScale;
   ```

3. Replace in `ProcessStereo()` (line 544):
   ```cpp
   outLeft *= kHeadroomScale;
   outRight *= kHeadroomScale;
   ```

### Task 5.4: Cache Voice Stealing Fade Delta (Minor)

**Problem:** `Voice::StartSteal()` computes `1.0 / fadeSamples` each time a voice is stolen (line 262). This is only called during voice stealing (not per-sample), so it's not a hot-path issue. However, for consistency with the caching pattern, we can pre-compute it.

**Decision:** Skip this — it's called infrequently. Document as "not cached because it's not in the per-sample path" to show it was considered.

### Task 5.5: Verify Audio Output & Regenerate Golden Masters

**Critical:** The detune caching SHOULD produce identical output because the mathematical operation is the same. However, the glide caching changes when the coefficient is evaluated — from "every sample" to "when SetGlideTime is called". This means:

- If `SetGlideTime` is called before NoteOn and not changed during playback → identical output
- If `SetGlideTime` is called during playback (parameter automation) → slightly different convergence rate during the transition period

**What to do:**
```bash
just build
just test
python3 scripts/golden_master.py --verify
```

**If golden masters pass:** Great, no audio change.

**If golden masters fail:**
1. Listen to the diff: `python3 scripts/golden_master.py --diff` (if available)
2. If the diff is only in glide-related demos and the RMS difference is small (< 0.01):
   - This is expected due to the glide coefficient caching
   - Regenerate: `python3 scripts/golden_master.py --generate`
   - Commit new golden masters with message: "Regenerate golden masters: glide coefficient now cached in SetGlideTime instead of per-sample"
3. If the diff is in non-glide demos or RMS is large: **something is wrong** — debug before proceeding

### Task 5.6: Add Performance Sanity Test

**What to do:** Add a test in `tests/unit/Test_Voice.cpp` or a new file:

```cpp
CATCH_TEST_CASE("8-voice render completes within reasonable time", "[Voice][Performance]") {
    // This is a soft check — not a hard timing requirement.
    // It catches catastrophic performance regressions (e.g., accidental O(n²) loops).
    PolySynthCore::Engine engine;
    engine.Init(48000.0);
    engine.OnNoteOn(60, 100);
    engine.OnNoteOn(64, 100);
    engine.OnNoteOn(67, 100);
    engine.OnNoteOn(72, 100);
    engine.OnNoteOn(76, 100);
    engine.OnNoteOn(79, 100);
    engine.OnNoteOn(84, 100);
    engine.OnNoteOn(88, 100);

    // Process 10 seconds of audio at 48kHz
    const int totalSamples = 48000 * 10;
    sample_t l, r;
    for (int i = 0; i < totalSamples; ++i) {
        engine.Process(l, r);
    }
    // If we got here without timeout, the performance is acceptable.
    CATCH_CHECK(true);
}
```

### Task 5.7: Update Code Comments

Add a comment block at the top of the cached member section in Voice:
```cpp
// --- Cached values (recomputed in setters, not per-sample) ---
sample_t mDetuneFactor = 1.0;  // = pow(2.0, mDetuneB / 1200.0)
sample_t mGlideAlpha = 1.0;    // = 1.0 - exp(-kGlideTimeConstant / (mGlideTime * mSampleRate))
```

---

## Cleanup & Style Improvements

1. **Remove redundant comments** — After extracting to named constants (Sprint 3) and caching, the inline comments like `// Map 0-1 to 0-5ms` are now redundant. Remove them from Engine.h since the constant name is self-documenting.
2. **Consistent setter pattern** — All Voice setters that affect cached values should follow the same pattern: validate input → store raw value → recompute cache. Document this pattern in a comment.

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `src/core/Voice.h` | **MODIFIED** | Add mDetuneFactor, mGlideAlpha; update setters; replace per-sample math |
| `src/core/VoiceManager.h` | **MODIFIED** | Add kHeadroomScale; replace sqrt calls |
| `tests/golden/` | **POSSIBLY MODIFIED** | Regenerate if glide caching changes output |
| `tests/unit/Test_Voice.cpp` | **MODIFIED** | Add performance sanity test |

---

## Testing Instructions

```bash
just build                                    # Compiles cleanly
just test                                     # All tests pass
just asan                                     # No memory errors
just tsan                                     # No data races
python3 scripts/golden_master.py --verify     # Check if audio changed
```

If golden masters changed:
```bash
python3 scripts/golden_master.py --generate   # Regenerate
# Commit new golden masters with explanation
```

---

## Definition of Done

- [ ] No `std::pow` calls remain in `Voice::Process()`
- [ ] No `std::exp` calls remain in `Voice::Process()`
- [ ] No `std::sqrt` calls remain in `VoiceManager::Process()` or `VoiceManager::ProcessStereo()`
- [ ] `mDetuneFactor` is recomputed in `SetMixer()` and `Init()` only
- [ ] `mGlideAlpha` is recomputed in `SetGlideTime()` and `Init()` only
- [ ] `kHeadroomScale` is a compile-time or init-time constant
- [ ] All existing tests pass
- [ ] Performance sanity test passes (10 seconds of 8-voice audio renders without timeout)
- [ ] If golden masters changed: new masters committed with explanation
- [ ] `just asan` / `just tsan` clean
- [ ] Cached member variables have explanatory comments

---

## Common Pitfalls

1. **Forgetting to recompute cache in Init()** — If `SetMixer()` isn't called before `NoteOn()`, the cached value must be correct from `Init()`.
2. **sqrt not constexpr** — `std::sqrt` is not `constexpr` in C++17. Use `static inline const` instead of `constexpr` for `kHeadroomScale`.
3. **Glide alpha edge case: mGlideTime=0** — Division by zero. Handle with `mGlideAlpha = 1.0` (instant snap) when `mGlideTime <= 0`.
4. **Order of Init operations** — `mDetuneFactor` depends on `mDetuneB`. Make sure `mDetuneB` is set (or defaults to 0.0) before computing the factor.
