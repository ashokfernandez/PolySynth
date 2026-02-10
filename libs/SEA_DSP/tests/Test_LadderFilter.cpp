#include "catch.hpp"
#include <sea_dsp/sea_ladder_filter.h>
// NOTE: Only sea_ladder_filter.h is included (after catch.hpp). This verifies
// the header is self-contained and brings all its own dependencies.
// If sea_ladder_filter.h is missing any includes, this file will fail to compile.
#include <cmath>

TEST_CASE("LadderFilter Self-Contained Header", "[Filter][LadderFilter][SelfContained]") {
  sea::LadderFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(sea::LadderFilter<double>::Model::Transistor, 1000.0, 0.5);

  double out = filter.Process(1.0);
  REQUIRE(std::abs(out) < 2.0); // Sanity check
}

TEST_CASE("LadderFilter Type Safety - Float vs Double", "[Filter][LadderFilter][TypeSafety]") {
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
