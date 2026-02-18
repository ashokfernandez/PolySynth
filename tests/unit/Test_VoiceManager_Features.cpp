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
    // BUT GetActiveVoiceCount might verify 'active' flag.
    // Stolen voices are still 'active' until they fade out?
    // Let's check Voice::Process logic: if mVoiceState == Stolen, mActive is
    // still true until mStolenFadeGain <= 0.

    // We expect the voices to NOT be killed instantly (crunch).
    // If they were killed instantly, Process() would return 0 for them or
    // they'd be inactive immediately.

    // Let's verify we still have 'active' voices processing the fade
    // The allocator's EnforcePolyphonyLimit calls StartSteal()

    // If fade is 2ms (default), at 44.1kHz that's ~88 samples.
    // Let's process 10 samples and verify output is not 0 (because they are
    // fading). Then process 200 samples and verify they represent
    // silence/inactive.

    double l, r;
    vm.ProcessStereo(l, r);
    // We can't easily isolate the stolen voice output here because Process
    // relies on summation. But we can check internal state via GetVoiceStates
    // (Sprint 1 helper).

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

    // Process for 500 samples (~11ms).
    // If fade is 2ms, they should be dead by now.
    // If fade is longer (which we want e.g. 20ms), they should still be
    // fading/stolen.

    for (int i = 0; i < 500; ++i)
      vm.Process();

    states = vm.GetVoiceStates();
    stolenCount = 0;
    for (const auto &s : states) {
      if (s.state == VoiceState::Stolen)
        stolenCount++;
    }

    // With 2ms fade, they should be gone (stolenCount == 0, state -> Idle).
    // With 20ms fade, they should still be Stolen.
    // We WANT them to still be Stolen (longer fade).
    // So this test expects >= 1 stolen voices remaining after 11ms.
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
    float panningSum = 0.0f;
    float absPanningSum = 0.0f;

    for (const auto &s : states) {
      if (s.state != VoiceState::Idle) {
        active++;
        panningSum += s.panPosition;
        absPanningSum += std::abs(s.panPosition);
      }
    }

    REQUIRE(active == 2);

    // Users expectation: Voices should be panned apart.
    // Currently: OnNoteOn with Unison=1 sets pan to 0.0.
    // So absPanningSum will be 0.0.
    // We WANT it to be > 0.0.
    CHECK(absPanningSum > 0.1f);
  }
}
