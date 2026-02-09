/**
 * Demo: Filter Envelope
 * Classic pluck/stab sounds using filter envelope
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Filter Envelope Demo..." << std::endl;

  double sampleRate = 44100.0;
  double noteDuration = 0.8;
  double gap = 0.2;
  int samplesPerNote = static_cast<int>(noteDuration * sampleRate);
  int samplesPerGap = static_cast<int>(gap * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.005, 0.3, 0.6, 0.2);

  // Different filter envelope settings: attack, decay, sustain, release
  double envs[4][4] = {
      {0.001, 0.1, 0.0, 0.1}, // Pluck
      {0.001, 0.3, 0.3, 0.2}, // Stab
      {0.2, 0.1, 0.8, 0.3},   // Swell
      {0.001, 0.5, 0.0, 0.1}, // Long Pluck
  };

  for (int s = 0; s < 4; s++) {
    engine.SetFilterEnv(envs[s][0], envs[s][1], envs[s][2], envs[s][3]);
    engine.SetFilter(200.0, 3.0, 0.8); // 80% env amount

    engine.OnNoteOn(45, 100);

    for (int i = 0; i < samplesPerNote; i++) {
      if (i == samplesPerNote - static_cast<int>(0.3 * sampleRate)) {
        engine.OnNoteOff(45);
      }
      double left, right;
      engine.Process(left, right);
      output.push_back(static_cast<float>(left));
    }

    for (int i = 0; i < samplesPerGap; i++) {
      output.push_back(0.0f);
    }
  }

  PolySynth::WavWriter::Write("demo_filter_envelope.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_filter_envelope.wav" << std::endl;
  return 0;
}
