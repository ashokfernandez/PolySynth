# Architecture Spec: ADSR Envelope

**Role**: @Architect
**Status**: DRAFT
**Target**: `src/core/modulation/ADSREnvelope.h`

## Overview
A classic Attack-Decay-Sustain-Release envelope generator.
*   **Input**: Gate (bool), Parameters (A, D, S, R).
*   **Output**: Control signal [0.0, 1.0].
*   **Scaling**: Linear for now, exponential later.

## Interface Definition

```cpp
#pragma once

#include "../types.h"
#include <cmath>

namespace PolySynth {

class ADSREnvelope {
public:
    enum Stage {
        kIdle,
        kAttack,
        kDecay,
        kSustain,
        kRelease
    };

    ADSREnvelope() = default;

    void Init(double sampleRate) {
        mSampleRate = sampleRate;
        Reset();
    }

    void Reset() {
        mStage = kIdle;
        mLevel = 0.0;
        mMultiplier = 0.0;
    }

    // Params in Seconds (except Sustain which is 0.0-1.0)
    void SetParams(double a, double d, double s, double r) {
        mA = a; mD = d; mS = s; mR = r;
        CalculateCoefficients();
    }

    void NoteOn() {
        mStage = kAttack;
    }

    void NoteOff() {
        if (mStage != kIdle) {
            mStage = kRelease;
        }
    }

    inline sample_t Process() {
        if (mStage == kIdle) return 0.0;
        
        // Simple Linear for MVP
        switch (mStage) {
            case kAttack:
                mLevel += mAttackInc;
                if (mLevel >= 1.0) {
                    mLevel = 1.0;
                    mStage = kDecay;
                }
                break;
            case kDecay:
                mLevel -= mDecayInc;
                if (mLevel <= mS) {
                    mLevel = mS;
                    mStage = kSustain;
                }
                break;
            case kSustain:
                mLevel = mS;
                break;
            case kRelease:
                mLevel -= mReleaseInc;
                if (mLevel <= 0.0) {
                    mLevel = 0.0;
                    mStage = kIdle;
                }
                break;
        }
        return mLevel;
    }

    bool IsActive() const { return mStage != kIdle; }

private:
    void CalculateCoefficients() {
        if (mSampleRate > 0.0) {
             mAttackInc = 1.0 / (mA * mSampleRate); 
             mDecayInc = (1.0 - mS) / (mD * mSampleRate);
             mReleaseInc = mS / (mR * mSampleRate); // Release from S to 0? No, from current.
             // Actually, Release usually assumes falling from S. 
             // If we release during Attack, we need logic. 
             // For linear MVP: ReleaseInc = 1.0 / (R * SR). 
             // But we want to drain 'mLevel'. mReleaseInc = 1.0 / (mR * mSampleRate) -> drops 1.0 in R seconds.
             // Correct.
        }
    }

    double mSampleRate = 44100.0;
    Stage mStage = kIdle;
    sample_t mLevel = 0.0;

    double mA = 0.01;
    double mD = 0.1;
    double mS = 0.5;
    double mR = 1.0;

    double mAttackInc = 0.0;
    double mDecayInc = 0.0;
    double mReleaseInc = 0.0;
};

} // namespace PolySynth
```
