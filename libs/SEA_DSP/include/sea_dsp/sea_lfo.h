#pragma once
#include "sea_math.h"
#include "sea_platform.h"
#include <algorithm>

#if !defined(SEA_DSP_LFO_BACKEND_SEA_CORE) && !defined(SEA_DSP_LFO_BACKEND_DAISYSP)
#define SEA_DSP_LFO_BACKEND_SEA_CORE
#endif

#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
#include "daisysp.h"
#endif

namespace sea {

class LFO {
public:
  LFO() = default;

  void Init(Real sampleRate) {
    mSampleRate = sampleRate;
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
    mOsc.Init(static_cast<float>(sampleRate));
    mOsc.SetAmp(1.0f);
    ApplyWaveform();
    mOsc.Reset(0.0f);
#else
    mPhase = static_cast<Real>(0.0);
    mPhaseIncrement = static_cast<Real>(0.0);
#endif
  }

  void SetRate(Real hz) {
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
    mOsc.SetFreq(static_cast<float>((hz < static_cast<Real>(0.0))
                                        ? static_cast<Real>(0.0)
                                        : hz));
#else
    if (mSampleRate > static_cast<Real>(0.0)) {
      mPhaseIncrement =
          ((hz < static_cast<Real>(0.0)) ? static_cast<Real>(0.0) : hz) /
          mSampleRate;
    }
#endif
  }

  void SetDepth(Real depth) {
    mDepth = Math::Clamp(depth, static_cast<Real>(0.0), static_cast<Real>(1.0));
  }

  void SetWaveform(int type) {
    mWaveform = type;
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
    ApplyWaveform();
#endif
  }

  SEA_INLINE Real Process() {
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
    return static_cast<Real>(mOsc.Process()) * mDepth;
#else
    Real out = static_cast<Real>(0.0);

    switch (mWaveform) {
    case 0: // Sine
      out = Math::Sin(kTwoPi * mPhase);
      break;
    case 1: // Triangle
      out = (static_cast<Real>(4.0) *
             Math::Abs(mPhase - static_cast<Real>(0.5))) -
            static_cast<Real>(1.0);
      break;
    case 2: // Square
      out = (mPhase < static_cast<Real>(0.5)) ? static_cast<Real>(1.0)
                                              : static_cast<Real>(-1.0);
      break;
    case 3: // Saw
      out = (static_cast<Real>(2.0) * mPhase) - static_cast<Real>(1.0);
      break;
    default:
      out = static_cast<Real>(0.0);
      break;
    }

    mPhase += mPhaseIncrement;
    if (mPhase >= static_cast<Real>(1.0)) {
      mPhase -= static_cast<Real>(1.0);
    }

    return out * mDepth;
#endif
  }

private:
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
  void ApplyWaveform() {
    switch (mWaveform) {
    case 0:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SIN);
      break;
    case 1:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_TRI);
      break;
    case 2:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SQUARE);
      break;
    case 3:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_RAMP);
      break;
    default:
      mOsc.SetWaveform(daisysp::Oscillator::WAVE_SIN);
      break;
    }
  }
#endif

  Real mSampleRate = static_cast<Real>(44100.0);
#ifndef SEA_DSP_LFO_BACKEND_DAISYSP
  Real mPhase = static_cast<Real>(0.0);
  Real mPhaseIncrement = static_cast<Real>(0.0);
#endif
  Real mDepth = static_cast<Real>(0.0);
  int mWaveform = 0;
#ifdef SEA_DSP_LFO_BACKEND_DAISYSP
  daisysp::Oscillator mOsc;
#endif
};

} // namespace sea
