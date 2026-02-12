#pragma once

#include "../sea_delay_line.h"
#include "../sea_one_pole_filter.h"
#include "../sea_sigmoid.h"
#include "../sea_lfo.h"
#include "../sea_math.h"
#include <algorithm>

namespace sea {

template <typename T>
class VintageDelay {
public:
  VintageDelay()
    : sample_rate_(T(48000)),
      time_ms_(T(100)),
      feedback_percent_(T(50)),
      mix_percent_(T(50)) {}

  // Managed mode: Allocates internal buffers
  void Init(T sample_rate, T max_delay_ms) {
    sample_rate_ = sample_rate;

    // Calculate buffer size
    size_t max_samples = static_cast<size_t>((max_delay_ms / T(1000)) * sample_rate);
    max_samples += static_cast<size_t>(sample_rate * T(0.02)); // Add headroom for drift

    // Initialize delay lines
    delay_left_.Init(max_samples);
    delay_right_.Init(max_samples);

    // Initialize feedback filters (2.5kHz fixed)
    filter_left_.Init(sample_rate);
    filter_right_.Init(sample_rate);
    filter_left_.SetCutoff(T(2500));
    filter_right_.SetCutoff(T(2500));

    // Initialize drift LFO (~0.2Hz, ~1ms depth)
    lfo_drift_.Init(static_cast<Real>(sample_rate));
    lfo_drift_.SetWaveform(0); // Sine
    lfo_drift_.SetRate(static_cast<Real>(0.2));
    lfo_drift_.SetDepth(static_cast<Real>(1.0));

    SetTime(T(100));
    SetFeedback(T(50));
    SetMix(T(50));
  }

  void SetTime(T time_ms) {
    time_ms_ = Math::Clamp(time_ms, T(10), T(2000));
  }

  void SetFeedback(T feedback_percent) {
    feedback_percent_ = Math::Clamp(feedback_percent, T(0), T(150));
  }

  void SetMix(T mix_percent) {
    mix_percent_ = Math::Clamp(mix_percent, T(0), T(100));
  }

  void Process(T input_l, T input_r, T* out_l, T* out_r) {
    // Get drift modulation
    Real drift_val = lfo_drift_.Process();
    T drift_ms = static_cast<T>(drift_val); // ~±1ms

    // Convert times to samples
    T time_samples_l = (time_ms_ + drift_ms) * sample_rate_ / T(1000);
    T time_samples_r = (time_ms_ + T(15) + drift_ms) * sample_rate_ / T(1000); // +15ms offset

    // Read delayed signals
    T delayed_l = delay_left_.Read(time_samples_l);
    T delayed_r = delay_right_.Read(time_samples_r);

    // Apply filtering in feedback path ONLY (not in output path)
    T filtered_l = filter_left_.Process(delayed_l);
    T filtered_r = filter_right_.Process(delayed_r);

    // Apply saturation in feedback path
    T feedback_gain = feedback_percent_ / T(100);
    T feedback_l = filtered_l * feedback_gain;
    T feedback_r = filtered_r * feedback_gain;

    feedback_l = Sigmoid<T>::SoftClipCubic(feedback_l);
    feedback_r = Sigmoid<T>::SoftClipCubic(feedback_r);

    // Mix input with feedback and write to delay line
    T delay_input_l = input_l + feedback_l;
    T delay_input_r = input_r + feedback_r;

    delay_left_.Push(delay_input_l);
    delay_right_.Push(delay_input_r);

    // Unity Dry + Variable Wet mix topology (use UNFILTERED delayed signal for output)
    T mix_gain = mix_percent_ / T(100);
    *out_l = input_l + delayed_l * mix_gain;
    *out_r = input_r + delayed_r * mix_gain;
  }

  void Clear() {
    // Re-initialize to clear buffers
    T sr = sample_rate_;
    T max_ms = time_ms_ * T(2); // Estimate max
    Init(sr, max_ms);
  }

private:
  DelayLine<T> delay_left_;
  DelayLine<T> delay_right_;
  OnePoleFilter<T> filter_left_;
  OnePoleFilter<T> filter_right_;
  LFO lfo_drift_;

  T sample_rate_;
  T time_ms_;
  T feedback_percent_;
  T mix_percent_;
};

} // namespace sea
