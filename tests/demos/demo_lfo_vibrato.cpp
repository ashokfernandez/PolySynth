/**
 * Demo: LFO Vibrato
 * Shows LFO modulating pitch with different shapes
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering LFO Vibrato Demo..." << std::endl;

  double sampleRate = 44100.0;
  double noteDuration = 1.5;
  double gap = 0.3;
  int samplesPerNote = static_cast<int>(noteDuration * sampleRate);
  int samplesPerGap = static_cast<int>(gap * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.01, 0.1, 1.0, 0.3);

  // Shapes: 0=Sine, 1=Tri, 2=Square, 3=Saw
  for (int shape = 0; shape < 4; shape++) {
    engine.SetLFO(shape, 6.0, 0.5);      // 6Hz, half depth
    engine.SetLFORouting(0.8, 0.0, 0.0); // 80% pitch mod

    engine.OnNoteOn(69, 100); // A4 (440Hz)

    for (int i = 0; i < samplesPerNote; i++) {
      if (i == samplesPerNote - static_cast<int>(0.2 * sampleRate)) {
        engine.OnNoteOff(69);
      }
      double left, right;
      engine.Process(left, right);
      output.push_back(static_cast<float>(left));
    }

    for (int i = 0; i < samplesPerGap; i++) {
      output.push_back(0.0f);
    }
  }

  PolySynth::WavWriter::Write("demo_lfo_vibrato.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_lfo_vibrato.wav" << std::endl;
  return 0;
}
