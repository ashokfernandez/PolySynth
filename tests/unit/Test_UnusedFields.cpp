// Tests for unused SynthState fields. See TESTING_GUIDE.md for patterns.
// Each test verifies that an unused field has no effect on audio output.
#define CATCH_CONFIG_PREFIX_ALL
#include "Engine.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 2048;

void verifyIdenticalOutput(const SynthState& stateA, const SynthState& stateB) {
    Engine engineA;
    engineA.Init(kSampleRate);
    engineA.UpdateState(stateA);
    engineA.OnNoteOn(60, 100);

    Engine engineB;
    engineB.Init(kSampleRate);
    engineB.UpdateState(stateB);
    engineB.OnNoteOn(60, 100);

    for (int i = 0; i < kRenderSamples; ++i) {
        sample_t lA = 0.0, rA = 0.0, lB = 0.0, rB = 0.0;
        engineA.Process(lA, rA);
        engineB.Process(lB, rB);
        CATCH_CHECK(lA == lB);
        CATCH_CHECK(rA == rB);
    }
}
} // namespace

CATCH_TEST_CASE("Unused field 'unison' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.unison = true;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'unisonDetune' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.unisonDetune = 1.0;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'oscASync' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.oscASync = true;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'oscBLoFreq' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.oscBLoFreq = true;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'oscBKeyboardPoints' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.oscBKeyboardPoints = false; // default is true
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'mixNoise' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.mixNoise = 1.0;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'filterKeyboardTrack' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.filterKeyboardTrack = true;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'oscAFreq' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.oscAFreq = 880.0;
    verifyIdenticalOutput(stateA, stateB);
}

CATCH_TEST_CASE("Unused field 'oscBFreq' has no audio effect", "[UnusedFields]") {
    SynthState stateA;
    SynthState stateB;
    stateB.oscBFreq = 880.0;
    verifyIdenticalOutput(stateA, stateB);
}
