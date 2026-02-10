#include "dsp/va/VAProphetFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_prophet_filter.h>
#include <vector>

TEST_CASE("SEA_DSP ProphetFilter Bit-Exactness", "[sea_dsp][prophet]") {
  PolySynthCore::VAProphetFilter original;
  sea::ProphetFilter<double> ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // Case A: 24dB Slope, Low Resonance
  original.SetParams(1000.0, 0.0, PolySynthCore::VAProphetFilter::Slope::dB24);
  ported.SetParams(1000.0, 0.0, sea::ProphetFilter<double>::Slope::dB24);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // Case B: 12dB Slope, High Resonance
  original.SetParams(2000.0, 0.8, PolySynthCore::VAProphetFilter::Slope::dB12);
  ported.SetParams(2000.0, 0.8, sea::ProphetFilter<double>::Slope::dB12);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }
}
