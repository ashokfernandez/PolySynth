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

  // First sample should be -1.0 (phase 0, default Saw)
  REQUIRE(osc.Process() == Approx(-1.0));

  // Advance 50 samples (half cycle) -> should be close to 0.0
  for (int i = 0; i < 49; i++) {
    osc.Process();
  }

  // At sample 50 (index 50), phase is 0.5. Output should be 0.0
  REQUIRE(osc.Process() == Approx(0.0).margin(0.01));

  // Advance another 50 samples -> should wrap around
  for (int i = 0; i < 49; i++) {
    osc.Process();
  }

  // At sample 100, phase wraps.
  REQUIRE(osc.Process() == Approx(-1.0).margin(0.01));
}

TEST_CASE("Oscillator Waveforms", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(44100.0);
  osc.SetFrequency(440.0);

  SECTION("Sawtooth") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Saw);
    osc.Reset();
    REQUIRE(osc.Process() == Approx(-1.0));
  }

  SECTION("Square") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Square);
    osc.Reset();
    // At phase 0, Square is 1.0 (0 < 0.5)
    REQUIRE(osc.Process() == Approx(1.0));
  }

  SECTION("Triangle") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Triangle);
    osc.Reset();
    // At phase 0, Triangle is (4 * 0.5) - 1 = 1.0
    REQUIRE(osc.Process() == Approx(1.0).margin(0.01));
  }

  SECTION("Sine") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Sine);
    osc.Reset();
    // At phase 0, Sine is 0.0
    REQUIRE(osc.Process() == Approx(0.0).margin(0.01));
  }
}
