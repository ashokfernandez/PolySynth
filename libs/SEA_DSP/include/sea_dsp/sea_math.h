#pragma once
#include "sea_platform.h"
#include "sea_wavetable.h"
#include <algorithm>
#include <cmath>

namespace sea {

struct Math {
  static SEA_INLINE Real Sin(Real x) {
#ifdef SEA_FAST_MATH
    // Wavetable lookup with linear interpolation.
    // 512 entries gives <0.01% max error — better than previous Taylor (0.016%).
    const Real two_pi = Real(6.28318530717958647692);
    // Range reduction to [0, 2π)
    x = std::fmod(x, two_pi);
    if (x < Real(0))
      x += two_pi;
    return SinWavetable<Real>::Lookup(x);
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

  /**
   * @brief Compute 2^x (base-2 exponentiation).
   *
   * On RP2350 (Cortex-M33) with GCC 15, std::pow(2,x) returns incorrect
   * results when deeply inlined into __time_critical_func SRAM-placed code.
   * The volatile barrier prevents the problematic inlining while keeping
   * the call to the correct libm function.
   */
  static SEA_INLINE Real Exp2(Real x) {
#ifdef SEA_PLATFORM_EMBEDDED
    volatile Real base = Real(2);
    return std::pow(base, x);
#else
    return std::pow(Real(2), x);
#endif
  }

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
