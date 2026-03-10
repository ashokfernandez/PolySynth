# 04. Testing & CI Strategy

## 1. The Strategy: "Trust but Verify"
Since we cannot easily "listen" to every commit, we rely on a multi-tiered automated strategy.

### Tier 1: Unit Tests (Fast)
*   **Tool**: Catch2 v2.13.9 (55+ test cases).
*   **Scope**: DSP math, envelope logic, voice stealing, filter models, parameter boundaries, poly mod, LFO routing, stereo panning, preset serialization.
*   **Execution**: Runs on every commit via `just test`. CI runs three variants:
    *   Clean build (no sanitizers) — `native-dsp-tests`
    *   AddressSanitizer + UBSan — `build-asan-tests`
    *   ThreadSanitizer — `build-tsan-tests`

### Tier 2: Golden Master Rendering (Regression)
*   **Tool**: Demo programs + Python analysis (`scripts/golden_master.py`, `scripts/analyze_audio.py`).
*   **Concept**:
    1.  19 demo programs each produce a WAV file exercising different DSP features.
    2.  `golden_master.py --verify` re-runs demos and compares RMS difference against stored files in `tests/golden/`.
    3.  `analyze_audio.py` validates expected frequencies via FFT (e.g., 440 Hz tone).
*   **Tolerance**: 0.001 RMS (very strict — catches any signal path change).
*   **Execution**: Runs in `native-dsp-tests` CI job after unit tests pass.

### Tier 3: Visual Regression (UI)
*   **Tool**: Skia pixel readback from ComponentGallery (deterministic, no window chrome).
*   **Concept**:
    1.  Generate baseline screenshots from the PR base commit.
    2.  Build the PR commit and capture new screenshots.
    3.  Pixel-diff comparison detects any visual changes.
*   **Execution**: Runs in `native-ui-tests` CI job (macOS, requires Cocoa/SWELL).

### Tier 4: Embedded Configuration Tests (Pico)
*   **Tool**: Same Catch2 tests compiled with embedded flags.
*   **Scope**: Verifies DSP logic under `float` precision (`POLYSYNTH_USE_FLOAT`), 4-voice limit (`POLYSYNTH_MAX_VOICES=4`), and `-Wdouble-promotion` warnings.
*   **Execution**: Runs in `pico-native-embedded-tests` CI job. Local: `just test-embedded`.

### Tier 5: Integration
*   **WAM build**: Emscripten compilation catches `#if IPLUG_EDITOR` guard issues.
*   **ARM cross-compile**: Proves firmware links for RP2350 target hardware.
*   **Wokwi emulation**: Runs actual firmware in emulator, checks self-test serial output.
*   **Desktop smoke test**: Verifies app reaches UI-loaded state without crashing.

## 2. Continuous Integration (GitHub Actions)

### Workflow: `ci.yml`

The pipeline uses a safety-gate pattern: fast analysis jobs run first in parallel, and downstream jobs only start after all gates pass.

```
Parallel start (safety gates):
  lint-static-analysis           — cppcheck, clang-tidy, platform guards
  build-asan-tests               — memory safety (ASan + UBSan)
  build-tsan-tests               — data race detection (TSan)
  pico-native-embedded-tests     — float precision + voice-count (independent)
  pico-arm-compile-check         — ARM cross-compile (independent)

After safety gates pass (parallel):
  native-dsp-tests               — golden masters + frequency analysis
  native-ui-tests                — VRT + desktop smoke test (macOS)
  build-wam-demo                 — Emscripten WAM build

Tag-only (release):
  build-macos / build-windows    — platform release builds
  publish-release                — GitHub Release with .pkg + .zip

Main-only (deploy):
  package-demo-site -> publish-pages
```

### Caching
*   **ccache**: Per-job cache keys (lint, asan, tsan, native, sandbox) to avoid cross-contamination between Debug/ASan/TSan builds.
*   **External dependencies**: `actions/cache` on `external/` directory, keyed on `hashFiles('scripts/download_dependencies.sh')`.
*   **Pico SDK**: `actions/cache` on `external/pico-sdk/`, keyed on `hashFiles('scripts/download_pico_sdk.sh')`.

### Local Verification
*   `just check` — lint + unit tests (quick gate)
*   `just ci-pr` — lint + asan + tsan + test + desktop smoke + VRT (full PR gate)
*   `just test-embedded` — embedded configuration tests

## 3. "No-Allocation" Enforcement
*   **Tool**: Custom Clang-Tidy check or a runtime "Heap Guard".
*   **Constraint**: We can hook `malloc`/`free` in debug builds. If called inside the `process()` callback, the plugin crashes (fail fast) or logs a critical error.
