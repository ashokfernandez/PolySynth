# PolySynth Detailed Roadmap

This document serves as the tactical roadmap for agents working on `polysynth`. It complements the high-level strategy in `05_program_management.md`.

## Phase 0: Rescue & First Build ✅
**Goal:** Fix broken build and enable "Blind" Agents to verify their work.

- [x] **Milestone 0.0: Fix Build Errors**
    - [x] Fix Namespace in `BiquadFilter.h` (PolySynth vs PolySynthCore).
    - [x] Fix Include paths in `tests/unit/Test_BiquadFilter.cpp`.
    - [x] Run `scripts/run_tests.sh`.
- [x] **Milestone 0.1: Test Runner Scripts**
    - [x] Create `scripts/run_tests.sh` to automate the CMake build/test cycle.
    - [x] Update `ci.yml` to use this script.
- [x] **Milestone 0.2: Audio Artifact Verification**
    - [x] Create `scripts/analyze_audio.py` (FFT/RMS check) to verify `demo_render_wav` output.
    - [x] Add "Golden Master" regression testing (compare new WAVs against known good WAVs).
    - [x] Automated CI for builds and tests.

## Phase 1: Core DSP ✅
**Goal:** Robust polyphonic synthesis with modulation and filters.

- [x] **Milestone 1.1: ADSR Envelope**
    - [x] Implement `ADSREnvelope` class with hardened transitions.
    - [x] Verify with `demo_adsr`.
- [x] **Milestone 1.2: Polyphony**
    - [x] Implement `VoiceManager` (8 voices).
    - [x] Implement Note Stealing (Oldest).
    - [x] Verify with `demo_poly_chords`.
- [x] **Milestone 1.3: Filter & Oscillators**
    - [x] Implement `BiquadFilter` (Lowpass).
    - [x] Implement multiple Oscillator waveforms (Saw, Square, Triangle, Sine).
    - [x] Implement LFO for filter-cutoff modulation.
    - [x] Verify with `demo_filter_sweep`.

## Phase 2: UI & Integration ✅
**Goal:** Seamless integration between DSP engine and Web UI.

- [x] **Milestone 2.1: Engine Integration**
    - [x] Replace `MidiSynth` boilerplate in `src/platform/desktop`.
    - [x] Connect `VoiceManager` to `ProcessBlock`.
- [x] **Milestone 2.2: Web UI**
    - [x] Implement `IPlugWebViewEditorDelegate`.
    - [x] Bind JS knobs to C++ parameters with bidirectional sync.
    - [x] Sync UI state on initialization.

## Phase 3: Release
**Goal:** Distribution-ready installers and certification.

- [ ] **Milestone 3.1: Installer** (Pkg/MSI).
- [ ] **Milestone 3.2: Signing & Notarization**.

## Agent "Next Task" Selector
1.  Check `task.md` (in `antigravity/` or `polysynth/plans/`).
2.  Phase 0, 1, and 2 are nearly complete.
3.  Next priority: Release packaging and installer creation.
