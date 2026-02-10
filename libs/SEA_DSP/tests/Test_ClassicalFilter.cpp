#include "catch.hpp"
// IMPORTANT: Include sea_classical_filter.h FIRST to verify it's self-contained
#include <sea_dsp/sea_classical_filter.h>

TEST_CASE("ClassicalFilter Self-Contained Header", "[Filter][Classical][Regression]") {
  // This test verifies that sea_classical_filter.h can be compiled in isolation
  // as the first include (no other SEA_DSP headers before it).
  //
  // NOTE: sea_classical_filter.h currently gets Math:: functions transitively
  // through sea_svf.h -> sea_tpt_integrator.h -> sea_math.h, so the explicit
  // #include "sea_math.h" in sea_classical_filter.h is technically redundant.
  // However, it's good practice to include headers for symbols you use directly.
  //
  // This test primarily ensures the header is self-contained and includes
  // its direct dependencies (sea_svf.h, algorithm, cmath, vector).

  sea::ClassicalFilter<double> filter;
  filter.Init(48000.0);
  filter.SetConfig(sea::ClassicalFilter<double>::Type::Butterworth, 4, 1.0);
  filter.SetCutoff(1000.0);

  filter.Reset();
  double out = 0.0;
  for (int i = 0; i < 100; ++i)
    out = filter.Process(1.0);

  // Verify it actually processes audio
  REQUIRE(out > 0.0);
  REQUIRE(out < 2.0);
}

TEST_CASE("ClassicalFilter Butterworth Response", "[Filter][Classical]") {
  sea::ClassicalFilter<double> filter;
  filter.Init(48000.0);
  filter.SetConfig(sea::ClassicalFilter<double>::Type::Butterworth, 4, 1.0);
  filter.SetCutoff(1000.0);

  // Test DC response (should pass through)
  filter.Reset();
  double dcOut = 0.0;
  for (int i = 0; i < 1000; ++i) {
    dcOut = filter.Process(1.0);
  }
  REQUIRE(dcOut == Approx(1.0).margin(0.1));

  // Test stability with impulse
  filter.Reset();
  double impulseOut = filter.Process(1.0);
  for (int i = 0; i < 1000; ++i) {
    impulseOut = filter.Process(0.0);
  }
  REQUIRE(std::abs(impulseOut) < 0.01);
}

TEST_CASE("ClassicalFilter Chebyshev1 Response", "[Filter][Classical]") {
  sea::ClassicalFilter<double> filter;
  filter.Init(48000.0);
  filter.SetConfig(sea::ClassicalFilter<double>::Type::Chebyshev1, 4, 0.5);
  filter.SetCutoff(1000.0);

  // Test DC response
  filter.Reset();
  double dcOut = 0.0;
  for (int i = 0; i < 1000; ++i) {
    dcOut = filter.Process(1.0);
  }
  // Chebyshev has ripple, so allow wider tolerance
  REQUIRE(dcOut > 0.5);
  REQUIRE(dcOut < 1.5);
}
