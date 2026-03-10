// PolySynth Pico — Sprint 8: Architecture Decomposition
// Slim entry point: boot, self-test, core 1 launch, serial polling loop.
// DSP engine, command dispatch, demo, and self-test are in separate modules.

#include <cstdio>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include "audio_i2s_driver.h"
#include "command_parser.h"
#include "pico_synth_app.h"
#include "pico_command_dispatch.h"
#include "pico_demo.h"
#include "pico_self_test.h"

static PicoSynthApp s_app;
static PicoDemo s_demo;

// ── Audio callback trampoline (static function → member call) ─────────────
static void audio_callback(uint32_t* buffer, uint32_t numFrames) {
    s_app.AudioCallback(buffer, numFrames);
}

// ── Core 1 entry point — dedicated to audio DSP ──────────────────────────
static void core1_audio_entry() {
    // Flush denormals to zero: set FPSCR FZ (bit 24) and DN (bit 25).
    // Denormal floats in filter tails can cost 10-80x more cycles than normal floats.
    // On Cortex-M33 with hard-float ABI, this prevents performance cliffs when
    // signals decay to near-zero (e.g. filter resonance ringing out).
    {
        uint32_t fpscr;
        __asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
        fpscr |= (1u << 24) | (1u << 25);  // FZ=1, DN=1
        __asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
        printf("[Core1] FPSCR set: FZ+DN enabled (0x%08lx)\n",
               static_cast<unsigned long>(fpscr));
    }

    // Init audio on core 1 so the DMA IRQ fires on this core
    if (!pico_audio::Init(audio_callback, 1)) {
        multicore_fifo_push_blocking(0);  // signal failure
        return;
    }
    multicore_fifo_push_blocking(1);  // signal success

    pico_audio::Start();

    // Core 1 just sleeps; all work happens in DMA ISR
    while (true) {
        __wfi();  // wait for interrupt — lowest power, wakes on DMA IRQ
    }
}

// ── Main ─────────────────────────────────────────────────────────────────
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
    printf("  PolySynth Pico v0.6 — Decomposed\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Engine:      %u bytes (effects stripped)\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("========================================\n");
    printf("Commands: NOTE_ON <n> <v>, NOTE_OFF <n>, SET <p> <v>,\n");
    printf("          GET <p>, STATUS, PANIC, RESET, DEMO, STOP\n");
    printf("========================================\n\n");

    // Run self-tests (includes engine init + audio I2S test)
    if (!pico_self_test::RunAll(s_app, audio_callback)) {
        printf("Self-tests failed. Halting.\n");
        while (true) { sleep_ms(1000); }
    }

    // Re-init engine cleanly after self-test
    s_app.Init(static_cast<float>(pico_audio::kSampleRate));

    // Launch core 1 for dedicated audio DSP
    multicore_launch_core1(core1_audio_entry);
    uint32_t audio_ok = multicore_fifo_pop_blocking();
    if (!audio_ok) {
        printf("ERROR: Audio init on core 1 failed, falling back to core 0\n");
        pico_audio::Init(audio_callback);
        pico_audio::Start();
    }
    printf("Audio started on core %d — running demo sequence\n", audio_ok ? 1 : 0);
    printf("Type STOP to halt demo, or send commands\n\n");

    // Start demo
    s_demo.Start(time_us_64());

    // Main loop: serial polling + demo + status
    pico_serial::CommandParser parser;
    uint32_t report_counter = 0;
    uint64_t last_report_us = time_us_64();

    while (true) {
        // Non-blocking serial polling
        int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            if (parser.Feed(static_cast<char>(ch))) {
                auto cmd = parser.Parse();
                pico_commands::Dispatch(cmd, s_app, s_demo);
            }
        }

        // Service CYW43 driver (required for pico_cyw43_arch_none)
        cyw43_arch_poll();

        // Demo tick
        s_demo.Update(time_us_64(), s_app);

        // Drain voice-change events
        int8_t from, to;
        while (s_app.DrainVoiceEvent(from, to)) {
            printf("[VOICE] %d -> %d\n", from, to);
        }

        // Periodic status report (every 5 seconds)
        uint64_t now = time_us_64();
        if (now - last_report_us >= 5000000ULL) {
            last_report_us = now;
            report_counter++;

            float fill_time_us = static_cast<float>(pico_audio::GetLastFillTimeUs());
            float buffer_time_us = static_cast<float>(pico_audio::kBufferFrames) * 1000000.0f /
                                   static_cast<float>(pico_audio::kSampleRate);
            float cpu_percent = (fill_time_us / buffer_time_us) * 100.0f;

            float peak = s_app.ExchangePeakLevel();

            printf("[%lus] CPU: %.1f%% | Voices: %d | Peak: %.3f | Phase: %s\n",
                   static_cast<unsigned long>(report_counter * 5),
                   static_cast<double>(cpu_percent),
                   s_app.GetActiveVoiceCount(),
                   static_cast<double>(peak),
                   s_demo.IsRunning() ? s_demo.CurrentPhaseName() : "OFF");

            // Blink LED
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, report_counter % 2);
        }
    }

    return 0;
}
