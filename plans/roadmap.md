# PolySynth Detailed Roadmap

This document serves as the tactical roadmap for agents working on `polysynth`. It complements the high-level strategy in `05_program_management.md`.

## Phase 0: Rescue & First Build (Immediate Priority)
**Goal:** Fix broken build and enable "Blind" Agents to verify their work.
**Coordinator:** @AgentRescue

- [ ] **Milestone 0.0: Fix Build Errors**
    - [ ] Fix Namespace in `BiquadFilter.h` (PolySynth vs PolySynthCore).
    - [ ] Fix Include paths in `tests/unit/Test_BiquadFilter.cpp`.
    - [ ] Run `scripts/run_tests.sh` (Create this script first).
- [ ] **Milestone 0.1: Test Runner Scripts**
    - [ ] Create `scripts/run_tests.sh` to automate the CMake build/test cycle.
    - [ ] Update `ci.yml` to use this script.
- [ ] **Milestone 0.2: Audio Artifact Verification**
    - [ ] Create `scripts/analyze_audio.py` (FFT/RMS check) to verify `demo_render_wav` output.
    - [ ] Add "Golden Master" regression testing (compare new WAVs against known good WAVs).

## Phase 1: Core DSP (Monophonic to Polyphonic)
**Reference:** `implementation_plan_phase3.md` (ADSR), `implementation_plan_phase4.md` (Polyphony)

- [ ] **Milestone 1.1: ADSR Envelope**
    - [ ] Implement `ADSREnvelope` class.
    - [ ] Verify with `demo_adsr`.
- [ ] **Milestone 1.2: Polyphony**
    - [ ] Implement `VoiceManager` (8 voices).
    - [ ] Implement Note Stealing (Oldest).
    - [ ] Verify with `demo_poly_chords`.
- [ ] **Milestone 1.3: Filter**
    - [ ] Implement `BiquadFilter` (Lowpass).
    - [ ] Verify with `demo_filter_sweep`.

## Phase 2: UI & Integration
**Reference:** `implementation_plan_phase5.md` (iPlug2 Integration)

- [ ] **Milestone 2.1: Engine Integration**
    - [ ] Replace `MidiSynth` boilerplate in `src/platform/desktop`.
    - [ ] Connect `VoiceManager` to `ProcessBlock`.
- [ ] **Milestone 2.2: Web UI**
    - [ ] Implement `IPlugWebViewEditorDelegate`.
    - [ ] Bind JS knobs to C++ parameters.

## Phase 3: Release
- [ ] **Milestone 3.1: Installer** (Pkg/MSI).
- [ ] **Milestone 3.2: Signing & Notarization**.

## Agent "Next Task" Selector
1.  Check `task.md` (in `antigravity/` or `polysynth/plans/`).
2.  If Phase 0 is incomplete, work on scripts.
3.  If Phase 0 is done, pick the next unchecked Box in Phase 1.
