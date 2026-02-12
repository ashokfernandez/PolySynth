#pragma once

#include "../sea_delay_line.h"
#include "../sea_one_pole_filter.h"
#include "../sea_lfo.h"
#include "../sea_math.h"
#include <algorithm>

namespace sea {

template <typename T>
class VintageChorus {
public:
  VintageChorus() : sample_rate_(T(48000)), rate_(T(1)), depth_ms_(T(2)), mix_(T(0.5)) {}

  // Managed mode: Allocates internal buffers
  void Init(T sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize delay lines (max ~50ms for chorus)
    size_t max_samples = static_cast<size_t>(sample_rate * T(0.05)); // 50ms
    delay_left_.Init(max_samples);
    delay_right_.Init(max_samples);

    // Initialize filters
    filter_left_.Init(sample_rate);
    filter_right_.Init(sample_rate);
    filter_left_.SetCutoff(T(8000)); // 8kHz cutoff for darkening
    filter_right_.SetCutoff(T(8000));

    // Initialize LFOs
    lfo_main_.Init(static_cast<Real>(sample_rate));
    lfo_main_.SetWaveform(1); // Triangle (WAVE_TRI = 1)
    lfo_main_.SetDepth(static_cast<Real>(1.0));

    lfo_turbo_.Init(static_cast<Real>(sample_rate));
    lfo_turbo_.SetWaveform(0); // Sine for turbo wobble
    lfo_turbo_.SetRate(static_cast<Real>(6.0)); // ~6Hz
    lfo_turbo_.SetDepth(static_cast<Real>(0.3)); // Low amplitude

    SetRate(T(1));
    SetDepth(T(2));
    SetMix(T(0.5));
  }

  void SetRate(T rate_hz) {
    rate_ = Math::Clamp(rate_hz, T(0.1), T(10.0));
    lfo_main_.SetRate(static_cast<Real>(rate_));
  }

  void SetDepth(T depth_ms) {
    depth_ms_ = Math::Clamp(depth_ms, T(0), T(5.0));
  }

  void SetMix(T mix) {
    mix_ = Math::Clamp(mix, T(0), T(1.0));
  }

  void Process(T input_l, T input_r, T* out_l, T* out_r) {
    // Get LFO values
    Real lfo_val = lfo_main_.Process();

    // Turbo mode: Add secondary wobble when depth > 4ms
    if (depth_ms_ > T(4.0)) {
      Real turbo_val = lfo_turbo_.Process();
      lfo_val += turbo_val * static_cast<Real>(0.3);
    }

    // Convert depth from ms to samples
    T depth_samples = depth_ms_ * sample_rate_ / T(1000);

    // Base delay (10-15ms range)
    T base_delay = T(0.012) * sample_rate_; // 12ms base

    // Calculate stereo delays with phase inversion
    T mod_l = static_cast<T>(lfo_val) * depth_samples;
    T mod_r = -static_cast<T>(lfo_val) * depth_samples; // Inverted

    T delay_time_l = base_delay + mod_l;
    T delay_time_r = base_delay + mod_r;

    // Clamp delays to valid range
    delay_time_l = Math::Clamp(delay_time_l, T(0), T(0.045) * sample_rate_);
    delay_time_r = Math::Clamp(delay_time_r, T(0), T(0.045) * sample_rate_);

    // Write to delay lines
    delay_left_.Push(input_l);
    delay_right_.Push(input_r);

    // Read from delay lines
    T wet_l = delay_left_.Read(delay_time_l);
    T wet_r = delay_right_.Read(delay_time_r);

    // Apply tone shaping (darken the wet signal)
    wet_l = filter_left_.Process(wet_l);
    wet_r = filter_right_.Process(wet_r);

    // Mix dry and wet
    *out_l = input_l * (T(1) - mix_) + wet_l * mix_;
    *out_r = input_r * (T(1) - mix_) + wet_r * mix_;
  }

private:
  DelayLine<T> delay_left_;
  DelayLine<T> delay_right_;
  OnePoleFilter<T> filter_left_;
  OnePoleFilter<T> filter_right_;
  LFO lfo_main_;
  LFO lfo_turbo_;

  T sample_rate_;
  T rate_;
  T depth_ms_;
  T mix_;
};

} // namespace sea
