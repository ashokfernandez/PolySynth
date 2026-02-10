#pragma once
#include "sea_platform.h"
#include <algorithm>
#include <cmath>

namespace sea {

struct Math {
  static SEA_INLINE Real Sin(Real x) { return std::sin(x); }

  static SEA_INLINE Real Cos(Real x) { return std::cos(x); }

  static SEA_INLINE Real Tan(Real x) { return std::tan(x); }

  static SEA_INLINE Real Tanh(Real x) {
#ifdef SEA_FAST_MATH
    // Optimized approximation for embedded (if needed later)
    // For now, default to std::tanh for correctness
    return std::tanh(x);
#else
    return std::tanh(x);
#endif
  }

  static SEA_INLINE Real Exp(Real x) { return std::exp(x); }

  static SEA_INLINE Real Pow(Real base, Real exp) {
    return std::pow(base, exp);
  }

  static SEA_INLINE Real Abs(Real x) { return std::abs(x); }

  template <typename T> static SEA_INLINE T Clamp(T x, T min, T max) {
    return std::clamp(x, min, max);
  }

  static SEA_INLINE Real Sqrt(Real x) { return std::sqrt(x); }

  static SEA_INLINE Real Log(Real x) { return std::log(x); }

  static SEA_INLINE Real Fmod(Real x, Real y) { return std::fmod(x, y); }

  static SEA_INLINE Real Ceil(Real x) { return std::ceil(x); }

  static SEA_INLINE Real Floor(Real x) { return std::floor(x); }
};

} // namespace sea
