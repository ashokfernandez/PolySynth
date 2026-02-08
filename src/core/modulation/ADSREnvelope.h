#pragma once

#include "../types.h"
#include <cmath>

namespace PolySynthCore {

class ADSREnvelope {
public:
  enum Stage { kIdle, kAttack, kDecay, kSustain, kRelease };

  ADSREnvelope() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    CalculateCoefficients(); // Default init
    Reset();
  }

  void Reset() {
    mStage = kIdle;
    mLevel = 0.0;
  }

  // Params in Seconds (except Sustain which is 0.0-1.0)
  void SetParams(double a, double d, double s, double r) {
    mA = a;
    mD = d;
    mS = s;
    mR = r;
    CalculateCoefficients();
  }

  void NoteOn() { mStage = kAttack; }

  void NoteOff() {
    if (mStage != kIdle) {
      mStage = kRelease;
    }
  }

  inline sample_t Process() {
    if (mStage == kIdle)
      return 0.0;

    switch (mStage) {
    case kIdle:
      break;
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
  sample_t GetLevel() const { return mLevel; }
  Stage GetStage() const { return mStage; }

private:
  void CalculateCoefficients() {
    if (mSampleRate > 0.0) {
      mAttackInc = 1.0 / (mA * mSampleRate);
      mDecayInc = (1.0 - mS) / (mD * mSampleRate);
      // Release from 1.0 down to 0 over R seconds (simplified linear slope)
      // But actually we release from mS.
      // To keep slope constant regardless of starting point:
      mReleaseInc = 1.0 / (mR * mSampleRate);
    }
  }

  double mSampleRate = 44100.0;
  Stage mStage = kIdle;
  sample_t mLevel = 0.0;

  double mA = 0.01;
  double mD = 0.1;
  double mS = 0.5;
  double mR = 0.2; // Default release

  double mAttackInc = 0.0;
  double mDecayInc = 0.0;
  double mReleaseInc = 0.0;
};

} // namespace PolySynthCore
