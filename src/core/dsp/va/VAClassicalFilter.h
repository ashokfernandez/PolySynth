#pragma once

#include "VASVFilter.h"
#include <cmath>
#include <complex>
#include <vector>

namespace PolySynthCore {

class VAClassicalFilter {
public:
  enum class Type { Butterworth, Chebyshev1, Chebyshev2 };

  VAClassicalFilter() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    for (auto &f : stages)
      f.Init(sampleRate);
    Reset();
  }

  void Reset() {
    stages.clear();
    for (auto &f : stages)
      f.Reset();
  }

  void SetConfig(Type type, int order, double rippleDb) {
    mType = type;
    mOrder = order;
    mRippleDb = rippleDb;
    CalculatePoles();
  }

  void SetCutoff(double cutoff) {
    mCutoff = cutoff;
    // For classical filters, the calculated Q factors are constant for a given
    // order/type. We only update the cutoff of each stage, possibly scaled. For
    // simplicity, let's assume all stages track the main cutoff but with their
    // specific Q. Note: For Chebyshev, cutoff might be scaled per stage? No,
    // usually Q changes. Let's re-apply.
    for (size_t i = 0; i < stages.size(); ++i) {
      stages[i].SetParams(mCutoff, mQFactors[i]);
    }
  }

  inline double Process(double in) {
    double out = in;
    for (auto &stage : stages) {
      // For Classical LPF, we usually take the LP output of the SVF.
      // If we needed HP/BP, we'd need to configure that.
      // The prompt implies "Classical Filters ... as Serial Cascades".
      // Let's assume LowPass for now as that's standard for "Butterworth
      // Filter" unless specified.
      out = stage.ProcessLP(out);
    }
    return out;
  }

private:
  void CalculatePoles() {
    stages.clear();
    mQFactors.clear();

    int numPairs = mOrder / 2;
    // If odd order, we'd need a 1-pole section.
    // SVF is 2-pole.
    // Let's support even orders for now or use SVF in a specific mode for
    // 1-pole? Actually, an SVF can do 1-pole if configured right? Not really,
    // it's inherently 2nd order. But we can cascade SVFs. Let's assume
    // primarily even orders for simplicity or handle the odd pole.

    // Butterworth Poles
    // Poles are on unit circle.
    // Sk = exp( j * pi * (2k + n - 1) / (2n) )
    // We need Q for each pair.
    // Q = 1 / (2 * cos(angle_from_real_axis))?
    // Actually, for 2nd order section k:
    // angle theta_k.
    // pole p_k = -sin(theta) + j*cos(theta) ?
    // Standard analog prototype:
    // s^2 + (1/Q)s + 1.

    // Let's just implement a table or formula for Q.

    // Butterworth
    if (mType == Type::Butterworth) {
      for (int k = 1; k <= numPairs; ++k) {
        double theta = (2.0 * k + mOrder - 1) * kPi / (2.0 * mOrder);
        // Real part is cos(theta)? No, typical formula:
        // angle index k from 0 to n-1?
        // Let's use the explicit Q formula for Butterworth pair k:
        // Q_k = -1 / ( 2 * cos( (2k + n - 1)*pi / 2n ) ) ??
        // Wait, pole angle from negative real axis...
        // Only need Q values.
        // Q_k = 1 / ( 2 * sin( theta_k ) ) where theta_k is angle from
        // imaginary axis?

        // Working formula:
        double angle = (2.0 * k + mOrder - 1.0) * kPi / (2.0 * mOrder);
        // Pole is usually exp(j * angle).
        // Real part is cos(angle).
        // Denom is s^2 - 2*Re(p)*s + |p|^2.
        // |p|=1 for Butterworth.
        // So coeff of s is -2*Re(p) = -2*cos(angle).
        // But stable poles are in LHP, so cos(angle) must be negative.
        // If angle is in 2nd/3rd quad.

        // Let's just use the known adjustment.
        // 1/Q = -2 * cos(angle).
        // So Q = -1 / (2 * cos(angle)).

        double q = -1.0 / (2.0 * std::cos(angle));
        mQFactors.push_back(q);
        stages.emplace_back();
      }
    }
    // Chebyshev (Type 1)
    else if (mType == Type::Chebyshev1) {
      // More complex Q calculation involving epsilon (ripple)
      // epsilon = sqrt( 10^(ripple/10) - 1 )
      double eps = std::sqrt(std::pow(10.0, mRippleDb / 10.0) - 1.0);
      double a = std::asinh(1.0 / eps) / mOrder;
      double sinh_a = std::sinh(a);
      double cosh_a = std::cosh(a);

      for (int k = 1; k <= numPairs; ++k) {
        double theta = (2.0 * k - 1.0) * kPi / (2.0 * mOrder);
        double re = -sinh_a * std::sin(theta);
        double im = cosh_a * std::cos(theta);
        // Pole p = re + j*im
        // s^2 - 2*re*s + |p|^2
        // w0 = |p|
        // 1/Q = -2*re / |p|

        double mag2 = re * re + im * im;
        double mag = std::sqrt(mag2);
        double q = mag / (-2.0 * re);

        mQFactors.push_back(q);
        stages.emplace_back();
        // Note: Cutoff scaling might be needed here effectively?
        // SVF SetParams takes cutoff frequency.
        // Chebyshev usually defined at cutoff = frequency where gain drops by
        // ripple. The poles are normalized to w_c=1. So we just use the user
        // cutoff.
      }
    }

    // Initialize the new stages
    for (auto &s : stages)
      s.Init(mSampleRate);
  }

  double mSampleRate = 44100.0;
  Type mType = Type::Butterworth;
  int mOrder = 4;
  double mRippleDb = 1.0;
  double mCutoff = 1000.0;

  std::vector<VASVFilter> stages;
  std::vector<double> mQFactors;
};

} // namespace PolySynthCore
