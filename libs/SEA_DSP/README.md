# SEA_DSP

`SEA_DSP` (Side Effects Audio DSP) is a reusable DSP library extracted from PolySynth.
It is designed to keep proven DSP behavior while making the code easier to reuse, test,
and target across desktop and embedded builds.

## What This Library Is

- Header-only C++ library (`INTERFACE` CMake target)
- Namespace-scoped API under `sea::`
- Dual-target numeric model:
  - Desktop/default: `sea::Real = double`
  - Embedded mode: `sea::Real = float` via `SEA_PLATFORM_EMBEDDED`
- A migration layer for PolySynth DSP code that preserves existing behavior and APIs

The main intent is incremental, low-risk refactoring: extract and validate first,
then optimize or swap internals later.

## What Is Included

Public headers are in `/Users/ashokfernandez/Software/PolySynth/libs/SEA_DSP/include/sea_dsp`.

- Platform and math:
  - `sea_platform.h` (`sea::Real`, constants, `SEA_INLINE`)
  - `sea_math.h` math wrapper helpers
- Core DSP components:
  - `sea_tpt_integrator.h`
  - `sea_svf.h`
  - `sea_ladder_filter.h`
  - `sea_prophet_filter.h`
  - `sea_biquad_filter.h`
  - `sea_sk_filter.h`
  - `sea_classical_filter.h`
  - `sea_oscillator.h`
  - `sea_adsr.h`
  - `sea_lfo.h`

## Design Philosophy

The library was created out of the DSP refactor effort documented in:

- `/Users/ashokfernandez/Software/PolySynth/.claude/worktrees/nice-kapitsa/plans/dsp-refactor-claude.md`
- `/Users/ashokfernandez/Software/PolySynth/plans/dsp-refactor-claude-v2.md`

Key principles:

- Keep changes incremental and testable (avoid big-bang rewrites)
- Preserve working DSP behavior while extracting to a reusable module
- Separate platform concerns (`double`/`float`) behind `sea::Real`
- Keep DSP headers standalone and easy to include
- Validate continuously with unit tests and CI

## Platforms Supported

- Desktop builds (default): `double` precision path
- Embedded-oriented builds: define `SEA_PLATFORM_EMBEDDED` for `float` precision path

The current repository CI runs SEA_DSP unit tests on GitHub Actions (`ubuntu-latest`).

## Dependencies

Core library:

- No external runtime dependency beyond the C++ standard library headers
- C++17
- CMake 3.15+

Tests:

- Catch2 single-header (`catch.hpp`) from `external/catch2/catch.hpp`
- Built when `SEA_BUILD_TESTS=ON`

## Building and Running SEA_DSP Tests

From repository root:

```bash
cmake -S libs/SEA_DSP -B build/sea_dsp_tests -DSEA_BUILD_TESTS=ON
cmake --build build/sea_dsp_tests
./build/sea_dsp_tests/tests/sea_tests
```

Expected current result: all test cases pass.

## Test Coverage (Current)

Standalone SEA_DSP tests live in
`/Users/ashokfernandez/Software/PolySynth/libs/SEA_DSP/tests` and cover:

- Platform typing/constants (`Test_Platform.cpp`)
- Math wrapper behavior (`Test_Math.cpp`)
- Oscillator behavior and waveform checks (`Test_Oscillator.cpp`)
- Filter/integrator stability and response checks (`Test_Filters.cpp`)
- ADSR lifecycle behavior (`Test_ADSR.cpp`)
- LFO behavior (`Test_LFO.cpp`)

Current suite size: 9 test cases, 45 assertions.

## CI Integration

SEA_DSP tests are integrated into the main CI workflow:

- `/Users/ashokfernandez/Software/PolySynth/.github/workflows/ci.yml`

The workflow builds and runs `sea_tests` in a dedicated step.

## Notes

- `SEA_FAST_MATH` hooks exist in `sea_math.h`; current implementation favors correctness.
- `sea_classical_filter.h` retains vector-based stage storage with an embedded optimization TODO.

