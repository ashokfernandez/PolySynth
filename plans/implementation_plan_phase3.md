# Implementation Plan: Phase 3 Testing & ADSR

## User Review Required
> [!IMPORTANT]
> This plan restructures the `tests/` directory. The existing `run_tests` and `render_demo` targets will be moved/renamed.

## Proposed Changes

### Tests Restructure
Move files to clearer subdirectories:
*   `tests/unit/`: Catch2 tests (`Test_*.cpp`).
*   `tests/demos/`: Standalone demo executables (`demo_*.cpp`).
*   `tests/utils/`: Shared helpers (`WavWriter.h`).

### ADSR Implementation
*   **Source**: `src/core/modulation/ADSREnvelope.h` (Already drafted, needs refinement).
*   **Unit Test**: `tests/unit/Test_ADSREnvelope.cpp`
    *   TestCase: "Attack Phase increments correctly"
    *   TestCase: "Sustain level holds"
    *   TestCase: "Release phase decrements"
*   **Demo**: `tests/demos/demo_adsr.cpp`
    *   Renders a 2-second CSV/WAV file showing the envelope shape for A=0.5, D=0.5, S=0.5.

### Voice Integration
*   Modify `Voice.h` to include `ADSREnvelope mAmpEnv`.
*   NoteOn triggers `mAmpEnv.NoteOn()`.
*   NoteOff triggers `mAmpEnv.NoteOff()`.
*   Process multiplies `Osc.Process() * mAmpEnv.Process()`.

### Documentation & Audio Artifacts
*   **Goal**: auto-generate a static site where we can listen to the latest build.
*   **Tools**: Python script `scripts/generate_test_report.py`.
*   **Workflow**:
    1.  CI runs `tests/demos/*`.
    2.  Demos output `.wav` files to `docs/audio/`.
    3.  Script generates `docs/index.md` listing each demo with an `<audio>` tag.
    4.  GitHub Actions deploys `docs/` to GitHub Pages.

## Verification Plan

### Automated Tests
Run `cd polysynth/tests/build && ctest` (or `make test`).
*   Expected: All unit tests pass.

### Manual Verification (Demos)
Run `cd polysynth/tests/build && ./demo_adsr`
*   Output: `adsr_demo.csv` -> Verify shape in plotting tool (or just visual check of values).
Run `cd polysynth/tests/build && ./demo_engine_poly`
*   Output: `engine_demo.wav` -> Listen. Should hear a fade-in (Attack) and fade-out (Release), unlike the previous hard-gated beep.
