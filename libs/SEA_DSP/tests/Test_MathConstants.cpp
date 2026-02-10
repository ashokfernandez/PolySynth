#include "catch.hpp"
#include <sea_dsp/sea_platform.h>
#include <sea_dsp/sea_biquad_filter.h>
// Test math constant precision and filter coefficient accuracy
#include <cmath>
#include <vector>

// Verify kPi constant casting works correctly
TEST_CASE("Math Constants Type Casting", "[Math][TypeSafety]") {
  // This test accesses sea::kPi and sea::kTwoPi constants from sea_platform.h
  // (included at top of file). Tests that casting produces correct values.
  float piF = static_cast<float>(sea::kPi);
  double piD = static_cast<double>(sea::kPi);

  REQUIRE(piF == Approx(3.14159265f).epsilon(0.0001));
  REQUIRE(piD == Approx(3.14159265358979323846).epsilon(0.0000001));

  float twoPiF = static_cast<float>(sea::kTwoPi);
  double twoPiD = static_cast<double>(sea::kTwoPi);

  REQUIRE(twoPiF == Approx(6.28318530f).epsilon(0.0001));
  REQUIRE(twoPiD == Approx(6.28318530717958647692).epsilon(0.0000001));
}

// Long-running precision test
TEST_CASE("Filter Coefficient Precision", "[Filter][Precision][TypeSafety]") {
  // Process 1000 samples with float and double filters
  // Compare outputs - should be very similar
  sea::BiquadFilter<float> filterF;
  sea::BiquadFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetParams(sea::FilterType::LowPass, 440.0f, 0.707f);
  filterD.SetParams(sea::FilterType::LowPass, 440.0, 0.707);

  std::vector<float> outF;
  std::vector<double> outD;

  for (int i = 0; i < 1000; ++i) {
    float input = (i % 2 == 0) ? 1.0f : -1.0f;
    outF.push_back(filterF.Process(input));
    outD.push_back(filterD.Process(static_cast<double>(input)));
  }

  // Compare outputs - should be very similar
  double maxDiff = 0.0;
  for (size_t i = 0; i < outF.size(); ++i) {
    double diff = std::abs(static_cast<double>(outF[i]) - outD[i]);
    if (diff > maxDiff) maxDiff = diff;
  }

  REQUIRE(maxDiff < 0.001); // Very small tolerance
}
