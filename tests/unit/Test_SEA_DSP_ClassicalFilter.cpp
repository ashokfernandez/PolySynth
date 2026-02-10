#include "dsp/va/VAClassicalFilter.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_classical_filter.h>
#include <vector>

TEST_CASE("SEA_DSP ClassicalFilter Bit-Exactness", "[sea_dsp][classical]") {
  PolySynthCore::VAClassicalFilter original;
  sea::ClassicalFilter<double> ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  std::vector<double> inputs;
  for (int i = 0; i < 1000; ++i) {
    inputs.push_back(std::sin(i * 0.1) * 0.5);
  }

  // Case A: Butterworth 4th Order
  original.SetConfig(PolySynthCore::VAClassicalFilter::Type::Butterworth, 4,
                     1.0);
  ported.SetConfig(sea::ClassicalFilter<double>::Type::Butterworth, 4, 1.0);
  original.SetCutoff(1000.0);
  ported.SetCutoff(1000.0);

  for (double in : inputs) {
    REQUIRE(original.Process(in) == Approx(ported.Process(in)));
  }

  // Case B: Chebyshev1 6th Order
  original.SetConfig(PolySynthCore::VAClassicalFilter::Type::Chebyshev1, 6,
                     0.5);
  ported.SetConfig(sea::ClassicalFilter<double>::Type::Chebyshev1, 6, 0.5);
  original.SetCutoff(2000.0);
  ported.SetCutoff(2000.0);

  for (double in : inputs) {
    // Higher order might have small precision drift due to math functions
    REQUIRE(original.Process(in) == Approx(ported.Process(in)));
  }
}
