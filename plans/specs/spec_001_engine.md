# Architecture Spec: The Core Engine

**Role**: @Architect
**Status**: APPROVED
**Target**: `src/core/Engine.h`

## Overview
The `Engine` class is the single entry point for all DSP operations. It must be strictly C++17, no heap allocations in `process()`, and platform agnostic.

## Dependencies
*   `src/core/types.h`: For `sample_t`.

## Interface Definition

```cpp
#pragma once

#include "types.h"
#include <cstdint>

namespace PolySynth {

class Engine {
public:
    Engine();
    ~Engine() = default;

    // --- Lifecycle ---
    /**
     * @brief Prepare the engine for playback.
     * @param sampleRate The host sample rate.
     * @param maxBlockSize The maximum number of frames per process call.
     */
    void Init(double sampleRate, int maxBlockSize);

    /**
     * @brief Reset DSP state (clear buffers, reset envelopes).
     */
    void Reset();

    // --- Events (Midi / Params) ---
    /**
     * @brief Handle MIDI Note On.
     * @param note MIDI Note Number (0-127)
     * @param velocity velocity (0-127)
     */
    void OnNoteOn(int note, int velocity);

    /**
     * @brief Handle MIDI Note Off.
     * @param note MIDI Note Number (0-127)
     */
    void OnNoteOff(int note);

    /**
     * @brief Set a parameter value.
     * @param paramNum Unique Parameter ID
     * @param value Normalized value usually, or real value. TBD.
     */
    void SetParameter(int paramNum, double value);

    // --- Audio Processing ---
    /**
     * @brief Process a block of audio.
     * 
     * @param inputs Pointer to input channel arrays (const).
     * @param outputs Pointer to output channel arrays.
     * @param nFrames Number of samples to process.
     * @param nChans Number of channels (usually 2).
     */
    void Process(const sample_t** inputs, sample_t** outputs, int nFrames, int nChans);

private:
    double mSampleRate = 44100.0;
    // VoiceManager mVoiceManager; // TODO
};

} // namespace PolySynth
```
