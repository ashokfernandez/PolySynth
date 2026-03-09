# Sprint 2: Eliminate PolySynth_DSP — Plugin Owns Engine Directly

## Goal

Delete `PolySynth_DSP.h` and have `PolySynthPlugin` own `PolySynthCore::Engine` directly. The ~15 lines of MIDI dispatch move inline into `PolySynthPlugin::ProcessMidiMsg()`.

## Why This Sprint

`PolySynth_DSP` is a 52-line pass-through adapter that adds zero value. It exists as a historical artifact from early prototyping. It pulls iPlug2 headers (`IPlugConstants.h`, `IPlug_include_in_plug_hdr.h`) into what should be a thin boundary between the plugin host and the core engine. Removing it:
- Eliminates a useless indirection layer
- Removes iPlug2 headers from DSP-adjacent code
- Makes the plugin ↔ engine boundary explicit and obvious

## Prerequisites

- None. Can be done in parallel with Sprint 1 (no overlapping files).

## Background Reading

- `src/platform/desktop/PolySynth_DSP.h` — The 52-line file being deleted
- `src/platform/desktop/PolySynth.h` — Lines 163-183 (DSP section, references to `mDSP`)
- `src/platform/desktop/PolySynth.cpp` — All methods that call `mDSP.*`
- `src/core/Engine.h` — The actual DSP engine (will be owned directly)

---

## Tasks

### Task 2.1: Identify all `mDSP` references in PolySynth.cpp

**What to do:** Search for every occurrence of `mDSP` in `PolySynth.cpp` and `PolySynth.h`. Here is the complete list (verified):

**PolySynth.h:**
- Line 67: `#include "PolySynth_DSP.h"` (inside `#if IPLUG_DSP`)
- Line 177: `PolySynthDSP mDSP{8};`

**PolySynth.cpp** (search for `mDSP`):
- `OnReset()`: `mDSP.Reset(GetSampleRate(), GetBlockSize());`
- `ProcessBlock()`: `mDSP.UpdateState(mAudioState);` and `mDSP.ProcessBlock(inputs, outputs, nOutputs, nFrames, ...);`
- `ProcessMidiMsg()`: `mDSP.ProcessMidiMsg(msg);`
- `OnIdle()`: `mDSP.GetActiveVoiceCount()` and `mDSP.GetHeldNotes(buf)`

### Task 2.2: Modify `src/platform/desktop/PolySynth.h`

**What to do:**

1. **Replace the include** (inside `#if IPLUG_DSP` block, around line 66-68):

   **Before:**
   ```cpp
   #if IPLUG_DSP
   #include "../../core/SPSCQueue.h"
   #include "PolySynth_DSP.h"
   #endif
   ```

   **After:**
   ```cpp
   #if IPLUG_DSP
   #include "../../core/SPSCQueue.h"
   #include "../../core/Engine.h"
   #endif
   ```

2. **Replace the member** (line 177):

   **Before:**
   ```cpp
   PolySynthDSP mDSP{8};
   ```

   **After:**
   ```cpp
   PolySynthCore::Engine mEngine;
   ```

### Task 2.3: Modify `src/platform/desktop/PolySynth.cpp`

**What to do:** Replace every `mDSP.xxx()` call with the equivalent `mEngine.xxx()` call. Here are the exact changes:

**Change 1: `OnReset()` method**

Find: `mDSP.Reset(GetSampleRate(), GetBlockSize());`
Replace with: `mEngine.Init(GetSampleRate());`

Note: `Engine::Init()` takes only `sampleRate`. The `blockSize` parameter was ignored by `PolySynthDSP::Reset()` anyway.

**Change 2: `ProcessBlock()` method**

Find:
```cpp
mDSP.UpdateState(mAudioState);
```
Replace with:
```cpp
mEngine.UpdateState(mAudioState);
```

Find:
```cpp
mDSP.ProcessBlock(inputs, outputs, nOutputs, nFrames, GetTransportState().mTempo,
                   GetTransportState().mFlags & ITimeInfo::kTransportRunning);
```
Replace with:
```cpp
mEngine.Process(inputs, outputs, nFrames, nOutputs);
```

Note: The transport state parameters (tempo, running) were passed to `ProcessBlock` but never used. **Do NOT add an explicit `UpdateVisualization()` call** — the block-processing overload `Engine::Process(inputs, outputs, nFrames, nChans)` already calls `UpdateVisualization()` internally at the end (Engine.h line 261). Adding a second call would double-update the atomic visualization state, which is harmless but wasteful.

**Change 3: `ProcessMidiMsg()` method**

Find:
```cpp
mDSP.ProcessMidiMsg(msg);
```
Replace with:
```cpp
int status = msg.StatusMsg();
if (status == IMidiMsg::kNoteOn) {
    mEngine.OnNoteOn(msg.NoteNumber(), msg.Velocity());
} else if (status == IMidiMsg::kNoteOff) {
    mEngine.OnNoteOff(msg.NoteNumber());
} else if (status == IMidiMsg::kControlChange) {
    if (msg.mData1 == 64) { // CC64 = Sustain pedal
        mEngine.OnSustainPedal(msg.mData2 >= 64);
    }
}
```

Note: This is an exact copy of the logic from `PolySynth_DSP.h` lines 29-40, with `mEngine` replacing `mEngine` (same member name in the deleted file).

**Change 4: `OnIdle()` method**

Find all occurrences of `mDSP.GetActiveVoiceCount()` and replace with `mEngine.GetActiveVoiceCount()`.

Find all occurrences of `mDSP.GetHeldNotes(...)` and replace with `mEngine.GetHeldNotes(...)`.

### Task 2.4: Check DemoSequencer.h

**What to do:** Search `DemoSequencer.h` for references to `PolySynthDSP`.

If `DemoSequencer` is templated on or takes a `PolySynthDSP` reference, update it to use `PolySynthCore::Engine` instead.

Based on the current code, `DemoSequencer` takes a lambda callback for NoteOn/NoteOff, so it does NOT reference `PolySynthDSP` directly. No changes needed. **Verify this before proceeding.**

### Task 2.5: Delete `src/platform/desktop/PolySynth_DSP.h`

**What to do:** Delete the file. Then verify no remaining references:
```bash
grep -r "PolySynth_DSP" src/ tests/ --include="*.h" --include="*.cpp"
grep -r "PolySynthDSP" src/ tests/ --include="*.h" --include="*.cpp"
```

Both searches should return zero results.

### Task 2.6: Verify desktop build compiles

**What to do:**
```bash
just desktop-build
```

If this is not available in the current environment, verify with the test build:
```bash
just build
just test
```

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `src/platform/desktop/PolySynth.h` | **MODIFIED** | Include Engine.h, replace `PolySynthDSP mDSP` with `Engine mEngine` |
| `src/platform/desktop/PolySynth.cpp` | **MODIFIED** | Replace all `mDSP.*` calls with `mEngine.*` calls, inline MIDI dispatch |
| `src/platform/desktop/PolySynth_DSP.h` | **DELETED** | Removed — no longer needed |

---

## Testing Instructions

### After Task 2.5 (deletion)
```bash
just build                                          # Test build compiles
just test                                           # All tests pass
just check                                          # Lint + tests pass
python3 scripts/golden_master.py --verify           # Audio unchanged
```

### Verification searches
```bash
# Must return zero results:
grep -r "PolySynth_DSP" src/ tests/ --include="*.h" --include="*.cpp"
grep -r "PolySynthDSP" src/ tests/ --include="*.h" --include="*.cpp"
grep -r "mDSP" src/ tests/ --include="*.h" --include="*.cpp"
```

### What a failure means
- **Compile error about `PolySynthDSP`** → Missed a reference. Search and replace it.
- **Compile error about `IMidiMsg`** → Make sure `PolySynth.cpp` already includes the iPlug2 headers that define `IMidiMsg`. It should, via `IPlug_include_in_plug_src.h`.
- **Compile error about `Engine`** → Check the include path. It should be `#include "../../core/Engine.h"`.
- **Test failure** → The MIDI dispatch logic was transcribed incorrectly. Compare against the original `PolySynth_DSP.h` lines 29-40.

---

## Definition of Done

- [ ] `src/platform/desktop/PolySynth_DSP.h` is deleted from the repository
- [ ] No file in the repo contains the string `PolySynthDSP` or `PolySynth_DSP`
- [ ] No file in the repo contains the string `mDSP` (except in unrelated contexts if any)
- [ ] `PolySynthPlugin` directly owns `PolySynthCore::Engine mEngine`
- [ ] MIDI dispatch is inlined in `PolySynthPlugin::ProcessMidiMsg()` (~8 lines)
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass
- [ ] `just check` — lint + tests pass
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged
- [ ] Zero functional changes to audio processing

---

## Common Pitfalls

1. **`IMidiMsg::kNoteOn` not found** — This constant comes from iPlug2's `IPlugConstants.h`. PolySynth.cpp already includes iPlug2 headers via `IPlug_include_in_plug_src.h`, so this should work. If not, add `#include "IPlugConstants.h"` explicitly.
2. **`Engine::Process` signature mismatch** — Engine has two `Process` overloads:
   - `Process(sample_t&, sample_t&)` — single sample, stereo
   - `Process(sample_t**, sample_t**, int, int)` — block processing
   Make sure you use the block version in `ProcessBlock()`.
3. **Double `UpdateVisualization()` call** — `Engine::Process(inputs, outputs, nFrames, nChans)` already calls `UpdateVisualization()` at the end (line 261). Don't call it again.
4. **`msg.Velocity()` returns 0 for NoteOff** — The original code handles this correctly by checking `status == IMidiMsg::kNoteOff` separately. Make sure both NoteOn and NoteOff are handled.
