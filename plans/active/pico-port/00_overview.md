# PolySynth Pico Port — Sprint Plan Overview

## Vision

Port the PolySynth DSP engine to run standalone on a Raspberry Pi Pico 2 W (RP2350) with a Waveshare Pico-Audio HAT (PCM5101A DAC). This is a proof-of-concept that validates the toolchain, hardware audio output, and core DSP engine on embedded hardware.

## Hardware Target

| Component | Specification |
|-----------|---------------|
| MCU | RP2350 — Dual Cortex-M33 @ 150 MHz |
| SRAM | 520 KB |
| Flash | 2 MB |
| FPU | Hardware single-precision (HW float) |
| DAC | PCM5101A (Waveshare Pico-Audio HAT) |
| I2S Pins | DIN=GPIO26, BCK=GPIO27, LRCK=GPIO28 |
| Audio | 48 kHz, 16-bit stereo |

## Sprint Structure (6 Sprints)

| Sprint | Title | Focus | Depends On |
|--------|-------|-------|------------|
| **Pico-1** | [Toolchain & Hello World](01_sprint_pico_toolchain.md) | SDK, CMake, blink + serial | — |
| **Pico-2** | [CI/CD & Emulation Pipeline](02_sprint_pico_ci_emulation.md) | Two-layer testing, Wokwi emulation | Pico-1 |
| **Pico-3** | [I2S Audio Hello World](03_sprint_pico_i2s_audio.md) | PIO I2S, DMA, 440 Hz sine | Pico-1, Pico-2 |
| **Pico-4** | [Core DSP Integration](04_sprint_pico_dsp_integration.md) | Engine → I2S, all voices working | Pico-3 |
| **Pico-5** | [Effects Architecture](05_sprint_pico_effects_architecture.md) | Deploy flags, soft clipper, effects analysis | Pico-4 |
| **Pico-6** | [Serial Control Interface](06_sprint_pico_serial_control.md) | Serial REPL, Python controller | Pico-4, Pico-5 |

## Dependency Graph

```
Pico-1 (Toolchain)
  ├── Pico-2 (CI/CD & Emulation Pipeline)  ←── Testing foundation
  │     └── All subsequent sprints require CI green
  └── Pico-3 (I2S Audio)
        └── Pico-4 (DSP Integration)
              ├── Pico-5 (Effects Architecture)
              └── Pico-6 (Serial Control) ← also depends on Pico-5
```

---

## Testing Architecture ("Hybrid" Two-Layer Approach)

Every PR must pass both testing layers before merging. No code reaches `main` without CI green.

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CI Pipeline                                  │
│                                                                     │
│  ┌──────────────────────────┐  ┌────────────────────────────────┐  │
│  │  Layer 1: Native Host    │  │  Layer 2a: ARM Cross-Compile   │  │
│  │  (x86, milliseconds)     │  │  (RP2350, compile-only)        │  │
│  │                          │  │                                │  │
│  │  • Catch2 unit tests     │  │  • cmake --build for ARM      │  │
│  │  • Embedded config flags │  │  • -Wdouble-promotion clean   │  │
│  │  • Float precision DSP   │  │  • .uf2 produced              │  │
│  │  • Deploy flag combos    │  │  • .elf size verified          │  │
│  │  • Command parser tests  │  │                                │  │
│  └──────────────────────────┘  └────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Layer 2b: Wokwi Emulated Firmware (RP2040 proxy)           │  │
│  │  (10-30 seconds per scenario)                                │  │
│  │                                                              │  │
│  │  • Firmware boots without hard-fault                         │  │
│  │  • Serial output: version string, SRAM report               │  │
│  │  • Self-test: [TEST:ALL_PASSED] marker detected             │  │
│  │  • I2S GPIO pins captured via Logic Analyzer (VCD)          │  │
│  │  • Serial protocol: NOTE_ON/OFF/SET/GET/STATUS verified     │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### Emulation Platform Reality Check

> **Neither Wokwi nor Renode officially supports the RP2350 as of March 2026.**
>
> - [Wokwi RP2350 issue #142](https://github.com/wokwi/rp2040js/issues/142) — not yet implemented
> - [Renode](https://github.com/renode/renode) — community RP2040 only, no RP2350
>
> **Our approach:** Use Wokwi's RP2040 as a behavioral proxy. The Pico SDK's HAL
> abstracts hardware differences — GPIO, PIO, DMA, USB CDC APIs are source-compatible.
> We compile a separate `pico_w` build for emulation while the primary build targets `pico2_w`.

### CI Mandate for All Sprints (Pico-3 onwards)

Every sprint's Definition of Done must include:

1. **`just test`** — Desktop tests pass (existing)
2. **`just test-embedded`** — Layer 1 embedded-config tests pass
3. **`just pico-build`** — RP2350 ARM cross-compile succeeds
4. **CI green** — All Pico CI jobs pass (Layer 1 + 2a + 2b)
5. **New tests** — Every new feature adds Catch2 tests + `[TEST:PASS]` firmware markers

---

## Memory Budget

| Component | Desktop | Pico (Est.) | Notes |
|-----------|---------|-------------|-------|
| Voice array (4 voices) | ~7.5 KB | ~2 KB | float vs double, 4 vs 16 voices |
| VoiceManager + allocator | ~1 KB | ~0.5 KB | |
| DMA audio buffers | — | ~2 KB | 2 × 256 stereo frames × int16 |
| Pico SDK overhead | — | ~20-30 KB | USB, PIO, DMA drivers |
| Stack | — | ~8 KB | |
| **Total (core only)** | | **~40 KB / 520 KB** | Comfortable |

### Effects (Disabled by Default)

| Effect | Memory | Status | Deploy Flag |
|--------|--------|--------|-------------|
| VintageChorus | ~19 KB | Portable w/ mod | `POLYSYNTH_DEPLOY_CHORUS` |
| VintageDelay | 38–750 KB | Partial (reduce max) | `POLYSYNTH_DEPLOY_DELAY` |
| LookaheadLimiter | ~330 KB | Impossible | `POLYSYNTH_DEPLOY_LIMITER` |
| PicoSoftClipper | ~100 B | Default replacement | Always on |

## Key Design Principles

1. **One codebase.** No Pico-specific forks of DSP code. Everything in `/src/core/` and `/libs/` compiles for both desktop and ARM.

2. **Compile everything, deploy selectively.** All SEA_DSP headers compile for ARM. Feature flags control what gets linked into the binary.

3. **No heap in the audio path.** Zero `new`, `malloc`, or `std::vector` in per-sample processing code.

4. **Float precision on embedded.** `sample_t = float` and `sea::Real = float` via `POLYSYNTH_USE_FLOAT` + `SEA_PLATFORM_EMBEDDED`.

5. **Desktop builds are unaffected.** All `#if defined(POLYSYNTH_DEPLOY_*)` guards are transparent on desktop (all flags ON). All `#ifndef SEA_PLATFORM_EMBEDDED` guards are transparent on desktop.

6. **Test before flash.** Every feature is verified in CI (Layer 1 native tests + Layer 2 emulation) before anyone touches physical hardware.

## File Layout (After All Sprints)

```
src/platform/pico/
├── CMakeLists.txt          # Top-level CMake for Pico target
├── pico_sdk_import.cmake   # Standard SDK bootstrap
├── pico_config.h           # Compile-time platform defines
├── pico_effects.h          # PicoSoftClipper
├── main.cpp                # Entry point + self-test + control loop
├── i2s.pio                 # PIO I2S program
├── audio_i2s_driver.h/.cpp # PIO I2S + DMA driver
├── serial_protocol.h       # Command protocol types
├── command_parser.h/.cpp   # Serial command parser
└── wokwi/                  # Wokwi emulation configuration
    ├── diagram.json         # Virtual hardware layout
    ├── wokwi.toml           # CI configuration
    └── test_boot.yaml       # Automation scenario
tools/
└── pico_control.py         # Python interactive controller
```

## Post-Sprint-6 Roadmap

| Sprint | Focus | Goal |
|--------|-------|------|
| 7 | Effects deployment | Enable Chorus (static buffers), Delay at 100ms |
| 8 | Dual-core optimization | DSP on Core 1, serial/control on Core 0 |
| 9 | Physical controls | ADC knobs, buttons, LED feedback |
| 10 | Preset system | Save/load patches via serial or flash storage |

## Build Commands

```bash
# ── Primary targets ──────────────────────────────────────────────────
just pico-build          # Build for RP2350 (real hardware)
just pico-flash          # Flash via picotool

# ── Testing (Layer 1 — no hardware needed) ───────────────────────────
just test                # Desktop tests (16 voices, double)
just test-embedded       # Embedded-config tests (4 voices, float)

# ── Emulation (Layer 2b — requires Wokwi token) ─────────────────────
just pico-build-emu      # Build for RP2040 (Wokwi proxy)
just pico-emu-test       # Run in Wokwi emulator

# ── Hardware testing ─────────────────────────────────────────────────
just pico-flash          # Flash .uf2 to Pico 2 W
minicom -D /dev/ttyACM0  # Monitor serial output

# ── Python controller ────────────────────────────────────────────────
python3 tools/pico_control.py --demo
```
