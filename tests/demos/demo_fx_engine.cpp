#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

namespace {

struct NoteEvent {
  int note;
  int velocity;
  int startSample;
  int endSample;
};

void RenderDemo(const std::string &filename,
                const std::function<void(PolySynthCore::Engine &)> &configureFx,
                double sampleRate) {
  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.01, 0.15, 0.8, 0.2);
  engine.SetFilter(9000.0, 0.4, 0.0);
  configureFx(engine);

  const int totalSamples = static_cast<int>(sampleRate * 4.0);
  std::vector<float> output;
  output.reserve(totalSamples);

  std::vector<NoteEvent> events = {
      {60, 110, 0, static_cast<int>(sampleRate * 0.5)},
      {64, 110, static_cast<int>(sampleRate * 1.0),
       static_cast<int>(sampleRate * 1.5)},
      {67, 110, static_cast<int>(sampleRate * 2.0),
       static_cast<int>(sampleRate * 2.5)}};

  for (int i = 0; i < totalSamples; ++i) {
    for (const auto &event : events) {
      if (i == event.startSample) {
        engine.OnNoteOn(event.note, event.velocity);
      }
      if (i == event.endSample) {
        engine.OnNoteOff(event.note);
      }
    }

    PolySynthCore::sample_t left = 0.0;
    PolySynthCore::sample_t right = 0.0;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write(filename, output, static_cast<int>(sampleRate));
}

} // namespace

int main() {
  std::cout << "Rendering FX demos..." << std::endl;
  const double sampleRate = 48000.0;

  RenderDemo(
      "demo_fx_chorus.wav",
      [](PolySynthCore::Engine &engine) {
        engine.SetChorus(0.35, 0.7, 0.45);
        engine.SetDelay(0.35, 0.25, 0.0);
        engine.SetLimiter(0.9, 5.0, 50.0);
      },
      sampleRate);

  RenderDemo(
      "demo_fx_delay.wav",
      [](PolySynthCore::Engine &engine) {
        engine.SetChorus(0.25, 0.2, 0.15);
        engine.SetDelay(0.45, 0.5, 0.45);
        engine.SetLimiter(0.9, 5.0, 50.0);
      },
      sampleRate);

  RenderDemo(
      "demo_fx_limiter.wav",
      [](PolySynthCore::Engine &engine) {
        engine.SetChorus(0.25, 0.3, 0.0);
        engine.SetDelay(0.25, 0.0, 0.0);
        engine.SetLimiter(0.25, 5.0, 30.0);
      },
      sampleRate);

  return 0;
}
