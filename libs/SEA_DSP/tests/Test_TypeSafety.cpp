#include "catch.hpp"
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_classical_filter.h>
#include <sea_dsp/sea_ladder_filter.h>
#include <sea_dsp/sea_tpt_integrator.h>
// NOTE: sea_math.h deliberately NOT included here - each filter must bring
// its own dependencies. If we include it, we can't detect missing includes.
// We do need sea_platform.h for the Math Constants test to access kPi/kTwoPi.
#include <sea_dsp/sea_platform.h>
#include <cmath>
#include <vector>

// Test that float and double versions produce consistent results
TEST_CASE("BiquadFilter Type Safety - Float vs Double", "[Filter][TypeSafety]") {
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

TEST_CASE("TPTIntegrator Type Safety - Float vs Double", "[Filter][TypeSafety]") {
  sea::TPTIntegrator<float> integF;
  sea::TPTIntegrator<double> integD;

  integF.Init(44100.0f);
  integD.Init(44100.0);

  integF.Prepare(1000.0f);
  integD.Prepare(1000.0);

  // Check gain calculation consistency
  float gF = integF.GetG();
  double gD = integD.GetG();

  REQUIRE(std::abs(static_cast<double>(gF) - gD) < 0.0001);

  // Process same input
  float outF = integF.Process(1.0f);
  double outD = integD.Process(1.0);

  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.0001);
}

TEST_CASE("LadderFilter Type Safety - Float vs Double", "[Filter][TypeSafety]") {
  sea::LadderFilter<float> filterF;
  sea::LadderFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetParams(sea::LadderFilter<float>::Model::Transistor, 1000.0f, 0.5f);
  filterD.SetParams(sea::LadderFilter<double>::Model::Transistor, 1000.0, 0.5);

  filterF.Reset();
  filterD.Reset();

  // Process multiple samples to check stability
  float lastF = 0.0f;
  double lastD = 0.0;
  for (int i = 0; i < 100; ++i) {
    lastF = filterF.Process(1.0f);
    lastD = filterD.Process(1.0);
  }

  REQUIRE(std::abs(static_cast<double>(lastF) - lastD) < 0.01);
}

TEST_CASE("ClassicalFilter Type Safety - Float vs Double", "[Filter][TypeSafety]") {
  sea::ClassicalFilter<float> filterF;
  sea::ClassicalFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetConfig(sea::ClassicalFilter<float>::Type::Butterworth, 4, 1.0f);
  filterD.SetConfig(sea::ClassicalFilter<double>::Type::Butterworth, 4, 1.0);

  filterF.SetCutoff(1000.0f);
  filterD.SetCutoff(1000.0);

  // Process same input
  float outF = filterF.Process(1.0f);
  double outD = filterD.Process(1.0);

  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.0001);
}

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
