#include "catch.hpp"
#include <sea_dsp/sea_oscillator.h>

TEST_CASE("Oscillator header is self-contained", "[Header][SelfContained][Oscillator]") {
  sea::Oscillator osc;
  osc.Init(48000.0);
  osc.SetFrequency(440.0);
  REQUIRE(osc.Process() >= -1.0);
}
