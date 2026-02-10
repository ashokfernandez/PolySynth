#include "dsp/va/TPTIntegrator.h" // Original implementation
#include <catch.hpp>
#include <iostream>
#include <sea_dsp/sea_tpt_integrator.h>
#include <vector>

TEST_CASE("SEA_DSP TPTIntegrator Bit-Exactness", "[sea_dsp][tpt]") {
  // 1. Instantiate both
  PolySynthCore::TPTIntegrator original;
  sea::TPTIntegrator<double> ported;

  // 2. Setup Parameters
  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  // 3. Generate Test Input (e.g., a simple ramp or random values)
  // Using deterministic values to ensure reproducibility
  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // 4. Process and Compare
  // Case A: High Cutoff
  double cutoff = 1000.0;
  original.Prepare(cutoff);
  ported.Prepare(cutoff);

  // Check internal state (g) if possible, but getters exist
  // original: g is private, has GetG()
  REQUIRE(original.GetG() == ported.GetG());

  for (double in : inputs) {
    double out_orig = original.Process(in);
    double out_port = ported.Process(in);

    // Exact match required
    REQUIRE(out_orig == out_port);
    // Also verify state is synced
    REQUIRE(original.GetS() == ported.GetS());
  }

  // Case B: Low Cutoff
  cutoff = 100.0;
  original.Prepare(cutoff);
  ported.Prepare(cutoff);

  for (double in : inputs) {
    double out_orig = original.Process(in);
    double out_port = ported.Process(in);
    REQUIRE(out_orig == out_port);
  }

  // Case C: Varying Cutoff per sample (common in modulation)
  for (int i = 0; i < 100; ++i) {
    double modCutoff = 500.0 + 400.0 * std::sin(i * 0.05);
    double in = inputs[i];

    original.Prepare(modCutoff);
    ported.Prepare(modCutoff);

    double out_orig = original.Process(in);
    double out_port = ported.Process(in);

    REQUIRE(out_orig == out_port);
  }
}
