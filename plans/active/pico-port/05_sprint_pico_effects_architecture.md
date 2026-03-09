# Sprint Pico-5: Effects Architecture & Deploy Flags

**Depends on:** Sprint Pico-4 (Core DSP Integration)
**Blocks:** Sprint Pico-6 (Serial Control), future effects enablement sprints
**Approach:** Analysis-first, then TDD for deploy guards and soft clipper
**Estimated effort:** 2–3 days

> **CI Mandate (from Sprint Pico-2):** This sprint's Definition of Done requires:
> 1. `just test` — Desktop tests pass (deploy flags transparent when all ON)
> 2. `just test-embedded` — Layer 1 tests pass (deploy flags OFF path exercised)
> 3. `just pico-build` — RP2350 builds with default flags OFF and with all flags ON
> 4. PicoSoftClipper unit tests pass in both desktop and embedded configurations
> 5. CI verifies both flag-OFF and flag-ON Pico compile configurations

---

## Goal

Establish a compile-time feature-flag architecture that lets every DSP component compile for ARM, but selectively controls what gets linked into the deployed binary. Analyze every effect for embedded feasibility. Replace the temporary `tanhf()` soft clipper from Sprint 3 with a proper `PicoSoftClipper` class. Produce a documented effects deployment matrix.

### Philosophy: "Port Everything, Deploy Selectively"

Every SEA_DSP effect header must compile cleanly for ARM. But the linked binary only includes components that fit within the 520KB SRAM budget. Feature flags control deployment, not compilation.

---

## Prerequisites

- Sprint Pico-3 complete (Engine producing audio through DAC, CPU < 30%)
- Understanding of memory budget constraints from Sprint 3's `.elf` map file analysis
- Familiarity with `Engine.h` effects chain: voices → gain → chorus → delay → limiter

---

## Task 4.1: Effects Porting Analysis

### What

For each effect in the Engine's signal chain, produce a detailed analysis of its embedded feasibility. This is a documentation task — no code changes yet.

### Analysis Document

Include this analysis in the PR description and update this plan file with actual measurements after Sprint 3's map file analysis is complete.

---

#### VintageChorus (`sea_vintage_chorus.h`)

**Desktop memory:** ~19 KB (delay line buffers allocated via `std::vector`)

**Status: PORTABLE WITH MODIFICATION**

**Issue:** Uses `std::vector` for internal delay line buffers. `std::vector` means heap allocation, which:
1. Violates the "no heap in audio path" rule
2. Adds unpredictable allocation time
3. Makes memory usage invisible to the linker map

**Solution path:**
- SEA_DSP's `DelayLine` class supports an `Init(T* external_buffer, size_t)` overload that uses externally-provided static buffers
- Create static `float[]` buffers at file scope and pass them to `Init()`
- This is a future sprint task (Sprint 5+ roadmap) — not this sprint

**Deploy flag:** `POLYSYNTH_DEPLOY_CHORUS` — OFF by default on Pico

**Memory if deployed:** ~19 KB with static buffers → fits in SRAM budget

---

#### VintageDelay (`sea_vintage_delay.h`)

**Desktop memory:** ~750 KB (2 × 96000 float samples for 2-second stereo delay at 48 kHz)

**Status: IMPOSSIBLE AT CURRENT SETTINGS**

**The math:**
```
Max delay time:     2000 ms (default)
Sample rate:        48000 Hz
Samples per channel: 2000ms × 48 = 96000
Channels:           2 (stereo)
Bytes per sample:   4 (float)
Total:              2 × 96000 × 4 = 768,000 bytes = 750 KB
Available SRAM:     520 KB total
```

750 KB > 520 KB. This effect physically cannot fit in SRAM at its default settings.

**Options:**
| Option | Max Delay | Memory | Feasibility |
|--------|-----------|--------|-------------|
| Reduce to 100ms | 100 ms | ~38 KB | Possible (short slapback echo) |
| Reduce to 50ms | 50 ms | ~19 KB | Possible (doubling effect) |
| External SPI PSRAM | 2000 ms | 0 KB SRAM | Requires hardware mod |
| Disable entirely | — | 0 KB | Default for Pico |

**Deploy flag:** `POLYSYNTH_DEPLOY_DELAY` — OFF by default. If enabled, `POLYSYNTH_DELAY_MAX_MS` must be set to a value that fits in SRAM (e.g., 100).

---

#### LookaheadLimiter (`sea_limiter.h`)

**Desktop memory:** ~330 KB (`kMaxBufferSize = 16384` fixed arrays)

**The math:**
```
Buffer size:        16384 entries
Data per entry:     ~5 floats (input, gain, lookahead, etc.)
Total:              16384 × 5 × 4 = 327,680 bytes ≈ 330 KB
```

**Status: IMPOSSIBLE**

330 KB is 63% of total SRAM. Not deployable on Pico.

**Replacement: PicoSoftClipper** (Task 4.3)
- `tanhf(x * drive)` — single function call, ~100 bytes code, zero state
- Provides basic output protection without the lookahead sophistication
- Good enough for a PoC — audible limiting on extreme signals

**Deploy flag:** `POLYSYNTH_DEPLOY_LIMITER` — OFF by default, replaced by `PicoSoftClipper`

---

### Effects Deployment Matrix

| Effect | Compiles? | Deployable? | SRAM Cost | Deploy Flag | Notes |
|--------|-----------|-------------|-----------|-------------|-------|
| Oscillators (2/voice) | Yes | Yes | ~40B/voice | Always on | Core signal path |
| Filters (4 models) | Yes | Yes | ~80B/voice | Always on | Core signal path |
| ADSR Envelopes (2/voice) | Yes | Yes | ~96B/voice | Always on | Core signal path |
| LFO | Yes | Yes | ~40B/voice | Always on | Core signal path |
| VintageChorus | Yes | Yes (w/ mod) | ~19 KB | `POLYSYNTH_DEPLOY_CHORUS` | Convert delay line to static buffers |
| VintageDelay | Yes | Partial | 38–750 KB | `POLYSYNTH_DEPLOY_DELAY` | Must reduce max from 2s to 50-100ms |
| LookaheadLimiter | Yes | No | ~330 KB | `POLYSYNTH_DEPLOY_LIMITER` | Replace with PicoSoftClipper |
| **PicoSoftClipper** | Yes | Yes | ~100 B | Pico default | `tanhf(x * drive)` replacement |

### Acceptance Criteria

- [ ] Analysis documented for Chorus, Delay, and Limiter
- [ ] Memory calculations verified against actual `.elf` map data from Sprint 3
- [ ] Deployment matrix table completed

---

## Task 4.2: Create CMake Feature-Flag System

### What

Add CMake options that control which effects are linked into the Pico binary. These flags enable/disable effect **members and processing** in Engine.h — the headers still compile.

### File to Modify

**`/src/platform/pico/CMakeLists.txt`** — Add deploy options:

```cmake
# ── Effect deployment flags ──────────────────────────────────────────────
# These control which effects are INSTANTIATED in the Engine.
# All SEA_DSP headers still compile — only member variables and processing
# are conditionally included.
option(POLYSYNTH_DEPLOY_CHORUS  "Link VintageChorus into binary"        OFF)
option(POLYSYNTH_DEPLOY_DELAY   "Link VintageDelay into binary"         OFF)
option(POLYSYNTH_DEPLOY_LIMITER "Link LookaheadLimiter into binary"     OFF)

# Propagate as compile definitions
if(POLYSYNTH_DEPLOY_CHORUS)
    target_compile_definitions(polysynth_pico PRIVATE POLYSYNTH_DEPLOY_CHORUS)
endif()
if(POLYSYNTH_DEPLOY_DELAY)
    target_compile_definitions(polysynth_pico PRIVATE POLYSYNTH_DEPLOY_DELAY)
endif()
if(POLYSYNTH_DEPLOY_LIMITER)
    target_compile_definitions(polysynth_pico PRIVATE POLYSYNTH_DEPLOY_LIMITER)
endif()
```

### Desktop Build: All Flags ON

For the desktop build, all effects are always enabled. Add to **`/tests/CMakeLists.txt`**:

```cmake
# Effects always deployed on desktop — ensures Engine.h guards are transparent
target_compile_definitions(run_tests PRIVATE
    POLYSYNTH_DEPLOY_CHORUS
    POLYSYNTH_DEPLOY_DELAY
    POLYSYNTH_DEPLOY_LIMITER
)
```

**Also add to all demo targets** that link Engine.h.

### TDD: Deploy Matrix Build Test

Test every combination compiles:

```bash
# All OFF (default Pico config)
cmake -S src/platform/pico -B build/pico-test-000 \
    -DPICO_SDK_PATH=$PICO_SDK_PATH
cmake --build build/pico-test-000

# Chorus ON only
cmake -S src/platform/pico -B build/pico-test-100 \
    -DPICO_SDK_PATH=$PICO_SDK_PATH \
    -DPOLYSYNTH_DEPLOY_CHORUS=ON
cmake --build build/pico-test-100

# All ON (would exceed SRAM — but must compile)
cmake -S src/platform/pico -B build/pico-test-111 \
    -DPICO_SDK_PATH=$PICO_SDK_PATH \
    -DPOLYSYNTH_DEPLOY_CHORUS=ON \
    -DPOLYSYNTH_DEPLOY_DELAY=ON \
    -DPOLYSYNTH_DEPLOY_LIMITER=ON
cmake --build build/pico-test-111
```

### Acceptance Criteria

- [ ] All 8 combinations of deploy flags compile without errors
- [ ] Desktop build has all flags ON (transparent — no behavior change)
- [ ] `just build && just test` still passes
- [ ] CMake options documented with descriptions

---

## Task 4.3: Create PicoSoftClipper

### What

Create a lightweight output protection effect that replaces `LookaheadLimiter` on the Pico. Uses `tanhf()` for soft clipping — mathematically smooth, CPU-cheap, zero state.

### File to Create

**`/src/platform/pico/pico_effects.h`**

```cpp
#pragma once

#include <cmath>

namespace pico_fx {

// PicoSoftClipper — Lightweight output protection for embedded targets.
// Replaces LookaheadLimiter on platforms where 330KB buffers won't fit.
//
// Uses tanhf() for mathematically smooth saturation:
//   output = tanhf(input * drive)
//
// Properties:
//   - Zero state (no buffers, no initialization needed)
//   - ~50-100 cycles per sample on Cortex-M33
//   - Memory: ~100 bytes (code only, no data)
//   - Smooth saturation curve — no hard clipping artifacts
//   - At drive=1.0: gentle compression above ±0.76
//   - At drive=2.0: more aggressive, reaches ±0.96 at input ±1.0
//
// Limitations vs. LookaheadLimiter:
//   - No lookahead — cannot prevent transient overshoot
//   - No gain reduction metering
//   - Fixed curve — no threshold/release controls
//   - Adds harmonic distortion (odd harmonics) at high drive
class PicoSoftClipper
{
public:
    PicoSoftClipper() = default;

    void SetDrive(float drive) { mDrive = drive; }
    float GetDrive() const { return mDrive; }

    // Process a single stereo pair in-place
    void Process(float& left, float& right) const
    {
        left  = tanhf(left  * mDrive);
        right = tanhf(right * mDrive);
    }

    // Process a single mono sample
    float Process(float sample) const
    {
        return tanhf(sample * mDrive);
    }

private:
    float mDrive = 1.0f;  // 1.0 = gentle, 2.0 = aggressive
};

} // namespace pico_fx
```

### TDD: Unit Tests

**Create `/tests/unit/Test_PicoSoftClipper.cpp`:**

```cpp
#include "../../src/platform/pico/pico_effects.h"
#include "catch.hpp"
#include <cmath>

TEST_CASE("PicoSoftClipper: zero input produces zero output", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    float left = 0.0f, right = 0.0f;
    clipper.Process(left, right);
    REQUIRE(left == Approx(0.0f));
    REQUIRE(right == Approx(0.0f));
}

TEST_CASE("PicoSoftClipper: small signals pass through nearly unchanged", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    float left = 0.1f, right = -0.1f;
    clipper.Process(left, right);
    // tanhf(0.1) ≈ 0.0997 — nearly linear for small inputs
    REQUIRE(left == Approx(0.1f).margin(0.01f));
    REQUIRE(right == Approx(-0.1f).margin(0.01f));
}

TEST_CASE("PicoSoftClipper: large signals are compressed", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    float left = 3.0f, right = -3.0f;
    clipper.Process(left, right);
    // tanhf(3.0) ≈ 0.995 — compressed to near ±1.0
    REQUIRE(left == Approx(0.995f).margin(0.01f));
    REQUIRE(right == Approx(-0.995f).margin(0.01f));
}

TEST_CASE("PicoSoftClipper: output never exceeds ±1.0", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    clipper.SetDrive(2.0f);

    for (float input = -10.0f; input <= 10.0f; input += 0.1f) {
        float result = clipper.Process(input);
        REQUIRE(result >= -1.0f);
        REQUIRE(result <= 1.0f);
    }
}

TEST_CASE("PicoSoftClipper: drive parameter affects compression amount", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper gentle;
    gentle.SetDrive(1.0f);

    pico_fx::PicoSoftClipper aggressive;
    aggressive.SetDrive(3.0f);

    float input = 0.5f;
    float gentle_out = gentle.Process(input);
    float aggressive_out = aggressive.Process(input);

    // Higher drive = more compression = lower output for same input
    // Actually: tanhf(0.5 * 1.0) = 0.462, tanhf(0.5 * 3.0) = 0.905
    // Higher drive pushes the signal further into saturation
    REQUIRE(aggressive_out > gentle_out);
}

TEST_CASE("PicoSoftClipper: symmetry — positive and negative inputs", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    float pos = 0.7f;
    float neg = -0.7f;
    float pos_out = clipper.Process(pos);
    float neg_out = clipper.Process(neg);
    REQUIRE(pos_out == Approx(-neg_out).margin(0.0001f));
}

TEST_CASE("PicoSoftClipper: default drive is 1.0", "[PicoSoftClipper]") {
    pico_fx::PicoSoftClipper clipper;
    REQUIRE(clipper.GetDrive() == Approx(1.0f));
}
```

**Add to `/tests/CMakeLists.txt`:**
Add `unit/Test_PicoSoftClipper.cpp` to `UNIT_TEST_SOURCES`.

### Acceptance Criteria

- [ ] All 7 PicoSoftClipper tests pass
- [ ] Output never exceeds ±1.0 for any input
- [ ] Zero state — no initialization required
- [ ] Compiles for both desktop and ARM targets
- [ ] Header usable from both Pico and test code (no Pico SDK dependency)

---

## Task 4.4: Add Deploy Guards to Engine.h

### What

Wrap effect members and processing in `#if POLYSYNTH_DEPLOY_*` guards. When an effect flag is OFF, the member variable doesn't exist and the processing is skipped. When ON (desktop default), everything works as before.

### File to Modify

**`/src/core/Engine.h`** — Wrap effect includes, members, and processing:

#### Effect Includes

```cpp
// At the top of Engine.h, with other includes:
#if defined(POLYSYNTH_DEPLOY_CHORUS)
#include <sea_dsp/sea_vintage_chorus.h>
#endif

#if defined(POLYSYNTH_DEPLOY_DELAY)
#include <sea_dsp/sea_vintage_delay.h>
#endif

#if defined(POLYSYNTH_DEPLOY_LIMITER)
#include <sea_dsp/sea_limiter.h>
#endif
```

#### Effect Members

```cpp
// In the private section of Engine class:
#if defined(POLYSYNTH_DEPLOY_CHORUS)
    sea::VintageChorus<sample_t> mChorus;
#endif

#if defined(POLYSYNTH_DEPLOY_DELAY)
    sea::VintageDelay<sample_t> mDelay;
#endif

#if defined(POLYSYNTH_DEPLOY_LIMITER)
    sea::LookaheadLimiter<sample_t> mLimiter;
#endif
```

#### Init() Method

```cpp
void Init(double sampleRate) {
    mVoiceManager.Init(static_cast<sample_t>(sampleRate));
#if defined(POLYSYNTH_DEPLOY_CHORUS)
    mChorus.Init(sampleRate);
#endif
#if defined(POLYSYNTH_DEPLOY_DELAY)
    mDelay.Init(sampleRate);
#endif
#if defined(POLYSYNTH_DEPLOY_LIMITER)
    mLimiter.Init(sampleRate);
#endif
}
```

#### Process() Method

```cpp
void Process(sample_t &left, sample_t &right) {
    mVoiceManager.ProcessStereo(left, right);

    // Apply master gain
    left  *= static_cast<sample_t>(mGain);
    right *= static_cast<sample_t>(mGain);

#if defined(POLYSYNTH_DEPLOY_CHORUS)
    mChorus.Process(left, right);
#endif

#if defined(POLYSYNTH_DEPLOY_DELAY)
    mDelay.Process(left, right);
#endif

#if defined(POLYSYNTH_DEPLOY_LIMITER)
    mLimiter.Process(left, right);
#endif

#ifndef SEA_PLATFORM_EMBEDDED
    UpdateVisualization();
#endif
}
```

#### UpdateState() Method

```cpp
void UpdateState(const SynthState& state) {
    // Voice parameters always applied...
    // ...

#if defined(POLYSYNTH_DEPLOY_CHORUS)
    SetChorus(state.fxChorusRate, state.fxChorusDepth, state.fxChorusMix);
#endif
#if defined(POLYSYNTH_DEPLOY_DELAY)
    SetDelay(state.fxDelayTime, state.fxDelayFeedback, state.fxDelayMix);
#endif
#if defined(POLYSYNTH_DEPLOY_LIMITER)
    SetLimiter(state.fxLimiterThreshold);
#endif
}
```

#### Setter Methods

Wrap `SetChorus()`, `SetDelay()`, `SetDelayTempo()`, `SetLimiter()` in their respective `#if defined(...)` guards.

### CRITICAL: Desktop Compatibility

On desktop, all three deploy flags are defined (via `tests/CMakeLists.txt` and the desktop build). This means:
- All tests pass unchanged
- All effects are included
- The guards are completely transparent

**Do NOT** change the guard style to `#ifdef` — use `#if defined()` for consistency with multi-flag expressions if needed later.

### TDD: Verification Matrix

```bash
# Desktop: all tests pass with all flags ON
just build && just test

# Pico: all OFF (default)
just pico-build

# Pico: each combination
cmake -S src/platform/pico -B build/pico-c -DPOLYSYNTH_DEPLOY_CHORUS=ON ...
cmake -S src/platform/pico -B build/pico-d -DPOLYSYNTH_DEPLOY_DELAY=ON ...
cmake -S src/platform/pico -B build/pico-l -DPOLYSYNTH_DEPLOY_LIMITER=ON ...
```

### Acceptance Criteria

- [ ] Desktop: `just build && just test` passes with zero failures (all flags ON)
- [ ] Pico: builds with all flags OFF
- [ ] Pico: builds with each flag individually ON
- [ ] Pico: builds with all flags ON (compiles, even if binary exceeds SRAM)
- [ ] No effect code appears in Pico binary when flag is OFF (verify via `nm`)
- [ ] `SetChorus/SetDelay/SetLimiter` are guarded (no compilation if effect not deployed)

---

## Task 4.5: Integrate PicoSoftClipper in Pico Main

### What

Replace the ad-hoc `tanhf()` in Sprint 3's audio callback with the proper `PicoSoftClipper` class. This gives us a named, tested, configurable output protection stage.

### File to Modify

**`/src/platform/pico/main.cpp`** — Update audio callback:

```cpp
#include "pico_effects.h"

static pico_fx::PicoSoftClipper s_clipper;

static void audio_callback(int16_t* buffer, uint32_t num_frames)
{
    uint32_t start = time_us_32();

    for (uint32_t i = 0; i < num_frames; i++) {
        float left = 0.0f;
        float right = 0.0f;

        s_engine.Process(left, right);

        // Output protection — replaces LookaheadLimiter on Pico
        s_clipper.Process(left, right);

        buffer[i * 2]     = static_cast<int16_t>(left  * 32767.0f);
        buffer[i * 2 + 1] = static_cast<int16_t>(right * 32767.0f);
    }

    s_process_time_us = time_us_32() - start;
}
```

### Acceptance Criteria

- [ ] `tanhf()` calls removed from audio callback
- [ ] `PicoSoftClipper` used instead
- [ ] Audio output unchanged (same mathematical operation, just encapsulated)

---

## Task 4.6: Update CI Pico Compile Check

### What

Update the CI job to test deploy flag combinations, ensuring the flag system doesn't break in future PRs.

### File to Modify

**`/.github/workflows/ci.yml`** — Update `pico-compile-check` job:

```yaml
      - name: Build Pico firmware (all effects OFF)
        env:
          PICO_SDK_PATH: /tmp/pico-sdk
        run: |
          cmake -S src/platform/pico -B build/pico-default \
            -DPICO_SDK_PATH=$PICO_SDK_PATH -G Ninja
          cmake --build build/pico-default

      - name: Build Pico firmware (all effects ON)
        env:
          PICO_SDK_PATH: /tmp/pico-sdk
        run: |
          cmake -S src/platform/pico -B build/pico-all-fx \
            -DPICO_SDK_PATH=$PICO_SDK_PATH -G Ninja \
            -DPOLYSYNTH_DEPLOY_CHORUS=ON \
            -DPOLYSYNTH_DEPLOY_DELAY=ON \
            -DPOLYSYNTH_DEPLOY_LIMITER=ON
          cmake --build build/pico-all-fx
```

### Acceptance Criteria

- [ ] CI tests both all-OFF and all-ON configurations
- [ ] Both configurations compile without errors

---

## Definition of Done

- [ ] Effects porting analysis documented (Chorus, Delay, Limiter)
- [ ] Deploy flag system implemented in CMake and Engine.h
- [ ] All 8 flag combinations compile for ARM (verify at least default and all-ON)
- [ ] PicoSoftClipper implemented with 7 passing unit tests
- [ ] PicoSoftClipper replaces ad-hoc `tanhf()` in Pico audio callback
- [ ] Desktop build and tests unaffected (all flags ON = transparent)
- [ ] CI tests both default and all-ON Pico configurations
- [ ] Deployment matrix table completed with actual memory measurements

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Create | `/src/platform/pico/pico_effects.h` | PicoSoftClipper |
| Create | `/tests/unit/Test_PicoSoftClipper.cpp` | PicoSoftClipper unit tests |
| Modify | `/src/core/Engine.h` | Deploy guards for effects |
| Modify | `/src/platform/pico/CMakeLists.txt` | Deploy flag CMake options |
| Modify | `/tests/CMakeLists.txt` | Add deploy flags ON for desktop |
| Modify | `/src/platform/pico/main.cpp` | Use PicoSoftClipper |
| Modify | `/.github/workflows/ci.yml` | Test deploy flag combinations |

---

## Review Checklist for Reviewers

- [ ] `#if defined(POLYSYNTH_DEPLOY_*)` guards are consistent (not `#ifdef`)
- [ ] Desktop build defines ALL three deploy flags → existing tests unaffected
- [ ] Guard placement: wraps member variables, Init(), Process(), UpdateState(), setters
- [ ] PicoSoftClipper has no Pico SDK dependency (usable from desktop tests)
- [ ] PicoSoftClipper output is bounded to ±1.0 for all inputs
- [ ] Effects analysis includes actual memory measurements (not just estimates)
- [ ] No behavioral change to desktop builds — guards are transparent when all flags ON
- [ ] CI tests compile both flag-OFF and flag-ON configurations
