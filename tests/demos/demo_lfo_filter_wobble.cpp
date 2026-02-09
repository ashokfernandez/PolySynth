/**
 * Demo: LFO Filter Wobble
 * Classic wub-wub sound with accelerating LFO modulation on filter
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering LFO Filter Wobble Demo..." << std::endl;

  double sampleRate = 44100.0;
  double duration = 4.0;
  int totalSamples = static_cast<int>(duration * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(PolySynthCore::Oscillator::WaveformType::Saw);
  engine.SetADSR(0.01, 0.1, 1.0, 0.2);

  // Set base filter low for maximum wobble effect
  engine.SetFilter(400.0, 4.0, 0.0);
  engine.SetLFORouting(0.0, 1.2, 0.0); // >1.0 for deep modulation

  engine.OnNoteOn(33, 100); // Low A

  for (int i = 0; i < totalSamples; i++) {
    // Accelerate LFO rate from 1Hz to 12Hz in steps
    double progress = static_cast<double>(i) / totalSamples;
    double rate;
    if (progress < 0.25)
      rate = 1.0;
    else if (progress < 0.5)
      rate = 2.0;
    else if (progress < 0.75)
      rate = 4.0;
    else
      rate = 8.0;

    engine.SetLFO(0, rate, 1.0);

    if (i == totalSamples - static_cast<int>(0.2 * sampleRate)) {
      engine.OnNoteOff(33);
    }

    double left, right;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write("demo_lfo_filter_wobble.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_lfo_filter_wobble.wav" << std::endl;
  return 0;
}
