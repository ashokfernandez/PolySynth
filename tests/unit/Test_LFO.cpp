#include "../../src/core/modulation/LFO.h"
#include "catch.hpp"

TEST_CASE("LFO Output Range", "[LFO]") {
  PolySynthCore::LFO lfo;
  double sr = 1000.0;
  lfo.Init(sr);
  lfo.SetRate(5.0);
  lfo.SetDepth(0.6);
  lfo.SetWaveform(0);

  for (int i = 0; i < 1000; ++i) {
    double val = lfo.Process();
    REQUIRE(val <= Approx(0.6).margin(1e-6));
    REQUIRE(val >= Approx(-0.6).margin(1e-6));
  }
}

TEST_CASE("LFO Depth Zero", "[LFO]") {
  PolySynthCore::LFO lfo;
  lfo.Init(48000.0);
  lfo.SetRate(2.0);
  lfo.SetDepth(0.0);

  for (int i = 0; i < 32; ++i) {
    REQUIRE(lfo.Process() == Approx(0.0));
  }
}

TEST_CASE("LFO Rate Zero Crossing", "[LFO]") {
  PolySynthCore::LFO lfo;
  double sr = 100.0;
  lfo.Init(sr);
  lfo.SetDepth(1.0);
  lfo.SetWaveform(0);

  lfo.SetRate(2.0);
  int crossings = 0;
  double prev = lfo.Process();
  for (int i = 1; i < 200; ++i) {
    double curr = lfo.Process();
    if (prev <= 0.0 && curr > 0.0) {
      ++crossings;
    }
    prev = curr;
  }
  REQUIRE(crossings == Approx(4).margin(1));

  lfo.Init(sr);
  lfo.SetDepth(1.0);
  lfo.SetWaveform(0);
  lfo.SetRate(5.0);
  crossings = 0;
  prev = lfo.Process();
  for (int i = 1; i < 200; ++i) {
    double curr = lfo.Process();
    if (prev <= 0.0 && curr > 0.0) {
      ++crossings;
    }
    prev = curr;
  }
  REQUIRE(crossings == Approx(10).margin(1));
}

TEST_CASE("LFO Waveforms", "[LFO]") {
  PolySynthCore::LFO lfo;
  lfo.Init(4.0);
  lfo.SetRate(1.0);
  lfo.SetDepth(1.0);

  lfo.SetWaveform(0); // Sine
  REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(-1.0).margin(1e-6));

  lfo.Init(4.0);
  lfo.SetRate(1.0);
  lfo.SetDepth(1.0);
  lfo.SetWaveform(1); // Triangle
  REQUIRE(lfo.Process() == Approx(1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(-1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));

  lfo.Init(4.0);
  lfo.SetRate(1.0);
  lfo.SetDepth(1.0);
  lfo.SetWaveform(2); // Square
  REQUIRE(lfo.Process() == Approx(1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(-1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(-1.0).margin(1e-6));

  lfo.Init(4.0);
  lfo.SetRate(1.0);
  lfo.SetDepth(1.0);
  lfo.SetWaveform(3); // Saw
  REQUIRE(lfo.Process() == Approx(-1.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(-0.5).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(0.0).margin(1e-6));
  REQUIRE(lfo.Process() == Approx(0.5).margin(1e-6));
}
