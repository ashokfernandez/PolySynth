#include "../src/core/Engine.h"
#include "catch.hpp"

TEST_CASE("Engine Produces Audio On Note", "[Engine]") {
  PolySynth::Engine engine;
  engine.Init(48000.0);

  // 1. Initial state: Silence
  PolySynth::sample_t *outputs[2];
  PolySynth::sample_t left[100];
  PolySynth::sample_t right[100];
  outputs[0] = left;
  outputs[1] = right;

  engine.Process(nullptr, outputs, 100, 2);
  REQUIRE(left[0] == Approx(0.0));

  // 2. Note On
  engine.OnNoteOn(69, 127); // A4

  // 3. Process
  engine.Process(nullptr, outputs, 100, 2);

  // Should have signal now.
  // Sawtooth starts at -1.0 or similar.
  // Check RMS or just non-zero
  double energy = 0.0;
  for (int i = 0; i < 100; i++) {
    energy += std::abs(left[i]);
  }
  REQUIRE(energy > 0.0);

  // 4. Note Off
  engine.OnNoteOff(69);

  // 5. Process (Instant silence for now due to lack of envelope)
  engine.Process(nullptr, outputs, 100, 2);
  REQUIRE(left[0] == Approx(0.0));
}
