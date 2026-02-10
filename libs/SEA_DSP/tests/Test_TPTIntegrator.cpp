#include "catch.hpp"
#include <sea_dsp/sea_tpt_integrator.h>
// NOTE: Only sea_tpt_integrator.h is included (after catch.hpp). This verifies
// the header is self-contained and brings all its own dependencies.
// If sea_tpt_integrator.h is missing any includes, this file will fail to compile.
#include <cmath>

TEST_CASE("TPTIntegrator Self-Contained Header", "[Filter][TPTIntegrator][SelfContained]") {
  sea::TPTIntegrator<double> integ;
  integ.Init(44100.0);
  integ.Prepare(1000.0);

  double g = integ.GetG();
  REQUIRE(g > 0.0);
  REQUIRE(g < 1.0);
}

TEST_CASE("TPTIntegrator Type Safety - Float vs Double", "[Filter][TPTIntegrator][TypeSafety]") {
  sea::TPTIntegrator<float> integF;
  sea::TPTIntegrator<double> integD;

  integF.Init(44100.0f);
  integD.Init(44100.0);

  integF.Prepare(1000.0f);
  integD.Prepare(1000.0);

  // Check gain calculation consistency
  float gF = integF.GetG();
  double gD = integD.GetG();

  REQUIRE(std::abs(static_cast<double>(gF) - gD) < 0.0001);

  // Process same input
  float outF = integF.Process(1.0f);
  double outD = integD.Process(1.0);

  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 0.0001);
}
