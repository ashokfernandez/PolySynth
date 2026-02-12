#include "catch.hpp"
#include <sea_dsp/sea_adsr.h>

namespace {
void ProcessSamples(sea::ADSREnvelope &adsr, int sampleCount) {
  for (int i = 0; i < sampleCount; ++i) {
    adsr.Process();
  }
}
} // namespace

TEST_CASE("ADSR Initial and idle behavior", "[ADSR]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.1, 0.5, 0.2);

  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
  REQUIRE(adsr.IsActive() == false);
  REQUIRE(adsr.Process() == Approx(0.0).margin(1e-12));
}

TEST_CASE("ADSR full stage lifecycle follows A-D-S-R contract", "[ADSR]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.05, 0.4, 0.02);

  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kAttack);

  ProcessSamples(adsr, 441);
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kDecay);
  REQUIRE(adsr.GetLevel() == Approx(1.0).margin(1e-3));

  ProcessSamples(adsr, 2205);
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(0.4).margin(1e-3));

  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kRelease);

  ProcessSamples(adsr, 882);
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0).margin(1e-6));
}

TEST_CASE("ADSR handles zero-time parameters without hanging between stages", "[ADSR]") {
  sea::ADSREnvelope adsr;
  adsr.Init(48000.0);

  SECTION("Zero attack jumps to decay/sustain immediately") {
    adsr.SetParams(0.0, 0.01, 0.25, 0.05);
    adsr.NoteOn();

    REQUIRE((adsr.GetStage() == sea::ADSREnvelope::kDecay ||
             adsr.GetStage() == sea::ADSREnvelope::kSustain));

    ProcessSamples(adsr, 1000);
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
    REQUIRE(adsr.GetLevel() == Approx(0.25).margin(1e-4));
  }

  SECTION("Zero decay lands on sustain immediately after attack") {
    adsr.SetParams(0.01, 0.0, 0.6, 0.05);
    adsr.NoteOn();

    ProcessSamples(adsr, 480);
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
    REQUIRE(adsr.GetLevel() == Approx(0.6).margin(1e-4));
  }

  SECTION("Zero release returns to idle immediately on note-off") {
    adsr.SetParams(0.01, 0.01, 0.7, 0.0);
    adsr.NoteOn();
    ProcessSamples(adsr, 2000);
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);

    adsr.NoteOff();
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
    REQUIRE(adsr.GetLevel() == Approx(0.0).margin(1e-12));
  }
}

TEST_CASE("ADSR parameter clamping is enforced", "[ADSR]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);

  // Negative times should clamp to zero, sustain should clamp to [0, 1].
  adsr.SetParams(-1.0, -1.0, 2.0, -1.0);

  adsr.NoteOn();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kSustain);
  REQUIRE(adsr.GetLevel() == Approx(1.0).margin(1e-12));

  adsr.NoteOff();
  REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
  REQUIRE(adsr.GetLevel() == Approx(0.0).margin(1e-12));
}
