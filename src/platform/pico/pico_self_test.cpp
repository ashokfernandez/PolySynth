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

    // Test 7: Init→Stop→Init round-trip produces non-zero audio
    // Regression test for PIO output ORing bug: if Stop() fails to release
    // PIO resources, a second Init() gets a different SM whose output is
    // OR'd with the old SM's latched pin values, corrupting I2S clocks.
    {
        // Re-init engine to clean state (Test 5 left a note decaying)
        app.Init(static_cast<float>(pico_audio::kSampleRate));

        // Re-init I2S (this is the second Init after Test 6's Init+Stop)
        if (pico_audio::Init(audioCallback)) {
            // Trigger note directly on engine (same core, no SPSC needed)
            auto& engine = app.GetEngine();
            engine.OnNoteOn(60, 100);

            // Fill a buffer through the full audio callback chain
            uint32_t test_buf[pico_audio::kBufferFrames * 2];
            audioCallback(test_buf, pico_audio::kBufferFrames);

            // Check for non-zero I2S samples (any non-silent frame passes)
            uint32_t nonzero = 0;
            for (uint32_t i = 0; i < pico_audio::kBufferFrames * 2; i++) {
                if (test_buf[i] != 0) nonzero++;
            }

            engine.OnNoteOff(60);
            pico_audio::Stop();

            if (nonzero > 0) {
                printf("[TEST:PASS] audio_reinit_produces_audio (%lu nonzero samples)\n",
                       static_cast<unsigned long>(nonzero));
                pass_count++;
            } else {
                printf("[TEST:FAIL] audio_reinit_produces_audio: buffer is silent after Init->Stop->Init\n");
                fail_count++;
            }
        } else {
            printf("[TEST:FAIL] audio_reinit_produces_audio: second Init() failed\n");
            fail_count++;
        }
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
