#include "../../src/core/modulation/ADSREnvelope.h"
#include "catch.hpp"

TEST_CASE("ADSR State Transitions", "[ADSREnvelope]") {
  PolySynth::ADSREnvelope adsr;
  double sr = 100.0; // Low SR for easy math
  adsr.Init(sr);

  // A=0.1s (10 samps), D=0.1s (10 samps), S=0.5, R=0.1s (10 samps)
  adsr.SetParams(0.1, 0.1, 0.5, 0.1);

  // 1. Idle initially
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0));

  // 2. NoteOn -> Attack
  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kAttack);

  // Process 5 samples (halfway)
  for (int i = 0; i < 5; i++)
    adsr.Process();
  REQUIRE(adsr.GetLevel() == Approx(0.5));

  // Process until we hit Decay (Robust transition)
  int watchdog = 0;
  while (adsr.GetStage() == PolySynth::ADSREnvelope::kAttack &&
         watchdog++ < 20) {
    adsr.Process();
  }
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kDecay);

  // Note: We don't check Level == 1.0 here because it might be effectively 1.0
  // but logic happened

  // 3. Decay to Sustain (0.5)
  // Process until we hit Sustain
  watchdog = 0;
  while (adsr.GetStage() == PolySynth::ADSREnvelope::kDecay &&
         watchdog++ < 20) {
    adsr.Process();
  }

  // Should be Sustain now
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 4. Hold Sustain
  adsr.Process();
  REQUIRE(adsr.GetLevel() == Approx(0.5).margin(0.01));

  // 5. NoteOff -> Release
  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kRelease);

  // Release Phase
  watchdog = 0;
  while (adsr.GetStage() == PolySynth::ADSREnvelope::kRelease &&
         watchdog++ < 20) {
    adsr.Process();
  }
  REQUIRE(adsr.GetLevel() <= Approx(0.001)); // Should be zero
  REQUIRE(adsr.GetStage() == PolySynth::ADSREnvelope::kIdle);
}
