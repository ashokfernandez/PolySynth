# SEA_DSP Test-Driven Development Spec (QA Refactor)

## Scope
This spec defines a behavior-first testing strategy for `lib/SEA_DSP` and captures immediate improvements made in this refactor.

## Critical Assessment of Existing Suite (Pre-refactor)
- Several tests asserted broad ranges instead of behavioral invariants, which weakens bug-detection sensitivity.
- Oscillator frequency/wrap tests had platform-dependent branching, reducing determinism across build targets.
- ADSR lifecycle coverage was incomplete for zero-time stage transitions and clamp behavior.
- Not all public headers had direct self-contained smoke tests.

## TDD Test Charter
For each module, tests should be added in the order below:

1. **Contract tests (red first)**
   - Public API pre/post conditions.
   - Reset/Init invariants.
   - Range and clamp guarantees.
2. **State-transition tests**
   - Explicit stage-machine transitions (e.g., ADSR A→D→S→R).
   - Transition timing in samples for deterministic engines.
3. **Numerical property tests**
   - Monotonicity, bounded outputs, no NaN/Inf.
   - Frequency- and phase-related invariants.
4. **Edge-case tests**
   - Zero/negative parameters.
   - Extreme resonance/Q and frequency near Nyquist.
5. **Regression tests**
   - Every fixed bug gets a focused reproducible test.

## Refactor Changes Implemented
- **ADSR:** Added full lifecycle transition checks, zero-time stage checks, and clamping behavior tests.
- **Oscillator:** Replaced platform-specific wrap expectation with deterministic bounded/ramp/wrap property checks.
- **Headers:** Added self-contained compile-and-smoke tests for ADSR, Oscillator, LFO, and SVF headers.
- **Filter aggregate test cleanup:** Removed unused transitive includes from `Test_Filters.cpp`.

## Remaining Recommended Backlog
- Add explicit NaN/Inf rejection tests for all filter/effect process paths.
- Add golden-vector tests for waveform phase continuity and filter impulse responses.
- Add property/fuzz-style randomized parameter tests with fixed seed for reproducibility.
- Add CI split: fast deterministic unit tests vs. long-running benchmarks.
