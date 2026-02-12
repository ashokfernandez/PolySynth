#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace sea {

template <typename T>
class OnePoleFilter {
public:
  OnePoleFilter() : sample_rate_(T(48000)), b1_(T(0)), z1_(T(0)) {}

  void Init(T sample_rate) {
    sample_rate_ = sample_rate;
    z1_ = T(0);
    b1_ = T(0);
  }

  void SetCutoff(T cutoff_hz) {
    // Clamp cutoff to valid range (minimum 0.001 Hz to avoid zero coefficient)
    if (cutoff_hz < T(0.001)) cutoff_hz = T(0.001);

    // Clamp to 0.49 * sample_rate (below Nyquist for stability)
    T max_cutoff = T(0.49) * sample_rate_;
    if (cutoff_hz > max_cutoff) cutoff_hz = max_cutoff;

    // Exact impulse-invariant formula: b1 = 1.0 - exp(-2*pi*fc/fs)
    T omega = T(2) * static_cast<T>(M_PI) * cutoff_hz / sample_rate_;
    b1_ = T(1) - std::exp(-omega);
  }

  T Process(T input) {
    // One-pole low-pass filter (leaky integrator)
    // y[n] = y[n-1] + b1 * (x[n] - y[n-1])
    z1_ = z1_ + b1_ * (input - z1_);
    return z1_;
  }

private:
  T sample_rate_;
  T b1_;  // Filter coefficient
  T z1_;  // State variable
};

} // namespace sea
