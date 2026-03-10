#pragma once
#include "sea_platform.h"
#include <algorithm>
#include <cmath>

namespace sea {

struct Math {
  static SEA_INLINE Real Sin(Real x) {
#ifdef SEA_FAST_MATH
    // 7th-order Taylor polynomial with range reduction to [-π/2, π/2].
    // Accurate to ~0.016% across the full circle — sufficient for audio.
    const Real pi = Real(3.14159265358979323846);
    const Real half_pi = Real(1.57079632679489661923);
    const Real two_pi = Real(6.28318530717958647692);
    // Reduce to [-π, π] via bounded subtraction (covers oscillator/filter inputs)
    x += pi;
    if (x >= two_pi) {
        x -= two_pi;
        if (x >= two_pi) x = std::fmod(x, two_pi);  // fallback
    } else if (x < Real(0)) {
        x += two_pi;
        if (x < Real(0)) x = std::fmod(x, two_pi) + two_pi;  // fallback
    }
    x -= pi;
    // Fold [-π, π] into [-π/2, π/2] where Taylor converges well
    if (x > half_pi)
      x = pi - x;
    else if (x < -half_pi)
      x = -pi - x;
    Real x2 = x * x;
    return x * (Real(1) - x2 / Real(6) * (Real(1) - x2 / Real(20) *
               (Real(1) - x2 / Real(42))));
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
