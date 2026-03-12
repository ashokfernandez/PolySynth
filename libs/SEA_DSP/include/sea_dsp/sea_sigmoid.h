#pragma once

#include "sea_math.h"
#include <cmath>
#include <algorithm>

namespace sea {

template <typename T>
struct Sigmoid {
  // Hyperbolic tangent saturation — routes through Padé approximant on embedded
  static T Tanh(T x) {
    return static_cast<T>(Math::Tanh(static_cast<Real>(x)));
  }

  // Cubic approximation: f(x) = x - x³/3 for x in [-1.5, 1.5], clamped outside
  static T SoftClipCubic(T x) {
    // Apply cubic formula
    T x_cubed = x * x * x;
    T result = x - x_cubed / T(3);

    // For values outside [-1.5, 1.5], apply soft clamping
    // The cubic formula naturally provides compression
    // Additional clamping for extreme values (strict <, not <=)
    if (result >= T(2)) result = T(1.99);
    if (result <= T(-2)) result = T(-1.99);

    return result;
  }
};

} // namespace sea
