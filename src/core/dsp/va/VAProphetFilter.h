#pragma once

#include "VASVFilter.h"
#include <algorithm>
#include <cmath>

namespace PolySynthCore {

class VAProphetFilter {
public:
  enum class Slope { dB12, dB24 };

  VAProphetFilter() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    stage1.Init(sampleRate);
    stage2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    stage1.Reset();
    stage2.Reset();
    mLastOut12 = 0.0;
    mLastOut24 = 0.0;
  }

  void SetParams(double cutoff, double resonance, Slope slope) {
    mCutoff = std::clamp(cutoff, 20.0, 20000.0);
    mResonance = std::clamp(resonance, 0.0, 1.0);
    mSlope = slope;
    UpdateStages();
  }

  inline double Process(double in) {
    double feedback = (mSlope == Slope::dB24) ? mLastOut24 : mLastOut12;
    double driven = std::tanh(in - feedback * mFeedbackGain);

    double out1 = stage1.ProcessLP(driven);
    double out2 = stage2.ProcessLP(out1);

    mLastOut12 = out1;
    mLastOut24 = out2;

    return (mSlope == Slope::dB24) ? out2 : out1;
  }

private:
  void UpdateStages() {
    double q = 0.5 + mResonance * 9.5;
    q = std::clamp(q, 0.5, 12.0);

    stage1.SetParams(mCutoff, q);
    stage2.SetParams(mCutoff, q);

    mFeedbackGain = mResonance * 1.6;
  }

  double mSampleRate = 44100.0;
  double mCutoff = 1000.0;
  double mResonance = 0.0;
  double mFeedbackGain = 0.0;
  Slope mSlope = Slope::dB24;

  double mLastOut12 = 0.0;
  double mLastOut24 = 0.0;

  VASVFilter stage1;
  VASVFilter stage2;
};

} // namespace PolySynthCore
