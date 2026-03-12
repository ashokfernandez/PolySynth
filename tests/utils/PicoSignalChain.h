#pragma once
// Portable version of the Pico audio callback post-processing chain.
// Mirrors PicoSynthApp::AudioCallback (pico_synth_app.cpp) but replaces
// ARM-specific intrinsics (SSAT) with portable C++ equivalents.
// Used by golden master tests to verify the full embedded signal path
// on the host without ARM hardware.

#include "../../src/core/Engine.h"
#include "../../src/platform/pico/sine_generator.h" // pico_audio::PackI2S

#include <algorithm> // std::clamp
#include <cmath>     // std::roundf
#include <cstdint>
#include <vector>

namespace PicoSignalChain {

// ── Fast tanh approximant (Padé) ─────────────────────────────────────────
// Identical to pico_synth_app.cpp — pure math, already portable.
inline float fast_tanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ── Saturating int16 conversion ──────────────────────────────────────────
// Portable equivalent of the ARM SSAT instruction used on Pico.
inline int16_t saturate_to_i16(int32_t val) {
    return static_cast<int16_t>(std::clamp(val, int32_t(-32768), int32_t(32767)));
}

// Output gain applied in PicoSynthApp::AudioCallback before soft-clipping.
static constexpr float kOutputGain = 4.0f;

// ── Render engine output through the full Pico signal chain ──────────────
// Processes numFrames samples through:
//   Engine.Process() → gain → fast_tanh → int16 saturation → I2S packing
//
// Returns packed stereo I2S buffer (2 × numFrames uint32_t words).
inline std::vector<uint32_t> RenderToI2S(PolySynthCore::Engine& engine,
                                          uint32_t numFrames) {
    std::vector<uint32_t> buffer(numFrames * 2);

    for (uint32_t i = 0; i < numFrames; i++) {
        float left = 0.0f, right = 0.0f;
        engine.Process(left, right);

        left *= kOutputGain;
        right *= kOutputGain;

        auto l16 = saturate_to_i16(static_cast<int32_t>(std::roundf(fast_tanh(left) * 32767.0f)));
        auto r16 = saturate_to_i16(static_cast<int32_t>(std::roundf(fast_tanh(right) * 32767.0f)));
        buffer[i * 2]     = pico_audio::PackI2S(l16);
        buffer[i * 2 + 1] = pico_audio::PackI2S(r16);
    }

    return buffer;
}

// ── Unpack I2S buffer to int16 samples ───────────────────────────────────
// Extracts left-channel int16 values from packed I2S words.
inline std::vector<int16_t> UnpackI2SLeft(const std::vector<uint32_t>& buffer,
                                           uint32_t numFrames) {
    std::vector<int16_t> samples(numFrames);
    for (uint32_t i = 0; i < numFrames; i++) {
        samples[i] = static_cast<int16_t>(buffer[i * 2] >> 16);
    }
    return samples;
}

} // namespace PicoSignalChain
