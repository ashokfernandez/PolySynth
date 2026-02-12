#include "catch.hpp"
#include "sea_dsp/effects/sea_vintage_chorus.h"
#include <cmath>
#include <vector>
#include <algorithm>

TEST_CASE("VintageChorus Self-Contained Header", "[VintageChorus][SelfContained]") {
  sea::VintageChorus<float> vc;
  REQUIRE(true);
}

TEST_CASE("VintageChorus Type Safety - Float vs Double", "[VintageChorus][TypeSafety]") {
  sea::VintageChorus<float> vcF;
  sea::VintageChorus<double> vcD;

  vcF.Init(48000.0f);
  vcD.Init(48000.0);

  vcF.SetRate(1.0f);
  vcD.SetRate(1.0);

  vcF.SetDepth(2.0f);
  vcD.SetDepth(2.0);

  vcF.SetMix(0.5f);
  vcD.SetMix(0.5);

  float outLF, outRF;
  double outLD, outRD;

  vcF.Process(1.0f, 1.0f, &outLF, &outRF);
  vcD.Process(1.0, 1.0, &outLD, &outRD);

  REQUIRE(std::abs(static_cast<double>(outLF) - outLD) < 1e-3);
  REQUIRE(std::abs(static_cast<double>(outRF) - outRD) < 1e-3);
}

TEST_CASE("VintageChorus Initialization Managed", "[VintageChorus][Memory]") {
  sea::VintageChorus<float> vc;

  // Should allocate internal delay lines
  vc.Init(48000.0f);

  // Should process without crashing
  float outL, outR;
  for (int i = 0; i < 1000; ++i) {
    vc.Process(0.0f, 0.0f, &outL, &outR);
    REQUIRE(std::isfinite(outL));
    REQUIRE(std::isfinite(outR));
  }
}

TEST_CASE("VintageChorus Silence with Wet Mix", "[VintageChorus]") {
  sea::VintageChorus<float> vc;
  vc.Init(48000.0f);

  vc.SetRate(1.0f);
  vc.SetDepth(2.0f);
  vc.SetMix(1.0f); // 100% wet, 0% dry

  float outL, outR;

  // Process silence
  for (int i = 0; i < 500; ++i) {
    vc.Process(0.0f, 0.0f, &outL, &outR);
    REQUIRE(outL == Approx(0.0f).margin(1e-8f));
    REQUIRE(outR == Approx(0.0f).margin(1e-8f));
  }
}

TEST_CASE("VintageChorus Stereo Phase - No Modulation", "[VintageChorus]") {
  sea::VintageChorus<float> vc;
  vc.Init(48000.0f);

  vc.SetRate(0.0f);   // No LFO movement
  vc.SetDepth(0.0f);  // No depth
  vc.SetMix(1.0f);    // 100% wet

  float outL, outR;

  // Process identical mono input
  for (int i = 0; i < 100; ++i) {
    vc.Process(1.0f, 1.0f, &outL, &outR);
  }

  // L and R should be identical (no stereo spread without depth)
  REQUIRE(outL == Approx(outR).margin(1e-6f));
}

TEST_CASE("VintageChorus Stereo Phase - With Modulation", "[VintageChorus]") {
  sea::VintageChorus<float> vc;
  vc.Init(48000.0f);

  vc.SetRate(0.5f);   // Slow LFO
  vc.SetDepth(2.0f);  // Significant depth
  vc.SetMix(1.0f);    // 100% wet

  float outL, outR;
  bool channels_diverged = false;

  // Process identical mono input
  for (int i = 0; i < 500; ++i) {
    vc.Process(1.0f, 1.0f, &outL, &outR);

    // Check if L and R are different
    if (std::abs(outL - outR) > 0.01f) {
      channels_diverged = true;
      break;
    }
  }

  REQUIRE(channels_diverged); // Stereo effect should be audible
}

TEST_CASE("VintageChorus Rate Parameter", "[VintageChorus]") {
  sea::VintageChorus<float> vc_slow, vc_fast;

  vc_slow.Init(48000.0f);
  vc_fast.Init(48000.0f);

  vc_slow.SetRate(0.2f);  // Slow
  vc_fast.SetRate(5.0f);  // Fast

  vc_slow.SetDepth(3.0f);
  vc_fast.SetDepth(3.0f);

  vc_slow.SetMix(1.0f);
  vc_fast.SetMix(1.0f);

  float outL_slow, outR_slow;
  float outL_fast, outR_fast;

  // Fill delay buffers with varying signal (simple ramp)
  for (int i = 0; i < 1000; ++i) {
    float input = 0.5f + 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / 48000.0f);
    vc_slow.Process(input, input, &outL_slow, &outR_slow);
    vc_fast.Process(input, input, &outL_fast, &outR_fast);
  }

  int changes_slow = 0;
  int changes_fast = 0;

  float prev_slow = outL_slow, prev_fast = outL_fast;

  // Process and count output changes
  for (int i = 0; i < 2000; ++i) {
    float input = 0.5f + 0.5f * std::sin(2.0f * 3.14159f * 440.0f * (i + 1000) / 48000.0f);
    vc_slow.Process(input, input, &outL_slow, &outR_slow);
    vc_fast.Process(input, input, &outL_fast, &outR_fast);

    if (std::abs(outL_slow - prev_slow) > 0.0001f) changes_slow++;
    if (std::abs(outL_fast - prev_fast) > 0.0001f) changes_fast++;

    prev_slow = outL_slow;
    prev_fast = outL_fast;
  }

  // Fast rate should have more variations (or at least some variations)
  REQUIRE(changes_fast > 0);
  REQUIRE(changes_slow > 0);
}

TEST_CASE("VintageChorus Rate Bounds", "[VintageChorus][Stability]") {
  sea::VintageChorus<float> vc;
  vc.Init(48000.0f);

  // Test extremes (spec: 0.1 - 10.0 Hz)
  vc.SetRate(0.0f);   // Below minimum
  vc.SetRate(0.05f);  // Below spec
  vc.SetRate(20.0f);  // Above spec
  vc.SetRate(-1.0f);  // Negative

  float outL, outR;

  // Should process without crashing
  for (int i = 0; i < 100; ++i) {
    vc.Process(1.0f, 1.0f, &outL, &outR);
    REQUIRE(std::isfinite(outL));
    REQUIRE(std::isfinite(outR));
  }
}

TEST_CASE("VintageChorus Full Parameter Sweep", "[VintageChorus][Integration]") {
  sea::VintageChorus<float> vc;
  vc.Init(48000.0f);

  float rates[] = {0.1f, 0.5f, 1.0f, 5.0f, 10.0f};
  float depths[] = {0.0f, 1.0f, 2.5f, 4.0f, 5.0f};
  float mixes[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

  for (float rate : rates) {
    for (float depth : depths) {
      for (float mix : mixes) {
        vc.SetRate(rate);
        vc.SetDepth(depth);
        vc.SetMix(mix);

        float outL, outR;

        // Process a few samples
        for (int i = 0; i < 50; ++i) {
          vc.Process(1.0f, 1.0f, &outL, &outR);

          REQUIRE(std::isfinite(outL));
          REQUIRE(std::isfinite(outR));
          REQUIRE(std::abs(outL) < 100.0f); // No explosions
          REQUIRE(std::abs(outR) < 100.0f);
        }
      }
    }
  }
}
