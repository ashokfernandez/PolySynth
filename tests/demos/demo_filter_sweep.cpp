#include "../../src/core/types.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_oscillator.h>
#include <vector>

int main() {
  std::cout << "Rendering Filter Sweep Demo..." << std::endl;
  double sr = 48000.0;

  sea::Oscillator osc;
  osc.Init(sr);
  osc.SetFrequency(100.0); // Low Sawtooth

  sea::BiquadFilter<PolySynthCore::sample_t> filter;
  filter.Init(sr);
  filter.SetParams(sea::FilterType::LowPass, 5000.0,
                   2.0); // Start open, Resonant

  int duration = 2 * (int)sr;
  std::vector<float> buffer;
  buffer.reserve(duration);

  for (int i = 0; i < duration; i++) {
    // Sweep cutoff from 5000Hz down to 100Hz
    double t = (double)i / duration;
    double cutoff = 5000.0 * (1.0 - t) + 100.0 * t;

    // Update filter every sample? Expensive but smooth.
    // In reality we might smooth the parameter or update per block.
    // For demo, per sample is fine.
    filter.SetParams(sea::FilterType::LowPass, cutoff, 4.0);

    double raw = osc.Process();
    double filtered = filter.Process(raw);

    buffer.push_back((float)filtered * 0.5f); // Scale down due to resonance
  }

  PolySynth::WavWriter::Write("demo_filter_sweep.wav", buffer, (int)sr);
  return 0;
}
