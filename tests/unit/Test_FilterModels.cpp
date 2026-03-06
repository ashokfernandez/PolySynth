// Tests for filter model behaviour. See TESTING_GUIDE.md for patterns.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 2048;
} // namespace

CATCH_TEST_CASE("Each filter model produces distinct spectral output", "[Voice][Filter]") {
    double rms[4] = {};
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(2000.0, 0.5, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        rms[model] = processRMS(v, kRenderSamples);
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            double relDiff = std::abs(rms[i] - rms[j]) / std::max(rms[i], rms[j]);
            CATCH_CHECK(relDiff > 0.01);
        }
    }
}

CATCH_TEST_CASE("Switching filter model mid-note doesn't crash", "[Voice][Filter]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
    v.NoteOn(60, 127);

    v.SetFilterModel(Voice::FilterModel::Classic);
    CATCH_CHECK(processAllFinite(v, 256));

    v.SetFilterModel(Voice::FilterModel::Ladder);
    CATCH_CHECK(processAllFinite(v, 256));

    v.SetFilterModel(Voice::FilterModel::Cascade12);
    CATCH_CHECK(processAllFinite(v, 256));

    v.SetFilterModel(Voice::FilterModel::Cascade24);
    CATCH_CHECK(processAllFinite(v, 256));
}

CATCH_TEST_CASE("All models stable at max resonance with rich input", "[Voice][Filter]") {
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(1000.0, 1.0, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        CATCH_CHECK(processAllFinite(v, 4096));
    }
}

CATCH_TEST_CASE("Low cutoff attenuates signal for all models", "[Voice][Filter]") {
    for (int model = 0; model < 4; ++model) {
        Voice vLow;
        vLow.Init(kSampleRate);
        vLow.SetFilterModel(static_cast<Voice::FilterModel>(model));
        vLow.SetFilter(20.0, 0.3, 0.0);
        vLow.SetADSR(0.001, 0.1, 1.0, 0.2);
        vLow.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        vLow.NoteOn(60, 127);
        double rmsLow = processRMS(vLow, kRenderSamples);

        Voice vHigh;
        vHigh.Init(kSampleRate);
        vHigh.SetFilterModel(static_cast<Voice::FilterModel>(model));
        vHigh.SetFilter(20000.0, 0.3, 0.0);
        vHigh.SetADSR(0.001, 0.1, 1.0, 0.2);
        vHigh.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        vHigh.NoteOn(60, 127);
        double rmsHigh = processRMS(vHigh, kRenderSamples);

        CATCH_CHECK(rmsLow < rmsHigh * 0.9);
    }
}

CATCH_TEST_CASE("High cutoff passes signal for all models", "[Voice][Filter]") {
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(20000.0, 0.3, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        double rms = processRMS(v, kRenderSamples);
        CATCH_CHECK(rms > 0.001);
        CATCH_CHECK(processAllFinite(v, kRenderSamples));
    }
}
