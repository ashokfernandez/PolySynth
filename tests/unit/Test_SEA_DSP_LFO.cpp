#include "modulation/LFO.h"
#include <catch.hpp>
#include <cmath>
#include <sea_dsp/sea_lfo.h>
#include <vector>

TEST_CASE("SEA_DSP LFO Bit-Exactness", "[sea_dsp][lfo]") {
  PolySynthCore::LFO original;
  sea::LFO ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  // Test Sine
  original.SetWaveform(0);
  ported.SetWaveform(0);
  original.SetRate(5.0);
  ported.SetRate(5.0);
  original.SetDepth(1.0);
  ported.SetDepth(1.0);

  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == Approx(ported.Process()));
  }

  // Test Saw with partial depth
  original.SetWaveform(3);
  ported.SetWaveform(3);
  original.SetDepth(0.5);
  ported.SetDepth(0.5);

  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  // Test Square
  original.SetWaveform(2);
  ported.SetWaveform(2);
  for (int i = 0; i < 1000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }
}
