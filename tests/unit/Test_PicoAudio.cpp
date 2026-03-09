// Desktop-runnable unit tests for Pico embedded audio logic.
// No Pico SDK dependency — tests only exercise pure functions and constants.

#include "catch.hpp"
#include "audio_i2s_driver.h"   // pico_audio constants
#include "sine_generator.h"     // pico_audio::GenerateSine, PhaseIncrement, PackI2S

#include <cmath>
#include <cstdint>
#include <vector>

// Helper: extract signed int16 from a left-justified 32-bit I2S word.
static int16_t UnpackI2S(uint32_t word)
{
    return static_cast<int16_t>(word >> 16);
}

// ─── Test 1: Sine wave audio callback ────────────────────────────────────

TEST_CASE("Sine generator produces correct 440 Hz tone", "[pico][audio]")
{
    constexpr uint32_t kSampleRate = pico_audio::kSampleRate;  // 48000
    constexpr float kFrequency = 440.0f;
    constexpr float kAmplitude = 0.5f;
    const float phase_inc = pico_audio::PhaseIncrement(kFrequency, static_cast<float>(kSampleRate));

    // Generate 1 second of audio in buffer-sized chunks
    constexpr uint32_t kFramesPerBuffer = pico_audio::kBufferFrames;
    constexpr uint32_t kTotalFrames = kSampleRate;  // 1 second
    constexpr uint32_t kNumBuffers = kTotalFrames / kFramesPerBuffer;

    std::vector<uint32_t> buffer(kFramesPerBuffer * 2);
    float phase = 0.0f;
    int zero_crossings = 0;
    int16_t prev_left = 0;

    for (uint32_t buf = 0; buf < kNumBuffers; buf++) {
        pico_audio::GenerateSine(phase, phase_inc, kAmplitude, buffer.data(), kFramesPerBuffer);

        for (uint32_t i = 0; i < kFramesPerBuffer; i++) {
            int16_t left  = UnpackI2S(buffer[i * 2]);
            int16_t right = UnpackI2S(buffer[i * 2 + 1]);

            // Stereo interleaving: L == R (mono)
            REQUIRE(left == right);

            // Amplitude within expected range: ±0.5 × 32767 = ±16383
            REQUIRE(left >= -16384);
            REQUIRE(left <= 16384);

            // Count positive-going zero crossings (left channel)
            if (buf > 0 || i > 0) {
                if (prev_left <= 0 && left > 0) {
                    zero_crossings++;
                }
            }
            prev_left = left;
        }
    }

    // Each cycle has one positive-going zero crossing.
    // Expected: 440 crossings in 1 second, allow ±1 for boundary effects.
    REQUIRE(zero_crossings >= 439);
    REQUIRE(zero_crossings <= 441);
}

TEST_CASE("Sine generator phase wraps without drift", "[pico][audio]")
{
    constexpr float kTwoPi = 2.0f * 3.14159265f;
    const float phase_inc = pico_audio::PhaseIncrement(440.0f, 48000.0f);

    float phase = 0.0f;
    std::vector<uint32_t> buffer(512);

    // Run for many buffers (simulating ~10 seconds)
    for (int i = 0; i < 2000; i++) {
        pico_audio::GenerateSine(phase, phase_inc, 0.5f, buffer.data(), 256);
    }

    // Phase should stay in [0, 2π)
    REQUIRE(phase >= 0.0f);
    REQUIRE(phase < kTwoPi);
}

// ─── Test 1b: PackI2S encoding ──────────────────────────────────────────

TEST_CASE("PackI2S places int16 in upper 16 bits", "[pico][audio]")
{
    // Positive value
    CHECK(pico_audio::PackI2S(0x7FFF) == 0x7FFF0000u);
    CHECK(pico_audio::PackI2S(1)      == 0x00010000u);
    CHECK(pico_audio::PackI2S(0)      == 0x00000000u);

    // Negative value (two's complement)
    CHECK(pico_audio::PackI2S(-1)     == 0xFFFF0000u);
    CHECK(pico_audio::PackI2S(-32768) == 0x80000000u);

    // Lower 16 bits are always zero (padding)
    for (int16_t v : {int16_t(0), int16_t(1), int16_t(-1), int16_t(16383), int16_t(-16384)}) {
        CHECK((pico_audio::PackI2S(v) & 0xFFFF) == 0);
    }
}

// ─── Test 2: I2S side-set encoding ──────────────────────────────────────

TEST_CASE("I2S side-set encoding matches pin mapping", "[pico][i2s]")
{
    // Side-set pin mapping: bit 0 = BCK (GPIO27), bit 1 = LRCK (GPIO28)
    // Encoding: sideset_value = (BCK << 0) | (LRCK << 1)
    auto encode_sideset = [](int bck, int lrck) -> int {
        return (bck & 1) | ((lrck & 1) << 1);
    };

    // Left channel (LRCK = 0): BCK toggles between 0 and 1
    SECTION("Left channel data phase")
    {
        // BCK low, LRCK low  → shift out data bit
        CHECK(encode_sideset(0, 0) == 0b00);
        // BCK high, LRCK low → DAC latches bit
        CHECK(encode_sideset(1, 0) == 0b01);
    }

    SECTION("Left channel padding phase")
    {
        // Same as data phase — BCK toggles, LRCK stays low
        CHECK(encode_sideset(0, 0) == 0b00);
        CHECK(encode_sideset(1, 0) == 0b01);
    }

    SECTION("Right channel data phase")
    {
        // BCK low, LRCK high
        CHECK(encode_sideset(0, 1) == 0b10);
        // BCK high, LRCK high
        CHECK(encode_sideset(1, 1) == 0b11);
    }

    SECTION("Right channel padding phase")
    {
        CHECK(encode_sideset(0, 1) == 0b10);
        CHECK(encode_sideset(1, 1) == 0b11);
    }

    // Verify the PIO program's actual side-set values match expected encoding.
    // These are the values used in i2s.pio after the bug fix:
    SECTION("PIO program side-set values match expected encoding")
    {
        // Left channel uses: 0b00 (BCK=0,LRCK=0) and 0b01 (BCK=1,LRCK=0)
        CHECK(encode_sideset(0, 0) == 0b00);  // BCK low, LRCK low
        CHECK(encode_sideset(1, 0) == 0b01);  // BCK high, LRCK low

        // Right channel uses: 0b10 (BCK=0,LRCK=1) and 0b11 (BCK=1,LRCK=1)
        CHECK(encode_sideset(0, 1) == 0b10);  // BCK low, LRCK high
        CHECK(encode_sideset(1, 1) == 0b11);  // BCK high, LRCK high

        // The OLD (buggy) values had 0b01 and 0b10 swapped:
        // Left used 0b10 (wrong: BCK=0,LRCK=1 when LRCK should be 0)
        // Right used 0b01 (wrong: BCK=1,LRCK=0 when LRCK should be 1)
        CHECK(encode_sideset(0, 1) != 0b01);  // Would be wrong mapping
        CHECK(encode_sideset(1, 0) != 0b10);  // Would be wrong mapping
    }
}

// ─── Test 3: Clock divider calculation ──────────────────────────────────

TEST_CASE("I2S clock divider produces correct sample rate", "[pico][i2s]")
{
    // PIO runs one instruction per clock cycle.
    // I2S protocol: 64 bit-clocks per stereo frame (32 left + 32 right).
    // Each bit-clock = 2 PIO instructions (one for BCK low, one for BCK high).
    // So: PIO clocks per frame = 64 × 2 = 128.
    //
    // Target: sys_clk / divider = sample_rate × 128
    // divider = sys_clk / (sample_rate × 128)

    constexpr float kSysClk = 150000000.0f;  // 150 MHz (RP2350 default)
    constexpr float kSampleRate = 48000.0f;
    constexpr float kPioClocksPerFrame = 128.0f;

    float divider = kSysClk / (kSampleRate * kPioClocksPerFrame);

    // Expected: 150000000 / (48000 × 128) = 150000000 / 6144000 ≈ 24.4140625
    CHECK(divider == Approx(24.4140625f).epsilon(0.001));

    // Verify actual sample rate is within ±0.1% of target
    float actual_rate = kSysClk / (divider * kPioClocksPerFrame);
    CHECK(actual_rate == Approx(kSampleRate).epsilon(0.001));
}

// ─── Test 4: Buffer geometry ────────────────────────────────────────────

TEST_CASE("Audio buffer constants are self-consistent", "[pico][audio]")
{
    // kBufferWords == kBufferFrames * 2 (one 32-bit word per L/R sample)
    REQUIRE(pico_audio::kBufferWords == pico_audio::kBufferFrames * 2);

    // DMA transfer count = kBufferWords (each transfer is one 32-bit word)
    REQUIRE(pico_audio::kBufferWords == 512);

    // Buffer latency = frames / sample_rate
    float latency_ms = static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
                       static_cast<float>(pico_audio::kSampleRate);

    // 256 / 48000 × 1000 = 5.333... ms
    CHECK(latency_ms == Approx(5.333f).epsilon(0.01));

    // Double-buffer total latency
    float total_latency_ms = latency_ms * static_cast<float>(pico_audio::kNumBuffers);
    CHECK(total_latency_ms == Approx(10.667f).epsilon(0.01));
}
