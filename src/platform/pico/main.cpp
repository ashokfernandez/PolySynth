// PolySynth Pico — Sprint 1: Prove DSP cross-compiles for ARM
// Validates toolchain, USB serial, onboard LED, and DSP Engine on Pico 2 W (RP2350).

#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// DSP smoke test: pull in the full Engine header chain.
// This forces VoiceManager, all SEA_DSP oscillator/filter/envelope headers,
// effects (chorus, delay, limiter), and <atomic> through the ARM compiler.
#include "Engine.h"

// Static Engine instance — proves the entire DSP object graph links for ARM.
static PolySynthCore::Engine g_engine;

// Linker-provided symbols for SRAM usage reporting
extern "C" {
    extern uint8_t __StackLimit;
    extern uint8_t __bss_end__;
    extern uint8_t __end__;
}

static void print_sram_report()
{
    // Static allocation: data + bss sections
    const uint32_t static_used = reinterpret_cast<uint32_t>(&__bss_end__);
    const uint32_t stack_limit = reinterpret_cast<uint32_t>(&__StackLimit);
    const uint32_t total_sram = 520 * 1024; // RP2350: 520KB

    printf("=== SRAM Report ===\n");
    printf("  Static (data+bss):  %lu bytes\n",
           static_cast<unsigned long>(static_used));
    printf("  Stack limit addr:   0x%08lx\n",
           static_cast<unsigned long>(stack_limit));
    printf("  Total SRAM:         %lu bytes (%lu KB)\n",
           static_cast<unsigned long>(total_sram),
           static_cast<unsigned long>(total_sram / 1024));
    printf("  sizeof(Engine):     %u bytes\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("===================\n");
}

int main()
{
    // Initialize USB serial (stdio)
    stdio_init_all();

    // Initialize CYW43 — required for onboard LED on Pico 2 W.
    // pico_cyw43_arch_none: minimal init, no WiFi/BT stack.
    if (cyw43_arch_init()) {
        printf("ERROR: CYW43 init failed\n");
        return 1;
    }

    // Wait for USB serial to connect (with timeout for headless operation)
    for (int i = 0; i < 30; i++) {
        if (stdio_usb_connected()) break;
        sleep_ms(100);
    }

    printf("\n");
    printf("========================================\n");
    printf("  PolySynth Pico v0.1\n");
    printf("  Board:    Pico 2 W (RP2350)\n");
    printf("  Voices:   %d\n", POLYSYNTH_MAX_VOICES);
    printf("  Precision: float (single)\n");
    printf("========================================\n");
    printf("\n");

    print_sram_report();

    printf("\nBlink loop started — LED toggles every 500ms\n");

    // Main loop: blink LED to prove firmware is running
    bool led_on = false;
    while (true) {
        led_on = !led_on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        sleep_ms(500);
    }

    return 0;
}
