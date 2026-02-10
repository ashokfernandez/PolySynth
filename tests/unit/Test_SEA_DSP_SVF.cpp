#include "dsp/va/VASVFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_svf.h>
#include <vector>

TEST_CASE("SEA_DSP SVFilter Bit-Exactness", "[sea_dsp][svf]") {
  // 1. Instantiate
  PolySynthCore::VASVFilter original;
  sea::SVFilter<double> ported;

  // 2. Setup
  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  // 3. Test Inputs
  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // 4. Compare
  // Case A: Standard Setup
  double cutoff = 1000.0;
  double Q = 0.707;
  original.SetParams(cutoff, Q);
  ported.SetParams(cutoff, Q);

  for (double in : inputs) {
    auto out_orig = original.Process(in);
    auto out_port = ported.Process(in);

    REQUIRE(out_orig.lp == out_port.lp);
    REQUIRE(out_orig.bp == out_port.bp);
    REQUIRE(out_orig.hp == out_port.hp);
  }

  // Case B: High Resonance
  Q = 5.0;
  cutoff = 2000.0;
  original.SetParams(cutoff, Q);
  ported.SetParams(cutoff, Q);

  for (double in : inputs) {
    auto out_orig = original.Process(in);
    auto out_port = ported.Process(in);

    REQUIRE(out_orig.lp == out_port.lp);
    REQUIRE(out_orig.bp == out_port.bp);
    REQUIRE(out_orig.hp == out_port.hp);
  }

  // Case C: Modulation
  for (int i = 0; i < 100; ++i) {
    double modCutoff = 1000.0 + 800.0 * std::sin(i * 0.05);
    double in = inputs[i];

    original.SetParams(modCutoff, Q);
    ported.SetParams(modCutoff, Q);

    auto out_orig = original.Process(in);
    auto out_port = ported.Process(in);

    REQUIRE(out_orig.lp == out_port.lp);
  }
}
