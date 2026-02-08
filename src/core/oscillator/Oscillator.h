#pragma once

#include "../types.h"
#include <cmath>

namespace PolySynthCore {

class Oscillator {
public:
  enum class WaveformType { Saw, Square, Triangle, Sine };

  Oscillator() = default;

  /**
   * @brief Initialize with sample rate.
   */
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    mPhase = 0.0;
    mPhaseIncrement = 0.0;
  }

  /**
   * @brief Reset phase.
   */
  void Reset() { mPhase = 0.0; }

  /**
   * @brief Set frequency in Hz.
   */
  void SetFrequency(double freq) {
    if (mSampleRate > 0.0) {
      mPhaseIncrement = freq / mSampleRate;
    }
  }

  /**
   * @brief Set waveform type.
   */
  void SetWaveform(WaveformType type) { mWaveform = type; }

  /**
   * @brief Render one sample.
   * Range: [-1.0, 1.0]
   */
  inline sample_t Process() {
    constexpr double kTwoPi = 6.28318530717958647692;
    sample_t out = 0.0;

    switch (mWaveform) {
    case WaveformType::Saw:
      // Naive Sawtooth: (2.0 * phase) - 1.0
      out = (sample_t)((2.0 * mPhase) - 1.0);
      break;
    case WaveformType::Square:
      out = (mPhase < 0.5) ? 1.0 : -1.0;
      break;
    case WaveformType::Triangle:
      out = (sample_t)((4.0 * std::abs(mPhase - 0.5)) - 1.0);
      break;
    case WaveformType::Sine:
      out = (sample_t)std::sin(kTwoPi * mPhase);
      break;
    }

    mPhase += mPhaseIncrement;
    if (mPhase >= 1.0) {
      mPhase -= 1.0;
    }

    return out;
  }

private:
  double mSampleRate = 44100.0;
  double mPhase = 0.0;
  double mPhaseIncrement = 0.0;
  WaveformType mWaveform = WaveformType::Saw;
};

} // namespace PolySynthCore
