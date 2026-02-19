#include "../../src/core/VoiceManager.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("Stereo: center pan gives equal L/R", "[StereoPanning]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  vm.OnNoteOn(60, 100);

  double left = 0.0, right = 0.0;
  // Let the voice output generate some signal
  for (int i = 0; i < 100; i++) {
    vm.ProcessStereo(left, right);
  }

  // Both channels should have equal magnitude
  REQUIRE(std::abs(left) == Approx(std::abs(right)).margin(0.001));
  REQUIRE(std::abs(left) > 0.0); // Make sure it's not silent
}

TEST_CASE("Stereo: Hard Pan Left isolates signal to left channel",
          "[StereoPanning]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  // StereoSpread toggles -spread/+spread for alternating voices. Voice 0 will
  // be -spread (-1.0).
  vm.SetStereoSpread(1.0);

  vm.OnNoteOn(60, 100);

  double left = 0.0, right = 0.0;
  double leftAccum = 0.0, rightAccum = 0.0;

  for (int i = 0; i < 100; i++) {
    vm.ProcessStereo(left, right);
    leftAccum += std::abs(left);
    rightAccum += std::abs(right);
  }

  // hard pan left formula: theta = (-1.0 + 1.0) * pi * 0.25 = 0
  // cos(0) = 1 (left), sin(0) = 0 (right)
  REQUIRE(leftAccum > 0.0);
  REQUIRE(rightAccum == Approx(0.0).margin(0.0001));
}

TEST_CASE("Stereo: Hard Pan Right isolates signal to right channel",
          "[StereoPanning]") {
  PolySynthCore::VoiceManager vm;
  vm.Init(48000.0);

  // Note: SetStereoSpread toggles -spread/+spread for alternating voices. Voice
  // 0 will be -spread (-1.0), Voice 1 will be +spread (1.0). Thus allocating
  // two voices to checking Voice 1
  vm.SetStereoSpread(1.0);
  vm.OnNoteOn(60, 100);
  vm.OnNoteOn(64, 100); // Voice 1 will be panned right (+spread)

  // Turn off Voice 0 so it doesn't bleed left
  vm.OnNoteOff(60);
  // Process out the stolen/release fade for Voice 0
  double tempL, tempR;
  for (int i = 0; i < 5000; i++) {
    vm.ProcessStereo(tempL, tempR);
  }

  double left = 0.0, right = 0.0;
  double leftAccum = 0.0, rightAccum = 0.0;

  for (int i = 0; i < 100; i++) {
    vm.ProcessStereo(left, right);
    leftAccum += std::abs(left);
    rightAccum += std::abs(right);
  }

  // hard pan right formula: theta = (1.0 + 1.0) * pi * 0.25 = pi/2
  // cos(pi/2) = 0 (left), sin(pi/2) = 1 (right)
  REQUIRE(rightAccum > 0.0);
  REQUIRE(leftAccum == Approx(0.0).margin(0.0001));
}
