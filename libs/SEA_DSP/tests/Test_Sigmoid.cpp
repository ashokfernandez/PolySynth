#include "catch.hpp"
#include "sea_dsp/sea_sigmoid.h"
#include <cmath>

TEST_CASE("Sigmoid Self-Contained Header", "[Sigmoid][SelfContained]") {
  float result = sea::Sigmoid<float>::Tanh(0.5f);
  REQUIRE(std::isfinite(result));
}

TEST_CASE("Sigmoid Type Safety - Float vs Double", "[Sigmoid][TypeSafety]") {
  float inputF = 1.0f;
  double inputD = 1.0;

  float tanhF = sea::Sigmoid<float>::Tanh(inputF);
  double tanhD = sea::Sigmoid<double>::Tanh(inputD);

  REQUIRE(std::abs(static_cast<double>(tanhF) - tanhD) < 1e-4);

  float cubicF = sea::Sigmoid<float>::SoftClipCubic(inputF);
  double cubicD = sea::Sigmoid<double>::SoftClipCubic(inputD);

  REQUIRE(std::abs(static_cast<double>(cubicF) - cubicD) < 1e-4);
}

TEST_CASE("Sigmoid Tanh Unity Gain at Origin", "[Sigmoid][Saturation]") {
  // For small inputs, tanh(x) ≈ x
  float input = 0.1f;
  float output = sea::Sigmoid<float>::Tanh(input);

  // Should be very close to input (linear region)
  REQUIRE(output == Approx(input).margin(0.01f));
}

TEST_CASE("Sigmoid Tanh Saturation", "[Sigmoid][Saturation]") {
  float large_positive = 10.0f;
  float large_negative = -10.0f;

  float out_pos = sea::Sigmoid<float>::Tanh(large_positive);
  float out_neg = sea::Sigmoid<float>::Tanh(large_negative);

  // Should approach +1 and -1
  REQUIRE(out_pos == Approx(1.0f).margin(0.01f));
  REQUIRE(out_neg == Approx(-1.0f).margin(0.01f));

  // Should never exceed bounds
  REQUIRE(std::abs(out_pos) <= 1.0f);
  REQUIRE(std::abs(out_neg) <= 1.0f);
}

TEST_CASE("Sigmoid Tanh Symmetry", "[Sigmoid][Saturation]") {
  float inputs[] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f};

  for (float x : inputs) {
    float pos = sea::Sigmoid<float>::Tanh(x);
    float neg = sea::Sigmoid<float>::Tanh(-x);

    REQUIRE(pos == Approx(-neg).margin(1e-6f));
  }
}

TEST_CASE("Sigmoid SoftClipCubic Unity Gain", "[Sigmoid][Saturation]") {
  float input = 0.1f;
  float output = sea::Sigmoid<float>::SoftClipCubic(input);

  // In linear region, cubic approximation: f(x) ≈ x
  REQUIRE(output == Approx(input).margin(0.01f));
}

TEST_CASE("Sigmoid SoftClipCubic Saturation", "[Sigmoid][Saturation]") {
  float large_input = 3.0f;
  float output = sea::Sigmoid<float>::SoftClipCubic(large_input);

  // Should be compressed (output < input)
  REQUIRE(std::abs(output) < std::abs(large_input));

  // Should still be finite and bounded
  REQUIRE(std::isfinite(output));
  REQUIRE(std::abs(output) < 2.0f); // Reasonable saturation limit
}

TEST_CASE("Sigmoid SoftClipCubic Symmetry", "[Sigmoid][Saturation]") {
  float inputs[] = {0.1f, 0.5f, 1.0f, 1.5f, 2.0f};

  for (float x : inputs) {
    float pos = sea::Sigmoid<float>::SoftClipCubic(x);
    float neg = sea::Sigmoid<float>::SoftClipCubic(-x);

    REQUIRE(pos == Approx(-neg).margin(1e-6f));
  }
}

TEST_CASE("Sigmoid SoftClipCubic Formula Verification", "[Sigmoid][Saturation]") {
  float x = 1.0f;
  float expected = x - (x * x * x) / 3.0f; // 1.0 - 1.0/3.0 = 0.666...

  float output = sea::Sigmoid<float>::SoftClipCubic(x);

  REQUIRE(output == Approx(expected).margin(0.001f));
}

TEST_CASE("Sigmoid SoftClipCubic Clamping", "[Sigmoid][Saturation]") {
  // Spec says: "for x in [-1.5, 1.5], clamped outside"
  float extreme_pos = 5.0f;
  float extreme_neg = -5.0f;

  float out_pos = sea::Sigmoid<float>::SoftClipCubic(extreme_pos);
  float out_neg = sea::Sigmoid<float>::SoftClipCubic(extreme_neg);

  // Should be clamped/bounded (exact value depends on implementation)
  REQUIRE(std::isfinite(out_pos));
  REQUIRE(std::isfinite(out_neg));
  REQUIRE(std::abs(out_pos) < 5.0f); // Compressed
  REQUIRE(std::abs(out_neg) < 5.0f);
}

TEST_CASE("Sigmoid Tanh vs Cubic Quality", "[Sigmoid][Saturation]") {
  // For moderate inputs, both should saturate similarly
  float input = 2.0f;

  float tanh_out = sea::Sigmoid<float>::Tanh(input);
  float cubic_out = sea::Sigmoid<float>::SoftClipCubic(input);

  // Both should compress the signal
  REQUIRE(std::abs(tanh_out) < std::abs(input));
  REQUIRE(std::abs(cubic_out) < std::abs(input));

  // Tanh should be smoother (closer to 1.0 for input=2.0)
  // This is subjective but std::tanh(2.0) ≈ 0.964
  REQUIRE(std::abs(tanh_out) > 0.9f);
}

TEST_CASE("Sigmoid Zero Input", "[Sigmoid]") {
  float zero = 0.0f;

  float tanh_out = sea::Sigmoid<float>::Tanh(zero);
  float cubic_out = sea::Sigmoid<float>::SoftClipCubic(zero);

  REQUIRE(tanh_out == Approx(0.0f).margin(1e-10f));
  REQUIRE(cubic_out == Approx(0.0f).margin(1e-10f));
}
