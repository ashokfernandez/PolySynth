#include <catch.hpp>
#include <sea_dsp/sea_wavetable.h>
#include <cmath>

TEST_CASE("SinWavetable accuracy", "[sea_dsp][wavetable]") {
  constexpr double two_pi = 6.28318530717958647692;
  constexpr int N = 10000;
  double maxErr = 0.0;

  for (int i = 0; i < N; ++i) {
    float phase =
        static_cast<float>(two_pi * static_cast<double>(i) / static_cast<double>(N));
    float wt = sea::SinWavetable<float, 512>::Lookup(phase);
    float ref = std::sin(phase);
    double err = std::abs(static_cast<double>(wt) - static_cast<double>(ref));
    if (err > maxErr)
      maxErr = err;
  }

  // Max error should be < 0.02% of full scale (amplitude 1.0)
  REQUIRE(maxErr < 0.0002);
}

TEST_CASE("SinWavetable boundary values", "[sea_dsp][wavetable]") {
  constexpr float pi = 3.14159265358979323846f;

  // sin(0) ≈ 0
  REQUIRE(sea::SinWavetable<float>::Lookup(0.0f) == Approx(0.0f).margin(0.001f));

  // sin(π/2) ≈ 1
  REQUIRE(sea::SinWavetable<float>::Lookup(pi / 2.0f) ==
          Approx(1.0f).margin(0.001f));

  // sin(π) ≈ 0
  REQUIRE(sea::SinWavetable<float>::Lookup(pi) == Approx(0.0f).margin(0.001f));

  // sin(3π/2) ≈ -1
  REQUIRE(sea::SinWavetable<float>::Lookup(3.0f * pi / 2.0f) ==
          Approx(-1.0f).margin(0.001f));

  // sin(2π - ε) ≈ 0
  REQUIRE(sea::SinWavetable<float>::Lookup(2.0f * pi - 0.001f) ==
          Approx(0.0f).margin(0.01f));
}

TEST_CASE("SinWavetable cos via offset", "[sea_dsp][wavetable]") {
  constexpr float pi = 3.14159265358979323846f;
  constexpr float two_pi = 2.0f * pi;
  constexpr int N = 1000;

  for (int i = 0; i < N; ++i) {
    float phase = two_pi * static_cast<float>(i) / static_cast<float>(N);
    // cos(x) = sin(x + π/2)
    float sin_offset = phase + pi / 2.0f;
    if (sin_offset >= two_pi)
      sin_offset -= two_pi;
    float wt_cos = sea::SinWavetable<float>::Lookup(sin_offset);
    float ref_cos = std::cos(phase);
    REQUIRE(wt_cos == Approx(ref_cos).margin(0.001f));
  }
}

TEST_CASE("SinWavetable wrap-around interpolation", "[sea_dsp][wavetable]") {
  // Test interpolation across the table boundary (index 511 → 0)
  constexpr float two_pi = 6.28318530717958647692f;

  // Phase just before 2π should interpolate between last entry and first
  float phase = two_pi - 0.001f;
  float wt = sea::SinWavetable<float>::Lookup(phase);
  float ref = std::sin(phase);
  REQUIRE(wt == Approx(ref).margin(0.01f));
}
