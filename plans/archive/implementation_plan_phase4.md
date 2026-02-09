# Implementation Plan: Phase 4 (Polyphony & Filters)

## Goal
Transform the Monophonic engine into a **Polyphonic** synthesizer (8 voices) and add a **Resonant Filter** to the signal path.

## User Review Required
> [!IMPORTANT]
> Voice Allocation strategy will be "Find Free, Else Steal Oldest". This requires tracking "age" or "last used" for each voice.

## Proposed Changes

### 1. Voice Allocation (The Round Table)
*   **Target**: `src/core/VoiceManager.h`
*   **Change**:
    *   Change `mVoice` to `std::array<Voice, 8> mVoices`.
    *   Add `FindFreeVoice()` helper.
    *   Add `FindOldestVoice()` helper (Voice Stealing).
    *   `NoteOn(note)` -> Assign to voice, mark as active, store `note`.
    *   `NoteOff(note)` -> Find voice playing `note`, trigger release.

### 2. Biquad Filter (The Sculptor)
*   **Target**: `src/core/dsp/BiquadFilter.h` [NEW]
*   **Math**: Standard RBJ Cookbook LowPass.
*   **Interface**:
    *   `SetParams(cutoff, Q)`
    *   `Process(sample)`
*   **Integration**:
    *   Add `BiquadFilter` to `Voice` class.
    *   Path: `Osc` -> `Filter` -> `Amp`.

### 3. Testing & Demos
*   **Unit**: `Test_VoiceManager_Allocation` (Verify it handles > 1 note).
*   **Unit**: `Test_Biquad` (Frequency response check - naive magnitude? or just stability).
*   **Demo**: `demo_poly_chords` (Play a C Major triad).
*   **Demo**: `demo_filter_sweep` (Sawtooth with moving cutoff).

## Verification Plan
*   **Automated**: `ctest`
*   **Manual**: Run `demo_poly_chords` and listen for distinct notes (not monophonic replacement).
