#include "catch.hpp"
#include <sea_dsp/sea_oscillator.h>

TEST_CASE("Oscillator saw waveform remains bounded and ramps upward", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(48000.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Saw);
  osc.SetFrequency(480.0); // 100 samples per cycle
  osc.Reset();

  double previous = osc.Process();
  REQUIRE(previous >= -1.0);
  REQUIRE(previous <= 1.0);

  bool sawWrapDetected = false;
  for (int i = 0; i < 200; ++i) {
    double current = osc.Process();
    REQUIRE(current >= -1.0);
    REQUIRE(current <= 1.0);

    if (current < previous - 1.5) {
      sawWrapDetected = true;
    }
    previous = current;
  }

  REQUIRE(sawWrapDetected);
}

TEST_CASE("Oscillator waveforms at phase-zero match expected values", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(44100.0);
  osc.SetFrequency(440.0);

  SECTION("Sawtooth") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Saw);
    osc.Reset();
    REQUIRE(osc.Process() == Approx(-1.0).margin(1e-6));
  }

  SECTION("Square") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Square);
    osc.Reset();
    REQUIRE(osc.Process() == Approx(1.0).margin(1e-6));
  }

  SECTION("Triangle") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Triangle);
    osc.Reset();
    REQUIRE(osc.Process() == Approx(1.0).margin(1e-6));
  }

  SECTION("Sine") {
    osc.SetWaveform(sea::Oscillator::WaveformType::Sine);
    osc.Reset();
    REQUIRE(osc.Process() == Approx(0.0).margin(1e-6));
  }
}

TEST_CASE("Oscillator pulse width is clamped for square waveform", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(44100.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Square);
  osc.SetFrequency(440.0);

  osc.SetPulseWidth(0.0);
  osc.Reset();

  int positiveCount = 0;
  for (int i = 0; i < 200; ++i) {
    if (osc.Process() > 0.0) {
      ++positiveCount;
    }
  }

  // If PW were not clamped above 0, we'd get almost always -1.
  REQUIRE(positiveCount > 0);
}
