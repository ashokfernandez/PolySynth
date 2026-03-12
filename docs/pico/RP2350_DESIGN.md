# RP2350 Runtime Design Reference

Canonical reference for the PolySynth Pico 2 W firmware architecture. Covers core allocation, FreeRTOS configuration, I2S audio pipeline, inter-core communication, math workarounds, and known quirks.

**Hardware:** Raspberry Pi Pico 2 W (RP2350, dual Cortex-M33, 520 KB SRAM, 4 MB flash)
**DAC:** PCM5101A via Waveshare Pico-Audio HAT

---

## Dual-Core Architecture

```
┌─────────────────────────────┐  ┌──────────────────────────────┐
│         CORE 0              │  │          CORE 1               │
│   FreeRTOS (preemptive)     │  │    Bare-metal (DMA ISR)       │
│                             │  │                               │
│  ┌───────────────────────┐  │  │  ┌─────────────────────────┐  │
│  │ Serial Task (pri 1)   │  │  │  │  core1_audio_entry()    │  │
│  │  - Command parsing    │  │  │  │  - FPSCR: FZ+DN bits    │  │
│  │  - Demo sequencing    │  │  │  │  - pico_audio::Init()   │  │
│  │  - Voice event drain  │  │  │  │  - __wfi() loop         │  │
│  │  - Status reporting   │  │  │  │                         │  │
│  └───────────────────────┘  │  │  └─────────────────────────┘  │
│                             │  │                               │
│  ┌───────────────────────┐  │  │  ┌─────────────────────────┐  │
│  │ Idle Task (pri 0)     │  │  │  │  DMA ISR (SRAM)         │  │
│  │  - LED heartbeat      │  │  │  │  - 256-frame callback   │  │
│  └───────────────────────┘  │  │  │  - Engine.ProcessDiag() │  │
│                             │  │  │  - Soft clip + pack I2S  │  │
│  ← SPSC command queue ──── │──│──│  - Voice diagnostics     │  │
│  ── voice events (ring) ── │──│──│                         │  │
│                             │  │  └─────────────────────────┘  │
└─────────────────────────────┘  └──────────────────────────────┘
```

**Why asymmetric (not SMP)?** RP2350 FreeRTOS SMP has known spinlock issues. Single-core FreeRTOS on Core 0 avoids these entirely. Core 1 runs bare-metal with only a DMA ISR — no scheduler overhead, deterministic latency.

---

## Boot Sequence

| Step | Core | What happens |
|------|------|--------------|
| 1 | 0 | `stdio_init_all()` — USB CDC serial |
| 2 | 0 | `cyw43_arch_init()` — onboard LED via WiFi chip |
| 3 | 0 | Wait up to 3s for USB serial host (`stdio_usb_connected()`) |
| 4 | 0 | Print boot banner (version, sample rate, buffer size, engine size) |
| 5 | 0 | `pico_self_test::RunAll()` — 6 tests including I2S chain validation |
| 6 | 0 | Re-init engine cleanly after self-test |
| 7 | 0 | `multicore_launch_core1(core1_audio_entry)` |
| 8 | 1 | Set FPSCR FZ+DN bits (flush denormals to zero) |
| 9 | 1 | `pico_audio::Init(callback, dma_irq=1)` — DMA ISR routed to Core 1 |
| 10 | 1 | Push handshake via `multicore_fifo_push_blocking(1)` |
| 11 | 0 | Receive handshake, confirm audio started |
| 12 | 0 | Create FreeRTOS serial task (4 KB stack, priority 1) |
| 13 | 0 | `vTaskStartScheduler()` — blocks forever |
| 14 | 1 | Enter `__wfi()` loop — wakes only on DMA IRQ |

---

## I2S Audio Pipeline

### Hardware

| Signal | GPIO | Function |
|--------|------|----------|
| DIN (data) | 26 | PIO output pin |
| BCK (bit clock) | 27 | PIO side-set |
| LRCK (word clock) | 28 | PIO side-set |

### PIO Program (`i2s.pio`)

Left-justified I2S, 32-bit frames (16-bit audio in upper bits, 16 zero-padded).

- 8 instructions per stereo frame (64 BCK cycles)
- LRCK transitions on last `out` of each channel (1-bit delay per I2S spec)
- DAC latches on rising BCK edge
- Clock divider: `sys_clock / (48000 × 64 × 2)` = 6.144 MHz PIO clock

### DMA Double-Buffer (Ping-Pong)

```
Buffer A ──DMA──→ PIO TX FIFO ──→ DAC
Buffer B ──DMA──→ PIO TX FIFO ──→ DAC  (chains automatically)
```

- 2 × 256 frames × 2 channels × 4 bytes = **4 KB** total
- Channels A and B chain to each other (hardware chaining, zero-copy)
- Transfer size: 32-bit (matches PIO FIFO width)
- DREQ: PIO TX FIFO level (DMA paces to FIFO consumption)
- IRQ fires when channel completes → ISR fills the just-completed buffer

### DMA ISR (`__time_critical_func` — SRAM placed)

```
1. Record entry time (diagnostics)
2. Determine which channel completed (IRQ status bits)
3. Acknowledge interrupt
4. Check for underrun: is the OTHER channel still busy?
   - If not → underrun counter++
5. Reset completed channel (read address + transfer count)
6. Call audio callback: fill 256 frames
7. Record fill time (diagnostics)
```

### Audio Callback (SRAM, Core 1 ISR context)

```
1. Drain SPSC command queue (NOTE_ON, NOTE_OFF, UPDATE_STATE, PANIC)
2. For each of 256 frames:
   a. Engine.ProcessDiag(left, right, voicePeaks)
   b. Scale ×4.0 (output gain)
   c. Track peak level (atomic, relaxed)
   d. fast_tanh() soft clip — Padé approximant, <10 cycles
   e. Saturate to int16 via ARM SSAT (single cycle)
   f. Pack left-justified I2S: (int16 << 16) | 0
3. Store per-voice diagnostics (peak, freq, phaseInc, note) — atomic relaxed
4. Update visualization state
5. Push voice-change events to ring buffer (if count changed)
```

### Timing Budget

| Metric | Value |
|--------|-------|
| Sample rate | 48 kHz |
| Buffer size | 256 frames |
| Buffer period | 5.33 ms |
| Total latency (double-buffer) | ~10.67 ms |
| CPU budget per buffer | 5.33 ms @ 150 MHz = 800K cycles |
| Target CPU usage | <20% @ 4 voices |

---

## Inter-Core Communication

### Core 0 → Core 1: Command Queue (SPSC, lock-free)

```cpp
SPSCQueue<AudioCommand, 16> mCommandQueue;
```

- **Producer:** Core 0 (serial task, demo sequencer)
- **Consumer:** Core 1 (DMA ISR, drained at top of audio callback)
- **Types:** `NOTE_ON`, `NOTE_OFF`, `UPDATE_STATE`, `PANIC`
- No mutex, no spinlock — single-producer single-consumer with atomic head/tail

### Core 0 → Core 1: State Update (flag-based handoff)

```
Core 0: modify mPendingState
Core 0: spin while !mStateConsumed  (virtually never blocks)
Core 0: copy pending → mStagedState
Core 0: mStateConsumed = false
Core 0: push UPDATE_STATE command

Core 1: dequeue UPDATE_STATE
Core 1: mEngine.UpdateState(mStagedState)
Core 1: mStateConsumed = true
```

State updates happen at ≥20 ms intervals (param changes, demo sweep). Audio buffer period is ~5 ms. The spin effectively never blocks.

### Core 1 → Core 0: Voice Diagnostics (atomics, relaxed)

```cpp
std::atomic<float> mVoicePeak[4];    // per-voice amplitude
std::atomic<float> mVoiceFreq[4];    // current pitch Hz
std::atomic<float> mVoicePhaseInc[4]; // osc phase increment
std::atomic<int>   mVoiceNote[4];    // MIDI note number
```

Relaxed memory order — reads can lag by 1–2 frames. Acceptable for serial diagnostics.

### Core 1 → Core 0: Voice Events (ring buffer)

```cpp
VoiceEvent mVoiceEvents[32];
std::atomic<uint32_t> mVoiceEventHead;  // Core 1 writes
uint32_t mVoiceEventTail;               // Core 0 reads
```

Single-writer (Core 1 ISR), single-reader (Core 0 serial task). No lock needed.

**Critical rule:** No `printf` from Core 1 ISR. All logging happens on Core 0 after draining events.

---

## FreeRTOS Configuration

| Setting | Value | Rationale |
|---------|-------|-----------|
| `configNUMBER_OF_CORES` | 1 | Avoids RP2350 SMP spinlock bugs |
| `configTICK_RATE_HZ` | 1000 | 1 ms tick resolution |
| `configCPU_CLOCK_HZ` | 150 MHz | RP2350 default |
| `configTOTAL_HEAP_SIZE` | 48 KB | <10% of 520 KB SRAM |
| `configMAX_PRIORITIES` | 6 | Serial=1, future MIDI=2, WiFi=1 |
| `configENABLE_FPU` | 1 | FPU context save/restore on task switch |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | 16 | Only tested value on RP2350 |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | Full pattern check (detects deep corruption) |
| `configUSE_IDLE_HOOK` | 1 | LED heartbeat diagnostic |
| Heap algorithm | heap_4 | Best-fit with coalescing |

### Task Layout

| Task | Stack | Priority | Purpose |
|------|-------|----------|---------|
| Serial | 4 KB | 1 | Command parsing, demo, diagnostics |
| Idle | 1 KB | 0 | LED heartbeat |
| *(future)* USB MIDI | 4 KB | 2 | TinyUSB MIDI input |
| *(future)* WiFi/OSC | 8 KB | 1 | CYW43 + lwIP + OSC |

### SDK Interop

- `configSUPPORT_PICO_SYNC_INTEROP = 1` — Pico SDK mutex/semaphore works with FreeRTOS
- `configSUPPORT_PICO_TIME_INTEROP = 1` — `sleep_ms()` yields to scheduler instead of busy-waiting

---

## Memory Budget

### SRAM (520 KB)

| Component | Size | Notes |
|-----------|------|-------|
| Engine (4 voices) | ~2–4 KB | Voices, ADSR, oscillators, filters |
| Audio DMA buffers | 4 KB | 2 × 256 frames × stereo × float |
| FreeRTOS heap | 48 KB | Serial task + headroom |
| Static state | <1 KB | DMA config, SPSC queue, atomics |
| **Available** | **~460 KB** | For effects, MIDI, WiFi |
| Sin wavetable | 2 KB | 512 × float, constexpr (flash/XIP) |
| MIDI freq table | 512 B | 128 × float, constexpr (flash/XIP) |

### Flash (XIP Cached)

- Firmware: ~540 KB UF2
- Constexpr tables (wavetable, MIDI freq) reside in flash, served via XIP cache
- `__time_critical_func` code copied to SRAM at boot (ISR + audio callback)

---

## Math Workarounds (RP2350 + GCC 15.2)

### The Core Problem

`std::pow`, `std::exp2`, and `std::exp` return **incorrect results** when called from `__time_critical_func` (SRAM-placed) code on Core 1. The flash-resident libm functions produce garbage when deeply inlined into SRAM ISR context by GCC 15.2 ARM.

### Workarounds

| Problem | Solution | File |
|---------|----------|------|
| `std::pow(2, x)` broken in SRAM | `volatile Real base = 2; std::pow(base, x)` — volatile barrier prevents bad inlining | `sea_math.h` `Exp2()` |
| MIDI note → freq needs `pow(2, n/12)` | 128-entry constexpr lookup table, zero runtime math | `Voice.h` `kMidiFreqTable` |
| Per-sample `sin()` in oscillator | 512-entry wavetable with linear interp (<0.01% error) | `sea_wavetable.h` |
| Per-sample `exp()` in limiter | Cache coefficient in `SetParams()`, reuse in `Process()` | `sea_limiter.h` |
| `std::tanh()` in sigmoid | Route through `Math::Tanh()` → Padé approximant on embedded | `sea_sigmoid.h` |
| Double-precision arithmetic | 10x slower on Cortex-M33 (single-precision FPU only) | `sea_platform.h` `Real = float` |
| Denormal floats in filter tails | FPSCR FZ+DN bits at Core 1 startup (10–80x perf penalty avoided) | `main.cpp` |

### Design Principle: Cacheable vs Signal-Dependent

**Process() should never compute cacheable transcendentals.** Cache them in SetParams().

- **Cacheable** (BUG if per-sample): Output depends only on parameters. Example: `exp(-1/(releaseMs*sr))`. Fix: compute in `SetParams()`.
- **Signal-dependent** (OK per-sample): Output depends on audio signal. Example: `tanh(signal)` for filter saturation. Fix: use fastest implementation (wavetable, Padé).

---

## Build Configuration

### Compile Definitions

| Define | Value | Purpose |
|--------|-------|---------|
| `POLYSYNTH_USE_FLOAT` | — | `sample_t = float` (not double) |
| `POLYSYNTH_MAX_VOICES` | 4 | Fits in SRAM with effects disabled |
| `PICO_AUDIO_SAMPLE_RATE` | 48000 | — |
| `PICO_AUDIO_BUFFER_FRAMES` | 256 | 5.33 ms latency per buffer |
| `SEA_FAST_MATH` | — | Wavetable sin, Padé tanh, reciprocal trig |
| `SEA_PLATFORM_EMBEDDED` | — | Float types, volatile barriers |
| `POLYSYNTH_DEPLOY_CHORUS` | 0 | Disabled to fit in SRAM |
| `POLYSYNTH_DEPLOY_DELAY` | 0 | Disabled to fit in SRAM |
| `POLYSYNTH_DEPLOY_LIMITER` | 0 | Disabled to fit in SRAM |

### Compiler Flags

| Flag | Purpose |
|------|---------|
| `-mfloat-abi=hard` | Hardware float ABI (5–10% DSP speedup over softfp) |
| `-ffast-math` | Unsafe FP transforms (safe here: no NaN/Inf deps) |
| `-fno-exceptions -fno-rtti` | Smaller binary, no overhead |
| `-funroll-loops` | Helps ladder filter inner loops |
| `-Wdouble-promotion` | Catches float→double promotions (10x cost) |

### Build Commands

```bash
just pico-build          # Configure + build firmware (.uf2)
just pico-flash          # Build + flash via picotool
just pico-clean          # Remove build/pico
just pico-serial         # Open interactive serial terminal (minicom)
just pico-serial-listen 10  # Capture serial output for N seconds
just test-embedded       # Unit tests with embedded compile flags
just pico-build-emu      # Build for Wokwi emulation (RP2040 proxy)
just pico-emu-test       # Run Wokwi CI test
```

---

## Known Quirks & Gotchas

| # | Quirk | Detail |
|---|-------|--------|
| 1 | **libm broken in SRAM ISR** | `std::pow`/`exp2`/`exp` return garbage from `__time_critical_func` on Core 1 with GCC 15.2. Use volatile barriers or lookup tables. |
| 2 | **Denormals are expensive** | Filter tails produce subnormals; 10–80x arithmetic cost. Set FPSCR FZ+DN bits at Core 1 startup. |
| 3 | **No printf from Core 1** | USB CDC serial is not ISR-safe. Buffer events; drain and print on Core 0. |
| 4 | **`picotool reboot` false error** | Reports `ERROR: rebooting` because USB drops on reboot. Flash actually succeeded. |
| 5 | **`-fno-rtti` warns on C files** | Pico SDK C sources trigger harmless warning. Fix with `$<COMPILE_LANGUAGE:CXX>` generator expression. |
| 6 | **`configMAX_SYSCALL_INTERRUPT_PRIORITY = 16`** | Only tested value on RP2350. Don't change without testing. |
| 7 | **FreeRTOS SMP broken on RP2350** | Known spinlock issues. Use `configNUMBER_OF_CORES=1` (Core 0 only). |
| 8 | **Double is 10x slower** | Cortex-M33 has single-precision FPU only. All DSP must use `float`. `-Wdouble-promotion` catches mistakes. |
| 9 | **XIP cache misses in ISR** | Flash reads can stall 10–100+ cycles. All ISR code must be `__time_critical_func` (SRAM-placed). |
| 10 | **Stack sizes in words, not bytes** | `xTaskCreate` stack parameter is in words (×4 for bytes). 1024 words = 4 KB. |
| 11 | **CYW43 LED needs init before scheduler** | `cyw43_arch_init()` must run before `vTaskStartScheduler()`. Otherwise GPIO access races with FreeRTOS. |
| 12 | **TinyUSB vTaskDelay conflict** | TinyUSB calls `vTaskDelay` internally — don't use before scheduler starts. SDK interop flag resolves. |
| 13 | **Tickless idle unsupported** | RP2350 FreeRTOS port doesn't support `configUSE_TICKLESS_IDLE`. Leave at 0. |

---

## File Map

| File | Purpose | Core |
|------|---------|------|
| `main.cpp` | Boot sequence, Core 1 launch, FreeRTOS scheduler | 0 + 1 init |
| `audio_i2s_driver.h/cpp` | PIO + DMA double-buffer I2S | 1 (ISR) |
| `i2s.pio` | PIO I2S assembly program | 1 (hardware) |
| `pico_synth_app.h/cpp` | Engine wrapper, cross-core sync, audio callback | 0 (API) + 1 (ISR) |
| `pico_demo.h/cpp` | Demo sequencer state machine | 0 |
| `pico_command_dispatch.h/cpp` | Serial command routing | 0 |
| `command_parser.h/cpp` | Zero-alloc serial line tokenizer | 0 |
| `pico_self_test.h/cpp` | 6-stage boot self-test | 0 |
| `sine_generator.h` | Test waveform generator | 0 (test only) |
| `FreeRTOSConfig.h` | RTOS tuning (single-core, 48 KB heap) | 0 |
| `CMakeLists.txt` | Cross-compile config, flags, link libs | build-time |
