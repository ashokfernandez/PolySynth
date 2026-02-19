#include "VoiceManager.h"
#include <catch.hpp>

using namespace PolySynthCore;

TEST_CASE("VoiceManager Polyphony Limiting and Stealing", "[VoiceManager]") {
  VoiceManager vm;
  vm.Init(44100.0);
  vm.SetADSR(0.0, 0.1, 1.0, 0.1); // Instant attack

  SECTION("Reducing polyphony gracefully kills voices") {
    vm.SetPolyphonyLimit(4);

    // Activate 4 voices
    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(62, 100);
    vm.OnNoteOn(64, 100);
    vm.OnNoteOn(65, 100);

    REQUIRE(vm.GetActiveVoiceCount() == 4);

    // Process a bit to get them running
    vm.Process();

    // Reduce limit to 1
    vm.SetPolyphonyLimit(1);

    // Should have 1 active (kept) and 3 stolen
    double l, r;
    vm.ProcessStereo(l, r);

    auto states = vm.GetVoiceStates();
    int stolenCount = 0;
    int activeCount = 0;
    for (const auto &s : states) {
      if (s.state == VoiceState::Stolen)
        stolenCount++;
      if (s.state != VoiceState::Idle)
        activeCount++;
    }

    CHECK(stolenCount == 3);
    CHECK(activeCount == 4); // 1 kept + 3 stolen

    // Process for 500 samples (~11ms) to verify voices are still fading out.

    for (int i = 0; i < 500; ++i)
      vm.Process();

    states = vm.GetVoiceStates();
    stolenCount = 0;
    for (const auto &s : states) {
      if (s.state == VoiceState::Stolen)
        stolenCount++;
    }

    // With 20ms fade, they should still be Stolen after 11ms.
    // So this test expects >= 1 stolen voices remaining.
    CHECK(stolenCount > 0);
  }
}

TEST_CASE("VoiceManager Stereo Spread", "[VoiceManager]") {
  VoiceManager vm;
  vm.Init(44100.0);

  SECTION("Spreads voices in non-unison mode") {
    vm.SetPolyphonyLimit(4);
    vm.SetStereoSpread(1.0); // Max width
    vm.SetUnisonCount(1);    // Non-unison

    // Play 2 notes
    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(64, 100);

    auto states = vm.GetVoiceStates();
    int active = 0;
    // float panningSum = 0.0f;
    float absPanningSum = 0.0f;

    for (const auto &s : states) {
      if (s.state != VoiceState::Idle) {
        active++;
        // panningSum += s.panPosition;
        absPanningSum += std::abs(s.panPosition);
      }
    }

    REQUIRE(active == 2);

    // Voices should be panned apart.
    CHECK(absPanningSum > 0.1f);
  }
}
