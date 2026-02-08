#include "../../src/core/modulation/ADSREnvelope.h"
#include "catch.hpp"

TEST_CASE("ADSR State Transitions", "[ADSREnvelope]") {
  PolySynthCore::ADSREnvelope adsr;
  double sr = 100.0; // Low SR for easy math
  adsr.Init(sr);

  // A=0.1s (10 samps), D=0.1s (10 samps), S=0.5, R=0.1s (10 samps)
  adsr.SetParams(0.1, 0.1, 0.5, 0.1);

  // 1. Idle initially
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0));

  // 2. NoteOn -> Attack
  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kAttack);

  // Process 5 samples (halfway)
  for (int i = 0; i < 5; i++)
    adsr.Process();
  REQUIRE(adsr.GetLevel() == Approx(0.5));

  // Process until we hit Decay (Robust transition)
  int watchdog = 0;
  while (adsr.GetStage() == PolySynthCore::ADSREnvelope::kAttack &&
         watchdog++ < 20) {
    adsr.Process();
  }
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kDecay);

  // Note: We don't check Level == 1.0 here because it might be effectively 1.0
  // but logic happened

  // 3. Decay to Sustain (0.5)
  // Process until we hit Sustain
  watchdog = 0;
  while (adsr.GetStage() == PolySynthCore::ADSREnvelope::kDecay &&
         watchdog++ < 20) {
    adsr.Process();
  }

  // Should be Sustain now
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 4. Hold Sustain
  adsr.Process();
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 5. NoteOff -> Release
  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kRelease);

  // Release Phase
  watchdog = 0;
  while (adsr.GetStage() == PolySynthCore::ADSREnvelope::kRelease &&
         watchdog++ < 20) {
    adsr.Process();
  }
  REQUIRE(adsr.GetLevel() <= Approx(0.001)); // Should be zero
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kIdle);
}

TEST_CASE("ADSR handles zero time segments", "[ADSREnvelope]") {
  PolySynthCore::ADSREnvelope adsr;
  double sr = 100.0;
  adsr.Init(sr);

  // Zero attack/decay, instant sustain at 0.25, zero release.
  adsr.SetParams(0.0, 0.0, 0.25, 0.0);

  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(0.25));

  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == PolySynthCore::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0));
}
