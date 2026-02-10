#include "../../src/core/types.h"
#include "catch.hpp"
#include <sea_dsp/sea_oscillator.h>

TEST_CASE("Oscillator Frequency Check", "[Oscillator]") {
  sea::Oscillator osc;
  double sr = 48000.0;
  osc.Init(sr);

  // Test 480Hz -> 100 samples per cycle
  double freq = 480.0;
  osc.SetFrequency(freq);
  osc.Reset();

  // First sample should be -1.0 (phase 0)
  REQUIRE(osc.Process() == Approx(-1.0));

  // Advance 50 samples (half cycle) -> should be close to 0.0
  for (int i = 0; i < 49; i++) {
    osc.Process();
  }

  // At sample 50 (index 50), phase is 0.5. Output should be 0.0
  // But verify the trend.
  REQUIRE(osc.Process() == Approx(0.0).margin(0.01));

  // Advance another 50 samples -> should wrap around
  for (int i = 0; i < 49; i++) {
    osc.Process();
  }

  // At sample 100, phase wraps. Process returns (2*ph - 1).
  // It might be -1.0 again or very close to it depending on float precision
  PolySynthCore::sample_t val = osc.Process();
  REQUIRE(val == Approx(-1.0).margin(0.01));
}
