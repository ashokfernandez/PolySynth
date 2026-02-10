#pragma once
#include "sea_svf.h"
#include <algorithm>

namespace sea {

template <typename T> class CascadeFilter {
public:
  enum class Slope { dB12, dB24 };

  CascadeFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    stage1.Init(sampleRate);
    stage2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    stage1.Reset();
    stage2.Reset();
    mLastOut12 = static_cast<T>(0.0);
    mLastOut24 = static_cast<T>(0.0);
  }

  void SetParams(T cutoff, T resonance, Slope slope) {
    mCutoff =
        Math::Clamp(cutoff, static_cast<T>(20.0), static_cast<T>(20000.0));
    mResonance =
        Math::Clamp(resonance, static_cast<T>(0.0), static_cast<T>(1.0));
    mSlope = slope;
    UpdateStages();
  }

  SEA_INLINE T Process(T in) {
    T feedback = (mSlope == Slope::dB24) ? mLastOut24 : mLastOut12;
    T driven = Math::Tanh(in - feedback * mFeedbackGain);

    T out1 = stage1.ProcessLP(driven);
    T out2 = stage2.ProcessLP(out1);

    mLastOut12 = out1;
    mLastOut24 = out2;

    return (mSlope == Slope::dB24) ? out2 : out1;
  }

private:
  void UpdateStages() {
    T q = static_cast<T>(0.5) + mResonance * static_cast<T>(9.5);
    q = Math::Clamp(q, static_cast<T>(0.5), static_cast<T>(12.0));

    stage1.SetParams(mCutoff, q);
    stage2.SetParams(mCutoff, q);

    mFeedbackGain = mResonance * static_cast<T>(1.6);
  }

  T mSampleRate = static_cast<T>(44100.0);
  T mCutoff = static_cast<T>(1000.0);
  T mResonance = static_cast<T>(0.0);
  T mFeedbackGain = static_cast<T>(0.0);
  Slope mSlope = Slope::dB24;

  T mLastOut12 = static_cast<T>(0.0);
  T mLastOut24 = static_cast<T>(0.0);

  SVFilter<T> stage1;
  SVFilter<T> stage2;
};

} // namespace sea
