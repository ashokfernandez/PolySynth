# Sprint Pico-7: Performance Foundations

**Depends on:** Sprint Pico-6 (Serial Control — shipped)
**Blocks:** Sprint Pico-8 (Architecture Decomposition)
**Approach:** Small, targeted fixes to the existing monolithic main.cpp. No restructuring.
**Estimated effort:** 1-2 days

> **Why this sprint exists:** The shipped firmware (Sprints 1-6) produces choppy audio
> under real-world conditions. Research into RP2350 specifics identified several root
> causes that are cheap to fix in the current code before restructuring.

---

## Goal

Fix the immediate audio quality issues without restructuring the codebase. Each task
is a small, safe, independently testable change to the existing monolithic main.cpp
and supporting files. After this sprint, the synth should produce clean, glitch-free
audio on hardware.

---

## Task 7.1: SRAM Placement for Audio Code

### Problem

All firmware code runs from external QSPI flash through the XIP (Execute-In-Place)
cache. When the DMA ISR fires on Core 1 and the instruction cache doesn't contain the
next DSP function, execution stalls while flash is fetched. With branchy DSP code
(switch on waveform, ADSR stage, filter model), cache misses are virtually guaranteed.

This is the most likely cause of choppy audio — fill time varies unpredictably based on
cache state, causing intermittent DMA underruns.

### Fix

Add `__time_critical_func` attribute to place audio-critical functions in SRAM:

```cpp
// audio_i2s_driver.cpp
static void __time_critical_func(dma_irq_handler)() { ... }

// main.cpp
static void __time_critical_func(audio_callback)(uint32_t* buffer, uint32_t num_frames) { ... }
```

Since SEA_DSP is header-only and inlined into `audio_callback`, the inlined DSP code
also ends up in SRAM automatically.

### SRAM cost

Estimated 4-8 KB of code. Out of 520 KB total — negligible.

### Validation

- `STATUS` command before/after: CPU% should be more stable (less variance)
- Underrun count should drop to zero during normal 4-voice playback
- Binary size increases slightly (SRAM usage in .elf map)

---

## Task 7.2: Denormal Flush-to-Zero on Core 1

### Problem

IIR filter feedback paths produce denormal floats (extremely small numbers near zero
that the FPU handles much slower than normal numbers) as signals decay toward zero.
While the M33 FPU handles denormals in hardware, edge cases in filter tails can still
cause multi-cycle penalties.

### Fix

Set FPSCR FZ (flush-to-zero) and DN (default-NaN) bits at Core 1 startup:

```cpp
static void core1_audio_entry() {
    // Flush denormals to zero — prevents performance cliffs in filter tails
    uint32_t fpscr = __get_FPSCR();
    __set_FPSCR(fpscr | (1 << 24) | (1 << 25));  // FZ=1, DN=1

    if (!pico_audio::Init(audio_callback, 1)) { ... }
    // ...
}
```

One line of code. Zero risk.

### Validation

- Read back FPSCR after setting and log the value at boot to confirm bits are set
- Filter tail behavior: play a note, release, verify CPU% doesn't spike during decay

---

## Task 7.2b: Fix `s_peakLevel` Data Race (PR #65 Review)

### Problem

Both the DMA ISR (Core 1) and the main loop (Core 0) read/write `s_peakLevel` without
atomics. On Cortex-M33 single-word float stores are atomic in practice, but it's
technically undefined behavior per the C++ standard. Flagged in PR #65 review.

### Fix

Use `std::atomic<float>` with relaxed ordering, or a simple exchange pattern:

```cpp
static std::atomic<float> s_peakLevel{0.0f};

// In ISR: update peak (relaxed — no ordering needed, just visibility)
float peak = ...;
float current = s_peakLevel.load(std::memory_order_relaxed);
if (peak > current) s_peakLevel.store(peak, std::memory_order_relaxed);

// In main loop: read and reset
float peak = s_peakLevel.exchange(0.0f, std::memory_order_relaxed);
```

### Also: Add comment to `s_prevVoiceCount` (PR #65 Review)

```cpp
static int s_prevVoiceCount = 0;  // ISR-private: only accessed from audio_callback
```

---

## Task 7.3: Remove UpdateVisualization() from DMA ISR

### Problem

`Engine::UpdateVisualization()` is called from the DMA ISR every 256-sample buffer
(main.cpp:137). It:
1. Iterates all voices to count active ones
2. Iterates all voices again with O(n^2) dedup for held notes
3. Stores to `std::atomic<uint64_t>` x 2 — NOT lock-free on 32-bit ARM Cortex-M33

The 64-bit atomic stores disable interrupts or use a hardware spinlock from within
the DMA ISR on Core 1, while Core 0's main loop may be reading the same atomics.

### Fix

Remove the `s_engine.UpdateVisualization()` call from `audio_callback()`. The simple
`std::atomic<int>` voice count (already at main.cpp:139) is sufficient for STATUS
reporting. The held-note bitmask feature is not used by any current consumer.

```diff
-    s_engine.UpdateVisualization();
     int vc = s_engine.GetActiveVoiceCount();
     s_activeVoices.store(vc, std::memory_order_relaxed);
```

### Validation

- Voice count still reported correctly in STATUS command
- No functional regression (held-note display not implemented on Pico)

---

## Task 7.4: Cache Oscillator Phase Increment

### Problem

`Oscillator::SetFrequency()` performs `freq / mSampleRate` (a VDIV.F32 at ~14 cycles)
every sample for every oscillator. Called from Voice::Process() lines 176 & 186.
With 4 voices x 2 oscillators = 8 divisions per sample = 2048 per buffer = ~29K cycles.

### Fix

Two changes to `sea_oscillator.h`:

1. Cache `mInvSampleRate = 1.0 / sampleRate` in `Init()` (one-time division)
2. Use multiply instead of divide in `SetFrequency()`:
   ```cpp
   mPhaseIncrement = freq * mInvSampleRate;
   ```

### Validation

- `just test` and `just test-embedded` pass (oscillator output unchanged)
- Add unit test: verify phase increment matches for several frequencies
- CPU% should drop measurably (~3-4% at 4 voices)

---

## Task 7.5: Add Compiler Optimization Flags

### Problem

The current CMakeLists.txt doesn't enable `-ffast-math` or `-funroll-loops`. The
compiler cannot replace divisions with reciprocal multiplications or unroll the
ladder filter's 4-stage loop.

### Fix

```cmake
target_compile_options(polysynth_pico PRIVATE
    -Wall -Wextra
    -Wdouble-promotion
    -fno-exceptions
    -fno-rtti
    -ffast-math              # reciprocal-math, FMA, reordering
    -funroll-loops           # helps ladder filter 4-stage loop
)
```

Note: `-ffast-math` may conflict with SDK ROM math functions. Since we use
`SEA_FAST_MATH` polynomial approximants (not `sinf`/`cosf`), this should be safe.
Test thoroughly.

### Validation

- `just pico-build` succeeds
- `just test` and `just test-embedded` pass
- Flash and verify audio output on hardware
- CPU% should improve slightly

---

## Definition of Done

- [ ] `__time_critical_func` on `dma_irq_handler` and `audio_callback`
- [ ] FPSCR FZ+DN bits set on Core 1 (verified via boot log readback)
- [ ] `UpdateVisualization()` removed from DMA ISR
- [ ] `s_peakLevel` converted to `std::atomic<float>` (PR #65 review fix)
- [ ] `s_prevVoiceCount` has ISR-private comment (PR #65 review fix)
- [ ] Oscillator uses cached `mInvSampleRate` (multiply, not divide)
- [ ] `-ffast-math -funroll-loops` added to CMake
- [ ] `just test` passes
- [ ] `just test-embedded` passes
- [ ] `just pico-build` succeeds
- [ ] Hardware test: clean audio, zero underruns with 4 voices over 60 seconds
- [ ] STATUS reports stable CPU% (variance <2% between 3 consecutive readings)
