#pragma once

#include "../types.h"
#include <algorithm>
#include <cmath>

namespace PolySynth {

enum class FilterType { LowPass, HighPass, BandPass, Notch };

class BiquadFilter {
public:
  BiquadFilter() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    Reset();
    CalculateCoefficients();
  }

  void Reset() {
    z1 = 0.0;
    z2 = 0.0;
  }

  void SetParams(FilterType type, double cutoff, double Q) {
    mType = type;
    mCutoff = cutoff;
    mQ = std::max(0.01, Q); // Safety
    CalculateCoefficients();
  }

  inline sample_t Process(sample_t in) {
    double out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return (sample_t)out;
  }

private:
  void CalculateCoefficients() {
    if (mSampleRate == 0.0)
      return;

    double omega = kTwoPi * mCutoff / mSampleRate;
    double sn = std::sin(omega);
    double cs = std::cos(omega);
    double alpha = sn / (2.0 * mQ);

    double b0_tmp = 0.0;
    double b1_tmp = 0.0;
    double b2_tmp = 0.0;
    double a0_tmp = 0.0;
    double a1_tmp = 0.0;
    double a2_tmp = 0.0;

    switch (mType) {
    case FilterType::LowPass:
      b0_tmp = (1.0 - cs) / 2.0;
      b1_tmp = 1.0 - cs;
      b2_tmp = (1.0 - cs) / 2.0;
      a0_tmp = 1.0 + alpha;
      a1_tmp = -2.0 * cs;
      a2_tmp = 1.0 - alpha;
      break;
    case FilterType::HighPass:
      b0_tmp = (1.0 + cs) / 2.0;
      b1_tmp = -(1.0 + cs);
      b2_tmp = (1.0 + cs) / 2.0;
      a0_tmp = 1.0 + alpha;
      a1_tmp = -2.0 * cs;
      a2_tmp = 1.0 - alpha;
      break;
    default: // Pass through
      b0_tmp = 1.0;
      b1_tmp = 0.0;
      b2_tmp = 0.0;
      a0_tmp = 1.0;
      a1_tmp = 0.0;
      a2_tmp = 0.0;
      break;
    }

    // Normalize
    double invA0 = 1.0 / a0_tmp;
    a0 = b0_tmp * invA0;
    a1 = b1_tmp * invA0;
    a2 = b2_tmp * invA0;
    b1 = a1_tmp * invA0;
    b2 = a2_tmp * invA0;
  }

  double mSampleRate = 44100.0;
  FilterType mType = FilterType::LowPass;
  double mCutoff = 1000.0;
  double mQ = 0.707;

  double a0 = 0.0, a1 = 0.0, a2 = 0.0;
  double b1 = 0.0, b2 = 0.0;

  // State
  double z1 = 0.0, z2 = 0.0;
};

} // namespace PolySynth
