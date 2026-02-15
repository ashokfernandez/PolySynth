# Architecture Review & Improvement Proposal

*Written after PR #34/35 fixes. Full codebase explored.*

This is a prioritised set of structural changes to harden the PolySynth codebase as it grows. The legacy React web UI is slated for removal and excluded from this analysis.

---

## 1. What's Working Well

**The core DSP layer is clean.** `SynthState`, `VoiceManager`, `Engine`, and `PresetManager` form a strict dependency chain with no cycles. `SynthState` is a plain aggregate guarded by `static_assert(is_aggregate_v)`, and `Reset()` uses the `*this = SynthState{}` pattern. SEA_DSP is fully independent.

**The audio path is real-time safe.** `ProcessBlock` makes zero heap allocations, throws no exceptions, and acquires no locks. Effects are pre-allocated and reset on sample-rate change.

**The test infrastructure has teeth.** 22 Catch2 unit tests, 17 WAV-generating demos, golden master RMS comparison (0.001 tolerance), SEA_DSP's own test suite, and Playwright visual regression. The round-trip test in `Test_SynthState.cpp` already caught a missing `filterModel` serialisation.

**The CI pipeline is comprehensive.** Native DSP tests, WAM Emscripten build, Storybook build, and GitHub Pages deployment all run on every PR.

---

## 2. Where the Architecture is Fragile

### 2.1 The Platform Layer is a Monolith
`PolySynth.cpp` does five distinct jobs. Every new parameter requires changes in four places in this file plus two in `PresetManager.cpp`. No compile-time enforcement keeps all six sites in sync.

### 2.2 PolySynth_DSP.h Duplicates Engine.h
Both wrap VoiceManager + Chorus + Delay + Limiter. They diverge in minor ways. Fix a bug in one, forget the other.

### 2.3 Magic Numbers in DSP Code
Resonance curve (`0.5 + res*res*20.0`), LFO pitch scaling (`0.05`), headroom (`0.25`), and the hardcoded `44100.0` in `VoiceManager::Reset()` are embedded without constants or comments.

### 2.4 Declared-But-Unused SynthState Fields
`mixNoise`, `oscASync`, `oscBLoFreq`, `unisonDetune`, `glideTime`, and `filterKeyboardTrack` are serialised but never read by `Voice::Process()`.

### 2.5 Testing Gaps
No unit tests for: Voice in isolation, LFO routing, filter model switching, parameter boundaries, or NaN/Inf propagation.

---

## 3. Proposed Changes (Prioritised)

### Priority 1: Eliminate PolySynth_DSP.h -- Use Engine Directly

Delete `PolySynth_DSP.h`. Make `PolySynthPlugin` own a `PolySynthCore::Engine` directly. Add `Engine::UpdateState(const SynthState&)` as the single translation point from state to DSP. Engine already has the FX setters and both Process overloads.

**Files:** Delete `PolySynth_DSP.h`. Modify `PolySynth.h`, `Engine.h`, `PolySynth.cpp`.

### Priority 2: Centralise Parameter Metadata

Create `PolySynthParams.h` with a `constexpr` array of `ParamMeta` structs (id, name, default, min, max, display multiplier, unit). Make `OnParamChange`, `SyncUIState`, and the constructor table-driven. A unit test verifies every enum value is present and round-trips correctly.

### Priority 3: Audit Unimplemented Fields

Add a comment block listing unimplemented fields. Add a "no-effect" test: set field to extreme value, render audio, compare to default -- assert identical. When someone wires the feature, the test fails as a reminder.

### Priority 4: Extract Voice Into Its Own Header

Split `VoiceManager.h` into `Voice.h` and `VoiceManager.h`. Both stay header-only for performance. Enables targeted tests: filter model behaviour, poly-mod routing, LFO routing, envelope gating.

### Priority 5: Extract Magic Numbers Into Named Constants

Define `constexpr` values: `kResonanceBaseQ`, `kResonanceScale`, `kLfoPitchScale`, `kPwmModScale`, `kVoiceHeadroomScale`, etc. Fix `VoiceManager::Reset()` to accept a sample rate parameter.

### Priority 6: Strengthen the Test Pyramid

- Filter model switching (distinct spectral characteristics)
- Parameter boundaries (no NaN/Inf at extremes)
- Voice stealing stress (16 notes into 8 voices)
- Preset versioning (backward compatibility)

---

## 4. What Not to Change

- **Header-only Voice/VoiceManager** -- performance-critical per-sample code benefits from inlining
- **Strict golden master tolerance** -- 0.001 RMS is the canary; regenerate masters, don't loosen
- **iPlug2 build system** -- it works; CMake is the parallel path for unit tests

---

## 5. Execution Order

1. PolySynth_DSP.h elimination (highest impact)
2. Voice.h extraction (pure refactor, unblocks tests)
3. Magic number extraction (pure refactor, fixes VoiceManager::Reset bug)
4. Unimplemented field audit (documentation + novel test pattern)
5. Parameter metadata centralisation (largest refactor, do last)
6. Test pyramid strengthening (ongoing)

Each is independently mergeable. Golden masters verified after each merge.
