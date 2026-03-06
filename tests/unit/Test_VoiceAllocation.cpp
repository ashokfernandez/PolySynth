#include "../../src/core/Engine.h"
#include "../../src/core/VoiceManager.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("Polyphony limit of 4 respected", "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);
  vm.SetPolyphonyLimit(4);

  for (int i = 0; i < 5; i++) {
    vm.OnNoteOn(60 + i, 100);
  }
  // 5th note should steal — total active still 4
  REQUIRE(vm.GetActiveVoiceCount() == 4);
}

TEST_CASE("Polyphony limit reduction kills excess", "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  // Start 8 voices
  for (int i = 0; i < 8; i++) {
    vm.OnNoteOn(60 + i, 100);
  }
  REQUIRE(vm.GetActiveVoiceCount() == 8);

  // Reduce limit to 4 — excess voices should be killed
  vm.SetPolyphonyLimit(4);
  // Steal fade is 20ms at 48kHz = 960 samples; process 1200 to be safe.
  for (int i = 0; i < 1200; i++)
    vm.Process();

  REQUIRE(vm.GetActiveVoiceCount() <= 4);
}

TEST_CASE("Mono mode: polyphony limit 1", "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);
  vm.SetPolyphonyLimit(1);

  vm.OnNoteOn(60, 100);
  REQUIRE(vm.GetActiveVoiceCount() == 1);

  vm.OnNoteOn(64, 100);
  // Should steal the first voice
  REQUIRE(vm.GetActiveVoiceCount() == 1);
  REQUIRE(vm.IsNoteActive(64));
}

TEST_CASE("Unison count=3 allocates 3 voices per note", "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);
  vm.SetUnisonCount(3);

  vm.OnNoteOn(60, 100);
  REQUIRE(vm.GetActiveVoiceCount() == 3);

  // All 3 voices should be playing note 60
  int noteCount = 0;
  auto states = vm.GetVoiceStates();
  for (const auto &s : states) {
    if (s.note == 60)
      noteCount++;
  }
  REQUIRE(noteCount == 3);
}

TEST_CASE("NoteOff releases all unison voices for note", "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);
  vm.SetUnisonCount(3);

  vm.OnNoteOn(60, 100);
  REQUIRE(vm.GetActiveVoiceCount() == 3);

  vm.OnNoteOff(60);
  // All 3 voices should be in Release state
  auto states = vm.GetVoiceStates();
  int releasingCount = 0;
  for (const auto &s : states) {
    if (s.state == PolySynthCore::VoiceState::Release)
      releasingCount++;
  }
  REQUIRE(releasingCount == 3);
}

TEST_CASE("Sustain pedal holds and releases through VoiceManager",
          "[VoiceAllocation]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  vm.OnSustainPedal(true);
  vm.OnNoteOn(60, 100);
  vm.OnNoteOff(60); // Should be held by sustain

  // Voice should still be active (not released)
  REQUIRE(vm.IsNoteActive(60));

  vm.OnSustainPedal(false); // Release sustained notes

  // Voice should now be in Release
  auto states = vm.GetVoiceStates();
  bool foundRelease = false;
  for (const auto &s : states) {
    if (s.note == 60 && s.state == PolySynthCore::VoiceState::Release) {
      foundRelease = true;
    }
  }
  REQUIRE(foundRelease);
}

// --- Stress Tests (Sprint 4) ---

TEST_CASE("32 rapid NoteOn into 8-voice polyphony", "[VoiceAllocation][Stress]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);
  PolySynthCore::SynthState state;
  state.polyphony = 8;
  engine.UpdateState(state);

  for (int note = 40; note < 72; ++note) {
    engine.OnNoteOn(note, 100);
  }
  REQUIRE(engine.GetActiveVoiceCount() <= 8);

  // Process and verify no crash
  for (int i = 0; i < 1024; ++i) {
    PolySynthCore::sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
  }
}

TEST_CASE("Unison overflow: 3 notes x 4 unison into 8 voices", "[VoiceAllocation][Stress]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);
  PolySynthCore::SynthState state;
  state.polyphony = 8;
  state.unisonCount = 4;
  engine.UpdateState(state);

  engine.OnNoteOn(60, 100);
  engine.OnNoteOn(64, 100);
  engine.OnNoteOn(67, 100);

  for (int i = 0; i < 1024; ++i) {
    PolySynthCore::sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
  }
}

TEST_CASE("Sustain pedal + rapid notes + release stress", "[VoiceAllocation][Stress]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);
  PolySynthCore::SynthState state;
  state.polyphony = 8;
  engine.UpdateState(state);

  engine.OnSustainPedal(true);
  for (int note = 60; note < 68; ++note) {
    engine.OnNoteOn(note, 100);
  }
  for (int note = 60; note < 68; ++note) {
    engine.OnNoteOff(note);
  }
  // 4 more notes while sustained
  for (int note = 70; note < 74; ++note) {
    engine.OnNoteOn(note, 100);
  }
  engine.OnSustainPedal(false);

  // Process enough for all voices to release
  for (int i = 0; i < 48000; ++i) {
    PolySynthCore::sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
  }
  REQUIRE(engine.GetActiveVoiceCount() == 0);
}

TEST_CASE("NoteOn/NoteOff same note in quick succession", "[VoiceAllocation][Stress]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);
  PolySynthCore::SynthState state;
  state.ampAttack = 0.01;
  state.ampDecay = 0.1;
  state.ampSustain = 1.0;
  state.ampRelease = 1.0;
  state.filterAttack = 0.01;
  state.filterDecay = 0.1;
  state.filterSustain = 1.0;
  state.filterRelease = 1.0;
  engine.UpdateState(state);

  engine.OnNoteOn(60, 100);
  for (int i = 0; i < 10; ++i) {
    PolySynthCore::sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
  }
  engine.OnNoteOff(60);
  engine.OnNoteOn(60, 100);

  // Verify no crash from rapid retrigger
  for (int i = 0; i < 2048; ++i) {
    PolySynthCore::sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
  }
}
