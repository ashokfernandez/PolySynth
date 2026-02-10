#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

#if !defined(SEA_DSP_ADSR_BACKEND_SEA_CORE) && !defined(SEA_DSP_ADSR_BACKEND_DAISYSP)
#define SEA_DSP_ADSR_BACKEND_SEA_CORE
#endif

#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
#include "daisysp.h"
#endif

namespace sea {

class ADSREnvelope {
public:
  enum Stage { kIdle, kAttack, kDecay, kSustain, kRelease };

  ADSREnvelope() = default;

  void Init(Real sampleRate) {
    mSampleRate = sampleRate;
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    mAdsr.Init(static_cast<float>(sampleRate));
    mGate = false;
    SetParams(mA, mD, mS, mR);
    Reset();
#else
    CalculateCoefficients();
    Reset();
#endif
  }

  void Reset() {
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    mGate = false;
    mStage = kIdle;
    mLevel = static_cast<Real>(0.0);
    mAdsr.Init(static_cast<float>(mSampleRate));
    mAdsr.SetAttackTime(static_cast<float>(mA));
    mAdsr.SetDecayTime(static_cast<float>(mD));
    mAdsr.SetSustainLevel(static_cast<float>(mS));
    mAdsr.SetReleaseTime(static_cast<float>(mR));
#else
    mStage = kIdle;
    mLevel = static_cast<Real>(0.0);
#endif
  }

  void SetParams(Real a, Real d, Real s, Real r) {
    mA = (a < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : a;
    mD = (d < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : d;
    mS = Math::Clamp(s, static_cast<Real>(0.0), static_cast<Real>(1.0));
    mR = (r < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : r;
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    mAdsr.SetAttackTime(static_cast<float>(mA));
    mAdsr.SetDecayTime(static_cast<float>(mD));
    mAdsr.SetSustainLevel(static_cast<float>(mS));
    mAdsr.SetReleaseTime(static_cast<float>(mR));
#else
    CalculateCoefficients();
#endif
  }

  void NoteOn() {
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    mGate = true;
    mStage = kAttack;
#else
    if (mA == static_cast<Real>(0.0)) {
      mLevel = static_cast<Real>(1.0);
      mStage = (mD == static_cast<Real>(0.0)) ? kSustain : kDecay;
      if (mStage == kSustain) {
        mLevel = mS;
      }
    } else {
      mStage = kAttack;
    }
#endif
  }

  void NoteOff() {
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    if (mStage != kIdle) {
      mGate = false;
      mStage = kRelease;
    }
#else
    if (mStage != kIdle) {
      if (mR == static_cast<Real>(0.0)) {
        mLevel = static_cast<Real>(0.0);
        mStage = kIdle;
      } else {
        // Multiply instead of divide - much faster
        mReleaseInc = mLevel * mReleaseRecip;
        mStage = kRelease;
      }
    }
#endif
  }

  SEA_INLINE Real Process() {
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
    Real out = static_cast<Real>(mAdsr.Process(mGate));
    mLevel = Math::Clamp(out, static_cast<Real>(0.0), static_cast<Real>(1.0));
    UpdateStageFromBackend(mLevel);
    return mLevel;
#else
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
#endif
  }

  bool IsActive() const { return mStage != kIdle; }
  Real GetLevel() const { return mLevel; }
  Stage GetStage() const { return mStage; }

private:
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
  void UpdateStageFromBackend(Real out) {
    uint8_t seg = mAdsr.GetCurrentSegment();
    if(!mAdsr.IsRunning() && !mGate) {
      mStage = kIdle;
      return;
    }
    if (mGate) {
      if (seg == daisysp::ADSR_SEG_ATTACK) {
        mStage = kAttack;
      } else if (seg == daisysp::ADSR_SEG_DECAY) {
        Real eps = static_cast<Real>(0.005);
        mStage = (Math::Abs(out - mS) <= eps) ? kSustain : kDecay;
      } else {
        mStage = kSustain;
      }
    } else {
      mStage = mAdsr.IsRunning() ? kRelease : kIdle;
    }
  }
#else
  void CalculateCoefficients() {
    if (mSampleRate > static_cast<Real>(0.0)) {
      mAttackInc = (mA > static_cast<Real>(0.0))
                       ? (static_cast<Real>(1.0) / (mA * mSampleRate))
                       : static_cast<Real>(0.0);
      mDecayInc = (mD > static_cast<Real>(0.0))
                      ? ((static_cast<Real>(1.0) - mS) / (mD * mSampleRate))
                      : static_cast<Real>(0.0);
      // Pre-calculate reciprocal to avoid division in NoteOff
      mReleaseRecip = (mR > static_cast<Real>(0.0))
                          ? (static_cast<Real>(1.0) / (mR * mSampleRate))
                          : static_cast<Real>(0.0);
      mReleaseInc = mS * mReleaseRecip;  // Keep this for reference
    }
  }
#endif

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
  Real mReleaseRecip = static_cast<Real>(0.0);  // Pre-calculated 1/(R*SR)
#ifdef SEA_DSP_ADSR_BACKEND_DAISYSP
  bool mGate = false;
  daisysp::Adsr mAdsr;
#endif
};

} // namespace sea
