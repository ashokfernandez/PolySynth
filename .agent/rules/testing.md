# Testing, Validation & E2E Verification

We employ a 3-tiered testing strategy to ensure audio quality and prevent regressions.

## Tier 1: Unit Testing (Catch2)
- **Tool**: Catch2 (C++17).
- **Location**: `tests/unit/`.
- **Requirements**:
    - Every new DSP class MUST have a corresponding unit test.
    - Tests should cover edge cases (e.g., zero frequencies, invalid gain levels).
    - Ensure DSP math is accurate (e.g., frequency to MIDI conversions).
- **Execution**: `just test`.

## Tier 2: Golden Master Verification (Regression)
- **Tool**: `scripts/golden_master.py` + `scripts/analyze_audio.py`.
- **Purpose**: Detect subtle changes in audio output across commits.
- **Rules**:
    - **Generate**: Use `python scripts/golden_master.py --generate` when changing DSP logic intentionally to update the "known good" audio.
    - **Verify**: Use `python scripts/golden_master.py --verify` as part of every validation step.
    - **Tolerance**: RMS difference should be below `0.001` for expected behavior.
- **Audio Analysis**: Use `scripts/analyze_audio.py` for FFT/RMS checks on specific artifacts.

## Tier 3: Plugin Validation
- **Requirement**: Build in Release mode (`-O3`) before validation.
- **Mac**: Use `auval` to validate the AU component.
- **Windows/VST3**: Use the VST3 `validator` tool.

## E2E Validation Workflow
1.  Run `just test` (Unit Tests).
2.  Run `./scripts/run_audio_tests.sh` (Artifact Verification).
3.  Perform manual verification by launching the app: `just desktop-run`.

## Failure Diagnostics
- Commands executed via `just` write full logs to `.artifacts/logs/<run-id>/`.
- Use `just logs-latest` to inspect summary + log paths after a failure.
- Use `POLYSYNTH_VERBOSE=1 just <task>` when you need full live output.
