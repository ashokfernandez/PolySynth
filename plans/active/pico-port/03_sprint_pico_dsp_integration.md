# Sprint Pico-3: Core DSP Integration

**Depends on:** Sprint Pico-2
**Blocks:** Sprint Pico-4, Sprint Pico-5
**Approach:** TDD where possible — compile tests first, then auditory verification
**Estimated effort:** 2–3 days

---

## Goal

Port the core DSP library (SEA_DSP + core Engine + VoiceManager + Voice) to compile cleanly for the RP2350 ARM target. Connect the voice engine to the I2S audio pipeline and produce real synthesizer audio through the DAC. This sprint focuses on getting the core voice path working — effects come in Sprint 4.

### Philosophy: "Compile Everything, Deploy Core"

The full SEA_DSP library must compile for ARM Cortex-M33 with zero errors. This proves the codebase is truly portable. But the deployed binary in this sprint only includes: Oscillators, Filters, Envelopes, LFO, VoiceManager — no effects (Chorus, Delay, Limiter). Effects are handled in Sprint 4.

---

## Prerequisites

- Sprint Pico-2 complete (440Hz sine wave playing through DAC, zero underruns)
- Familiarity with `/src/core/Engine.h`, `Voice.h`, `VoiceManager.h`, `SynthState.h`
- Understanding of `sample_t` (float on Pico, double on desktop) and `sea::Real`

---

## Task 3.1: Create `pico_config.h` (Compile-Time Platform Defines)

### What

Create a single header that centralizes all Pico-specific compile-time configuration. This is the "source of truth" for embedded platform settings.

### File to Create

**`/src/platform/pico/pico_config.h`**

```cpp
#pragma once

// PolySynth Pico — Platform Configuration
// This header centralizes all compile-time settings for the RP2350 target.
// It is included by main.cpp and should NOT be included by core/ or libs/ code.
// Core/lib code uses the compile definitions set in CMakeLists.txt.

// ── Audio Configuration ─────────────────────────────────────────────────
#ifndef PICO_AUDIO_SAMPLE_RATE
#define PICO_AUDIO_SAMPLE_RATE 48000
#endif

#ifndef PICO_AUDIO_BUFFER_FRAMES
#define PICO_AUDIO_BUFFER_FRAMES 256
#endif

// ── DSP Configuration ───────────────────────────────────────────────────
// These are set via CMakeLists.txt target_compile_definitions, but we
// document them here for clarity:
//
// POLYSYNTH_USE_FLOAT      — sample_t = float (not double)
// SEA_PLATFORM_EMBEDDED    — sea::Real = float
// POLYSYNTH_MAX_VOICES=4   — 4 voices (not 16)

// ── Memory Budget (Reference — Not Enforced by Code) ────────────────────
// Voice array (4 voices):  ~2 KB
// VoiceManager + alloc:    ~0.5 KB
// DMA audio buffers:       ~2 KB
// Pico SDK overhead:       ~20-30 KB
// Stack:                   ~8 KB
// ──────────────────────────────────────
// Total estimate:          ~40 KB / 520 KB available
//
// Effects are disabled in this sprint:
// VintageChorus:           ~19 KB  (Sprint 4: deployable with static buffers)
// VintageDelay:            ~750 KB (IMPOSSIBLE at 2s delay — reduce to 50-100ms)
// LookaheadLimiter:        ~330 KB (IMPOSSIBLE — replace with soft clipper)

// ── Diagnostics ─────────────────────────────────────────────────────────
// Set to 1 to enable per-buffer CPU timing via serial
#ifndef PICO_ENABLE_DIAGNOSTICS
#define PICO_ENABLE_DIAGNOSTICS 1
#endif
```

### Acceptance Criteria

- [ ] File exists and documents all configuration
- [ ] No functional code — purely defines and documentation
- [ ] Memory budget documented for Sprint 4 reference

---

## Task 3.2: Fix SEA_DSP Compilation for ARM Target

### What

Build ALL SEA_DSP headers for the ARM Cortex-M33 target. Fix any compilation issues caused by:
- Double literals in float contexts (`2.0` → `2.0f`)
- Missing ARM intrinsics
- `std::pow(2.0, ...)` → `std::pow(2.0f, ...)`
- Any other portability issues

### Process

1. **Build with `-Wdouble-promotion`** to catch all float→double promotions
2. **Fix each warning** in SEA_DSP and core/ files
3. **Verify** desktop build still passes after changes

### Known Issues to Fix

**`/src/core/Voice.h`** — `std::pow(2.0, ...)` in frequency calculation:
```cpp
// BEFORE (line in NoteOn):
mFreq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);

// AFTER:
mFreq = static_cast<sample_t>(440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0));
```

**Note on frequency calculation precision:** The frequency calculation uses `double` math intentionally — this runs at note-on time (control rate), not per-sample. The result is cast to `sample_t` (float on Pico). This is correct: we want maximum precision for pitch calculation, then store as float for per-sample processing.

### Approach: Staged Verification

```bash
# Step 1: Try building — collect all errors
cmake --build build/pico 2>&1 | tee build_log.txt

# Step 2: Count warnings by type
grep -c "Wdouble-promotion" build_log.txt

# Step 3: Fix each warning, rebuild, repeat until clean
```

### TDD: Compile-Time Tests

The "test" for this task is that the build succeeds with zero errors and zero `-Wdouble-promotion` warnings. This is verified by:

```bash
# Must produce zero lines
cmake --build build/pico 2>&1 | grep -i "warning.*double-promotion"
```

### Review Guardrails

- **DO NOT** change `SynthState` fields from `double` to `float`. `SynthState` is control-rate (parameter updates at ~188Hz) — `double` precision is correct and costs nothing.
- **DO** fix per-sample DSP code (`Voice::Process()`, oscillators, filters) to use `float` literals.
- **DO NOT** suppress `-Wdouble-promotion` warnings — fix the source.
- **Exception:** `std::pow(2.0, ...)` in frequency calculation is intentionally `double` (control-rate). The cast to `sample_t` at the end is correct.

### Acceptance Criteria

- [ ] `cmake --build build/pico` succeeds with zero errors
- [ ] Zero `-Wdouble-promotion` warnings in per-sample DSP code
- [ ] `just build && just test` (desktop) still passes — no regressions
- [ ] ALL SEA_DSP headers compile for ARM (not just the ones we use)

---

## Task 3.3: Conditionally Compile Out Visualization Atomics

### What

`Engine.h` uses `std::atomic<uint64_t>` for thread-safe visualization state. On 32-bit ARM (Cortex-M33), 64-bit atomics may not be lock-free — they could use a hidden mutex, which is not RT-safe. We wrap these in `#ifndef SEA_PLATFORM_EMBEDDED`.

### File to Modify

**`/src/core/Engine.h`** — Wrap visualization members:

```cpp
// In the private section of Engine class:
#ifndef SEA_PLATFORM_EMBEDDED
    // Visualization state — lock-free atomics for audio→UI thread communication.
    // Disabled on embedded: std::atomic<uint64_t> may not be lock-free on 32-bit ARM.
    std::atomic<int> mVisualActiveVoiceCount{0};
    std::atomic<uint64_t> mVisualHeldNotesLow{0};
    std::atomic<uint64_t> mVisualHeldNotesHigh{0};
#endif
```

Also wrap the visualization methods:
```cpp
#ifndef SEA_PLATFORM_EMBEDDED
    int GetActiveVoiceCount() const { ... }
    std::vector<int> GetHeldNotes() const { ... }
    void UpdateVisualization() { ... }
#endif
```

And wrap the `UpdateVisualization()` call in `Process()`:
```cpp
void Process(sample_t &left, sample_t &right)
{
    mVoiceManager.ProcessStereo(left, right);
    // ... effects chain ...
#ifndef SEA_PLATFORM_EMBEDDED
    UpdateVisualization();
#endif
}
```

### Important: Desktop Builds Are Unaffected

`SEA_PLATFORM_EMBEDDED` is only defined in the Pico build. Desktop builds continue to compile all visualization code. This is a deployment guard, not a code deletion.

### TDD Verification

```bash
# Desktop: visualization still works
just build && just test

# Pico: compiles without std::atomic<uint64_t>
just pico-build
```

### Acceptance Criteria

- [ ] Pico build compiles without `std::atomic<uint64_t>` warnings or lock-free issues
- [ ] Desktop build unchanged — all visualization tests pass
- [ ] Guards use `#ifndef SEA_PLATFORM_EMBEDDED` (consistent with existing convention)

---

## Task 3.4: Connect Engine to Audio Callback

### What

Replace the sine wave test with the actual PolySynth engine. The DMA audio callback now calls `Engine::Process()` per sample and converts float output to int16 for the DAC.

### File to Modify

**`/src/platform/pico/main.cpp`** — Major update:

```cpp
// PolySynth Pico — Sprint 3: Core DSP Integration
// Full voice engine connected to I2S audio pipeline.

#include <cstdio>
#include <cstdint>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"

#include "audio_i2s_driver.h"
#include "pico_config.h"

// Core DSP engine — the same code that runs on desktop
#include "Engine.h"
#include "SynthState.h"

// ── Global engine instance ──────────────────────────────────────────────
// Static allocation — no heap. This is the same Engine class used on desktop.
static PolySynthCore::Engine s_engine;
static PolySynthCore::SynthState s_state;

// ── Per-buffer CPU profiling ────────────────────────────────────────────
static volatile uint32_t s_process_time_us = 0;

// ── Audio callback (called from DMA ISR) ────────────────────────────────
static void audio_callback(int16_t* buffer, uint32_t num_frames)
{
    uint32_t start = time_us_32();

    for (uint32_t i = 0; i < num_frames; i++) {
        float left = 0.0f;
        float right = 0.0f;

        s_engine.Process(left, right);

        // Soft-clip to prevent DAC overflow (simple tanh approximation)
        // tanhf is ~50 cycles on Cortex-M33 — acceptable at per-sample rate
        left  = tanhf(left);
        right = tanhf(right);

        // Float [-1.0, 1.0] → int16 [-32767, 32767]
        buffer[i * 2]     = static_cast<int16_t>(left  * 32767.0f);
        buffer[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
    }

    s_process_time_us = time_us_32() - start;
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
    printf("  Total SRAM:  %lu KB\n", static_cast<unsigned long>(total_sram / 1024));
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
    printf("  PolySynth Pico v0.3 — DSP Engine\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Voices:      %d (max)\n", POLYSYNTH_MAX_VOICES);
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Precision:   float (single)\n");
    printf("========================================\n\n");

    print_sram_report();

    // ── Initialize DSP engine ───────────────────────────────────────
    s_state.Reset();
    s_engine.Init(static_cast<double>(pico_audio::kSampleRate));
    s_engine.UpdateState(s_state);

    printf("DSP engine initialized.\n");

    // ── Initialize audio output ─────────────────────────────────────
    if (!pico_audio::Init(audio_callback)) {
        printf("ERROR: Audio init failed\n");
        return 1;
    }

    pico_audio::Start();
    printf("Audio started.\n\n");

    // ── Trigger a test note (Middle C = MIDI 60) ────────────────────
    printf("Playing Middle C (MIDI 60, velocity 100)...\n");
    s_engine.OnNoteOn(60, 100);

    // Let it play for 2 seconds, then release
    sleep_ms(2000);
    printf("Releasing note...\n");
    s_engine.OnNoteOff(60);

    // Wait for release tail
    sleep_ms(1000);

    // ── Play a 4-note chord to test polyphony ───────────────────────
    printf("\nPlaying C major chord (C4-E4-G4-C5)...\n");
    s_engine.OnNoteOn(60, 100);  // C4
    s_engine.OnNoteOn(64, 90);   // E4
    s_engine.OnNoteOn(67, 80);   // G4
    s_engine.OnNoteOn(72, 85);   // C5

    // ── Status reporting loop ───────────────────────────────────────
    uint32_t report_counter = 0;
    while (true) {
        sleep_ms(1000);
        report_counter++;

        float process_us = static_cast<float>(s_process_time_us);
        float buffer_us = static_cast<float>(pico_audio::kBufferFrames) * 1000000.0f /
                          static_cast<float>(pico_audio::kSampleRate);
        float cpu_percent = (process_us / buffer_us) * 100.0f;

        printf("[%lus] CPU: %.1f%% (%lu us/%lu us) | Underruns: %lu\n",
               static_cast<unsigned long>(report_counter),
               static_cast<double>(cpu_percent),
               static_cast<unsigned long>(s_process_time_us),
               static_cast<unsigned long>(static_cast<uint32_t>(buffer_us)),
               static_cast<unsigned long>(pico_audio::GetUnderrunCount()));

        // Blink LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, report_counter % 2);

        // After 10 seconds, release the chord and play something else
        if (report_counter == 10) {
            printf("\nReleasing chord...\n");
            s_engine.OnNoteOff(60);
            s_engine.OnNoteOff(64);
            s_engine.OnNoteOff(67);
            s_engine.OnNoteOff(72);
        }

        // After 13 seconds, test different waveform
        if (report_counter == 13) {
            printf("\nSwitching to square wave...\n");
            s_state.oscAWaveform = 1;  // Square
            s_engine.UpdateState(s_state);
            s_engine.OnNoteOn(60, 100);
        }

        if (report_counter == 18) {
            printf("\nReleasing and switching to triangle...\n");
            s_engine.OnNoteOff(60);
            s_state.oscAWaveform = 2;  // Triangle
            s_state.oscBWaveform = 0;  // Saw
            s_state.mixOscB = 0.5;     // Mix in Osc B
            s_engine.UpdateState(s_state);
            sleep_ms(500);
            s_engine.OnNoteOn(60, 100);
        }
    }

    return 0;
}
```

### What This Tests

The `main.cpp` runs an automated demo sequence that tests:

1. **Single note (Middle C):** Verifies pitch accuracy (261.6 Hz on a tuner)
2. **ADSR envelope:** Note-on attack transient, 2-second sustain, note-off release tail
3. **4-voice polyphony:** C major chord — all 4 voices active simultaneously
4. **CPU monitoring:** Per-buffer timing shows whether 4 voices fit in the budget
5. **Waveform switching:** Saw → Square → Triangle — verifies oscillator models work
6. **Oscillator mixing:** Both Osc A and Osc B active — verifies mixer

### TDD: What to Verify

| Test | How | Expected |
|------|-----|----------|
| Pitch accuracy | Tuner app | Middle C = 261.6 Hz ±1 Hz |
| ADSR envelope | Listen | Attack transient → sustain → release fade |
| 4-voice chord | Listen | All 4 notes audible simultaneously |
| CPU utilization | Serial output | < 30% with 4 voices |
| Waveform change | Listen | Saw (buzzy) → Square (hollow) → Triangle (mellow) |
| Zero underruns | Serial output | Underrun count = 0 over 60 seconds |

### Acceptance Criteria

- [ ] Middle C (261.6 Hz) audible with correct pitch
- [ ] ADSR envelope audible (attack transient + release tail)
- [ ] 4-voice chord plays cleanly (no voice stealing needed — we have exactly 4)
- [ ] Saw/Square/Triangle waveforms produce correct timbres
- [ ] Serial: CPU < 30%, zero underruns, 4 active voices
- [ ] `just build && just test` (desktop) still passes

---

## Task 3.5: Verify .elf Memory Map

### What

Check the compiled binary's memory usage to verify we're within the 520KB SRAM budget. The ARM linker produces a `.map` file that shows exactly where every symbol lives.

### How

```bash
# Enable map file generation — add to CMakeLists.txt:
# target_link_options(polysynth_pico PRIVATE -Wl,-Map=polysynth_pico.map)

# After building, check the map file:
arm-none-eabi-size build/pico/polysynth_pico.elf

# Expected output:
#    text    data     bss     dec     hex filename
#  XXXXX    XXXX   XXXXX   XXXXX   XXXXX polysynth_pico.elf
```

### What to Look For

| Section | Contains | Budget |
|---------|----------|--------|
| `.text` | Code (flash) | 2 MB flash available — no concern |
| `.data` | Initialized globals | Part of SRAM |
| `.bss` | Zero-initialized globals | Part of SRAM |
| `.data + .bss` | Total static SRAM | Must be < 100 KB |

### Key Verification

```bash
# Check for std::vector in the binary (should NOT appear in audio path)
arm-none-eabi-nm build/pico/polysynth_pico.elf | grep -i vector

# If std::vector symbols appear, investigate which source file includes them
```

### File to Modify

**`/src/platform/pico/CMakeLists.txt`** — Add map file generation:

```cmake
# Generate linker map file for memory analysis
target_link_options(polysynth_pico PRIVATE -Wl,-Map=polysynth_pico.map)
```

### Acceptance Criteria

- [ ] `data + bss` < 100 KB
- [ ] No `std::vector` symbols in audio path code
- [ ] SRAM usage printed via serial matches map file analysis
- [ ] Results documented for Sprint 4 effects planning

---

## Task 3.6: CPU Profiling

### What

Measure `Engine::Process()` time per buffer and report CPU utilization. This is already implemented in the audio callback (Task 3.4) — this task is about analyzing and documenting the results.

### Expected CPU Budget

| Component | Est. Cycles/Sample | Est. μs/Buffer (256 frames) |
|-----------|-------------------|----------------------------|
| 2 oscillators × 4 voices | ~200 | ~34 μs |
| 4 filter models × 4 voices | ~400 | ~68 μs |
| 2 ADSR envelopes × 4 voices | ~100 | ~17 μs |
| LFO × 4 voices | ~50 | ~8 μs |
| Panning (cosf/sinf) × 4 | ~400 | ~68 μs |
| **Total estimate** | **~1150** | **~195 μs** |

Buffer time at 48 kHz / 256 frames = 5333 μs → ~3.7% CPU estimated.

### TDD: Performance Budget Test

The performance "test" is:
```
Measured CPU% < 30% with 4 active voices
```

If CPU exceeds 30%, investigate:
1. Check for unintended `double` arithmetic (look for `Wdouble-promotion` warnings)
2. Check for software `sinf()`/`cosf()` emulation instead of FPU
3. Consider reducing to 2 voices temporarily

### Acceptance Criteria

- [ ] CPU < 30% with 4 active voices (all oscillators + filter + envelopes)
- [ ] Serial output shows per-buffer timing
- [ ] If CPU > 30%, performance investigation documented

---

## Definition of Done

- [ ] ALL SEA_DSP headers compile for ARM Cortex-M33 target (zero errors)
- [ ] Middle C (261.6 Hz) audible through DAC with correct pitch
- [ ] Saw/Square/Triangle waveforms produce correct timbres
- [ ] ADSR envelope audible (attack transient + release tail)
- [ ] 4-voice chord plays cleanly
- [ ] Serial: CPU < 30%, SRAM < 100 KB, zero underruns
- [ ] `just build && just test` (desktop) — zero regressions
- [ ] `.elf` map file shows no `std::vector` in audio path
- [ ] Visualization atomics conditionally compiled out on embedded

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Create | `/src/platform/pico/pico_config.h` | Centralized platform configuration |
| Modify | `/src/core/Engine.h` | `#ifndef SEA_PLATFORM_EMBEDDED` guards for visualization |
| Modify | `/src/core/Voice.h` | Fix double-promotion warnings (if any) |
| Modify | `/src/platform/pico/main.cpp` | Engine integration + test sequence |
| Modify | `/src/platform/pico/CMakeLists.txt` | Map file generation, core/ includes |

---

## Troubleshooting Guide

### "undefined reference to `PolySynthCore::Engine::Init`"
→ Engine.h is header-only in this project. Ensure `target_include_directories` includes `src/core/`.

### "`std::atomic<uint64_t>` is not lock-free" warning
→ This is expected on 32-bit ARM. Task 3.3 wraps these in `#ifndef SEA_PLATFORM_EMBEDDED`.

### "-Wdouble-promotion: implicit conversion from 'float' to 'double'"
→ Fix by using `f` suffix on float literals: `2.0` → `2.0f`, `0.5` → `0.5f`. Only in per-sample code — control-rate `double` is fine.

### CPU% > 30%
→ Check: (1) `-Wdouble-promotion` warnings remaining? (2) Is `cosf()`/`sinf()` in panning using FPU or software? (3) Try reducing to 2 voices. (4) Check if `tanhf()` soft clipper is expensive.

### No sound from engine (was working with sine wave)
→ Check: (1) `s_engine.Init()` called before audio start? (2) `s_engine.OnNoteOn()` called? (3) `s_state.masterGain` is non-zero (default 0.75)? (4) ADSR attack not too long?

---

## Review Checklist for Reviewers

- [ ] `#ifndef SEA_PLATFORM_EMBEDDED` guards only wrap visualization — not DSP code
- [ ] No changes to `SynthState` field types (must remain `double` for control-rate precision)
- [ ] `-Wdouble-promotion` fixes only affect per-sample code, not control-rate code
- [ ] Engine::Process() call is per-sample (not per-buffer) — matching desktop behavior
- [ ] `tanhf()` soft clipper is temporary — replaced by proper limiter system in Sprint 4
- [ ] `just build && just test` passes — no desktop regressions
- [ ] Map file analysis included in PR description (data + bss < 100 KB)
