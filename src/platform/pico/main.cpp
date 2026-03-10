// PolySynth Pico — Sprint 9: FreeRTOS Migration
// FreeRTOS on Core 0 (serial task + idle heartbeat). Core 1 bare-metal (DMA ISR audio).

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

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

// ── Audio callback trampoline (static function → member call) ─────────────
static void audio_callback(uint32_t* buffer, uint32_t numFrames) {
    s_app.AudioCallback(buffer, numFrames);
}

// ── Core 1 entry point — dedicated to audio DSP (bare-metal, no FreeRTOS) ─
static void core1_audio_entry() {
    // Flush denormals to zero: set FPSCR FZ (bit 24) and DN (bit 25).
    // Denormal floats in filter tails can cost 10-80x more cycles than normal floats.
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

// ── Serial task — runs on Core 0 under FreeRTOS ──────────────────────────
// Handles serial command parsing, demo sequencing, voice event draining,
// and periodic status reporting.
static void serial_task(void* params) {
    (void)params;

    PicoDemo demo;
    pico_serial::CommandParser parser;
    uint32_t report_counter = 0;
    uint64_t last_report_us = time_us_64();

    // Start demo
    demo.Start(time_us_64());
    printf("Type STOP to halt demo, or send commands\n\n");

    while (true) {
        // Non-blocking serial read with 1ms timeout — yields CPU to lower-priority tasks
        int ch = getchar_timeout_us(1000);
        if (ch != PICO_ERROR_TIMEOUT) {
            if (parser.Feed(static_cast<char>(ch))) {
                auto cmd = parser.Parse();
                pico_commands::Dispatch(cmd, s_app, demo);
            }
        }

        // Demo tick
        demo.Update(time_us_64(), s_app);

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
                   demo.IsRunning() ? demo.CurrentPhaseName() : "OFF");
        }
    }
}

// ── LED heartbeat — FreeRTOS idle hook ───────────────────────────────────
// Blinks the onboard LED at 1Hz. No blink = system hung.
// Called from the idle task — must NOT block or call any blocking FreeRTOS API.
static bool s_led_state = false;
static uint32_t s_last_led_toggle = 0;

extern "C" void vApplicationIdleHook() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - s_last_led_toggle >= 1000) {
        s_led_state = !s_led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, s_led_state);
        s_last_led_toggle = now;
    }
}

// ── Stack overflow hook — required when configCHECK_FOR_STACK_OVERFLOW > 0 ─
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    printf("STACK OVERFLOW in task: %s\n", pcTaskName);
    // Halt — stack overflow is unrecoverable
    while (true) { __wfi(); }
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
    printf("  PolySynth Pico v0.9 — FreeRTOS\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Engine:      %u bytes (effects stripped)\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("  RTOS:        FreeRTOS (Core 0 only)\n");
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

    // Launch core 1 for dedicated audio DSP (BEFORE vTaskStartScheduler)
    multicore_launch_core1(core1_audio_entry);
    uint32_t audio_ok = multicore_fifo_pop_blocking();
    if (!audio_ok) {
        printf("ERROR: Audio init on core 1 failed, falling back to core 0\n");
        pico_audio::Init(audio_callback);
        pico_audio::Start();
    }
    printf("Audio started on core %d\n", audio_ok ? 1 : 0);

    // Create serial task — priority 4, 1024-word (4KB) stack
    xTaskCreate(serial_task, "Serial", 1024, nullptr, 4, nullptr);

    // Start FreeRTOS scheduler — never returns
    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    // Should never reach here
    printf("ERROR: FreeRTOS scheduler exited\n");
    return 1;
}
