/**
 * Demo: Factory Presets
 * Showcases the 3 curated factory presets in musical context
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

void renderPhrase(PolySynthCore::Engine &engine, std::vector<float> &output,
                  double sampleRate) {
  int notes[] = {60, 64, 67, 72, 67, 64}; // C major arpeggio
  int noteSamples = static_cast<int>(0.5 * sampleRate);

  for (int note : notes) {
    engine.OnNoteOn(note, 100);
    for (int i = 0; i < noteSamples; i++) {
      if (i == noteSamples - static_cast<int>(0.1 * sampleRate)) {
        engine.OnNoteOff(note);
      }
      double left, right;
      engine.Process(left, right);
      output.push_back(static_cast<float>(left));
    }
  }

  // Gap after phrase
  for (int i = 0; i < static_cast<int>(1.0 * sampleRate); i++) {
    double left, right;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }
}

int main() {
  std::cout << "Rendering Factory Presets Demo..." << std::endl;

  double sampleRate = 44100.0;
  std::vector<float> output;
  PolySynthCore::Engine engine;
  engine.Init(sampleRate);

  // Preset 1: Warm Pad
  std::cout << " - Preset 1: Warm Pad" << std::endl;
  engine.Reset();
  engine.SetWaveform(sea::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.5, 0.3, 0.7, 1.0);
  engine.SetFilter(800.0, 0.1, 0.0);
  engine.SetLFO(0, 0.5, 0.3);
  engine.SetLFORouting(0.0, 0.2, 0.0);
  renderPhrase(engine, output, sampleRate);

  // Preset 2: Bright Lead
  std::cout << " - Preset 2: Bright Lead" << std::endl;
  engine.Reset();
  engine.SetWaveform(sea::Oscillator::WaveformType::Square);
  engine.SetADSR(0.005, 0.1, 0.6, 0.2);
  engine.SetFilter(18000.0, 0.7, 0.5); // Resonant but open
  engine.SetLFO(2, 6.0, 0.0);          // Square LFO for trills?
  renderPhrase(engine, output, sampleRate);

  // Preset 3: Dark Bass
  std::cout << " - Preset 3: Dark Bass" << std::endl;
  engine.Reset();
  engine.SetWaveform(sea::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.02, 0.5, 0.4, 0.15);
  engine.SetFilter(300.0, 0.5, 0.6);
  engine.SetLFO(1, 2.0, 0.5);
  engine.SetLFORouting(0.0, 0.4, 0.0);
  renderPhrase(engine, output, sampleRate);

  PolySynth::WavWriter::Write("demo_factory_presets.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_factory_presets.wav" << std::endl;
  return 0;
}
