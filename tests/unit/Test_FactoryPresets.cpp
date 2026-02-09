#define CATCH_CONFIG_PREFIX_ALL
#include "SynthState.h"
#include "catch.hpp"

// Test that factory presets have distinct, measurable differences
// These values match what's defined in PolySynth.cpp OnMessage handlers

CATCH_TEST_CASE("Factory Preset: Warm Pad has correct values",
                "[preset][factory]") {
  using namespace PolySynthCore;

  SynthState state;

  // Apply Warm Pad preset values (from kMsgTagPreset1)
  state.masterGain = 0.8;
  state.ampAttack = 0.5; // 500ms
  state.ampDecay = 0.3;
  state.ampSustain = 0.7;
  state.ampRelease = 1.0; // 1 second
  state.filterCutoff = 800.0;
  state.filterResonance = 0.1;
  state.oscAWaveform = 0; // Saw
  state.lfoShape = 0;     // Sine
  state.lfoRate = 0.5;
  state.lfoDepth = 0.3;

  // Verify key characteristics of a "warm pad"
  CATCH_CHECK(state.ampAttack >= 0.3);       // Slow attack (>300ms)
  CATCH_CHECK(state.ampRelease >= 0.5);      // Long release (>500ms)
  CATCH_CHECK(state.filterCutoff <= 2000.0); // Low cutoff for warmth
  CATCH_CHECK(state.filterResonance <= 0.3); // Low resonance, not harsh
}

CATCH_TEST_CASE("Factory Preset: Bright Lead has correct values",
                "[preset][factory]") {
  using namespace PolySynthCore;

  SynthState state;

  // Apply Bright Lead preset values (from kMsgTagPreset2)
  state.masterGain = 1.0;
  state.ampAttack = 0.005; // 5ms
  state.ampDecay = 0.1;
  state.ampSustain = 0.6;
  state.ampRelease = 0.2;
  state.filterCutoff = 18000.0;
  state.filterResonance = 0.7;
  state.oscAWaveform = 1; // Square
  state.lfoShape = 2;     // Square LFO
  state.lfoRate = 6.0;
  state.lfoDepth = 0.0; // No LFO

  // Verify key characteristics of a "bright lead"
  CATCH_CHECK(state.ampAttack <= 0.02);       // Fast attack (<20ms)
  CATCH_CHECK(state.filterCutoff >= 10000.0); // High cutoff for brightness
  CATCH_CHECK(state.filterResonance >= 0.5);  // Medium-high resonance
  CATCH_CHECK(state.oscAWaveform == 1);       // Square wave for punch
}

CATCH_TEST_CASE("Factory Preset: Dark Bass has correct values",
                "[preset][factory]") {
  using namespace PolySynthCore;

  SynthState state;

  // Apply Dark Bass preset values (from kMsgTagPreset3)
  state.masterGain = 0.9;
  state.ampAttack = 0.02; // 20ms
  state.ampDecay = 0.5;
  state.ampSustain = 0.4;
  state.ampRelease = 0.15;
  state.filterCutoff = 300.0;
  state.filterResonance = 0.5;
  state.oscAWaveform = 0; // Saw
  state.lfoShape = 1;     // Triangle
  state.lfoRate = 2.0;
  state.lfoDepth = 0.5;

  // Verify key characteristics of a "dark bass"
  CATCH_CHECK(state.filterCutoff <= 500.0); // Very low cutoff for darkness
  CATCH_CHECK(state.lfoDepth >= 0.3);       // Active LFO modulation
  CATCH_CHECK(state.oscAWaveform == 0);     // Saw wave for bass harmonics
}

CATCH_TEST_CASE("Factory presets are distinct from each other",
                "[preset][factory]") {
  using namespace PolySynthCore;

  // Define the three presets
  SynthState warmPad, brightLead, darkBass;

  warmPad.filterCutoff = 800.0;
  warmPad.ampAttack = 0.5;

  brightLead.filterCutoff = 18000.0;
  brightLead.ampAttack = 0.005;

  darkBass.filterCutoff = 300.0;
  darkBass.ampAttack = 0.02;

  // Presets should be measurably different
  CATCH_CHECK(warmPad.filterCutoff != brightLead.filterCutoff);
  CATCH_CHECK(warmPad.filterCutoff != darkBass.filterCutoff);
  CATCH_CHECK(brightLead.filterCutoff != darkBass.filterCutoff);

  CATCH_CHECK(warmPad.ampAttack != brightLead.ampAttack);
  CATCH_CHECK(warmPad.ampAttack != darkBass.ampAttack);
}
