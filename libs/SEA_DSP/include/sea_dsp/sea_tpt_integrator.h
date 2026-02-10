#pragma once
#include "sea_math.h"
#include "sea_platform.h"

namespace sea {

template <typename T> class TPTIntegrator {
public:
  TPTIntegrator() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    Reset();
  }

  void Reset() { s = static_cast<T>(0.0); }

  // Prepare the integrator for the current sample
  // cutoff: Cutoff frequency in Hz
  SEA_INLINE void Prepare(T cutoff) {
    if (mSampleRate <= static_cast<T>(0.0)) {
      g = static_cast<T>(0.0);
      return;
    }

    // Clamp cutoff to prevent instability
    T nyquist = static_cast<T>(0.5) * mSampleRate;
    T maxCutoff = nyquist * static_cast<T>(0.49);

    if (cutoff < static_cast<T>(0.0))
      cutoff = static_cast<T>(0.0);
    else if (cutoff > maxCutoff)
      cutoff = maxCutoff;

    T wd = static_cast<T>(kTwoPi) * cutoff;
    T T_period = static_cast<T>(1.0) / mSampleRate;
    T wa = (static_cast<T>(2.0) / T_period) *
           Math::Tan(wd * T_period / static_cast<T>(2.0));
    g = wa * T_period / static_cast<T>(2.0);
  }

  // Process the input sample and return the output
  // in: Input sample
  SEA_INLINE T Process(T in) {
    // y = g * in + s
    T y = g * in + s;

    // Update state: s = g * in + y
    // Matches the original implementation logic: s = (sample_t)(g * in + y);
    s = g * in + y;

    return y;
  }

  // Get the instantaneous gain G
  SEA_INLINE T GetG() const { return g; }

  // Get the current state S
  SEA_INLINE T GetS() const { return s; }

  // Set the state S (needed for some ladder implementations)
  SEA_INLINE void SetS(T newState) { s = newState; }

private:
  T mSampleRate = static_cast<T>(44100.0);
  T s = static_cast<T>(0.0); // State
  T g = static_cast<T>(0.0); // Instantaneous gain
};

} // namespace sea
