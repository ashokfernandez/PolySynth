// Tests for LFO routing to pitch, filter, amp, and pan targets.
// See TESTING_GUIDE.md for test patterns and conventions.
#define CATCH_CONFIG_PREFIX_ALL
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"
#include <cmath>
#include <vector>

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 4096; // ~85ms, enough for LFO to modulate
} // namespace

CATCH_TEST_CASE("LFO pitch modulation varies frequency over time", "[Voice][LFO]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.SetLFO(0, 5.0, 1.0); // Sine, 5Hz, full depth
    v.SetLFORouting(1.0, 0.0, 0.0, 0.0);
    v.NoteOn(60, 127);

    // Count zero-crossings in first half vs second half
    int crossingsFirst = 0, crossingsSecond = 0;
    double prev = v.Process();
    for (int i = 1; i < kRenderSamples; ++i) {
        double s = v.Process();
        bool crossed = (prev > 0.0 && s <= 0.0) || (prev < 0.0 && s >= 0.0);
        if (crossed) {
            if (i < kRenderSamples / 2)
                crossingsFirst++;
            else
                crossingsSecond++;
        }
        prev = s;
    }
    // Frequency varies due to LFO vibrato, so zero-crossing counts should differ
    CATCH_CHECK(crossingsFirst != crossingsSecond);
}

CATCH_TEST_CASE("LFO filter modulation varies spectral content", "[Voice][LFO]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.SetFilter(1000.0, 0.3, 0.0);
    v.SetWaveformA(sea::Oscillator::WaveformType::Saw);
    v.SetLFO(0, 5.0, 1.0);
    v.SetLFORouting(0.0, 1.0, 0.0, 0.0);
    v.NoteOn(60, 127);

    // Compare RMS of first quarter vs third quarter
    int quarter = kRenderSamples / 4;
    double rmsQ1 = processRMS(v, quarter);
    processNSamples(v, quarter); // skip second quarter
    double rmsQ3 = processRMS(v, quarter);

    double relDiff = std::abs(rmsQ1 - rmsQ3) / std::max(rmsQ1, rmsQ3);
    CATCH_CHECK(relDiff > 0.01);
}

CATCH_TEST_CASE("LFO amp modulation varies amplitude (tremolo)", "[Voice][LFO]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.SetLFO(0, 5.0, 1.0);
    v.SetLFORouting(0.0, 0.0, 1.0, 0.0);
    v.NoteOn(60, 127);

    // Compute max amplitude of consecutive 512-sample blocks
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = kRenderSamples / kBlockSize;
    double blockMax[kNumBlocks];
    for (int b = 0; b < kNumBlocks; ++b) {
        blockMax[b] = processMaxAbs(v, kBlockSize);
    }

    // Find min and max block amplitude
    double minBlock = blockMax[0], maxBlock = blockMax[0];
    for (int b = 1; b < kNumBlocks; ++b) {
        if (blockMax[b] < minBlock) minBlock = blockMax[b];
        if (blockMax[b] > maxBlock) maxBlock = blockMax[b];
    }

    double relDiff = (maxBlock - minBlock) / maxBlock;
    CATCH_CHECK(relDiff > 0.05);
}

CATCH_TEST_CASE("LFO pan modulation varies pan position", "[Voice][LFO]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.001, 0.1, 1.0, 0.2);
    v.SetLFO(0, 5.0, 1.0);
    v.SetLFORouting(0.0, 0.0, 0.0, 1.0);
    v.NoteOn(60, 127);

    // mLastLfoVal is only updated inside Process(), so we must call Process()
    // between GetPanPosition() reads to advance the LFO
    float pan1 = v.GetPanPosition();
    for (int i = 0; i < 2400; ++i) v.Process(); // ~half LFO cycle at 5Hz
    float pan2 = v.GetPanPosition();
    for (int i = 0; i < 2400; ++i) v.Process();
    float pan3 = v.GetPanPosition();

    // At least two of the sampled pan positions should differ measurably
    bool diff12 = std::abs(pan1 - pan2) > 0.01f;
    bool diff23 = std::abs(pan2 - pan3) > 0.01f;
    bool diff13 = std::abs(pan1 - pan3) > 0.01f;
    CATCH_CHECK((diff12 || diff23 || diff13));
}

CATCH_TEST_CASE("LFO depth=0 produces no modulation", "[Voice][LFO]") {
    // Voice with depth=0, all routing at 1.0
    Voice v1;
    v1.Init(kSampleRate);
    v1.SetADSR(0.001, 0.1, 1.0, 0.2);
    v1.SetLFO(0, 5.0, 0.0); // depth = 0
    v1.SetLFORouting(1.0, 1.0, 1.0, 1.0);
    v1.NoteOn(60, 127);

    // Voice with depth=0, routing=0
    Voice v2;
    v2.Init(kSampleRate);
    v2.SetADSR(0.001, 0.1, 1.0, 0.2);
    v2.SetLFO(0, 5.0, 0.0); // depth = 0
    v2.SetLFORouting(0.0, 0.0, 0.0, 0.0);
    v2.NoteOn(60, 127);

    // Output should be identical since LFO depth is zero
    for (int i = 0; i < kRenderSamples; ++i) {
        double s1 = v1.Process();
        double s2 = v2.Process();
        CATCH_CHECK(s1 == s2);
    }
}

CATCH_TEST_CASE("All LFO waveforms produce valid output", "[Voice][LFO]") {
    for (int waveform = 0; waveform < 4; ++waveform) {
        Voice v;
        v.Init(kSampleRate);
        v.SetADSR(0.001, 0.1, 1.0, 0.2);
        v.SetLFO(waveform, 5.0, 0.5);
        v.SetLFORouting(1.0, 0.0, 0.0, 0.0);
        v.NoteOn(60, 127);
        CATCH_CHECK(processAllFinite(v, kRenderSamples));
    }
}

CATCH_TEST_CASE("LFO rate affects modulation speed", "[Voice][LFO]") {
    // Use pan modulation to directly observe LFO rate differences,
    // since GetPanPosition() reflects the LFO value without oscillator interference.

    // Slow LFO: 1Hz
    Voice vSlow;
    vSlow.Init(kSampleRate);
    vSlow.SetADSR(0.001, 0.1, 1.0, 0.2);
    vSlow.SetLFO(0, 1.0, 1.0);
    vSlow.SetLFORouting(0.0, 0.0, 0.0, 1.0); // pan routing

    // Fast LFO: 10Hz
    Voice vFast;
    vFast.Init(kSampleRate);
    vFast.SetADSR(0.001, 0.1, 1.0, 0.2);
    vFast.SetLFO(0, 10.0, 1.0);
    vFast.SetLFORouting(0.0, 0.0, 0.0, 1.0);

    vSlow.NoteOn(60, 127);
    vFast.NoteOn(60, 127);

    // Count pan zero-crossings (sign changes in pan position) over 0.5 seconds
    constexpr int kHalfSecondSamples = 24000;

    auto countPanCrossings = [](Voice& v, int n) {
        int crossings = 0;
        v.Process();
        float prevPan = v.GetPanPosition();
        for (int i = 1; i < n; ++i) {
            v.Process();
            float pan = v.GetPanPosition();
            if ((prevPan > 0.0f && pan <= 0.0f) || (prevPan < 0.0f && pan >= 0.0f))
                crossings++;
            prevPan = pan;
        }
        return crossings;
    };

    int slowCrossings = countPanCrossings(vSlow, kHalfSecondSamples);
    int fastCrossings = countPanCrossings(vFast, kHalfSecondSamples);

    CATCH_CHECK(fastCrossings > slowCrossings);
}
