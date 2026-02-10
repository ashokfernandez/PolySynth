#include "modulation/ADSREnvelope.h"
#include <catch.hpp>
#include <sea_dsp/sea_adsr.h>
#include <vector>

TEST_CASE("SEA_DSP ADSREnvelope Bit-Exactness", "[sea_dsp][adsr]") {
  PolySynthCore::ADSREnvelope original;
  sea::ADSREnvelope ported;

  double sampleRate = 44100.0;
  original.Init(sampleRate);
  ported.Init(sampleRate);

  // Case A: Normal ADSR cycle
  original.SetParams(0.1, 0.2, 0.5, 0.3);
  ported.SetParams(0.1, 0.2, 0.5, 0.3);

  original.NoteOn();
  ported.NoteOn();

  // Process for a while (Attack + Decay + Sustain)
  for (int i = 0; i < 20000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  original.NoteOff();
  ported.NoteOff();

  // Process Release
  for (int i = 0; i < 20000; ++i) {
    REQUIRE(original.Process() == ported.Process());
  }

  // Case B: Instant Attack/Release
  original.SetParams(0.0, 0.1, 0.5, 0.0);
  ported.SetParams(0.0, 0.1, 0.5, 0.0);

  original.NoteOn();
  ported.NoteOn();
  REQUIRE(original.Process() == ported.Process());

  original.NoteOff();
  ported.NoteOff();
  REQUIRE(original.Process() == ported.Process());
  REQUIRE(original.IsActive() == ported.IsActive());
}
