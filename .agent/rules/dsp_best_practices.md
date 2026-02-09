# DSP & Audio Thread Best Practices

Strict adherence to these rules is required to prevent audio glitches and maintain high performance.

## 1. Audio Thread Constraints
The following are **STRICTLY FORBIDDEN** in the audio thread (`Process()` callbacks):
- `malloc`, `free`, `new`, `delete` (No runtime allocations).
- `printf`, `std::cout`, or any logging to console.
- File I/O (No reading or writing to disk).
- Mutexes or any blocking locks.
- Heavy system calls.

## 2. Numerical Precision & Types
- Use `sample_t` for all audio signal calculations.
- Be careful with `float` vs `double` conversions; use explicit casts if necessary.
- Avoid large arrays on the stack; pre-allocate them as class members.

## 3. Parameter Handling
- **Parameter Smoothing**: All control parameters (Cutoff, Volume) SHOULD be smoothed to prevent "zipper noise" or clicks.
- **Sample-Accurate Processing**: Internal modulation (Envelopes, LFOs) must be computed per-sample, not per-block, unless performance critical.

## 4. Stability
- Ensure filters remain stable by clamping coefficients and resonance levels.
- Handle denormals where necessary (though many modern CPUs handle them efficiently, it is good to be aware).
- Clamp inputs to oscillators and envelopes to prevent `NaN` or `Inf`.
