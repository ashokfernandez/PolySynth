#include "../../src/core/VoiceManager.h"
#include "catch.hpp"
#include <vector>

namespace {
std::vector<PolySynthCore::sample_t> RenderBlock(PolySynthCore::VoiceManager &vm,
                                                 int frames) {
  std::vector<PolySynthCore::sample_t> buffer;
  buffer.reserve(frames);
  for (int i = 0; i < frames; ++i) {
    buffer.push_back(vm.Process());
  }
  return buffer;
}

double AverageAbsDiff(const std::vector<PolySynthCore::sample_t> &a,
                      const std::vector<PolySynthCore::sample_t> &b) {
  REQUIRE(a.size() == b.size());
  double sum = 0.0;
  for (size_t i = 0; i < a.size(); ++i) {
    sum += std::abs(a[i] - b[i]);
  }
  return sum / static_cast<double>(a.size());
}
} // namespace

TEST_CASE("PolyMod OscB to FreqA adds audio-rate FM", "[PolyMod]") {
  PolySynthCore::VoiceManager base;
  base.Init(48000.0);
  base.SetWaveformA(PolySynthCore::Oscillator::WaveformType::Saw);
  base.SetWaveformB(PolySynthCore::Oscillator::WaveformType::Sine);
  base.SetMixer(1.0, 0.0, 0.0);
  base.SetFilter(20000.0, 0.0, 0.0);
  base.SetADSR(0.01, 0.1, 1.0, 0.1);
  base.SetFilterEnv(0.01, 0.1, 0.0, 0.1);
  base.OnNoteOn(60, 100);

  PolySynthCore::VoiceManager modded = base;
  modded.SetPolyModOscBToFreqA(0.4);

  auto baseOut = RenderBlock(base, 512);
  auto modOut = RenderBlock(modded, 512);

  double diff = AverageAbsDiff(baseOut, modOut);
  REQUIRE(diff > 0.001);
}

TEST_CASE("PolyMod OscB to Filter modulates cutoff", "[PolyMod]") {
  PolySynthCore::VoiceManager base;
  base.Init(48000.0);
  base.SetWaveformA(PolySynthCore::Oscillator::WaveformType::Saw);
  base.SetWaveformB(PolySynthCore::Oscillator::WaveformType::Square);
  base.SetMixer(1.0, 0.0, 0.0);
  base.SetFilter(1200.0, 0.7, 0.0);
  base.SetADSR(0.01, 0.1, 1.0, 0.1);
  base.SetFilterEnv(0.01, 0.1, 0.0, 0.1);
  base.OnNoteOn(60, 100);

  PolySynthCore::VoiceManager modded = base;
  modded.SetPolyModOscBToFilter(0.6);

  auto baseOut = RenderBlock(base, 512);
  auto modOut = RenderBlock(modded, 512);

  double diff = AverageAbsDiff(baseOut, modOut);
  REQUIRE(diff > 0.001);
}

TEST_CASE("PolyMod Filter Env to PWM shapes OscA width", "[PolyMod]") {
  PolySynthCore::VoiceManager base;
  base.Init(48000.0);
  base.SetWaveformA(PolySynthCore::Oscillator::WaveformType::Square);
  base.SetWaveformB(PolySynthCore::Oscillator::WaveformType::Sine);
  base.SetPulseWidthA(0.5);
  base.SetMixer(1.0, 0.0, 0.0);
  base.SetFilter(20000.0, 0.0, 0.0);
  base.SetADSR(0.01, 0.1, 1.0, 0.1);
  base.SetFilterEnv(0.01, 0.2, 0.0, 0.1);
  base.OnNoteOn(60, 100);

  PolySynthCore::VoiceManager modded = base;
  modded.SetPolyModFilterEnvToPWM(0.8);

  auto baseOut = RenderBlock(base, 512);
  auto modOut = RenderBlock(modded, 512);

  double diff = AverageAbsDiff(baseOut, modOut);
  REQUIRE(diff > 0.001);
}
