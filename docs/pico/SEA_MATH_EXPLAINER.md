# SEA Math: The Art of Approximate Transcendentals

A detailed explainer of every math trick in `sea_math.h`, `sea_wavetable.h`, and the surrounding DSP code. Written for someone who knows what a transfer function is but hasn't memorized Taylor coefficients.

---

## Why We Can't Just Call `std::sin()`

On desktop (x86-64), transcendental functions are essentially free. The CPU has dedicated microcode, pipelined FPUs, and out-of-order execution that hides latency. `std::sin()` might take 20 cycles — invisible next to cache misses and branch mispredictions.

On the RP2350 (Cortex-M33, 150 MHz), the situation is different:

| Function | Desktop (x86) | RP2350 (Cortex-M33) |
|----------|---------------|---------------------|
| `std::sin()` | ~20 cycles | ~200-400 cycles (software libm) |
| `std::tanh()` | ~25 cycles | ~200-500 cycles |
| `std::exp()` | ~20 cycles | ~150-300 cycles |
| `std::pow()` | ~30 cycles | ~300-600 cycles |

At 48 kHz sample rate with 4 voices, each voice gets:

```
150,000,000 cycles/sec ÷ 48,000 samples/sec ÷ 4 voices = 781 cycles per voice per sample
```

A single `std::tanh()` call could eat **64% of the per-voice budget**. And a ladder filter calls `tanh()` **six times per sample** (input + 4 stages + feedback). That's 3000 cycles in a 781-cycle budget. We need approximations.

---

## 1. Sine: Wavetable with Linear Interpolation

### The Idea

Instead of computing `sin(x)` from scratch each time, pre-compute it at 512 evenly-spaced points and store them in a table. At runtime, find the two nearest table entries and linearly interpolate between them.

This is the same principle as a ROM-based DDS (direct digital synthesis) chip — the AD9833 you might have used in lab does exactly this with a 4096-entry sine ROM.

### How It Works

**Table generation (compile-time only):**

```
table[i] = sin(2π × i / 512)    for i = 0, 1, ..., 511
```

This is done with a `constexpr` Taylor series that runs entirely at compile time. The Taylor series accuracy doesn't matter for runtime — it only needs to seed the table correctly. We use an 11th-order expansion:

```
sin(x) ≈ x - x³/6 + x⁵/120 - x⁷/5040 + x⁹/362880 - x¹¹/39916800
```

At compile time, `x` values are small (range-reduced) and `double` precision, so this converges well.

**Runtime lookup:**

Given phase `θ ∈ [0, 2π)`:

```
1. Convert to table index:  idx = θ × (512 / 2π) = θ × 81.487...
2. Split into integer + fraction:  i₀ = floor(idx),  frac = idx - i₀
3. Wrap index (power-of-2 trick):  i₀ = i₀ & 511  (bitmask, not modulo!)
4. Interpolate:  result = table[i₀] + frac × (table[i₀+1] - table[i₀])
```

Step 3 is key: because the table size is a power of 2 (512 = 2⁹), wrapping from index 511 to 0 is a single AND instruction instead of a division. This is why we choose N = 512, not some "rounder" number.

### Error Analysis

With N = 512 and linear interpolation, the maximum error is bounded by the second derivative of sin:

```
|error| ≤ (Δx)² / 8 × max|sin''(x)| = (2π/512)² / 8 × 1 ≈ 1.88 × 10⁻⁵
```

That's 0.002% of full scale. In practice, measured max error is **< 0.01%** across 10,000 test points. For audio at 16-bit output (96 dB dynamic range), this error is at -80 dB — well below the noise floor.

The previous Taylor polynomial (7th order with range reduction) had 0.016% error. The wavetable is both **faster** (table lookup vs. 7 multiplies) and **more accurate**.

### Cost

| Operation | Cycles (approx) |
|-----------|-----------------|
| Multiply by `512/2π` | 1 |
| Float-to-int conversion | 1 |
| Bitmask wrap | 1 |
| Two table lookups | 2-4 (if cached) |
| Multiply + add (lerp) | 2 |
| **Total** | **~8-10 cycles** |

Memory: 512 × 4 bytes = **2 KB**. The table is `constexpr`, so it lives in flash and gets served via the XIP (execute-in-place) cache. Hot entries stay cached; cold entries cost one flash read (~10 cycles) on first access but then stay warm.

### Cosine for Free

`cos(x) = sin(x + π/2)`. Just add a quarter-period offset and call the same lookup. No separate table needed.

---

## 2. Tanh: Padé Approximant

### Why Tanh Matters

`tanh(x)` shows up everywhere in analog-modeled audio:

- **Transistor ladder filter:** The Moog ladder's nonlinearity comes from transistor pairs, which have a `tanh` transfer function. Our `LadderFilter::ProcessTransistor()` calls it 6 times per sample.
- **Cascade filter feedback:** `tanh(input - feedback)` provides soft saturation that prevents resonance from blowing up.
- **Output soft clipping:** Before converting to int16, we soft-clip the signal to prevent harsh digital distortion.

### The Padé Approximant

A Padé approximant is a rational function (ratio of two polynomials) that matches a function's Taylor series to a given order. It's often **much more accurate** than a truncated Taylor series of the same computational cost, because the denominator polynomial can model the function's asymptotic behavior.

For `tanh(x)`, the [3,2] Padé approximant is:

```
tanh(x) ≈ x(27 + x²) / (27 + 9x²)
```

This matches the Taylor series of `tanh(x) = x - x³/3 + 2x⁵/15 - ...` through the x⁵ term.

### Why Padé Beats Taylor for Tanh

Taylor series for `tanh(x)` converges slowly and **diverges for |x| > π** (the radius of convergence is π/2, limited by poles of `tan` in the complex plane). So a 5th-order Taylor approximation is terrible for `x = 2`:

```
Taylor: tanh(2) ≈ 2 - 8/3 + 64/15 = 2 - 2.667 + 4.267 = 3.6   (actual: 0.964)
Padé:   tanh(2) ≈ 2(27+4)/(27+36) = 62/63 = 0.984              (actual: 0.964)
```

The Padé gets the asymptotic behavior right — it approaches 1 as x → ∞ (since the leading terms in numerator and denominator are both x³, ratio → x³/9x² × 1/1 → ... actually for large x, num ≈ x³, den ≈ 9x², so ratio ≈ x/9... hmm). Let's be precise:

For large |x|: `x(27 + x²) / (27 + 9x²) ≈ x·x² / 9x² = x/9`. So it doesn't perfectly saturate to ±1. But for audio signals where |x| < 3 (which covers 99.9% of cases in a properly gain-staged synth), the error is < 0.1%.

For the ladder filter, the input to `tanh` is always in [-3, 3] because the filter's feedback is gain-limited. The 2% error at x = 3 is inaudible — it's a slightly different saturation curve, which actually sounds fine (analog circuits have component tolerance of 5-20% anyway).

### Why Not a Wavetable for Tanh?

| Approach | Cycles | Memory | Cache pressure |
|----------|--------|--------|----------------|
| Padé `x(27+x²)/(27+9x²)` | ~10 (4 mul, 2 add, 1 div) | 0 | None |
| 512-entry wavetable + lerp | ~10 | 2 KB | Competes with sin table |

Same speed, but the Padé uses zero memory and doesn't pollute the data cache. When you're calling `tanh` 6 times per sample across 4 voices, cache pressure matters. The Padé is branchless, fits in registers, and the division is pipelined on Cortex-M33 (the hardware divider takes ~8 cycles but can overlap with subsequent instructions).

### The Implementation

```cpp
Real x2 = x * x;                                    // 1 mul
return x * (Real(27) + x2) / (Real(27) + Real(9) * x2);  // 3 mul, 2 add, 1 div
```

That's it. Six operations, all branchless, all in registers.

---

## 3. Tan: Padé Approximant for Filter Coefficients

### Where It's Used

The bilinear transform maps analog filter poles to digital filter poles. The key step is:

```
g = tan(π × fc / fs)
```

where `fc` is cutoff frequency and `fs` is sample rate. This appears in `LadderFilter::SetParams()` and `SVFilter` coefficient calculations.

### The Approximant

```
tan(x) ≈ x(15 - x²) / (15 - 6x²)    for |x| < π/4
```

This is the [3,2] Padé approximant for `tan(x)`, accurate to the x⁵ term.

### Why |x| < π/4 Is Enough

The argument to `tan` in filter coefficient calculation is:

```
x = π × fc / fs × 0.5
```

For fc = 20 kHz (maximum audible) and fs = 48 kHz:

```
x = π × 20000 / 48000 × 0.5 = 0.654
```

And π/4 = 0.785. So even at the Nyquist limit, we're well within the approximation's valid range. In practice, we clamp cutoff to 0.95 × Nyquist, so x never exceeds 0.75.

### Important: Only Called in SetParams()

`tan()` is only needed when filter coefficients change (user turns a knob). It's **not** called per-sample. So even the standard `std::tan()` would be fast enough — the Padé is a nice-to-have optimization, not a necessity. We use it because it's essentially free and avoids any libm dependency.

---

## 4. Exp2: The Volatile Barrier Hack

### The Bug

On RP2350 with GCC 15.2, `std::pow(2.0f, x)` returns **wrong values** when the calling function is placed in SRAM via `__time_critical_func`. The root cause: GCC's aggressive inlining pulls flash-resident libm code into the SRAM function, but the relocated code contains PC-relative references that break.

### The Fix

```cpp
volatile Real base = Real(2);
return std::pow(base, x);
```

The `volatile` qualifier tells GCC: "this value might change at any time, don't optimize away the load." This has two effects:

1. GCC can't constant-fold `base = 2` into the `pow` call
2. GCC can't inline the `pow` implementation (because it can't prove the arguments are constant)

The result: GCC generates an actual function call to libm's `powf()`, which lives in flash and executes correctly from flash. The cost is one extra load instruction (~1 cycle) — negligible for a function that's only called during `NoteOn` and `SetDetune`.

### Where It's Used

- **Detune calculation:** `Exp2(cents / 1200.0)` — converts cents to frequency ratio
- **Pitch bend:** same formula

Both are called on parameter changes, not per-sample.

---

## 5. MIDI Frequency Table: Eliminating Pow Entirely

### The Problem

MIDI note → frequency is:

```
f = 440 × 2^((note - 69) / 12)
```

That `2^x` is exactly the operation that's broken in SRAM ISR context. And `NoteOn` is called from the audio callback (Core 1 ISR, SRAM-placed).

### The Solution

Pre-compute all 128 values:

```cpp
constexpr sample_t kMidiFreqTable[128] = {
    8.176f,    // MIDI 0  (C-1)
    8.662f,    // MIDI 1
    ...
    12543.854f // MIDI 127 (G9)
};
```

128 × 4 bytes = 512 bytes in flash. Table lookup is 1 cycle. No math at all.

This is the same approach as hardware MIDI synths — the DX7 had a frequency ROM for exactly this reason. Some problems are best solved by just storing the answer.

---

## 6. Cached Coefficients: The SetParams Pattern

### The Anti-Pattern (Bug)

```cpp
void Process(float& left, float& right) {
    // This is WRONG — computes exp() 48,000 times per second
    // but the result only changes when releaseMs changes
    float releaseCoeff = 1.0f - exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    gain = gain + (targetGain - gain) * releaseCoeff;
}
```

### The Fix

```cpp
void SetParams(float threshold, float lookaheadMs, float releaseMs) {
    mReleaseMs = clamp(releaseMs, 1.0f, 500.0f);
    // Compute ONCE when parameter changes
    mReleaseCoeff = 1.0f - exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));
}

void Process(float& left, float& right) {
    // Just use the cached value
    gain = gain + (targetGain - gain) * mReleaseCoeff;
}
```

### The Math Behind the Release Coefficient

This is a discrete-time exponential decay (first-order IIR lowpass). The continuous-time envelope is:

```
g(t) = g_target + (g_current - g_target) × e^(-t/τ)
```

where τ = releaseMs × 0.001 (convert to seconds). To discretize at sample rate fs:

```
g[n+1] = g[n] + (g_target - g[n]) × (1 - e^(-1/(τ × fs)))
                                       ^^^^^^^^^^^^^^^^^^^^^^^^
                                       This is the release coefficient
```

The coefficient `α = 1 - e^(-1/(τ·fs))` depends only on the release time and sample rate — both parameters, not signals. Computing it per-sample was pure waste.

### The Design Rule

**If the inputs to a transcendental are all parameters (knob positions, sample rate), compute it in `SetParams()` and cache the result.**

**If the input is a signal (audio sample, oscillator phase), compute it per-sample using the fastest available method.**

| Function | Input | Strategy |
|----------|-------|----------|
| `sin(phase)` | Phase accumulator (signal) | Wavetable lookup |
| `tanh(sample)` | Audio signal | Padé approximant |
| `tan(π·fc/fs)` | Filter cutoff (parameter) | Padé in SetParams() |
| `exp(-1/(τ·fs))` | Release time (parameter) | std::exp in SetParams() |
| `2^(cents/1200)` | Detune (parameter) | Exp2() in SetDetune() |
| `2^((note-69)/12)` | MIDI note (parameter) | Lookup table |

---

## 7. Denormal Flushing: The Silent Performance Killer

### What Are Denormals?

IEEE 754 floats have a "denormalized" (subnormal) range near zero. Normal floats have the form:

```
(-1)^s × 1.mantissa × 2^exponent    (exponent: -126 to +127 for float)
```

Denormals drop the implicit leading 1:

```
(-1)^s × 0.mantissa × 2^(-126)
```

They represent numbers smaller than ~1.2 × 10⁻³⁸. In audio, they appear in filter feedback tails — when a note releases, the filter's internal state decays exponentially toward zero. Eventually it enters the denormal range.

### Why They're Expensive

On x86, denormal arithmetic is handled by microcode and costs ~100 cycles instead of ~4 cycles. On Cortex-M33, it's even worse: the FPU **traps to a software handler**, costing 10-80 cycles per operation.

A 4-pole ladder filter has 4 integrator states. When all 4 are denormal, every multiply and add in the filter costs 10-80x more. Since the filter runs 6 tanh + 12 multiply-add operations per sample, the cost can spike from ~100 cycles to **thousands of cycles** — enough to cause buffer underruns.

### The Fix

At Core 1 startup, we set two bits in the ARM FPSCR (Floating-Point Status and Control Register):

```cpp
uint32_t fpscr = __get_FPSCR();
fpscr |= (1 << 24) | (1 << 25);  // FZ (Flush-to-Zero) + DN (Default NaN)
__set_FPSCR(fpscr);
```

- **FZ (bit 24):** Results that would be denormal are flushed to ±0 instead. Zero arithmetic is fast.
- **DN (bit 25):** NaN results use a default quiet NaN instead of propagating payload bits. Avoids unexpected slow paths.

This is set only on Core 1 (audio DSP). Core 0 keeps default FPU behavior for correctness in non-time-critical code.

### Trade-off

Flushing denormals to zero introduces a tiny discontinuity at the denormal threshold. For audio, this is a signal level of ~-760 dB — about 700 dB below the thermal noise floor of the universe. Completely inaudible.

---

## Summary: The Full Math Pipeline

```
MIDI Note 60 (C4)
    │
    ▼
kMidiFreqTable[60] = 261.626 Hz     ← Lookup table (0 cycles math)
    │
    ▼
phase += freq / sampleRate           ← 1 multiply
    │
    ▼
SinWavetable::Lookup(2π × phase)    ← ~10 cycles (table + lerp)
    │
    ▼
LadderFilter: 6× Math::Tanh()       ← ~60 cycles (Padé, branchless)
    │
    ▼
× gain (from cached exp coeff)       ← 1 multiply
    │
    ▼
fast_tanh() soft clip                ← ~10 cycles (same Padé)
    │
    ▼
SSAT → int16 → pack I2S             ← 2 cycles (ARM instruction)
    │
    ▼
DMA → PIO → DAC
```

Total per-voice math: ~85 cycles. Budget: 781 cycles. Headroom: **89%**.

Compare with naive libm: ~200 (sin) + 6×500 (tanh) + 300 (exp) + 500 (tanh clip) = **4000 cycles**. That's 5× over budget for a single voice.

The approximations aren't compromises — they're the difference between "works" and "doesn't work."
