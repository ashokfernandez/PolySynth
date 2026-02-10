#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

#if !defined(SEA_DSP_OSC_BACKEND_SEA_CORE) && !defined(SEA_DSP_OSC_BACKEND_DAISYSP)
#define SEA_DSP_OSC_BACKEND_SEA_CORE
#endif

#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
#include "daisysp.h"
#endif

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
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    mOsc.Init(static_cast<float>(sampleRate));
    mOsc.SetAmp(1.0f);
    ApplyWaveform();
    mOsc.SetPw(static_cast<float>(mPulseWidth));
    mOsc.Reset(0.0f);
#else
    mPhase = static_cast<Real>(0.0);
    mPhaseIncrement = static_cast<Real>(0.0);
#endif
  }

  /**
   * @brief Reset phase.
   */
  void Reset() {
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    mOsc.Reset(0.0f);
#else
    mPhase = static_cast<Real>(0.0);
#endif
  }

  /**
   * @brief Set frequency in Hz.
   */
  void SetFrequency(Real freq) {
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    mOsc.SetFreq(static_cast<float>(freq));
#else
    if (mSampleRate > static_cast<Real>(0.0)) {
      mPhaseIncrement = freq / mSampleRate;
    }
#endif
  }

  /**
   * @brief Set waveform type.
   */
  void SetWaveform(WaveformType type) {
    mWaveform = type;
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    ApplyWaveform();
#endif
  }

  /**
   * @brief Set pulse width for square wave.
   * Range: (0.0, 1.0)
   */
  void SetPulseWidth(Real pw) {
    mPulseWidth =
        Math::Clamp(pw, static_cast<Real>(0.01), static_cast<Real>(0.99));
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    mOsc.SetPw(static_cast<float>(mPulseWidth));
#endif
  }

  /**
   * @brief Render one sample.
   * Range: [-1.0, 1.0]
   */
  SEA_INLINE Real Process() {
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
    Real out = static_cast<Real>(mOsc.Process());
    if (mWaveform == WaveformType::Saw) {
      out = -out;
    }
    return out;
#else
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
#endif
  }

private:
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
  void ApplyWaveform() {
    switch (mWaveform) {
    case WaveformType::Saw:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SAW);
      break;
    case WaveformType::Square:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SQUARE);
      break;
    case WaveformType::Triangle:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_TRI);
      break;
    case WaveformType::Sine:
    default:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SIN);
      break;
    }
  }
#endif

  Real mSampleRate = static_cast<Real>(44100.0);
#ifndef SEA_DSP_OSC_BACKEND_DAISYSP
  Real mPhase = static_cast<Real>(0.0);
  Real mPhaseIncrement = static_cast<Real>(0.0);
#endif
  Real mPulseWidth = static_cast<Real>(0.5);
  WaveformType mWaveform = WaveformType::Saw;
#ifdef SEA_DSP_OSC_BACKEND_DAISYSP
  daisysp::Oscillator mOsc;
#endif
};

} // namespace sea
