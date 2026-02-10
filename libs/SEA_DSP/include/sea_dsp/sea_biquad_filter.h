#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

namespace sea {

enum class FilterType { LowPass, HighPass, BandPass, Notch };

template <typename T> class BiquadFilter {
public:
  BiquadFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    Reset();
    CalculateCoefficients();
  }

  void Reset() {
    z1 = static_cast<T>(0.0);
    z2 = static_cast<T>(0.0);
  }

  void SetParams(FilterType type, T cutoff, T Q) {
    mType = type;

    // ClampCutoff
    if (mSampleRate <= static_cast<T>(0.0)) {
      mCutoff = static_cast<T>(0.0);
    } else {
      T nyquist = static_cast<T>(0.5) * mSampleRate;
      T maxCutoff = nyquist * static_cast<T>(0.49);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    mQ = (Q < static_cast<T>(0.01)) ? static_cast<T>(0.01) : Q;
    CalculateCoefficients();
  }

  SEA_INLINE T Process(T in) {
    T out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
  }

private:
  void CalculateCoefficients() {
    if (mSampleRate == static_cast<T>(0.0))
      return;

    T omega = static_cast<T>(kTwoPi) * mCutoff / mSampleRate;
    T sn = Math::Sin(omega);
    T cs = Math::Cos(omega);
    T alpha = sn / (static_cast<T>(2.0) * mQ);

    T b0_tmp = static_cast<T>(0.0);
    T b1_tmp = static_cast<T>(0.0);
    T b2_tmp = static_cast<T>(0.0);
    T a0_tmp = static_cast<T>(0.0);
    T a1_tmp = static_cast<T>(0.0);
    T a2_tmp = static_cast<T>(0.0);

    T one = static_cast<T>(1.0);
    T two = static_cast<T>(2.0);

    switch (mType) {
    case FilterType::LowPass:
      b0_tmp = (one - cs) / two;
      b1_tmp = one - cs;
      b2_tmp = (one - cs) / two;
      a0_tmp = one + alpha;
      a1_tmp = -two * cs;
      a2_tmp = one - alpha;
      break;
    case FilterType::HighPass:
      b0_tmp = (one + cs) / two;
      b1_tmp = -(one + cs);
      b2_tmp = (one + cs) / two;
      a0_tmp = one + alpha;
      a1_tmp = -two * cs;
      a2_tmp = one - alpha;
      break;
    case FilterType::BandPass:
      b0_tmp = alpha;
      b1_tmp = static_cast<T>(0.0);
      b2_tmp = -alpha;
      a0_tmp = one + alpha;
      a1_tmp = -two * cs;
      a2_tmp = one - alpha;
      break;
    case FilterType::Notch:
      b0_tmp = one;
      b1_tmp = -two * cs;
      b2_tmp = one;
      a0_tmp = one + alpha;
      a1_tmp = -two * cs;
      a2_tmp = one - alpha;
      break;
    default: // Pass through
      b0_tmp = one;
      b1_tmp = static_cast<T>(0.0);
      b2_tmp = static_cast<T>(0.0);
      a0_tmp = one;
      a1_tmp = static_cast<T>(0.0);
      a2_tmp = static_cast<T>(0.0);
      break;
    }

    // Normalize
    T invA0 = one / a0_tmp;
    a0 = b0_tmp * invA0;
    a1 = b1_tmp * invA0;
    a2 = b2_tmp * invA0;
    b1 = a1_tmp * invA0;
    b2 = a2_tmp * invA0;
  }

  T mSampleRate = static_cast<T>(44100.0);
  FilterType mType = FilterType::LowPass;
  T mCutoff = static_cast<T>(1000.0);
  T mQ = static_cast<T>(0.707);

  T a0 = static_cast<T>(0.0);
  T a1 = static_cast<T>(0.0);
  T a2 = static_cast<T>(0.0);
  T b1 = static_cast<T>(0.0);
  T b2 = static_cast<T>(0.0);

  // State
  T z1 = static_cast<T>(0.0);
  T z2 = static_cast<T>(0.0);
};

} // namespace sea
