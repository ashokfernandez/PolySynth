#include "../../src/core/types.h"
#include "catch.hpp"
#include <sea_dsp/sea_adsr.h>

TEST_CASE("ADSR State Transitions", "[ADSREnvelope]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.1, 0.5, 0.2);

  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0));

  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kAttack);

  // Process until we hit Decay (Robust transition)
  while (adsr.GetStage() == sea::ADSREnvelope::kAttack &&
         adsr.Process() < 1.0) {
  }
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kDecay);

  // 3. Decay to Sustain (0.5)
  while (adsr.GetStage() == sea::ADSREnvelope::kDecay && adsr.Process() > 0.5) {
  }
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 4. Hold Sustain
  adsr.Process();
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 5. NoteOff -> Release
  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kRelease);

  // Release Phase
  while (adsr.GetStage() == sea::ADSREnvelope::kRelease &&
         adsr.Process() > 0.0) {
  }
  REQUIRE(adsr.GetLevel() <= Approx(0.001)); // Should be zero
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
}

TEST_CASE("ADSR handles zero time segments", "[ADSREnvelope]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.0, 0.0, 1.0, 0.0);

  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(1.0)); // Sustain level is 1.0

  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0));
}
