#define CATCH_CONFIG_PREFIX_ALL
#include "../../src/core/Engine.h"
#include "catch.hpp"

CATCH_TEST_CASE("Engine can be instantiated", "[Engine]") {
  PolySynthCore::Engine engine;
  engine.Init(48000.0);

  CATCH_SECTION("Process produces silence by default") {
    PolySynthCore::sample_t *outputs[2];
    PolySynthCore::sample_t left[32];
    PolySynthCore::sample_t right[32];
    outputs[0] = left;
    outputs[1] = right;

    // Fill with noise to ensure it's cleared
    for (int i = 0; i < 32; i++) {
      left[i] = 1.0;
      right[i] = 1.0;
    }

    engine.Process(nullptr, outputs, 32, 2);

    for (int i = 0; i < 32; i++) {
      CATCH_REQUIRE(left[i] == Approx(0.0));
      CATCH_REQUIRE(right[i] == Approx(0.0));
    }
  }
}
