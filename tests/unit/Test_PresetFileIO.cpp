#define CATCH_CONFIG_PREFIX_ALL
#include "PresetManager.h"
#include "SynthState.h"
#include "catch.hpp"
#include <cstdio>
#include <fstream>

// Test preset file I/O to verify save/load works as expected

CATCH_TEST_CASE("User preset file save and load round-trip", "[preset][file]") {
  using namespace PolySynthCore;

  const std::string testPath = "/tmp/polysynth_test_preset.json";

  // Create a unique state with non-default values
  SynthState original;
  original.masterGain = 0.42;
  original.ampAttack = 0.123;
  original.ampDecay = 0.234;
  original.ampSustain = 0.567;
  original.ampRelease = 0.789;
  original.filterCutoff = 1234.5;
  original.filterResonance = 0.65;
  original.oscAWaveform = 2;
  original.lfoShape = 3;
  original.lfoRate = 4.5;
  original.lfoDepth = 0.88;

  // Save to file
  bool saveSuccess = PresetManager::SaveToFile(original, testPath);
  CATCH_REQUIRE(saveSuccess);

  // Verify file exists
  std::ifstream checkFile(testPath);
  CATCH_REQUIRE(checkFile.good());
  checkFile.close();

  // Load from file
  SynthState loaded;
  bool loadSuccess = PresetManager::LoadFromFile(testPath, loaded);
  CATCH_REQUIRE(loadSuccess);

  // Verify all values match
  CATCH_CHECK(loaded.masterGain == original.masterGain);
  CATCH_CHECK(loaded.ampAttack == original.ampAttack);
  CATCH_CHECK(loaded.ampDecay == original.ampDecay);
  CATCH_CHECK(loaded.ampSustain == original.ampSustain);
  CATCH_CHECK(loaded.ampRelease == original.ampRelease);
  CATCH_CHECK(loaded.filterCutoff == original.filterCutoff);
  CATCH_CHECK(loaded.filterResonance == original.filterResonance);
  CATCH_CHECK(loaded.oscAWaveform == original.oscAWaveform);
  CATCH_CHECK(loaded.lfoShape == original.lfoShape);
  CATCH_CHECK(loaded.lfoRate == original.lfoRate);
  CATCH_CHECK(loaded.lfoDepth == original.lfoDepth);

  // Cleanup
  std::remove(testPath.c_str());
}

CATCH_TEST_CASE("Loading non-existent preset file fails gracefully",
                "[preset][file]") {
  using namespace PolySynthCore;

  SynthState state;
  bool success =
      PresetManager::LoadFromFile("/tmp/nonexistent_preset_file.json", state);
  CATCH_CHECK(!success);
}

CATCH_TEST_CASE("Preset file contains valid JSON structure", "[preset][file]") {
  using namespace PolySynthCore;

  const std::string testPath = "/tmp/polysynth_json_test.json";

  SynthState state;
  state.masterGain = 0.5;
  state.filterCutoff = 1000.0;

  PresetManager::SaveToFile(state, testPath);

  // Read file content
  std::ifstream file(testPath);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  // Verify it looks like JSON
  CATCH_CHECK(content.find("{") != std::string::npos);
  CATCH_CHECK(content.find("}") != std::string::npos);
  CATCH_CHECK(content.find("masterGain") != std::string::npos);
  CATCH_CHECK(content.find("filterCutoff") != std::string::npos);
  CATCH_CHECK(content.find("0.5") != std::string::npos);
  CATCH_CHECK(content.find("1000") != std::string::npos);

  // Cleanup
  std::remove(testPath.c_str());
}
