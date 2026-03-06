// Tests for parameter boundary safety. See TESTING_GUIDE.md for patterns.
#define CATCH_CONFIG_PREFIX_ALL
#include "Engine.h"
#include "Voice.h"
#include "TestHelpers.h"
#include "catch.hpp"
#include <cmath>

using namespace PolySynthCore;
using namespace TestHelpers;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kRenderSamples = 2048;

bool engineProcessAllFinite(Engine& engine, int n) {
    for (int i = 0; i < n; ++i) {
        sample_t left = 0.0, right = 0.0;
        engine.Process(left, right);
        if (!std::isfinite(left) || !std::isfinite(right)) return false;
    }
    return true;
}
} // namespace

CATCH_TEST_CASE("All SynthState fields at minimum value", "[Engine][Boundaries]") {
    Engine engine;
    engine.Init(kSampleRate);
    SynthState state;
    state.masterGain = 0.0;
    state.polyphony = 1;
    state.glideTime = 0.0;
    state.ampAttack = 0.0;
    state.ampDecay = 0.0;
    state.ampSustain = 0.0;
    state.ampRelease = 0.0;
    state.filterAttack = 0.0;
    state.filterDecay = 0.0;
    state.filterSustain = 0.0;
    state.filterRelease = 0.0;
    state.filterCutoff = 20.0;
    state.filterResonance = 0.0;
    state.filterEnvAmount = 0.0;
    state.lfoRate = 0.01;
    state.lfoDepth = 0.0;
    state.mixOscA = 0.0;
    state.mixOscB = 0.0;
    state.mixNoise = 0.0;
    state.fxChorusMix = 0.0;
    state.fxDelayMix = 0.0;
    engine.UpdateState(state);
    engine.OnNoteOn(60, 127);
    CATCH_CHECK(engineProcessAllFinite(engine, kRenderSamples));
}

CATCH_TEST_CASE("All SynthState fields at maximum value", "[Engine][Boundaries]") {
    Engine engine;
    engine.Init(kSampleRate);
    SynthState state;
    state.masterGain = 1.0;
    state.polyphony = 16;
    state.ampAttack = 10.0;
    state.ampDecay = 10.0;
    state.ampSustain = 1.0;
    state.ampRelease = 10.0;
    state.filterAttack = 10.0;
    state.filterDecay = 10.0;
    state.filterSustain = 1.0;
    state.filterRelease = 10.0;
    state.filterCutoff = 20000.0;
    state.filterResonance = 1.0;
    state.filterEnvAmount = 1.0;
    state.lfoRate = 20.0;
    state.lfoDepth = 1.0;
    state.mixOscA = 1.0;
    state.mixOscB = 1.0;
    state.polyModOscBToFreqA = 1.0;
    state.polyModOscBToPWM = 1.0;
    state.polyModOscBToFilter = 1.0;
    state.polyModFilterEnvToFreqA = 1.0;
    state.polyModFilterEnvToPWM = 1.0;
    state.polyModFilterEnvToFilter = 1.0;
    state.fxChorusRate = 5.0;
    state.fxChorusDepth = 1.0;
    state.fxChorusMix = 1.0;
    state.fxDelayTime = 1.2;
    state.fxDelayFeedback = 0.95;
    state.fxDelayMix = 1.0;
    state.fxLimiterThreshold = 1.0;
    engine.UpdateState(state);
    engine.OnNoteOn(60, 127);
    CATCH_CHECK(engineProcessAllFinite(engine, kRenderSamples));
}

CATCH_TEST_CASE("Rapid parameter changes every sample", "[Engine][Boundaries]") {
    Engine engine;
    engine.Init(kSampleRate);
    SynthState state;
    engine.UpdateState(state);
    engine.OnNoteOn(60, 127);
    for (int i = 0; i < 1024; ++i) {
        state.filterCutoff = (i % 2 == 0) ? 20.0 : 20000.0;
        engine.UpdateState(state);
        sample_t left = 0.0, right = 0.0;
        engine.Process(left, right);
        CATCH_REQUIRE(std::isfinite(left));
        CATCH_REQUIRE(std::isfinite(right));
    }
}

CATCH_TEST_CASE("Zero-duration envelope", "[Voice][Boundaries]") {
    Voice v;
    v.Init(kSampleRate);
    v.SetADSR(0.0, 0.0, 0.0, 0.0);
    v.SetFilterEnv(0.0, 0.0, 0.0, 0.0);
    v.NoteOn(60, 127);
    CATCH_CHECK(processAllFinite(v, 512));
}

CATCH_TEST_CASE("Max polyphony 16 simultaneous notes", "[Engine][Boundaries]") {
    Engine engine;
    engine.Init(kSampleRate);
    SynthState state;
    state.polyphony = 16;
    engine.UpdateState(state);
    for (int note = 48; note < 64; ++note) {
        engine.OnNoteOn(note, 127);
    }
    CATCH_CHECK(engineProcessAllFinite(engine, kRenderSamples));
}

CATCH_TEST_CASE("MIDI boundary notes", "[Engine][Boundaries]") {
    Engine engine;
    engine.Init(kSampleRate);
    SynthState state;
    engine.UpdateState(state);

    engine.OnNoteOn(0, 127);
    CATCH_CHECK(engineProcessAllFinite(engine, 256));
    engine.OnNoteOff(0);

    engine.OnNoteOn(127, 127);
    CATCH_CHECK(engineProcessAllFinite(engine, 256));
    engine.OnNoteOff(127);
}
