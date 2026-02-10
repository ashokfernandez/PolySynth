#include "catch.hpp"
#include <sea_dsp/sea_sk_filter.h>
// NOTE: Only sea_sk_filter.h is included (after catch.hpp). This verifies
// the header is self-contained and brings all its own dependencies.
// If sea_sk_filter.h is missing any includes, this file will fail to compile.
#include <cmath>

TEST_CASE("SKFilter Self-Contained Header", "[Filter][SK][SelfContained]") {
  sea::SKFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(1000.0, 0.7);

  double out = filter.Process(1.0);
  REQUIRE(std::abs(out) < 2.0); // Sanity check
}

TEST_CASE("SKFilter Basic Operation", "[Filter][SK]") {
  sea::SKFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(1000.0, 0.7);

  // Test DC response
  filter.Reset();
  double dcOut = 0.0;
  for (int i = 0; i < 1000; ++i) {
    dcOut = filter.Process(1.0);
  }
  REQUIRE(dcOut == Approx(1.0).margin(0.1));

  // Test stability with impulse
  filter.Reset();
  double impulse = filter.Process(1.0);
  for (int i = 0; i < 1000; ++i) {
    impulse = filter.Process(0.0);
  }
  REQUIRE(std::abs(impulse) < 0.01);
}

TEST_CASE("SKFilter Parameter Range", "[Filter][SK]") {
  sea::SKFilter<double> filter;
  filter.Init(48000.0);

  SECTION("Low resonance") {
    filter.SetParams(1000.0, 0.0);
    filter.Reset();

    double out = 0.0;
    for (int i = 0; i < 100; ++i) {
      out = filter.Process(1.0);
    }
    REQUIRE(std::abs(out) < 2.0);
  }

  SECTION("High resonance") {
    filter.SetParams(1000.0, 1.5);
    filter.Reset();

    double out = 0.0;
    for (int i = 0; i < 100; ++i) {
      out = filter.Process(1.0);
    }
    REQUIRE(std::abs(out) < 5.0);
  }
}

TEST_CASE("SKFilter Type Safety", "[Filter][SK][TypeSafety]") {
  sea::SKFilter<float> filterF;
  sea::SKFilter<double> filterD;

  filterF.Init(48000.0f);
  filterD.Init(48000.0);

  filterF.SetParams(1000.0f, 0.7f);
  filterD.SetParams(1000.0, 0.7);

  // Process same input
  float outF = filterF.Process(1.0f);
  double outD = filterD.Process(1.0);

  // Results should be close
  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.01);
}
