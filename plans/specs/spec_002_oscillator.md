# Architecture Spec: Oscillator

**Role**: @Architect
**Status**: APPROVED
**Target**: `src/core/oscillator/Oscillator.h`

## Overview
The `Oscillator` class generates audio waveforms.
For Phase 2 (Monophonic), we implement a basic **Sawtooth** wave.
Future phases will add PolyBLEP anti-aliasing.

## Interface Definition

```cpp
#pragma once

#include "../types.h"
#include <cmath>

namespace PolySynth {

class Oscillator {
public:
    Oscillator() = default;

    void Init(double sampleRate) {
        mSampleRate = sampleRate;
        mPhase = 0.0;
        mPhaseIncrement = 0.0;
    }

    void Reset() {
        mPhase = 0.0;
    }

    void SetFrequency(double freq) {
        // Basic increment: F / SR
        if (mSampleRate > 0.0) {
            mPhaseIncrement = freq / mSampleRate;
        }
    }

    // Naive Sawtooth: (2.0 * phase) - 1.0
    // Range: [-1.0, 1.0]
    inline sample_t Process() {
        sample_t out = (sample_t)((2.0 * mPhase) - 1.0);
        
        mPhase += mPhaseIncrement;
        if (mPhase >= 1.0) {
            mPhase -= 1.0;
        }
        
        return out;
    }

private:
    double mSampleRate = 44100.0;
    double mPhase = 0.0;
    double mPhaseIncrement = 0.0;
};

} // namespace PolySynth
```
