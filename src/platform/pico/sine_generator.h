#pragma once
// Sine wave generator — pure logic, no hardware dependency.
// Used by Pico firmware and desktop unit tests.

#include <cmath>
#include <cstdint>

namespace pico_audio {

// Compute the phase increment for a given frequency and sample rate.
inline constexpr float PhaseIncrement(float freq, float sample_rate)
{
    return 2.0f * 3.14159265f * freq / sample_rate;
}

// Pack an int16 sample into a left-justified 32-bit I2S word.
// Upper 16 bits = audio data, lower 16 bits = zero padding.
inline uint32_t PackI2S(int16_t sample)
{
    return static_cast<uint32_t>(static_cast<uint16_t>(sample)) << 16;
}

// Fill a left-justified 32-bit stereo buffer with a sine wave.
// phase: current oscillator phase (updated in place, wraps at 2π)
// phase_inc: per-sample phase increment (from PhaseIncrement())
// amplitude: peak amplitude in [0, 1] range (maps to int16 ±32767)
// buffer: interleaved stereo output [L0,R0,L1,R1,...] as uint32_t (left-justified)
// num_frames: number of stereo frames to fill
inline void GenerateSine(float& phase,
                         float phase_inc,
                         float amplitude,
                         uint32_t* buffer,
                         uint32_t num_frames)
{
    constexpr float kTwoPi = 2.0f * 3.14159265f;
    for (uint32_t i = 0; i < num_frames; i++) {
        float sample = sinf(phase) * amplitude;
        auto sample_i16 = static_cast<int16_t>(sample * 32767.0f);
        uint32_t word = PackI2S(sample_i16);

        buffer[i * 2]     = word;  // Left
        buffer[i * 2 + 1] = word;  // Right (mono → both channels)

        phase += phase_inc;
        if (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
    }
}

} // namespace pico_audio
