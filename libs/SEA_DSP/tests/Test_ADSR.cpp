#include "catch.hpp"
#include <sea_dsp/sea_adsr.h>

TEST_CASE("ADSR Basic Lifecycle", "[ADSR]") {
  sea::ADSREnvelope adsr;
  adsr.Init(44100.0);
  adsr.SetParams(0.01, 0.1, 0.5, 0.2); // A, D, S, R in seconds

  SECTION("Initial State") {
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kIdle);
    REQUIRE(adsr.Process() == Approx(0.0));
  }

  SECTION("Attack Segment") {
    adsr.NoteOn();
    REQUIRE(adsr.GetStage() == sea::ADSREnvelope::kAttack);
    double val = 0.0;
    // Process until peak (approx 0.01s = 441 samples)
    for (int i = 0; i < 500; ++i)
      val = adsr.Process();
    REQUIRE(val > 0.9);
  }
}
