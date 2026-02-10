#include "../../src/core/types.h"
#include "../utils/WavWriter.h"
#include <cmath>
#include <iostream>
#include <sea_dsp/sea_platform.h>
#include <sea_dsp/sea_prophet_filter.h>
#include <vector>

int main() {
  const double sampleRate = 44100.0;
  const double duration = 4.0;
  const int totalSamples = static_cast<int>(duration * sampleRate);

  std::cout << "Rendering Prophet Filter Demo..." << std::endl;

  sea::ProphetFilter<PolySynthCore::sample_t> filter;
  filter.Init(sampleRate);

  std::vector<float> output;
  output.reserve(totalSamples);

  double phase = 0.0;
  double phaseInc = sea::kTwoPi * 110.0 / sampleRate;

  for (int i = 0; i < totalSamples; ++i) {
    double t = static_cast<double>(i) / sampleRate;
    double cutoff = 400.0 + 4600.0 * (0.5 * (1.0 + std::sin(t * 0.5)));
    double resonance = 0.2 + 0.6 * (0.5 * (1.0 + std::sin(t * 0.25)));
    auto slope = (t < duration * 0.5)
                     ? sea::ProphetFilter<PolySynthCore::sample_t>::Slope::dB12
                     : sea::ProphetFilter<PolySynthCore::sample_t>::Slope::dB24;

    filter.SetParams(cutoff, resonance, slope);

    double input = std::sin(phase) * 0.4;
    phase += phaseInc;
    if (phase > sea::kTwoPi)
      phase -= sea::kTwoPi;

    double filtered = filter.Process(input);
    output.push_back(static_cast<float>(filtered));
  }

  PolySynth::WavWriter::Write("demo_prophet_filter.wav", output,
                              static_cast<int>(sampleRate));

  std::cout << "Generated: demo_prophet_filter.wav" << std::endl;
  return 0;
}
