# PolySynth Design Principles

These are the non-negotiable rules of the project. Every change — feature, refactor, or bugfix — must satisfy all of them. If a change conflicts with a principle, the principle wins.

---

## 1. Real-Time Safety in the Audio Path

**Rule:** Zero heap allocations, zero exceptions, and zero locks in any code called from `ProcessBlock`.

**Why:** Audio runs on a real-time thread with a hard deadline (typically 1-10ms per buffer). A single `new`, `malloc`, `std::vector::push_back`, mutex lock, or exception throw can cause an audible glitch. The operating system's memory allocator and exception unwinder are not real-time safe.

**What this means in practice:**

- All DSP objects (voices, filters, effects) are pre-allocated in the plugin constructor or `OnReset`.
- Use `std::array` (fixed size, stack-allocated), never `std::vector` in the audio path.
- No `try`/`catch`/`throw` in any file included by `Engine.h` or `VoiceManager.h`.
- No `std::string`, `std::map`, `printf`, `std::cout`, or file I/O in the audio path.
- No `std::function` or lambdas that capture by value (they can allocate).
- The `memset` in `ProcessBlock` is fine — it operates on pre-allocated output buffers.

**How to verify:** The audio path is the call tree rooted at `PolySynthPlugin::ProcessBlock`. If you're unsure whether a function allocates, don't call it from ProcessBlock.

---

## 2. SynthState is the Single Source of Truth

**Rule:** All synth parameter values live in `SynthState`. Default values are defined once, as inline member initializers. Nothing else is canonical.

**Why:** Before this rule was enforced, `Reset()` had a hand-written list of 36 assignments that diverged from the struct's inline defaults — wrong values, missing fields. Three places (member defaults, Reset, PresetManager) had to stay in sync manually. Now there's one source.

**How it works:**

```cpp
struct SynthState {
    double masterGain = 0.75;   // ← This IS the default. Period.
    // ...
    void Reset() { *this = SynthState{}; }  // ← Reuses the above.
};

static_assert(std::is_aggregate_v<SynthState>,
              "SynthState must remain an aggregate for Reset() to work");
```

**Constraints:**

- **Never add a user-defined constructor** to SynthState. The `static_assert(is_aggregate_v)` will catch this at compile time.
- **Never add virtual methods** to SynthState.
- **Always use inline member initializers** for defaults, never a constructor body.
- **Reset() must remain `*this = SynthState{}`** — never revert to manual field assignment.

---

## 3. Serialization Must Cover Every Field

**Rule:** `PresetManager::Serialize` and `PresetManager::Deserialize` must handle every field in `SynthState`. The round-trip test (`Test_SynthState.cpp`) enforces this.

**Why:** A missing field in serialization means presets silently lose data. The `filterModel` field was missing from PresetManager for months — the round-trip test caught it immediately when added.

**How the safety net works:**

1. `Test_SynthState.cpp` creates a SynthState with every field set to a distinct non-default value.
2. It serializes to JSON, deserializes back, and compares every field.
3. If you add a field to SynthState but forget to add it to Serialize/Deserialize, this test fails.

**When adding a new SynthState field, you must simultaneously:**
- Add the field with an inline default to `SynthState.h`
- Add `SERIALIZE(j, state, fieldName)` to `PresetManager::Serialize`
- Add `DESERIALIZE(j, outState, fieldName)` to `PresetManager::Deserialize`
- Add the field to the round-trip test in `Test_SynthState.cpp`

See [ADDING_A_PARAMETER.md](ADDING_A_PARAMETER.md) for the complete checklist.

---

## 4. Golden Master Discipline

**Rule:** Golden master audio comparisons use a strict 0.001 RMS tolerance. Never loosen it.

**Why:** The golden masters are the project's regression safety net for audio output. A tight tolerance means any change to the signal path — intentional or accidental — is immediately caught. This is by design.

**Workflow:**

- **Routine change (non-audio):** Run `python3 scripts/golden_master.py --verify`. All 19 files should pass with RMS diff = 0.000000.
- **Intentional audio change:** Run `python3 scripts/golden_master.py --generate` to create new reference files. Commit the new golden masters alongside the code change. The PR reviewer can then see exactly what changed in the audio output.
- **Unexpected failure:** Something in the signal path changed that you didn't intend. Investigate before regenerating.

**Never bypass golden master verification.** If your change doesn't affect audio, the verification takes 30 seconds and confirms it. If it does affect audio, you need to know.

---

## 5. Test-First for State Changes

**Rule:** Any change to SynthState, parameter mapping, or the DSP signal path must have a corresponding test before or alongside the code change.

**Levels of testing (use all that apply):**

| Level | What it catches | When to use |
|---|---|---|
| **Unit test** (Catch2) | Logic errors in a single class | New DSP feature, new parameter, bug fix |
| **Golden master** (WAV comparison) | Unintended audio regressions | Every merge |
| **Demo program** (WAV generator) | New audio features need audible verification | New synthesis feature |
| **Visual regression** (Playwright) | UI rendering changes | New or modified controls |

**The minimum for adding a SynthState field:**
1. Round-trip serialization test (already enforced by existing test)
2. Reset-equals-default test (already enforced by existing test)
3. A unit test or golden master demo that exercises the new field's DSP effect

---

## 6. Layer Boundaries Are Enforced by Includes

**Rule:** Dependency direction is strictly: SEA_DSP <- Core <- Platform <- iPlug2. Never reverse.

| Layer | May include | Must not include |
|---|---|---|
| `src/core/` | SEA_DSP headers, standard library | Anything from `src/platform/`, iPlug2 headers |
| `src/platform/desktop/` | Core headers, iPlug2 headers, UI headers (under `#if IPLUG_EDITOR`) | Nothing — it's the top layer |
| `libs/SEA_DSP/` | Standard library only | Anything from core or platform |
| `tests/unit/` | Core headers, Catch2, SEA_DSP | Platform headers, iPlug2 headers |

**Why this matters:** Core code compiles and runs in the native test build (plain CMake, no iPlug2). If core accidentally includes an iPlug2 header, tests break. This is the canary.

---

## 7. Conditional Compilation Must Be Minimal and Guarded

**Rule:** Use `#if IPLUG_DSP` and `#if IPLUG_EDITOR` only in the platform layer. Core code should have zero conditional compilation guards.

**Current guards and their purpose:**

| Guard | Where | Purpose |
|---|---|---|
| `#if IPLUG_DSP` | `PolySynth.h/cpp` | DSP-only members and methods (ProcessBlock, OnParamChange) |
| `#if IPLUG_EDITOR` | `PolySynth.h/cpp` | UI-only methods (OnLayout, BuildXxx) |
| `#if !defined(WEB_API)` | `PolySynth.cpp` | Desktop-only preset file I/O |
| `#error` | `PolySynth.h` | Catch misconfigured builds (neither flag set) |

**If you find yourself adding `#if IPLUG_DSP` to a core file, the design is wrong.** Refactor so the core code is unconditional and the platform layer handles the branching.

---

## 8. Performance-Sensitive Code Stays Header-Only

**Rule:** `Voice::Process()` and `VoiceManager::Process()` remain in header files, marked `inline`.

**Why:** These functions are called once per sample per voice — at 48kHz with 8 voices, that's 384,000 calls per second. The compiler needs to see the implementation to inline it effectively. Moving them to a `.cpp` file would prevent cross-translation-unit inlining and likely degrade performance.

**This does NOT mean all code should be in headers.** Only the hot inner loop (Voice::Process, VoiceManager::Process) benefits from this. PresetManager, DemoSequencer, and UI code should be in `.cpp` files.

---

## Summary

| Principle | Enforced by |
|---|---|
| Real-time safety | Code review, no automated check (yet) |
| SynthState single source of truth | `static_assert(is_aggregate_v)`, Reset pattern |
| Serialization completeness | `Test_SynthState.cpp` round-trip test |
| Golden master discipline | `golden_master.py --verify` in CI |
| Test-first for state changes | CI test suite, PR review |
| Layer boundaries | CMake test build (no iPlug2 headers available) |
| Minimal conditional compilation | `#error` guard, code review |
| Hot path stays inline | Code review |
