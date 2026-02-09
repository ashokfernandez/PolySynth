#pragma once

#include "SynthState.h"
#include <string>

namespace PolySynthCore {

class PresetManager {
public:
  // Serialize state to a JSON string
  static std::string Serialize(const SynthState &state);

  // Deserialize JSON string into state. Returns true on success.
  static bool Deserialize(const std::string &jsonStr, SynthState &outState);

  // Save state to a file
  static bool SaveToFile(const SynthState &state, const std::string &path);

  // Load state from a file
  static bool LoadFromFile(const std::string &path, SynthState &outState);
};

} // namespace PolySynthCore
