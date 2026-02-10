/**
 * Demo: Filter Resonance
 * Shows filter at different resonance levels
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Filter Resonance Demo..." << std::endl;

  double sampleRate = 44100.0;
  double noteDuration = 1.5;
  double gap = 0.3;
  int samplesPerNote = static_cast<int>(noteDuration * sampleRate);
  int samplesPerGap = static_cast<int>(gap * sampleRate);

  std::vector<float> output;
  double resonances[] = {0.707, 2.0, 5.0, 10.0};

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(sea::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.01, 0.1, 0.8, 0.3);

  for (int r = 0; r < 4; r++) {
    engine.SetFilter(800.0, resonances[r], 0.0);
    engine.OnNoteOn(33, 100); // Very low A

    for (int i = 0; i < samplesPerNote; i++) {
      if (i == samplesPerNote - static_cast<int>(0.4 * sampleRate)) {
        engine.OnNoteOff(33);
      }
      double left, right;
      engine.Process(left, right);
      output.push_back(static_cast<float>(left));
    }

    for (int i = 0; i < samplesPerGap; i++) {
      output.push_back(0.0f);
    }
  }

  PolySynth::WavWriter::Write("demo_filter_resonance.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_filter_resonance.wav" << std::endl;
  return 0;
}
