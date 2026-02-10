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
- Target platform option:
  - `SEA_DSP_TARGET_PLATFORM=desktop` (default)
  - `SEA_DSP_TARGET_PLATFORM=daisy_mcu` (enables embedded precision path)

## Backend Configuration

SEA_DSP supports per-component backend selection for basics:

- `SEA_DSP_OSC_BACKEND=sea_core|daisysp`
- `SEA_DSP_ADSR_BACKEND=sea_core|daisysp`
- `SEA_DSP_LFO_BACKEND=sea_core|daisysp`

This allows mixed builds (for example, TPT filters in `sea_core` while
oscillator/modulation comes from `daisysp`).

## Dependencies

Core library:

- C++17
- CMake 3.15+
- `sea_core` path: C++ standard library headers only
- `daisysp` path (optional): DaisySP source in `external/daisysp`

Tests:

- Catch2 single-header (`catch.hpp`)
  - If not found at `external/catch2/catch.hpp`, SEA_DSP test CMake downloads
    Catch2 v2.13.9 into the test build directory automatically

## Build and Test

Build and run the standalone SEA_DSP tests:

```bash
cmake -S libs/SEA_DSP -B build/sea_dsp_tests -DSEA_BUILD_TESTS=ON
cmake --build build/sea_dsp_tests
./build/sea_dsp_tests/tests/sea_tests
```

Run with explicit backend/platform selection:

```bash
cmake -S libs/SEA_DSP -B build/sea_dsp_tests_daisysp \
  -DSEA_BUILD_TESTS=ON \
  -DSEA_DSP_TARGET_PLATFORM=daisy_mcu \
  -DSEA_DSP_OSC_BACKEND=daisysp \
  -DSEA_DSP_ADSR_BACKEND=daisysp \
  -DSEA_DSP_LFO_BACKEND=daisysp
cmake --build build/sea_dsp_tests_daisysp
./build/sea_dsp_tests_daisysp/tests/sea_tests
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

## Current TODOs

- Embedded performance pass:
  - remove or replace dynamic allocations in `sea_classical_filter.h`
  - review `SEA_FAST_MATH` approximations and error bounds before enabling by default
- Coverage expansion:
  - add stronger response/accuracy tests for all filter variants
  - add behavior parity tests for both `double` and `float` paths
- Packaging polish:
  - provide a clean external-consumer integration example
  - document versioning and API stability policy

## Roadmap

Short-term:

- keep the current stable, header-only core as the baseline
- improve tests and embedded readiness without changing public API semantics

Mid-term:

- add optional facade/adaptor layers for alternative DSP backends where needed
- keep public `sea_*` headers backend-agnostic

DaisySP status:

- DaisySP is now supported as an optional backend for oscillator/ADSR/LFO
- filters remain on the `sea_core` implementation path
- backend mixing is supported through per-component backend options

## Notes

- `sea_classical_filter.h` currently uses vector-backed stage storage and includes
  a TODO for embedded-focused allocation optimization.
