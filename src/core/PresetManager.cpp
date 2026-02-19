#include "PresetManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace PolySynthCore {

// Helper macro to serialize a field
#define SERIALIZE(j, obj, field) j[#field] = obj.field
// Helper macro to deserialize a field with a default fallback (though we rely
// on state being init)
#define DESERIALIZE(j, obj, field)                                             \
  if (j.contains(#field))                                                      \
  obj.field = j.at(#field)

std::string PresetManager::Serialize(const SynthState &state) {
  json j;

  // Global
  SERIALIZE(j, state, masterGain);
  SERIALIZE(j, state, polyphony);
  SERIALIZE(j, state, unison);
  SERIALIZE(j, state, unisonDetune);
  SERIALIZE(j, state, glideTime);

  // Voice Allocation
  SERIALIZE(j, state, allocationMode);
  SERIALIZE(j, state, stealPriority);
  SERIALIZE(j, state, unisonCount);
  SERIALIZE(j, state, unisonSpread);
  SERIALIZE(j, state, stereoSpread);

  // Osc A
  SERIALIZE(j, state, oscAWaveform);
  SERIALIZE(j, state, oscAFreq);
  SERIALIZE(j, state, oscAPulseWidth);
  SERIALIZE(j, state, oscASync);

  // Osc B
  SERIALIZE(j, state, oscBWaveform);
  SERIALIZE(j, state, oscBFreq);
  SERIALIZE(j, state, oscBFineTune);
  SERIALIZE(j, state, oscBPulseWidth);
  SERIALIZE(j, state, oscBLoFreq);
  SERIALIZE(j, state, oscBKeyboardPoints);

  // Mixer
  SERIALIZE(j, state, mixOscA);
  SERIALIZE(j, state, mixOscB);
  SERIALIZE(j, state, mixNoise);

  // Filter
  SERIALIZE(j, state, filterCutoff);
  SERIALIZE(j, state, filterResonance);
  SERIALIZE(j, state, filterEnvAmount);
  SERIALIZE(j, state, filterModel);
  SERIALIZE(j, state, filterKeyboardTrack);

  // Filter Env
  SERIALIZE(j, state, filterAttack);
  SERIALIZE(j, state, filterDecay);
  SERIALIZE(j, state, filterSustain);
  SERIALIZE(j, state, filterRelease);

  // Amp Env
  SERIALIZE(j, state, ampAttack);
  SERIALIZE(j, state, ampDecay);
  SERIALIZE(j, state, ampSustain);
  SERIALIZE(j, state, ampRelease);

  // LFO
  SERIALIZE(j, state, lfoShape);
  SERIALIZE(j, state, lfoRate);
  SERIALIZE(j, state, lfoDepth);

  // Poly Mod
  SERIALIZE(j, state, polyModOscBToFreqA);
  SERIALIZE(j, state, polyModOscBToPWM);
  SERIALIZE(j, state, polyModOscBToFilter);
  SERIALIZE(j, state, polyModFilterEnvToFreqA);
  SERIALIZE(j, state, polyModFilterEnvToPWM);
  SERIALIZE(j, state, polyModFilterEnvToFilter);

  // FX
  SERIALIZE(j, state, fxChorusRate);
  SERIALIZE(j, state, fxChorusDepth);
  SERIALIZE(j, state, fxChorusMix);
  SERIALIZE(j, state, fxDelayTime);
  SERIALIZE(j, state, fxDelayFeedback);
  SERIALIZE(j, state, fxDelayMix);
  SERIALIZE(j, state, fxLimiterThreshold);

  return j.dump(2); // Pretty print with 2 space indent
}

bool PresetManager::Deserialize(const std::string &jsonStr,
                                SynthState &outState) {
  try {
    auto j = json::parse(jsonStr);

    // Global
    DESERIALIZE(j, outState, masterGain);
    DESERIALIZE(j, outState, polyphony);
    DESERIALIZE(j, outState, unison);
    DESERIALIZE(j, outState, unisonDetune);
    DESERIALIZE(j, outState, glideTime);

    // Voice Allocation
    DESERIALIZE(j, outState, allocationMode);
    DESERIALIZE(j, outState, stealPriority);
    DESERIALIZE(j, outState, unisonCount);
    DESERIALIZE(j, outState, unisonSpread);
    DESERIALIZE(j, outState, stereoSpread);

    // Osc A
    DESERIALIZE(j, outState, oscAWaveform);
    DESERIALIZE(j, outState, oscAFreq);
    DESERIALIZE(j, outState, oscAPulseWidth);
    DESERIALIZE(j, outState, oscASync);

    // Osc B
    DESERIALIZE(j, outState, oscBWaveform);
    DESERIALIZE(j, outState, oscBFreq);
    DESERIALIZE(j, outState, oscBFineTune);
    DESERIALIZE(j, outState, oscBPulseWidth);
    DESERIALIZE(j, outState, oscBLoFreq);
    DESERIALIZE(j, outState, oscBKeyboardPoints);

    // Mixer
    DESERIALIZE(j, outState, mixOscA);
    DESERIALIZE(j, outState, mixOscB);
    DESERIALIZE(j, outState, mixNoise);

    // Filter
    DESERIALIZE(j, outState, filterCutoff);
    DESERIALIZE(j, outState, filterResonance);
    DESERIALIZE(j, outState, filterEnvAmount);
    DESERIALIZE(j, outState, filterModel);
    DESERIALIZE(j, outState, filterKeyboardTrack);

    // Filter Env
    DESERIALIZE(j, outState, filterAttack);
    DESERIALIZE(j, outState, filterDecay);
    DESERIALIZE(j, outState, filterSustain);
    DESERIALIZE(j, outState, filterRelease);

    // Amp Env
    DESERIALIZE(j, outState, ampAttack);
    DESERIALIZE(j, outState, ampDecay);
    DESERIALIZE(j, outState, ampSustain);
    DESERIALIZE(j, outState, ampRelease);

    // LFO
    DESERIALIZE(j, outState, lfoShape);
    DESERIALIZE(j, outState, lfoRate);
    DESERIALIZE(j, outState, lfoDepth);

    // Poly Mod
    DESERIALIZE(j, outState, polyModOscBToFreqA);
    DESERIALIZE(j, outState, polyModOscBToPWM);
    DESERIALIZE(j, outState, polyModOscBToFilter);
    DESERIALIZE(j, outState, polyModFilterEnvToFreqA);
    DESERIALIZE(j, outState, polyModFilterEnvToPWM);
    DESERIALIZE(j, outState, polyModFilterEnvToFilter);

    // FX
    DESERIALIZE(j, outState, fxChorusRate);
    DESERIALIZE(j, outState, fxChorusDepth);
    DESERIALIZE(j, outState, fxChorusMix);
    DESERIALIZE(j, outState, fxDelayTime);
    DESERIALIZE(j, outState, fxDelayFeedback);
    DESERIALIZE(j, outState, fxDelayMix);
    DESERIALIZE(j, outState, fxLimiterThreshold);

    return true;
  } catch (const std::exception & /*e*/) {
    // TODO: Logging
    return false;
  }
}

bool PresetManager::SaveToFile(const SynthState &state,
                               const std::string &path) {
  std::ofstream file(path);
  if (!file.is_open())
    return false;
  file << Serialize(state);
  return true;
}

bool PresetManager::LoadFromFile(const std::string &path,
                                 SynthState &outState) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;
  std::string str((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
  return Deserialize(str, outState);
}

} // namespace PolySynthCore
