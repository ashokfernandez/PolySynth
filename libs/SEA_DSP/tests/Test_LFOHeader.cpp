#include "catch.hpp"
#include <sea_dsp/sea_lfo.h>

TEST_CASE("LFO header is self-contained", "[Header][SelfContained][LFO]") {
  sea::LFO lfo;
  lfo.Init(48000.0);
  lfo.SetRate(1.0);
  REQUIRE(lfo.Process() >= -1.0);
}
