#include "dsp/BiquadFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_biquad_filter.h>
#include <vector>

TEST_CASE("SEA_DSP BiquadFilter Bit-Exactness", "[sea_dsp][biquad]") {
  PolySynthCore::BiquadFilter original;
  sea::BiquadFilter<double> ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1));
  }

  // LowPass
  original.SetParams(PolySynthCore::FilterType::LowPass, 1000.0, 0.707);
  ported.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // HighPass
  original.SetParams(PolySynthCore::FilterType::HighPass, 5000.0, 2.0);
  ported.SetParams(sea::FilterType::HighPass, 5000.0, 2.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // BandPass
  original.SetParams(PolySynthCore::FilterType::BandPass, 500.0, 5.0);
  ported.SetParams(sea::FilterType::BandPass, 500.0, 5.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }

  // Notch
  original.SetParams(PolySynthCore::FilterType::Notch, 60.0, 10.0);
  ported.SetParams(sea::FilterType::Notch, 60.0, 10.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == ported.Process(in));
  }
}
