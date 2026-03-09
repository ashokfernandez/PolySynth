// PolySynth Pico — Sprint 3: I2S Audio Hello World
// Outputs a 440Hz sine wave through the Waveshare Pico-Audio HAT.

#include <cstdio>
#include <cstdint>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "audio_i2s_driver.h"

// DSP link smoke test: verify the full Engine object graph links for ARM.
// Removed before Sprint Pico-4 when real engine integration happens.
#ifdef PICO_LINK_SMOKE_TEST
#include "Engine.h"
static PolySynthCore::Engine g_engine;
#endif

#include "sine_generator.h"

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
    printf("===================\n");
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
    printf("  PolySynth Pico v0.2 — I2S Audio Test\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Test signal: %d Hz sine wave\n", static_cast<int>(kFrequency));
    printf("========================================\n\n");

    print_sram_report();

    // Initialize I2S audio
    if (!pico_audio::Init(audio_callback)) {
        printf("ERROR: Audio init failed\n");
        return 1;
    }
    printf("Audio initialized. Starting playback...\n");

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
