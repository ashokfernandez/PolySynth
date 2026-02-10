#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

namespace sea {

class ADSREnvelope {
public:
  enum Stage { kIdle, kAttack, kDecay, kSustain, kRelease };

  ADSREnvelope() = default;

  void Init(Real sampleRate) {
    mSampleRate = sampleRate;
    CalculateCoefficients();
    Reset();
  }

  void Reset() {
    mStage = kIdle;
    mLevel = static_cast<Real>(0.0);
  }

  void SetParams(Real a, Real d, Real s, Real r) {
    mA = (a < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : a;
    mD = (d < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : d;
    mS = Math::Clamp(s, static_cast<Real>(0.0), static_cast<Real>(1.0));
    mR = (r < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : r;
    CalculateCoefficients();
  }

  void NoteOn() {
    if (mA == static_cast<Real>(0.0)) {
      mLevel = static_cast<Real>(1.0);
      mStage = (mD == static_cast<Real>(0.0)) ? kSustain : kDecay;
      if (mStage == kSustain) {
        mLevel = mS;
      }
    } else {
      mStage = kAttack;
    }
  }

  void NoteOff() {
    if (mStage != kIdle) {
      if (mR == static_cast<Real>(0.0)) {
        mLevel = static_cast<Real>(0.0);
        mStage = kIdle;
      } else {
        mReleaseInc = (mR > static_cast<Real>(0.0))
                          ? (mLevel / (mR * mSampleRate))
                          : static_cast<Real>(0.0);
        mStage = kRelease;
      }
    }
  }

  SEA_INLINE Real Process() {
    if (mStage == kIdle)
      return static_cast<Real>(0.0);

    switch (mStage) {
    case kIdle:
      break;
    case kAttack:
      if (mAttackInc <= static_cast<Real>(0.0)) {
        mLevel = static_cast<Real>(1.0);
        mStage = (mD == static_cast<Real>(0.0)) ? kSustain : kDecay;
        if (mStage == kSustain) {
          mLevel = mS;
        }
      } else {
        mLevel += mAttackInc;
        if (mLevel >= static_cast<Real>(1.0)) {
          mLevel = static_cast<Real>(1.0);
          mStage = (mD == static_cast<Real>(0.0)) ? kSustain : kDecay;
          if (mStage == kSustain) {
            mLevel = mS;
          }
        }
      }
      break;
    case kDecay:
      if (mDecayInc <= static_cast<Real>(0.0)) {
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
      if (mReleaseInc <= static_cast<Real>(0.0)) {
        mLevel = static_cast<Real>(0.0);
        mStage = kIdle;
      } else {
        mLevel -= mReleaseInc;
        if (mLevel <= static_cast<Real>(0.0)) {
          mLevel = static_cast<Real>(0.0);
          mStage = kIdle;
        }
      }
      break;
    }
    return mLevel;
  }

  bool IsActive() const { return mStage != kIdle; }
  Real GetLevel() const { return mLevel; }
  Stage GetStage() const { return mStage; }

private:
  void CalculateCoefficients() {
    if (mSampleRate > static_cast<Real>(0.0)) {
      mAttackInc = (mA > static_cast<Real>(0.0))
                       ? (static_cast<Real>(1.0) / (mA * mSampleRate))
                       : static_cast<Real>(0.0);
      mDecayInc = (mD > static_cast<Real>(0.0))
                      ? ((static_cast<Real>(1.0) - mS) / (mD * mSampleRate))
                      : static_cast<Real>(0.0);
      mReleaseInc = (mR > static_cast<Real>(0.0)) ? (mS / (mR * mSampleRate))
                                                  : static_cast<Real>(0.0);
    }
  }

  Real mSampleRate = static_cast<Real>(44100.0);
  Stage mStage = kIdle;
  Real mLevel = static_cast<Real>(0.0);

  Real mA = static_cast<Real>(0.01);
  Real mD = static_cast<Real>(0.1);
  Real mS = static_cast<Real>(0.5);
  Real mR = static_cast<Real>(0.2);

  Real mAttackInc = static_cast<Real>(0.0);
  Real mDecayInc = static_cast<Real>(0.0);
  Real mReleaseInc = static_cast<Real>(0.0);
};

} // namespace sea
