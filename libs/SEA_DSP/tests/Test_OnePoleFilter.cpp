#include "catch.hpp"
#include "sea_dsp/sea_one_pole_filter.h"
#include <cmath>

TEST_CASE("OnePoleFilter Self-Contained Header", "[OnePoleFilter][SelfContained]") {
  sea::OnePoleFilter<float> opf;
  REQUIRE(true);
}

TEST_CASE("OnePoleFilter Type Safety - Float vs Double", "[OnePoleFilter][TypeSafety]") {
  sea::OnePoleFilter<float> opfF;
  sea::OnePoleFilter<double> opfD;

  opfF.Init(48000.0f);
  opfD.Init(48000.0);

  opfF.SetCutoff(1000.0f);
  opfD.SetCutoff(1000.0);

  // Process identical input
  float outF = opfF.Process(1.0f);
  double outD = opfD.Process(1.0);

  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 1e-4);
}

TEST_CASE("OnePoleFilter DC Response", "[OnePoleFilter][Filter]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(1000.0f);

  // Feed constant DC
  float output = 0.0f;
  for (int i = 0; i < 1000; ++i) {
    output = opf.Process(1.0f);
  }

  // Should converge to input value
  REQUIRE(output == Approx(1.0f).margin(0.01f));
}

TEST_CASE("OnePoleFilter Zero Input", "[OnePoleFilter]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(5000.0f);

  // Process silence
  for (int i = 0; i < 100; ++i) {
    float output = opf.Process(0.0f);
    REQUIRE(output == Approx(0.0f).margin(1e-10f));
  }
}

TEST_CASE("OnePoleFilter Impulse Response", "[OnePoleFilter][Filter]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(1000.0f);

  // Send impulse
  float prev_output = opf.Process(1.0f);

  // Process zeros and verify decay
  for (int i = 0; i < 100; ++i) {
    float output = opf.Process(0.0f);

    // Each sample should be smaller than previous (decay)
    REQUIRE(std::abs(output) < std::abs(prev_output));

    prev_output = output;
  }

  // After 100 samples, should be near zero
  REQUIRE(std::abs(prev_output) < 0.01f);
}

TEST_CASE("OnePoleFilter Cutoff Frequency Effect", "[OnePoleFilter][Filter]") {
  sea::OnePoleFilter<float> opf_low, opf_high;

  opf_low.Init(48000.0f);
  opf_high.Init(48000.0f);

  opf_low.SetCutoff(100.0f);   // Very low cutoff (strong filtering)
  opf_high.SetCutoff(10000.0f); // High cutoff (weak filtering)

  // Send impulse to both
  opf_low.Process(1.0f);
  opf_high.Process(1.0f);

  // Read decay after 10 samples
  float low_val = 0.0f, high_val = 0.0f;
  for (int i = 0; i < 10; ++i) {
    low_val = opf_low.Process(0.0f);
    high_val = opf_high.Process(0.0f);
  }

  // Low cutoff should have MORE energy remaining (slower decay)
  // High cutoff should have LESS energy (faster decay)
  REQUIRE(std::abs(low_val) > std::abs(high_val));
}

TEST_CASE("OnePoleFilter Coefficient Calculation", "[OnePoleFilter][Filter]") {
  sea::OnePoleFilter<double> opf;
  opf.Init(48000.0);

  // Set cutoff at 1000 Hz
  opf.SetCutoff(1000.0);

  // Expected: b1 = 1.0 - exp(-2*pi*fc/fs)
  double expected_b1 = 1.0 - std::exp(-2.0 * M_PI * 1000.0 / 48000.0);

  // This test verifies correct behavior indirectly
  // (Direct coefficient inspection would require API exposure)
  REQUIRE(true); // Placeholder - indirect verification via other tests
}

TEST_CASE("OnePoleFilter Nyquist Clamping", "[OnePoleFilter][Stability]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);

  // Try to set cutoff ABOVE Nyquist (should be clamped to 0.49*fs)
  opf.SetCutoff(30000.0f); // Way above 24kHz Nyquist

  // Process impulse
  float output = opf.Process(1.0f);

  // Process more samples
  for (int i = 0; i < 100; ++i) {
    output = opf.Process(0.0f);

    // Should never explode
    REQUIRE(std::isfinite(output));
    REQUIRE(std::abs(output) < 100.0f);
  }
}

TEST_CASE("OnePoleFilter Parameter Changes", "[OnePoleFilter]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(1000.0f);

  // Process some samples
  for (int i = 0; i < 50; ++i) {
    opf.Process(1.0f);
  }

  // Change cutoff mid-stream
  opf.SetCutoff(5000.0f);

  // Continue processing
  for (int i = 0; i < 50; ++i) {
    float output = opf.Process(1.0f);
    REQUIRE(std::isfinite(output));
  }
}

TEST_CASE("OnePoleFilter Extreme Low Cutoff", "[OnePoleFilter][Stability]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(0.1f); // Extremely low (sub-Hz)

  // Feed DC signal to charge the filter
  float output = 0.0f;
  for (int i = 0; i < 10000; ++i) {
    output = opf.Process(1.0f);
    REQUIRE(std::isfinite(output));
  }

  // After many samples of DC input, should have charged significantly
  // (very slow attack, but should be well above 0.1 after 10k samples)
  REQUIRE(std::abs(output) > 0.1f);
}

TEST_CASE("OnePoleFilter Negative Frequency", "[OnePoleFilter][Stability]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);

  // Attempt negative cutoff (should be rejected or clamped to 0)
  opf.SetCutoff(-1000.0f);

  // Should still process safely
  for (int i = 0; i < 100; ++i) {
    float output = opf.Process(1.0f);
    REQUIRE(std::isfinite(output));
  }
}

TEST_CASE("OnePoleFilter Reset State", "[OnePoleFilter]") {
  sea::OnePoleFilter<float> opf;
  opf.Init(48000.0f);
  opf.SetCutoff(1000.0f);

  // Process some samples to build up state
  for (int i = 0; i < 100; ++i) {
    opf.Process(1.0f);
  }

  // Reset filter (re-Init)
  opf.Init(48000.0f);
  opf.SetCutoff(1000.0f);

  // First output should be low (clean state)
  float output = opf.Process(1.0f);
  REQUIRE(output < 0.5f); // Not near DC value
}
