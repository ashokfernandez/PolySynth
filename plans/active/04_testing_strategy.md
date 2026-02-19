# 04. Testing & CI Strategy

## 1. The Strategy: "Trust but Verify"
Since we cannot easily "listen" to every commit, we rely on a 3-tiered automated strategy.

### Tier 1: Unit Tests (Fast)
*   **Tool**: Catch2.
*   **Scope**: DSP Math, Envelope Logic, Voice Stealing Logic.
*   **Examples**:
    *   `Test_Envelope.cpp`: "Check that Attack phase moves from 0.0 to 1.0 in correct time."
    *   `Test_Oscillator.cpp`: "Check that frequency is correct for MIDI note 69 (440Hz)."
*   **Execution**: Runs on every commit (Mac/Win).

### Tier 2: Golden Master Rendering (Regression)
*   **Tool**: Custom "Headless" Runner + Python Analysis.
*   **Concept**:
    1.  We have a "Golden" reference audio file (`reference_C4_saw.wav`).
    2.  The test runner instantiates `Engine` (no GUI).
    3.  Sends MIDI Note On (C4).
    4.  Processes 2 seconds of audio to a buffer.
    5.  Compares the buffer against the Golden file.
*   **Tolerance**: Bit-exact for pure math changes. RMS threshold for approximate changes.

### Tier 3: Plugin Validation (Integration)
*   **Tool**: `auval` (macOS), `validator` (VST3 SDK).
*   **Scope**: Checks if the plugin conforms to the host API standards.
*   **Execution**: Runs on CI before packaging release.

## 2. Continuous Integration (GitHub Actions)

### Workflow: `build_and_test.yml`
1.  **Checkout** (with submodules).
2.  **Setup Audio SDKs** (iPlug2, VST3 SDK).
3.  **Build Headless Test Runner**.
4.  **Run Tests** (`./test_runner`).
    *   *Failure blocks the build.*
5.  **Build Plugin Targets** (VST3, AU, CLAP).
    *   *Release Mode with -O3*.
6.  **Run Plugin Validation**.
7.  **Upload Artifacts** (Zip file).

## 3. "No-Allocation" Enforcement
*   **Tool**: Custom Clang-Tidy check or a runtime "Heap Guard".
*   **Constraint**: We can hook `malloc`/`free` in debug builds. If called inside the `process()` callback, the plugin crashes (fail fast) or logs a critical error.
