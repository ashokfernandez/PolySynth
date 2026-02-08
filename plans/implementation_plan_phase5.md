# Implementation Plan: Phase 5 (iPlug2 Integration)

## Goal
Integrate the core C++ engine (`VoiceManager`, `Engine`) into the iPlug2 framework to create a playable VST3/APP.

## User Review Required
> [!IMPORTANT]
> The current iPlug2 project (`src/platform/desktop`) uses a boilerplate `MidiSynth` class. We will DELETE this boilerplate and Replace it with `PolySynth::VoiceManager`.

## Proposed Changes

### 1. DSP Integration (The Bridge)
*   **Target**: `src/platform/desktop/PolySynth_DSP.h`
*   **Change**:
    *   Remove `MidiSynth`, `VoiceAllocator`, etc.
    *   Include `../../core/VoiceManager.h`.
    *   Add `PolySynth::VoiceManager mVoiceManager` member.
    *   Implement `ProcessBlock` to call `mVoiceManager.Process()`.
    *   Implement `ProcessMidiMsg` to call `mVoiceManager.OnNoteOn/OnNoteOff`.

### 2. Parameter Mapping (The Controls)
*   **Target**: `src/platform/desktop/PolySynth.cpp`
*   **Change**:
    *   Update `OnParamChange` to pass values to `VoiceManager`.
    *   Map:
        *   `kParamGain` -> Output Gain (maybe add to VoiceManager?).
        *   `kParamAttack/Decay/Sustain/Release` -> `VoiceManager::SetADSR(...)` (Need to add this setter).
        *   `kParamFilterCutoff` (New) -> `VoiceManager::SetFilter(...)` (Need to add this setter).

### 3. Build Configuration
*   **Target**: `src/platform/desktop/CMakeLists.txt`
*   **Change**:
    *   Ensure include paths can find `../../core`.

## Verification Plan
*   **Automated**:
    *   Build the APP target using CMake.
*   **Manual**:
    *   Run `./PolySynth.app` (or executable).
    *   Use computer keyboard (Z,X,C...) or MIDI keyboard to play notes.
    *   Verify sound output and polyphony.
