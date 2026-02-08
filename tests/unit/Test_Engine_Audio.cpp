#include "../../src/core/Engine.h"
#include "catch.hpp"

TEST_CASE("Engine Produces Audio On Note", "[Engine]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);

  // 1. Initial state: Silence
  PolySynthCore::sample_t *outputs[2];
  PolySynthCore::sample_t left[100];
  PolySynthCore::sample_t right[100];
  outputs[0] = left;
  outputs[1] = right;

  engine.Process(nullptr, outputs, 100, 2);
  REQUIRE(left[0] == Approx(0.0));

  // 2. Note On
  engine.OnNoteOn(69, 127); // A4

  // 3. Process
  engine.Process(nullptr, outputs, 100, 2);

  // Should have signal now.
  double energy = 0.0;
  for (int i = 0; i < 100; i++) {
    energy += std::abs(left[i]);
  }
  REQUIRE(energy > 0.0);

  // 4. Note Off
  engine.OnNoteOff(69);

  // 5. Process (Release phase)
  // Should NOT be silence immediately due to Release envelope
  engine.Process(nullptr, outputs, 100, 2);
  REQUIRE(std::abs(left[0]) > 0.001);

  // 6. Drain Release
  // Release is 0.2s * 48000 = 9600 samples.
  // We process 15000 samples to be sure.
  PolySynthCore::sample_t tempBuffer[100];
  PolySynthCore::sample_t *tempPtrs[2] = {tempBuffer, tempBuffer};

  for (int i = 0; i < 150; i++) {
    engine.Process(nullptr, tempPtrs, 100, 2);
  }

  // 7. Verify Silence
  engine.Process(nullptr, outputs, 100, 2);
  REQUIRE(left[0] == Approx(0.0).margin(0.001));
}
