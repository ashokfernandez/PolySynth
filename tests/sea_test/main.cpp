#include <iostream>
#include <sea_dsp/sea_oscillator.h>

int main() {
  sea::Oscillator osc;
  osc.Init(44100.0);
  osc.SetFrequency(440.0);
  osc.SetWaveform(sea::Oscillator::WaveformType::Sine);

  float sample = osc.Process();
  std::cout << "SEA_DSP Oscillator Test: " << sample << std::endl;

  return 0;
}
