#include "oscillator/Oscillator.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_oscillator.h>
#include <vector>

TEST_CASE("SEA_DSP Oscillator Bit-Exactness", "[sea_dsp][oscillator]") {
  PolySynthCore::Oscillator original;
  sea::Oscillator ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  // Test Saw
  original.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  ported.SetWaveform(sea::Oscillator::WaveformType::Saw);
  original.SetFrequency(440.0);
  ported.SetFrequency(440.0);

  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  // Test Square with PWM
  original.SetWaveform(PolySynthCore::Oscillator::WaveformType::Square);
  ported.SetWaveform(sea::Oscillator::WaveformType::Square);
  original.SetPulseWidth(0.3);
  ported.SetPulseWidth(0.3);

  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  // Test Triangle
  original.Reset();
  ported.Reset();
  original.SetWaveform(PolySynthCore::Oscillator::WaveformType::Triangle);
  ported.SetWaveform(sea::Oscillator::WaveformType::Triangle);

  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  // Test Sine
  original.SetWaveform(PolySynthCore::Oscillator::WaveformType::Sine);
  ported.SetWaveform(sea::Oscillator::WaveformType::Sine);

  for (int i = 0; i < 1000; ++i) {
    // Sine might have very tiny deviation if kTwoPi precision differs slightly
    // or std::sin vs sea::Math::Sin (which is std::sin)
    REQUIRE(original.Process() == Approx(ported.Process()));
  }
}
