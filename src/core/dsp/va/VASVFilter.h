#pragma once

#include "TPTIntegrator.h"
#include <algorithm>
#include <cmath>

namespace PolySynthCore {

class VASVFilter {
public:
  VASVFilter() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    integrator1.Init(sampleRate);
    integrator2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    integrator1.Reset();
    integrator2.Reset();
  }

  void SetParams(double cutoff, double Q) {
    mCutoff = ClampCutoff(cutoff);
    mQ = std::max(0.05, Q);
    CalculateCoefficients();
  }

  // Process a sample and return LP, BP, HP
  struct Outputs {
    double lp;
    double bp;
    double hp;
  };

  inline Outputs Process(double in) {
    // Resolve ZDF loop
    // Terms from Zavalishin's book for TPT SVF:
    // hp = (in - (2*R + g)*s1 - s2) / (1 + 2*R*g + g*g)
    // But let's check the integrators.
    // Integrator 1: y1 = g*hp + s1
    // Integrator 2: y2 = g*y1 + s2
    // Feedback loop: u - 2R*y1 - y2 = hp? No, topology is:
    // HP goes into Int1 -> BP.
    // BP goes into Int2 -> LP.
    // Feedback: in - 2R*BP - LP = HP.

    // Substituting:
    // HP = in - 2R(g*HP + s1) - (g*BP + s2)
    //    = in - 2R*g*HP - 2R*s1 - (g*(g*HP + s1) + s2)
    //    = in - 2R*g*HP - 2R*s1 - g^2*HP - g*s1 - s2
    // HP(1 + 2Rg + g^2) = in - 2R*s1 - g*s1 - s2
    // HP = (in - (2R + g)*s1 - s2) / (1 + 2Rg + g^2)

    double g = integrator1.GetG(); // Assumes both have same g
    double R = 1.0 / (2.0 * mQ);
    double s1 = integrator1.GetS();
    double s2 = integrator2.GetS();

    double denominator = 1.0 + 2.0 * R * g + g * g;
    double hp = (in - (2.0 * R + g) * s1 - s2) / denominator;

    double bp = integrator1.Process(hp);
    double lp = integrator2.Process(bp);

    return {lp, bp, hp};
  }

  // Convenience for single output
  inline double ProcessLP(double in) { return Process(in).lp; }
  inline double ProcessBP(double in) { return Process(in).bp; }
  inline double ProcessHP(double in) { return Process(in).hp; }

private:
  void CalculateCoefficients() {
    // Prewarp cutoff
    // g = tan(pi * fc / fs)
    // We set this on the integrators
    // The integrators calculate g internally if we pass cutoff to them?
    // TPTIntegrator has Prepare(cutoff).
    integrator1.Prepare(mCutoff);
    integrator2.Prepare(mCutoff);
  }

  double mSampleRate = 44100.0;
  double mCutoff = 1000.0;
  double mQ = 0.707;

  TPTIntegrator integrator1;
  TPTIntegrator integrator2;

  double ClampCutoff(double cutoff) const {
    if (mSampleRate <= 0.0)
      return 0.0;
    double nyquist = 0.5 * mSampleRate;
    double maxCutoff = nyquist * 0.49;
    return std::clamp(cutoff, 0.0, maxCutoff);
  }
};

} // namespace PolySynthCore
