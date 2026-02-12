#include "catch.hpp"
#include "sea_dsp/sea_delay_line.h"
#include <cmath>

TEST_CASE("DelayLine Self-Contained Header", "[DelayLine][SelfContained]") {
  sea::DelayLine<float> dl;
  REQUIRE(true); // Compilation is the test
}

TEST_CASE("DelayLine Type Safety - Float vs Double", "[DelayLine][TypeSafety]") {
  sea::DelayLine<float> dlF;
  sea::DelayLine<double> dlD;

  dlF.Init(100);  // Managed mode
  dlD.Init(100);

  // Push identical samples
  dlF.Push(0.5f);
  dlD.Push(0.5);

  // Read zero delay (should return last pushed sample)
  float outF = dlF.Read(0.0f);
  double outD = dlD.Read(0.0);

  REQUIRE(std::abs(static_cast<double>(outF) - outD) < 1e-6);
}

TEST_CASE("DelayLine Managed Mode Initialization", "[DelayLine][Memory]") {
  sea::DelayLine<float> dl;

  // Initialize with capacity
  dl.Init(512);

  // Should accept pushes without crashing
  for (int i = 0; i < 1000; ++i) {
    dl.Push(static_cast<float>(i));
  }

  // Should be able to read
  float val = dl.Read(0.0f);
  REQUIRE(std::isfinite(val));
}

TEST_CASE("DelayLine Unmanaged Mode - External Buffer", "[DelayLine][Memory][Embedded]") {
  float external_buffer[128];

  // Zero out buffer
  for (int i = 0; i < 128; ++i) {
    external_buffer[i] = 0.0f;
  }

  sea::DelayLine<float> dl;
  dl.Init(external_buffer, 128);

  // Push a known pattern
  dl.Push(1.0f);
  dl.Push(2.0f);
  dl.Push(3.0f);

  // Verify writes went directly to external buffer
  bool found_writes = false;
  for (int i = 0; i < 128; ++i) {
    if (external_buffer[i] != 0.0f) {
      found_writes = true;
      break;
    }
  }
  REQUIRE(found_writes);
}

TEST_CASE("DelayLine Zero Delay Read", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(100);

  dl.Push(42.0f);
  float result = dl.Read(0.0f);

  REQUIRE(result == Approx(42.0f));

  dl.Push(99.0f);
  result = dl.Read(0.0f);

  REQUIRE(result == Approx(99.0f));
}

TEST_CASE("DelayLine Integer Delay Read", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(10);

  // Fill with unique values
  for (int i = 0; i < 10; ++i) {
    dl.Push(static_cast<float>(i));
  }

  // Read 5 samples back (should be value 4, since current is 9)
  float result = dl.Read(5.0f);
  REQUIRE(result == Approx(4.0f));

  // Read 9 samples back
  result = dl.Read(9.0f);
  REQUIRE(result == Approx(0.0f));
}

TEST_CASE("DelayLine Fractional Delay Interpolation", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(10);

  // Create simple ramp for easy interpolation verification
  dl.Push(0.0f);
  dl.Push(1.0f);
  dl.Push(2.0f);

  // Read at delay 0.5 (halfway between last two samples)
  // Should interpolate between 2.0 (current) and 1.0 (prev)
  float result = dl.Read(0.5f);

  // Linear interpolation: (2.0 + 1.0) / 2 = 1.5
  REQUIRE(result == Approx(1.5f).margin(0.001f));
}

TEST_CASE("DelayLine Fractional Delay - Edge Values", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(10);

  dl.Push(-1.0f);
  dl.Push(1.0f);

  // Quarter-way interpolation
  float result = dl.Read(0.25f);

  // Expected: 0.75 * 1.0 + 0.25 * (-1.0) = 0.5
  REQUIRE(result == Approx(0.5f).margin(0.001f));

  // Three-quarter interpolation
  result = dl.Read(0.75f);

  // Expected: 0.25 * 1.0 + 0.75 * (-1.0) = -0.5
  REQUIRE(result == Approx(-0.5f).margin(0.001f));
}

TEST_CASE("DelayLine Circular Buffer Wraparound", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(8); // Small buffer to force wrapping quickly

  // Push more samples than capacity
  for (int i = 0; i < 20; ++i) {
    dl.Push(static_cast<float>(i));
  }

  // Read should still work (most recent is 19)
  float result = dl.Read(0.0f);
  REQUIRE(result == Approx(19.0f));

  // Read oldest available (should be 19-7=12)
  result = dl.Read(7.0f);
  REQUIRE(result == Approx(12.0f));
}

TEST_CASE("DelayLine Read Pointer Wraparound", "[DelayLine][Stability]") {
  sea::DelayLine<float> dl;
  dl.Init(8);

  // Fill buffer
  for (int i = 0; i < 8; ++i) {
    dl.Push(static_cast<float>(i));
  }

  // Now push 2 more (write_head is at index 1, wrapping back)
  dl.Push(8.0f);  // write_head = 0
  dl.Push(9.0f);  // write_head = 1

  // Try to read 7 samples back from write_head=1
  // This should cross the buffer boundary correctly
  float result = dl.Read(7.0f);

  REQUIRE(std::isfinite(result));

  // More importantly: ensure no discontinuities
  float prev = dl.Read(6.5f);
  float next = dl.Read(6.0f);

  // These should be close (smooth interpolation across boundary)
  REQUIRE(std::abs(prev - next) < 5.0f); // No huge jumps
}

TEST_CASE("DelayLine Maximum Delay Read", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(10);

  // Fill completely
  for (int i = 0; i < 10; ++i) {
    dl.Push(static_cast<float>(i));
  }

  // Read oldest sample (delay = size-1)
  float result = dl.Read(9.0f);
  REQUIRE(result == Approx(0.0f));

  // Try to read beyond capacity (should clamp or wrap safely)
  result = dl.Read(15.0f);
  REQUIRE(std::isfinite(result)); // Should not crash or return NaN
}

TEST_CASE("DelayLine Silence Produces Silence", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(100);

  // Push silence
  for (int i = 0; i < 200; ++i) {
    dl.Push(0.0f);
  }

  // Read at various delays
  for (float delay = 0.0f; delay < 50.0f; delay += 5.5f) {
    float result = dl.Read(delay);
    REQUIRE(result == Approx(0.0f).margin(1e-10f));
  }
}

TEST_CASE("DelayLine State Persistence", "[DelayLine]") {
  sea::DelayLine<float> dl;
  dl.Init(50);

  // Push impulse
  dl.Push(1.0f);

  // Push zeros
  for (int i = 0; i < 10; ++i) {
    dl.Push(0.0f);
  }

  // Impulse should still be readable at delay 10
  float result = dl.Read(10.0f);
  REQUIRE(result == Approx(1.0f).margin(0.01f));

  // Continue pushing zeros (39 more to reach total of 50, not 51)
  for (int i = 0; i < 39; ++i) {
    dl.Push(0.0f);
  }

  // Impulse should still be at delay 49 (max readable before wraparound)
  result = dl.Read(49.0f);
  REQUIRE(result == Approx(1.0f).margin(0.01f));
}
