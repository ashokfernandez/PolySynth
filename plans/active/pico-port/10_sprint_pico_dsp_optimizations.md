# Sprint Pico-10: DSP Optimizations

**Depends on:** Sprint Pico-9 (FreeRTOS Migration)
**Approach:** Lookup tables, compile-time filter selection, and CPU budget expansion.
**Estimated effort:** 2-3 days

> **Why this sprint exists:** After performance foundations (Sprint 7) and architecture
> cleanup (Sprints 8-9), the DSP hot path still has optimization opportunities. The
> ladder filter's 6 tanh calls per sample dominate the per-voice CPU budget, and each
> voice carries 3 filter types when only one is used. This sprint addresses these with
> LUTs and compile-time selection to maximize polyphony headroom.

---

## Goal

Reduce per-voice CPU cost enough to comfortably run 4 voices with headroom for effects
(chorus, delay) in future sprints. Target: <20% total CPU at 4 voices.

---

## Task 10.1: Tanh Lookup Table

### Problem

The ladder filter calls `Math::Tanh()` 6 times per sample per voice. The current Padé
approximant (`x * (27 + x²) / (27 + 9*x²)`) contains a division (~14 cycles on FPv5).
At 4 voices × 6 calls × 48000 samples/s = 1.15M tanh evaluations per second.

### Fix

Create a 256-entry tanh LUT with linear interpolation in `libs/SEA_DSP/include/sea_dsp/sea_lut.h`:

```cpp
namespace sea {

class TanhLUT {
public:
    static constexpr int kSize = 256;
    static constexpr float kRange = 4.0f;  // covers [-4, 4]
    static constexpr float kScale = kSize / (2.0f * kRange);

    static void Init();  // Pre-compute table (call once at startup)

    static SEA_INLINE float Process(float x) {
        // Clamp to table range
        if (x >= kRange) return 1.0f;
        if (x <= -kRange) return -1.0f;

        // Linear interpolation
        float idx = (x + kRange) * kScale;
        int i = static_cast<int>(idx);
        float frac = idx - i;
        return mTable[i] + frac * (mTable[i + 1] - mTable[i]);
    }

private:
    static float mTable[kSize + 1];
};

}
```

**Cost:** 1028 bytes of SRAM (257 floats × 4 bytes). Eliminates division entirely —
~3-4 cycles per lookup vs ~14+ for Padé with division.

### Integration

Guard with `#ifdef SEA_USE_TANH_LUT`:
- `sea_math.h`: `Math::Tanh()` uses `TanhLUT::Process()` when `SEA_USE_TANH_LUT` defined
- `CMakeLists.txt` (pico): add `-DSEA_USE_TANH_LUT`
- Desktop builds unaffected (no LUT, full precision)

### Validation

- Unit test: LUT output matches `std::tanh()` within 0.5% for [-4, 4]
- `just test-embedded` passes (add LUT accuracy test)
- CPU% drops measurably (tanh is ~30% of ladder filter cost)

---

## Task 10.2: Sine Lookup Table

### Problem

`Math::Sin()` uses a 7th-order Taylor polynomial with range reduction and branching.
Called once per sample per sine oscillator.

### Fix

Add a 1024-entry sine LUT (quarter-wave symmetry, linear interpolation):

```cpp
class SineLUT {
public:
    static constexpr int kSize = 1024;  // quarter-wave entries

    static void Init();

    // Input: phase in [0, 1) (normalized, not radians)
    static SEA_INLINE float Process(float phase);

private:
    static float mTable[kSize + 1];
};
```

**Cost:** 4100 bytes (1025 floats × 4 bytes).

Quarter-wave symmetry means we store only [0, π/2] and derive the rest, keeping the
table small while maintaining good accuracy.

### Integration

- `sea_oscillator.h`: Sine waveform case uses `SineLUT::Process(mPhase)` directly
  (phase is already normalized to [0, 1))
- Guarded by `#ifdef SEA_USE_SINE_LUT`
- Desktop builds use full Taylor/std::sin

### Validation

- Unit test: LUT output matches `std::sin()` within 0.1% across full circle
- `just test-embedded` passes
- Sine oscillator sounds clean (no audible artifacts from interpolation)

---

## Task 10.2b: Verify Sin() Accuracy Comment (PR #65 Review)

### Problem

The `Math::Sin()` comment in `sea_math.h` says "~0.016%" max error, but the 7th-order
Taylor series on [-π/2, π/2] should have max error closer to ~0.00016 ((π/2)^7/5040).
The approximation itself is fine — only the comment may be wrong.

### Fix

Add a sweep test to `test-embedded` that compares `Math::Sin()` against `std::sin()`
for 10,000 points across [0, 2π] and reports actual max error. Update the accuracy
comment to match measured results.

### Validation

- Sweep test passes and reports actual max relative error
- Comment in `sea_math.h` updated to match measured value

---

## Task 10.3: MIDI Note-to-Frequency Table

### Problem

MIDI note-to-frequency conversion uses `440 * pow(2, (note-69)/12.0)` which involves
`Pow()` (a `std::pow` call — expensive on Cortex-M33, no hardware support).

### Fix

Static `constexpr` table of 128 float frequencies:

```cpp
// Pre-computed at compile time — zero runtime cost, zero init overhead
static constexpr float kMidiFreqTable[128] = {
    8.1758f,    // MIDI 0  (C-1)
    8.6620f,    // MIDI 1
    // ... all 128 notes
    12543.85f,  // MIDI 127 (G9)
};
```

**Cost:** 512 bytes (128 × 4 bytes). `constexpr` so it lives in flash, not SRAM.
With `__time_critical_func` on the audio callback, XIP cache will keep it hot.

### Integration

- Add to `Voice.h` or a new `midi_table.h`
- Replace `pow(2, ...)` calls in `Voice::NoteOn()`
- Works on both desktop and embedded (no conditional compilation needed)

### Validation

- Unit test: table values match `440 * pow(2, (n-69)/12.0)` within 0.01%
- `just test` and `just test-embedded` pass

---

## Task 10.4: Compile-Time Filter Selection

### Problem

Each Voice instantiates all 3 filter types (`BiquadFilter`, `LadderFilter`,
`CascadeFilter`) but only uses one at runtime. This wastes ~400 bytes of cache
per voice — 1.6KB total for 4 voices, polluting the L1 data cache on Core 1.

### Fix

Add `POLYSYNTH_FILTER_MODEL` compile-time flag:

```cmake
# CMakeLists.txt (pico)
target_compile_definitions(polysynth_pico PRIVATE
    POLYSYNTH_FILTER_MODEL=1  # 0=Biquad, 1=Ladder, 2=Cascade
)
```

In `Voice.h`, conditionally compile only the selected filter:

```cpp
#if defined(POLYSYNTH_FILTER_MODEL)
  #if POLYSYNTH_FILTER_MODEL == 0
    using FilterType = sea::BiquadFilter<sample_t>;
  #elif POLYSYNTH_FILTER_MODEL == 1
    using FilterType = sea::LadderFilter<sample_t>;
  #elif POLYSYNTH_FILTER_MODEL == 2
    using FilterType = sea::CascadeFilter<sample_t>;
  #endif
    FilterType mFilter;
#else
    // Desktop: all filters available, runtime selection
    sea::BiquadFilter<sample_t> mBiquadFilter;
    sea::LadderFilter<sample_t> mLadderFilter;
    sea::CascadeFilter<sample_t> mCascadeFilter;
#endif
```

### Why per-voice filtering (not post-mix)

All classic polysynths (Minimoog, Prophet-5, Jupiter-8, OB-Xa) use per-voice filtering.
This is **polyphonic** (each voice has its own filter envelope and cutoff), not
**paraphonic** (shared filter). Post-mix filtering would fundamentally change the
instrument's character and eliminate per-note filter dynamics. The waste isn't the
per-voice approach — it's carrying unused filter types.

### Validation

- `just test` passes (desktop still has all filters)
- `just test-embedded` passes (embedded uses only selected filter)
- `just pico-build` succeeds with `POLYSYNTH_FILTER_MODEL=1`
- Voice struct size decreases by ~400 bytes on embedded
- CPU% may improve slightly (better cache utilization)

---

## Task 10.5: CPU Budget Analysis and Documentation

### Problem

After all optimizations, we need to document the actual per-voice CPU budget to
inform future feature decisions (effects, higher voice counts).

### Fix

Measure and document in `07_pico_dsp_architecture.md`:

```
Per-voice budget at 48kHz on RP2350 @ 150MHz:
  Oscillator × 2:       ~X cycles/sample
  ADSR envelope:         ~X cycles/sample
  Filter (Ladder):       ~X cycles/sample
  LFO:                   ~X cycles/sample
  Pan + mix:             ~X cycles/sample
  ─────────────────────────────────────
  Total per voice:       ~X cycles/sample
  Budget per sample:     3125 cycles (150MHz / 48kHz)
  4-voice headroom:      X%
```

Use the existing `STATUS` command's CPU% reporting with before/after measurements
for each optimization.

### Validation

- Architecture doc updated with measured values
- CPU budget table shows >40% headroom at 4 voices (leaving room for effects)

---

## Task 10.6: LUT Initialization Ordering

### Problem

`TanhLUT::Init()` and `SineLUT::Init()` must be called before any audio processing
starts. The ordering relative to `PicoSynthApp::Init()` matters.

### Fix

Call LUT init functions in `main()` before `PicoSynthApp::Init()` and before
launching Core 1:

```cpp
int main() {
    // ... hardware init ...
    sea::TanhLUT::Init();  // Pre-compute tables before any audio
    sea::SineLUT::Init();
    app.Init(pico_audio::kSampleRate);
    // ... launch Core 1 ...
}
```

---

## Definition of Done

- [ ] Tanh LUT: 256 entries, linear interpolation, <0.5% error, guarded by `SEA_USE_TANH_LUT`
- [ ] Sine LUT: 1024 entries (quarter-wave), linear interpolation, <0.1% error, guarded by `SEA_USE_SINE_LUT`
- [ ] `Math::Sin()` accuracy comment verified via sweep test (PR #65 review fix)
- [ ] MIDI note-to-frequency table: 128 entries, constexpr
- [ ] Compile-time filter selection via `POLYSYNTH_FILTER_MODEL`
- [ ] LUT init called before audio starts (ordering documented)
- [ ] CPU budget measured and documented
- [ ] `just test` passes (desktop unaffected — no LUT flags)
- [ ] `just test-embedded` passes
- [ ] `just pico-build` succeeds
- [ ] Audio quality maintained (play sustained chord with high resonance — listen for metallic/buzzy artifacts)
- [ ] Memory budget documented (LUT SRAM cost vs. total available)
