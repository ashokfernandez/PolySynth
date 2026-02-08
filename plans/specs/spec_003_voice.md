# Architecture Spec: Voice Manager

**Role**: @Architect
**Status**: APPROVED
**Target**: `src/core/VoiceManager.h`

## Overview
The `VoiceManager` handles polyphony. For Phase 2 (Monophonic), it will just manage a single "Voice" or directly control the oscillator and envelope.
To keep it simple for now, we will implement `Voice` class first, then `VoiceManager`.

But for the "Monophonic Engine" task, we can simplify:
`Voice` class contains:
- 1x Oscillator
- 1x ADSR (later)
- 1x Gain 

## Interface Definition (Voice)

```cpp
#pragma once

#include "types.h"
#include "oscillator/Oscillator.h"

namespace PolySynth {

class Voice {
public:
    Voice() = default;

    void Init(double sampleRate) {
        mOsc.Init(sampleRate);
        mOsc.SetFrequency(440.0);
        mActive = false;
    }

    void Reset() {
        mOsc.Reset();
        mActive = false;
    }

    void NoteOn(int note, int velocity) {
        // Convert MIDI to Freq
        double freq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
        mOsc.SetFrequency(freq);
        mActive = true;
        mVelocity = velocity / 127.0;
    }

    void NoteOff() {
        mActive = false; // Instant off for now (Gate)
    }

    inline sample_t Process() {
        if (!mActive) return 0.0;
        return mOsc.Process() * mVelocity;
    }

    bool IsActive() const { return mActive; }

private:
    Oscillator mOsc;
    bool mActive = false;
    double mVelocity = 0.0;
};

} // namespace PolySynth
```
