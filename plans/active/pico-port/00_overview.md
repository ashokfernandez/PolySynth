# PolySynth Pico Port — Sprint Plan Overview

> **What is PolySynth?** A software polyphonic synthesizer that runs identically on
> desktop (via iPlug2 audio plugin framework) and embedded (Raspberry Pi Pico 2 W).
> The same DSP engine code compiles for both targets with zero platform-specific forks.

## Vision

Build a standalone polyphonic synthesizer on the Raspberry Pi Pico 2 W (RP2350) with
a Waveshare Pico-Audio HAT (PCM5101A DAC). The same DSP engine runs on desktop (via
iPlug2) and embedded — zero platform-specific DSP code.

## Source of Truth

**`07_pico_dsp_architecture.md` is the canonical reference.** It describes the target
architecture after Sprints 7-10. Not all components exist yet — it is a target state
document, not a description of current state. If a sprint document contradicts the
architecture document, the **sprint document wins** for its own scope (it may
intentionally defer or simplify something), but the architecture document wins for
overall design direction.

## Hardware Target

| Component | Specification |
|-----------|---------------|
| MCU | RP2350 — Dual Cortex-M33 @ 150 MHz |
| SRAM | 520 KB |
| Flash | 2 MB |
| FPU | Hardware single-precision (FPv5) |
| DAC | PCM5101A (Waveshare Pico-Audio HAT) |
| I2S Pins | DIN=GPIO26, BCK=GPIO27, LRCK=GPIO28 |
| Audio | 48 kHz, 16-bit stereo, 256-frame buffer (5.33ms) |

---

## Sprint Status

### Completed Sprints (Shipped)

| Sprint | Title | Status | Key Deliverables |
|--------|-------|--------|------------------|
| **Pico-1** | [Toolchain & Hello World](01_sprint_pico_toolchain.md) | **DONE** | Pico SDK v2.1.0 integration, CMakeLists.txt, LED blink + USB serial, Justfile targets (`pico-build`, `pico-flash`, `pico-clean`). Full DSP header chain compiles for ARM with zero code changes. 541.5K UF2 confirmed on hardware. |
| **Pico-2** | [CI/CD & Emulation Pipeline](02_sprint_pico_ci_emulation.md) | **DONE** | Three-layer CI: Layer 1 native embedded tests (Catch2, float, 4 voices), Layer 2a ARM cross-compile (.uf2 generation), Layer 2b Wokwi emulation (RP2040 proxy). `[TEST:ALL_PASSED]` serial markers. All CI jobs green. |
| **Pico-3** | [I2S Audio Hello World](03_sprint_pico_i2s_audio.md) | **DONE** | Custom PIO I2S driver, DMA double-buffering (ping-pong), 440Hz sine wave through DAC, zero underruns at 48kHz/256 frames. PR #63 merged. |
| **Pico-4** | [Core DSP Integration](04_sprint_pico_dsp_integration.md) | **DONE** | Full Engine wired to I2S. 4-voice polyphony with all oscillators, ADSR envelopes, filters (Ladder/Cascade/Biquad), LFO. Hard-float ABI, SSAT saturation, SEA_FAST_MATH. Per-buffer CPU profiling. CPU < 30% with 4 voices. |
| **Pico-5** | [Effects Architecture](05_sprint_pico_effects_architecture.md) | **DONE** | Deploy flags (`POLYSYNTH_DEPLOY_CHORUS/DELAY/LIMITER=0`). Soft clipper via `fast_tanh()` Pade approximant + SSAT in output stage. Effects stripped from binary, all compile for ARM. |
| **Pico-6** | [Serial Control Interface](06_sprint_pico_serial_control.md) | **DONE** | Zero-allocation command parser (NOTE_ON/OFF, SET/GET 20+ params, STATUS, PANIC, RESET, DEMO, STOP). 15+ Catch2 parser tests. Demo sequence state machine. Voice-change event ring buffer (ISR-safe, no printf from ISR). |

**What shipped in Sprints 1-6:**
- Real polyphonic synthesizer running on RP2350 (Pico 2 W)
- 4-voice polyphony, all oscillators/filters/envelopes/LFO active
- 48kHz stereo audio via custom PIO I2S to PCM5101A DAC
- Serial command interface with 20+ controllable parameters
- Automated demo sequence (saw, square, chord, filter sweep)
- Three-layer CI pipeline (desktop tests, ARM compile, Wokwi emulation)
- Desktop-identical DSP (same `src/core/` code, zero forks)

**What was NOT shipped from the original plans:**
- `tools/pico_control.py` Python interactive controller (Sprint 6) — not started
- `SynthStateFields.h` generic field table (mentioned in Sprint 6) — not started
- `pico_config.h` centralized config header — not created (defines are in CMakeLists.txt)
- `pico_effects.h` PicoSoftClipper class — soft clipping done inline in main.cpp instead

### PR #65 Review Notes (Merged — Prototype State)

PR #65 (`feat(pico): DSP Port MVP`) shipped Sprints 1-6 as a working prototype.
The reviewer flagged these items for follow-up in Sprints 7-10:

1. **`s_peakLevel` data race** — Both the ISR and main loop read/write `s_peakLevel`
   without atomics. On Cortex-M33 single-word float stores are atomic *in practice*,
   but it's technically UB. → **Fix in Sprint 7** (`std::atomic<float>` or exchange).
2. **`s_prevVoiceCount` ISR-only access** — Fine, but needs a comment noting it's
   ISR-private so future readers don't mistake it for a shared variable. → **Sprint 7**.
3. **`demo_phase_name` missing default** — Switch covers all enum values, trailing
   `return "?"` handles it, but some compilers warn. → **Sprint 8** (demo extraction).
4. **`kOutputGain = 4.0f` magic number** — Good enough for now, but will need to
   become tunable as more waveforms/effects come online. → **Sprint 11+** (effects).
5. **`Sin()` accuracy comment** — Comment says "~0.016%" but 7th-order Taylor on
   [-π/2, π/2] has max error closer to ~0.00016. Verify with sweep test. → **Sprint 10**.

### Known Issues (Discovered During Architecture Research)

These were identified through deep research into RP2350 specifics and embedded DSP
optimization. They affect the current shipped code:

1. **Audio code runs from flash (XIP cache)** — No `__time_critical_func` annotations.
   XIP (Execute-In-Place) cache misses during DMA ISR cause variable-latency stalls.
   Likely #1 cause of reported choppy audio.

2. **`UpdateVisualization()` in DMA ISR** — Uses `std::atomic<uint64_t>` which is
   NOT lock-free on 32-bit ARM. Disables interrupts or uses hardware spinlock from
   within the DMA ISR.

3. **Oscillator division every sample** — `SetFrequency()` does `freq / mSampleRate`
   per sample per oscillator. 8 VDIV.F32 per sample with 4 voices.

4. **Voice struct carries all 3 filter types** — BiquadFilter + LadderFilter +
   CascadeFilter instantiated per voice, but only one used at runtime. ~400 bytes
   of wasted cache per voice.

5. **No denormal flush** — FPSCR FZ/DN bits not set on Core 1. IIR filter tails
   may hit performance cliffs. (A denormal is an extremely small floating-point
   number near zero that some processors handle much more slowly than normal numbers.)

6. **Monolithic main.cpp (670 lines)** — Mixes audio callback, command dispatch,
   demo sequencing, state handoff, self-tests, and main loop. No separation of concerns.

---

### Forward Sprints (Planned)

| Sprint | Title | Focus | Depends On |
|--------|-------|-------|------------|
| **Pico-7** | [Performance Foundations](07a_sprint_pico_performance.md) | Fix choppy audio: SRAM placement, denormals, ISR cleanup, oscillator cache | Pico-6 |
| **Pico-8** | [Architecture Decomposition](08_sprint_pico_decomposition.md) | Break main.cpp monolith into PicoSynthApp + modules | Pico-7 |
| **Pico-9** | [FreeRTOS Migration](09_sprint_pico_freertos.md) | Replace busy-loop with FreeRTOS tasks on Core 0 | Pico-8 |
| **Pico-10** | [DSP Optimizations](10_sprint_pico_dsp_optimizations.md) | LUTs (tanh, sine, MIDI-to-freq), compile-time filter, CPU budget expansion | Pico-9 |

```
Pico-7 (Performance)           ← Fix immediate audio quality issues
  └── Pico-8 (Decomposition)   ← Clean architecture for maintainability
        └── Pico-9 (FreeRTOS)  ← Enable WiFi, USB MIDI, display, ADC
              └── Pico-10 (DSP Optimizations)  ← More voices, effects headroom
```

### Future Roadmap (Post Sprint 10)

| Sprint | Focus | Goal |
|--------|-------|------|
| 11 | Effects deployment | Chorus with static buffers, Delay at 100ms max |
| 12 | Physical controls | ADC knobs, buttons, encoder, LED feedback |
| 13 | USB MIDI | TinyUSB MIDI device class, note/CC input |
| 14 | WiFi / OSC | CYW43 WiFi, lwIP, OSC parameter control |
| 15 | Preset system | Save/load patches to flash, serial preset transfer |

---

## Architecture Reference

The detailed architecture plan (signal path, FreeRTOS config, inter-core communication,
CPU budget analysis, memory budget, RP2350-specific optimizations) lives in:

**[07_pico_dsp_architecture.md](07_pico_dsp_architecture.md)**

Key architectural decisions documented there:
- **FreeRTOS on Core 0 only** (`configNUMBER_OF_CORES=1`) — avoids known RP2350 SMP spinlock hardfault bugs
- **Core 1 bare-metal** — DMA ISR + `__wfi()`, launched via `multicore_launch_core1()`
- **Per-voice filtering** — essential for polyphonic (not paraphonic) architecture
- **Compile-time filter selection** on embedded — eliminates per-voice filter bloat
- **SPSCQueue for cross-core** — lock-free, no FreeRTOS primitives across core boundary
- **FreeRTOS-Kernel** in `external/` as git submodule (Raspberry Pi fork, not mainline)

---

## Testing Architecture

```
Layer 1: Native Host Tests (x86, milliseconds)
├─ Desktop: sample_t=double, all effects ON
└─ Embedded: sample_t=float, MAX_VOICES=4, effects OFF, -Wdouble-promotion

Layer 2a: ARM Cross-Compile (RP2350, compile-only)
├─ Verifies .uf2 generation
└─ Checks binary size, zero -Wdouble-promotion warnings

Layer 2b: Wokwi Emulation (RP2040 proxy, optional for forks)
├─ Firmware boots, self-test runs
├─ [TEST:ALL_PASSED] marker detected
└─ Serial protocol verified
```

CI mandate: Every PR must pass all layers before merge.

---

## Key Design Principles

1. **One codebase.** All DSP in `src/core/` and `libs/SEA_DSP/`. Zero Pico-specific DSP forks.
2. **Compile everything, deploy selectively.** Feature flags control binary contents.
3. **No heap in the audio path.** Zero `new`/`malloc`/`std::vector` in per-sample code.
4. **Float precision on embedded.** `sample_t = float` via `POLYSYNTH_USE_FLOAT`.
5. **Desktop builds unaffected.** All embedded guards are transparent on desktop.
6. **Test before flash.** CI validates before hardware testing.
7. **Audio code in SRAM.** `__time_critical_func` on DMA ISR and audio callback.
8. **Per-voice filtering.** Polyphonic, not paraphonic. One filter type at compile time on embedded.

---

## Build Commands

```bash
just pico-build          # Build .uf2 for RP2350 (real hardware)
just pico-flash          # Flash via picotool (requires BOOTSEL)
just pico-clean          # Remove build/pico

just test                # Desktop tests (16 voices, double)
just test-embedded       # Embedded-config tests (4 voices, float)

just pico-build-emu      # Build for RP2040 (Wokwi proxy)
just pico-emu-test       # Run firmware in Wokwi emulator
```

---

## Glossary

| Term | Definition |
|------|-----------|
| **XIP** | Execute-In-Place — running code directly from external flash via a cache, rather than copying to SRAM first |
| **DMA** | Direct Memory Access — hardware transfers data (e.g., audio buffers) without CPU involvement |
| **ISR** | Interrupt Service Routine — a function that runs when hardware signals an event (e.g., DMA buffer complete) |
| **I2S** | Inter-IC Sound — a serial bus protocol for transmitting digital audio between chips |
| **PIO** | Programmable I/O — RP2350's programmable state machines that can implement custom protocols like I2S |
| **DAC** | Digital-to-Analog Converter — converts digital audio samples to analog voltage for speakers |
| **FPU** | Floating-Point Unit — dedicated hardware for fast float math (single-precision on Cortex-M33) |
| **FPSCR** | Floating-Point Status and Control Register — ARM register controlling FPU behavior (rounding, denormals) |
| **FZ/DN bits** | Flush-to-Zero / Default-NaN — FPSCR flags that make the FPU treat tiny numbers as zero |
| **SPSC** | Single-Producer, Single-Consumer — a lock-free queue pattern where one thread writes and one reads |
| **SRAM** | Static RAM — fast on-chip memory (520 KB on RP2350), used for time-critical code and data |
| **SSAT** | Signed Saturate — ARM instruction that clamps a value to a signed range in one cycle |
| **ABI** | Application Binary Interface — calling convention (hard-float ABI passes floats in FPU registers) |
| **SMP** | Symmetric Multi-Processing — running an OS across multiple cores (avoided here due to RP2350 bugs) |
| **Denormal** | An extremely small float near zero that some processors handle much slower than normal numbers |
| **Padé approximant** | A rational function (ratio of polynomials) that approximates a transcendental function like tanh |
| **Polyphonic** | Each voice has its own filter with independent cutoff/envelope (Prophet-5, Jupiter-8 architecture) |
| **Paraphonic** | Multiple oscillators share one filter (cheaper, but loses per-note filter dynamics) |
| **iPlug2** | The audio plugin framework used for the desktop build of PolySynth |
| **UF2** | USB Flashing Format — the binary format flashed to the Pico via drag-and-drop |
| **SEA_FAST_MATH** | Compile flag enabling PolySynth's own polynomial math approximations instead of `libm` |

---

## Learnings

Hard-won lessons are documented in [learnings/embedded_pico.md](learnings/embedded_pico.md).
