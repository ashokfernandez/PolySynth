#pragma once

#include "TPTIntegrator.h"
#include <algorithm>
#include <cmath>

namespace PolySynthCore {

class VASKFilter {
public:
  VASKFilter() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    lpf1.Init(sampleRate);
    lpf2.Init(sampleRate);
    Reset();
  }

  void Reset() {
    lpf1.Reset();
    lpf2.Reset();
  }

  // Set params
  // k is the resonance feedback gain.
  // For SKF, Q = 1 / (2 - k).
  // range of k: [0, 2). Self oscillation at 2.
  void SetParams(double cutoff, double k) {
    mCutoff = ClampCutoff(cutoff);
    mK = std::clamp(k, 0.0, 1.98);
    lpf1.Prepare(mCutoff);
    lpf2.Prepare(mCutoff);
  }

  inline double Process(double in) {
    // Topology:
    // u = in + k * (y1 - y2)
    // y1 = LPF1(u)
    // y2 = LPF2(y1)

    // Instantaneous responses of LPFs (TPT 1-pole LPF):
    // For TPTIntegrator: y = g*x + s.
    // 1-pole LPF composed of TPTIntegrator:
    // v = g(u - v) + s  (where v is output, u is input of LPF)
    // v(1+g) = gu + s
    // v = (g/(1+g))*u + s/(1+g)
    // Let G = g/(1+g), S_eff = s/(1+g)
    // v = G*u + S_eff

    double g = lpf1.GetG(); // Assumes same g for both
    double G = g / (1.0 + g);

    // State accumulation
    double s1 = lpf1.GetS();
    double S1 = s1 / (1.0 + g);

    double s2 = lpf2.GetS();
    double S2 = s2 / (1.0 + g);

    // Resolve loop for u:
    // u = in + k * (y1 - y2)
    // y1 = G*u + S1
    // y2 = G*y1 + S2 = G(G*u + S1) + S2 = G^2*u + G*S1 + S2
    // y1 - y2 = G*u + S1 - (G^2*u + G*S1 + S2)
    //         = (G - G^2)*u + (1 - G)*S1 - S2
    // u = in + k * [ (G - G^2)*u + (1 - G)*S1 - S2 ]
    // u * (1 - k*(G - G^2)) = in + k*((1 - G)*S1 - S2)

    double feedbackFactor = mK * (G - G * G);
    double denom = 1.0 - feedbackFactor;

    // Safety for high resonance (k near 2)
    if (std::abs(denom) < 1e-6)
      denom = 1e-6; // prevent div by zero

    double rhs = in + mK * ((1.0 - G) * S1 - S2);
    double u = rhs / denom;

    // Compute outputs
    // We need to process the LPFs properly to update states.
    // LPF1 input is u.
    // But we can't just call Process(u) on TPTIntegrator directly because
    // TPTIntegrator is just an integrator. We need a helper for "TPT 1-pole
    // LPF" or do the math here. TPT 1-pole LPF equation: v = v + g*(u - v). No,
    // that's Euler. TPT 1-pole LPF: v = (g*u + s) / (1 + g). Then s update: s =
    // 2*v - s. (Canonical) Let's implement this explicitly.

    // LPF1
    double y1 = (g * u + s1) / (1.0 + g);
    // Update LPF1 state
    // s1_new = 2*y1 - s1?
    // Let's use the TPTIntegrator update logic if we can, but
    // TPTIntegrator::Process does: y_int = g*in_int + s. For LPF: in_int = u -
    // v. v = y_int. v = g*(u - v) + s => v(1+g) = g*u + s. Correct. So we can
    // compute v (which is y1) directly shown above. Then we update s1. The
    // integrator input was (u - y1). TPTIntegrator update: s = g*in_int + y_int
    // = g*(u - y1) + y1. Which equals 2*y1 - s1? g*(u - y1) + y1 = g*u - g*y1 +
    // y1 = g*u + (1-g)y1 ??? Wait. s_new = 2*y1 - s1 is much simpler and
    // standard. Let's verify: s_new = 2*y1 - s1. 2*((g*u + s)/(1+g)) - s =
    // (2*g*u + 2*s - s(1+g))/(1+g) = (2*g*u + s - g*s)/(1+g). This seems
    // consistent with LPF TPT.

    lpf1.SetS(2.0 * y1 - s1);

    // LPF2
    // Input to LPF2 is y1.
    double y2 = (g * y1 + s2) / (1.0 + g);
    lpf2.SetS(2.0 * y2 - s2);

    return y2;
  }

private:
  double mSampleRate = 44100.0;
  double mCutoff = 1000.0;
  double mK = 0.0;

  TPTIntegrator lpf1;
  TPTIntegrator lpf2;

  double ClampCutoff(double cutoff) const {
    if (mSampleRate <= 0.0)
      return 0.0;
    double nyquist = 0.5 * mSampleRate;
    double maxCutoff = nyquist * 0.49;
    return std::clamp(cutoff, 0.0, maxCutoff);
  }
};

} // namespace PolySynthCore
