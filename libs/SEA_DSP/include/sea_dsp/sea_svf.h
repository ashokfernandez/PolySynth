#pragma once
#include "sea_tpt_integrator.h"
#include <algorithm>

namespace sea {

template <typename T> class SVFilter {
public:
  struct Outputs {
    T lp;
    T bp;
    T hp;
  };

  SVFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    integrator1.Init(sampleRate);
    integrator2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    integrator1.Reset();
    integrator2.Reset();
  }

  void SetParams(T cutoff, T Q) {
    // Clamp cutoff
    if (mSampleRate <= static_cast<T>(0.0)) {
      mCutoff = static_cast<T>(0.0);
    } else {
      T nyquist = static_cast<T>(0.5) * mSampleRate;
      T maxCutoff = nyquist * static_cast<T>(0.49);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    mQ = (Q < static_cast<T>(0.05)) ? static_cast<T>(0.05) : Q;

    CalculateCoefficients();
  }

  SEA_INLINE Outputs Process(T in) {
    // This relies on Prepare happening in SetParams
    // Original implementation accessed g from integrator1
    T g = integrator1.GetG();
    // R = 1 / (2Q)
    T R = static_cast<T>(1.0) / (static_cast<T>(2.0) * mQ);
    T s1 = integrator1.GetS();
    T s2 = integrator2.GetS();

    T denominator = static_cast<T>(1.0) + static_cast<T>(2.0) * R * g + g * g;

    // (in - (2R + g)*s1 - s2) / den
    // Original had: (in - (2.0 * R + g) * s1 - s2)
    // Let's match exact order of operations if possible for bit-exactness
    T hp = (in - (static_cast<T>(2.0) * R + g) * s1 - s2) / denominator;

    T bp = integrator1.Process(hp);
    T lp = integrator2.Process(bp);

    return {lp, bp, hp};
  }

  // Convenience for single output
  SEA_INLINE T ProcessLP(T in) { return Process(in).lp; }
  SEA_INLINE T ProcessBP(T in) { return Process(in).bp; }
  SEA_INLINE T ProcessHP(T in) { return Process(in).hp; }

private:
  void CalculateCoefficients() {
    integrator1.Prepare(mCutoff);
    integrator2.Prepare(mCutoff);
  }

  T mSampleRate = static_cast<T>(44100.0);
  T mCutoff = static_cast<T>(1000.0);
  T mQ = static_cast<T>(0.707);

  TPTIntegrator<T> integrator1;
  TPTIntegrator<T> integrator2;
};

} // namespace sea
