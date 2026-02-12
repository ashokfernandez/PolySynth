#include "catch.hpp"
#include "sea_dsp/effects/sea_vintage_delay.h"
#include <cmath>
#include <vector>

TEST_CASE("VintageDelay Self-Contained Header", "[VintageDelay][SelfContained]") {
  sea::VintageDelay<float> vd;
  REQUIRE(true);
}

TEST_CASE("VintageDelay Type Safety - Float vs Double", "[VintageDelay][TypeSafety]") {
  sea::VintageDelay<float> vdF;
  sea::VintageDelay<double> vdD;

  vdF.Init(48000.0f, 500.0f);
  vdD.Init(48000.0, 500.0);

  vdF.SetTime(100.0f);
  vdD.SetTime(100.0);

  vdF.SetFeedback(50.0f);
  vdD.SetFeedback(50.0);

  vdF.SetMix(50.0f);
  vdD.SetMix(50.0);

  float outLF, outRF;
  double outLD, outRD;

  vdF.Process(1.0f, 1.0f, &outLF, &outRF);
  vdD.Process(1.0, 1.0, &outLD, &outRD);

  REQUIRE(std::abs(static_cast<double>(outLF) - outLD) < 1e-3);
  REQUIRE(std::abs(static_cast<double>(outRF) - outRD) < 1e-3);
}

TEST_CASE("VintageDelay Initialization Managed", "[VintageDelay][Memory]") {
  sea::VintageDelay<float> vd;

  vd.Init(48000.0f, 1000.0f); // 1 second max

  float outL, outR;
  for (int i = 0; i < 1000; ++i) {
    vd.Process(0.0f, 0.0f, &outL, &outR);
    REQUIRE(std::isfinite(outL));
    REQUIRE(std::isfinite(outR));
  }
}

TEST_CASE("VintageDelay Silence", "[VintageDelay]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 500.0f);

  vd.SetTime(100.0f);
  vd.SetFeedback(0.0f);
  vd.SetMix(100.0f);

  float outL, outR;

  for (int i = 0; i < 1000; ++i) {
    vd.Process(0.0f, 0.0f, &outL, &outR);
    REQUIRE(outL == Approx(0.0f).margin(1e-8f));
    REQUIRE(outR == Approx(0.0f).margin(1e-8f));
  }
}

TEST_CASE("VintageDelay Mix Logic", "[VintageDelay]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 100.0f);

  vd.SetTime(10.0f);  // 10ms = 480 samples at 48kHz
  vd.SetFeedback(0.0f);
  vd.SetMix(50.0f);

  float outL, outR;

  // Fill delay buffer with 1.0 values
  for (int i = 0; i < 600; ++i) {
    vd.Process(1.0f, 1.0f, &outL, &outR);
  }

  // After 600 samples, the delayed signal (10ms ago) should be 1.0
  // Output should be: 1.0 (dry) + 1.0 (delayed) * 0.5 (mix) = 1.5
  REQUIRE(outL > 1.1f);  // Should be significantly more than dry-only
  REQUIRE(outL < 2.0f);  // But not crazy large

  // Test with different mix values
  vd.SetMix(0.0f);
  vd.Process(1.0f, 1.0f, &outL, &outR);
  float dry_only = outL;
  REQUIRE(dry_only == Approx(1.0f).margin(0.01f));

  vd.SetMix(100.0f);
  vd.Process(1.0f, 1.0f, &outL, &outR);
  REQUIRE(outL > dry_only);  // 100% mix should add more wet than 0% mix
}

TEST_CASE("VintageDelay Feedback Repeats", "[VintageDelay][Feedback]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 500.0f);

  vd.SetTime(50.0f);
  vd.SetFeedback(70.0f);
  vd.SetMix(100.0f);

  float outL, outR;

  vd.Process(1.0f, 1.0f, &outL, &outR);

  int repeat_count = 0;
  float prev_outL = 0.0f;

  for (int i = 0; i < 20000; ++i) {
    vd.Process(0.0f, 0.0f, &outL, &outR);

    if (outL > 0.1f && prev_outL < 0.1f) {
      repeat_count++;
    }
    prev_outL = outL;
  }

  // With filtering and saturation, 2+ repeats is reasonable for 70% feedback
  REQUIRE(repeat_count >= 2);
}

TEST_CASE("VintageDelay Feedback Saturation", "[VintageDelay][Feedback][Saturation]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 200.0f);

  vd.SetTime(20.0f);
  vd.SetFeedback(110.0f); // >100%
  vd.SetMix(100.0f);

  for (int i = 0; i < 100; ++i) {
    float noise = static_cast<float>(rand()) / RAND_MAX * 0.1f;
    float outL, outR;
    vd.Process(noise, noise, &outL, &outR);
  }

  float max_output = 0.0f;
  for (int i = 0; i < 10000; ++i) {
    float outL, outR;
    vd.Process(0.0f, 0.0f, &outL, &outR);

    max_output = std::max(max_output, std::abs(outL));

    REQUIRE(std::isfinite(outL));
    REQUIRE(std::abs(outL) < 100.0f);
  }

  // With filtering and sigmoid saturation, output remains bounded
  // Input noise is 0.1 amplitude, so expect modest output even with >100% feedback
  REQUIRE(max_output > 0.05f);  // Some self-oscillation occurs
  REQUIRE(max_output < 5.0f);    // But saturation prevents runaway
}

TEST_CASE("VintageDelay Stereo Width", "[VintageDelay]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 500.0f);

  vd.SetTime(100.0f);
  vd.SetFeedback(0.0f);
  vd.SetMix(100.0f);

  float outL, outR;

  vd.Process(1.0f, 1.0f, &outL, &outR);

  int left_delay_time = -1;
  int right_delay_time = -1;

  for (int i = 0; i < 10000; ++i) {
    vd.Process(0.0f, 0.0f, &outL, &outR);

    if (left_delay_time == -1 && outL > 0.1f) {
      left_delay_time = i;
    }
    if (right_delay_time == -1 && outR > 0.1f) {
      right_delay_time = i;
    }
  }

  int delta = right_delay_time - left_delay_time;
  REQUIRE(delta == Approx(720).margin(100)); // 15ms = 720 samples at 48kHz
}

TEST_CASE("VintageDelay Clear Function", "[VintageDelay]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 500.0f);

  vd.SetTime(100.0f);
  vd.SetFeedback(90.0f);
  vd.SetMix(100.0f);

  for (int i = 0; i < 1000; ++i) {
    float noise = static_cast<float>(rand()) / RAND_MAX;
    float outL, outR;
    vd.Process(noise * 0.1f, noise * 0.1f, &outL, &outR);
  }

  vd.Clear();

  float outL, outR;
  for (int i = 0; i < 100; ++i) {
    vd.Process(0.0f, 0.0f, &outL, &outR);
    REQUIRE(outL == Approx(0.0f).margin(1e-6f));
    REQUIRE(outR == Approx(0.0f).margin(1e-6f));
  }
}

TEST_CASE("VintageDelay Full Parameter Sweep", "[VintageDelay][Integration]") {
  sea::VintageDelay<float> vd;
  vd.Init(48000.0f, 500.0f);

  float times[] = {10.0f, 50.0f, 100.0f, 250.0f, 500.0f};
  float feedbacks[] = {0.0f, 25.0f, 50.0f, 90.0f, 110.0f};
  float mixes[] = {0.0f, 25.0f, 50.0f, 75.0f, 100.0f};

  for (float time : times) {
    for (float feedback : feedbacks) {
      for (float mix : mixes) {
        vd.Clear();
        vd.SetTime(time);
        vd.SetFeedback(feedback);
        vd.SetMix(mix);

        float outL, outR;

        for (int i = 0; i < 50; ++i) {
          vd.Process(1.0f, 1.0f, &outL, &outR);

          REQUIRE(std::isfinite(outL));
          REQUIRE(std::isfinite(outR));
          REQUIRE(std::abs(outL) < 100.0f);
          REQUIRE(std::abs(outR) < 100.0f);
        }
      }
    }
  }
}
