#pragma once
#include "sea_tpt_integrator.h"
#include <algorithm>

namespace sea {

template <typename T> class LadderFilter {
public:
  enum class Model { Transistor, Diode };

  LadderFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    if (sampleRate > static_cast<T>(0.0)) {
      mInvSampleRate = static_cast<T>(1.0) / sampleRate;
      mTwoTimesSampleRate = static_cast<T>(2.0) * sampleRate;
    }
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
      T maxCutoff = nyquist * static_cast<T>(0.95);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    mResonance =
        Math::Clamp(resonance, static_cast<T>(0.0), static_cast<T>(1.2));

    // Prepare G — compute Tan once, share with all integrators via SetG
    if (mSampleRate <= static_cast<T>(0.0)) {
      g = static_cast<T>(0.0);
      mInv1PlusG = static_cast<T>(1.0);
      mInvFeedbackDenom = static_cast<T>(1.0);
      return;
    }
    T wd = static_cast<T>(kTwoPi) * mCutoff;
    T wa = mTwoTimesSampleRate *
           Math::Tan(wd * mInvSampleRate * static_cast<T>(0.5));
    g = wa * mInvSampleRate * static_cast<T>(0.5);

    // Cache reciprocal: eliminates 9 divisions per ProcessTransistor call
    mInv1PlusG = static_cast<T>(1.0) / (static_cast<T>(1.0) + g);

    // Cache feedback denominator reciprocal: 1/(1 + k * BETA)
    T G_lpf = g * mInv1PlusG;
    T BETA = G_lpf * G_lpf * G_lpf * G_lpf;
    T k = mResonance * static_cast<T>(4.0);
    mInvFeedbackDenom = static_cast<T>(1.0) / (static_cast<T>(1.0) + k * BETA);

    // Share g with integrators directly (avoids 4 redundant Tan computations)
    for (int i = 0; i < 4; ++i) {
      integrators[i].SetG(g);
    }
  }

  SEA_INLINE T Process(T in) {
    // The original instruction snippet was syntactically incorrect.
    // Assuming the intent was to ensure the filter model is set or used.
    // The class already uses `mModel` set in `SetParams`.
    // No change to the logic of Process(T in) based on the provided snippet,
    // as it would introduce undefined symbols (`mFilterModel`, `FilterModel`)
    // and break the function's structure.
    if (mModel == Model::Transistor) {
      return ProcessTransistor(in);
    } else {
      return ProcessDiode(in);
    }
  }

private:
  SEA_INLINE T ProcessTransistor(T in) {
    // 4 cascaded 1-pole LPFs with global feedback.
    // Uses cached mInv1PlusG and mInvFeedbackDenom (set in SetParams)
    // to replace 9 divisions with multiplications.
    T G_lpf = g * mInv1PlusG;

    // Accumulate S terms
    T S[4];
    for (int i = 0; i < 4; ++i)
      S[i] = integrators[i].GetS() * mInv1PlusG;

    T S_total = G_lpf * G_lpf * G_lpf * S[0] + G_lpf * G_lpf * S[1] +
                G_lpf * S[2] + S[3];

    T k = mResonance * static_cast<T>(4.0);

    // Per-sample tanh: nonlinear waveshaping, input varies per sample
    T u = (Math::Tanh(in) - k * S_total) * mInvFeedbackDenom;

    // Compute stages
    T v = Math::Tanh(u);
    for (int i = 0; i < 4; ++i) {
      T v_out = (g * v + integrators[i].GetS()) * mInv1PlusG;
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
  T mInvSampleRate = static_cast<T>(1.0) / static_cast<T>(44100.0);
  T mTwoTimesSampleRate = static_cast<T>(2.0) * static_cast<T>(44100.0);
  Model mModel = Model::Transistor;
  T mCutoff = static_cast<T>(1000.0);
  T mResonance = static_cast<T>(0.0);
  T g = static_cast<T>(0.0);
  T mInv1PlusG = static_cast<T>(1.0);
  T mInvFeedbackDenom = static_cast<T>(1.0);

  TPTIntegrator<T> integrators[4];
};

} // namespace sea
