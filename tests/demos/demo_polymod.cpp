/**
 * Demo: Poly-Mod Matrix (Audio-Rate)
 * Shows Osc B modulating Osc A frequency, filter cutoff, and PWM.
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Poly-Mod Matrix Demo..." << std::endl;

  const double sampleRate = 44100.0;
  const double duration = 6.0;
  const int totalSamples = static_cast<int>(duration * sampleRate);
  const int sectionSamples = totalSamples / 3;

  std::vector<float> output;
  output.reserve(totalSamples);

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveformA(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetWaveformB(PolySynthCore::Oscillator::WaveformType::Sine);
  engine.SetMixer(1.0, 0.0, 0.0);
  engine.SetADSR(0.01, 0.1, 0.9, 0.2);
  engine.SetFilterEnv(0.01, 0.2, 0.0, 0.2);

  engine.OnNoteOn(52, 110); // E3

  for (int i = 0; i < totalSamples; ++i) {
    if (i == sectionSamples) {
      engine.OnNoteOff(52);
      engine.SetPolyModOscBToFreqA(0.0);
      engine.SetPolyModOscBToFilter(0.6);
      engine.SetFilter(1200.0, 0.7, 0.0);
      engine.OnNoteOn(52, 110);
    } else if (i == sectionSamples * 2) {
      engine.OnNoteOff(52);
      engine.SetPolyModOscBToFilter(0.0);
      engine.SetPolyModFilterEnvToPWM(0.8);
      engine.SetFilter(20000.0, 0.0, 0.0);
      engine.OnNoteOn(52, 110);
    }

    if (i < sectionSamples) {
      engine.SetPolyModOscBToFreqA(0.4);
      engine.SetFilter(20000.0, 0.0, 0.0);
    }

    double left = 0.0;
    double right = 0.0;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write("demo_polymod.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_polymod.wav" << std::endl;
  return 0;
}
