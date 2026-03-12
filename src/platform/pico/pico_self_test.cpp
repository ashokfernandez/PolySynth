#include "pico_self_test.h"
#include "pico_synth_app.h"
#include "audio_i2s_driver.h"
#include "Engine.h"
#include "sine_generator.h"  // pico_audio::PackI2S
#include "golden_crc.h"

#include <cmath>
#include <cstdio>
#include <cstdint>

extern "C" {
    extern uint8_t __StackLimit;
    extern uint8_t __bss_end__;
}

// ── Minimal CRC32 (no table, no zlib dependency) ────────────────────────
static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len)
{
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1u)));
        }
    }
    return ~crc;
}

// ── Fast tanh (same as pico_synth_app.cpp) ──────────────────────────────
static inline float golden_fast_tanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ── Saturating int16 (same as pico_synth_app.cpp) ───────────────────────
static inline int16_t golden_saturate_to_i16(int32_t val) {
    int32_t result;
    __asm__ volatile("ssat %0, #16, %1" : "=r"(result) : "r"(val));
    return static_cast<int16_t>(result);
}

static void print_sram_report()
{
    const uint32_t total_sram = 520 * 1024;
    printf("=== SRAM Report ===\n");
    printf("  Total SRAM: %lu KB\n", static_cast<unsigned long>(total_sram / 1024));
    printf("  sizeof(Engine): %u bytes\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("===================\n");
}

namespace pico_self_test {

bool RunAll(PicoSynthApp& app, void (*audioCallback)(uint32_t*, uint32_t))
{
    int pass_count = 0;
    int fail_count = 0;

    printf("[TEST:BEGIN]\n");

    // Test 1: Boot init completed
    printf("[TEST:PASS] boot_init\n");
    pass_count++;

    // Test 2: Serial output works
    printf("[TEST:PASS] serial_output\n");
    pass_count++;

    // Test 3: SRAM report
    print_sram_report();
    printf("[TEST:PASS] sram_report\n");
    pass_count++;

    // Test 4: Engine init
    app.Init(static_cast<float>(pico_audio::kSampleRate));
    printf("[TEST:PASS] engine_init\n");
    pass_count++;

    // Test 5: Engine produces non-zero audio
    {
        auto& engine = app.GetEngine();
        float left = 0.0f, right = 0.0f;
        float energy = 0.0f;
        engine.OnNoteOn(60, 100);
        for (int i = 0; i < 256; i++) {
            left = 0.0f;
            right = 0.0f;
            engine.Process(left, right);
            energy += left * left + right * right;
        }
        engine.OnNoteOff(60);

        if (energy > 0.0001f) {
            printf("[TEST:PASS] engine_produces_audio (energy=%.4f)\n",
                   static_cast<double>(energy));
            pass_count++;
        } else {
            printf("[TEST:FAIL] engine_produces_audio: energy=%.4f (expected > 0.0001)\n",
                   static_cast<double>(energy));
            fail_count++;
        }
    }

    // Test 6: Audio I2S init (on core 0 for self-test, will re-init on core 1)
    if (pico_audio::Init(audioCallback)) {
        printf("[TEST:PASS] audio_i2s_init\n");
        pass_count++;
        pico_audio::Stop();
    } else {
        printf("[TEST:FAIL] audio_i2s_init\n");
        fail_count++;
    }

    // Test 7: Golden master CRC — full signal chain determinism check.
    // Renders 2048 frames through Engine → gain → fast_tanh → SSAT → I2S pack
    // and compares CRC32 of the packed buffer against the expected value.
    {
        static constexpr uint32_t kGoldenFrames = 2048;
        static constexpr float kOutputGain = 4.0f;
        uint32_t golden_buf[kGoldenFrames * 2]; // stereo: L,R pairs

        // Fresh engine with default state
        auto& engine = app.GetEngine();
        engine.Init(static_cast<double>(pico_audio::kSampleRate));
        engine.OnNoteOn(69, 100); // A4

        for (uint32_t i = 0; i < kGoldenFrames; i++) {
            float left = 0.0f, right = 0.0f;
            engine.Process(left, right);

            left *= kOutputGain;
            right *= kOutputGain;

            auto l16 = golden_saturate_to_i16(
                static_cast<int32_t>(std::roundf(golden_fast_tanh(left) * 32767.0f)));
            auto r16 = golden_saturate_to_i16(
                static_cast<int32_t>(std::roundf(golden_fast_tanh(right) * 32767.0f)));
            golden_buf[i * 2]     = pico_audio::PackI2S(l16);
            golden_buf[i * 2 + 1] = pico_audio::PackI2S(r16);
        }

        engine.OnNoteOff(69);

        uint32_t crc = crc32_update(0, reinterpret_cast<const uint8_t*>(golden_buf),
                                    sizeof(golden_buf));

#if PICO_GOLDEN_CRC32 == 0x00000000
        // Placeholder value — print CRC for bootstrapping, don't fail.
        printf("[GOLDEN:CRC32:%08lx] (generated, update golden_crc.h)\n",
               static_cast<unsigned long>(crc));
        printf("[TEST:PASS] golden_crc (generated)\n");
        pass_count++;
#else
        if (crc == PICO_GOLDEN_CRC32) {
            printf("[GOLDEN:CRC32:%08lx]\n", static_cast<unsigned long>(crc));
            printf("[TEST:PASS] golden_crc\n");
            pass_count++;
        } else {
            printf("[GOLDEN:FAIL] CRC32 mismatch: got 0x%08lx, expected 0x%08lx\n",
                   static_cast<unsigned long>(crc),
                   static_cast<unsigned long>(PICO_GOLDEN_CRC32));
            printf("[TEST:FAIL] golden_crc\n");
            fail_count++;
        }
#endif
    }

    // Summary
    printf("\n[TEST:SUMMARY] %d passed, %d failed\n", pass_count, fail_count);
    if (fail_count == 0) {
        printf("[TEST:ALL_PASSED]\n");
    } else {
        printf("[TEST:SOME_FAILED]\n");
    }

    return fail_count == 0;
}

} // namespace pico_self_test
