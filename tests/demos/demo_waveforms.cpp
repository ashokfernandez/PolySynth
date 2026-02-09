/**
 * Demo: Waveform Comparison
 * Plays the same note with Saw, Square, Triangle, and Sine waveforms
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Waveform Comparison Demo..." << std::endl;

  double sampleRate = 44100.0;
  double noteDuration = 1.0;
  double silenceGap = 0.3;
  int samplesPerNote = static_cast<int>(noteDuration * sampleRate);
  int samplesPerGap = static_cast<int>(silenceGap * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetADSR(0.01, 0.1, 0.8, 0.2);

  // Waveforms: Saw, Square, Triangle, Sine
  PolySynthCore::Oscillator::WaveformType types[] = {
      PolySynthCore::Oscillator::WaveformType::Saw,
      PolySynthCore::Oscillator::WaveformType::Square,
      PolySynthCore::Oscillator::WaveformType::Triangle,
      PolySynthCore::Oscillator::WaveformType::Sine};

  for (int i = 0; i < 4; i++) {
    engine.SetWaveform(types[i]);
    engine.OnNoteOn(60, 100); // Middle C

    // Play note
    for (int s = 0; s < samplesPerNote; s++) {
      if (s == samplesPerNote - static_cast<int>(0.3 * sampleRate)) {
        engine.OnNoteOff(60);
      }
      double left, right;
      engine.Process(left, right);
      output.push_back(static_cast<float>(left));
    }

    // Gap
    for (int s = 0; s < samplesPerGap; s++) {
      output.push_back(0.0f);
    }
  }

  PolySynth::WavWriter::Write("demo_waveforms.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_waveforms.wav" << std::endl;
  return 0;
}
