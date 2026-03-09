# PolySynth Architecture Review & Refactoring Agent Prompt

> **Purpose:** This document is a comprehensive prompt for an AI agent (or human reviewer) to perform deep architectural review, identify violations, propose refactors, and write tests for the PolySynth codebase. It encodes the project's vision, constraints, known issues, and quality standards.
>
> **How to use:** Give this entire document as context to an agent tasked with reviewing or refactoring the codebase. It is self-contained.

---

## 1. PROJECT VISION

PolySynth is a modular polyphonic synthesizer written in C++ using the iPlug2 framework. It currently ships as a desktop audio plugin (VST3/AU/Standalone).

**The architectural north star is strict portability.** The core DSP engine must run identically in two contexts with zero code changes:

1. **Desktop Plugin** — iPlug2 handles the host interface, parameter management, MIDI routing, and NanoVG-based GUI. Runs on macOS/Windows/Linux at 44.1–192kHz sample rates with double-precision floating point.

2. **Embedded Hardware** — A microcontroller (e.g., Raspberry Pi RP2350, STM32H7) runs the same core engine. Physical controls (rotary encoders, buttons, sliders) replace the GUI. An I2S DAC replaces the host audio callback. An SPI/I2C OLED or small LCD replaces the NanoVG canvas. The embedded target may use single-precision float (`POLYSYNTH_USE_FLOAT`), has limited RAM (264KB–1MB), no heap allocator in the audio path, and no OS-level threading.

**This means:** every architectural decision must be evaluated through the lens of "would this compile and run on a Cortex-M33 with 264KB of SRAM?" If the answer is no, it belongs in the platform layer, not the core.

---

## 2. LAYER ARCHITECTURE

The codebase enforces a strict four-layer dependency hierarchy. Dependencies flow downward only — never upward or sideways.

```
┌──────────────────────────────────────────────────┐
│  Platform Layer (src/platform/desktop/)           │
│  iPlug2 plugin shell, NanoVG UI, host integration │
│  ── OR ──                                         │
│  Embedded Platform (future: src/platform/embed/)  │
│  RTOS/bare-metal, GPIO, I2S, SPI display          │
├──────────────────────────────────────────────────┤
│  Application Core (src/core/)                     │
│  Engine, VoiceManager, SynthState, PresetManager  │
│  100% platform-agnostic standard C++              │
├──────────────────────────────────────────────────┤
│  SEA_Util (libs/SEA_Util/)                        │
│  VoiceAllocator, SPSCRingBuffer, TheoryEngine     │
│  Audio-adjacent utilities, no sample processing   │
├──────────────────────────────────────────────────┤
│  SEA_DSP (libs/SEA_DSP/)                          │
│  Oscillator, Filters, ADSR, LFO, Effects          │
│  Pure sample-processing primitives                │
└──────────────────────────────────────────────────┘
```

### Layer Rules (Enforced)

| Layer | May include | Must NEVER include |
|---|---|---|
| `libs/SEA_DSP/` | `<cmath>`, `<cstdint>`, `<algorithm>`, standard library math | Anything from core, platform, or iPlug2 |
| `libs/SEA_Util/` | SEA_DSP headers, standard library | Anything from core, platform, or iPlug2 |
| `src/core/` | SEA_DSP, SEA_Util, standard library | Anything from `src/platform/`, any iPlug2 header (`IPlug*.h`, `IGraphics*.h`) |
| `src/platform/desktop/` | Core, SEA_DSP, SEA_Util, iPlug2 (UI code under `#if IPLUG_EDITOR`) | Nothing — it's the top layer |
| `tests/unit/` | Core, SEA_DSP, SEA_Util, Catch2 | Platform headers, iPlug2 headers |

**Verification:** The CMake test build (`just build && just test`) compiles core code without iPlug2 headers available. If a core file accidentally includes an iPlug2 header, the test build fails. This is the canary.

---

## 3. REAL-TIME SAFETY CONSTRAINTS

The audio thread has a hard deadline: typically 1–10ms per buffer. Any blocking operation causes audible glitches.

### Absolute prohibitions in the audio path (anything called from `ProcessBlock` or `Voice::Process`):

- **No heap allocation:** `new`, `malloc`, `std::vector::push_back`, `std::string`, `std::map`, `std::shared_ptr`, `std::make_unique`
- **No locks:** `std::mutex`, `std::lock_guard`, `std::unique_lock`, any pthread mutex
- **No blocking I/O:** file reads/writes, `printf`, `std::cout`, `std::cerr`, network calls
- **No exceptions:** `throw`, `try`/`catch` — anywhere in the audio call tree
- **No unbounded iteration:** algorithms whose runtime depends on input size without a compile-time bound
- **No `std::function` or heap-capturing lambdas** — these can allocate on construction

### What IS allowed:

- `std::array` (stack-allocated, fixed size)
- `std::atomic` (lock-free on the target platform — verify with `is_always_lock_free`)
- `memset`, `memcpy` on pre-allocated buffers
- `std::clamp`, `std::min`, `std::max`, `std::abs`
- `std::pow`, `std::exp`, `std::sin`, `std::cos`, `std::tanh` (but prefer lookup tables on embedded)
- Stack-allocated local variables
- Inline function calls

### Communication between threads:

| Direction | Mechanism | Current Implementation |
|---|---|---|
| UI → Audio (parameter updates) | Lock-free | `SynthState` + dirty flag (see Known Issue #1) |
| Audio → UI (voice states) | `std::atomic` | Atomic voice count + bitmask-packed held notes |
| Audio → UI (events) | SPSC ring buffer | `SPSCRingBuffer<VoiceEvent>` (defined but underused) |

---

## 4. KNOWN ARCHITECTURAL ISSUES

These are confirmed problems that a reviewing agent should address. They are ordered by severity.

### Issue 1: Data Race on SynthState (CRITICAL)

**Location:** `PolySynth.h:181-182`, `PolySynth.cpp` (`OnParamChange` and `ProcessBlock`)

**Problem:** `mState` (a ~300-byte `SynthState` struct) is written by the UI thread in `OnParamChange()` and read by the audio thread in `ProcessBlock()`. The dirty flag `mIsDirty` is a plain `bool`, not `std::atomic<bool>`. Even if it were atomic, reading a large struct that's being concurrently written is a data race (undefined behavior in C++).

**Required fix:** Either:
- (a) Double-buffer: maintain two `SynthState` instances, write to the inactive one, atomically swap a pointer/index. The audio thread always reads the "current" buffer.
- (b) SPSC queue: push `SynthState` snapshots through `SPSCRingBuffer<SynthState, 4>`. The audio thread drains the queue at the start of each `ProcessBlock` and uses the most recent snapshot.
- (c) Triple-buffer: a lock-free triple-buffer pattern (writer always writes to a free slot, reader always reads the most recently published slot).

Option (b) is simplest given that `SPSCRingBuffer` already exists in the codebase.

**Test to write:** A stress test that writes SynthState from one thread while reading from another, verifying no torn reads (all fields consistent from the same "generation").

### Issue 2: PolySynth_DSP Duplicates Engine (MODERATE)

**Location:** `src/platform/desktop/PolySynth_DSP.h`

**Problem:** `PolySynth_DSP` is a near-copy of `src/core/Engine.h`. Both own a `VoiceManager` + `VintageChorus` + `VintageDelay` + `LookaheadLimiter` and have almost identical `Process` loops. `PolySynth_DSP` adds:
1. `UpdateState(SynthState&)` — the 60-line bulk parameter push (this belongs in Engine)
2. Atomic visualization mirrors (`mVisualActiveVoiceCount`, held-note bitmasks)
3. `ProcessMidiMsg(const IMidiMsg&)` — depends on iPlug2's `IMidiMsg` type

**Coupling:** `PolySynth_DSP.h` includes `IPlugConstants.h` and `IPlug_include_in_plug_hdr.h`, and has `using namespace iplug`. Its `ProcessBlock` signature uses `iplug::sample**`.

**Required refactor:**
1. Move `UpdateState(SynthState&)` into `Engine.h` — this is pure parameter mapping, nothing platform-specific
2. Move the atomic visualization mirrors into `Engine.h` — they're useful for any platform that needs to display voice state
3. Reduce `PolySynth_DSP` to a thin adapter that: (a) owns an `Engine`, (b) translates `IMidiMsg` to `OnNoteOn/Off/SustainPedal` calls, (c) translates `iplug::sample**` buffers to `Engine::Process` calls
4. Alternatively, eliminate `PolySynth_DSP` entirely and have `PolySynthPlugin` own an `Engine` directly, with the MIDI translation inline in `ProcessMidiMsg`

**Test:** After refactoring, all existing `Test_Engine_*` tests must pass. The engine should be fully exercisable without iPlug2.

### Issue 3: Engine Missing Bulk State Update (MODERATE)

**Location:** `src/core/Engine.h`

**Problem:** `Engine` has ~20 individual setter methods but no `UpdateState(const SynthState&)`. The mapping from `SynthState` fields to engine methods lives in `PolySynth_DSP.h` (platform layer), meaning an embedded platform would have to reimplement this mapping.

**Required fix:** Add `void UpdateState(const SynthState& state)` to `Engine`. Move the parameter fan-out logic from `PolySynth_DSP::UpdateState` into it.

### Issue 4: iplug::sample vs sample_t Type Mismatch (MINOR)

**Location:** `PolySynth_DSP.h:84`

**Problem:** `ProcessBlock` uses `iplug::sample**` (which is `double**`). The core engine uses `PolySynthCore::sample_t` (which is `double` or `float` depending on `POLYSYNTH_USE_FLOAT`). If the embedded target uses float precision, the types diverge. The adapter layer should handle the cast.

### Issue 5: Visualization State Could Use VoiceEvent Queue (MINOR)

**Location:** `PolySynth_DSP.h:122-137`, `src/core/types.h:52-61`

**Problem:** `VoiceEvent` and `SPSCRingBuffer` are defined but the audio→UI voice event path still uses atomic bitmask packing instead. This works, but the event queue would be cleaner and support richer UI animations (e.g., per-voice envelope phase). The infrastructure is already built — it just needs to be wired up.

---

## 5. REVIEW CHECKLIST

When reviewing any file or proposed change, evaluate against ALL of the following criteria:

### 5.1 Separation of Concerns
- [ ] Does any `src/core/` file include iPlug2 headers? (Must not)
- [ ] Does any `libs/SEA_DSP/` file include core or platform headers? (Must not)
- [ ] Is `#if IPLUG_DSP` or `#if IPLUG_EDITOR` used in core files? (Must not)
- [ ] Can this module be compiled and tested without iPlug2? (Must be yes for core/libs)

### 5.2 Real-Time Safety
- [ ] Are there heap allocations in the audio path? (`new`, `malloc`, `std::vector`, `std::string`, `std::map`, `std::shared_ptr`)
- [ ] Are there locks or blocking calls in the audio path?
- [ ] Are there exception throws in the audio path call tree?
- [ ] Is `std::function` used anywhere in the audio path? (It can heap-allocate)
- [ ] Are thread communication mechanisms lock-free?

### 5.3 Embedded Portability
- [ ] Does the core code use `sample_t` consistently (not hardcoded `double`)?
- [ ] Are voice arrays and buffers fixed-size (`std::array`, not `std::vector`)?
- [ ] Would this code compile on a Cortex-M33 with no OS? (No POSIX, no threads, no filesystem)
- [ ] Are math operations embedded-friendly? (Prefer polynomial approximations over `std::tanh` for hot paths)
- [ ] Is the memory footprint bounded at compile time?

### 5.4 Testability
- [ ] Can this class be instantiated in a unit test without mocking the framework?
- [ ] Are dependencies injectable? (Or at minimum, not hidden behind singletons)
- [ ] Is there a corresponding test file in `tests/unit/`?
- [ ] Does the test cover edge cases? (Zero input, max polyphony, parameter extremes, note-on/off ordering)

### 5.5 State Management
- [ ] Is `SynthState` the single source of truth for parameter values?
- [ ] Are parameter defaults defined as inline member initializers in `SynthState` (not in constructors or Reset methods)?
- [ ] Is `SynthState` still an aggregate? (`static_assert` enforces this)
- [ ] Is the state transfer to the audio thread race-free?

### 5.6 Code Quality
- [ ] Are there duplicated signal chains? (e.g., `PolySynth_DSP` vs `Engine`)
- [ ] Are there magic numbers without named constants?
- [ ] Is parameter mapping (e.g., `depth * 5.0` for chorus) documented or at least consistent between `Engine` and `PolySynth_DSP`?
- [ ] Are unused parameters suppressed with `(void)param`?
- [ ] Are return values checked or explicitly discarded?

---

## 6. BUG RESOLUTION PROTOCOL (TDD)

When a bug is found during review or reported by the user, follow this strict sequence. Do NOT skip steps.

### Step 1: Characterize
Identify the root cause. Is it a logic error, a threading issue, a type mismatch, an edge case, or an architectural violation?

### Step 2: Write a Failing Test
Before writing any fix, create a Catch2 test in the appropriate `tests/unit/Test_*.cpp` file that:
- Reproduces the exact conditions that trigger the bug
- Asserts the correct behavior
- **Fails** with the current code (proving the test catches the bug)

### Step 3: Fix the Code
Implement the minimal fix. Do not refactor unrelated code in the same commit.

### Step 4: Verify
- The new test passes
- All existing tests pass (`just test`)
- Golden masters verify (`python3 scripts/golden_master.py --verify`)
- If the fix changes audio output, regenerate golden masters and commit them alongside the fix

### Step 5: Document
Add an entry to `learnings/` or `plans/SPRINT_RETROSPECTIVE_NOTES.md` in Problem/Action/Lesson format.

---

## 7. REFACTORING GUIDELINES

When proposing refactors, adhere to these principles:

### Do:
- Make the smallest change that achieves the goal
- Preserve existing test coverage — never delete a passing test unless the tested behavior is intentionally removed
- Keep the hot path (Voice::Process) in header files for inlining
- Use `sample_t` for signal values, `float` for control parameters, `uint32_t` for timestamps
- Prefer compile-time polymorphism (templates, `constexpr if`) over runtime polymorphism (virtual methods) in the audio path
- Run `just check` (lint + test) after every refactor

### Don't:
- Add virtual methods to classes in the audio path (vtable indirection + prevents inlining)
- Add `std::unique_ptr` or `std::shared_ptr` in DSP classes (heap allocation on construction)
- Add RTTI (`dynamic_cast`, `typeid`) — it's not available on all embedded targets
- Over-abstract — three similar lines of code is better than a premature abstraction
- Break the `just test` build — core code must always compile without iPlug2

---

## 8. LIVING GUIDELINES PROTOCOL

This document and `DESIGN_PRINCIPLES.md` are living documents. They must evolve as the project does.

### When to update:
- A new architectural decision is made (e.g., "we will use triple-buffering for state transfer")
- A new constraint is discovered (e.g., "RP2350 doesn't support `std::atomic<double>` lock-free")
- A bug is found that reveals a missing rule
- A refactor changes how layers communicate

### How to update:
1. Add the rule to the appropriate section of this document or `DESIGN_PRINCIPLES.md`
2. If the rule is enforceable by tooling (static analysis, compile-time check), add the check
3. If the rule was learned from a bug, add it to `learnings/` as well

### Format for new rules:
```
### Rule: [Short name]
**Why:** [What goes wrong if this rule is violated]
**Constraint:** [The specific prohibition or requirement]
**Enforced by:** [Test name, static_assert, lint script, or "code review"]
```

---

## 9. CURRENT CODEBASE MAP

For quick orientation when starting a review:

```
src/core/
  types.h              - sample_t, kMaxVoices, VoiceState, VoiceRenderState, VoiceEvent
  SynthState.h         - Parameter aggregate struct (single source of truth)
  Voice.h              - Per-voice DSP chain: oscillators, filter, envelopes, LFO, modulation
  VoiceManager.h       - Voice allocation, polyphony management, stereo mixing
  Engine.h             - Top-level DSP engine (VoiceManager + FX chain)
  DspConstants.h       - Named constants for all DSP tuning parameters
  PresetManager.h/.cpp - JSON serialization of SynthState

src/platform/desktop/
  config.h             - Plugin identity, dimensions, font paths
  PolySynth.h          - Plugin class (EParams, EControlTags, PolySynthPlugin)
  PolySynth.cpp         - Plugin implementation (OnParamChange, ProcessBlock, OnLayout, UI builders). Owns Engine directly.
  DemoSequencer.h      - Programmatic MIDI playback for demos

UI/Controls/
  PolyTheme.h          - Design system (colors, fonts, layout constants)
  PolyKnob.h           - Custom rotary knob control
  PolySection.h        - Section panel background
  Envelope.h           - Animated ADSR envelope visualization
  ADSRViewModel.h      - ADSR geometry/color computation (no iPlug2 dependency)
  [others]             - Toggle buttons, LCD panel, section frame, preset save button

libs/SEA_DSP/          - Pure DSP primitives (oscillator, filters, ADSR, LFO, effects)
libs/SEA_Util/         - Audio utilities (voice allocator, SPSC ring buffer, theory engine)

tests/unit/            - Catch2 tests (DSP, voice management, state, presets, effects)
tests/demos/           - Standalone audio rendering programs for audible verification
```

---

## 10. ISSUE RESOLUTION STATUS

| Issue | Original Description | Status | Resolution |
|-------|---------------------|--------|------------|
| #1 | SynthState data race between UI/audio threads | RESOLVED | SPSCQueue<SynthState, 4> in PolySynth.h |
| #2 | PolySynth_DSP unnecessary indirection | RESOLVED (Sprint 2) | Deleted; plugin owns Engine directly |
| #3 | Engine::UpdateState missing | RESOLVED | Engine::UpdateState centralises parameter fan-out |
| #4 | Voice monolith in VoiceManager.h | RESOLVED (Sprint 1) | Voice extracted to Voice.h |
| #5 | Magic numbers in hot path | RESOLVED (Sprint 3) | All constants in DspConstants.h |
| #6 | Missing test coverage | RESOLVED (Sprints 1, 4, 6) | 55+ test cases covering Voice, filters, boundaries, LFO |
| #7 | Performance: per-sample pow/exp/sqrt | RESOLVED (Sprint 5) | Cached in setter methods |

---

## 11. IMMEDIATE PRIORITIES FOR REVIEW

If tasked with a full codebase review, address these in order:

1. **Fix the SynthState data race** (Issue #1) — this is undefined behavior in production
2. **Consolidate PolySynth_DSP into Engine** (Issue #2 + #3) — eliminate duplication, make Engine self-sufficient
3. **Audit the audio path for RT violations** — walk the full call tree from `ProcessBlock` through `Voice::Process` and every DSP primitive, looking for hidden allocations or blocking calls
4. **Wire up VoiceEvent SPSC queue** (Issue #5) — replace atomic bitmask packing with the event queue that's already defined
5. **Add missing test coverage** — particularly for:
   - Thread safety of the state transfer mechanism (after fixing Issue #1)
   - Engine with `UpdateState` (after fixing Issue #3)
   - Edge cases in voice stealing with unison enabled
   - Parameter extremes (cutoff at 20Hz and 20kHz, resonance at 0 and 1, zero attack/release)

---

## APPENDIX A: EMBEDDED TARGET REFERENCE

For evaluating portability decisions:

| Property | RP2350 (Target) | Desktop (Current) |
|---|---|---|
| CPU | Dual Cortex-M33 @ 150MHz | x86-64 @ 3-5GHz |
| RAM | 520KB SRAM | 8-64GB |
| FPU | Single-precision (float) | Double-precision (double) |
| Heap | Optional (FreeRTOS) or none | Full OS allocator |
| Threading | FreeRTOS tasks or bare-metal | OS threads |
| Audio I/O | I2S DMA, typically 48kHz/16-bit | Host callback, 44.1-192kHz/32-bit float |
| Display | SPI OLED 128x64 or small LCD | NanoVG on 1024x800 window |
| `std::atomic<double>` | NOT lock-free | Lock-free |
| `std::atomic<float>` | Lock-free | Lock-free |
| `std::atomic<uint32_t>` | Lock-free | Lock-free |
| Exceptions | Typically disabled (`-fno-exceptions`) | Available |
| RTTI | Typically disabled (`-fno-rtti`) | Available |

**Key implication:** Use `float` (not `double`) for atomics shared between threads. Use `uint32_t` for timestamps. The `POLYSYNTH_USE_FLOAT` flag switches `sample_t` to `float` for the entire signal path.
