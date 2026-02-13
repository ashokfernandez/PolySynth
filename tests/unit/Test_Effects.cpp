#include "catch.hpp"
#include <algorithm>
#include <cmath>
#include <sea_dsp/effects/sea_vintage_chorus.h>
#include <sea_dsp/effects/sea_vintage_delay.h>
#include <sea_dsp/sea_limiter.h>

TEST_CASE("VintageDelay produces an audible tap at delay time", "[FX]") {
  sea::VintageDelay<double> delay;
  const double sampleRate = 48000.0;
  delay.Init(sampleRate, 1000.0);
  delay.SetTime(10.0); // 10ms
  delay.SetFeedback(0.0);
  delay.SetMix(100.0); // 100% wet

  // 10ms @ 48kHz = 480 samples.
  // We'll search in a window of 470 to 490 samples to account for
  // drift/latency.
  const int expectedDelay = 480;
  double maxPeak = 0.0;

  for (int i = 0; i <= expectedDelay + 100; ++i) {
    double left = (i == 0) ? 1.0 : 0.0;
    double right = left;
    double outL, outR;
    delay.Process(left, right, &outL, &outR);

    if (i >= 470 && i <= 495) {
      maxPeak = std::max(maxPeak, std::abs(outL));
    }
  }

  // Should have a significant peak.
  // Note: VintageDelay has a 1-pole filter in the feedback path,
  // but it's 100% wet and only one pass, so it should be near 1.0
  // if no feedback.
  REQUIRE(maxPeak > 0.5);
}

TEST_CASE("VintageChorus preserves dry signal when mix is zero", "[FX]") {
  sea::VintageChorus<double> chorus;
  chorus.Init(48000.0);
  chorus.SetRate(0.5);
  chorus.SetDepth(0.8);
  chorus.SetMix(0.0);

  for (int i = 0; i < 32; ++i) {
    double left = 0.2;
    double right = -0.2;
    double outL, outR;
    chorus.Process(left, right, &outL, &outR);
    REQUIRE(outL == Approx(0.2).margin(1e-6));
    REQUIRE(outR == Approx(-0.2).margin(1e-6));
  }
}

TEST_CASE("Sea Lookahead limiter caps peaks above threshold", "[FX]") {
  sea::LookaheadLimiter<double> limiter;
  const double sampleRate = 48000.0;
  const double threshold = 0.4;
  limiter.Init(sampleRate);
  limiter.SetParams(threshold, 5.0, 50.0);

  const int lookaheadSamples =
      std::max(1, static_cast<int>(sampleRate * 0.005));
  double maxOut = 0.0;

  for (int i = 0; i < 1000; ++i) {
    double left = 1.0;
    double right = 1.0;
    limiter.Process(left, right);
    if (i > lookaheadSamples + 10) {
      maxOut = std::max(maxOut, std::abs(left));
    }
  }

  REQUIRE(maxOut <= threshold + 0.05);
}
