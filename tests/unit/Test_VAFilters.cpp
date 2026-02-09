#include "../../src/core/dsp/va/TPTIntegrator.h"
#include "../../src/core/dsp/va/VALadderFilter.h"
#include "../../src/core/dsp/va/VASKFilter.h"
#include "../../src/core/dsp/va/VASVFilter.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;

TEST_CASE("TPT Integrator Basic", "[VAFilter][TPT]") {
  TPTIntegrator integrator;
  integrator.Init(44100.0);

  // Test Gain Calculation
  integrator.Prepare(1000.0);
  double g = integrator.GetG();
  // tan(pi * 1000 / 44100) approx tan(0.071) approx 0.071
  // g = wa * T / 2 = tan(wd*T/2)
  REQUIRE(g > 0.0);
  REQUIRE(g < 1.0);

  // DC Test
  integrator.Reset();
  // Step response
  double out = 0.0;
  for (int i = 0; i < 100; ++i)
    out = integrator.Process(1.0);

  // Integrator of constant 1 should ramp up indefinitely?
  // No, standard TPT integrator is leaky? Or pure?
  // "Instantaneous Response: y = g*x + s"
  // s[n+1] = 2*y[n] - s[n]
  // If x=1, s=0 -> y = g. s_new = 2g.
  // Next: y = g + 2g = 3g. s_new = 6g - 2g = 4g.
  // Next: y = g + 4g = 5g.
  // It ramps linearly with slope 2g per sample?
  // This confirms it acts as an integrator.
  REQUIRE(out > 1.0);
}

TEST_CASE("SVF Response", "[VAFilter][SVF]") {
  VASVFilter svf;
  svf.Init(44100.0);
  svf.SetParams(1000.0, 0.707); // Butterworth Q

  // DC gain of LP should be 1.0
  double out = 0.0;
  for (int i = 0; i < 500; ++i)
    out = svf.ProcessLP(1.0);
  REQUIRE(out == Approx(1.0).margin(0.01));

  // DC gain of HP should be 0.0
  out = svf.ProcessHP(1.0); // with steady state input
  REQUIRE(std::abs(out) < 0.01);

  // Resonance check (Q=10)
  svf.SetParams(1000.0, 10.0);
  // At cutoff, gain should be Q (linear)?
  // Hard to test exactly without sine sweep, but let's check it doesn't explode
  for (int i = 0; i < 1000; ++i)
    svf.ProcessLP((i % 2 == 0) ? 1.0 : -1.0); // Noise/Signal
                                              // Just ensure stability

  // Extreme params should not generate NaNs
  svf.SetParams(50000.0, 0.0);
  auto outputs = svf.Process(0.25);
  REQUIRE(std::isfinite(outputs.lp));
  REQUIRE(std::isfinite(outputs.bp));
  REQUIRE(std::isfinite(outputs.hp));
}

TEST_CASE("Ladder Filter Stability", "[VAFilter][Ladder]") {
  VALadderFilter ladder;
  ladder.Init(44100.0);
  ladder.SetParams(VALadderFilter::Model::Transistor, 1000.0, 0.0); // No res

  // DC Gain
  double out = 0.0;
  for (int i = 0; i < 500; ++i)
    out = ladder.Process(1.0);
  REQUIRE(out == Approx(1.0).margin(0.01));

  // Self Oscillation Threshold Check
  // With max resonance (k=4?), filter should oscillate.
  // Our param range is 0..1 for resonance usually in synths, mapped to k=0..4
  // internally. The implementation uses mResonance * 4.0.
  ladder.SetParams(VALadderFilter::Model::Transistor, 1000.0, 1.0); // Max res

  // Use Sine wave at cutoff to test resonance gain
  // At k=4.0 (max res), gain should be theoretically infinite (limited by
  // precision/range) We expect it to grow large.
  ladder.Reset();
  double maxAmp = 0.0;
  double twoPiF = 2.0 * 3.1415926535 * 1000.0 / 44100.0;
  for (int i = 0; i < 1000; ++i) {
    double in = std::sin(i * twoPiF) * 0.1; // 0.1 amplitude input
    double v = ladder.Process(in);
    if (std::abs(v) > maxAmp)
      maxAmp = std::abs(v);
  }
  // With significant resonance, gain should be > 1.0 (input was 0.1, so output
  // > 0.1) Actually typically gain > 10dB easily.
  REQUIRE(maxAmp > 0.5);
}

TEST_CASE("Sallen-Key Filter Check", "[VAFilter][SKF]") {
  VASKFilter skf;
  skf.Init(44100.0);
  skf.SetParams(1000.0, 0.0); // k=0 -> Q=0.5 (overdamped)

  // DC Gain
  double out = 0.0;
  for (int i = 0; i < 500; ++i)
    out = skf.Process(1.0);
  // SKF LP gain is 1?
  // Transfer function H(0) = 1.
  REQUIRE(out == Approx(1.0).margin(0.01));

  skf.SetParams(1000.0, 1.0); // k=1 -> Q=1.0
  // Check stability
  for (int i = 0; i < 500; ++i)
    skf.Process(0.0); // Decay check

  skf.SetParams(20000.0, 3.0);
  out = skf.Process(0.1);
  REQUIRE(std::isfinite(out));
}
