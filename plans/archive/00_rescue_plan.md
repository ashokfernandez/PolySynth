# Rescue Implementation Plan (Phase 0)

This plan addresses immediate build failures preventing any further development.
The build is currently blocked by namespace mismatches in the test suite.

## Diagnosis
The `BiquadFilter` class is defined in:
*   File: `src/core/dsp/BiquadFilter.h`
*   Namespace: `PolySynthCore`

The Unit Test tries to use it as:
*   File: `tests/unit/Test_BiquadFilter.cpp`
*   Namespace: `PolySynth` (which does not exist)

## Validation Steps
1.  **Build**: Run `cd polysynth/tests/build && cmake .. && make`.
2.  **Verify**: Ensure `[100%] Built target run_tests` appears.
3.  **Run**: `./run_tests` and ensure all tests pass.

## Proposed Changes

### [MODIFY] [Test_BiquadFilter.cpp](file:///Users/ashokfernandez/Software/PolySynth/polysynth/tests/unit/Test_BiquadFilter.cpp)
*   Update namespace usage from `PolySynth::BiquadFilter` to `PolySynthCore::BiquadFilter`.
*   Update enum usage from `PolySynthCore::FilterType` (this was actually correct) to match.

### [CHECK] [types.h](file:///Users/ashokfernandez/Software/PolySynth/polysynth/src/core/types.h)
*   Verify it compiles standalone. (Confirmed: only depends on `<stdint.h>`).
