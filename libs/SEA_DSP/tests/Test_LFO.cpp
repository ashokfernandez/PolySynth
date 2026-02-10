#include "catch.hpp"
#include <sea_dsp/sea_lfo.h>

TEST_CASE("LFO Basic", "[LFO]") {
  sea::LFO lfo;
  lfo.Init(44100.0);
  lfo.SetWaveform(0); // Sine
  lfo.SetRate(1.0);   // 1Hz
  lfo.SetDepth(1.0);

  SECTION("Initial value") {
    // Phase starts at 0, Sine(2*pi*0) = 0
    // Use tight margin for values that should be exactly zero
    REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));
  }

  SECTION("Peak value") {
    lfo.Init(44100.0);
    lfo.SetWaveform(0);
    lfo.SetRate(1.0);
    lfo.SetDepth(1.0);

    // Advance a quarter cycle (0.25s / 44100) -> 11025 samples
    for (int i = 0; i < 11024; ++i)
      lfo.Process();

    // At sample 11025, phase is 0.25, Sin(pi/2) = 1.0
    // Use tighter margin - should be very close to 1.0
    REQUIRE(lfo.Process() == Approx(1.0).margin(1e-6));
  }
}
