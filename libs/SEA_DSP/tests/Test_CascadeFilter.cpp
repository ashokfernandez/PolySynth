#include "catch.hpp"
#include <sea_dsp/sea_cascade_filter.h>
// NOTE: Only sea_cascade_filter.h is included (after catch.hpp). This verifies
// the header is self-contained and brings all its own dependencies.
// If sea_cascade_filter.h is missing any includes, this file will fail to compile.
#include <cmath>

TEST_CASE("CascadeFilter Self-Contained Header", "[Filter][Cascade][SelfContained]") {
  sea::CascadeFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(1000.0, 0.5, sea::CascadeFilter<double>::Slope::dB12);

  double out = filter.Process(1.0);
  REQUIRE(std::abs(out) < 2.0); // Sanity check
}

TEST_CASE("CascadeFilter Basic Operation", "[Filter][Cascade]") {
  sea::CascadeFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(1000.0, 0.5, sea::CascadeFilter<double>::Slope::dB12);

  // Test DC response - cascaded filter with resonance will have gain < 1.0
  filter.Reset();
  double dcOut = 0.0;
  for (int i = 0; i < 1000; ++i) {
    dcOut = filter.Process(1.0);
  }
  // Verify reasonable output range (not exact 1.0 due to resonance feedback)
  REQUIRE(dcOut > 0.1);
  REQUIRE(dcOut < 2.0);

  // Test stability with impulse
  filter.Reset();
  double impulse = filter.Process(1.0);
  for (int i = 0; i < 1000; ++i) {
    impulse = filter.Process(0.0);
  }
  REQUIRE(std::abs(impulse) < 0.01);
}

TEST_CASE("CascadeFilter Slope Selection", "[Filter][Cascade]") {
  sea::CascadeFilter<double> filter;
  filter.Init(48000.0);

  SECTION("12dB Slope") {
    filter.SetParams(1000.0, 0.5, sea::CascadeFilter<double>::Slope::dB12);
    filter.Reset();

    // Process signal
    double out = 0.0;
    for (int i = 0; i < 100; ++i) {
      out = filter.Process(1.0);
    }
    REQUIRE(std::abs(out) < 2.0);
  }

  SECTION("24dB Slope") {
    filter.SetParams(1000.0, 0.5, sea::CascadeFilter<double>::Slope::dB24);
    filter.Reset();

    // Process signal - 24dB has two stages and more resonance gain
    double out = 0.0;
    for (int i = 0; i < 100; ++i) {
      out = filter.Process(1.0);
    }
    // 24dB cascaded filter can have higher gain due to resonance
    REQUIRE(std::abs(out) < 10.0);
  }
}

TEST_CASE("CascadeFilter Type Safety", "[Filter][Cascade][TypeSafety]") {
  sea::CascadeFilter<float> filterF;
  sea::CascadeFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetParams(1000.0f, 0.5f, sea::CascadeFilter<float>::Slope::dB12);
  filterD.SetParams(1000.0, 0.5, sea::CascadeFilter<double>::Slope::dB12);

  // Process same input
  float outF = filterF.Process(1.0f);
  double outD = filterD.Process(1.0);

  // Results should be close
  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.01);
}
