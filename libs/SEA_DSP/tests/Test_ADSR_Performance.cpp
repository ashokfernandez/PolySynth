#include "catch.hpp"
#include <sea_dsp/sea_adsr.h>
#include <chrono>
#include <cmath>

TEST_CASE("ADSR NoteOff Correctness", "[ADSR][Correctness]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.1, 0.5, 0.2);

  SECTION("Release from different levels") {
    // Test 1: Release from peak (level = 1.0)
    adsr.Reset();
    adsr.NoteOn();
    for (int i = 0; i < 500; ++i) adsr.Process(); // Reach attack peak

    REQUIRE(adsr.GetLevel() > 0.9);
    double peakLevel = adsr.GetLevel();

    adsr.NoteOff();
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kRelease);

    // Process release - should decay linearly
    double level1 = adsr.Process();
    double level2 = adsr.Process();
    double decrement = level1 - level2;

    // Verify consistent decrement (linear release)
    for (int i = 0; i < 100; ++i) {
      double prev = adsr.GetLevel();
      adsr.Process();
      double current = adsr.GetLevel();
      REQUIRE((prev - current) == Approx(decrement).epsilon(0.0001));
    }

    // Test 2: Release from sustain (level = 0.5)
    adsr.Reset();
    adsr.NoteOn();
    for (int i = 0; i < 10000; ++i) adsr.Process(); // Reach sustain

    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
    REQUIRE(adsr.GetLevel() == Approx(0.5));

    adsr.NoteOff();
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kRelease);

    // Should release proportionally to level
    double sustainLevel = adsr.GetLevel();
    level1 = adsr.Process();
    level2 = adsr.Process();
    double sustainDecrement = level1 - level2;

    // Release from sustain should be proportional
    REQUIRE(sustainDecrement < decrement); // Slower from lower level
    REQUIRE(sustainDecrement > 0.0);
  }

  SECTION("Multiple note cycles") {
    // Simulate real-world usage: multiple note on/off cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
      adsr.Reset();
      adsr.NoteOn();

      // Attack to peak
      for (int i = 0; i < 500; ++i) adsr.Process();

      // Decay to sustain
      for (int i = 0; i < 5000; ++i) adsr.Process();

      double levelBeforeRelease = adsr.GetLevel();
      adsr.NoteOff();

      // Release to idle
      int releaseCount = 0;
      while (adsr.GetStage() != sea::ADSREnvelope::kIdle && releaseCount < 20000) {
        adsr.Process();
        releaseCount++;
      }

      REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
      REQUIRE(adsr.GetLevel() == Approx(0.0));
    }
  }
}

TEST_CASE("ADSR NoteOff No Division", "[ADSR][Performance]") {
  // This test verifies that the optimization works by checking
  // that different release levels use the same coefficient
  sea::ADSREnvelope adsr1, adsr2;
  adsr1.Init(44100.0);
  adsr2.Init(44100.0);

  adsr1.SetParams(0.01, 0.1, 0.5, 0.2);
  adsr2.SetParams(0.01, 0.1, 0.5, 0.2);

  // Get to different levels
  adsr1.NoteOn();
  for (int i = 0; i < 500; ++i) adsr1.Process(); // Peak
  double level1 = adsr1.GetLevel();

  adsr2.NoteOn();
  for (int i = 0; i < 10000; ++i) adsr2.Process(); // Sustain
  double level2 = adsr2.GetLevel();

  REQUIRE(level1 > level2); // Different starting levels

  // Trigger release
  adsr1.NoteOff();
  adsr2.NoteOff();

  // The release slope should be proportional to starting level
  double dec1 = adsr1.GetLevel();
  adsr1.Process();
  dec1 -= adsr1.GetLevel();

  double dec2 = adsr2.GetLevel();
  adsr2.Process();
  dec2 -= adsr2.GetLevel();

  // Ratio of decrements should match ratio of levels
  double ratio = dec1 / dec2;
  double levelRatio = level1 / level2;

  REQUIRE(ratio == Approx(levelRatio).epsilon(0.01));
}

// Benchmark test (informational, not a failure condition)
TEST_CASE("ADSR Performance Benchmark", "[ADSR][.benchmark]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.1, 0.5, 0.2);

  const int iterations = 100000;

  // Measure NoteOff performance
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    adsr.Reset();
    adsr.NoteOn();
    for (int j = 0; j < 10; ++j) adsr.Process();
    adsr.NoteOff();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Just informational
  INFO("NoteOff called " << iterations << " times in " << duration.count() << " microseconds");
  INFO("Average: " << (duration.count() / static_cast<double>(iterations)) << " microseconds per call");

  // Sanity check - should be very fast
  REQUIRE(duration.count() < 1000000); // Less than 1 second total
}
