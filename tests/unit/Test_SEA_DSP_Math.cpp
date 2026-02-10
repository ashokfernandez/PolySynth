#include <catch.hpp>
#include <sea_dsp/sea_math.h>

TEST_CASE("SEA_DSP Math Functions", "[sea_dsp][math]") {
  // Check key constants
  REQUIRE(sea::kPi ==
          Approx(3.1415926535897932)); // sea::Real might be float or double
  REQUIRE(sea::kTwoPi == Approx(6.2831853071795864));

  // Check basic functions (within reason for floating point)
  REQUIRE(sea::Math::Sin(0.0) == Approx(0.0));
  REQUIRE(sea::Math::Sin(sea::kPi / 2.0) == Approx(1.0));
  REQUIRE(sea::Math::Cos(0.0) == Approx(1.0));
  REQUIRE(sea::Math::Cos(sea::kPi) == Approx(-1.0));

  REQUIRE(sea::Math::Tan(0.0) == Approx(0.0));
  REQUIRE(sea::Math::Tanh(0.0) == Approx(0.0));

  REQUIRE(sea::Math::Exp(0.0) == Approx(1.0));
  REQUIRE(sea::Math::Pow(2.0, 3.0) == Approx(8.0));

  REQUIRE(sea::Math::Abs(-5.0) == Approx(5.0));
  REQUIRE(sea::Math::Abs(5.0) == Approx(5.0));

  REQUIRE(sea::Math::Clamp(1.5, 0.0, 1.0) == Approx(1.0));
  REQUIRE(sea::Math::Clamp(-0.5, 0.0, 1.0) == Approx(0.0));
  REQUIRE(sea::Math::Clamp(0.5, 0.0, 1.0) == Approx(0.5));

  REQUIRE(sea::Math::Sqrt(4.0) == Approx(2.0));
  REQUIRE(sea::Math::Log(sea::Math::Exp(1.0)) == Approx(1.0));

  REQUIRE(sea::Math::Ceil(1.2) == Approx(2.0));
  REQUIRE(sea::Math::Floor(1.8) == Approx(1.0));
}
