# Sprint Pico-13: Effects Deployment (Chorus + Delay)

**Depends on:** Sprint Pico-10 (DSP Optimizations — CPU headroom), Sprint Pico-12 (Presets — effect params in presets)
**Approach:** Re-enable deploy flags with static buffer allocation and constrained parameters.
**Estimated effort:** 3-4 days

> **Why this sprint exists:** The effects already compile for ARM (Sprint 5 verified
> this) but are stripped via deploy flags. Chorus and delay add sonic richness. This
> sprint re-enables them with static buffers (no heap) and embedded-appropriate
> parameter limits.

---

## Goal

Enable chorus and delay effects on the Pico with static memory allocation and
constrained parameters. Target: <70% total CPU with 4 voices + both effects.

---

## Task 13.1: Static Delay Line Buffers

### Problem

`DelayLine` in `libs/SEA_DSP/include/sea_dsp/sea_delay_line.h` uses `std::vector` for
its buffer — heap allocation. Embedded needs static buffers.

### Memory Budget

| Effect | Buffer Size | SRAM Cost |
|---|---|---|
| Delay (100ms stereo) | 2 × 4,800 floats | 38.4 KB |
| Chorus (50ms stereo) | 2 × 2,400 floats | 19.2 KB |
| **Total** | | **~57.6 KB** |

Current SRAM usage ~107KB. With effects: ~165KB of 520KB available. Comfortable.

### Fix

`DelayLine` already supports external buffers via `Init(T* external_buffer, size_t size)`.
Declare static buffers in `pico_synth_app.cpp` or a new `pico_effects.h`:

```cpp
#if POLYSYNTH_DEPLOY_DELAY
static float s_delayBufL[kPicoDelayMaxSamples];  // 4800 = 100ms @ 48kHz
static float s_delayBufR[kPicoDelayMaxSamples];
#endif
#if POLYSYNTH_DEPLOY_CHORUS
static float s_chorusBufL[kPicoChorusMaxSamples]; // 2400 = 50ms @ 48kHz
static float s_chorusBufR[kPicoChorusMaxSamples];
#endif
```

Add `InitExternal()` methods to `VintageChorus` and `VintageDelay` that pass external
buffers to their internal `DelayLine` objects. Desktop builds continue using the vector
path.

### Validation

- Effects initialize without heap allocation
- `just test` passes (desktop uses vector path unchanged)

---

## Task 13.2: Enable Deploy Flags

### Problem

Effects are currently stripped from the Pico binary.

### Fix

In `src/platform/pico/CMakeLists.txt`:

```cmake
POLYSYNTH_DEPLOY_CHORUS=1
POLYSYNTH_DEPLOY_DELAY=1
POLYSYNTH_DEPLOY_LIMITER=0   # Keep limiter off (lookahead buffer adds complexity)
```

The `Engine.h` deploy guards already handle conditional compilation.

### Validation

- `just pico-build` succeeds
- Binary size increase documented

---

## Task 13.3: Constrained Effect Parameters

### Problem

Desktop delay max is 2000ms (`kMaxDelayMs`) — that's 768KB of SRAM, exceeding total
available. Need Pico-specific limits.

### Fix

Define Pico-specific limits:

| Parameter | Desktop | Pico |
|---|---|---|
| Delay max time | 2000ms | 100ms |
| Delay feedback | 0-95% | 0-85% (prevents runaway in float precision) |
| Chorus depth | 0-5ms | 0-3ms (saves buffer space) |

Add `POLYSYNTH_MAX_DELAY_MS=100` compile definition. Clamp effect parameters in
`PicoSynthApp::SetParam()` before passing to engine.

### Validation

- `SET fxDelayTime 0.5` is clamped to 0.1 max
- Serial feedback confirms clamping

---

## Task 13.4: Effect Parameter Serial Commands

### Problem

Effect parameters aren't in the serial dispatch table.

### Fix

Extend `set_param`/`get_param` in `pico_synth_app.cpp`:
- `fxChorusRate`, `fxChorusDepth`, `fxChorusMix`
- `fxDelayTime`, `fxDelayFeedback`, `fxDelayMix`

These fields already exist in `SynthState` — just add the strcmp entries.

### Validation

- `SET fxChorusMix 0.5` enables chorus audibly
- `SET fxDelayMix 0.3` enables delay audibly

---

## Task 13.5: CPU Impact Measurement

### Problem

Need to verify effects fit within CPU budget.

### Fix

Measure CPU% via `STATUS` command with:
- 4 voices, no effects (baseline from Sprint 10)
- 4 voices + chorus only
- 4 voices + delay only
- 4 voices + chorus + delay

Document in `07_pico_dsp_architecture.md`. Target: <70% CPU with all effects.

### Validation

- CPU budget table updated with measured values
- No underruns during 4-voice + both effects sustained chord

---

## Task 13.6: Output Gain Adjustment

### Problem

`kOutputGain = 4.0f` was tuned for dry signal. Effects add volume (especially delay
feedback), risking clipping.

### Fix

- Re-tune `kOutputGain` with effects enabled (likely reduce to 2.0-3.0f)
- Or make it configurable: `SET outputGain <value>`
- Verify peak level stays <1.0 as reported by `STATUS`

### Validation

- No clipping with 4 voices + both effects at moderate settings

---

## Risks

1. **SRAM pressure** — 57KB for effect buffers. Pre-effects usage ~107KB. Total ~165KB
   of 520KB. Well within budget.
2. **CPU budget** — Chorus and delay each add ~20-30 cycles/sample (~2-3% CPU each).
   Should be fine after Sprint 10 optimizations.
3. **`std::vector` in DelayLine** — Desktop code changes could break external buffer
   API. Add a CI test exercising the external buffer path.

---

## Definition of Done

- [ ] Chorus enabled with static 50ms stereo buffers (no heap)
- [ ] Delay enabled with static 100ms stereo buffers (no heap)
- [ ] Effect parameters controllable via serial and MIDI CC
- [ ] Pico-specific parameter limits enforced (100ms delay max, 85% feedback max)
- [ ] `kOutputGain` tuned for effects-on operation
- [ ] CPU budget documented with effects
- [ ] `just test` and `just test-embedded` pass
- [ ] `just pico-build` succeeds
- [ ] Audio: chorus sounds smooth, delay feeds back cleanly without runaway
- [ ] Binary size and SRAM usage documented
