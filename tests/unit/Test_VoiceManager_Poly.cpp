#include "../../src/core/VoiceManager.h"
#include "catch.hpp"

TEST_CASE("VoiceManager Polyphony", "[VoiceManager]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  // 1. Play a chord (3 notes)
  vm.OnNoteOn(60, 100); // C4
  vm.OnNoteOn(64, 100); // E4
  vm.OnNoteOn(67, 100); // G4

  // Process a bit
  PolySynthCore::sample_t out = vm.Process();
  REQUIRE(std::abs(out) > 0.0); // Should have output
  REQUIRE(vm.GetActiveVoiceCount() == 3);
  REQUIRE(vm.IsNoteActive(60));
  REQUIRE(vm.IsNoteActive(64));
  REQUIRE(vm.IsNoteActive(67));

  // 2. Voice Stealing Logic
  // We have 8 voices. We already used 3.
  // Let's use 6 more (Total 9 requests).
  // The first one (60) should be stolen if it's the oldest?
  // Actually, all voices are same age (0) if I didn't process in between?
  // In my impl, Age increments on Process().

  // Let's reset and be precise.
  vm.Reset();

  // Note 1: Age will be 100
  vm.OnNoteOn(1, 100);
  for (int i = 0; i < 100; i++)
    vm.Process();

  // Note 2: Age will be 50
  vm.OnNoteOn(2, 100);
  for (int i = 0; i < 50; i++)
    vm.Process();

  // Fill remaining 6 voices (3..8). Ages will be 0.
  for (int i = 3; i <= 8; i++) {
    vm.OnNoteOn(i, 100);
  }

  // Now all 8 voices are active.
  // Voice 1 (Note 1) has age 100 + 50 = 150.
  // Voice 2 (Note 2) has age 50.
  // Voices 3..8 have age 0.

  // Trigger 9th note. Should steal Note 1 (Oldest).
  vm.OnNoteOn(9, 100);

  REQUIRE(vm.GetActiveVoiceCount() == 8);
  REQUIRE_FALSE(vm.IsNoteActive(1));
  REQUIRE(vm.IsNoteActive(9));

  // Let's verify that we still produce sound.
  PolySynthCore::sample_t val = vm.Process();
  REQUIRE(std::abs(val) > 0.0);
}

TEST_CASE("VoiceManager Silence", "[VoiceManager]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  // No notes -> Silence
  REQUIRE(vm.Process() == Approx(0.0));

  // Note On -> Sound
  vm.OnNoteOn(60, 100);
  vm.Process();
  REQUIRE(vm.GetActiveVoiceCount() == 1);
  REQUIRE(vm.IsNoteActive(60));
  REQUIRE(std::abs(vm.Process()) > 0.0);

  // Note Off -> Release -> Eventually Silence
  vm.OnNoteOff(60);
  REQUIRE(vm.IsNoteActive(60));

  // Drain
  for (int i = 0; i < 20000; i++)
    vm.Process();

  REQUIRE(vm.GetActiveVoiceCount() == 0);
  REQUIRE_FALSE(vm.IsNoteActive(60));
  REQUIRE(vm.Process() == Approx(0.0).margin(0.001));
}
