// PolySynth Pico — Sprint 3: I2S Audio Hello World
// Outputs a 440Hz sine wave through the Waveshare Pico-Audio HAT.

#include <cstdio>
#include <cstdint>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "audio_i2s_driver.h"
#include "Engine.h"
#include "SynthState.h"
#include "sine_generator.h"

// ── DSP engine (used in self-test) ───────────────────────────────────────
static PolySynthCore::Engine s_engine;
static PolySynthCore::SynthState s_state;

// ── Sine wave generator state ───────────────────────────────────────────
static float s_phase = 0.0f;
static constexpr float kFrequency = 440.0f;
static constexpr float kAmplitude = 0.5f;  // -6 dBFS to avoid clipping
static constexpr float kPhaseIncrement =
    pico_audio::PhaseIncrement(kFrequency, static_cast<float>(pico_audio::kSampleRate));

// ── Audio callback (called from DMA ISR) ────────────────────────────────
static void audio_callback(uint32_t* buffer, uint32_t num_frames)
{
    pico_audio::GenerateSine(s_phase, kPhaseIncrement, kAmplitude, buffer, num_frames);
}

// ── SRAM report ─────────────────────────────────────────────────────────
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

// ── Self-test sequence ──────────────────────────────────────────────────
// Results are printed as structured markers for CI detection.
// Wokwi CI checks for "[TEST:ALL_PASSED]" in serial output.
static bool run_self_tests()
{
    int pass_count = 0;
    int fail_count = 0;

    printf("[TEST:BEGIN]\n");

    // Test 1: Boot init completed (if we got here, it passed)
    printf("[TEST:PASS] boot_init\n");
    pass_count++;

    // Test 2: Serial output works (if you can read this, it passed)
    printf("[TEST:PASS] serial_output\n");
    pass_count++;

    // Test 3: SRAM report
    print_sram_report();
    printf("[TEST:PASS] sram_report\n");
    pass_count++;

    // Test 4: Engine init
    s_state.Reset();
    s_engine.Init(static_cast<double>(pico_audio::kSampleRate));
    s_engine.UpdateState(s_state);
    printf("[TEST:PASS] engine_init\n");
    pass_count++;

    // Test 5: Engine produces non-zero audio
    {
        float left = 0.0f, right = 0.0f;
        float energy = 0.0f;
        s_engine.OnNoteOn(60, 100);
        for (int i = 0; i < 256; i++) {
            left = 0.0f;
            right = 0.0f;
            s_engine.Process(left, right);
            energy += left * left + right * right;
        }
        s_engine.OnNoteOff(60);

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

    // Test 6: Audio I2S init
    if (pico_audio::Init(audio_callback)) {
        printf("[TEST:PASS] audio_i2s_init\n");
        pass_count++;
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

// ── Main ────────────────────────────────────────────────────────────────
int main()
{
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("ERROR: CYW43 init failed\n");
        return 1;
    }

    // Wait for USB serial
    for (int i = 0; i < 30; i++) {
        if (stdio_usb_connected()) break;
        sleep_ms(100);
    }

    printf("\n");
    printf("========================================\n");
    printf("  PolySynth Pico v0.3 — I2S Audio Test\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Test signal: %d Hz sine wave\n", static_cast<int>(kFrequency));
    printf("========================================\n\n");

    // Run self-tests (includes audio init)
    if (!run_self_tests()) {
        printf("Self-tests failed. Halting.\n");
        while (true) { sleep_ms(1000); }
    }

    // Start audio playback (Init was done in self-test)
    pico_audio::Start();
    printf("Playback started — 440 Hz sine wave\n\n");

    // Status reporting loop
    uint32_t report_counter = 0;
    while (true) {
        sleep_ms(1000);
        report_counter++;

        float fill_time_us = static_cast<float>(pico_audio::GetLastFillTimeUs());
        float buffer_time_us = static_cast<float>(pico_audio::kBufferFrames) * 1000000.0f /
                               static_cast<float>(pico_audio::kSampleRate);
        float cpu_percent = (fill_time_us / buffer_time_us) * 100.0f;

        printf("[%lus] CPU: %.1f%% | Fill: %lu us | Underruns: %lu\n",
               static_cast<unsigned long>(report_counter),
               static_cast<double>(cpu_percent),
               static_cast<unsigned long>(pico_audio::GetLastFillTimeUs()),
               static_cast<unsigned long>(pico_audio::GetUnderrunCount()));

        // Blink LED to show firmware is alive
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, report_counter % 2);
    }

    return 0;
}
