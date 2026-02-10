#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

namespace sea {

class LFO {
public:
  LFO() = default;

  void Init(Real sampleRate) {
    mSampleRate = sampleRate;
    mPhase = static_cast<Real>(0.0);
    mPhaseIncrement = static_cast<Real>(0.0);
  }

  void SetRate(Real hz) {
    if (mSampleRate > static_cast<Real>(0.0)) {
      mPhaseIncrement =
          ((hz < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : hz) /
          mSampleRate;
    }
  }

  void SetDepth(Real depth) {
    mDepth = Math::Clamp(depth, static_cast<Real>(0.0), static_cast<Real>(1.0));
  }

  void SetWaveform(int type) { mWaveform = type; }

  SEA_INLINE Real Process() {
    Real out = static_cast<Real>(0.0);

    switch (mWaveform) {
    case 0: // Sine
      out = Math::Sin(kTwoPi * mPhase);
      break;
    case 1: // Triangle
      out = (static_cast<Real>(4.0) *
             Math::Abs(mPhase - static_cast<Real>(0.5))) -
            static_cast<Real>(1.0);
      break;
    case 2: // Square
      out = (mPhase < static_cast<Real>(0.5)) ? static_cast<Real>(1.0)
                                              : static_cast<Real>(-1.0);
      break;
    case 3: // Saw
      out = (static_cast<Real>(2.0) * mPhase) - static_cast<Real>(1.0);
      break;
    default:
      out = static_cast<Real>(0.0);
      break;
    }

    mPhase += mPhaseIncrement;
    if (mPhase >= static_cast<Real>(1.0)) {
      mPhase -= static_cast<Real>(1.0);
    }

    return out * mDepth;
  }

private:
  Real mSampleRate = static_cast<Real>(44100.0);
  Real mPhase = static_cast<Real>(0.0);
  Real mPhaseIncrement = static_cast<Real>(0.0);
  Real mDepth = static_cast<Real>(0.0);
  int mWaveform = 0;
};

} // namespace sea
