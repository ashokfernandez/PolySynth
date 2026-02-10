#include "../../src/core/types.h"
#include "catch.hpp"
#include <sea_dsp/sea_oscillator.h>

TEST_CASE("Oscillator Waveform Ranges", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);

  const sea::Oscillator::WaveformType waveforms[] = {
      sea::Oscillator::WaveformType::Saw, sea::Oscillator::WaveformType::Square,
      sea::Oscillator::WaveformType::Triangle,
      sea::Oscillator::WaveformType::Sine};

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
  sea::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Saw);
  osc.Reset();

  const auto first = osc.Process();
  const auto second = osc.Process();
  REQUIRE(second > first);
}

TEST_CASE("Oscillator Square Outputs Only Extremes", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Square);
  osc.Reset();

  for (int i = 0; i < 100; ++i) {
    const auto sample = osc.Process();
    REQUIRE((sample == 1.0 || sample == -1.0));
  }
}

TEST_CASE("Oscillator Sine Starts at Zero", "[Oscillator]") {
  sea::Oscillator osc;
  osc.Init(100.0);
  osc.SetFrequency(1.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Sine);
  osc.Reset();

  REQUIRE(osc.Process() == Approx(0.0));
}
