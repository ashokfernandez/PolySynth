# Sprint Pico-2: I2S Audio Hello World

**Depends on:** Sprint Pico-1
**Blocks:** Sprint Pico-3
**Approach:** Incremental — PIO I2S driver first, then sine wave test, then DMA tuning
**Estimated effort:** 2–3 days

---

## Goal

Configure PIO-based I2S with DMA for the PCM5101A DAC (Waveshare Pico-Audio HAT) and output a clean 440Hz sine wave. Establish the DMA double-buffer audio callback pattern that all subsequent sprints will build on.

---

## Prerequisites

- Sprint Pico-1 complete (`just pico-build` works, LED blinks, serial output confirmed)
- Waveshare Pico-Audio HAT physically connected to Pico 2 W
- Headphones or speakers connected to the HAT's 3.5mm jack
- A tuner app (phone or computer) for frequency verification

### Hardware Pin Mapping (Waveshare Pico-Audio HAT)

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| DIN (I2S Data) | GPIO26 | Pico → DAC | Serial audio data |
| BCK (Bit Clock) | GPIO27 | Pico → DAC | 48000 × 32 = 1.536 MHz |
| LRCK (Word Select) | GPIO28 | Pico → DAC | Left/Right channel select = 48 kHz |

**PCM5101A DAC format:** I2S standard, MSB-first, 16-bit or 32-bit words. We use 16-bit.

---

## Task 2.1: Evaluate and Set Up I2S Approach

### What

The core Pico SDK does **not** include I2S support — it must come from `pico-extras` (the official extras library) or a custom PIO program. We use `pico-extras` which provides `audio_i2s` with PIO-based I2S and DMA support.

### Design Decision: pico-extras vs. Custom PIO

| Approach | Pros | Cons |
|----------|------|------|
| **pico-extras `audio_i2s`** | Official, maintained, DMA built-in | Extra dependency, opinionated API |
| **Custom PIO program** | Full control, minimal code | Must write/debug PIO assembly, DMA setup |

**Decision: Custom PIO + DMA.** The `pico-extras` `audio_i2s` module targets the RP2040 and its API adds complexity we don't need. A custom PIO I2S program is ~15 lines of PIO assembly, gives us full control over the DMA callback pattern, and avoids an external dependency. This is the approach used by most production RP2350 audio projects.

### No File Changes

This task is a design decision only. Document it in this plan — no code changes.

### Acceptance Criteria

- [ ] Decision documented: custom PIO + DMA, not pico-extras

---

## Task 2.2: Create PIO I2S Program

### What

Create the PIO (Programmable I/O) assembly program that shifts out I2S audio data. PIO is the RP2350's hardware state machine that runs independently of the CPU — it handles the precise timing of I2S bit-clocking while the CPU does DSP work.

### File to Create

**`/src/platform/pico/i2s.pio`**

```pio
; I2S output program for PCM5101A DAC
; Format: I2S standard, MSB-first, 16-bit stereo
;
; Protocol:
;   - BCK (bit clock) toggles on every instruction cycle
;   - LRCK (word select) low = left channel, high = right channel
;   - DIN shifts out MSB-first on rising BCK edge
;   - 32 BCK cycles per channel (16 data bits + 16 padding zeros)
;
; Autopull: 32-bit words from TX FIFO (left-justified: data in upper 16 bits)
; Side-set: 2 pins (bit 0 = BCK, bit 1 = LRCK)

.program i2s_out
.side_set 2

; Each channel: 32 bit-clocks (16 data + 16 zeros)
; We use autopull with 32-bit threshold — PIO pulls a new word per channel

                        ;        BCK  LRCK
.wrap_target
    ; Left channel (LRCK = 0)
    set x, 14           side 0b10  ; BCK high, LRCK low — start left
left_loop:
    out pins, 1          side 0b00  ; BCK low:  shift out data bit
    jmp x--, left_loop   side 0b10  ; BCK high: DAC latches bit
    out pins, 1          side 0b00  ; Last data bit (bit 0)
    nop                  side 0b10  ; BCK high for last data bit

    ; Padding zeros for bits 16-31 of left channel
    set x, 14            side 0b00  ; BCK low
left_pad:
    nop                  side 0b10  ; BCK high
    jmp x--, left_pad    side 0b00  ; BCK low
    nop                  side 0b10  ; Last pad high
    nop                  side 0b00  ; Last pad low

    ; Right channel (LRCK = 1)
    set x, 14            side 0b11  ; BCK high, LRCK high — start right
right_loop:
    out pins, 1          side 0b01  ; BCK low, LRCK high
    jmp x--, right_loop  side 0b11  ; BCK high, LRCK high
    out pins, 1          side 0b01  ; Last data bit
    nop                  side 0b11  ; BCK high for last bit

    ; Padding zeros for bits 16-31 of right channel
    set x, 14            side 0b01  ; BCK low, LRCK high
right_pad:
    nop                  side 0b11  ; BCK high, LRCK high
    jmp x--, right_pad   side 0b01  ; BCK low, LRCK high
    nop                  side 0b11  ; Last pad high
.wrap
```

### Implementation Notes for Juniors

**What is PIO?**
PIO (Programmable I/O) is a hardware peripheral unique to RP2040/RP2350. It's a tiny state machine that runs independently of the CPU at a configurable clock rate. Each PIO instruction executes in exactly one clock cycle (with side-set happening simultaneously). This gives bit-exact timing for protocols like I2S.

**Why not use the CPU to bit-bang I2S?**
I2S requires BCK to toggle at 1.536 MHz (48kHz × 32). At 150 MHz CPU clock, that's only ~97 CPU cycles per bit — not enough time for DSP. PIO handles the timing in hardware, freeing the CPU for audio processing.

**Side-set pins:**
PIO instructions can set "side-set" pins simultaneously with the main instruction. We use 2 side-set pins: BCK (bit clock) and LRCK (word select). This lets us toggle clocks with zero additional instructions.

### Acceptance Criteria

- [ ] PIO program assembles without errors (verified by `pioasm` during CMake build)
- [ ] Timing: 32 BCK cycles per channel, 64 total per frame
- [ ] LRCK is low for left channel, high for right channel
- [ ] Data shifts out MSB-first on rising BCK edge

---

## Task 2.3: Create I2S Audio Driver

### What

Create the C++ driver that initializes the PIO I2S program and sets up DMA double-buffering. This is the hardware abstraction layer between the DSP engine and the DAC.

### Files to Create

**`/src/platform/pico/audio_i2s_driver.h`**

```cpp
#pragma once

#include <cstdint>

// Audio callback signature: fill the buffer with interleaved stereo int16 samples.
// buffer: pointer to int16_t array of size (2 * num_frames) — [L0,R0,L1,R1,...]
// num_frames: number of stereo frames to fill
using AudioCallback = void (*)(int16_t* buffer, uint32_t num_frames);

namespace pico_audio {

// Audio buffer configuration
static constexpr uint32_t kSampleRate     = 48000;
static constexpr uint32_t kBufferFrames   = 256;    // Stereo frames per DMA buffer
static constexpr uint32_t kBufferSamples  = kBufferFrames * 2;  // Total int16 samples per buffer
static constexpr uint32_t kNumBuffers     = 2;      // Double-buffering (ping-pong)

// Buffer latency: 256 frames / 48000 Hz = 5.33 ms per buffer
// Total latency: 2 × 5.33 ms = 10.67 ms (double-buffered)

// Initialize PIO I2S and DMA. Must be called once at startup.
// callback: function called from DMA ISR to fill the next buffer.
// Returns true on success.
bool Init(AudioCallback callback);

// Start audio playback. Call after Init().
void Start();

// Stop audio playback.
void Stop();

// Get DMA underrun count (for diagnostics).
uint32_t GetUnderrunCount();

// Get current buffer fill time in microseconds (for CPU% calculation).
uint32_t GetLastFillTimeUs();

} // namespace pico_audio
```

**`/src/platform/pico/audio_i2s_driver.cpp`**

```cpp
#include "audio_i2s_driver.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/time.h"

// Generated PIO header (created by pioasm during CMake build)
#include "i2s.pio.h"

namespace pico_audio {

// ── State ────────────────────────────────────────────────────────────────
static int16_t s_buffer[kNumBuffers][kBufferSamples];  // ~2 KB total
static volatile uint32_t s_active_buffer = 0;          // Which buffer DMA is sending
static AudioCallback s_callback = nullptr;
static volatile uint32_t s_underrun_count = 0;
static volatile uint32_t s_last_fill_time_us = 0;

static PIO s_pio = pio0;
static uint s_sm = 0;           // PIO state machine index
static int s_dma_channel_a = -1;
static int s_dma_channel_b = -1;

// ── Pin configuration (Waveshare Pico-Audio HAT) ────────────────────────
static constexpr uint PIN_DIN  = 26;  // I2S data
static constexpr uint PIN_BCK  = 27;  // Bit clock (side-set pin 0)
static constexpr uint PIN_LRCK = 28;  // Word select (side-set pin 1)

// ── DMA interrupt handler ───────────────────────────────────────────────
// Called when one DMA channel completes — triggers fill of the next buffer.
static void dma_irq_handler()
{
    uint32_t start_us = time_us_32();

    // Determine which channel completed and acknowledge
    uint32_t fill_idx;
    if (dma_channel_get_irq0_status(s_dma_channel_a)) {
        dma_channel_acknowledge_irq0(s_dma_channel_a);
        // Channel A just finished sending buffer 0 — fill buffer 0
        fill_idx = 0;
    } else if (dma_channel_get_irq0_status(s_dma_channel_b)) {
        dma_channel_acknowledge_irq0(s_dma_channel_b);
        // Channel B just finished sending buffer 1 — fill buffer 1
        fill_idx = 1;
    } else {
        return;  // Spurious interrupt
    }

    // Fill the buffer that just finished sending (it's now free)
    if (s_callback) {
        s_callback(s_buffer[fill_idx], kBufferFrames);
    }

    s_last_fill_time_us = time_us_32() - start_us;
}

// ── Public API ──────────────────────────────────────────────────────────
bool Init(AudioCallback callback)
{
    if (!callback) return false;
    s_callback = callback;

    // ── PIO setup ────────────────────────────────────────────────────
    uint offset = pio_add_program(s_pio, &i2s_out_program);
    s_sm = pio_claim_unused_sm(s_pio, true);

    // Configure PIO state machine
    pio_sm_config cfg = i2s_out_program_get_default_config(offset);

    // DIN: output on GPIO26 (1 pin for 'out')
    sm_config_set_out_pins(&cfg, PIN_DIN, 1);
    pio_gpio_init(s_pio, PIN_DIN);
    pio_sm_set_consecutive_pindirs(s_pio, s_sm, PIN_DIN, 1, true);

    // BCK + LRCK: side-set on GPIO27-28 (2 pins)
    sm_config_set_sideset_pins(&cfg, PIN_BCK);
    pio_gpio_init(s_pio, PIN_BCK);
    pio_gpio_init(s_pio, PIN_LRCK);
    pio_sm_set_consecutive_pindirs(s_pio, s_sm, PIN_BCK, 2, true);

    // Autopull: shift out MSB-first, 32-bit threshold, shift right
    sm_config_set_out_shift(&cfg, false, true, 32);

    // Clock divider: PIO clock = system_clock / divider
    // Each I2S frame = 64 PIO cycles (32 per channel, 2 instructions per bit)
    // Target: 48000 frames/sec × 64 cycles/frame × 2 instructions/cycle = 6.144 MHz PIO rate
    // But our PIO program uses ~128 instructions per frame (including loops)
    // Actual: set_clock_div = sys_clk / (sample_rate × bits_per_frame × 2)
    float sys_clk = (float)clock_get_hz(clk_sys);
    float pio_clk = (float)kSampleRate * 64.0f * 2.0f;  // 2 PIO cycles per bit period
    sm_config_set_clkdiv(&cfg, sys_clk / pio_clk);

    pio_sm_init(s_pio, s_sm, offset, &cfg);

    // ── DMA setup (ping-pong double buffer) ─────────────────────────
    s_dma_channel_a = dma_claim_unused_channel(true);
    s_dma_channel_b = dma_claim_unused_channel(true);

    // Channel A config: sends buffer 0, chains to channel B
    dma_channel_config dma_cfg_a = dma_channel_get_default_config(s_dma_channel_a);
    channel_config_set_transfer_data_size(&dma_cfg_a, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg_a, true);
    channel_config_set_write_increment(&dma_cfg_a, false);
    channel_config_set_dreq(&dma_cfg_a, pio_get_dreq(s_pio, s_sm, true));
    channel_config_set_chain_to(&dma_cfg_a, s_dma_channel_b);

    dma_channel_configure(
        s_dma_channel_a,
        &dma_cfg_a,
        &s_pio->txf[s_sm],           // Write to PIO TX FIFO
        s_buffer[0],                   // Read from buffer 0
        kBufferSamples / 2,            // Transfer count: 32-bit words (2 × int16 per word)
        false                          // Don't start yet
    );

    // Channel B config: sends buffer 1, chains to channel A
    dma_channel_config dma_cfg_b = dma_channel_get_default_config(s_dma_channel_b);
    channel_config_set_transfer_data_size(&dma_cfg_b, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg_b, true);
    channel_config_set_write_increment(&dma_cfg_b, false);
    channel_config_set_dreq(&dma_cfg_b, pio_get_dreq(s_pio, s_sm, true));
    channel_config_set_chain_to(&dma_cfg_b, s_dma_channel_a);

    dma_channel_configure(
        s_dma_channel_b,
        &dma_cfg_b,
        &s_pio->txf[s_sm],           // Write to PIO TX FIFO
        s_buffer[1],                   // Read from buffer 1
        kBufferSamples / 2,            // Transfer count
        false                          // Don't start yet
    );

    // Enable DMA interrupt on both channels (for refill notification)
    dma_channel_set_irq0_enabled(s_dma_channel_a, true);
    dma_channel_set_irq0_enabled(s_dma_channel_b, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Pre-fill both buffers with silence
    for (uint32_t i = 0; i < kBufferSamples; i++) {
        s_buffer[0][i] = 0;
        s_buffer[1][i] = 0;
    }

    return true;
}

void Start()
{
    // Pre-fill buffer 0 with audio data
    if (s_callback) {
        s_callback(s_buffer[0], kBufferFrames);
    }

    // Start PIO state machine
    pio_sm_set_enabled(s_pio, s_sm, true);

    // Start DMA channel A (will chain to B automatically)
    dma_channel_start(s_dma_channel_a);
}

void Stop()
{
    pio_sm_set_enabled(s_pio, s_sm, false);
    dma_channel_abort(s_dma_channel_a);
    dma_channel_abort(s_dma_channel_b);
}

uint32_t GetUnderrunCount()
{
    return s_underrun_count;
}

uint32_t GetLastFillTimeUs()
{
    return s_last_fill_time_us;
}

} // namespace pico_audio
```

### Implementation Notes for Juniors

**DMA Double-Buffering (Ping-Pong) Pattern:**
```
Time →
Buffer 0: [FILL] [SEND───────] [FILL] [SEND───────] ...
Buffer 1: [SEND───────] [FILL] [SEND───────] [FILL] ...
```
While DMA sends buffer 0 to the DAC, the CPU fills buffer 1 with new audio data. When DMA finishes buffer 0, it automatically chains to buffer 1 (and vice versa). The DMA ISR fires to notify the CPU to fill the buffer that just finished sending.

**Why `DMA_SIZE_32` when we have 16-bit samples?**
The PIO program consumes 32-bit words from its TX FIFO (one word per channel). We pack two int16 samples into each 32-bit DMA transfer. The interleaved buffer layout is `[L0, R0, L1, R1, ...]` where each pair of int16 values forms one 32-bit word.

**Clock divider math:**
- System clock: 150 MHz (RP2350 default)
- I2S needs: 48000 Hz × 64 bits/frame = 3.072 MHz bit rate
- PIO cycles per bit: varies by program, but the divider scales the PIO clock to achieve the target sample rate

### Acceptance Criteria

- [ ] PIO program loads and initializes without error
- [ ] DMA channels allocated (check via serial print)
- [ ] No compile errors or warnings
- [ ] Double-buffer structure is correct (2 × 256 frames × 2 channels × 2 bytes = 2 KB)

---

## Task 2.4: Implement 440Hz Sine Wave Test

### What

Replace the LED blink loop in `main.cpp` with a 440Hz sine wave audio test. This proves the entire audio pipeline: CPU → DMA → PIO → I2S → DAC → speakers.

### File to Modify

**`/src/platform/pico/main.cpp`** — Replace with:

```cpp
// PolySynth Pico — Sprint 2: I2S Audio Hello World
// Outputs a 440Hz sine wave through the Waveshare Pico-Audio HAT.

#include <cstdio>
#include <cstdint>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "audio_i2s_driver.h"

// ── Sine wave generator state ───────────────────────────────────────────
static float s_phase = 0.0f;
static constexpr float kFrequency = 440.0f;
static constexpr float kAmplitude = 0.5f;  // -6 dBFS to avoid clipping
static constexpr float kPhaseIncrement =
    2.0f * 3.14159265f * kFrequency / static_cast<float>(pico_audio::kSampleRate);

// ── Audio callback (called from DMA ISR) ────────────────────────────────
static void audio_callback(int16_t* buffer, uint32_t num_frames)
{
    for (uint32_t i = 0; i < num_frames; i++) {
        float sample = sinf(s_phase) * kAmplitude;
        int16_t sample_i16 = static_cast<int16_t>(sample * 32767.0f);

        // Interleaved stereo: L, R, L, R, ...
        buffer[i * 2]     = sample_i16;  // Left
        buffer[i * 2 + 1] = sample_i16;  // Right (mono → both channels)

        s_phase += kPhaseIncrement;
        if (s_phase >= 2.0f * 3.14159265f) {
            s_phase -= 2.0f * 3.14159265f;
        }
    }
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
```

### Verification Procedure

1. **Build and flash:**
   ```bash
   just pico-build
   # Hold BOOTSEL, connect USB, release BOOTSEL
   cp build/pico/polysynth_pico.uf2 /media/$USER/RP2350/
   ```

2. **Listen:** Connect headphones to the Pico-Audio HAT. You should hear a clean 440Hz tone.

3. **Verify frequency:** Open a guitar tuner app. It should show A4 (440Hz ±1Hz).

4. **Monitor serial:**
   ```bash
   minicom -D /dev/ttyACM0 -b 115200
   ```
   Expected output:
   ```
   [1s] CPU: 0.3% | Fill: 15 us | Underruns: 0
   [2s] CPU: 0.3% | Fill: 15 us | Underruns: 0
   ...
   ```

5. **60-second stability test:** Let it run for 60 seconds. Underrun count must remain 0.

### Acceptance Criteria

- [ ] Clean 440Hz sine wave audible through the Pico-Audio HAT
- [ ] No clicks, pops, or distortion
- [ ] Frequency verifiable with a tuner app (440Hz ±1Hz)
- [ ] Serial confirms sample rate (48kHz) and buffer size (256 frames)
- [ ] Zero DMA underruns during 60 seconds of playback
- [ ] CPU usage < 5% for sine wave generation

---

## Task 2.5: Update CMakeLists.txt for Audio Sources

### What

Add the new audio driver files and PIO program to the Pico CMake build.

### File to Modify

**`/src/platform/pico/CMakeLists.txt`** — Update the executable sources and add PIO/DMA dependencies:

Add to the `add_executable` call:
```cmake
add_executable(polysynth_pico
    main.cpp
    audio_i2s_driver.cpp
)
```

Add PIO program compilation (after `add_executable`):
```cmake
# Generate PIO header from .pio assembly
pico_generate_pio_header(polysynth_pico ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)
```

Add hardware libraries to `target_link_libraries`:
```cmake
target_link_libraries(polysynth_pico PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    hardware_pio
    hardware_dma
    hardware_clocks
    SEA_DSP
    SEA_Util
)
```

### Acceptance Criteria

- [ ] `cmake --build build/pico` succeeds with audio driver included
- [ ] `i2s.pio.h` is generated in the build directory
- [ ] No new warnings

---

## Task 2.6: Buffer Size Tuning (Optional — Stretch Goal)

### What

Test different buffer sizes to find the optimal latency vs. stability tradeoff. This is not required for the sprint but is valuable data for Sprint 3.

### Test Matrix

| Buffer Frames | Latency (ms) | Expected Behavior |
|---------------|---------------|-------------------|
| 64 | 1.33 | May underrun under DSP load |
| 128 | 2.67 | Good balance for lightweight DSP |
| 256 | 5.33 | Conservative, very stable |
| 512 | 10.67 | Maximum stability, noticeable latency |

Change `kBufferFrames` in `audio_i2s_driver.h` and rebuild. Monitor underrun count for 60 seconds at each setting.

### Acceptance Criteria

- [ ] Results documented: buffer size vs. underrun count for each setting
- [ ] Default remains 256 frames unless testing shows a better option

---

## Definition of Done

- [ ] Clean 440Hz sine wave audible through the Pico-Audio HAT
- [ ] Serial confirms sample rate (48kHz) and buffer size
- [ ] Zero DMA underruns during 60 seconds of playback
- [ ] No clicks, pops, or distortion
- [ ] Frequency verifiable with a tuner app (440Hz ±1Hz)
- [ ] `just pico-build` still produces a valid `.uf2`
- [ ] `just build && just test` (desktop) still passes

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Create | `/src/platform/pico/i2s.pio` | PIO I2S program |
| Create | `/src/platform/pico/audio_i2s_driver.h` | Audio driver header |
| Create | `/src/platform/pico/audio_i2s_driver.cpp` | Audio driver implementation |
| Modify | `/src/platform/pico/CMakeLists.txt` | Add audio sources + PIO/DMA libs |
| Modify | `/src/platform/pico/main.cpp` | Sine wave test |

---

## Troubleshooting Guide

### No sound at all
1. Check physical connections: DIN=GPIO26, BCK=GPIO27, LRCK=GPIO28
2. Check DAC power: Pico-Audio HAT needs 3.3V from Pico's VSYS
3. Check serial output: is "Playback started" printed?
4. Check with oscilloscope: probe BCK (GPIO27) — should see 1.536 MHz square wave

### Clicks or pops in audio
1. DMA underruns: check serial output for underrun count > 0
2. Phase discontinuity: ensure phase wraps cleanly (no floating-point drift)
3. Buffer boundary: check for off-by-one in audio callback loop

### Wrong pitch (not 440Hz)
1. Clock divider may be wrong — check `sys_clk` value printed in serial
2. Phase increment calculation: verify `2 * pi * 440 / 48000`
3. PIO program timing: verify 64 BCK cycles per stereo frame

### DMA underruns
1. Increase buffer size from 256 to 512 frames
2. Check that no other interrupts are blocking the DMA ISR
3. Ensure `sinf()` isn't using software emulation (check compiler output)

---

## Review Checklist for Reviewers

- [ ] PIO program matches I2S standard format (MSB-first, LRCK low=left)
- [ ] DMA ping-pong correctly chains A→B→A→B...
- [ ] Audio callback is called from ISR — verify no heap allocation or blocking calls
- [ ] Phase accumulator wraps correctly (no floating-point drift over time)
- [ ] Int16 conversion: `sample * 32767.0f` (not 32768 — avoid int16 overflow)
- [ ] Pin mapping matches Waveshare Pico-Audio HAT datasheet
- [ ] Clock divider calculation produces correct sample rate
- [ ] Desktop build and tests unaffected
