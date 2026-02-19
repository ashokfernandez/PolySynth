#include "../../src/core/VoiceManager.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("Stereo: center pan gives equal L/R", "[StereoPanning]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  vm.OnNoteOn(60, 100);
  // Set all voices to center pan (default)

  // Process stereo
  double left = 0.0, right = 0.0;
  vm.ProcessStereo(left, right);

  // Both channels should have equal magnitude
  REQUIRE(std::abs(left) == Approx(std::abs(right)).margin(0.001));
}
