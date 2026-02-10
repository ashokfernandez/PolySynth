#include "../../src/core/types.h"
#include "catch.hpp"
#include <sea_dsp/sea_biquad_filter.h>

TEST_CASE("Biquad LowPass Response", "[Biquad]") {
  sea::BiquadFilter<PolySynthCore::sample_t> filter;
  double sr = 48000.0;
  filter.Init(sr);

  // LowPass at 100Hz
  filter.SetParams(sea::FilterType::LowPass, 100.0, 0.707);

  // 1. DC Check (0 Hz) -> Gain should be 1.0 (0dB)
  // Run constant 1.0 signal
  filter.Reset();
  double dcOut = 0.0;
  for (int i = 0; i < 1000; i++) {
    dcOut = filter.Process(1.0);
  }
  REQUIRE(dcOut == Approx(1.0).margin(0.01));

  // 2. High Frequency Check (Nyquist) -> Gain should be close to 0.0
  // Input: Alternating 1.0, -1.0 (Nyquist frequency)
  filter.Reset();
  double nyquistOut = 0.0;
  for (int i = 0; i < 1000; i++) {
    double in = (i % 2 == 0) ? 1.0 : -1.0;
    nyquistOut = filter.Process(in);
  }
  // Low pass at 100Hz should basically kill 24kHz
  REQUIRE(std::abs(nyquistOut) < 0.001);
}

TEST_CASE("Biquad Stability", "[Biquad]") {
  sea::BiquadFilter<PolySynthCore::sample_t> filter;
  filter.Init(44100.0);
  filter.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);
  // High Q

  // Pulse input
  double out = filter.Process(1.0); // Impulse
  double maxVal = 0.0;

  for (int i = 0; i < 10000; i++) {
    out = filter.Process(0.0);
    maxVal = std::max(maxVal, std::abs(out));

    // Should decay eventually? High Q might ring but shouldn't explode
    if (std::abs(out) > 100.0) {
      FAIL("Filter exploded");
    }
  }

  // It should settle
  REQUIRE(std::abs(out) < 0.01);
}

TEST_CASE("Biquad BandPass and Notch Basics", "[Biquad]") {
  sea::BiquadFilter<PolySynthCore::sample_t> filter;
  filter.Init(48000.0);

  filter.SetParams(sea::FilterType::BandPass, 1000.0, 0.707);
  filter.Reset();
  double out = 0.0;
  for (int i = 0; i < 1000; ++i)
    out = filter.Process(1.0);
  // BandPass should reject DC
  REQUIRE(std::abs(out) < 0.01);

  filter.SetParams(sea::FilterType::Notch, 1000.0, 0.707);
  filter.Reset();
  for (int i = 0; i < 1000; ++i)
    out = filter.Process(1.0);
  // Notch should pass DC
  REQUIRE(out == Approx(1.0).margin(0.01));
}
