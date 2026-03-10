# Pico DSP Architecture — Embedded Platform Foundation

> **This document describes the target architecture after Sprints 7-10.** Not all
> components exist yet. CMake snippets, task lists, and file structures shown here
> represent the end state. For what exists today, see the individual sprint documents.
> If a sprint document contradicts this document on a specific config value or API,
> the sprint document wins for its own scope.

## Table of Contents

- [Problem Statement](#problem-statement)
- [Design Goals](#design-goals)
- [Architecture Overview](#architecture-overview)
- [Design Decision: FreeRTOS on Core 0](#design-decision-freertos-on-core-0-bare-metal-core-1)
- [Signal Path Architecture](#signal-path-architecture)
- [CPU Budget Analysis](#cpu-budget-analysis)
- [DSP Optimizations](#dsp-optimizations)
- [File Structure](#file-structure)
- [FreeRTOS Configuration](#freertos-configuration)
- [Inter-Core Communication](#inter-core-communication)
- [Memory Budget](#memory-budget)
- [Risk Register](#risk-register)

## Problem Statement

The current Pico port (`main.cpp`, 670 lines) is a working proof-of-concept but has
accumulated into a monolith that mixes audio callbacks, command dispatch, demo
sequencing, state handoff, self-tests, and the main loop into a single file with
ad-hoc inter-core communication. This creates several problems:

1. **Duplication** — Custom SPSC queue duplicates `SPSCQueue.h`; manual `set_param`/`get_param` with 20+ strcmp chains will diverge from `SynthState` as fields are added
2. **No task scheduling** — Core 0 busy-loops polling serial, wasting power and making it impossible to add USB MIDI, ADC, display, or WiFi without creating priority inversions
3. **No architectural parity with desktop** — iPlug2 provides a clean "application" class that owns the Engine, manages state transfer, and bridges I/O; the Pico has no equivalent
4. **Untestable** — Everything is tangled in `main()`, making unit testing of individual subsystems impossible
5. **CPU headroom** — The ladder filter alone costs ~200 cycles/sample/voice (6 tanh calls: input + feedback + 4 stages), leaving less room than necessary for effects and higher voice counts

## Design Goals

1. **One codebase** — All DSP code in `src/core/` and `libs/SEA_DSP/` remains 100% shared. Zero Pico-specific DSP.
2. **Platform glue parity** — Create a `PicoSynthApp` class that does for Pico what `PolySynth.cpp` (iPlug2) does for desktop: owns Engine, manages state, bridges I/O.
3. **FreeRTOS from the start** — The target is a full standalone instrument with knobs, screens, USB MIDI, UART MIDI, WiFi, and Bluetooth. FreeRTOS provides the preemptive priority scheduling, driver integration (CYW43, TinyUSB, lwIP), and task isolation these require. FreeRTOS runs on **Core 0 only** (`configNUMBER_OF_CORES=1`) due to known SMP spinlock hardfault bugs in the RP2350 port. Core 1 stays bare-metal for audio.
4. **Leverage RP2350 fully** — Dedicated audio core, hardware FPU, hard-float ABI, DSP instructions, DMA-driven I2S, lookup tables for expensive transcendentals.
5. **No embedded-specific duplication** — Reuse existing core infrastructure (SPSCQueue, SynthState).
6. **CPU efficiency** — Identify and address the per-voice cycle budget bottlenecks to maximize polyphony and headroom for effects.

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────────┐
│                     CORE 0 — FreeRTOS Control Plane                      │
│                                                                          │
│  ┌────────────────┐  ┌────────────────┐  ┌─────────────────────────┐    │
│  │ Serial Task    │  │ MIDI Task      │  │ HID Task                │    │
│  │ Pri 3 (Normal) │  │ Pri 4 (High)   │  │ Pri 2 (Low-Normal)      │    │
│  │                │  │                │  │                         │    │
│  │ USB CDC serial │  │ USB MIDI dev   │  │ ADC knobs (100Hz)       │    │
│  │ Command parse  │  │ UART MIDI      │  │ Button debounce         │    │
│  │ Response print │  │ BLE MIDI       │  │ Encoder reading         │    │
│  └───────┬────────┘  └───────┬────────┘  └────────────┬────────────┘    │
│          │                   │                        │                  │
│          └───────────────────┼────────────────────────┘                  │
│                              ▼                                           │
│                   ┌─────────────────────┐                                │
│                   │   PicoSynthApp      │                                │
│                   │   (Platform Glue)   │                                │
│                   │                     │                                │
│                   │  • Owns Engine      │  ◄── Mirrors what iPlug2's    │
│                   │  • Owns SynthState  │      PolySynth class does     │
│                   │  • Event dispatch   │      for desktop               │
│                   │  • State bridging   │                                │
│                   └────────┬────────────┘                                │
│                            │                                             │
│  ┌─────────────────┐      │ SPSCQueue<Cmd>     ┌───────────────────┐    │
│  │ Display Task    │      │ SPSCQueue<State>    │ Radio Task        │    │
│  │ Pri 1 (Low)     │      │                     │ Pri 2 (Low-Normal)│    │
│  │                 │      │                     │                   │    │
│  │ SPI DMA to LCD  │      │                     │ CYW43 WiFi poll   │    │
│  │ 20-30 Hz        │      │                     │ BLE advertising   │    │
│  │ UI state render  │      │                     │ OSC/Web server    │    │
│  └─────────────────┘      │                     └───────────────────┘    │
│                            │                                             │
│  ┌─────────────────┐      │                     ┌───────────────────┐    │
│  │ Status Task     │      │                     │ Idle Hook         │    │
│  │ Pri 0 (Idle+)   │      │                     │ Pri 0 (Idle)      │    │
│  │ Diagnostics 5s  │      │                     │ LED heartbeat     │    │
│  └─────────────────┘      │                     │ Power management  │    │
│                            │                     └───────────────────┘    │
└────────────────────────────┼─────────────────────────────────────────────┘
                             │ Lock-free queues (no FreeRTOS primitives needed)
┌────────────────────────────┼─────────────────────────────────────────────┐
│                            ▼              CORE 1 (Bare-Metal Audio DSP)   │
│                                                                           │
│  NO FreeRTOS here. Launched via multicore_launch_core1().                 │
│  Runs __wfi() + DMA ISR only. Zero scheduler overhead.                    │
│                                                                           │
│  ┌────────────────────────────────────────────────────────────────────┐   │
│  │  DMA ISR (audio_callback) — highest priority, not a FreeRTOS task  │   │
│  │                                                                    │   │
│  │  1. Drain command queue (NOTE_ON/OFF, UPDATE_STATE, PANIC)         │   │
│  │  2. Engine::Process(left, right) × 256 frames                      │   │
│  │  3. OutputStage: gain → soft-clip → SSAT → PackI2S                 │   │
│  │  4. Update diagnostics (voice count, peak level)                   │   │
│  └────────────────────────────────────────────────────────────────────┘   │
│                                                                           │
│  Bare-metal __wfi() loop between DMA interrupts                           │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
```

---

## Design Decision: FreeRTOS on Core 0, Bare-Metal Core 1

**Decision: FreeRTOS on Core 0 only (`configNUMBER_OF_CORES=1`). Core 1 stays bare-metal for audio.**

### Why FreeRTOS (not pico-rtos or custom):

| Factor | FreeRTOS | Alternatives |
|--------|----------|-------------|
| **Pico SDK integration** | First-party, bundled in SDK | pico-rtos: 2 GitHub stars, zero SDK integration |
| **CYW43 WiFi/BLE** | `pico_cyw43_arch_lwip_sys_freertos` — official driver mode | Would need custom driver integration |
| **TinyUSB** | Integrated for USB MIDI host/device | Would need custom integration |
| **lwIP TCP/IP** | Thread-safe mode designed for FreeRTOS | Would need custom integration |
| **Maturity** | 20+ years, billions of devices, IEC 62304 certifiable | No alternatives come close |
| **Community** | Every embedded problem has a solved example | One author, minimal docs |

### Why Core 0 only (NOT SMP on both cores):

The RP2350 FreeRTOS SMP port has **known spinlock hardfault bugs** ([reported](https://forums.raspberrypi.com/viewtopic.php?p=2305280)) when ISRs fire on Core 1 while Core 0 holds a FreeRTOS spinlock. This crashes in `taskEXIT_CRITICAL_FROM_ISR()` or `PendSV_Handler` under load — exactly the scenario we'd hit with DMA audio ISRs on Core 1.

The RP2350 port README **explicitly supports** running FreeRTOS on Core 0 only with bare-metal code on Core 1, communicating via Pico SDK sync primitives. This is a first-class configuration, not a workaround.

| Factor | FreeRTOS SMP (both cores) | FreeRTOS Core 0 only |
|--------|--------------------------|---------------------|
| **Stability** | Known spinlock hardfault bugs under ISR load | Zero SMP port bugs — single-core kernel |
| **Audio latency** | ISR + FreeRTOS scheduler interaction | Zero — DMA ISR is the only code on Core 1 |
| **Complexity** | Core affinity, SMP-aware critical sections | Simple: FreeRTOS on Core 0, bare-metal Core 1 |
| **Inter-core comm** | FreeRTOS queues (overhead + ISR-safe variants) | Lock-free SPSCQueue (zero overhead, proven) |
| **SDK interop** | `configSUPPORT_PICO_SYNC_INTEROP` required | Same — works between FreeRTOS tasks and Core 1 |
| **Core 1 startup** | FreeRTOS manages both cores | `multicore_launch_core1()` — existing pattern |

### How audio stays deterministic:

The DMA ISR is **not** a FreeRTOS task. It's a hardware interrupt on Core 1 where FreeRTOS doesn't run at all:

```
Core 1: multicore_launch_core1() → __wfi() loop
        DMA hardware fires → DMA ISR runs → fills buffer → returns to __wfi()
        ↑ No FreeRTOS on this core. No scheduler. No spinlocks. No contention.
```

### Critical RP2350 FreeRTOS port requirements:

**Must use Raspberry Pi's FreeRTOS-Kernel fork** (`raspberrypi/FreeRTOS-Kernel`), not mainline FreeRTOS. The mainline `ARM_CM33_NTZ/non_secure` port builds but crashes in `crt0.S` on RP2350.

```c
// Required FreeRTOSConfig.h defines for RP2350
#define configNUMBER_OF_CORES                1      // Core 0 only — avoids SMP bugs
#define configTICK_CORE                      0
#define configRUN_MULTIPLE_PRIORITIES        0      // Single-core: not applicable
#define configUSE_CORE_AFFINITY              0      // Single-core: not applicable
#define configSUPPORT_PICO_SYNC_INTEROP      1      // REQUIRED: SDK mutex/semaphore interop
#define configSUPPORT_PICO_TIME_INTEROP      1      // REQUIRED: SDK sleep_ms() interop
#define configENABLE_FPU                     1      // REQUIRED: save/restore FPU on context switch
#define configENABLE_MPU                     0      // Not supported on this port
#define configENABLE_TRUSTZONE               0      // Not supported on this port
#define configRUN_FREERTOS_SECURE_ONLY       1      // RP2350 runs in secure mode
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16     // Only tested value — do not change
```

### Known RP2350 FreeRTOS gotchas:

1. **TinyUSB `vTaskDelay` bug** — `vTaskDelay(pdMS_TO_TICKS(1))` delays far too long on RP2350, causing USB enumeration to take ~15s. Use `tud_task_ext(timeout, false)` or `portYIELD()` in USB task loop.
2. **GPIO init before scheduler** — Call all `gpio_init()` before `vTaskStartScheduler()`. Cross-core GPIO init can hang.
3. **Tickless idle unsupported** — Don't enable `configUSE_TICKLESS_IDLE`.
4. **Stack sizes in words** — `configMINIMAL_STACK_SIZE = 256` means 1024 bytes (not 256 bytes).
5. **Hard-float ABI** — Ensure `configENABLE_FPU = 1` so context switches preserve FPU registers with our `-mfloat-abi=hard`.

---

## Design Decision: FreeRTOS Task Hierarchy

### Task Priority Map

```
Priority 5 (configMAX_PRIORITIES - 1)  ── MIDI Input Task
Priority 4                              ── Serial Command Task
Priority 3                              ── HID/ADC Task (knobs, buttons)
Priority 2                              ── Radio Task (WiFi, BLE)
Priority 1                              ── Display Task (SPI LCD/OLED)
Priority 0 (tskIDLE_PRIORITY)          ── Idle hook (LED, power mgmt)

Above all tasks (hardware interrupt):   ── DMA Audio ISR on Core 1
```

### Priority Rationale

| Task | Priority | Why | Core | Stack | Rate |
|------|----------|-----|------|-------|------|
| **MIDI Input** | 5 (Highest) | MIDI timing jitter is directly audible. Must process within ~1ms of arrival. USB MIDI + UART MIDI polling. | 0 | 512 words | Blocks on event, wakes on USB/UART IRQ |
| **Serial Command** | 4 (High) | Interactive control needs responsive feedback. Serial polling + command dispatch. | 0 | 1024 words | Blocks on `getchar`, wakes on USB CDC |
| **HID / ADC** | 3 (Normal) | Potentiometer readings need smoothing but aren't as time-critical as MIDI. ~100Hz scan, debounce, deadzone. | 0 | 512 words | 10ms period via `vTaskDelay` |
| **Radio** | 2 (Low-Normal) | WiFi/BLE is background communication. CYW43 has its own interrupt handling but needs periodic servicing. | 0 | 2048 words | Event-driven + periodic poll |
| **Display** | 1 (Low) | Visual feedback. 20-30Hz refresh via SPI DMA. Humans can't perceive faster. Can be preempted by everything. | 0 | 1024 words | 33ms period via `vTaskDelay` |
| **Status** | 0+ | Pure diagnostics logging every 5s. Runs when nothing else needs CPU. | 0 | 256 words | 5000ms period |
| **Idle** | 0 | LED heartbeat toggle. FreeRTOS idle hook on Core 0. | 0 | N/A | Runs when CPU idle |

### Stack size rationale:
- Radio task needs 2048 words (8KB) because lwIP/CYW43 use deep call stacks
- Serial needs 1024 words (4KB) for printf formatting buffers
- MIDI/HID need only 512 words (2KB) — tight, focused tasks
- Display needs 1024 words for framebuffer manipulation
- Total FreeRTOS stack: ~24KB (of 520KB SRAM) — comfortable

### FreeRTOS Configuration

```c
// FreeRTOSConfig.h — Full configuration for RP2350 (Core 0 only)

// ── RP2350 Port Requirements (do not change) ──────────────────────
#define configNUMBER_OF_CORES               1       // Core 0 only — avoids SMP spinlock bugs
#define configTICK_CORE                     0
#define configENABLE_FPU                    1       // REQUIRED: -mfloat-abi=hard context switches
#define configENABLE_MPU                    0       // Not supported on this port
#define configENABLE_TRUSTZONE              0       // Not supported on this port
#define configRUN_FREERTOS_SECURE_ONLY      1       // RP2350 runs in secure mode
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16     // Only tested value
#define configSUPPORT_PICO_SYNC_INTEROP     1       // SDK mutex/semaphore interop with Core 1
#define configSUPPORT_PICO_TIME_INTEROP     1       // SDK sleep_ms() interop

// ── Scheduler ─────────────────────────────────────────────────────
#define configMAX_PRIORITIES                6
#define configMINIMAL_STACK_SIZE            256     // words (1KB) for idle task
#define configTOTAL_HEAP_SIZE               (48 * 1024)  // 48KB FreeRTOS heap
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1
#define configTICK_RATE_HZ                  1000    // 1ms tick resolution

// ── Features ──────────────────────────────────────────────────────
#define configUSE_IDLE_HOOK                 1       // LED heartbeat
#define configUSE_TICK_HOOK                 0
#define configCHECK_FOR_STACK_OVERFLOW      2       // Full pattern check (debug)
#define configUSE_MUTEXES                   1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_TASK_NOTIFICATIONS        1       // Lightweight IPC between Core 0 tasks
#define configUSE_TIMERS                    1       // Software timers for periodic status
#define configTIMER_TASK_PRIORITY           (configMAX_PRIORITIES - 1)
#define configTIMER_TASK_STACK_DEPTH        1024
#define configSUPPORT_STATIC_ALLOCATION     1
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configUSE_TICKLESS_IDLE             0       // NOT supported on RP2350 port
```

### CMakeLists.txt Changes

```cmake
# ── FreeRTOS kernel ──────────────────────────────────────────────────────
# IMPORTANT: Must use Raspberry Pi's fork, NOT mainline FreeRTOS.
# The mainline ARM_CM33_NTZ port crashes in crt0.S on RP2350.
# Add as git submodule (follows existing external/ convention for third-party deps):
#   git submodule add https://github.com/raspberrypi/FreeRTOS-Kernel.git external/FreeRTOS-Kernel
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

target_link_libraries(polysynth_pico PRIVATE
    pico_stdlib
    pico_multicore                       # KEEP: Core 1 bare-metal via multicore_launch_core1()
    pico_cyw43_arch_lwip_sys_freertos    # CYW43 + lwIP with FreeRTOS threading (replaces _none)
    FreeRTOS-Kernel-Heap4                # FreeRTOS memory allocator
    hardware_pio
    hardware_dma
    hardware_clocks
    hardware_adc                          # For potentiometer reading (future)
    SEA_DSP
    SEA_Util
)
```

### Task Creation Pattern

Each task follows the same pattern — a standalone function with a `void*` context parameter:

```cpp
// Example: Serial command task
void serial_task(void* ctx) {
    auto* app = static_cast<PicoSynthApp*>(ctx);
    pico_serial::CommandParser parser;

    while (true) {
        int ch = getchar_timeout_us(1000);  // Block up to 1ms
        if (ch != PICO_ERROR_TIMEOUT) {
            if (parser.Feed(static_cast<char>(ch))) {
                auto cmd = parser.Parse();
                dispatch_command(*app, cmd);
            }
        }
        // FreeRTOS preemption handles yielding to higher-priority tasks
    }
}

// In main():
xTaskCreate(serial_task, "serial", 1024, &s_app, 4, NULL);
```

---

## Design Decision: Core Assignment

**Decision: Dedicate Core 1 to audio DSP via DMA ISR (bare-metal). FreeRTOS runs on Core 0 only.**

### Why this is correct:

| Factor | Analysis |
|--------|----------|
| **Latency** | DMA ISR is the highest-priority execution context on Core 1's Cortex-M33. Nothing can preempt it. Guarantees buffer fills complete before the next DMA transfer. |
| **Determinism** | FreeRTOS doesn't run on Core 1 at all. No scheduler, no spinlocks, no contention. Core 1's instruction cache, data cache, and FPU are exclusively for audio. |
| **Stability** | Avoids known RP2350 FreeRTOS SMP spinlock hardfault bugs that crash under ISR load on Core 1. |
| **Simplicity** | No priority inheritance, no mutex, no critical sections in the audio path. The DMA ISR is the only code on Core 1. `multicore_launch_core1()` — the existing proven pattern. |
| **Power** | `__wfi()` puts Core 1 in lowest-power wait state between buffer fills. Wakes only when DMA needs a new buffer (~every 5.3ms at 48kHz/256 frames). |

### Core setup implementation:

```cpp
// All FreeRTOS tasks run on Core 0 automatically (configNUMBER_OF_CORES=1)
// No core affinity needed — FreeRTOS only knows about Core 0
xTaskCreate(serial_task, "serial", 1024, &s_app, 4, NULL);
xTaskCreate(status_task, "status", 256, &s_app, 1, NULL);

// Core 1 is bare-metal, launched before FreeRTOS starts
multicore_launch_core1(core1_audio_entry);  // Existing pattern, unchanged

// Audio DMA IRQ initialized on Core 1 inside core1_audio_entry()
// FreeRTOS never sees or touches Core 1
```

---

## Design Decision: Signal Path — Per-Voice Filtering, Single Filter Type

### Decision: Keep per-voice filtering. Eliminate per-voice filter bloat by choosing ONE filter type at compile time for embedded.

### Why per-voice filtering (not post-mix):

Moving to post-mix filtering would make PolySynth **paraphonic, not polyphonic**. This
is not just a technical distinction — it fundamentally changes how the instrument plays:

| Aspect | Per-voice filter (current) | Post-mix filter |
|--------|--------------------------|-----------------|
| **Filter envelope** | Each note gets its own attack/decay/sustain/release. New notes in a chord have independent timbral evolution. | ONE envelope for everything. Adding a note either retriggars (disrupts held notes) or doesn't (new note has no attack). |
| **Key tracking** | Filter cutoff follows pitch per-voice (higher notes = brighter). Standard polyphonic behavior. | Must choose which note to track — highest? lowest? last? Any choice is a compromise. |
| **Resonance** | Each voice has its own resonant peak at its own cutoff. Rich, spread-out harmonic result. | One peak across the entire mix. Thinner character. |
| **Filter saturation** | Each voice driven into filter independently — classic analog distortion per voice. | Summed signal drives filter — different (punchier but less nuanced) saturation character. |
| **Musical result** | Behaves like Prophet-5, Jupiter-8, Juno-106 — every classic polysynth used this. | Behaves like Korg Mono/Poly, ARP Omni — explicitly "paraphonic" instruments. |
| **Architecture class** | Polyphonic synthesizer | Paraphonic synthesizer |

**Every classic polysynth** (Prophet-5, Jupiter-8, Juno-106, OB-X, Polysix, Memorymoog)
used per-voice filters. It was considered the defining feature separating polysynths from
paraphonic instruments. Modern digital synths (Surge, Vital, Serum) also use per-voice
filters as the default architecture.

### The real problem: carrying all 3 filter types per voice

The current `Voice` class instantiates ALL THREE filter types:
```cpp
sea::BiquadFilter<sample_t> mFilter;         // ~64 bytes
sea::LadderFilter<sample_t> mLadderFilter;   // ~128 bytes (4 TPT integrators)
sea::CascadeFilter<sample_t> mCascadeFilter; // ~256 bytes (2 SVFs)
```
Only ONE is used at runtime (selected by `mFilterModel`). The unused filters waste
~300-400 bytes of cache per voice, bloat the Voice struct, and pollute D-cache lines
that could hold active data.

### Fix: Single filter type at compile time for embedded

```cpp
// In Voice.h — selected by POLYSYNTH_FILTER_MODEL compile flag
#if POLYSYNTH_FILTER_MODEL == 0
    sea::BiquadFilter<sample_t> mFilter;     // 10-15 cy/sample, clean digital
#elif POLYSYNTH_FILTER_MODEL == 1
    sea::LadderFilter<sample_t> mFilter;     // 60-70 cy/sample, rich analog
#elif POLYSYNTH_FILTER_MODEL == 2
    sea::CascadeFilter<sample_t> mFilter;    // 40-50 cy/sample (12dB), warm
#elif POLYSYNTH_FILTER_MODEL == 3
    sea::CascadeFilter<sample_t> mFilter;    // 80-90 cy/sample (24dB), bold
#endif
```

**Desktop** keeps runtime selection (all 3 filters instantiated, user chooses in UI).
**Embedded** uses compile-time selection — one filter type, no waste.

### Recommended embedded default: Cascade12 (SVF 2-pole)

| Filter | Cost/voice/sample | Character | Recommendation |
|--------|------------------|-----------|----------------|
| Biquad | 10-15 cycles | Clean, digital, no saturation | Too "cold" for analog synth |
| **Cascade12** | **40-50 cycles** | **Warm, slight saturation, efficient** | **Best balance for embedded** |
| Cascade24 | 80-90 cycles | Bold, analog, good saturation | Viable at 4 voices (320-360 cy total) |
| Ladder | 60-70 cycles (with LUT) | Rich, classic Moog, 6× tanh | Premium option, tight at 4 voices |

Cascade12 at 4 voices = 160-200 cycles/sample total. Well within budget.

### Signal path (per voice):

```
                        PER VOICE (×4 on embedded)
                 ┌─────────────────────────────────────────┐
                 │                                         │
  NoteOn ───────►│  OscA ──┐                               │
                 │         ├── Mixer ── Filter ── AmpEnv ──┼──┐
  Detune ───────►│  OscB ──┘    ↑         ↑         ↑      │  │
                 │              │         │         │      │  │
  LFO ──────────►│     (mix A/B) (cutoff+  (velocity │      │  │
                 │              │  env+LFO) × env)   │      │  │
                 │              │         │         │      │  │
  FilterEnv ────►│              └─ poly-mod ────────┘      │  │
                 │                                         │  │
                 └─────────────────────────────────────────┘  │
                                                              │
                        POST-MIX (×1)                         ▼
                 ┌─────────────────────────────────────────────┐
                 │                                             │
                 │  Voice Sum ── Headroom ── Gain ──           │
                 │  (4 voices    (1/√N)      (master)          │
                 │   with pan)                                 │
                 │                     ↓                       │
                 │              Soft Clip (fast_tanh)           │
                 │                     ↓                       │
                 │              SSAT → PackI2S → DMA buffer     │
                 │                                             │
                 └─────────────────────────────────────────────┘
```

This is the **standard polyphonic synthesizer signal path** — identical in structure
to the Prophet-5, Jupiter-8, and every modern virtual analog. Per-voice filtering is
NOT optional for a polyphonic instrument — it is the architecture.

### What post-mix shared processing IS appropriate for:

- **Output soft clipping** (already done: `fast_tanh` on the stereo mix)
- **Effects** (chorus, delay, reverb — these operate on the mixed signal)
- **Master EQ** (a gentle tilt or presence filter on the output, not per-voice)
- **Limiting / compression** (output stage protection)

None of these should replace the per-voice filter — they are complementary.

---

## CPU Budget Analysis — Per-Voice Cycle Breakdown

### Where the cycles actually go (per sample, per voice):

| Component | Cycles | % | Notes |
|-----------|--------|---|-------|
| **Ladder Filter Process()** | **180-220** | **~40%** | 6× tanh (Padé): input, feedback, 4 stages |
| LFO (sine, fast math) | 50-70 | ~12% | 7th-order polynomial sin() via SEA_FAST_MATH |
| OscA (saw) + FM routing | 30-40 | ~7% | Phase accumulator + modulation math |
| OscB (saw) | 20-25 | ~5% | Phase accumulator + waveform |
| Filter SetParams (amortized) | 10-30 | ~5% | Only when cutoff/resonance change. Includes tan() |
| Filter Envelope | 15-20 | ~4% | Linear interpolation per stage |
| Amp Envelope | 15-20 | ~4% | Linear interpolation per stage |
| Mixer + final amp + velocity | 20-30 | ~5% | Multiply-accumulate chain |
| Admin (age, active check, branch) | 10-15 | ~3% | |
| **Total (saw osc, ladder filter)** | **~480-520** | | |

### Alternative filter costs:

| Filter Model | Process Cycles | vs Ladder | Notes |
|-------------|----------------|-----------|-------|
| **Ladder** (4-pole, 6× tanh) | 180-220 | baseline | Rich analog character, most expensive |
| **Cascade24** (4-pole SVF, 2× tanh) | 85-100 | ~55% cheaper | Good analog character, much cheaper |
| **Cascade12** (2-pole SVF, 1× tanh) | 40-50 | ~78% cheaper | Lighter, still warm |
| **Biquad** (classic IIR) | 10-12 | ~95% cheaper | Clean digital, very efficient |

### Voice count vs CPU at 48kHz / 150MHz:

```
Cycles per sample = 150,000,000 / 48,000 = 3,125

                      Ladder    Cascade24   Biquad
                      (~500cy)  (~380cy)    (~310cy)
4 voices:             64%       49%         40%
6 voices:             96%  ⚠️   73%         60%
8 voices:             128% ❌    97%  ⚠️     79%
10 voices:            —         —           99%  ⚠️
```

### Optimization Opportunities (ranked by cycle savings)

#### 1. Tanh Lookup Table — Saves ~50 cycles/voice/sample

**The single biggest win.** The ladder filter calls tanh 6× per sample (input + feedback + 4 stages). Even with the Padé approximant (~15 cycles each), that's 90 cycles in tanh alone.

A 256-entry lookup table with linear interpolation costs ~5 cycles per lookup (table fetch + lerp). For 5 lookups: 25 cycles vs 75 — a saving of **50 cycles per voice per sample**.

```cpp
// In sea_math.h — compile-switchable, identical sound character
#ifdef SEA_USE_TANH_LUT
namespace detail {
    // 256 entries covering [-4, +4], linearly interpolated
    // Table generated at compile time or Init()
    alignas(16) static float kTanhLUT[257]; // +1 for lerp boundary
    static bool sLUTInitialized = false;

    inline void InitTanhLUT() {
        for (int i = 0; i <= 256; ++i) {
            float x = (static_cast<float>(i) / 256.0f) * 8.0f - 4.0f;
            kTanhLUT[i] = std::tanh(x);  // Computed once at startup
        }
        sLUTInitialized = true;
    }
}

SEA_INLINE float Tanh(float x) {
    // Clamp to table range
    float ax = x < 0 ? -x : x;
    if (ax >= 4.0f) return x < 0 ? -1.0f : 1.0f;
    // Table index + lerp
    float idx = ax * (256.0f / 8.0f) + 128.0f;  // Map [-4,4] to [0,256]
    int i = static_cast<int>(idx);
    float frac = idx - static_cast<float>(i);
    float result = kTanhLUT[i] + frac * (kTanhLUT[i + 1] - kTanhLUT[i]);
    return x < 0 ? -result : result;
}
#endif
```

**Impact**: This is a **universal optimization** — the LUT tanh is indistinguishable from std::tanh at 8-bit table resolution with linear interpolation (error < 0.1%). Safe to use on both desktop and embedded.

#### 2. Sine Wavetable for LFO — Saves ~40 cycles/voice/sample

The LFO runs every sample when active. The current 7th-order polynomial sin() costs 50-70 cycles. A 1024-entry sine wavetable with lerp costs ~8 cycles:

```cpp
// In sea_math.h or sea_lfo.h
#ifdef SEA_USE_SINE_LUT
alignas(16) static float kSineLUT[1025]; // One full cycle + boundary

SEA_INLINE float SinLUT(float phase) {
    // phase in [0, 1)
    float idx = phase * 1024.0f;
    int i = static_cast<int>(idx);
    float frac = idx - static_cast<float>(i);
    return kSineLUT[i] + frac * (kSineLUT[i + 1] - kSineLUT[i]);
}
#endif
```

**Impact**: Universal optimization. 1024-point sine with lerp has <0.001% error. Identical to std::sin for all practical purposes. **4KB ROM cost** (1025 × 4 bytes).

#### 3. Default to Cascade24 on Embedded — Saves ~120 cycles/voice/sample

The Cascade24 (4-pole SVF) provides rich analog character with only 2× tanh vs the Ladder's 6×. On embedded, default `filterModel` to Cascade24 and offer Ladder as a "HQ" mode with automatic voice reduction:

```cpp
// In pico_config.h or SynthState defaults
#ifdef SEA_PLATFORM_EMBEDDED
    static constexpr int kDefaultFilterModel = 3;  // Cascade24
#else
    static constexpr int kDefaultFilterModel = 1;  // Ladder
#endif
```

This is a **compile-guarded behavioral change** — the sound character differs between models, so this must be explicit.

#### 4. Revised CPU Budget with Optimizations

With tanh LUT + sine LUT (universal) + Cascade24 default (embedded):

```
Per-voice (Cascade24 + LUTs):    ~290 cycles/sample
Per-voice (Ladder + LUTs):       ~430 cycles/sample

                      Cascade24+LUT  Ladder+LUT
4 voices:             37%            55%
6 voices:             56%            83%
8 voices:             74%            ⚠️ 110%
10 voices:            93%            —
```

**8 voices with Cascade24 at 74% CPU** — comfortable headroom for effects, with the ladder available as a quality option at reduced polyphony.

---

## Design Decision: State Transfer (Core 0 → Core 1)

**Decision: Use the existing `SPSCQueue<T>` from `src/core/SPSCQueue.h` — NOT FreeRTOS queues.**

### Why SPSCQueue over FreeRTOS xQueue:

| Factor | SPSCQueue | FreeRTOS xQueue |
|--------|-----------|-----------------|
| **ISR safety** | Lock-free, no critical sections | Uses `taskENTER_CRITICAL_FROM_ISR()` which briefly disables interrupts |
| **Latency** | Zero overhead — atomic load/store only | ~1-2µs per operation (critical section + copy) |
| **Proven** | Already used on desktop + validated on Cortex-M33 | Would need testing |
| **Dependencies** | None (standalone header) | Requires FreeRTOS kernel |

The audio DMA ISR runs on Core 1 and must never be delayed. SPSCQueue's lock-free design guarantees this. FreeRTOS queues use critical sections that would momentarily disable interrupts — unacceptable for audio.

### Data flow:

```
Core 0 (FreeRTOS tasks):             Core 1 (DMA ISR):
────────────────────────             ────────────────────
Serial "SET cutoff 5000"
  → CommandParser
  → PicoSynthApp::SetParam()
  → mPendingState.filterCutoff = 5000
  → mStateQueue.TryPush(mPendingState)
                                     → mStateQueue.TryPop(localState)
                                     → mEngine.UpdateState(localState)

MIDI NOTE_ON 60 100
  → MIDI Task
  → PicoSynthApp::NoteOn(60, 100)
  → mCmdQueue.TryPush({NOTE_ON, 60, 100})
                                     → mCmdQueue.TryPop(cmd)
                                     → mEngine.OnNoteOn(60, 100)
```

### Diagnostics flow (Core 1 → Core 0):

```cpp
// Atomics (not queued — latest-value-wins semantics is correct):
std::atomic<int> mActiveVoiceCount{0};    // Written by ISR, read by status task
std::atomic<float> mPeakLevel{0.0f};      // Written by ISR, read by status task
std::atomic<uint32_t> mFillTimeUs{0};     // Written by audio driver, read by status
```

---

## Design Decision: Parameter Dispatch

**Decision: Create a platform-agnostic `SynthStateFields` table in `src/core/` using `offsetof()`, reusable by both Pico serial and desktop.**

### Problem:

The current `set_param`/`get_param` in main.cpp has 20+ strcmp chains that must be manually kept in sync with `SynthState`. `ParamMeta.h` already solves this for desktop but is coupled to iPlug2 (`#include "PolySynth.h"` for `EParams`).

### Solution:

Extract the core concept (field name → offset + type) into a platform-agnostic table:

```cpp
// src/core/SynthStateFields.h
#pragma once
#include "SynthState.h"
#include <cstddef>
#include <cstring>

namespace PolySynthCore {

struct SynthStateField {
    const char* name;       // "filterCutoff", "ampAttack", etc.
    size_t offset;          // offsetof() into SynthState
    bool isInt;             // true for int fields, false for float
};

#define SSF_FLOAT(field) { #field, offsetof(SynthState, field), false }
#define SSF_INT(field)   { #field, offsetof(SynthState, field), true }

static constexpr SynthStateField kSynthStateFields[] = {
    // Global
    SSF_FLOAT(masterGain), SSF_INT(polyphony), SSF_FLOAT(glideTime),
    SSF_INT(allocationMode), SSF_INT(stealPriority),
    SSF_INT(unisonCount), SSF_FLOAT(unisonSpread), SSF_FLOAT(stereoSpread),
    // Oscillators
    SSF_INT(oscAWaveform), SSF_FLOAT(oscAPulseWidth),
    SSF_INT(oscBWaveform), SSF_FLOAT(oscBFineTune), SSF_FLOAT(oscBPulseWidth),
    // Mixer
    SSF_FLOAT(mixOscA), SSF_FLOAT(mixOscB),
    // Filter
    SSF_FLOAT(filterCutoff), SSF_FLOAT(filterResonance),
    SSF_FLOAT(filterEnvAmount), SSF_INT(filterModel),
    // Envelopes
    SSF_FLOAT(filterAttack), SSF_FLOAT(filterDecay),
    SSF_FLOAT(filterSustain), SSF_FLOAT(filterRelease),
    SSF_FLOAT(ampAttack), SSF_FLOAT(ampDecay),
    SSF_FLOAT(ampSustain), SSF_FLOAT(ampRelease),
    // LFO
    SSF_INT(lfoShape), SSF_FLOAT(lfoRate), SSF_FLOAT(lfoDepth),
    // Poly-Mod
    SSF_FLOAT(polyModOscBToFreqA), SSF_FLOAT(polyModOscBToPWM),
    SSF_FLOAT(polyModOscBToFilter), SSF_FLOAT(polyModFilterEnvToFreqA),
    SSF_FLOAT(polyModFilterEnvToPWM), SSF_FLOAT(polyModFilterEnvToFilter),
    // FX
    SSF_FLOAT(fxChorusRate), SSF_FLOAT(fxChorusDepth), SSF_FLOAT(fxChorusMix),
    SSF_FLOAT(fxDelayTime), SSF_FLOAT(fxDelayFeedback), SSF_FLOAT(fxDelayMix),
    SSF_FLOAT(fxLimiterThreshold),
};

static constexpr int kSynthStateFieldCount =
    sizeof(kSynthStateFields) / sizeof(kSynthStateFields[0]);

#undef SSF_FLOAT
#undef SSF_INT

// Generic set/get using the field table (replaces 20+ strcmp chains).
// SynthState fields are float — no double anywhere in the pipeline.
inline bool SetField(SynthState& state, const char* name, float value) {
    char* base = reinterpret_cast<char*>(&state);
    for (int i = 0; i < kSynthStateFieldCount; ++i) {
        if (std::strcmp(kSynthStateFields[i].name, name) == 0) {
            if (kSynthStateFields[i].isInt)
                *reinterpret_cast<int*>(base + kSynthStateFields[i].offset) = static_cast<int>(value);
            else
                *reinterpret_cast<float*>(base + kSynthStateFields[i].offset) = value;
            return true;
        }
    }
    return false;
}

inline bool GetField(const SynthState& state, const char* name, float& out) {
    const char* base = reinterpret_cast<const char*>(&state);
    for (int i = 0; i < kSynthStateFieldCount; ++i) {
        if (std::strcmp(kSynthStateFields[i].name, name) == 0) {
            if (kSynthStateFields[i].isInt)
                out = static_cast<float>(*reinterpret_cast<const int*>(base + kSynthStateFields[i].offset));
            else
                out = *reinterpret_cast<const float*>(base + kSynthStateFields[i].offset);
            return true;
        }
    }
    return false;
}

} // namespace PolySynthCore
```

### Benefits:

1. **Single source of truth** — Add a field to `SynthState` + one line to `kSynthStateFields[]` and it's available everywhere
2. **Testable** — Can unit-test `SetField`/`GetField` on desktop with `test-embedded` config
3. **Desktop can use it too** — `ParamMeta` could reference `kSynthStateFields` for its offset/isInt info instead of duplicating it
4. **Serial protocol auto-discovers** — `LIST_PARAMS` command can iterate the table and report all available parameter names

---

## Design Decision: Output Stage

**Decision: The output stage (gain → soft-clip → sample conversion) is Pico-specific platform code, NOT part of Engine.**

On desktop, the output stage is handled by the DAW/audio interface. On Pico, there is no DAW — the output stage must apply gain, soft-clip, and convert to int16/I2S format. This is platform-specific post-processing, not DSP.

```cpp
// src/platform/pico/pico_output_stage.h
#pragma once
#include <cstdint>

namespace pico_audio {

static constexpr float kOutputGain = 4.0f;

// Padé approximant for tanh — ~10 cycles on Cortex-M33
inline float FastTanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Single-cycle saturating clamp via ARM DSP extension
inline int16_t SaturateToI16(int32_t val) {
    int32_t result;
    __asm__ volatile("ssat %0, #16, %1" : "=r"(result) : "r"(val));
    return static_cast<int16_t>(result);
}

inline void ProcessOutputSample(float left, float right,
                                 uint32_t& outLeft, uint32_t& outRight) {
    left *= kOutputGain;
    right *= kOutputGain;
    auto l16 = SaturateToI16(static_cast<int32_t>(FastTanh(left) * 32767.0f));
    auto r16 = SaturateToI16(static_cast<int32_t>(FastTanh(right) * 32767.0f));
    outLeft  = static_cast<uint32_t>(static_cast<uint16_t>(l16)) << 16;
    outRight = static_cast<uint32_t>(static_cast<uint16_t>(r16)) << 16;
}

} // namespace pico_audio
```

---

## Design Decision: Embedded-Specific Optimizations

### Critical: SRAM Placement for Audio Code (XIP Cache Issue)

**This is the most likely cause of choppy audio on the current build.**

All code on the RP2350 runs from external QSPI flash via the XIP (Execute-In-Place) cache
by default. Cache misses cause **variable-latency stalls** (10-100+ cycles per miss) while
the flash is fetched. The DMA ISR + audio callback + all inlined DSP code have branchy
paths (switch on waveform, switch on ADSR stage, switch on filter model) that virtually
guarantee cache misses, especially after Core 0 evicts audio code from the shared XIP cache.

**Fix: Place all audio-critical code in SRAM using `__time_critical_func`.**

```cpp
// audio_i2s_driver.cpp — DMA ISR must be in SRAM
static void __time_critical_func(dma_irq_handler)() { ... }

// main.cpp — audio callback must be in SRAM
static void __time_critical_func(audio_callback)(uint32_t* buffer, uint32_t num_frames) { ... }
```

The `__time_critical_func` attribute places the function in the `.time_critical` section,
which the linker maps to SRAM. Since SEA_DSP is header-only and inlined into `audio_callback`,
the inlined DSP code also ends up in SRAM automatically.

**SRAM cost**: The audio callback + inlined DSP is estimated at 4-8 KB of code. The RP2350
has 520 KB SRAM — this is negligible. Place DMA buffers in a separate SRAM bank
(`__scratch_y`) to avoid bus contention with code fetches.

### Critical: Denormal Flush-to-Zero

IIR filter feedback paths (ladder, SVF, biquad) produce denormal floats as signals decay
toward zero. On Cortex-M33, denormal operations are handled in hardware but may still
incur multi-cycle penalties. Set the FPSCR FZ (flush-to-zero) and DN (default-NaN) bits
at Core 1 startup to eliminate this:

```cpp
static void core1_audio_entry() {
    // Flush denormals to zero — prevents performance cliffs in filter tails
    uint32_t fpscr = __get_FPSCR();
    __set_FPSCR(fpscr | (1 << 24) | (1 << 25));  // FZ=1, DN=1

    pico_audio::Init(audio_callback, 1);
    // ...
}
```

### Critical: Remove `UpdateVisualization()` from DMA ISR

`Engine::UpdateVisualization()` is called from the DMA ISR every 256-sample buffer. It:
1. Iterates all voices to count active ones
2. Iterates all voices again with O(n²) dedup for held notes
3. Stores to `std::atomic<uint64_t>` × 2 — **NOT lock-free on 32-bit ARM**

On Cortex-M33, `std::atomic<uint64_t>` operations use `__atomic_store_8` from libatomic,
which disables interrupts or uses a global spinlock. This is called from the DMA ISR on
Core 1 while Core 0's main loop may be reading the same atomics — causing cross-core
contention via the Pico SDK hardware spinlock.

**Fix: Remove `UpdateVisualization()` from the ISR entirely.** The simple
`std::atomic<int>` voice count (already at main.cpp:139) is sufficient for status
reporting. Move held-note tracking to the main loop on Core 0 if needed.

### Critical: Cache Oscillator Phase Increment

`Oscillator::SetFrequency()` performs a **division every sample** (`freq / mSampleRate`).
It's called from Voice::Process() lines 176 & 186 for both oscillators. With 4 voices:
8 VDIV.F32 per sample × 256 samples = 2048 divisions per buffer × ~14 cycles = ~29K cycles.

**Fix: Skip SetFrequency when frequency hasn't changed, or cache the reciprocal.**

```cpp
// In sea_oscillator.h
void SetFrequency(Real freq) {
    if (freq == mLastFreq) return;  // Skip redundant division
    mLastFreq = freq;
    mPhaseIncrement = freq * mInvSampleRate;  // Multiply by cached 1/sampleRate
}
```

### Already applied (keep as-is):

| Optimization | Mechanism | Sound Impact |
|-------------|-----------|--------------|
| Float precision | `POLYSYNTH_USE_FLOAT` / `SEA_PLATFORM_EMBEDDED` | Minimal — float has >120dB SNR |
| Hard-float ABI | CMake `-mfloat-abi=hard` | None — same math, faster calling convention |
| Fast math | `SEA_FAST_MATH` in `sea_math.h` | <0.1% error — polynomial approximants |
| Effects stripped | `POLYSYNTH_DEPLOY_*=0` | Effects disabled entirely |
| Double promotion warning | `-Wdouble-promotion` | Prevents accidental 10× slowdowns |
| No exceptions/RTTI | `-fno-exceptions -fno-rtti` | None — reduces code size |
| SSAT saturation | ARM `ssat` instruction for int16 output | 1 cycle vs ~5 for std::clamp |
| fast_tanh Padé | Division-based approximant in output path | <0.1% error, ~12 cycles |

### Additional compiler flags to add:

```cmake
target_compile_options(polysynth_pico PRIVATE
    -Wall -Wextra
    -Wdouble-promotion
    -fno-exceptions
    -fno-rtti
    -ffast-math              # NEW: enables reciprocal-math, FMA, reordering
    -funroll-loops           # NEW: helps ladder filter's 4-stage loop
)
# Note: -mcpu=cortex-m33+fp+dsp is set by the SDK automatically via PICO_PLATFORM
# Note: -ffast-math may conflict with SDK ROM math functions; test carefully
#        Since we use SEA_FAST_MATH (not sinf/cosf), this should be safe
```

### MIDI Note-to-Frequency Lookup Table (512 bytes):

Eliminates `pow(2, (note-69)/12)` from NoteOn path. Pre-computed at startup or constexpr:

```cpp
// 128-entry table: MIDI note 0-127 → frequency in Hz
static constexpr float kMidiNoteFreq[128] = {
    8.1758f, 8.6620f, 9.1770f, 9.7227f, // C-1 to Eb-1
    // ... generated at compile time via constexpr function
    12543.9f // G9
};

// Usage in Voice::NoteOn():
sample_t newFreq = kMidiNoteFreq[std::clamp(note, 0, 127)];
// Detune cents applied via: newFreq * pow2_approx(cents / 1200.0f)
```

### Division-free Tanh (eliminate VDIV from ladder filter inner loop):

The current Padé tanh `x*(27+x²)/(27+9*x²)` contains a division (~14 cycles on M33).
Called 6 times per voice per sample in the ladder filter = ~84 cycles of division per voice.
A LUT eliminates all divisions. When `SEA_USE_TANH_LUT` is implemented:

```
Before: 6 tanh × ~15 cycles (Padé with VDIV) = ~90 cycles/voice/sample
After:  6 tanh × ~5 cycles (LUT + lerp)       = ~30 cycles/voice/sample
Saving: 60 cycles/voice/sample = 240 cycles for 4 voices per sample
```

### Compile-time optimization switches:

```cpp
// UNIVERSAL — identical behavior, just faster. Apply on all platforms:
SEA_FAST_MATH              // Polynomial approximants
SEA_USE_TANH_LUT           // Tanh lookup table (~0.1% error, indistinguishable)
SEA_USE_SINE_LUT           // Sine lookup table (<0.001% error)

// BEHAVIORAL CHANGES — compile-guarded per platform:
POLYSYNTH_USE_FLOAT        // float vs double precision (embedded only)
POLYSYNTH_MAX_VOICES=4     // Voice limit (embedded only)
POLYSYNTH_DEPLOY_CHORUS=0  // Feature removal (embedded only)
POLYSYNTH_DEPLOY_DELAY=0
POLYSYNTH_DEPLOY_LIMITER=0
Default filter model        // Cascade24 on embedded, Ladder on desktop

// ARM-SPECIFIC — naturally guarded by architecture:
__time_critical_func       // SRAM placement for audio code (avoids XIP cache misses)
SSAT instruction           // ARM Cortex-M only
FPSCR FZ+DN bits           // Denormal flush-to-zero
-mfloat-abi=hard           // ARM-specific ABI
-ffast-math                // Reciprocal math, FMA, reordering
```

### RP2350 Hardware Accelerators (future optimization):

| Accelerator | Use Case | Notes |
|---|---|---|
| **SIO Interpolator** (2 per core) | LUT address generation for sin/tanh wavetables | ~3 cycles per lookup vs ~5 for manual index + lerp |
| **Hardware Integer Divider** | 8-cycle integer division | Auto-used by compiler for integer divides |
| **DCP (Double Coprocessor)** | 2-3 cycle double-precision ops | Not needed — we use float. Avoidance is correct. |
| **Dual 16-bit SIMD** (SMLAD) | Q15 fixed-point FIR/mixing | Potential 2× throughput for final mix-down |

---

## File Structure

### Before (monolith):
```
src/platform/pico/
├── main.cpp              ← 670 lines: everything mixed together
├── audio_i2s_driver.h/cpp
├── command_parser.h/cpp
├── i2s.pio
└── ...
```

### After (decomposed):
```
src/core/
├── SynthStateFields.h     ← NEW: Platform-agnostic field table for SynthState
├── ... (existing files unchanged)

src/platform/pico/
├── main.cpp               ← SIMPLIFIED: ~80 lines. FreeRTOS task creation + init.
├── FreeRTOSConfig.h       ← NEW: FreeRTOS configuration for RP2350
├── PicoSynthApp.h         ← NEW: Platform glue (owns Engine, state, queues)
├── PicoSynthApp.cpp       ← NEW: Implementation
├── pico_output_stage.h    ← NEW: fast_tanh + SSAT + gain (from main.cpp)
├── pico_audio_core.h      ← NEW: Core 1 DMA audio callback (from main.cpp)
├── pico_audio_core.cpp    ← NEW: Implementation
├── pico_tasks.h           ← NEW: FreeRTOS task entry points (serial, MIDI, etc.)
├── pico_tasks.cpp         ← NEW: Task implementations
├── pico_demo.h            ← NEW: Demo sequencer (from main.cpp)
├── pico_demo.cpp          ← NEW: Implementation
├── pico_self_test.h       ← NEW: Self-test sequence (from main.cpp)
├── pico_self_test.cpp     ← NEW: Implementation
├── audio_i2s_driver.h/cpp ← UNCHANGED
├── command_parser.h/cpp   ← UNCHANGED
├── i2s.pio                ← UNCHANGED
└── ...
```

---

## Module Responsibilities

### `PicoSynthApp` — Platform Glue (the iPlug2 equivalent)

This is the central coordinator. It mirrors what `PolySynth.cpp` does for desktop:

```cpp
class PicoSynthApp {
public:
    // Lifecycle
    void Init(sample_t sampleRate);  // sample_t = float on embedded, double on desktop
    void Reset();

    // Input events (called from Core 0 FreeRTOS tasks — thread-safe via queues)
    void NoteOn(uint8_t note, uint8_t velocity);
    void NoteOff(uint8_t note);
    void SetParam(const char* name, float value);
    bool GetParam(const char* name, float& value) const;
    void Panic();

    // State management
    void PushStateUpdate();  // Snapshots mPendingState → mStateQueue

    // Audio callback (called from Core 1 DMA ISR — NOT a FreeRTOS task)
    void AudioCallback(uint32_t* buffer, uint32_t numFrames);

    // Diagnostics (called from Core 0 status task)
    int GetActiveVoiceCount() const;
    float GetPeakLevel();  // Returns and resets
    float GetCpuPercent() const;

    SynthState& GetPendingState() { return mPendingState; }

private:
    Engine mEngine;
    SynthState mPendingState;
    SPSCQueue<AudioCommand, 16> mCmdQueue;
    SPSCQueue<SynthState, 2> mStateQueue;

    std::atomic<int> mActiveVoiceCount{0};
    std::atomic<float> mPeakLevel{0.0f};
};
```

### `pico_audio_core` — Core 1 DMA Audio

- Initializes audio I2S driver on Core 1
- Provides the DMA ISR callback that calls `PicoSynthApp::AudioCallback()`
- Core 1 idle task runs `__wfi()` between interrupts

### `pico_tasks` — FreeRTOS Task Entry Points

Contains all task functions:
- `serial_task()` — USB CDC serial polling + command dispatch
- `midi_task()` — USB MIDI + UART MIDI processing (future, stub for now)
- `hid_task()` — ADC knob scanning + button debounce (future, stub for now)
- `display_task()` — SPI LCD/OLED rendering (future, stub for now)
- `radio_task()` — CYW43 WiFi/BLE servicing (future, stub for now)
- `status_task()` — Periodic diagnostics reporting

### `pico_demo` — Demo Sequencer

Extracted from the current 140-line demo state machine:
- Self-contained state machine with tick function
- Takes `PicoSynthApp&` reference for NoteOn/NoteOff/SetParam
- Can be triggered from serial ("DEMO") or run at boot

### `pico_self_test` — Boot-Time Self-Tests

Extracted from the current `run_self_tests()`:
- Runs engine init, audio production, I2S init tests
- Returns pass/fail for boot decision
- Outputs `[TEST:PASS]`/`[TEST:FAIL]` markers for CI

---

## Simplified main.cpp

After decomposition, `main.cpp` becomes FreeRTOS task setup:

```cpp
#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include "PicoSynthApp.h"
#include "pico_audio_core.h"
#include "pico_self_test.h"
#include "pico_tasks.h"

static PicoSynthApp s_app;

int main() {
    // ── All hardware init BEFORE FreeRTOS starts ──────────────────
    // (GPIO init across cores after scheduler start can hang)
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("ERROR: CYW43 init failed\n");
        return 1;
    }

    // Wait for USB serial (up to 3s)
    for (int i = 0; i < 30 && !stdio_usb_connected(); i++)
        sleep_ms(100);

    print_banner();

    // Run self-tests before anything else
    if (!pico_self_test::Run(s_app)) {
        printf("Self-tests failed. Halting.\n");
        while (true) sleep_ms(1000);
    }

    // Initialize synth engine
    s_app.Init(pico_audio::kSampleRate);

    // ── Launch Core 1 BEFORE FreeRTOS ─────────────────────────────
    // Core 1 is bare-metal: __wfi() loop + DMA ISR audio callback.
    // FreeRTOS (configNUMBER_OF_CORES=1) will never touch Core 1.
    // Must launch before vTaskStartScheduler() because
    // multicore_launch_core1() uses pico_sync primitives that
    // are safe to call pre-scheduler.
    pico_audio_core::Launch(s_app);

    // ── Create FreeRTOS tasks (all run on Core 0) ─────────────────
    pico_tasks::CreateAll(s_app);

    // ── Start FreeRTOS scheduler (never returns) ──────────────────
    vTaskStartScheduler();

    // Should never reach here
    return 0;
}

// FreeRTOS idle hook — LED heartbeat
extern "C" void vApplicationIdleHook() {
    static uint32_t last_toggle = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_toggle >= 500) {
        last_toggle = now;
        static bool led_state = false;
        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
    }
}
```

---

## Inter-Core Communication Summary

```
┌──────── Core 0 (FreeRTOS) ────────┐    ┌──── Core 1 (Bare-Metal) ─────────┐
│                                     │    │                                   │
│  PicoSynthApp                      │    │  DMA ISR (audio_callback)         │
│  ├── mPendingState (Core 0 only)   │    │                                   │
│  │                                 │    │                                   │
│  ├── mCmdQueue ────────────────────┼───►│  TryPop → Engine::OnNoteOn/Off   │
│  │   SPSCQueue<AudioCommand, 16>   │    │           Engine::Reset          │
│  │                                 │    │                                   │
│  ├── mStateQueue ──────────────────┼───►│  TryPop → Engine::UpdateState    │
│  │   SPSCQueue<SynthState, 2>      │    │                                   │
│  │                                 │    │                                   │
│  ├── mActiveVoiceCount ◄───────────┼────┤  atomic store (relaxed)          │
│  ├── mPeakLevel ◄──────────────────┼────┤  atomic store (relaxed)          │
│  │   (latest-value-wins atomics)   │    │                                   │
│  │                                 │    │                                   │
│  └── GetCpuPercent() ◄─────────────┼────┤  audio_i2s_driver::GetLastFill() │
│                                     │    │                                   │
└─────────────────────────────────────┘    └───────────────────────────────────┘

Memory ordering:
- SPSCQueue: acquire/release on head/tail atomics (proven in SPSCQueue.h)
- Diagnostics: relaxed ordering (latest-value-wins, no happens-before needed)
- No mutexes, spinlocks, or critical sections cross the core boundary
- FreeRTOS primitives used ONLY within Core 0 tasks (task-to-task on same core)
- Core 1 is completely FreeRTOS-unaware — uses only std::atomic and __wfi()
```

---

## How This Mirrors Desktop Architecture

| Concern | Desktop (iPlug2) | Pico (FreeRTOS) |
|---------|------------------|-----------------|
| **Application class** | `PolySynth : IPlug` | `PicoSynthApp` |
| **Task management** | OS threads (iPlug2 manages) | FreeRTOS tasks with priorities |
| **Audio callback** | `ProcessBlock()` | `AudioCallback()` via DMA ISR |
| **State transfer** | `SPSCQueue<SynthState>` | Same `SPSCQueue<SynthState>` |
| **MIDI input** | iPlug2 MIDI → `ProcessMidiMsg()` | MIDI task → `NoteOn()`/`NoteOff()` |
| **Parameter dispatch** | `ParamMeta` table + `ApplyParamToState()` | `SynthStateFields` table + `SetField()` |
| **Output stage** | DAW handles float→DAC | `pico_output_stage.h` (fast_tanh + SSAT) |
| **Engine ownership** | `mEngine` member of PolySynth | `mEngine` member of PicoSynthApp |
| **Visualization** | `Engine::GetActiveVoiceCount()` via atomics | Same atomics, read by status/display task |

---

## Implementation Plan (Priority-Ordered Phases)

### Phase 0: Immediate Performance Fixes (Pre-Refactor)

Apply these to the current monolithic `main.cpp` before restructuring. Each is a
small, safe change that addresses a root cause of choppy audio:

**Step 0.1: SRAM placement for audio code**
- Add `__time_critical_func` to `dma_irq_handler()` in `audio_i2s_driver.cpp`
- Add `__time_critical_func` to `audio_callback()` in `main.cpp`
- This alone may fix the choppiness by eliminating XIP cache miss stalls

**Step 0.2: Denormal flush on Core 1**
- Set `FPSCR.FZ=1, DN=1` at `core1_audio_entry()` before audio init
- One line of code, prevents potential performance cliffs in filter tails

**Step 0.3: Remove `UpdateVisualization()` from DMA ISR**
- The `std::atomic<uint64_t>` stores are NOT lock-free on 32-bit ARM
- Keep only the `std::atomic<int>` voice count store (already present)
- Move held-note tracking to Core 0 if needed

**Step 0.4: Cache oscillator phase increment**
- Skip `SetFrequency()` division when frequency hasn't changed
- Or use cached `mInvSampleRate` to multiply instead of divide
- Eliminates ~2048 VDIV.F32 per buffer (4 voices × 2 oscs × 256 samples)

### Phase 1: Core Infrastructure (Foundation)

**Step 1.1: Create `SynthStateFields.h` in `src/core/`**
- Platform-agnostic field table with `SetField()`/`GetField()`
- Add unit tests in `tests/` for both desktop and embedded configs

**Step 1.2: Create `pico_output_stage.h`**
- Extract `fast_tanh()`, `saturate_to_i16()`, output gain constant, `ProcessOutputSample()`

**Step 1.3: Create `PicoSynthApp.h/cpp`**
- Own Engine, SynthState, SPSCQueue instances
- Implement `NoteOn/NoteOff/SetParam/GetParam/Panic/Reset/AudioCallback`
- Use `SynthStateFields::SetField/GetField` for parameter dispatch

**Step 1.4: Create `pico_audio_core.h/cpp`**
- DMA ISR callback delegating to `PicoSynthApp::AudioCallback()`
- Init function for audio driver on Core 1

### Phase 2: FreeRTOS Integration

**Step 2.1: Create `FreeRTOSConfig.h`**
- Core 0 only (`configNUMBER_OF_CORES=1`) — avoids RP2350 SMP spinlock bugs
- Priority levels, stack sizes, heap size
- FPU, sync interop, secure mode flags (see FreeRTOS design decision above)
- Idle hook, stack overflow checking

**Step 2.2: Create `pico_tasks.h/cpp`**
- `serial_task()` — current serial polling logic
- `status_task()` — current periodic reporting
- `CreateAll()` — creates all tasks (all on Core 0 — single-core FreeRTOS)
- Stub functions for future tasks (MIDI, HID, display, radio)

**Step 2.3: Create `pico_demo.h/cpp`**
- Extract demo state machine from main.cpp

**Step 2.4: Create `pico_self_test.h/cpp`**
- Extract self-test sequence from main.cpp

**Step 2.5: Update `CMakeLists.txt`**
- Add FreeRTOS-Kernel as git submodule at `external/FreeRTOS-Kernel/` (Raspberry Pi fork)
- Keep `pico_multicore` (needed for `multicore_launch_core1()`)
- Replace `pico_cyw43_arch_none` with `pico_cyw43_arch_lwip_sys_freertos`
- Add new source files
- Add `hardware_adc` for future knob support
- Add `-ffast-math -funroll-loops` compiler flags

### Phase 3: Rewrite main.cpp + Integration

**Step 3.1: Rewrite `main.cpp`**
- ~80 lines: init, self-test, FreeRTOS task creation, `vTaskStartScheduler()`

**Step 3.2: Verify CI**
- `just test` — Desktop tests still pass
- `just test-embedded` — Embedded tests pass (including SynthStateFields)
- `just pico-build` — ARM cross-compile succeeds with FreeRTOS

### Phase 4: CPU Optimizations (SEA_DSP)

**Step 4.1: Add tanh lookup table to `sea_math.h`**
- Behind `SEA_USE_TANH_LUT` compile flag
- 256 entries, linear interpolation
- Unit test verifying max error < 0.2% vs std::tanh
- Enable on both desktop and embedded

**Step 4.2: Add sine wavetable to `sea_math.h`**
- Behind `SEA_USE_SINE_LUT` compile flag
- 1024 entries, linear interpolation
- Unit test verifying max error < 0.01% vs std::sin
- Enable on both desktop and embedded

**Step 4.3: Benchmark and validate**
- Compare CPU% before/after on hardware
- Verify audio output is perceptually identical (energy test, null test)
- Update `POLYSYNTH_MAX_VOICES` if budget allows more voices

### Phase 5: Validation

**Step 5.1: Hardware validation**
- Flash firmware, verify audio output
- Verify serial commands, demo sequence
- Verify FreeRTOS tasks run at correct priorities
- Check CPU% and stack high-water marks

**Step 5.2: Stress testing**
- All voices active simultaneously
- Rapid parameter changes during playback
- Serial + demo running concurrently
- Verify no DMA underruns under load

---

## What This Plan Does NOT Change

- **Zero changes to `src/core/`** except adding `SynthStateFields.h`
- **Zero changes to `src/platform/desktop/`**
- **Zero changes to `audio_i2s_driver.h/cpp`**
- **Zero changes to `command_parser.h/cpp`**
- **Zero changes to `i2s.pio`**
- **Binary-identical audio output** (until Phase 4 LUTs, which are perceptually identical)

## Changes to `libs/SEA_DSP/` (Phase 4 only)

- `sea_math.h` gains tanh LUT and sine LUT behind compile flags
- These are universal optimizations usable on all platforms
- Existing code paths preserved when flags are not defined

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| FreeRTOS + bare-metal Core 1 interaction | `configSUPPORT_PICO_SYNC_INTEROP=1` enables SDK sync primitives between FreeRTOS tasks and bare-metal Core 1. Explicitly supported by RP2350 port README. SPSCQueue uses only `std::atomic` — no SDK/FreeRTOS sync needed. |
| FreeRTOS heap fragmentation | Use Heap4 (coalescing allocator). All tasks created at startup, no dynamic task creation. |
| `pico_cyw43_arch_lwip_sys_freertos` increases binary size | WiFi stack adds ~60-80KB flash. We have 2MB. Acceptable for the functionality gained. |
| `multicore_launch_core1` + FreeRTOS | Safe because FreeRTOS is Core 0 only (`configNUMBER_OF_CORES=1`). Launch Core 1 before `vTaskStartScheduler()`. |
| TinyUSB `vTaskDelay` bug on RP2350 | Known issue: USB enumeration takes ~15s. Workaround: use `tud_task_ext(timeout, false)` or `portYIELD()` in USB task loop. |
| CYW43 WiFi + BLE simultaneously | Known stability issues (deadlocks). Plan for WiFi OR BLE, not both. Test thoroughly if both needed. |
| Tanh/Sine LUT memory cost | Tanh: 1KB, Sine: 4KB. Total 5KB of 520KB SRAM. Negligible. |
| FreeRTOS stack overflow | Enable `configCHECK_FOR_STACK_OVERFLOW=2` in debug. Monitor high-water marks via `uxTaskGetStackHighWaterMark()`. |
| SPSCQueue vs FreeRTOS queue confusion | Rule: SPSCQueue for Core 0 ↔ Core 1 (ISR-safe, lock-free). FreeRTOS queues/notifications for task-to-task within Core 0 only. |
| RP2350 FreeRTOS SMP spinlock bugs | Avoided entirely by using `configNUMBER_OF_CORES=1`. No SMP code runs. |

---

## Memory Budget (Updated)

| Component | Bytes | Notes |
|-----------|-------|-------|
| Engine (4 voices, float, no effects) | ~2,000 | Stack-allocated voice objects |
| DMA audio buffers | ~2,048 | 2 × 256 frames × 2 channels × int16 |
| FreeRTOS heap | 49,152 | Task stacks + kernel objects |
| FreeRTOS kernel | ~8,000 | SMP scheduler, timer task |
| Pico SDK + CYW43 + lwIP | ~40,000 | USB, PIO, DMA, WiFi, TCP/IP drivers |
| Tanh LUT | 1,028 | 257 × float |
| Sine LUT | 4,100 | 1025 × float |
| SPSCQueue instances | ~800 | Command queue + state queue |
| **Total** | **~107 KB** | **of 520 KB — 21% used, 79% free** |

The remaining ~413KB provides ample room for display framebuffers (~10KB for SSD1306, ~150KB for ST7789), effect buffers when re-enabled, and any future subsystems.
