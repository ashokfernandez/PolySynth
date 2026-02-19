#include "VoiceManager.h"
#include <catch.hpp>
#include <cmath>
#include <fstream>
#include <sstream>

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

TEST_CASE("Voice legato retrigger does not compound unison detune",
          "[VoiceManager][Portamento][Unison]") {
  Voice v;
  v.Init(48000.0);
  v.SetGlideTime(0.1); // Ensure legato path is active while note is held

  const double detuneCents = 100.0;
  const double detuneFactor = std::pow(2.0, detuneCents / 1200.0);
  const double expected = 440.0 * std::pow(2.0, (60.0 - 69.0) / 12.0) *
                          detuneFactor;

  v.SetUnisonDetuneCents(detuneCents);
  v.NoteOn(60, 100);

  // Re-trigger legato several times with same detune. Prior behavior multiplied
  // current frequency repeatedly on each retrigger.
  for (int i = 0; i < 5; ++i) {
    for (int s = 0; s < 256; ++s)
      v.Process();
    v.SetUnisonDetuneCents(detuneCents);
    v.NoteOn(60, 100);
  }

  for (int s = 0; s < 1024; ++s)
    v.Process();

  CHECK(v.GetCurrentPitch() == Approx(expected).margin(0.5f));
}

TEST_CASE("Desktop ProcessBlock contains no printf logging",
          "[Desktop][Realtime]") {
  const char *candidates[] = {
      "src/platform/desktop/PolySynth.cpp",
      "../src/platform/desktop/PolySynth.cpp",
      "../../src/platform/desktop/PolySynth.cpp",
  };

  std::string source;
  for (const char *path : candidates) {
    std::ifstream in(path);
    if (!in.good())
      continue;
    std::ostringstream ss;
    ss << in.rdbuf();
    source = ss.str();
    break;
  }

  REQUIRE_FALSE(source.empty());

  const std::string beginSig = "void PolySynthPlugin::ProcessBlock";
  const std::string endSig = "void PolySynthPlugin::OnIdle";
  const auto begin = source.find(beginSig);
  const auto end = source.find(endSig);

  REQUIRE(begin != std::string::npos);
  REQUIRE(end != std::string::npos);
  REQUIRE(end > begin);

  const std::string processBlock = source.substr(begin, end - begin);
  CHECK(processBlock.find("printf(") == std::string::npos);
}
