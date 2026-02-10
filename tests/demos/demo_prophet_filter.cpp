#include "../../src/core/dsp/va/VAProphetFilter.h"
#include "../utils/WavWriter.h"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
  const double sampleRate = 44100.0;
  const double duration = 4.0;
  const int totalSamples = static_cast<int>(duration * sampleRate);

  std::cout << "Rendering Prophet Filter Demo..." << std::endl;

  PolySynthCore::VAProphetFilter filter;
  filter.Init(sampleRate);

  std::vector<float> output;
  output.reserve(totalSamples);

  double phase = 0.0;
  double phaseInc = PolySynthCore::kTwoPi * 110.0 / sampleRate;

  for (int i = 0; i < totalSamples; ++i) {
    double t = static_cast<double>(i) / sampleRate;
    double cutoff = 400.0 + 4600.0 * (0.5 * (1.0 + std::sin(t * 0.5)));
    double resonance = 0.2 + 0.6 * (0.5 * (1.0 + std::sin(t * 0.25)));
    auto slope = (t < duration * 0.5) ? PolySynthCore::VAProphetFilter::Slope::dB12
                                      : PolySynthCore::VAProphetFilter::Slope::dB24;

    filter.SetParams(cutoff, resonance, slope);

    double input = std::sin(phase) * 0.4;
    phase += phaseInc;
    if (phase > PolySynthCore::kTwoPi)
      phase -= PolySynthCore::kTwoPi;

    double filtered = filter.Process(input);
    output.push_back(static_cast<float>(filtered));
  }

  PolySynth::WavWriter::Write("demo_prophet_filter.wav", output,
                              static_cast<int>(sampleRate));

  std::cout << "Generated: demo_prophet_filter.wav" << std::endl;
  return 0;
}
