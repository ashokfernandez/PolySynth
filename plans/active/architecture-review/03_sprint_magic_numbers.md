# Sprint 3: Extract Magic Numbers into Named Constants

## Goal

Replace every undocumented numeric literal in the DSP hot path (`Voice::Process()`, `Engine::SetChorus/SetDelay/SetLimiter`, `VoiceManager::ProcessStereo()`) with a named `constexpr` constant. Each constant gets a one-line comment explaining what it controls.

## Why This Sprint

The codebase contains ~15 magic numbers in performance-critical audio code. Examples:
- `0.05` (LFO pitch modulation depth scale — what unit? what range?)
- `10000.0` (filter envelope max Hz range — why this number?)
- `0.020` (voice stealing fade time — seconds? milliseconds?)
- `0.5 + (res * res * 20.0)` (resonance-to-Q mapping — what curve?)

Named constants make the code self-documenting, prevent copy-paste errors, and give embedded developers a single place to tune DSP parameters for their target.

## Prerequisites

- **Sprint 1 must be complete** — Voice is now in `Voice.h`, so we modify that file instead of VoiceManager.h.

## Background Reading

- `src/core/Voice.h` (after Sprint 1 extraction) — Process method with magic numbers
- `src/core/Engine.h` — FX setters with magic numbers
- `src/core/VoiceManager.h` — ProcessStereo with sqrt

---

## Tasks

### Task 3.1: Create `src/core/DspConstants.h`

**What to do:** Create a new header-only file with all the constants. Use `constexpr` for compile-time evaluation.

```cpp
#pragma once

#include "types.h"

namespace PolySynthCore {

// ============================================================================
// Voice: Resonance Mapping
// ============================================================================

// Maps UI resonance (0.0–1.0) to biquad filter Q value.
// Q = kResonanceBaseQ + (res² × kResonanceScale)
// At res=0: Q=0.5 (gentle rolloff). At res=1: Q=20.5 (sharp peak).
constexpr sample_t kResonanceBaseQ = 0.5;
constexpr sample_t kResonanceScale = 20.0;

// ============================================================================
// Voice: LFO Modulation Scaling
// ============================================================================

// LFO pitch modulation: scales lfoVal × depth to a frequency multiplier.
// At depth=1.0 and lfoVal=1.0: frequency shifts by ±5% (≈0.85 semitones).
constexpr sample_t kLfoPitchScale = 0.05;

// ============================================================================
// Voice: Poly-Mod PWM Scaling
// ============================================================================

// Scales OscB/FilterEnv modulation of pulse width.
// At full modulation (1.0), PW shifts by ±0.5 around the base PW.
constexpr sample_t kPwmModScale = 0.5;

// ============================================================================
// Voice: Filter Envelope Scaling
// ============================================================================

// Maximum Hz range added by the filter envelope.
// When filterEnvAmount=1.0 and envelope=1.0, cutoff increases by 10,000 Hz.
constexpr sample_t kFilterEnvMaxHz = 10000.0;

// ============================================================================
// Voice: Portamento / Glide
// ============================================================================

// Exponential smoothing time constant for portamento.
// Higher values = faster convergence. 3.0 means ~95% convergence in glideTime seconds.
constexpr sample_t kGlideTimeConstant = 3.0;

// Snap threshold (Hz): when gliding frequency is within this distance of target,
// snap immediately to avoid asymptotic creep.
constexpr sample_t kGlideSnapThresholdHz = 0.01;

// ============================================================================
// Voice: Voice Stealing
// ============================================================================

// Minimum fade time (seconds) when a voice is stolen.
// 20ms prevents audible clicks during voice stealing.
constexpr sample_t kStolenFadeTimeSec = 0.020;

// ============================================================================
// Engine: Chorus FX
// ============================================================================

// Maps UI depth (0.0–1.0) to chorus delay modulation depth in milliseconds.
// At depth=1.0: ±5ms of delay modulation.
constexpr sample_t kChorusDepthMs = 5.0;

// ============================================================================
// Engine: Delay FX
// ============================================================================

// Maximum delay line length in milliseconds.
constexpr sample_t kMaxDelayMs = 2000.0;

// Conversion factor: UI delay time (0.0–1.2 seconds) → internal milliseconds.
constexpr sample_t kDelayTimeToMs = 1000.0;

// Conversion factor: UI feedback (0.0–0.95) → internal percentage (0–95).
constexpr sample_t kDelayFeedbackScale = 100.0;

// Conversion factor: UI mix (0.0–1.0) → internal percentage (0–100).
constexpr sample_t kDelayMixScale = 100.0;

// ============================================================================
// Engine: Limiter FX
// ============================================================================

// Default lookahead time in milliseconds for the lookahead limiter.
constexpr sample_t kLimiterLookaheadMs = 5.0;

// Default release time in milliseconds for the lookahead limiter.
constexpr sample_t kLimiterReleaseMs = 50.0;

// ============================================================================
// Engine: OscB Fine Tune Scaling
// ============================================================================

// Multiplier to convert UI fine tune (semitones, ±12) to cents for detune.
// Engine::UpdateState multiplies oscBFineTune by this to get cents.
constexpr sample_t kFineTuneToCents = 100.0;

} // namespace PolySynthCore
```

### Task 3.2: Update `src/core/Voice.h` — Replace Magic Numbers

**What to do:** Add `#include "DspConstants.h"` at the top of Voice.h, then make these exact replacements.

> **Note on line numbers:** After Sprint 1 extracts Voice to its own file, line numbers will differ from VoiceManager.h. Search for the **Before** string to find each location — every pattern below is unique in the file.

**Change 1: Resonance mapping** (in `Voice::SetFilter` — search for `0.5) + (res * res`)

Before:
```cpp
mBaseRes = sample_t(0.5) + (res * res * sample_t(20.0));
```
After:
```cpp
mBaseRes = kResonanceBaseQ + (res * res * kResonanceScale);
```

**Change 2: LFO pitch modulation** (in `Voice::Process`, pitch modulation section — search for `mLfoPitchDepth * 0.05`)

Before:
```cpp
sample_t modMult = (1.0 + lfoVal * mLfoPitchDepth * 0.05);
```
After:
```cpp
sample_t modMult = (1.0 + lfoVal * mLfoPitchDepth * kLfoPitchScale);
```

**Change 3: PWM modulation** (in `Voice::Process`, PWM section — search for `pwmMod * 0.5`)

Before:
```cpp
sample_t pwmA = mBasePulseWidthA + (pwmMod * 0.5);
```
After:
```cpp
sample_t pwmA = mBasePulseWidthA + (pwmMod * kPwmModScale);
```

**Change 4: Filter envelope scaling** (in `Voice::Process`, filter modulation section — search for `* 10000.0`)

Before:
```cpp
cutoff += filterEnvVal * (mFilterEnvAmount + mPolyModFilterEnvToFilter) * 10000.0;
```
After:
```cpp
cutoff += filterEnvVal * (mFilterEnvAmount + mPolyModFilterEnvToFilter) * kFilterEnvMaxHz;
```

**Change 5: Glide time constant** (in `Voice::Process`, portamento section — search for `-3.0 / (mGlideTime`)

Before:
```cpp
sample_t alpha = 1.0 - std::exp(-3.0 / (mGlideTime * mSampleRate));
```
After:
```cpp
sample_t alpha = 1.0 - std::exp(-kGlideTimeConstant / (mGlideTime * mSampleRate));
```

**Change 6: Glide snap threshold** (in `Voice::Process`, portamento section — search for `> 0.01` near `mTargetFreq`. There are TWO occurrences: one `>` and one `<`)

Before:
```cpp
if (mGlideTime > 0.0 && std::abs(mFreq - mTargetFreq) > 0.01) {
    ...
    if (std::abs(mFreq - mTargetFreq) < 0.01) {
```
After:
```cpp
if (mGlideTime > 0.0 && std::abs(mFreq - mTargetFreq) > kGlideSnapThresholdHz) {
    ...
    if (std::abs(mFreq - mTargetFreq) < kGlideSnapThresholdHz) {
```

**Change 7: Voice stealing fade time** (in `Voice::StartSteal` — search for `0.020`)

Before:
```cpp
const sample_t fadeSamples = std::max(sample_t(1.0), sample_t(0.020) * mSampleRate);
```
After:
```cpp
const sample_t fadeSamples = std::max(sample_t(1.0), kStolenFadeTimeSec * mSampleRate);
```

### Task 3.3: Update `src/core/Engine.h` — Replace Magic Numbers

**What to do:** Add `#include "DspConstants.h"` at the top of Engine.h, then make these exact replacements:

**Change 1: Delay init** (in `Engine::Init` — search for `2000.0` near `mDelay.Init`)

Before:
```cpp
mDelay.Init(sampleRate, 2000.0);
```
After:
```cpp
mDelay.Init(sampleRate, kMaxDelayMs);
```

**Change 2: Limiter params** (in `Engine::UpdateState` — search for `5.0, 50.0` near `SetLimiter`)

Before:
```cpp
SetLimiter(state.fxLimiterThreshold, 5.0, 50.0);
```
After:
```cpp
SetLimiter(state.fxLimiterThreshold, kLimiterLookaheadMs, kLimiterReleaseMs);
```

**Change 3: Chorus depth** (in `Engine::SetChorus` — search for `depth * 5.0`)

Before:
```cpp
mChorus.SetDepth(depth * 5.0); // Map 0-1 to 0-5ms
```
After:
```cpp
mChorus.SetDepth(depth * kChorusDepthMs);
```

**Change 4: Delay time/feedback/mix scaling** (in `Engine::SetDelay` — search for `timeSec * 1000.0`)

Before:
```cpp
mDelay.SetTime(timeSec * 1000.0);
mDelay.SetFeedback(feedback * 100.0);
mDelay.SetMix(mix * 100.0);
```
After:
```cpp
mDelay.SetTime(timeSec * kDelayTimeToMs);
mDelay.SetFeedback(feedback * kDelayFeedbackScale);
mDelay.SetMix(mix * kDelayMixScale);
```

**Change 5: Fine tune scaling** (in `Engine::UpdateState` — search for `oscBFineTune * 100.0`)

Before:
```cpp
mVoiceManager.SetMixer(state.mixOscA, state.mixOscB, state.oscBFineTune * 100.0);
```
After:
```cpp
mVoiceManager.SetMixer(state.mixOscA, state.mixOscB, state.oscBFineTune * kFineTuneToCents);
```

### Task 3.4: Verify — Pure Rename, No Value Changes

**Critical check:** Every constant value must exactly match the original magic number. This is a rename-only change. No audio behaviour should change.

```bash
just build
just test
python3 scripts/golden_master.py --verify   # MUST be unchanged
```

### Task 3.5: Optional — Add compile-time assertions

In `DspConstants.h`, add assertions that constants are positive:
```cpp
static_assert(kResonanceBaseQ > 0.0);
static_assert(kResonanceScale > 0.0);
static_assert(kLfoPitchScale > 0.0);
// ... etc for all constants
```

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `src/core/DspConstants.h` | **NEW** | All DSP magic numbers as named constexpr values |
| `src/core/Voice.h` | **MODIFIED** | Replace 7 magic numbers with named constants |
| `src/core/Engine.h` | **MODIFIED** | Replace 6 magic numbers with named constants |

---

## Testing Instructions

```bash
just build                                          # Compiles cleanly
just test                                           # All tests pass
just check                                          # Lint + tests
python3 scripts/golden_master.py --verify           # CRITICAL: audio MUST be unchanged
```

### What a failure means
- **Golden master changed** → A constant value doesn't match the original magic number. Diff the constant against the original literal.
- **Compilation error** → Missing include of DspConstants.h or namespace qualification.

---

## Definition of Done

- [ ] `src/core/DspConstants.h` exists with ≥13 named constants
- [ ] Every constant has a one-line comment explaining its purpose
- [ ] Zero bare numeric literals remain in Voice::Process() (except for 0.0, 1.0, 2.0 which are structural)
- [ ] Zero bare numeric literals remain in Engine::SetChorus/SetDelay/SetLimiter/Init
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged (pure rename)
- [ ] All constant values exactly match their original magic numbers

---

## Common Pitfalls

1. **Changing a value accidentally** — e.g., writing `5.0` as `50.0`. Always verify the constant matches the original literal.
2. **Namespace issues** — If Voice.h uses `kResonanceBaseQ` without `PolySynthCore::`, it must be within the namespace block.
3. **Not all numbers are magic** — Leave 0.0, 1.0, 2.0 in arithmetic expressions. Only extract domain-specific scaling factors.
4. **Don't extract loop counters or array indices** — Only extract DSP tuning parameters.
