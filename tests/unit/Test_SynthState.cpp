#define CATCH_CONFIG_PREFIX_ALL
#include "PresetManager.h"
#include "SynthState.h"
#include "catch.hpp"

CATCH_TEST_CASE("SynthState Reset restores all fields to defaults",
                "[SynthState]") {
  using namespace PolySynthCore;

  SynthState modified;

  // Set every field to a non-default value
  modified.masterGain = 0.1;
  modified.polyphony = 2;
  modified.unison = true;
  modified.unisonDetune = 0.5;
  modified.glideTime = 1.0;

  modified.oscAWaveform = 1;
  modified.oscAFreq = 220.0;
  modified.oscAPulseWidth = 0.3;
  modified.oscASync = true;

  modified.oscBWaveform = 2;
  modified.oscBFreq = 880.0;
  modified.oscBFineTune = 7.0;
  modified.oscBPulseWidth = 0.8;
  modified.oscBLoFreq = true;
  modified.oscBKeyboardPoints = false;

  modified.mixOscA = 0.5;
  modified.mixOscB = 0.5;
  modified.mixNoise = 0.3;

  modified.filterCutoff = 10000.0;
  modified.filterResonance = 0.9;
  modified.filterEnvAmount = 0.7;
  modified.filterModel = 3;
  modified.filterKeyboardTrack = true;

  modified.filterAttack = 0.5;
  modified.filterDecay = 0.5;
  modified.filterSustain = 0.3;
  modified.filterRelease = 1.0;

  modified.ampAttack = 0.5;
  modified.ampDecay = 0.5;
  modified.ampSustain = 0.5;
  modified.ampRelease = 0.5;

  modified.lfoShape = 3;
  modified.lfoRate = 10.0;
  modified.lfoDepth = 0.8;

  modified.polyModOscBToFreqA = 0.5;
  modified.polyModOscBToPWM = 0.5;
  modified.polyModOscBToFilter = 0.5;
  modified.polyModFilterEnvToFreqA = 0.5;
  modified.polyModFilterEnvToPWM = 0.5;
  modified.polyModFilterEnvToFilter = 0.5;

  modified.fxChorusRate = 1.0;
  modified.fxChorusDepth = 1.0;
  modified.fxChorusMix = 0.5;
  modified.fxDelayTime = 1.0;
  modified.fxDelayFeedback = 0.8;
  modified.fxDelayMix = 0.5;
  modified.fxLimiterThreshold = 0.5;

  // Reset and compare against a freshly constructed state
  modified.Reset();
  SynthState fresh;

  // Global
  CATCH_CHECK(modified.masterGain == fresh.masterGain);
  CATCH_CHECK(modified.polyphony == fresh.polyphony);
  CATCH_CHECK(modified.unison == fresh.unison);
  CATCH_CHECK(modified.unisonDetune == fresh.unisonDetune);
  CATCH_CHECK(modified.glideTime == fresh.glideTime);

  // Osc A
  CATCH_CHECK(modified.oscAWaveform == fresh.oscAWaveform);
  CATCH_CHECK(modified.oscAFreq == fresh.oscAFreq);
  CATCH_CHECK(modified.oscAPulseWidth == fresh.oscAPulseWidth);
  CATCH_CHECK(modified.oscASync == fresh.oscASync);

  // Osc B
  CATCH_CHECK(modified.oscBWaveform == fresh.oscBWaveform);
  CATCH_CHECK(modified.oscBFreq == fresh.oscBFreq);
  CATCH_CHECK(modified.oscBFineTune == fresh.oscBFineTune);
  CATCH_CHECK(modified.oscBPulseWidth == fresh.oscBPulseWidth);
  CATCH_CHECK(modified.oscBLoFreq == fresh.oscBLoFreq);
  CATCH_CHECK(modified.oscBKeyboardPoints == fresh.oscBKeyboardPoints);

  // Mixer
  CATCH_CHECK(modified.mixOscA == fresh.mixOscA);
  CATCH_CHECK(modified.mixOscB == fresh.mixOscB);
  CATCH_CHECK(modified.mixNoise == fresh.mixNoise);

  // Filter
  CATCH_CHECK(modified.filterCutoff == fresh.filterCutoff);
  CATCH_CHECK(modified.filterResonance == fresh.filterResonance);
  CATCH_CHECK(modified.filterEnvAmount == fresh.filterEnvAmount);
  CATCH_CHECK(modified.filterModel == fresh.filterModel);
  CATCH_CHECK(modified.filterKeyboardTrack == fresh.filterKeyboardTrack);

  // Filter Envelope
  CATCH_CHECK(modified.filterAttack == fresh.filterAttack);
  CATCH_CHECK(modified.filterDecay == fresh.filterDecay);
  CATCH_CHECK(modified.filterSustain == fresh.filterSustain);
  CATCH_CHECK(modified.filterRelease == fresh.filterRelease);

  // Amp Envelope
  CATCH_CHECK(modified.ampAttack == fresh.ampAttack);
  CATCH_CHECK(modified.ampDecay == fresh.ampDecay);
  CATCH_CHECK(modified.ampSustain == fresh.ampSustain);
  CATCH_CHECK(modified.ampRelease == fresh.ampRelease);

  // LFO
  CATCH_CHECK(modified.lfoShape == fresh.lfoShape);
  CATCH_CHECK(modified.lfoRate == fresh.lfoRate);
  CATCH_CHECK(modified.lfoDepth == fresh.lfoDepth);

  // Poly-Mod
  CATCH_CHECK(modified.polyModOscBToFreqA == fresh.polyModOscBToFreqA);
  CATCH_CHECK(modified.polyModOscBToPWM == fresh.polyModOscBToPWM);
  CATCH_CHECK(modified.polyModOscBToFilter == fresh.polyModOscBToFilter);
  CATCH_CHECK(modified.polyModFilterEnvToFreqA ==
              fresh.polyModFilterEnvToFreqA);
  CATCH_CHECK(modified.polyModFilterEnvToPWM == fresh.polyModFilterEnvToPWM);
  CATCH_CHECK(modified.polyModFilterEnvToFilter ==
              fresh.polyModFilterEnvToFilter);

  // FX
  CATCH_CHECK(modified.fxChorusRate == fresh.fxChorusRate);
  CATCH_CHECK(modified.fxChorusDepth == fresh.fxChorusDepth);
  CATCH_CHECK(modified.fxChorusMix == fresh.fxChorusMix);
  CATCH_CHECK(modified.fxDelayTime == fresh.fxDelayTime);
  CATCH_CHECK(modified.fxDelayFeedback == fresh.fxDelayFeedback);
  CATCH_CHECK(modified.fxDelayMix == fresh.fxDelayMix);
  CATCH_CHECK(modified.fxLimiterThreshold == fresh.fxLimiterThreshold);
}

CATCH_TEST_CASE("SynthState default values are within valid ranges",
                "[SynthState]") {
  using namespace PolySynthCore;

  SynthState state;

  // Gain
  CATCH_CHECK(state.masterGain >= 0.0);
  CATCH_CHECK(state.masterGain <= 1.0);

  // Polyphony
  CATCH_CHECK(state.polyphony >= 1);
  CATCH_CHECK(state.polyphony <= 16);

  // Oscillator frequencies must be positive
  CATCH_CHECK(state.oscAFreq > 0.0);
  CATCH_CHECK(state.oscBFreq > 0.0);

  // Pulse widths in [0,1]
  CATCH_CHECK(state.oscAPulseWidth >= 0.0);
  CATCH_CHECK(state.oscAPulseWidth <= 1.0);
  CATCH_CHECK(state.oscBPulseWidth >= 0.0);
  CATCH_CHECK(state.oscBPulseWidth <= 1.0);

  // Mixer levels in [0,1]
  CATCH_CHECK(state.mixOscA >= 0.0);
  CATCH_CHECK(state.mixOscA <= 1.0);
  CATCH_CHECK(state.mixOscB >= 0.0);
  CATCH_CHECK(state.mixOscB <= 1.0);
  CATCH_CHECK(state.mixNoise >= 0.0);
  CATCH_CHECK(state.mixNoise <= 1.0);

  // Filter
  CATCH_CHECK(state.filterCutoff >= 20.0);
  CATCH_CHECK(state.filterCutoff <= 20000.0);
  CATCH_CHECK(state.filterResonance >= 0.0);
  CATCH_CHECK(state.filterResonance <= 1.0);

  // Envelopes must be non-negative
  CATCH_CHECK(state.ampAttack >= 0.0);
  CATCH_CHECK(state.ampDecay >= 0.0);
  CATCH_CHECK(state.ampSustain >= 0.0);
  CATCH_CHECK(state.ampSustain <= 1.0);
  CATCH_CHECK(state.ampRelease >= 0.0);
  CATCH_CHECK(state.filterAttack >= 0.0);
  CATCH_CHECK(state.filterDecay >= 0.0);
  CATCH_CHECK(state.filterSustain >= 0.0);
  CATCH_CHECK(state.filterSustain <= 1.0);
  CATCH_CHECK(state.filterRelease >= 0.0);

  // LFO rate must be positive
  CATCH_CHECK(state.lfoRate > 0.0);

  // FX
  CATCH_CHECK(state.fxLimiterThreshold > 0.0);
  CATCH_CHECK(state.fxLimiterThreshold <= 1.0);
  CATCH_CHECK(state.fxDelayFeedback < 1.0); // Must be < 1 to avoid runaway
}

CATCH_TEST_CASE("SynthState serialization round-trip preserves all fields",
                "[SynthState][preset]") {
  using namespace PolySynthCore;

  // Create a state where every field has a distinct non-default value
  SynthState original;
  original.masterGain = 0.42;
  original.polyphony = 3;
  original.unison = true;
  original.unisonDetune = 0.33;
  original.glideTime = 0.07;

  original.oscAWaveform = 1;
  original.oscAFreq = 330.0;
  original.oscAPulseWidth = 0.4;
  original.oscASync = true;

  original.oscBWaveform = 2;
  original.oscBFreq = 550.0;
  original.oscBFineTune = 3.0;
  original.oscBPulseWidth = 0.7;
  original.oscBLoFreq = true;
  original.oscBKeyboardPoints = false;

  original.mixOscA = 0.4;
  original.mixOscB = 0.6;
  original.mixNoise = 0.15;

  original.filterCutoff = 8000.0;
  original.filterResonance = 0.6;
  original.filterEnvAmount = 0.4;
  original.filterModel = 2;
  original.filterKeyboardTrack = true;

  original.filterAttack = 0.03;
  original.filterDecay = 0.25;
  original.filterSustain = 0.6;
  original.filterRelease = 0.4;

  original.ampAttack = 0.04;
  original.ampDecay = 0.2;
  original.ampSustain = 0.7;
  original.ampRelease = 0.15;

  original.lfoShape = 2;
  original.lfoRate = 3.5;
  original.lfoDepth = 0.4;

  original.polyModOscBToFreqA = 0.15;
  original.polyModOscBToPWM = 0.25;
  original.polyModOscBToFilter = 0.35;
  original.polyModFilterEnvToFreqA = 0.45;
  original.polyModFilterEnvToPWM = 0.55;
  original.polyModFilterEnvToFilter = 0.65;

  original.fxChorusRate = 0.6;
  original.fxChorusDepth = 0.8;
  original.fxChorusMix = 0.4;
  original.fxDelayTime = 0.6;
  original.fxDelayFeedback = 0.55;
  original.fxDelayMix = 0.35;
  original.fxLimiterThreshold = 0.8;

  // Serialize and deserialize
  std::string json = PresetManager::Serialize(original);
  CATCH_REQUIRE(!json.empty());

  SynthState loaded;
  CATCH_REQUIRE(PresetManager::Deserialize(json, loaded));

  // Verify every field survived the round-trip
  CATCH_CHECK(loaded.masterGain == original.masterGain);
  CATCH_CHECK(loaded.polyphony == original.polyphony);
  CATCH_CHECK(loaded.unison == original.unison);
  CATCH_CHECK(loaded.unisonDetune == original.unisonDetune);
  CATCH_CHECK(loaded.glideTime == original.glideTime);

  CATCH_CHECK(loaded.oscAWaveform == original.oscAWaveform);
  CATCH_CHECK(loaded.oscAFreq == original.oscAFreq);
  CATCH_CHECK(loaded.oscAPulseWidth == original.oscAPulseWidth);
  CATCH_CHECK(loaded.oscASync == original.oscASync);

  CATCH_CHECK(loaded.oscBWaveform == original.oscBWaveform);
  CATCH_CHECK(loaded.oscBFreq == original.oscBFreq);
  CATCH_CHECK(loaded.oscBFineTune == original.oscBFineTune);
  CATCH_CHECK(loaded.oscBPulseWidth == original.oscBPulseWidth);
  CATCH_CHECK(loaded.oscBLoFreq == original.oscBLoFreq);
  CATCH_CHECK(loaded.oscBKeyboardPoints == original.oscBKeyboardPoints);

  CATCH_CHECK(loaded.mixOscA == original.mixOscA);
  CATCH_CHECK(loaded.mixOscB == original.mixOscB);
  CATCH_CHECK(loaded.mixNoise == original.mixNoise);

  CATCH_CHECK(loaded.filterCutoff == original.filterCutoff);
  CATCH_CHECK(loaded.filterResonance == original.filterResonance);
  CATCH_CHECK(loaded.filterEnvAmount == original.filterEnvAmount);
  CATCH_CHECK(loaded.filterModel == original.filterModel);
  CATCH_CHECK(loaded.filterKeyboardTrack == original.filterKeyboardTrack);

  CATCH_CHECK(loaded.filterAttack == original.filterAttack);
  CATCH_CHECK(loaded.filterDecay == original.filterDecay);
  CATCH_CHECK(loaded.filterSustain == original.filterSustain);
  CATCH_CHECK(loaded.filterRelease == original.filterRelease);

  CATCH_CHECK(loaded.ampAttack == original.ampAttack);
  CATCH_CHECK(loaded.ampDecay == original.ampDecay);
  CATCH_CHECK(loaded.ampSustain == original.ampSustain);
  CATCH_CHECK(loaded.ampRelease == original.ampRelease);

  CATCH_CHECK(loaded.lfoShape == original.lfoShape);
  CATCH_CHECK(loaded.lfoRate == original.lfoRate);
  CATCH_CHECK(loaded.lfoDepth == original.lfoDepth);

  CATCH_CHECK(loaded.polyModOscBToFreqA == original.polyModOscBToFreqA);
  CATCH_CHECK(loaded.polyModOscBToPWM == original.polyModOscBToPWM);
  CATCH_CHECK(loaded.polyModOscBToFilter == original.polyModOscBToFilter);
  CATCH_CHECK(loaded.polyModFilterEnvToFreqA ==
              original.polyModFilterEnvToFreqA);
  CATCH_CHECK(loaded.polyModFilterEnvToPWM == original.polyModFilterEnvToPWM);
  CATCH_CHECK(loaded.polyModFilterEnvToFilter ==
              original.polyModFilterEnvToFilter);

  CATCH_CHECK(loaded.fxChorusRate == original.fxChorusRate);
  CATCH_CHECK(loaded.fxChorusDepth == original.fxChorusDepth);
  CATCH_CHECK(loaded.fxChorusMix == original.fxChorusMix);
  CATCH_CHECK(loaded.fxDelayTime == original.fxDelayTime);
  CATCH_CHECK(loaded.fxDelayFeedback == original.fxDelayFeedback);
  CATCH_CHECK(loaded.fxDelayMix == original.fxDelayMix);
  CATCH_CHECK(loaded.fxLimiterThreshold == original.fxLimiterThreshold);
}
