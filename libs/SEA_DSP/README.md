# SEA_DSP

`SEA_DSP` (Side Effects Audio DSP) is a standalone, header-only C++ DSP library.
It is designed for predictable behavior, portability, and incremental evolution.

## Overview

- Header-only library exposed as a CMake `INTERFACE` target
- Public API under the `sea::` namespace
- Dual-target numeric model:
  - desktop/default: `sea::Real = double`
  - embedded: `sea::Real = float` via `SEA_PLATFORM_EMBEDDED`
- Focus on stable DSP primitives that are easy to test and reuse

## Included Components

Public headers are under `include/sea_dsp/`.

- Platform and math:
  - `sea_platform.h`
  - `sea_math.h`
- Filters and filter building blocks:
  - `sea_tpt_integrator.h`
  - `sea_svf.h`
  - `sea_ladder_filter.h`
  - `sea_cascade_filter.h`
  - `sea_biquad_filter.h`
  - `sea_sk_filter.h`
  - `sea_classical_filter.h`
- Modulation and oscillation:
  - `sea_oscillator.h`
  - `sea_adsr.h`
  - `sea_lfo.h`

## Design Philosophy

- Keep architecture simple: headers only, explicit types, minimal hidden state
- Prefer incremental, test-backed changes over large rewrites
- Isolate platform precision decisions behind `sea::Real`
- Keep APIs direct and practical for real-time audio use
- Preserve deterministic behavior as a baseline before optimization work

## Platforms and Build Modes

- Default mode (`double` precision) for desktop-class builds
- Embedded mode (`float` precision) by defining `SEA_PLATFORM_EMBEDDED`
- Optional fast-math hook (`SEA_FAST_MATH`) is available in `sea_math.h`

## Dependencies

Core library:

- C++17
- CMake 3.15+
- C++ standard library headers only

Tests:

- Catch2 single-header (`catch.hpp`), expected at `external/catch2/catch.hpp`

## Build and Test

Build and run the standalone SEA_DSP tests:

```bash
cmake -S libs/SEA_DSP -B build/sea_dsp_tests -DSEA_BUILD_TESTS=ON
cmake --build build/sea_dsp_tests
./build/sea_dsp_tests/tests/sea_tests
```

## Test Coverage (Current)

Test sources live in `libs/SEA_DSP/tests/` and currently cover:

- platform typing and constants (`Test_Platform.cpp`)
- math wrapper behavior (`Test_Math.cpp`)
- oscillator waveform behavior (`Test_Oscillator.cpp`)
- filter and integrator behavior checks (`Test_Filters.cpp`)
- ADSR stage/lifecycle behavior (`Test_ADSR.cpp`)
- LFO behavior (`Test_LFO.cpp`)

Current suite size: 9 test cases, 45 assertions.

## CI

The repository CI runs the SEA_DSP suite by building and executing `sea_tests`
as a dedicated workflow step.

## Notes

- `sea_classical_filter.h` currently uses vector-backed stage storage and includes
  a TODO for embedded-focused allocation optimization.
