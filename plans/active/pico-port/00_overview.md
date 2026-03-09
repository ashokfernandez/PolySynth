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

## Sprint Structure

| Sprint | Title | Focus | Depends On |
|--------|-------|-------|------------|
| **Pico-1** | [Toolchain & Hello World](01_sprint_pico_toolchain.md) | SDK, CMake, blink + serial | — |
| **Pico-2** | [I2S Audio Hello World](02_sprint_pico_i2s_audio.md) | PIO I2S, DMA, 440 Hz sine | Pico-1 |
| **Pico-3** | [Core DSP Integration](03_sprint_pico_dsp_integration.md) | Engine → I2S, all voices working | Pico-2 |
| **Pico-4** | [Effects Architecture](04_sprint_pico_effects_architecture.md) | Deploy flags, soft clipper, effects analysis | Pico-3 |
| **Pico-5** | [Serial Control Interface](05_sprint_pico_serial_control.md) | Serial REPL, Python controller | Pico-3, Pico-4 |

## Dependency Graph

```
Pico-1 (Toolchain)
  └── Pico-2 (I2S Audio)
        └── Pico-3 (DSP Integration)
              ├── Pico-4 (Effects Architecture)
              └── Pico-5 (Serial Control) ← also depends on Pico-4
```

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

## File Layout (After All Sprints)

```
src/platform/pico/
├── CMakeLists.txt          # Top-level CMake for Pico target
├── pico_sdk_import.cmake   # Standard SDK bootstrap
├── pico_config.h           # Compile-time platform defines
├── pico_effects.h          # PicoSoftClipper
├── main.cpp                # Entry point + control loop
├── i2s.pio                 # PIO I2S program
├── audio_i2s_driver.h/.cpp # PIO I2S + DMA driver
├── serial_protocol.h       # Command protocol types
└── command_parser.h/.cpp   # Serial command parser
tools/
└── pico_control.py         # Python interactive controller
```

## Post-Sprint-5 Roadmap

| Sprint | Focus | Goal |
|--------|-------|------|
| 6 | Effects deployment | Enable Chorus (static buffers), Delay at 100ms |
| 7 | Dual-core optimization | DSP on Core 1, serial/control on Core 0 |
| 8 | Physical controls | ADC knobs, buttons, LED feedback |
| 9 | Preset system | Save/load patches via serial or flash storage |

## Build Commands

```bash
# Build Pico firmware
just pico-build

# Flash (picotool)
just pico-flash

# Flash (manual — hold BOOTSEL, connect USB)
cp build/pico/polysynth_pico.uf2 /media/$USER/RP2350/

# Monitor serial
minicom -D /dev/ttyACM0 -b 115200

# Desktop regression check
just build && just test

# Python controller
python3 tools/pico_control.py --demo
```
