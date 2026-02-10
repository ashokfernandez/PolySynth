/**
 * Demo: Pulse Width Modulation
 * Square wave with PWM sweep
 */
#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Pulse Width Modulation Demo..." << std::endl;

  double sampleRate = 44100.0;
  double duration = 4.0;
  int totalSamples = static_cast<int>(duration * sampleRate);

  std::vector<float> output;

  PolySynthCore::Engine engine;
  engine.Init(sampleRate);
  engine.SetWaveform(sea::Oscillator::WaveformType::Square);
  engine.SetADSR(0.1, 0.2, 0.8, 0.5);

  engine.OnNoteOn(45, 100); // Low A

  for (int i = 0; i < totalSamples; i++) {
    // Sweep pulse width from 0.1 to 0.9
    double pwm = 0.1 + 0.8 * (static_cast<double>(i) / totalSamples);
    engine.SetPulseWidth(pwm);

    if (i == totalSamples - static_cast<int>(0.5 * sampleRate)) {
      engine.OnNoteOff(45);
    }

    double left, right;
    engine.Process(left, right);
    output.push_back(static_cast<float>(left));
  }

  PolySynth::WavWriter::Write("demo_pulse_width.wav", output,
                              static_cast<int>(sampleRate));
  std::cout << "Generated: demo_pulse_width.wav" << std::endl;
  return 0;
}
