# DSP Refactor Plan v2: SEA_DSP Library Extraction

## Context

The original DSP refactor plan (`plans/active/dsp_refactor.md`) was attempted on branch `dsp-refactor-sea` and failed. A single WIP commit changed ~40 files simultaneously, introduced DaisySP facades with broken PIMPL, changed every API signature, and never compiled. The branch has been abandoned.

This plan takes a phased approach: **first port, then facade**. Phase A extracts the existing PolySynth DSP code into `libs/SEA_DSP/` with templated filters, platform abstraction, and the `sea::` namespace — using only proven, working algorithms. Phase B (future) replaces the oscillator/envelope/LFO internals with DaisySP facades once the architecture is stable and fully tested. Each step compiles, passes all tests, and is committed independently.

**Goal**: Build the SEA_DSP core library ("Side Effects Audio") as a reusable, header-only, dual-target (double/float) C++ library. TPT filters are templated (`template<typename T>`). Commodity DSP (oscillator, ADSR, LFO) is ported first, then upgraded to DaisySP facades in a later phase.

---

## Why the Previous Attempt Failed

1. **Big-bang**: 40 files changed in one commit. Nothing compiled.
2. **DaisySP + port simultaneously**: Introduced DaisySP facades for oscillator/envelope at the same time as the architecture change. Different APIs (gate-based ADSR, different waveform enums, missing `Reset()`). PIMPL with `alignas(16) uint8_t[64]` was fragile.
3. **No build system**: `libs/SE_DSP/CMakeLists.txt` was never created. Tests couldn't link.
4. **API contract breaks**: Every method renamed (`SetFrequency` -> `SetFreq`, `NoteOn`/`NoteOff` -> gate bool). VoiceManager had to be rewritten.
5. **No incremental testing**: No golden master comparison, no bit-exact verification between old and new.

---

## Key Design Decisions

- **D1: Header-only INTERFACE library.** All current DSP is header-only. SEA_DSP stays header-only (Phase A). CMake INTERFACE target.
- **D2: `template<typename T>` for filters.** TPT filters (LadderFilter, SVFilter, SKFilter, ClassicalFilter) are templated on the sample type. BiquadFilter also templated. This lets both `float` and `double` coexist in the same binary.
- **D3: `sea::Real` for non-templated components.** Oscillator, ADSR, LFO use `sea::Real` (compile-time `double`/`float` via `SEA_PLATFORM_EMBEDDED`). These will later become DaisySP facades.
- **D4: Port first, DaisySP facades later.** Phase A ports existing working code. Phase B (separate plan) introduces DaisySP with proper `FetchContent`, PIMPL, and API translation.
- **D5: Identical APIs during Phase A.** Every SEA_DSP class has the same method names, enums, and struct layouts as the original PolySynth code. Wrapper switchover is trivial.
- **D6: Wrappers, not VoiceManager changes.** Old headers become `using X = sea::X<double>;`. VoiceManager.h is never touched during Phase A.
- **D7: DaisySP isolation rule.** When DaisySP is added (Phase B), `daisysp` headers appear only in `.cpp` files. Public `sea_*.h` headers never expose DaisySP types. PIMPL or forward declarations enforce this.

---

## Phase A: Port & Template (This Plan)

### Step 0: Establish Golden Baseline

Verify the current build, tests, and golden master before touching anything.

**Actions**:
1. Build tests: `mkdir -p tests/build && cd tests/build && cmake .. && make -j$(nproc)`
2. Run `./run_tests` — all must pass
3. Run `python3 scripts/golden_master.py --generate` — regenerate golden WAVs
4. Run `python3 scripts/golden_master.py --verify` — confirm zero diff

**Commit**: `test: verify golden master baseline before DSP refactor`

**Gate**: All unit tests pass. Golden verify passes (tolerance 0.001). All demos produce WAVs.

---

### Step 1: SEA_DSP Library Skeleton + CMake

Create the library directory with a working CMake INTERFACE target and `sea_platform.h`.

**Create**:
- `libs/SEA_DSP/CMakeLists.txt` — INTERFACE library with `SEA_PLATFORM_EMBEDDED` option
- `libs/SEA_DSP/include/sea_dsp/sea_platform.h` — `sea::Real` (`double`/`float`), `sea::kPi`, `sea::kTwoPi`, `SEA_INLINE`

**Modify**:
- `tests/CMakeLists.txt` — add `add_subdirectory(../libs/SEA_DSP ...)`, link `SEA_DSP` to `run_tests` and all demo targets

**Test**: Add `tests/unit/Test_SEA_DSP_Platform.cpp` — compile check that `sea::Real` and `sea::kPi` work.

**Commit**: `build: add SEA_DSP library skeleton with platform abstraction`

**Gate**: Build succeeds. All tests pass (old + new). Golden verify passes.

---

### Step 2: Add `sea_math.h`

Math facade that all library code uses instead of `std::` directly.

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_math.h` — `sea::Math` struct with static methods: `Sin`, `Cos`, `Tan`, `Tanh`, `Abs`, `Sqrt`, `Clamp`, `Max`, `Min`, `Exp`, `Pow`, `Sinh`, `Cosh`, `Asinh`, `IsFinite`
- Desktop: forwards to `std::` functions
- `#ifdef SEA_FAST_MATH`: `Tanh` uses Pade approximation `x * (27 + x^2) / (27 + 9*x^2)`

**Test**: Add `tests/unit/Test_SEA_DSP_Math.cpp` — verify against `std::` reference values.

**Commit**: `feat(SEA_DSP): add sea_math.h with platform-aware math wrappers`

**Gate**: Build + all tests + golden verify.

---

### Step 3: Port TPTIntegrator as `template<typename T>`

The core building block for all VA filters.

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_tpt_integrator.h`

**Design**: `template<typename T> class TPTIntegrator` — all internal state (`g`, `s`, `mSampleRate`) becomes `T`. Uses `sea::Math::Tan`. API identical: `Init(T sampleRate)`, `Reset()`, `Prepare(T cutoff)`, `Process(T in)`, `GetG()`, `GetS()`, `SetS(T)`.

**Source**: Verbatim port of `src/core/dsp/va/TPTIntegrator.h` (84 lines) with `double` -> `T`, `std::tan` -> `sea::Math::Tan`, `std::clamp` -> `sea::Math::Clamp`.

**Test**: Add `tests/unit/Test_SEA_DSP_TPTIntegrator.cpp`:
- Instantiate both `PolySynthCore::TPTIntegrator` and `sea::TPTIntegrator<double>`
- Same params (SR=48000, cutoff=1000), feed 1000 samples of `sin(i*0.1)`
- Assert per-sample delta < 1e-14 (bit-exact for `double`)

**Commit**: `feat(SEA_DSP): port TPTIntegrator<T> with bit-exact verification`

**Gate**: Bit-exact test passes. All existing tests pass. Golden verify passes.

---

### Step 4: Port VASVFilter -> `sea::SVFilter<T>`

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_svf.h`

**Design**: `template<typename T> class SVFilter`. Internal `sea::TPTIntegrator<T>` members. `Outputs` struct with `T lp, bp, hp`. All methods (`Init`, `Reset`, `SetParams`, `Process`, `ProcessLP/BP/HP`) take and return `T`.

**Source**: Verbatim port of `src/core/dsp/va/VASVFilter.h` (104 lines).

**Test**: `tests/unit/Test_SEA_DSP_SVF.cpp` — bit-exact vs `PolySynthCore::VASVFilter` for 1000 samples.

**Commit**: `feat(SEA_DSP): port SVFilter<T> with bit-exact verification`

---

### Step 5: Port VALadderFilter -> `sea::LadderFilter<T>`

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_ladder_filter.h`

**Design**: `template<typename T> class LadderFilter`. `Model` enum preserved exactly (`Transistor`, `Diode`). `std::tanh` -> `sea::Math::Tanh` (critical — this is the embedded fast-math path). Internal `sea::TPTIntegrator<T> integrators[4]`.

**Source**: Verbatim port of `src/core/dsp/va/VALadderFilter.h` (172 lines).

**Test**: `tests/unit/Test_SEA_DSP_LadderFilter.cpp` — bit-exact for both models.

**Commit**: `feat(SEA_DSP): port LadderFilter<T> with bit-exact verification`

---

### Step 6: Port VAProphetFilter -> `sea::ProphetFilter<T>`

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_prophet_filter.h`

**Design**: `template<typename T> class ProphetFilter`. Internal `sea::SVFilter<T> stage1, stage2`. `Slope` enum preserved (`dB12`, `dB24`).

**Source**: Verbatim port of `src/core/dsp/va/VAProphetFilter.h` (74 lines).

**Test**: `tests/unit/Test_SEA_DSP_ProphetFilter.cpp` — bit-exact.

**Commit**: `feat(SEA_DSP): port ProphetFilter<T> with bit-exact verification`

---

### Step 7: Port BiquadFilter, VASKFilter, VAClassicalFilter

Three remaining filters in one step (small, isolated, lower risk).

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_biquad_filter.h` — `template<typename T> class BiquadFilter` + `sea::FilterType` enum (identical values: `LowPass`, `HighPass`, `BandPass`, `Notch`)
- `libs/SEA_DSP/include/sea_dsp/sea_sk_filter.h` — `template<typename T> class SKFilter`
- `libs/SEA_DSP/include/sea_dsp/sea_classical_filter.h` — `template<typename T> class ClassicalFilter` (uses `std::vector<sea::SVFilter<T>>` internally)

**Tests**: `tests/unit/Test_SEA_DSP_BiquadFilter.cpp`, `Test_SEA_DSP_SKFilter.cpp`, `Test_SEA_DSP_ClassicalFilter.cpp` — all bit-exact.

**Commit**: `feat(SEA_DSP): port BiquadFilter<T>, SKFilter<T>, ClassicalFilter<T> with bit-exact tests`

---

### Step 8: Port Oscillator (using `sea::Real`, not template)

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_oscillator.h`

**Design**: `class Oscillator` (not templated — uses `sea::Real`). `WaveformType` enum preserved exactly (`Saw`, `Square`, `Triangle`, `Sine`). API identical: `Init`, `Reset`, `SetFrequency`, `SetWaveform`, `SetPulseWidth`, `Process`. This class will be replaced with a DaisySP facade in Phase B.

**Source**: Verbatim port of `src/core/oscillator/Oscillator.h` (88 lines). `std::sin` -> `sea::Math::Sin`, `std::abs` -> `sea::Math::Abs`.

**Test**: `tests/unit/Test_SEA_DSP_Oscillator.cpp` — bit-exact for all 4 waveforms.

**Commit**: `feat(SEA_DSP): port Oscillator with bit-exact verification`

---

### Step 9: Port ADSREnvelope (using `sea::Real`, not template)

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_adsr.h`

**Design**: `class ADSREnvelope` using `sea::Real`. `Stage` enum preserved exactly (`kIdle`, `kAttack`, `kDecay`, `kSustain`, `kRelease`). API identical: `Init`, `Reset`, `SetParams(a,d,s,r)`, `NoteOn()`, `NoteOff()`, `Process()`, `IsActive()`, `GetLevel()`, `GetStage()`. Will become DaisySP facade in Phase B.

**Source**: Verbatim port of `src/core/modulation/ADSREnvelope.h` (143 lines).

**Test**: `tests/unit/Test_SEA_DSP_ADSR.cpp` — bit-exact.

**Commit**: `feat(SEA_DSP): port ADSREnvelope with bit-exact verification`

---

### Step 10: Port LFO (using `sea::Real`, not template)

**Create**:
- `libs/SEA_DSP/include/sea_dsp/sea_lfo.h`

**Design**: `class LFO` using `sea::Real`. Will become DaisySP facade in Phase B.

**Source**: Verbatim port of `src/core/modulation/LFO.h` (67 lines).

**Test**: `tests/unit/Test_SEA_DSP_LFO.cpp` — bit-exact for all 4 waveforms.

**Commit**: `feat(SEA_DSP): port LFO with bit-exact verification`

---

### Step 11: Wrapper Switchover

Replace the original in-tree DSP headers with thin wrappers that forward to SEA_DSP types. **VoiceManager.h and Engine.h are NOT modified.**

**Modify** (rewrite to wrappers):

- `src/core/types.h` — add `#include <sea_dsp/sea_platform.h>` and `static_assert(std::is_same_v<sample_t, sea::Real>)`. Keep existing definitions.

- `src/core/dsp/va/TPTIntegrator.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_tpt_integrator.h>
  namespace PolySynthCore { using TPTIntegrator = sea::TPTIntegrator<double>; }
  ```

- `src/core/dsp/va/VASVFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_svf.h>
  namespace PolySynthCore { using VASVFilter = sea::SVFilter<double>; }
  ```

- `src/core/dsp/va/VALadderFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_ladder_filter.h>
  namespace PolySynthCore { using VALadderFilter = sea::LadderFilter<double>; }
  ```

- `src/core/dsp/va/VAProphetFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_prophet_filter.h>
  namespace PolySynthCore { using VAProphetFilter = sea::ProphetFilter<double>; }
  ```

- `src/core/dsp/va/VASKFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_sk_filter.h>
  namespace PolySynthCore { using VASKFilter = sea::SKFilter<double>; }
  ```

- `src/core/dsp/va/VAClassicalFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_classical_filter.h>
  namespace PolySynthCore { using VAClassicalFilter = sea::ClassicalFilter<double>; }
  ```

- `src/core/dsp/BiquadFilter.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_biquad_filter.h>
  namespace PolySynthCore {
    using FilterType = sea::FilterType;
    using BiquadFilter = sea::BiquadFilter<double>;
  }
  ```

- `src/core/oscillator/Oscillator.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_oscillator.h>
  namespace PolySynthCore { using Oscillator = sea::Oscillator; }
  ```

- `src/core/modulation/ADSREnvelope.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_adsr.h>
  namespace PolySynthCore { using ADSREnvelope = sea::ADSREnvelope; }
  ```

- `src/core/modulation/LFO.h`:
  ```cpp
  #pragma once
  #include <sea_dsp/sea_lfo.h>
  namespace PolySynthCore { using LFO = sea::LFO; }
  ```

**Zero changes** to: `VoiceManager.h`, `Engine.h`, `SynthState.h`, `FXEngine.h`, any test file, any demo file.

**Verification**:
1. Build succeeds with zero changes to any consumer code
2. All unit tests pass
3. Golden master verify passes with `--tolerance 0.0` (bit-exact — same algorithm, same type)

**Commit**: `refactor: replace in-tree DSP headers with SEA_DSP wrapper aliases`

**Gate**: All tests pass. Golden verify tolerance=0.0. No consumer code touched.

---

### Step 12: SEA_DSP Standalone Test Suite

Create a self-contained test build for the library (no PolySynth dependency).

**Create**:
- `libs/SEA_DSP/tests/CMakeLists.txt` — downloads Catch2, builds `sea_dsp_tests`
- `libs/SEA_DSP/tests/test_main.cpp`
- `libs/SEA_DSP/tests/test_all.cpp` — exercises every component in isolation (both `<double>` and `<float>` instantiations for templated filters)

**Modify**:
- `libs/SEA_DSP/CMakeLists.txt` — add `SEA_DSP_BUILD_TESTS` option

**Commit**: `test(SEA_DSP): add standalone test suite for library validation`

**Gate**: Standalone tests pass (both double and float). PolySynth tests still pass. Golden verify still passes.

---

### Step 13: Cleanup + Docs

- Add `libs/SEA_DSP/README.md` documenting build flags (`SEA_PLATFORM_EMBEDDED`, `SEA_FAST_MATH`), dual-target philosophy, and API reference
- Add Doxygen-style comments to all public SEA_DSP headers
- Save this plan as `plans/dsp-refactor-v2.md` in the repo

**Commit**: `docs: document SEA_DSP library and dual-target architecture`

---

## Phase B: DaisySP Facades (Future Plan — Not This PR)

Once Phase A is complete and stable, a separate plan will cover:

1. **Add DaisySP via CMake FetchContent** — pinned to specific commit
2. **`sea::Oscillator` facade** — PIMPL wrapping `daisysp::BlOsc`, `.cpp` file hides DaisySP headers. Library becomes STATIC (not INTERFACE) to support .cpp files.
3. **`sea::ADSREnvelope` facade** — PIMPL wrapping `daisysp::Adsr`, translate gate-based API to NoteOn/NoteOff API
4. **`sea::LFO` facade** — wrap DaisySP LFO
5. **Each facade gets its own commit** with golden master regression tests
6. **DaisySP isolation enforced**: only `sea_*.cpp` files include `daisysp.h`, public headers never expose it

This phased approach means if DaisySP facades cause issues, we can always fall back to the Phase A implementations.

---

## Dependency Graph

```
Step 0 (baseline)
  |
Step 1 (skeleton + CMake)
  |
Step 2 (sea_math.h)
  |
Step 3 (TPTIntegrator<T>)
  |--- Step 4 (SVFilter<T>) --- Step 6 (ProphetFilter<T>)
  |                          \-- Step 7 (ClassicalFilter<T>)
  |--- Step 5 (LadderFilter<T>)
  |--- Step 7 (SKFilter<T>)
  |
Step 7 (BiquadFilter<T> — standalone)
Step 8 (Oscillator — depends on Step 2)
Step 9 (ADSREnvelope — depends on Step 2)
Step 10 (LFO — depends on Step 2)
  |
Step 11 (Wrapper switchover — depends on ALL of 3-10)
  |
Step 12 (Standalone tests)
  |
Step 13 (Cleanup + docs)
  |
--- Phase B (future): DaisySP facades ---
```

Steps 3-10 can be done in any order respecting dependency arrows. The critical path is: 0 -> 1 -> 2 -> 3 -> 4 -> 6 -> 11.

---

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Template instantiation errors at wrapper step | Bit-exact tests in Steps 3-7 verify `<double>` works. Step 12 also tests `<float>`. |
| Enum name mismatch (`Model`, `Slope`, `FilterType`, `WaveformType`, `Stage`) | Each port preserves enum names verbatim. Tests verify API compatibility. |
| `sample_t` vs `sea::Real` type divergence | `static_assert` in `types.h` (Step 11) catches at compile time. |
| `FilterType` used outside `BiquadFilter.h` | Wrapper aliases both `FilterType` and `BiquadFilter` in `PolySynthCore` namespace. Used at `VoiceManager.h:33,151` and test files. |
| `Outputs` struct field names | Preserved exactly: `lp`, `bp`, `hp` (type becomes `T` but names identical). |
| `VAClassicalFilter` uses `std::vector<VASVFilter>` | Sea version uses `std::vector<sea::SVFilter<T>>`. Wrapper `using` makes this transparent. |
| Golden master tolerance | Desktop `double` mode: algorithms identical, tolerance 0.0 is valid at Step 11. |

---

## Verification Protocol (Every Step)

```bash
cd tests/build && cmake .. && make -j$(nproc)
./run_tests                                    # All unit tests pass
python3 ../../scripts/golden_master.py --verify # Audio output unchanged
```

After Step 11 specifically, also verify with `--tolerance 0.0`.

---

## Files Summary

**Created (Steps 1-13):**
- `libs/SEA_DSP/CMakeLists.txt`
- `libs/SEA_DSP/include/sea_dsp/sea_platform.h`
- `libs/SEA_DSP/include/sea_dsp/sea_math.h`
- `libs/SEA_DSP/include/sea_dsp/sea_tpt_integrator.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_svf.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_ladder_filter.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_prophet_filter.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_biquad_filter.h` — `template<typename T>` + `sea::FilterType`
- `libs/SEA_DSP/include/sea_dsp/sea_sk_filter.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_classical_filter.h` — `template<typename T>`
- `libs/SEA_DSP/include/sea_dsp/sea_oscillator.h` — uses `sea::Real`
- `libs/SEA_DSP/include/sea_dsp/sea_adsr.h` — uses `sea::Real`
- `libs/SEA_DSP/include/sea_dsp/sea_lfo.h` — uses `sea::Real`
- `libs/SEA_DSP/tests/CMakeLists.txt`
- `libs/SEA_DSP/tests/test_main.cpp`
- `libs/SEA_DSP/tests/test_all.cpp`
- `libs/SEA_DSP/README.md`
- `tests/unit/Test_SEA_DSP_Platform.cpp`
- `tests/unit/Test_SEA_DSP_Math.cpp`
- `tests/unit/Test_SEA_DSP_TPTIntegrator.cpp`
- `tests/unit/Test_SEA_DSP_SVF.cpp`
- `tests/unit/Test_SEA_DSP_LadderFilter.cpp`
- `tests/unit/Test_SEA_DSP_ProphetFilter.cpp`
- `tests/unit/Test_SEA_DSP_BiquadFilter.cpp`
- `tests/unit/Test_SEA_DSP_SKFilter.cpp`
- `tests/unit/Test_SEA_DSP_ClassicalFilter.cpp`
- `tests/unit/Test_SEA_DSP_Oscillator.cpp`
- `tests/unit/Test_SEA_DSP_ADSR.cpp`
- `tests/unit/Test_SEA_DSP_LFO.cpp`
- `plans/dsp-refactor-v2.md`

**Modified (Steps 1, 11):**
- `tests/CMakeLists.txt` (Step 1: add SEA_DSP subdirectory and linking)
- `src/core/types.h` (Step 11: add static_assert)
- `src/core/dsp/va/TPTIntegrator.h` (Step 11: wrapper -> `sea::TPTIntegrator<double>`)
- `src/core/dsp/va/VASVFilter.h` (Step 11: wrapper -> `sea::SVFilter<double>`)
- `src/core/dsp/va/VALadderFilter.h` (Step 11: wrapper -> `sea::LadderFilter<double>`)
- `src/core/dsp/va/VAProphetFilter.h` (Step 11: wrapper -> `sea::ProphetFilter<double>`)
- `src/core/dsp/va/VASKFilter.h` (Step 11: wrapper -> `sea::SKFilter<double>`)
- `src/core/dsp/va/VAClassicalFilter.h` (Step 11: wrapper -> `sea::ClassicalFilter<double>`)
- `src/core/dsp/BiquadFilter.h` (Step 11: wrapper -> `sea::BiquadFilter<double>` + `sea::FilterType`)
- `src/core/oscillator/Oscillator.h` (Step 11: wrapper -> `sea::Oscillator`)
- `src/core/modulation/ADSREnvelope.h` (Step 11: wrapper -> `sea::ADSREnvelope`)
- `src/core/modulation/LFO.h` (Step 11: wrapper -> `sea::LFO`)

**Never modified in Phase A:**
- `src/core/VoiceManager.h`
- `src/core/Engine.h`
- `src/core/SynthState.h`
- `src/core/dsp/fx/FXEngine.h`
- Any existing test file
- Any demo file
