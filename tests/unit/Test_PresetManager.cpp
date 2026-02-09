#define CATCH_CONFIG_PREFIX_ALL
#include "PresetManager.h"
#include "SynthState.h"
#include "catch.hpp"

CATCH_TEST_CASE("PresetManager serialization round-trip", "[preset]") {
  using namespace PolySynthCore;

  // Create a state with non-default values
  SynthState original;
  original.masterGain = 0.75;
  original.polyphony = 6;
  original.unison = true;
  original.unisonDetune = 0.2;
  original.glideTime = 0.05;

  original.oscAWaveform = 1; // Square
  original.oscAFreq = 220.0;
  original.oscAPulseWidth = 0.3;
  original.oscASync = true;

  original.oscBWaveform = 2; // Triangle
  original.oscBFreq = 330.0;
  original.oscBFineTune = 7.0;
  original.oscBPulseWidth = 0.6;
  original.oscBLoFreq = true;
  original.oscBKeyboardPoints = false;

  original.mixOscA = 0.6;
  original.mixOscB = 0.3;
  original.mixNoise = 0.1;

  original.filterCutoff = 5000.0;
  original.filterResonance = 0.8;
  original.filterEnvAmount = 0.5;
  original.filterKeyboardTrack = true;

  original.filterAttack = 0.05;
  original.filterDecay = 0.2;
  original.filterSustain = 0.7;
  original.filterRelease = 0.3;

  original.ampAttack = 0.02;
  original.ampDecay = 0.15;
  original.ampSustain = 0.9;
  original.ampRelease = 0.25;

  original.lfoShape = 3; // Saw
  original.lfoRate = 5.0;
  original.lfoDepth = 0.6;

  original.polyModOscBToFreqA = 0.1;
  original.polyModOscBToPWM = 0.2;
  original.polyModOscBToFilter = 0.3;
  original.polyModFilterEnvToFreqA = 0.4;
  original.polyModFilterEnvToPWM = 0.45;
  original.polyModFilterEnvToFilter = 0.5;

  // Serialize
  std::string jsonStr = PresetManager::Serialize(original);
  CATCH_REQUIRE(!jsonStr.empty());

  // Deserialize
  SynthState loaded;
  bool success = PresetManager::Deserialize(jsonStr, loaded);
  CATCH_REQUIRE(success);

  // Verify all fields match
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
  CATCH_CHECK(loaded.polyModFilterEnvToPWM ==
              original.polyModFilterEnvToPWM);
  CATCH_CHECK(loaded.polyModFilterEnvToFilter ==
              original.polyModFilterEnvToFilter);
}

CATCH_TEST_CASE("PresetManager handles invalid JSON gracefully", "[preset]") {
  using namespace PolySynthCore;

  SynthState state;
  std::string badJson = "{this is not valid json}";
  bool success = PresetManager::Deserialize(badJson, state);
  CATCH_REQUIRE(!success);
}
