#include "../../src/core/VoiceManager.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("Portamento: glideTime=0 is instant pitch jump", "[Portamento]") {
  PolySynthCore::Voice voice;
  voice.Init(48000.0, 0);
  voice.SetGlideTime(0.0);

  voice.NoteOn(60, 100, 1); // C4 = 261.63 Hz
  voice.Process();

  double expectedFreq = 440.0 * std::pow(2.0, (60 - 69.0) / 12.0);
  REQUIRE(voice.GetCurrentPitch() ==
          Approx(static_cast<float>(expectedFreq)).margin(0.1));

  // Retrigger to different note — should jump instantly
  voice.NoteOn(72, 100, 2); // C5 = 523.25 Hz
  voice.Process();

  double newFreq = 440.0 * std::pow(2.0, (72 - 69.0) / 12.0);
  REQUIRE(voice.GetCurrentPitch() ==
          Approx(static_cast<float>(newFreq)).margin(0.1));
}

TEST_CASE("Portamento: glideTime>0 interpolates pitch", "[Portamento]") {
  PolySynthCore::Voice voice;
  voice.Init(48000.0, 0);
  voice.SetGlideTime(0.1); // 100ms glide

  voice.NoteOn(60, 100, 1); // C4
  // Process a few samples to establish pitch
  for (int i = 0; i < 10; i++)
    voice.Process();

  double startFreq = 440.0 * std::pow(2.0, (60 - 69.0) / 12.0);
  double endFreq = 440.0 * std::pow(2.0, (72 - 69.0) / 12.0);

  // Retrigger to C5 — should glide, not jump
  voice.NoteOn(72, 100, 2);
  voice.Process();

  // After 1 sample, pitch should be between start and end
  float currentPitch = voice.GetCurrentPitch();
  // Use Approx with margin for float comparisons
  REQUIRE(currentPitch > static_cast<float>(startFreq));
  // Should not have reached target yet
  REQUIRE(currentPitch < static_cast<float>(endFreq));
}

TEST_CASE("Portamento: reaches within 1% of target within 3x glideTime",
          "[Portamento]") {
  PolySynthCore::Voice voice;
  voice.Init(48000.0, 0);
  double glideTime = 0.05; // 50ms
  voice.SetGlideTime(glideTime);

  voice.NoteOn(60, 100, 1);
  for (int i = 0; i < 100; i++)
    voice.Process();

  voice.NoteOn(72, 100, 2);
  double targetFreq = 440.0 * std::pow(2.0, (72 - 69.0) / 12.0);

  // Process for 3x glide time
  int samples = static_cast<int>(3.0 * glideTime * 48000.0);
  for (int i = 0; i < samples; i++)
    voice.Process();

  float currentPitch = voice.GetCurrentPitch();
  double error = std::abs(currentPitch - targetFreq) / targetFreq;
  REQUIRE(error < 0.01); // Within 1%
}

TEST_CASE("Portamento: no overshoot past target", "[Portamento]") {
  PolySynthCore::Voice voice;
  voice.Init(48000.0, 0);
  voice.SetGlideTime(0.05);

  voice.NoteOn(60, 100, 1);
  for (int i = 0; i < 100; i++)
    voice.Process();

  voice.NoteOn(72, 100, 2); // Glide up
  double targetFreq = 440.0 * std::pow(2.0, (72 - 69.0) / 12.0);

  // Process many samples — pitch should never exceed target
  for (int i = 0; i < 48000; i++) {
    voice.Process();
    float pitch = voice.GetCurrentPitch();
    REQUIRE(pitch <= static_cast<float>(targetFreq) + 0.01f);
  }
}
