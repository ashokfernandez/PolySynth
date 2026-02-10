#include "dsp/va/VASKFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_sk_filter.h>
#include <vector>

TEST_CASE("SEA_DSP SKFilter Bit-Exactness", "[sea_dsp][skf]") {
  PolySynthCore::VASKFilter original;
  sea::SKFilter<double> ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // Case A: Low Resonance
  original.SetParams(1000.0, 0.0);
  ported.SetParams(1000.0, 0.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // Case B: High Resonance
  original.SetParams(2000.0, 1.8);
  ported.SetParams(2000.0, 1.8);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }
}
