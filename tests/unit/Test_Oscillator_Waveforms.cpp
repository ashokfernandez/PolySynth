#include "../../src/core/oscillator/Oscillator.h"
#include "catch.hpp"

TEST_CASE("Oscillator Waveform Ranges", "[Oscillator]") {
  PolySynthCore::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);

  const PolySynthCore::Oscillator::WaveformType waveforms[] = {
      PolySynthCore::Oscillator::WaveformType::Saw,
      PolySynthCore::Oscillator::WaveformType::Square,
      PolySynthCore::Oscillator::WaveformType::Triangle,
      PolySynthCore::Oscillator::WaveformType::Sine};

  for (auto waveform : waveforms) {
    osc.SetWaveform(waveform);
    osc.Reset();

    for (int i = 0; i < 100; ++i) {
      const auto sample = osc.Process();
      REQUIRE(sample <= 1.0);
      REQUIRE(sample >= -1.0);
    }
  }
}

TEST_CASE("Oscillator Saw Slope at Phase Zero", "[Oscillator]") {
  PolySynthCore::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  osc.Reset();

  const auto first = osc.Process();
  const auto second = osc.Process();
  REQUIRE(second > first);
}

TEST_CASE("Oscillator Square Outputs Only Extremes", "[Oscillator]") {
  PolySynthCore::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(PolySynthCore::Oscillator::WaveformType::Square);
  osc.Reset();

  for (int i = 0; i < 100; ++i) {
    const auto sample = osc.Process();
    REQUIRE((sample == 1.0 || sample == -1.0));
  }
}

TEST_CASE("Oscillator Sine Starts at Zero", "[Oscillator]") {
  PolySynthCore::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(PolySynthCore::Oscillator::WaveformType::Sine);
  osc.Reset();

  REQUIRE(osc.Process() == Approx(0.0));
}
