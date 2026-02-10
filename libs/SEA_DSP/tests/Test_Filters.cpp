#include "catch.hpp"
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_ladder_filter.h>
#include <sea_dsp/sea_cascade_filter.h>
#include <sea_dsp/sea_sk_filter.h>
#include <sea_dsp/sea_svf.h>
#include <sea_dsp/sea_tpt_integrator.h>

TEST_CASE("TPT Integrator Basic", "[VAFilter][TPT]") {
  sea::TPTIntegrator<double> integrator;
  integrator.Init(44100.0);

  // Test Gain Calculation
  integrator.Prepare(1000.0);
  double g = integrator.GetG();
  REQUIRE(g > 0.0);
  REQUIRE(g < 1.0);

  // DC Test
  integrator.Reset();
  double out = 0.0;
  for (int i = 0; i < 100; ++i)
    out = integrator.Process(1.0);

  REQUIRE(out > 1.0);
}

TEST_CASE("SVF Response", "[VAFilter][SVF]") {
  sea::SVFilter<double> svf;
  svf.Init(44100.0);
  svf.SetParams(1000.0, 0.707); // Butterworth Q

  // Basic stability check
  svf.Reset();
  double lp = 0.0;
  for (int i = 0; i < 100; ++i) {
    lp = svf.ProcessLP(0.1);
  }
  REQUIRE(std::abs(lp) < 1.0);
}

TEST_CASE("Biquad Filter Detailed Checks", "[Filter][Biquad]") {
  sea::BiquadFilter<double> filter;
  double sr = 48000.0;
  filter.Init(sr);

  SECTION("LowPass DC and Nyquist") {
    filter.SetParams(sea::FilterType::LowPass, 100.0, 0.707);
    filter.Reset();
    double dcOut = 0.0;
    for (int i = 0; i < 1000; i++)
      dcOut = filter.Process(1.0);
    REQUIRE(dcOut == Approx(1.0).margin(0.01));

    filter.Reset();
    double nyquistOut = 0.0;
    for (int i = 0; i < 1000; i++) {
      double in = (i % 2 == 0) ? 1.0 : -1.0;
      nyquistOut = filter.Process(in);
    }
    REQUIRE(std::abs(nyquistOut) < 0.001);
  }

  SECTION("Stability") {
    filter.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);
    double out = filter.Process(1.0); // Impulse
    for (int i = 0; i < 1000; i++)
      out = filter.Process(0.0);
    REQUIRE(std::abs(out) < 0.01);
  }

  SECTION("BandPass and Notch") {
    filter.SetParams(sea::FilterType::BandPass, 1000.0, 0.707);
    filter.Reset();
    double out = 0.0;
    for (int i = 0; i < 1000; ++i)
      out = filter.Process(1.0);
    REQUIRE(std::abs(out) < 0.01);

    filter.SetParams(sea::FilterType::Notch, 1000.0, 0.707);
    filter.Reset();
    for (int i = 0; i < 1000; ++i)
      out = filter.Process(1.0);
    REQUIRE(out == Approx(1.0).margin(0.01));
  }
}
