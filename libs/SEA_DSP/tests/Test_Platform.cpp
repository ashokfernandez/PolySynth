#include <catch.hpp>
#include <sea_dsp/sea_platform.h>
#include <type_traits>

TEST_CASE("SEA_DSP Platform Types", "[sea_dsp][platform]") {
  // Verify sea::Real type based on build configuration
#ifdef SEA_PLATFORM_EMBEDDED
  REQUIRE(std::is_same_v<sea::Real, float>);
  REQUIRE(sizeof(sea::Real) == 4);
#else
  REQUIRE(std::is_same_v<sea::Real, double>);
  REQUIRE(sizeof(sea::Real) == 8);
#endif

  // Verify constants
  REQUIRE(sea::kPi > 3.14);
  REQUIRE(sea::kTwoPi > 6.28);
}
