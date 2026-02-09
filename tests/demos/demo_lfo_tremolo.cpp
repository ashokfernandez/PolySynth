/**
 * Demo: LFO Tremolo
 * Shows LFO modulating amplitude at increasing rates
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering LFO Tremolo Demo..." << std::endl;

  double sampleRate = 44100.0;
  double duration = 5.0;
  int totalSamples = static_cast<int>(duration * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.5, 0.2, 1.0, 0.5);

  engine.SetLFO(0, 2.0, 0.6);          // Start at 2Hz
  engine.SetLFORouting(0.0, 0.0, 0.7); // 70% amp mod

  engine.OnNoteOn(60, 100);

  for (int i = 0; i < totalSamples; i++) {
    // Accelerate LFO from 2Hz to 15Hz
    double rate = 2.0 + 13.0 * (static_cast<double>(i) / totalSamples);
    engine.SetLFO(0, rate, 0.6);

    if (i == totalSamples - static_cast<int>(0.5 * sampleRate)) {
      engine.OnNoteOff(60);
    }

    double left, right;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write("demo_lfo_tremolo.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_lfo_tremolo.wav" << std::endl;
  return 0;
}
