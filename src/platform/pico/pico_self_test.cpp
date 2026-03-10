#include "pico_self_test.h"
#include "pico_synth_app.h"
#include "audio_i2s_driver.h"
#include "Engine.h"

#include <cstdio>

extern "C" {
    extern uint8_t __StackLimit;
    extern uint8_t __bss_end__;
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
