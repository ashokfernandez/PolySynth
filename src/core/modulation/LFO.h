#pragma once

#include "../types.h"
#include <algorithm>
#include <cmath>

namespace PolySynthCore {

class LFO {
public:
  LFO() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    mPhase = 0.0;
    mPhaseIncrement = 0.0;
  }

  void SetRate(double hz) {
    if (mSampleRate > 0.0) {
      mPhaseIncrement = std::max(0.0, hz) / mSampleRate;
    }
  }

  void SetDepth(double depth) { mDepth = std::clamp(depth, 0.0, 1.0); }

  void SetWaveform(int type) { mWaveform = type; }

  inline sample_t Process() {
    sample_t out = 0.0;

    switch (mWaveform) {
    case 0: // Sine
      out = (sample_t)std::sin(kTwoPi * mPhase);
      break;
    case 1: // Triangle
      out = (sample_t)((4.0 * std::abs(mPhase - 0.5)) - 1.0);
      break;
    case 2: // Square
      out = (mPhase < 0.5) ? 1.0 : -1.0;
      break;
    case 3: // Saw
      out = (sample_t)((2.0 * mPhase) - 1.0);
      break;
    default:
      out = 0.0;
      break;
    }

    mPhase += mPhaseIncrement;
    if (mPhase >= 1.0) {
      mPhase -= 1.0;
    }

    return out * mDepth;
  }

private:
  double mSampleRate = 44100.0;
  double mPhase = 0.0;
  double mPhaseIncrement = 0.0;
  double mDepth = 0.0;
  int mWaveform = 0;
};

} // namespace PolySynthCore
