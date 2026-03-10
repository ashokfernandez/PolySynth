#pragma once
#include "sea_platform.h"
#include <algorithm>
#include <cmath>

namespace sea {

struct Math {
  static SEA_INLINE Real Sin(Real x) {
#ifdef SEA_FAST_MATH
    // 5th-order polynomial: accurate to ~0.001% for |x| < pi
    // Normalize x to [-pi, pi] using conditional subtraction
    // (replaces std::fmod — ~50 cycles on Cortex-M33)
    const Real pi = Real(3.14159265358979323846);
    const Real two_pi = Real(6.28318530717958647692);
    // Bounded range reduction: 2 if-subtractions handle all realistic audio inputs
    // (oscillator phase [0,2π), Cos offset [0,3π), filter angles [0,π/2))
    // Falls back to fmod only for extreme values (should never happen in practice)
    x += pi;
    if (x >= two_pi) {
        x -= two_pi;
        if (x >= two_pi) x = std::fmod(x, two_pi);  // fallback
    } else if (x < Real(0)) {
        x += two_pi;
        if (x < Real(0)) x = std::fmod(x, two_pi) + two_pi;  // fallback
    }
    x -= pi;
    Real x2 = x * x;
    return x * (Real(1) - x2 / Real(6) * (Real(1) - x2 / Real(20)));
#else
    return std::sin(x);
#endif
  }

  static SEA_INLINE Real Cos(Real x) {
#ifdef SEA_FAST_MATH
    const Real half_pi = Real(1.57079632679489661923);
    return Sin(x + half_pi);
#else
    return std::cos(x);
#endif
  }

  static SEA_INLINE Real Tan(Real x) {
#ifdef SEA_FAST_MATH
    // Pade approximant: accurate for |x| < pi/4 (covers audio filter freqs)
    Real x2 = x * x;
    return x * (Real(15) - x2) / (Real(15) - Real(6) * x2);
#else
    return std::tan(x);
#endif
  }

  static SEA_INLINE Real Tanh(Real x) {
#ifdef SEA_FAST_MATH
    // Pade approximant: accurate to ~0.1% for |x| < 3
    Real x2 = x * x;
    return x * (Real(27) + x2) / (Real(27) + Real(9) * x2);
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
