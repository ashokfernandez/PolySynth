#pragma once

#include <stdint.h>

namespace PolySynthCore {

// Fixed precision mode can be defined here if needed
#ifdef POLYSYNTH_USE_FLOAT
using sample_t = float;
#else
using sample_t = double;
#endif

// Constants
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr int kMaxBlockSize = 4096;

} // namespace PolySynthCore
