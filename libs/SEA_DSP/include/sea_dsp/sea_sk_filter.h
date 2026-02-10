#pragma once
#include "sea_tpt_integrator.h"
#include <algorithm>

namespace sea {

template <typename T> class SKFilter {
public:
  SKFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    lpf1.Init(sampleRate);
    lpf2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    lpf1.Reset();
    lpf2.Reset();
  }

  void SetParams(T cutoff, T k) {
    // ClampCutoff
    if (mSampleRate <= static_cast<T>(0.0)) {
      mCutoff = static_cast<T>(0.0);
    } else {
      T nyquist = static_cast<T>(0.5) * mSampleRate;
      T maxCutoff = nyquist * static_cast<T>(0.49);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    mK = Math::Clamp(k, static_cast<T>(0.0), static_cast<T>(1.98));
    lpf1.Prepare(mCutoff);
    lpf2.Prepare(mCutoff);
  }

  SEA_INLINE T Process(T in) {
    T g = lpf1.GetG();
    T one = static_cast<T>(1.0);
    T G = g / (one + g);

    T s1 = lpf1.GetS();
    T S1 = s1 / (one + g);

    T s2 = lpf2.GetS();
    T S2 = s2 / (one + g);

    T feedbackFactor = mK * (G - G * G);
    T denom = one - feedbackFactor;

    // Safety for high resonance
    if (Math::Abs(denom) < static_cast<T>(1e-6))
      denom = static_cast<T>(1e-6);

    T rhs = in + mK * ((one - G) * S1 - S2);
    T u = rhs / denom;

    // LPF1
    T y1 = (g * u + s1) / (one + g);
    lpf1.SetS(static_cast<T>(2.0) * y1 - s1);

    // LPF2
    T y2 = (g * y1 + s2) / (one + g);
    lpf2.SetS(static_cast<T>(2.0) * y2 - s2);

    return y2;
  }

private:
  T mSampleRate = static_cast<T>(44100.0);
  T mCutoff = static_cast<T>(1000.0);
  T mK = static_cast<T>(0.0);

  TPTIntegrator<T> lpf1;
  TPTIntegrator<T> lpf2;
};

} // namespace sea
