#pragma once
#include "sea_tpt_integrator.h"
#include <algorithm>
#include <vector>

namespace sea {

template <typename T> class LadderFilter {
public:
  enum class Model { Transistor, Diode };

  LadderFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    for (int i = 0; i < 4; ++i)
      integrators[i].Init(sampleRate);
    Reset();
  }

  void Reset() {
    for (int i = 0; i < 4; ++i)
      integrators[i].Reset();
  }

  void SetParams(Model model, T cutoff, T resonance) {
    mModel = model;

    // ClampCutoff logic
    if (mSampleRate <= static_cast<T>(0.0)) {
      mCutoff = static_cast<T>(0.0);
    } else {
      T nyquist = static_cast<T>(0.5) * mSampleRate;
      T maxCutoff = nyquist * static_cast<T>(0.49);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    mResonance =
        Math::Clamp(resonance, static_cast<T>(0.0), static_cast<T>(1.2));

    // Prepare G
    if (mSampleRate <= static_cast<T>(0.0)) {
      g = static_cast<T>(0.0);
      return;
    }
    T wd = static_cast<T>(kTwoPi) * mCutoff;
    T T_period = static_cast<T>(1.0) / mSampleRate;
    T wa = (static_cast<T>(2.0) / T_period) *
           Math::Tan(wd * T_period / static_cast<T>(2.0));

    g = wa * T_period / static_cast<T>(2.0);

    for (int i = 0; i < 4; ++i) {
      integrators[i].Prepare(mCutoff);
    }
  }

  SEA_INLINE T Process(T in) {
    if (mModel == Model::Transistor) {
      return ProcessTransistor(in);
    } else {
      return ProcessDiode(in);
    }
  }

private:
  SEA_INLINE T ProcessTransistor(T in) {
    // 4 cascaded 1-pole LPFs with global feedback.
    T G_lpf = g / (static_cast<T>(1.0) + g);
    T BETA = G_lpf * G_lpf * G_lpf * G_lpf;

    // Accumulate S terms
    T S[4];
    for (int i = 0; i < 4; ++i)
      S[i] = integrators[i].GetS() / (static_cast<T>(1.0) + g);

    T S_total = G_lpf * G_lpf * G_lpf * S[0] + G_lpf * G_lpf * S[1] +
                G_lpf * S[2] + S[3];

    T k = mResonance * static_cast<T>(4.0);

    T u = (Math::Tanh(in) - k * S_total) / (static_cast<T>(1.0) + k * BETA);

    // Compute stages
    T v = Math::Tanh(u);
    for (int i = 0; i < 4; ++i) {
      T v_out = (g * v + integrators[i].GetS()) / (static_cast<T>(1.0) + g);
      v_out = Math::Tanh(v_out);
      integrators[i].SetS(static_cast<T>(2.0) * v_out - integrators[i].GetS());
      v = v_out;
    }

    return v;
  }

  SEA_INLINE T ProcessDiode(T in) {
    // Fallback to Transistor for now, matching original implementation
    return ProcessTransistor(in);
  }

  T mSampleRate = static_cast<T>(44100.0);
  Model mModel = Model::Transistor;
  T mCutoff = static_cast<T>(1000.0);
  T mResonance = static_cast<T>(0.0);
  T g = static_cast<T>(0.0);

  TPTIntegrator<T> integrators[4];
};

} // namespace sea
