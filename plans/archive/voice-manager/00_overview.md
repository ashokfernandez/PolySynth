# Voice Manager & Theory Engine — Overview

## Context

The PolySynth voice manager (`src/core/VoiceManager.h`) currently has 8 hardcoded voices, simple find-first-free allocation, and oldest-voice stealing. There is no polyphony limit enforcement, no allocation modes, no sustain pedal, no unison, no portamento, no stereo panning, and no chord detection.

This feature set upgrades the voice manager in-place and extracts reusable components into the SEA library ecosystem — building toward a standalone library of instrument building blocks for desktop and embedded targets.

## Design Decisions

| # | Decision | Rationale |
|---|----------|-----------|
| 1 | **No IVoiceManager virtual interface** | TheoryEngine takes note arrays directly. UI reads VoiceManager through PolySynth_DSP. No vtable overhead. |
| 2 | **Immediate retrigger on steal** | New NoteOn overwrites the stolen voice; envelope attack masks clicks. `StartSteal()` used only for polyphony-limit reduction (killing excess voices without a new note). No 2ms fade queue. |
| 3 | **Compile-time configurable max voices** | `kMaxVoices` defaults to 16 for desktop, overridable via `POLYSYNTH_MAX_VOICES`. |
| 4 | **Defaults preserve existing behavior** | polyphony=kMaxVoices, allocation=Reset, glide=0, unison=1, spread=0. Golden masters unchanged until Sprint 4. |

## SEA Library Organization

The SEA libraries are split by a clear principle:

- **SEA_DSP** (`libs/SEA_DSP/`) — Signal processing primitives that operate on audio samples. Oscillators, filters, envelopes, effects, delay lines, limiters. **Rule: if it has a `Process(sample)` method, it belongs here.**
- **SEA_Util** (`libs/SEA_Util/`) — Instrument building blocks that don't process audio samples. Data structures, resource management, music theory, MIDI utilities. **Rule: if it *supports* audio applications but doesn't touch sample data, it belongs here.**

Both libraries share these goals:
- Header-only INTERFACE CMake libraries (zero build overhead)
- C++17, portable to desktop (double) and embedded ARM (float)
- No heap allocations after initialization (real-time safe)
- `sea::` namespace for all public types
- Independently testable with no framework dependencies

## Component Architecture

```
libs/SEA_DSP/          (sea:: namespace — signal processing)
├── sea_oscillator.h         existing
├── sea_adsr.h               existing
├── sea_biquad_filter.h      existing
├── ... (filters, effects)   existing
└── (no changes in this plan)

libs/SEA_Util/         (sea:: namespace — instrument utilities)  NEW LIB
├── CMakeLists.txt           NEW
├── README.md                NEW — library goals, conventions, testing guide
├── include/sea_util/
│   ├── sea_spsc_ring_buffer.h   NEW — lock-free SPSC queue
│   ├── sea_voice_allocator.h    NEW — generic voice slot allocator (template)
│   └── sea_theory_engine.h      NEW — chord detection, music theory
└── (future: MIDI utils, param smoothing, scale detection, etc.)

src/core/              (PolySynthCore:: — PolySynth-specific)
├── types.h            MOD — VoiceState enum, VoiceRenderState, kMaxVoices
├── VoiceManager.h     MOD — Voice class + VoiceManager (composes sea::VoiceAllocator)
├── SynthState.h       MOD — 5 new fields
└── PresetManager.cpp  MOD — serialize new fields

src/platform/desktop/  (iPlug2 plugin layer)
├── PolySynth.h        MOD — new EParams, EControlTags
├── PolySynth.cpp      MOD — param wiring, UI layout, OnIdle
└── PolySynth_DSP.h    MOD — DSP wiring, CC64, stereo output
```

### Why this separation?

- **`sea::SPSCRingBuffer`** in SEA_Util — A lock-free data structure. It doesn't process audio samples; it moves data between threads. Reusable for audio-to-UI events, parameter changes, MIDI queues.
- **`sea::VoiceAllocator<Slot, MaxSlots>`** in SEA_Util — Resource management (slot allocation, priority eviction, sustain hold, unison grouping). It decides *which* voice to use, but doesn't generate sound. Reusable for any polyphonic instrument: synths, samplers, drum machines.
- **`sea::TheoryEngine`** in SEA_Util — Music theory analysis (chord detection). Useful for MIDI controllers, notation tools, educational apps. Pure interval arithmetic, no audio.
- **`PolySynthCore::Voice` / `VoiceManager`** in src/core/ — PolySynth-specific DSP voice (oscillators, filters, envelopes, poly-mod). VoiceManager composes `sea::VoiceAllocator` with its own `Voice` array.

## Sprint Plan

| Sprint | Name | Depends On | Deliverables |
|--------|------|------------|--------------|
| 1 | [Scaffolding](./01_sprint_scaffolding.md) | — | SEA_Util lib, SPSCRingBuffer, Voice state machine, types |
| 2 | [Core Allocation](./02_sprint_core_allocation.md) | Sprint 1 | VoiceAllocator, SynthState fields, PresetManager, VoiceManager refactor |
| 3 | [Theory Engine](./03_sprint_theory_engine.md) | Sprint 1 | Chord detection, music theory analysis |
| 4 | [UI Integration](./04_sprint_ui_integration.md) | Sprints 2 & 3 | Params, DSP wiring, stereo, portamento, chord display |

**Note:** Sprints 2 and 3 can proceed in parallel — they both depend only on Sprint 1.

## Files Summary

### New Files (12)
| File | Sprint | Layer | Purpose |
|------|--------|-------|---------|
| `libs/SEA_Util/CMakeLists.txt` | 1 | SEA_Util | New library build config |
| `libs/SEA_Util/README.md` | 1 | SEA_Util | Library conventions & goals doc |
| `libs/SEA_Util/include/sea_util/sea_spsc_ring_buffer.h` | 1 | SEA_Util | Lock-free SPSC ring buffer |
| `libs/SEA_Util/include/sea_util/sea_voice_allocator.h` | 2 | SEA_Util | Generic voice slot allocator (template) |
| `libs/SEA_Util/include/sea_util/sea_theory_engine.h` | 3 | SEA_Util | Chord detection engine |
| `tests/unit/Test_VoiceStateMachine.cpp` | 1 | Test | Voice state transition tests |
| `tests/unit/Test_SPSCRingBuffer.cpp` | 1 | Test | Ring buffer tests |
| `tests/unit/Test_VoiceAllocator.cpp` | 2 | Test | Allocator unit tests with MockSlot |
| `tests/unit/Test_VoiceAllocation.cpp` | 2 | Test | Integration: allocator + real Voice |
| `tests/unit/Test_TheoryEngine.cpp` | 3 | Test | Chord detection tests |
| `tests/unit/Test_Portamento.cpp` | 4 | Test | Glide behavior tests |
| `tests/unit/Test_StereoPanning.cpp` | 4 | Test | Stereo output tests |

### Modified Files (10)
| File | Sprints | Changes |
|------|---------|---------|
| `src/core/types.h` | 1, 2 | VoiceState, VoiceRenderState, VoiceEvent, kMaxVoices, enum aliases |
| `src/core/VoiceManager.h` | 1, 2, 4 | State machine, compose VoiceAllocator, stereo, portamento |
| `src/core/SynthState.h` | 2 | 5 new fields |
| `src/core/PresetManager.cpp` | 2 | SERIALIZE/DESERIALIZE for new fields |
| `src/platform/desktop/PolySynth.h` | 4 | 6 new EParams, 2 new EControlTags |
| `src/platform/desktop/PolySynth.cpp` | 4 | Param init, OnParamChange, SyncUIState, BuildHeader, OnIdle |
| `src/platform/desktop/PolySynth_DSP.h` | 4 | UpdateState wiring, CC64, stereo ProcessBlock |
| `tests/CMakeLists.txt` | 1-4 | Add test files, add SEA_Util subdirectory + link |
| `tests/unit/Test_SynthState.cpp` | 2 | New fields in Reset + round-trip |
| `tests/unit/Test_VoiceManager_Poly.cpp` | 1 | Update for kMaxVoices capacity |

## How to Build & Test

```bash
# Build and run unit tests
cd tests && mkdir -p build && cd build
cmake .. -DIPLUG_UNIT_TESTS=ON
make -j
./run_tests

# Verify golden masters haven't changed
python3 scripts/golden_master.py --verify
```

## Key References

- **Design Principles**: `docs/architecture/DESIGN_PRINCIPLES.md` — 8 non-negotiable rules
- **Existing Tests**: `tests/unit/Test_VoiceManager_Poly.cpp`, `tests/unit/Test_SynthState.cpp`
- **SEA_DSP pattern to follow**: `libs/SEA_DSP/CMakeLists.txt` (INTERFACE library)
- **Platform abstraction**: `libs/SEA_DSP/include/sea_dsp/sea_platform.h` (`sea::Real`, `SEA_INLINE`)
- **Testing Strategy**: `plans/active/04_testing_strategy.md`
- **Agentic Workflow**: `plans/active/03_agentic_workflow.md`
