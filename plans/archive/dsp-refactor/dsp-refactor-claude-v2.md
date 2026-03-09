# DSP Refactor Plan v2: SEA_DSP Library Extraction

## Context

The original DSP refactor plan (`plans/active/dsp_refactor.md`) was attempted on branch `dsp-refactor-sea` and failed due to a "big-bang" approach that changed too many things at once (architecture, API, and implementation). This new plan uses a strict **port-then-modify** strategy to ensure bit-exactness and stability.

**Core Objective**: Extract the existing DSP code from `PolySynth` into a new, reusable, header-only library called **`SEA_DSP`** (Side Effects Audio). This library will support a "Dual-Target" build (Desktop `double` / Embedded `float`).

**Strict Constraints**:
*   **Zero Logic Changes**: The refactor must be invisible to the consumer (`VoiceManager.h`, `Engine.h`, `Voice.h` must remain untouched in logic/API).
*   **Bit-Exactness**: Verification against "Golden Master" WAV files with **0.0** tolerance.
*   **No Allocation**: No `malloc`, `new`, or `std::vector` resizing inside `Process()`. (Exception: `VAClassicalFilter` existing vector usage preserved with TODO).
*   **Forward Declarations**: All forward declarations of DSP types must be replaced with `#include`.

---

## Architecture & Design

*   **Library**: `libs/SEA_DSP` (CMake `INTERFACE` library).
*   **Namespace**: `sea::`.
*   **Platform**: `sea_platform.h` defines `sea::Real` (`double` by default, `float` if `SEA_PLATFORM_EMBEDDED` is defined).
*   **Math**: `sea_math.h` wraps standard math (e.g., `sea::Math::Tan`) to allow for optimized approximations on embedded platforms (`SEA_FAST_MATH`).

### Porting Strategy (Phase A)

1.  **Templates for TPT Filters**: `LadderFilter`, `SVFilter`, `ProphetFilter`, `SKFilter`, `BiquadFilter`, `ClassicalFilter` will be converted to `template <typename T>` classes.
2.  **Aliases for Commodity DSP**: `Oscillator`, `ADSREnvelope`, `LFO` will use `sea::Real` and keep their **exact** original API (enums, method names) to ensure seamless switchover via `using` aliases.

---

## Phase A: Port & Template (Execution Steps)

### Step 0: Establish Golden Baseline
*   Build current main branch.
*   Run tests: `./run_tests`.
*   Generate Golden Master: `python3 scripts/golden_master.py --generate`.
*   Verify Golden Master: `python3 scripts/golden_master.py --verify`.
*   **Gate**: All tests pass, verify passes with 0.0 tolerance.

### Step 1: SEA_DSP Library Skeleton
*   Create `libs/SEA_DSP/CMakeLists.txt` (INTERFACE target).
*   Create `libs/SEA_DSP/include/sea_dsp/sea_platform.h`:
    *   Define `sea::Real` based on `SEA_PLATFORM_EMBEDDED`.
    *   Define constants `sea::kPi`, `sea::kTwoPi`.
*   Update `tests/CMakeLists.txt` to link `SEA_DSP`.
*   Add unit test `tests/unit/Test_SEA_DSP_Platform.cpp`.

### Step 2: Math Abstraction (`sea_math.h`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_math.h`.
*   Implement `sea::Math` struct with static methods (`Sin`, `Cos`, `Tan`, `Tanh`, `Exp`, `Pow`, `Abs`, `Clamp`, etc.).
*   Desktop: Forward to `std::` functions.
*   Embedded/Fast: Use approximations if `SEA_FAST_MATH` is set (e.g. Pade approximation for Tanh).
*   Add unit test `tests/unit/Test_SEA_DSP_Math.cpp`.

### Step 3: Port TPTIntegrator (`template<typename T>`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_tpt_integrator.h`.
*   Port `src/core/dsp/va/TPTIntegrator.h` to `sea::TPTIntegrator<T>`.
*   Replace `double` with `T`, `std::tan` with `sea::Math::Tan`.
*   **Test**: `tests/unit/Test_SEA_DSP_TPTIntegrator.cpp` (instantiate `<double>` and compare bit-exact with original).

### Step 4: Port SVFilter (`template<typename T>`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_svf.h`.
*   Port `src/core/dsp/va/VASVFilter.h` to `sea::SVFilter<T>`.
*   Use `sea::TPTIntegrator<T>`.
*   **Test**: `tests/unit/Test_SEA_DSP_SVF.cpp` (bit-exact check).

### Step 5: Port LadderFilter (`template<typename T>`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_ladder_filter.h`.
*   Port `src/core/dsp/va/VALadderFilter.h` to `sea::LadderFilter<T>`.
*   Replace `std::tanh` with `sea::Math::Tanh`.
*   **Test**: `tests/unit/Test_SEA_DSP_LadderFilter.cpp` (bit-exact check).

### Step 6: Port ProphetFilter (`template<typename T>`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_prophet_filter.h`.
*   Port `src/core/dsp/va/VAProphetFilter.h` to `sea::ProphetFilter<T>`.
*   **Test**: `tests/unit/Test_SEA_DSP_ProphetFilter.cpp` (bit-exact check).

### Step 7: Port Remaining Filters (`template<typename T>`)
*   **BiquadFilter**: `libs/SEA_DSP/include/sea_dsp/sea_biquad_filter.h` (`sea::BiquadFilter<T>`, `sea::FilterType`).
*   **SKFilter**: `libs/SEA_DSP/include/sea_dsp/sea_sk_filter.h` (`sea::SKFilter<T>`).
*   **ClassicalFilter**: `libs/SEA_DSP/include/sea_dsp/sea_classical_filter.h` (`sea::ClassicalFilter<T>`).
    *   *Note*: Preserve `std::vector` usage for now, but mark with `// TODO: Optimize for embedded (remove allocation)` per constraints.
*   **Tests**: Unit tests for all, checking bit-exactness.

### Step 8: Port Oscillator (`sea::Real`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_oscillator.h`.
*   Port `src/core/oscillator/Oscillator.h` to `sea::Oscillator`.
*   **NOT Templated**: Use `sea::Real` directly.
*   Preserve `WaveformType` enum and all method signatures EXACTLY.
*   Replace `std::sin` -> `sea::Math::Sin`.
*   **Test**: `tests/unit/Test_SEA_DSP_Oscillator.cpp`.

### Step 9: Port ADSREnvelope (`sea::Real`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_adsr.h`.
*   Port `src/core/modulation/ADSREnvelope.h` to `sea::ADSREnvelope`.
*   Preserve `Stage` enum and API.
*   **Test**: `tests/unit/Test_SEA_DSP_ADSR.cpp`.

### Step 10: Port LFO (`sea::Real`)
*   Create `libs/SEA_DSP/include/sea_dsp/sea_lfo.h`.
*   Port `src/core/modulation/LFO.h` to `sea::LFO`.
*   **Test**: `tests/unit/Test_SEA_DSP_LFO.cpp`.

### Step 11: Wrapper Switchover & Forward Decl Cleanup
*   **Objective**: Replace original DSP implementations with wrappers around `sea::` types.
*   **Modify `src/core/types.h`**: Add `static_assert(std::is_same_v<sample_t, sea::Real>)`.
*   **Modify DSP Headers** (`TPTIntegrator.h`, `Oscillator.h`, etc.):
    *   Delete implementation code.
    *   `#include <sea_dsp/sea_*.h>`
    *   `using ClassName = sea::ClassName<double>;` (for templates) or `using ClassName = sea::ClassName;` (for others).
*   **Clean Forward Declarations**:
    *   Scan `src/core` for any forward declarations of DSP classes (e.g., `class Oscillator;`).
    *   Replace them with `#include "oscillator/Oscillator.h"` (which now contains the alias).
    *   *Reason*: You cannot forward declare a type alias.
*   **Verification**:
    *   Compile `PolySynth`.
    *   Run `scripts/golden_master.py --verify`.
    *   **Requirement**: Tolerance must be **0.0**.

### Step 12: Standalone Tests & Cleanup
*   Create standalone test suite in `libs/SEA_DSP/tests`.
*   Add `README.md` to `libs/SEA_DSP`.
*   Document build flags (`SEA_PLATFORM_EMBEDDED`).

---

## Reviewer Checklist for PRs

*   [ ] **Directory Structure**: Is `libs/SEA_DSP` an `INTERFACE` library?
*   [ ] **Namespace**: Is everything in `namespace sea`?
*   [ ] **Bit-Exact**: Did Golden Master pass with `0.0` tolerance?
*   [ ] **No Header Changes**: Are `VoiceManager.h` and `Engine.h` untouched (except potentially removing forward decls)?
*   [ ] **Allocation**: No `new`/`malloc` in `Process`? (ClassicalFilter TODO present?)
*   [ ] **Forward Declarations**: All removed and replaced with includes?
