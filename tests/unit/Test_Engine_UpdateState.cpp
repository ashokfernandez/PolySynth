#include "../../src/core/Engine.h"
#include "../../src/core/SynthState.h"
#include "catch.hpp"

#include <cmath>
#include <memory>

using namespace PolySynthCore;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;

/// Helper: initialise an Engine with default sample rate.
/// Returns a unique_ptr because Engine contains std::atomic (non-copyable).
std::unique_ptr<Engine> MakeEngine() {
  auto engine = std::make_unique<Engine>();
  engine->Init(kSampleRate);
  return engine;
}

/// Helper: process N samples of stereo audio, return sum of squared magnitudes
/// (energy) across both channels.
double ProcessAndMeasureRMS(Engine &engine, int nSamples) {
  double energy = 0.0;
  for (int i = 0; i < nSamples; ++i) {
    sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    energy += static_cast<double>(left * left + right * right);
  }
  return std::sqrt(energy / nSamples);
}

/// Helper: check that all samples in a block are finite (no NaN / Inf).
bool ProcessAndCheckFinite(Engine &engine, int nSamples) {
  for (int i = 0; i < nSamples; ++i) {
    sample_t left = 0.0, right = 0.0;
    engine.Process(left, right);
    if (!std::isfinite(static_cast<double>(left)) ||
        !std::isfinite(static_cast<double>(right))) {
      return false;
    }
  }
  return true;
}

} // namespace

TEST_CASE("Engine: gain=0 produces silence", "[Engine][UpdateState]") {
  auto engine = MakeEngine();

  SynthState state{};
  state.masterGain = 0.0;
  engine->UpdateState(state);

  // Trigger a note so the voice manager is actively producing signal
  engine->OnNoteOn(60, 127);

  double rms = ProcessAndMeasureRMS(*engine, kBlockSize);
  REQUIRE(rms < 1e-6);
}

TEST_CASE("Engine: waveform change produces different output",
          "[Engine][UpdateState]") {
  // Saw waveform
  auto engineSaw = MakeEngine();
  SynthState stateSaw{};
  stateSaw.oscAWaveform = 0; // Saw
  stateSaw.mixOscA = 1.0;
  stateSaw.mixOscB = 0.0;
  stateSaw.ampAttack = 0.001;
  stateSaw.ampSustain = 1.0;
  stateSaw.filterCutoff = 20000.0;
  engineSaw->UpdateState(stateSaw);
  engineSaw->OnNoteOn(60, 127);

  // Square waveform
  auto engineSquare = MakeEngine();
  SynthState stateSquare{};
  stateSquare.oscAWaveform = 1; // Square
  stateSquare.mixOscA = 1.0;
  stateSquare.mixOscB = 0.0;
  stateSquare.ampAttack = 0.001;
  stateSquare.ampSustain = 1.0;
  stateSquare.filterCutoff = 20000.0;
  engineSquare->UpdateState(stateSquare);
  engineSquare->OnNoteOn(60, 127);

  // Collect samples from both
  std::vector<double> sawSamples, squareSamples;
  for (int i = 0; i < kBlockSize; ++i) {
    sample_t l1 = 0.0, r1 = 0.0, l2 = 0.0, r2 = 0.0;
    engineSaw->Process(l1, r1);
    engineSquare->Process(l2, r2);
    sawSamples.push_back(static_cast<double>(l1));
    squareSamples.push_back(static_cast<double>(l2));
  }

  // The two waveforms should differ in at least some samples
  int diffCount = 0;
  for (int i = 0; i < kBlockSize; ++i) {
    if (std::abs(sawSamples[i] - squareSamples[i]) > 1e-6)
      ++diffCount;
  }
  REQUIRE(diffCount > kBlockSize / 4);
}

TEST_CASE("Engine: zero attack envelope produces instant onset",
          "[Engine][UpdateState]") {
  auto engine = MakeEngine();

  SynthState state{};
  state.ampAttack = 0.001; // Near-zero attack (< 1ms)
  state.ampDecay = 0.1;
  state.ampSustain = 1.0;
  state.ampRelease = 0.1;
  state.mixOscA = 1.0;
  state.filterCutoff = 20000.0;
  engine->UpdateState(state);

  engine->OnNoteOn(60, 127);

  // The limiter has ~5ms lookahead (~220 samples at 44.1kHz), so skip past that.
  // With near-zero attack, signal should be at full level immediately after
  // the lookahead buffer fills.
  ProcessAndMeasureRMS(*engine, 256); // flush limiter lookahead

  double rms = ProcessAndMeasureRMS(*engine, 256);
  REQUIRE(rms > 0.01);
}

TEST_CASE("Engine: zero release envelope stops signal after note off",
          "[Engine][UpdateState]") {
  auto engine = MakeEngine();

  SynthState state{};
  state.ampAttack = 0.001;
  state.ampDecay = 0.001;
  state.ampSustain = 1.0;
  state.ampRelease = 0.0;
  state.mixOscA = 1.0;
  state.filterCutoff = 20000.0;
  engine->UpdateState(state);

  engine->OnNoteOn(60, 127);

  // Let the note sustain for a bit
  ProcessAndMeasureRMS(*engine, 1000);

  // Release the note
  engine->OnNoteOff(60);

  // After sufficient samples with zero release, output should be near silence
  // Give it a small buffer for any fade-out (limiter lookahead, etc.)
  ProcessAndMeasureRMS(*engine, 256);

  double rms = ProcessAndMeasureRMS(*engine, kBlockSize);
  REQUIRE(rms < 1e-4);
}

TEST_CASE("Engine: filter cutoff extremes affect output level",
          "[Engine][UpdateState]") {
  // High cutoff (20kHz) — lets everything through
  auto engineHigh = MakeEngine();
  SynthState stateHigh{};
  stateHigh.filterCutoff = 20000.0;
  stateHigh.filterResonance = 0.0;
  stateHigh.ampAttack = 0.001;
  stateHigh.ampSustain = 1.0;
  stateHigh.mixOscA = 1.0;
  stateHigh.oscAWaveform = 0; // Saw (harmonically rich)
  engineHigh->UpdateState(stateHigh);
  engineHigh->OnNoteOn(60, 127);

  // Low cutoff (20Hz) — filters out almost everything
  auto engineLow = MakeEngine();
  SynthState stateLow{};
  stateLow.filterCutoff = 20.0;
  stateLow.filterResonance = 0.0;
  stateLow.ampAttack = 0.001;
  stateLow.ampSustain = 1.0;
  stateLow.mixOscA = 1.0;
  stateLow.oscAWaveform = 0; // Saw
  engineLow->UpdateState(stateLow);
  engineLow->OnNoteOn(60, 127);

  // Let both settle past the attack phase
  ProcessAndMeasureRMS(*engineHigh, 500);
  ProcessAndMeasureRMS(*engineLow, 500);

  double rmsHigh = ProcessAndMeasureRMS(*engineHigh, kBlockSize);
  double rmsLow = ProcessAndMeasureRMS(*engineLow, kBlockSize);

  REQUIRE(rmsHigh > rmsLow);
}

TEST_CASE("Engine: filter resonance=1.0 produces no NaN or blow-up",
          "[Engine][UpdateState]") {
  auto engine = MakeEngine();

  SynthState state{};
  state.filterCutoff = 1000.0;
  state.filterResonance = 1.0;
  state.ampAttack = 0.001;
  state.ampSustain = 1.0;
  state.mixOscA = 1.0;
  state.oscAWaveform = 0; // Saw
  engine->UpdateState(state);

  engine->OnNoteOn(60, 127);

  // Process a significant number of samples and verify all are finite
  bool allFinite = ProcessAndCheckFinite(*engine, kBlockSize * 4);
  REQUIRE(allFinite);
}
