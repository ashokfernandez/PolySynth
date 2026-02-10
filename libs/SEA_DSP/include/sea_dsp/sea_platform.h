#pragma once
#include <cmath>

namespace sea {

#ifdef SEA_PLATFORM_EMBEDDED
using Real = float;
static constexpr Real kPi = 3.14159265358979323846f;
static constexpr Real kTwoPi = kPi * 2.0f;
#else
using Real = double;
static constexpr Real kPi = 3.14159265358979323846;
static constexpr Real kTwoPi = kPi * 2.0;
#endif

#if defined(_MSC_VER)
#define SEA_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define SEA_INLINE inline __attribute__((always_inline))
#else
#define SEA_INLINE inline
#endif

} // namespace sea
