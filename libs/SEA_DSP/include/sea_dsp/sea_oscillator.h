#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

namespace sea {

class Oscillator {
public:
  enum class WaveformType { Saw, Square, Triangle, Sine };

  Oscillator() = default;

  /**
   * @brief Initialize with sample rate.
   */
  void Init(Real sampleRate) {
    mSampleRate = sampleRate;
    mPhase = static_cast<Real>(0.0);
    mPhaseIncrement = static_cast<Real>(0.0);
  }

  /**
   * @brief Reset phase.
   */
  void Reset() { mPhase = static_cast<Real>(0.0); }

  /**
   * @brief Set frequency in Hz.
   */
  void SetFrequency(Real freq) {
    if (mSampleRate > static_cast<Real>(0.0)) {
      mPhaseIncrement = freq / mSampleRate;
    }
  }

  /**
   * @brief Set waveform type.
   */
  void SetWaveform(WaveformType type) { mWaveform = type; }

  /**
   * @brief Set pulse width for square wave.
   * Range: (0.0, 1.0)
   */
  void SetPulseWidth(Real pw) {
    mPulseWidth =
        Math::Clamp(pw, static_cast<Real>(0.01), static_cast<Real>(0.99));
  }

  /**
   * @brief Render one sample.
   * Range: [-1.0, 1.0]
   */
  SEA_INLINE Real Process() {
    Real out = static_cast<Real>(0.0);

    switch (mWaveform) {
    case WaveformType::Saw:
      out = (static_cast<Real>(2.0) * mPhase) - static_cast<Real>(1.0);
      break;
    case WaveformType::Square:
      out = (mPhase < mPulseWidth) ? static_cast<Real>(1.0)
                                   : static_cast<Real>(-1.0);
      break;
    case WaveformType::Triangle:
      out = (static_cast<Real>(4.0) *
             Math::Abs(mPhase - static_cast<Real>(0.5))) -
            static_cast<Real>(1.0);
      break;
    case WaveformType::Sine:
      out = Math::Sin(kTwoPi * mPhase);
      break;
    }

    mPhase += mPhaseIncrement;
    if (mPhase >= static_cast<Real>(1.0)) {
      mPhase -= static_cast<Real>(1.0);
    }

    return out;
  }

private:
  Real mSampleRate = static_cast<Real>(44100.0);
  Real mPhase = static_cast<Real>(0.0);
  Real mPhaseIncrement = static_cast<Real>(0.0);
  Real mPulseWidth = static_cast<Real>(0.5);
  WaveformType mWaveform = WaveformType::Saw;
};

} // namespace sea
