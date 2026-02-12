#include "catch.hpp"
#include <sea_dsp/sea_adsr.h>

TEST_CASE("ADSREnvelope header is self-contained", "[Header][SelfContained][ADSR]") {
  sea::ADSREnvelope env;
  env.Init(48000.0);
  env.SetParams(0.01, 0.1, 0.5, 0.2);
  env.NoteOn();
  REQUIRE(env.Process() >= 0.0);
}
