/**
 * Demo: Oscillator Mixing
 * Blends Sawtooth and Square oscillators with detune for a thick sound
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Oscillator Mixing Demo..." << std::endl;

  double sampleRate = 44100.0;
  double duration = 4.0;
  int totalSamples = static_cast<int>(duration * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);

  // Osc A: Saw, Osc B: Square
  engine.SetWaveformA(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetWaveformB(PolySynthCore::Oscillator::WaveformType::Square);

  // Set 50/50 mix and 10 cents detune on Osc B
  engine.SetMixer(0.5, 0.5, 10.0);

  engine.SetADSR(0.1, 0.2, 0.8, 0.5);
  engine.OnNoteOn(33, 100); // Low A

  for (int i = 0; i < totalSamples; i++) {
    // Crossfade from Osc A only to Osc B only
    double mixB = static_cast<double>(i) / totalSamples;
    double mixA = 1.0 - mixB;
    engine.SetMixer(mixA, mixB, 10.0);

    if (i == totalSamples - static_cast<int>(0.5 * sampleRate)) {
      engine.OnNoteOff(33);
    }

    double left, right;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write("demo_osc_mix.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_osc_mix.wav" << std::endl;
  return 0;
}
