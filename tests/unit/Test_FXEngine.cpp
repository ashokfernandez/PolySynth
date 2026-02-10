#include "../../src/core/dsp/fx/FXEngine.h"
#include "catch.hpp"
#include <algorithm>

TEST_CASE("StereoDelay produces an audible tap at delay time", "[FX]") {
  PolySynthCore::StereoDelay delay;
  const double sampleRate = 48000.0;
  delay.Init(sampleRate);
  delay.SetParams(0.01, 0.0, 1.0);

  const int delaySamples = static_cast<int>(sampleRate * 0.01);
  double outAtDelay = 0.0;

  for (int i = 0; i <= delaySamples; ++i) {
    PolySynthCore::sample_t left = (i == 0) ? 1.0 : 0.0;
    PolySynthCore::sample_t right = left;
    delay.Process(left, right);
    if (i == delaySamples) {
      outAtDelay = left;
    }
  }

  REQUIRE(outAtDelay == Approx(1.0).margin(0.05));
}

TEST_CASE("Chorus preserves dry signal when mix is zero", "[FX]") {
  PolySynthCore::Chorus chorus;
  chorus.Init(48000.0);
  chorus.SetParams(0.5, 0.8, 0.0);

  for (int i = 0; i < 32; ++i) {
    PolySynthCore::sample_t left = 0.2;
    PolySynthCore::sample_t right = -0.2;
    chorus.Process(left, right);
    REQUIRE(left == Approx(0.2).margin(1e-6));
    REQUIRE(right == Approx(-0.2).margin(1e-6));
  }
}

TEST_CASE("Lookahead limiter caps peaks above threshold", "[FX]") {
  PolySynthCore::LookaheadLimiter limiter;
  const double sampleRate = 48000.0;
  const double threshold = 0.4;
  limiter.Init(sampleRate);
  limiter.SetParams(threshold, 5.0, 50.0);

  const int lookaheadSamples = std::max(1, static_cast<int>(sampleRate * 0.005));
  double maxOut = 0.0;

  for (int i = 0; i < 1000; ++i) {
    PolySynthCore::sample_t left = 1.0;
    PolySynthCore::sample_t right = 1.0;
    limiter.Process(left, right);
    if (i > lookaheadSamples) {
      maxOut = std::max(maxOut, static_cast<double>(std::abs(left)));
    }
  }

  REQUIRE(maxOut <= threshold + 0.02);
}
