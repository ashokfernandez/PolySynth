# Sprint 1: Scaffolding — SEA_Util, SPSCRingBuffer, Voice State Machine

**Depends on:** Nothing
**Blocks:** Sprints 2, 3, 4
**Approach:** TDD — write all tests first, then implement until green

---

## Task 1.1: Create `libs/SEA_Util/` Library

### What
Create the SEA_Util header-only INTERFACE library, following the same pattern as SEA_DSP.

### Files to Create

**`libs/SEA_Util/CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(SEA_Util)

add_library(SEA_Util INTERFACE)

target_include_directories(SEA_Util INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
```

**`libs/SEA_Util/include/sea_util/`** — empty directory (files added in subsequent tasks)

**`libs/SEA_Util/README.md`** — Library documentation covering:
- Purpose: instrument building blocks that don't process audio samples
- Split principle: SEA_DSP = signal processing (`Process(sample)`), SEA_Util = everything else
- Design goals: header-only, C++17, no heap in hot path, `sea::` namespace
- Performance targets: all hot-path code inlineable, no virtual dispatch
- Portability: desktop (double) and embedded ARM (float via `sea::Real`)
- Testing: each component must have Catch2 unit tests in the consuming project's test suite
- Conventions: file naming (`sea_<component>.h`), namespace (`sea::`), include guards (`#pragma once`)

### Files to Modify

**`tests/CMakeLists.txt`** — Add SEA_Util subdirectory and link to targets.

After line 23 (the SEA_DSP add_subdirectory), add:
```cmake
# --- SEA_Util Library ---
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libs/SEA_Util" "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_Util")
```

After line 72 (`target_link_libraries(run_tests PRIVATE SEA_DSP)`), add:
```cmake
target_link_libraries(run_tests PRIVATE SEA_Util)
```

### Acceptance Criteria
- [ ] `cmake ..` in tests/build succeeds with SEA_Util found
- [ ] `#include <sea_util/sea_spsc_ring_buffer.h>` compiles (after Task 1.2)
- [ ] README.md documents library conventions

---

## Task 1.2: Implement `sea::SPSCRingBuffer`

### What
A lock-free single-producer single-consumer ring buffer. Thread-safe for one writer and one reader without locks. Used for audio thread → UI thread event passing.

### File to Create

**`libs/SEA_Util/include/sea_util/sea_spsc_ring_buffer.h`**

### API Specification
```cpp
namespace sea {

template <typename T, uint32_t Capacity>
class SPSCRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static_assert(Capacity > 0, "Capacity must be > 0");

public:
    SPSCRingBuffer();

    // Producer (single thread): returns false if full
    bool TryPush(const T& item);

    // Consumer (single thread): returns false if empty
    bool TryPop(T& item);

    // Consumer: number of items available to read (approximate)
    uint32_t Size() const;

    // Consumer: discard all items
    void Clear();

private:
    std::array<T, Capacity> mBuffer;
    std::atomic<uint32_t> mHead{0};  // written by producer
    std::atomic<uint32_t> mTail{0};  // written by consumer
};

} // namespace sea
```

### Implementation Notes
- Use `std::memory_order_acquire` for loads, `std::memory_order_release` for stores
- Mask index with `(Capacity - 1)` for wrap-around (power-of-2 trick)
- `TryPush`: write to `mBuffer[head & mask]`, store `head + 1` with release
- `TryPop`: read from `mBuffer[tail & mask]`, store `tail + 1` with release
- Full condition: `head - tail == Capacity`
- Empty condition: `head == tail`
- Only includes: `<array>`, `<atomic>`, `<cstdint>`
- No heap allocations — `std::array` storage

### Test File to Create FIRST

**`tests/unit/Test_SPSCRingBuffer.cpp`**

```cpp
#include <sea_util/sea_spsc_ring_buffer.h>
#include "catch.hpp"

TEST_CASE("SPSCRingBuffer: push and pop single item", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    REQUIRE(buf.Size() == 0);

    REQUIRE(buf.TryPush(42));
    REQUIRE(buf.Size() == 1);

    int val = 0;
    REQUIRE(buf.TryPop(val));
    REQUIRE(val == 42);
    REQUIRE(buf.Size() == 0);
}

TEST_CASE("SPSCRingBuffer: returns false when full", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    REQUIRE(buf.TryPush(1));
    REQUIRE(buf.TryPush(2));
    REQUIRE(buf.TryPush(3));
    REQUIRE(buf.TryPush(4));
    REQUIRE_FALSE(buf.TryPush(5));  // Full
}

TEST_CASE("SPSCRingBuffer: returns false when empty", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    int val = 0;
    REQUIRE_FALSE(buf.TryPop(val));
}

TEST_CASE("SPSCRingBuffer: FIFO ordering preserved", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 8> buf;
    for (int i = 0; i < 5; i++) {
        REQUIRE(buf.TryPush(i * 10));
    }
    for (int i = 0; i < 5; i++) {
        int val = -1;
        REQUIRE(buf.TryPop(val));
        REQUIRE(val == i * 10);
    }
}

TEST_CASE("SPSCRingBuffer: wraps around correctly", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    // Fill and drain multiple times to test wrap-around
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            REQUIRE(buf.TryPush(round * 100 + i));
        }
        for (int i = 0; i < 4; i++) {
            int val = -1;
            REQUIRE(buf.TryPop(val));
            REQUIRE(val == round * 100 + i);
        }
        REQUIRE(buf.Size() == 0);
    }
}

TEST_CASE("SPSCRingBuffer: Clear empties buffer", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    buf.TryPush(1);
    buf.TryPush(2);
    REQUIRE(buf.Size() == 2);
    buf.Clear();
    REQUIRE(buf.Size() == 0);
    int val = -1;
    REQUIRE_FALSE(buf.TryPop(val));
}

TEST_CASE("SPSCRingBuffer: Size reports correctly", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 8> buf;
    REQUIRE(buf.Size() == 0);
    buf.TryPush(1);
    REQUIRE(buf.Size() == 1);
    buf.TryPush(2);
    REQUIRE(buf.Size() == 2);
    int val;
    buf.TryPop(val);
    REQUIRE(buf.Size() == 1);
}
```

### Add to `tests/CMakeLists.txt`

Add `unit/Test_SPSCRingBuffer.cpp` to `UNIT_TEST_SOURCES`.

### Acceptance Criteria
- [ ] All 7 test cases pass
- [ ] No heap allocations (std::array storage)
- [ ] Only standard library headers included
- [ ] Compiles with `-std=c++17`

---

## Task 1.3: Add Enums and Structs to `src/core/types.h`

### What
Add the voice state machine types and compile-time max voices constant to the core types header.

### File to Modify

**`src/core/types.h`** — Add after the existing constants block (after line 22, `constexpr int kMaxBlockSize = 4096;`):

```cpp
// Compile-time voice count — override with -DPOLYSYNTH_MAX_VOICES=N
#ifndef POLYSYNTH_MAX_VOICES
#define POLYSYNTH_MAX_VOICES 16
#endif
constexpr int kMaxVoices = POLYSYNTH_MAX_VOICES;

// Voice lifecycle states
enum class VoiceState : uint8_t {
    Idle = 0,     // Not playing — available for allocation
    Attack,       // Note just triggered — envelope in attack phase
    Sustain,      // Note held — envelope past decay
    Release,      // Note released — envelope fading out
    Stolen        // Being killed (polyphony limit reduction) — 2ms fade-out
};

// Snapshot of a single voice for UI display (read from audio thread)
struct VoiceRenderState {
    uint8_t voiceID = 0;
    VoiceState state = VoiceState::Idle;
    int note = -1;
    int velocity = 0;
    float currentPitch = 0.0f;    // Current frequency (may differ from target during glide)
    float panPosition = 0.0f;     // -1.0 (left) to +1.0 (right)
    float amplitude = 0.0f;       // Current envelope amplitude (0.0–1.0)
};

// Event sent from audio thread to UI thread via SPSCRingBuffer
struct VoiceEvent {
    enum class Type : uint8_t { NoteOn, NoteOff, Steal, StateChange };
    Type type = Type::NoteOn;
    uint8_t voiceID = 0;
    int note = -1;
    int velocity = 0;
    float pitch = 0.0f;
    float pan = 0.0f;
};
```

### Acceptance Criteria
- [ ] `VoiceState`, `VoiceRenderState`, `VoiceEvent` compile
- [ ] `kMaxVoices` defaults to 16
- [ ] `static_assert(std::is_aggregate_v<SynthState>)` still compiles (no SynthState changes here)
- [ ] No new includes beyond `<stdint.h>` (already included)

---

## Task 1.4: Add State Machine to `Voice` Class

### What
Extend the existing `Voice` class in `src/core/VoiceManager.h` with a state machine, metadata fields, and trait-compatible getters.

### File to Modify

**`src/core/VoiceManager.h`** — Voice class modifications:

### New Private Fields (add after existing privates, line ~285)
```cpp
// State machine (Sprint 1)
uint8_t mVoiceID = 0;
VoiceState mVoiceState = VoiceState::Idle;
uint64_t mTimestamp = 0;          // Global monotonic timestamp from VoiceManager
float mCurrentPitch = 0.0f;       // Current pitch in Hz (for UI / portamento)
float mPanPosition = 0.0f;        // -1.0 to +1.0 (center default)
float mStolenFadeGain = 0.0f;     // Gain multiplier during stolen fade-out
double mStolenFadeDelta = 0.0;    // Per-sample gain decrement for 2ms fade
double mSampleRate = 48000.0;     // Stored for fade time calculation
```

### Modified Methods

**`Init(double sampleRate)` → `Init(double sampleRate, uint8_t voiceID = 0)`**
- Store `mSampleRate = sampleRate`
- Store `mVoiceID = voiceID`
- Set `mVoiceState = VoiceState::Idle`
- Set `mTimestamp = 0`
- Set `mCurrentPitch = 0.0f`
- Set `mPanPosition = 0.0f`
- Keep all existing initialization

**`NoteOn(int note, int velocity)` → `NoteOn(int note, int velocity, uint64_t timestamp = 0)`**
- Keep all existing logic (frequency calc, osc reset, envelope trigger, etc.)
- Add: `mVoiceState = VoiceState::Attack`
- Add: `mTimestamp = timestamp`
- Add: `mCurrentPitch = static_cast<float>(mFreq)`

**`NoteOff()`**
- Keep existing logic (envelope NoteOff)
- Add: `mVoiceState = VoiceState::Release`

**`Process()`** — Add state transitions:
- At the existing `mActive = false` / `mNote = -1` block (line ~169-173):
  - Also set `mVoiceState = VoiceState::Idle`
- Add at the top of Process (after the `if (!mActive) return 0.0` guard):
  - If `mVoiceState == VoiceState::Stolen`: apply fade-out gain, decrement, transition to Idle when gain <= 0
  ```cpp
  if (mVoiceState == VoiceState::Stolen) {
      mStolenFadeGain -= static_cast<float>(mStolenFadeDelta);
      if (mStolenFadeGain <= 0.0f) {
          mActive = false;
          mNote = -1;
          mVoiceState = VoiceState::Idle;
          return 0.0;
      }
      // Continue processing but scale output by fade gain at the end
  }
  ```
- At the final `return out` line: if Stolen, multiply by `mStolenFadeGain`
- Transition from Attack to Sustain: after envelope passes attack phase.
  - Check: if `mVoiceState == VoiceState::Attack` and `mAmpEnv` is past attack (amplitude started decreasing or sustain reached). Simplest approach: track if `ampEnvVal` dropped below 1.0 after being at 1.0, or check if the ADSR is in a sustain/decay state.
  - **Pragmatic approach**: Use the ADSR's internal state if accessible, or transition to Sustain after the first Process() where the amp envelope is not in attack. If the ADSR doesn't expose its phase, a simpler heuristic: transition on `NoteOn()` always sets Attack, and after `attackTime * sampleRate` samples have processed, transition to Sustain.
  - **Recommended**: Check if `sea::ADSREnvelope` exposes a phase getter. If yes, use it. If not, skip the Attack→Sustain auto-transition for now — it's only needed for the "skip recently attacked voices during steal" heuristic, which VoiceManager can handle by checking timestamp instead.

**New Method: `StartSteal()`**
```cpp
void StartSteal() {
    // Polyphony-limit kill: 2ms fade-out to avoid click
    mStolenFadeGain = 1.0f;
    double fadeSamples = 0.002 * mSampleRate;  // 2ms
    mStolenFadeDelta = 1.0 / fadeSamples;
    mVoiceState = VoiceState::Stolen;
}
```

### New Getters
```cpp
VoiceState GetVoiceState() const { return mVoiceState; }
uint8_t GetVoiceID() const { return mVoiceID; }
uint64_t GetTimestamp() const { return mTimestamp; }
float GetCurrentPitch() const { return mCurrentPitch; }
float GetPitch() const { return mCurrentPitch; }  // VoiceAllocator trait
float GetPanPosition() const { return mPanPosition; }
void SetPanPosition(float pan) { mPanPosition = std::clamp(pan, -1.0f, 1.0f); }

VoiceRenderState GetRenderState() const {
    VoiceRenderState rs;
    rs.voiceID = mVoiceID;
    rs.state = mVoiceState;
    rs.note = mNote;
    rs.velocity = static_cast<int>(mVelocity * 127.0);
    rs.currentPitch = mCurrentPitch;
    rs.panPosition = mPanPosition;
    rs.amplitude = 0.0f;  // TODO: set from last amp envelope value
    return rs;
}
```

### Acceptance Criteria
- [ ] Voice starts in Idle state
- [ ] NoteOn transitions to Attack
- [ ] NoteOff transitions to Release
- [ ] Envelope finishing transitions to Idle
- [ ] StartSteal sets Stolen state with 2ms fade
- [ ] Stolen fade completes and transitions to Idle
- [ ] All existing Voice tests still pass
- [ ] Getters return correct values

---

## Task 1.5: Update `VoiceManager` Class

### What
Update VoiceManager to use `kMaxVoices`, add global timestamp, and add state query methods.

### File to Modify

**`src/core/VoiceManager.h`** — VoiceManager class modifications:

### Changes

1. **Change voice count** (line 290):
   ```cpp
   // BEFORE:
   static constexpr int kNumVoices = 8;
   // AFTER:
   static constexpr int kNumVoices = kMaxVoices;
   ```

2. **Add global timestamp** (new private field):
   ```cpp
   uint64_t mGlobalTimestamp = 0;
   ```

3. **Modify `Init()`** — pass voiceID to each voice:
   ```cpp
   void Init(double sampleRate) {
       for (int i = 0; i < kNumVoices; i++) {
           mVoices[i].Init(sampleRate, static_cast<uint8_t>(i));
       }
   }
   ```

4. **Modify `Reset()`** — same pattern:
   ```cpp
   void Reset() {
       mGlobalTimestamp = 0;
       for (int i = 0; i < kNumVoices; i++) {
           mVoices[i].Init(44100.0, static_cast<uint8_t>(i));
       }
   }
   ```

5. **Modify `OnNoteOn()`** — pass timestamp:
   ```cpp
   void OnNoteOn(int note, int velocity) {
       Voice *voice = FindFreeVoice();
       if (!voice) {
           voice = FindVoiceToSteal();
       }
       if (voice) {
           voice->NoteOn(note, velocity, ++mGlobalTimestamp);
       }
   }
   ```

6. **Add new query methods**:
   ```cpp
   // Returns array of render states for all voices (for UI display)
   std::array<VoiceRenderState, kMaxVoices> GetVoiceStates() const {
       std::array<VoiceRenderState, kMaxVoices> states;
       for (int i = 0; i < kNumVoices; i++) {
           states[i] = mVoices[i].GetRenderState();
       }
       return states;
   }

   // Returns count of unique held notes (for TheoryEngine). Writes notes into buf.
   int GetHeldNotes(std::array<int, kMaxVoices>& buf) const {
       int count = 0;
       for (const auto& voice : mVoices) {
           if (voice.IsActive() && voice.GetNote() >= 0) {
               int note = voice.GetNote();
               // Deduplicate (unison voices play same note)
               bool found = false;
               for (int j = 0; j < count; j++) {
                   if (buf[j] == note) { found = true; break; }
               }
               if (!found && count < kMaxVoices) {
                   buf[count++] = note;
               }
           }
       }
       return count;
   }
   ```

7. **Update headroom scaling in `Process()`** (line 334):
   ```cpp
   // BEFORE:
   return sum * 0.25; // Headroom scaling ... for 8 voices
   // AFTER:
   return sum * (1.0 / std::sqrt(static_cast<double>(kNumVoices)));
   ```

### Acceptance Criteria
- [ ] `kNumVoices` equals `kMaxVoices` (16 by default)
- [ ] Global timestamp increments on each OnNoteOn
- [ ] `GetVoiceStates()` returns correct render states
- [ ] `GetHeldNotes()` deduplicates same pitch across voices
- [ ] Headroom scales correctly for 16 voices

---

## Task 1.6: Update Existing Tests for `kMaxVoices`

### What
The existing `Test_VoiceManager_Poly.cpp` fills 8 voices to trigger stealing. With 16 voices, the steal test needs to fill all 16.

### File to Modify

**`tests/unit/Test_VoiceManager_Poly.cpp`**

In the "Voice Stealing Logic" section (lines 42-56):
- Change `for (int i = 3; i <= 8; i++)` → `for (int i = 3; i <= 16; i++)` (fill remaining 14 voices)
- Change `REQUIRE(vm.GetActiveVoiceCount() == 8)` → `REQUIRE(vm.GetActiveVoiceCount() == 16)` (or use `kMaxVoices`)
- Change comments about "8 voices" to reference `kMaxVoices`
- The 17th note (`vm.OnNoteOn(17, 100)`) should trigger stealing of Note 1 (oldest)

### Acceptance Criteria
- [ ] Steal test fills all 16 voices before triggering steal
- [ ] Test still validates oldest-voice stealing behavior
- [ ] All existing assertions updated for new voice count

---

## Task 1.7: Write Voice State Machine Tests

### What
New test file validating voice state transitions.

### File to Create

**`tests/unit/Test_VoiceStateMachine.cpp`**

```cpp
#include "../../src/core/VoiceManager.h"
#include "catch.hpp"

TEST_CASE("Voice starts in Idle state", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    REQUIRE(voice.IsActive() == false);
}

TEST_CASE("Voice transitions Idle -> Attack on NoteOn", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    REQUIRE(voice.IsActive() == true);
    REQUIRE(voice.GetNote() == 60);
    REQUIRE(voice.GetTimestamp() == 1);
}

TEST_CASE("Voice transitions Attack -> Release on NoteOff", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.NoteOff();
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Release);
}

TEST_CASE("Voice transitions Release -> Idle when envelope finishes", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.NoteOff();
    // Process enough samples for envelope to fully release
    for (int i = 0; i < 48000; i++) {
        voice.Process();
    }
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    REQUIRE(voice.IsActive() == false);
}

TEST_CASE("Voice StartSteal triggers Stolen state with fade-out", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.StartSteal();
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Stolen);
}

TEST_CASE("Stolen voice completes fade in ~2ms", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    // Process a few samples so voice has some output
    for (int i = 0; i < 10; i++) voice.Process();

    voice.StartSteal();
    // 2ms at 48kHz = 96 samples
    int samples = 0;
    while (voice.GetVoiceState() == PolySynthCore::VoiceState::Stolen && samples < 200) {
        voice.Process();
        samples++;
    }
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    // Should complete within 2ms (96 samples) + small margin
    REQUIRE(samples <= 100);
    REQUIRE(samples >= 90);  // Not too fast
}

TEST_CASE("Re-trigger: NoteOn during Attack transitions back to Attack", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    // Retrigger with different note
    voice.NoteOn(64, 80, 2);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    REQUIRE(voice.GetNote() == 64);
    REQUIRE(voice.GetTimestamp() == 2);
}

TEST_CASE("VoiceManager timestamp increments on each NoteOn", "[VoiceStateMachine]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(64, 100);
    vm.OnNoteOn(67, 100);

    // Check that timestamps are monotonically increasing
    auto states = vm.GetVoiceStates();
    uint64_t prevTimestamp = 0;
    int activeCount = 0;
    for (const auto& s : states) {
        if (s.state != PolySynthCore::VoiceState::Idle) {
            REQUIRE(s.state == PolySynthCore::VoiceState::Attack);
            activeCount++;
        }
    }
    REQUIRE(activeCount == 3);
}

TEST_CASE("GetRenderState reflects current voice state", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 5);

    auto rs = voice.GetRenderState();
    REQUIRE(rs.voiceID == 5);
    REQUIRE(rs.state == PolySynthCore::VoiceState::Idle);
    REQUIRE(rs.note == -1);

    voice.NoteOn(72, 127, 42);
    rs = voice.GetRenderState();
    REQUIRE(rs.state == PolySynthCore::VoiceState::Attack);
    REQUIRE(rs.note == 72);
    REQUIRE(rs.velocity == 127);
    REQUIRE(rs.voiceID == 5);
}

TEST_CASE("GetHeldNotes deduplicates same pitch across voices", "[VoiceStateMachine]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    // Same note triggered twice (simulating unison, future feature)
    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(60, 100);  // Second voice with same note
    vm.OnNoteOn(64, 100);

    std::array<int, PolySynthCore::kMaxVoices> notes{};
    int count = vm.GetHeldNotes(notes);

    REQUIRE(count == 2);  // 60 and 64 (60 deduplicated)
    // Verify both notes are present
    bool has60 = false, has64 = false;
    for (int i = 0; i < count; i++) {
        if (notes[i] == 60) has60 = true;
        if (notes[i] == 64) has64 = true;
    }
    REQUIRE(has60);
    REQUIRE(has64);
}
```

### Add to `tests/CMakeLists.txt`

Add `unit/Test_VoiceStateMachine.cpp` to `UNIT_TEST_SOURCES`.

### Acceptance Criteria
- [ ] All 10 test cases pass
- [ ] No existing tests broken

---

## Task Order

Execute tasks in this order:
1. **1.1** — Create SEA_Util library structure
2. **1.2** — Write SPSCRingBuffer tests, then implement
3. **1.3** — Add types to `types.h`
4. **1.4** — Write voice state machine tests (Task 1.7), then implement state machine in Voice
5. **1.5** — Update VoiceManager (kMaxVoices, timestamp, queries)
6. **1.6** — Update existing tests for 16 voices
7. Run all tests: `cd tests/build && cmake .. && make -j && ./run_tests`

## Definition of Done

- [ ] All existing tests pass (updated for kMaxVoices = 16)
- [ ] 7 new SPSCRingBuffer tests pass
- [ ] 10 new VoiceStateMachine tests pass
- [ ] `static_assert(is_aggregate_v<SynthState>)` still compiles (SynthState not modified in Sprint 1)
- [ ] Golden masters unchanged: `python3 scripts/golden_master.py --verify`
- [ ] No iPlug2 headers in core or library files
- [ ] SEA_Util README.md documents library conventions
- [ ] Code compiles cleanly with `-std=c++17 -Wall -Wextra`

## Key File References

| File | Purpose |
|------|---------|
| `src/core/VoiceManager.h` | Voice class (lines 16-286) + VoiceManager (lines 288-499) |
| `src/core/types.h` | Foundation types — add enums and kMaxVoices here |
| `tests/CMakeLists.txt` | Test build — add SEA_Util and new test files |
| `libs/SEA_DSP/CMakeLists.txt` | Pattern to follow for SEA_Util CMakeLists |
| `tests/unit/Test_VoiceManager_Poly.cpp` | Existing test to update for 16 voices |
