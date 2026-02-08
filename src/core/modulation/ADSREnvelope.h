#pragma once

#include "../types.h"
#include <algorithm>
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
    mA = std::max(0.0, a);
    mD = std::max(0.0, d);
    mS = std::clamp(s, 0.0, 1.0);
    mR = std::max(0.0, r);
    CalculateCoefficients();
  }

  void NoteOn() {
    if (mA == 0.0) {
      mLevel = 1.0;
      mStage = (mD == 0.0) ? kSustain : kDecay;
      if (mStage == kSustain) {
        mLevel = mS;
      }
    } else {
      mStage = kAttack;
    }
  }

  void NoteOff() {
    if (mStage != kIdle) {
      if (mR == 0.0) {
        mLevel = 0.0;
        mStage = kIdle;
      } else {
        mReleaseInc = (mR > 0.0) ? (mLevel / (mR * mSampleRate)) : 0.0;
        mStage = kRelease;
      }
    }
  }

  inline sample_t Process() {
    if (mStage == kIdle)
      return 0.0;

    switch (mStage) {
    case kIdle:
      break;
    case kAttack:
      if (mAttackInc <= 0.0) {
        mLevel = 1.0;
        mStage = (mD == 0.0) ? kSustain : kDecay;
        if (mStage == kSustain) {
          mLevel = mS;
        }
      } else {
        mLevel += mAttackInc;
        if (mLevel >= 1.0) {
          mLevel = 1.0;
          mStage = (mD == 0.0) ? kSustain : kDecay;
          if (mStage == kSustain) {
            mLevel = mS;
          }
        }
      }
      break;
    case kDecay:
      if (mDecayInc <= 0.0) {
        mLevel = mS;
        mStage = kSustain;
      } else {
        mLevel -= mDecayInc;
        if (mLevel <= mS) {
          mLevel = mS;
          mStage = kSustain;
        }
      }
      break;
    case kSustain:
      mLevel = mS;
      break;
    case kRelease:
      if (mReleaseInc <= 0.0) {
        mLevel = 0.0;
        mStage = kIdle;
      } else {
        mLevel -= mReleaseInc;
        if (mLevel <= 0.0) {
          mLevel = 0.0;
          mStage = kIdle;
        }
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
      mAttackInc = (mA > 0.0) ? (1.0 / (mA * mSampleRate)) : 0.0;
      mDecayInc = (mD > 0.0) ? ((1.0 - mS) / (mD * mSampleRate)) : 0.0;
      mReleaseInc = (mR > 0.0) ? (mS / (mR * mSampleRate)) : 0.0;
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
