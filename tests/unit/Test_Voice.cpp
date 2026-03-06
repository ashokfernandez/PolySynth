// Isolated Voice tests. See TESTING_GUIDE.md for patterns.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 256;
} // namespace

CATCH_TEST_CASE("Voice idle produces silence", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    for (int i = 0; i < kBlockSize; ++i) {
        CATCH_REQUIRE(std::abs(v.Process()) < 1e-15);
    }
}

CATCH_TEST_CASE("Voice NoteOn produces non-zero output", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.NoteOn(60, 127);
    double maxVal = processMaxAbs(v, kBlockSize);
    CATCH_REQUIRE(std::isfinite(maxVal));
    CATCH_REQUIRE(maxVal > 1e-6);
}

CATCH_TEST_CASE("Voice returns to silence after NoteOff + release", "[Voice]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.05, 0.8, 0.01);
    v.SetFilterEnv(0.001, 0.05, 0.5, 0.01); // Fast filter env release too
    v.NoteOn(60, 127);
    processNSamples(v, kBlockSize);
    v.NoteOff();
    // Both envelopes have 10ms release; process generous margin
    processNSamples(v, 48000);
    CATCH_CHECK(!v.IsActive());
    CATCH_CHECK(std::abs(v.Process()) < 1e-10);
}

CATCH_TEST_CASE("Filter models produce distinct output", "[Voice][Filter]") {
    double rms[4] = {};
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(2000.0, 0.5, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        rms[model] = processRMS(v, kBlockSize * 4);
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            double relDiff = std::abs(rms[i] - rms[j]) / std::max(rms[i], rms[j]);
            CATCH_CHECK(relDiff > 0.01);
        }
    }
}

CATCH_TEST_CASE("All filter models stable at extreme resonance", "[Voice][Filter]") {
    for (int model = 0; model < 4; ++model) {
        Voice v;
        v.Init(kSampleRate);
        v.SetFilterModel(static_cast<Voice::FilterModel>(model));
        v.SetFilter(1000.0, 1.0, 0.0);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
        v.NoteOn(60, 127);
        CATCH_CHECK(processAllFinite(v, kBlockSize * 4));
    }
}

CATCH_TEST_CASE("Zero attack time produces immediate output", "[Voice][Envelope]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.0, 0.1, 1.0, 0.2);
    v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
    v.NoteOn(60, 127);
    double maxFirst8 = processMaxAbs(v, 8);
    CATCH_CHECK(std::isfinite(maxFirst8));
    CATCH_CHECK(maxFirst8 > 1e-6);
}

CATCH_TEST_CASE("Velocity scales output amplitude", "[Voice]") {
    Voice vLow;
    vLow.Init(kSampleRate);
    vLow.SetADSR(0.001, 0.1, 1.0, 0.2);
    vLow.NoteOn(60, 10);
    double rmsLow = processRMS(vLow, kBlockSize);

    Voice vHigh;
    vHigh.Init(kSampleRate);
    vHigh.SetADSR(0.001, 0.1, 1.0, 0.2);
    vHigh.NoteOn(60, 127);
    double rmsHigh = processRMS(vHigh, kBlockSize);

    CATCH_CHECK(rmsHigh > rmsLow * 1.5);
}

CATCH_TEST_CASE("StartSteal fades voice to zero within 25ms", "[Voice][Stealing]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 5.0);
    v.NoteOn(60, 127);
    processNSamples(v, kBlockSize);

    v.StartSteal();
    CATCH_CHECK(v.GetVoiceState() == VoiceState::Stolen);

    int samples25ms = static_cast<int>(0.025 * kSampleRate);
    processNSamples(v, samples25ms);

    CATCH_CHECK(!v.IsActive());
    CATCH_CHECK(v.GetVoiceState() == VoiceState::Idle);
}

CATCH_TEST_CASE("PolyMod OscB to FreqA modulates output", "[Voice][PolyMod]") {
    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetMixer(1.0, 1.0, 0.0);
    v1.SetPolyModOscBToFreqA(0.0);
    v1.NoteOn(60, 127);
    double rmsNoMod = processRMS(v1, kBlockSize * 2);

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetMixer(1.0, 1.0, 0.0);
    v2.SetPolyModOscBToFreqA(0.8);
    v2.NoteOn(60, 127);
    double rmsWithMod = processRMS(v2, kBlockSize * 2);

    double relDiff = std::abs(rmsNoMod - rmsWithMod) / std::max(rmsNoMod, rmsWithMod);
    CATCH_CHECK(relDiff > 0.01);
}

CATCH_TEST_CASE("PolyMod OscB to PWM modulates output", "[Voice][PolyMod]") {
    // Collect per-sample output to verify PWM modulation changes the waveform
    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetWaveformA(sea::Oscillator::WaveformType::Square);
    v1.SetWaveformB(sea::Oscillator::WaveformType::Saw);
    v1.SetMixer(1.0, 0.0, 700.0); // Only OscA in mix, OscB detuned for modulation
    v1.SetPolyModOscBToPWM(0.0);
    v1.NoteOn(60, 127);

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetWaveformA(sea::Oscillator::WaveformType::Square);
    v2.SetWaveformB(sea::Oscillator::WaveformType::Saw);
    v2.SetMixer(1.0, 0.0, 700.0);
    v2.SetPolyModOscBToPWM(0.8);
    v2.NoteOn(60, 127);

    // Compare sample-by-sample: at least some samples should differ
    int diffCount = 0;
    int totalSamples = kBlockSize * 4;
    for (int i = 0; i < totalSamples; ++i) {
        double s1 = v1.Process();
        double s2 = v2.Process();
        if (std::abs(s1 - s2) > 1e-10) diffCount++;
    }
    CATCH_CHECK(diffCount > totalSamples / 10); // At least 10% of samples differ
}

CATCH_TEST_CASE("PolyMod OscB to Filter modulates output", "[Voice][PolyMod]") {
    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetFilter(1000.0, 0.3, 0.0);
    v1.SetMixer(1.0, 1.0, 0.0);
    v1.SetPolyModOscBToFilter(0.0);
    v1.NoteOn(60, 127);
    double rmsNoMod = processRMS(v1, kBlockSize * 2);

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetFilter(1000.0, 0.3, 0.0);
    v2.SetMixer(1.0, 1.0, 0.0);
    v2.SetPolyModOscBToFilter(0.8);
    v2.NoteOn(60, 127);
    double rmsWithMod = processRMS(v2, kBlockSize * 2);

    double relDiff = std::abs(rmsNoMod - rmsWithMod) / std::max(rmsNoMod, rmsWithMod);
    CATCH_CHECK(relDiff > 0.01);
}

CATCH_TEST_CASE("Filter envelope modulates filter cutoff", "[Voice][Filter]") {
    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetFilterEnv(0.01, 0.1, 0.5, 0.2);
    v1.SetFilter(1000.0, 0.3, 0.0);
    v1.NoteOn(60, 127);
    double rmsNoEnv = processRMS(v1, kBlockSize * 4);

    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetFilterEnv(0.01, 0.1, 0.5, 0.2);
    v2.SetFilter(1000.0, 0.3, 0.8);
    v2.NoteOn(60, 127);
    double rmsWithEnv = processRMS(v2, kBlockSize * 4);

    double relDiff = std::abs(rmsNoEnv - rmsWithEnv) / std::max(rmsNoEnv, rmsWithEnv);
    CATCH_CHECK(relDiff > 0.01);
}
