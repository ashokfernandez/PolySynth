#pragma once

#include <sea_dsp/sea_platform.h>
#include <stdint.h>
#include <type_traits>

namespace PolySynthCore {

// Fixed precision mode can be defined here if needed
#ifdef POLYSYNTH_USE_FLOAT
using sample_t = float;
#else
using sample_t = double;
#endif

static_assert(std::is_same_v<sample_t, sea::Real>,
              "sample_t and sea::Real must be the same type");

// Constants
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr int kMaxBlockSize = 4096;

} // namespace PolySynthCore
