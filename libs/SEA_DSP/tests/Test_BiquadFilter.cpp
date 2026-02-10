#include "catch.hpp"
#include <sea_dsp/sea_biquad_filter.h>
// NOTE: Only sea_biquad_filter.h is included (after catch.hpp). This verifies
// the header is self-contained and brings all its own dependencies.
// If sea_biquad_filter.h is missing any includes, this file will fail to compile.
#include <cmath>

TEST_CASE("BiquadFilter Self-Contained Header", "[Filter][BiquadFilter][SelfContained]") {
  sea::BiquadFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);

  double out = filter.Process(1.0);
  REQUIRE(std::abs(out) < 2.0); // Sanity check
}

TEST_CASE("BiquadFilter Type Safety - Float vs Double", "[Filter][BiquadFilter][TypeSafety]") {
  sea::BiquadFilter<float> filterF;
  sea::BiquadFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetParams(sea::FilterType::LowPass, 1000.0f, 0.707f);
  filterD.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);

  // Process same input
  float outF = filterF.Process(1.0f);
  double outD = filterD.Process(1.0);

  // Results should be very close (accounting for precision differences)
  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.0001);
}
