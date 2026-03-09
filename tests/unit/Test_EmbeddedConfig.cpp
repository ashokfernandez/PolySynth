// Embedded-configuration unit tests.
// Only compiled when SEA_PLATFORM_EMBEDDED is defined (run_tests_embedded target).
// Verifies that the DSP engine works correctly with float precision and 4 voices.

#include "catch.hpp"
#include "types.h"
#include "Engine.h"
#include "SynthState.h"
#include <cmath>
#include <type_traits>

#if defined(SEA_PLATFORM_EMBEDDED)

using namespace PolySynthCore;

TEST_CASE("Embedded: sample_t is float", "[EmbeddedConfig]") {
    STATIC_REQUIRE(std::is_same_v<sample_t, float>);
}

TEST_CASE("Embedded: sea::Real is float", "[EmbeddedConfig]") {
    STATIC_REQUIRE(std::is_same_v<sea::Real, float>);
}

TEST_CASE("Embedded: kMaxVoices is 4", "[EmbeddedConfig]") {
    REQUIRE(kMaxVoices == 4);
}

TEST_CASE("Embedded: Engine produces audio in float mode", "[EmbeddedConfig]") {
    Engine engine;
    SynthState state;
    state.Reset();

    engine.Init(48000.0);
    engine.UpdateState(state);
    engine.OnNoteOn(60, 100);

    float left = 0.0f, right = 0.0f;
    float energy = 0.0f;
    for (int i = 0; i < 256; i++) {
        left = 0.0f;
        right = 0.0f;
        engine.Process(left, right);
        energy += left * left + right * right;
    }

    REQUIRE(energy > 0.0f);
    REQUIRE(std::isfinite(left));
    REQUIRE(std::isfinite(right));
}

TEST_CASE("Embedded: 4-voice polyphony works", "[EmbeddedConfig]") {
    Engine engine;
    SynthState state;
    state.Reset();

    engine.Init(48000.0);
    engine.UpdateState(state);

    engine.OnNoteOn(60, 100);  // C4
    engine.OnNoteOn(64, 90);   // E4
    engine.OnNoteOn(67, 80);   // G4
    engine.OnNoteOn(72, 70);   // C5

    float left, right;
    float energy = 0.0f;
    for (int i = 0; i < 256; i++) {
        left = 0.0f;
        right = 0.0f;
        engine.Process(left, right);
        energy += left * left + right * right;
    }

    REQUIRE(energy > 0.0001f);
}

TEST_CASE("Embedded: float precision sufficient for pitch", "[EmbeddedConfig]") {
    // Middle C (261.63 Hz) — verify float precision doesn't ruin pitch
    float freq = 261.63f;
    float period_samples = 48000.0f / freq;
    REQUIRE(period_samples == Approx(183.5f).margin(0.5f));

    float phase_inc = 2.0f * 3.14159265f * freq / 48000.0f;
    REQUIRE(std::isfinite(phase_inc));
    REQUIRE(phase_inc > 0.0f);
    REQUIRE(phase_inc < 1.0f);
}

TEST_CASE("Embedded: SynthState Reset works with all field types", "[EmbeddedConfig]") {
    SynthState state;
    state.Reset();

    REQUIRE(state.masterGain == Approx(0.75));
    REQUIRE(state.filterCutoff == Approx(2000.0));
    REQUIRE(state.ampSustain == Approx(1.0));
}

#endif // SEA_PLATFORM_EMBEDDED
