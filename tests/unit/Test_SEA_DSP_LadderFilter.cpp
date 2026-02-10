#include "dsp/va/VALadderFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_ladder_filter.h>
#include <vector>

TEST_CASE("SEA_DSP LadderFilter Bit-Exactness", "[sea_dsp][ladder]") {
  PolySynthCore::VALadderFilter original;
  sea::LadderFilter<double> ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // Case A: Transistor, Low Resonance
  original.SetParams(PolySynthCore::VALadderFilter::Model::Transistor, 1000.0,
                     0.0);
  ported.SetParams(sea::LadderFilter<double>::Model::Transistor, 1000.0, 0.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // Case B: Transistor, High Resonance
  original.SetParams(PolySynthCore::VALadderFilter::Model::Transistor, 2000.0,
                     1.0);
  ported.SetParams(sea::LadderFilter<double>::Model::Transistor, 2000.0, 1.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // Case C: Diode (even if simplified, logic must match)
  original.SetParams(PolySynthCore::VALadderFilter::Model::Diode, 500.0, 0.5);
  ported.SetParams(sea::LadderFilter<double>::Model::Diode, 500.0, 0.5);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }
}
