#include "../../src/core/Engine.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("Engine Modulation: LFO sweeps Pan Position",
          "[Modulation][Panning]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);

  // Setup LFO to sweep very slowly so we can trace the panning
  // Sine wave LFO, 1Hz rate, depth 1.0
  engine.SetLFO(0, 1.0, 1.0);

  // Route LFO: Pitch 0, Filter 0, Amp 0, Pan 1.0
  engine.SetLFORouting(0.0, 0.0, 0.0, 1.0);

  // Setup instant attack so we get signal instantly for the startL/R check
  engine.SetADSR(0.001, 1.0, 1.0, 1.0);
  engine.SetFilterEnv(0.001, 1.0, 1.0, 1.0);

  // Note On
  engine.OnNoteOn(60, 100);

  // We expect the LFO (1Hz, Sine) starting phase to begin shifting values
  // smoothly. Over the course of 48000 samples (1 second), the pan should sweep
  // back and forth. Let's sample the audio output at quarter-second intervals
  // and assert the L/R balance changes.

  // Helper to measure RMS power over N samples
  auto measureRMS = [&engine](int samples, double &rmsL, double &rmsR) {
    double sumL = 0.0;
    double sumR = 0.0;
    for (int i = 0; i < samples; ++i) {
      double l, r;
      engine.Process(l, r);
      sumL += l * l;
      sumR += r * r;
    }
    rmsL = std::sqrt(sumL / samples);
    rmsR = std::sqrt(sumR / samples);
  };

  double startL = 0.0, startR = 0.0;
  measureRMS(1000, startL, startR); // Capture initial 1000 samples

  REQUIRE(startL > 0.0);
  REQUIRE(startR > 0.0);
  // Near start, LFO is likely around 0. Pan should be near center.
  REQUIRE(startL == Approx(startR).margin(0.1));

  // Fast forward to LFO maximum (+1.0 at 12000 samples). We already processed
  // 1000. We process 10500 samples silently, then measure RMS for 1000 samples
  // (spanning 11500 to 12500)
  double discardL, discardR;
  for (int i = 0; i < 10500; i++)
    engine.Process(discardL, discardR);

  double qL = 0.0, qR = 0.0;
  measureRMS(1000, qL, qR);

  REQUIRE(qR > qL * 2.0); // Right channel should be significantly louder
  REQUIRE(qL < 0.05);     // Left channel should be nearly muted

  // Fast forward to LFO minimum (-1.0 at 36000 samples). We are at 12500.
  // We process 23000 samples silently, then measure 1000 samples (spanning
  // 35500 to 36500)
  for (int i = 0; i < 23000; i++)
    engine.Process(discardL, discardR);

  double tqL = 0.0, tqR = 0.0;
  measureRMS(1000, tqL, tqR);

  REQUIRE(tqL > tqR * 2.0); // Left channel should be significantly louder
  REQUIRE(tqR < 0.05);      // Right channel should be nearly muted
}
