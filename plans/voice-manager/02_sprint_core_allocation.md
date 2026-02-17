# Sprint 2: Core Allocation Logic — VoiceAllocator + Wiring

**Depends on:** Sprint 1
**Blocks:** Sprint 4
**Can run in parallel with:** Sprint 3
**Approach:** TDD — write all tests first, then implement until green

---

## Task 2.1: Create `sea::VoiceAllocator` in SEA_Util

### What
A generic, reusable voice slot allocator. It manages allocation strategy, polyphony limits, steal victim selection, sustain pedal, and unison grouping — but never touches audio samples. It operates on slot indices and delegates actual NoteOn/NoteOff/kill to the caller.

### File to Create

**`libs/SEA_Util/include/sea_util/sea_voice_allocator.h`**

### Enums (in `sea::` namespace)
```cpp
namespace sea {

enum class AllocationMode : uint8_t {
    ResetMode = 0,  // Always scan from slot 0 (lowest available index)
    CycleMode       // Round-robin: continue from last allocated slot
};

enum class StealPriority : uint8_t {
    Oldest = 0,         // Steal slot with lowest timestamp
    LowestPitch,        // Steal slot with lowest pitch
    LowestAmplitude     // Steal slot with lowest amplitude (future — requires amplitude getter)
};

} // namespace sea
```

### Template Class
```cpp
namespace sea {

// UnisonVoiceInfo: detune and pan for each unison voice
struct UnisonVoiceInfo {
    double detuneCents = 0.0;  // Detune in cents from base pitch
    double panPosition = 0.0;  // -1.0 (left) to +1.0 (right)
};

template <typename Slot, size_t MaxSlots>
class VoiceAllocator {
    // Trait requirements (checked at compile time via static_assert in methods):
    // bool     Slot::IsActive() const;
    // uint64_t Slot::GetTimestamp() const;
    // float    Slot::GetPitch() const;
    // void     Slot::StartSteal();

public:
    VoiceAllocator() = default;

    // --- Configuration ---
    void SetPolyphonyLimit(int limit);      // Clamp to [1, MaxSlots]
    int GetPolyphonyLimit() const;
    void SetAllocationMode(AllocationMode mode);
    void SetStealPriority(StealPriority priority);
    void SetUnisonCount(int count);         // Clamp to [1, 8]
    void SetUnisonSpread(double spread);    // 0.0–1.0 (maps to cents)
    void SetStereoSpread(double spread);    // 0.0–1.0

    // --- Allocation ---
    // Returns slot index or -1 if no free slot within polyphony limit
    int AllocateSlot(Slot* slots);

    // Returns index of best steal victim or -1 if all slots empty
    // Does NOT call StartSteal — caller decides what to do
    int FindStealVictim(Slot* slots);

    // --- Sustain Pedal ---
    void OnSustainPedal(bool down);
    bool IsSustainDown() const;
    bool ShouldHold(int note) const;        // Returns true if sustain is down
    void MarkSustained(int note);           // Mark note as sustained (will release on pedal up)
    // Fills releasedNotes with notes to release, returns count
    int ReleaseSustainedNotes(int* releasedNotes, int maxCount);

    // --- Unison ---
    int GetUnisonCount() const;
    UnisonVoiceInfo GetUnisonVoiceInfo(int unisonIndex) const;

    // --- Polyphony Limit Enforcement ---
    // Finds excess active voices beyond polyphony limit
    // Fills killIndices with slot indices to kill, returns count
    int EnforcePolyphonyLimit(Slot* slots, int* killIndices, int maxKill);

private:
    AllocationMode mAllocationMode = AllocationMode::ResetMode;
    StealPriority mStealPriority = StealPriority::Oldest;
    int mPolyphonyLimit = static_cast<int>(MaxSlots);
    int mUnisonCount = 1;
    double mUnisonSpread = 0.0;    // 0.0–1.0
    double mStereoSpread = 0.0;    // 0.0–1.0
    int mRoundRobinIndex = 0;       // For CycleMode

    bool mSustainDown = false;
    std::array<bool, 128> mSustainedNotes{};  // MIDI notes 0-127
};

} // namespace sea
```

### Implementation Notes

**`AllocateSlot(Slot* slots)`**:
- ResetMode: scan from index 0, return first inactive slot within polyphony limit
- CycleMode: scan from `mRoundRobinIndex`, wrap around, return first inactive slot, update `mRoundRobinIndex`
- Count active slots first. If `activeCount >= mPolyphonyLimit`, return -1

**`FindStealVictim(Slot* slots)`**:
- Scan all active slots within polyphony limit range
- Oldest: pick slot with lowest timestamp
- LowestPitch: pick slot with lowest `GetPitch()` value
- LowestAmplitude: (future) for now, fall back to Oldest
- Return -1 if no active slots found

**`GetUnisonVoiceInfo(int unisonIndex)`**:
- For count=1: `{0.0, 0.0}` (no detune, center pan)
- For count=N: spread voices symmetrically
  - Detune: linearly from `-maxDetune` to `+maxDetune`, where `maxDetune = mUnisonSpread * 50.0` cents
  - Pan: linearly from `-mStereoSpread` to `+mStereoSpread`
  - Formula for index `i` of `N`:
    - `fraction = (N == 1) ? 0.0 : (2.0 * i / (N - 1)) - 1.0`  // -1.0 to +1.0
    - `detuneCents = fraction * mUnisonSpread * 50.0`
    - `panPosition = fraction * mStereoSpread`

**`ReleaseSustainedNotes()`**:
- Iterate `mSustainedNotes`, collect all marked notes into `releasedNotes`
- Clear all marks
- Return count

**`EnforcePolyphonyLimit()`**:
- Count active slots. If `activeCount <= mPolyphonyLimit`, return 0
- Find `activeCount - mPolyphonyLimit` victims using steal priority
- Fill `killIndices`, call `slots[idx].StartSteal()` on each
- Return count

### Only Includes
- `<array>`, `<cstdint>`, `<algorithm>` — no DSP, no iPlug2, no platform headers

### Acceptance Criteria
- [ ] Template compiles with MockSlot and with real Voice
- [ ] Static asserts verify trait requirements
- [ ] No heap allocations

---

## Task 2.2: Add Enum Aliases to `src/core/types.h`

### What
Add convenience aliases so PolySynth code can use `AllocationMode` and `StealPriority` without the `sea::` prefix.

### File to Modify

**`src/core/types.h`** — Add after the VoiceEvent struct:

```cpp
// Convenience aliases for sea:: enums (defined in sea_voice_allocator.h)
// These are used by SynthState and PolySynth params
// Note: included via VoiceManager.h which pulls in sea_voice_allocator.h
```

The actual `using` declarations will be added when VoiceManager.h includes the allocator. For now, SynthState uses `int` fields (0, 1, 2) which map to the enum values.

### Acceptance Criteria
- [ ] SynthState fields remain `int` (aggregate compatible)
- [ ] Mapping documented in comments

---

## Task 2.3: Add New Fields to `src/core/SynthState.h`

### What
Add 5 new fields for voice allocation configuration.

### File to Modify

**`src/core/SynthState.h`** — Add after `glideTime` (line 18):

```cpp
  // --- Voice Allocation (Sprint 2) --------
  int allocationMode = 0;     // 0=Reset (scan from 0), 1=Cycle (round-robin)
  int stealPriority = 0;      // 0=Oldest, 1=LowestPitch, 2=LowestAmplitude
  int unisonCount = 1;        // 1-8 voices per note
  double unisonSpread = 0.0;  // 0.0-1.0 (detune amount)
  double stereoSpread = 0.0;  // 0.0-1.0 (stereo width)
```

### Important
- All fields have inline defaults → `Reset()` via `*this = SynthState{}` still works
- `static_assert(is_aggregate_v<SynthState>)` must still compile
- Default values preserve existing behavior (Reset mode, Oldest steal, 1 unison voice, no spread)

### Acceptance Criteria
- [ ] `static_assert(is_aggregate_v<SynthState>)` compiles
- [ ] `SynthState{}.allocationMode == 0`
- [ ] `SynthState{}.unisonCount == 1`
- [ ] `Reset()` restores all defaults

---

## Task 2.4: Update `src/core/PresetManager.cpp`

### What
Add serialization for the 5 new SynthState fields so they persist across presets.

### File to Modify

**`src/core/PresetManager.cpp`** — In the `Serialize()` function, after the existing Global section (after `SERIALIZE(j, state, glideTime);`, around line 27):

```cpp
  // Voice Allocation
  SERIALIZE(j, state, allocationMode);
  SERIALIZE(j, state, stealPriority);
  SERIALIZE(j, state, unisonCount);
  SERIALIZE(j, state, unisonSpread);
  SERIALIZE(j, state, stereoSpread);
```

In the `Deserialize()` function, at the corresponding location:
```cpp
  // Voice Allocation
  DESERIALIZE(j, state, allocationMode);
  DESERIALIZE(j, state, stealPriority);
  DESERIALIZE(j, state, unisonCount);
  DESERIALIZE(j, state, unisonSpread);
  DESERIALIZE(j, state, stereoSpread);
```

### Note
The `DESERIALIZE` macro uses `if (j.contains(#field))` which provides backward compatibility — old presets without these fields will use the defaults from `SynthState{}`.

### Acceptance Criteria
- [ ] New fields appear in JSON output
- [ ] Old presets without new fields load without error (defaults used)
- [ ] Round-trip: serialize → deserialize produces identical state

---

## Task 2.5: Update SynthState Tests

### File to Modify

**`tests/unit/Test_SynthState.cpp`** — Add to the existing Reset test case:

```cpp
CATCH_CHECK(state.allocationMode == 0);
CATCH_CHECK(state.stealPriority == 0);
CATCH_CHECK(state.unisonCount == 1);
CATCH_CHECK(state.unisonSpread == 0.0);
CATCH_CHECK(state.stereoSpread == 0.0);
```

Add to the round-trip serialization test:
```cpp
state.allocationMode = 1;
state.stealPriority = 2;
state.unisonCount = 4;
state.unisonSpread = 0.75;
state.stereoSpread = 0.5;
```

And verify after deserialization:
```cpp
CATCH_CHECK(loaded.allocationMode == 1);
CATCH_CHECK(loaded.stealPriority == 2);
CATCH_CHECK(loaded.unisonCount == 4);
CATCH_CHECK(loaded.unisonSpread == Approx(0.75));
CATCH_CHECK(loaded.stereoSpread == Approx(0.5));
```

### Acceptance Criteria
- [ ] Reset test verifies all 5 new fields
- [ ] Round-trip test covers non-default values

---

## Task 2.6: Refactor VoiceManager to Compose `sea::VoiceAllocator`

### What
Replace VoiceManager's `FindFreeVoice()` and `FindVoiceToSteal()` with the generic allocator. Add sustain pedal support and unison allocation.

### File to Modify

**`src/core/VoiceManager.h`**

### Add Include
```cpp
#include <sea_util/sea_voice_allocator.h>
```

### New Private Member
```cpp
sea::VoiceAllocator<Voice, kMaxVoices> mAllocator;
```

### Remove
- `FindFreeVoice()` — replaced by `mAllocator.AllocateSlot()`
- `FindVoiceToSteal()` — replaced by `mAllocator.FindStealVictim()`

### Modified `OnNoteOn()`
```cpp
void OnNoteOn(int note, int velocity) {
    int unisonCount = mAllocator.GetUnisonCount();
    for (int u = 0; u < unisonCount; u++) {
        int idx = mAllocator.AllocateSlot(mVoices.data());
        if (idx < 0) {
            idx = mAllocator.FindStealVictim(mVoices.data());
        }
        if (idx < 0) break;  // No voice available

        mVoices[idx].NoteOn(note, velocity, ++mGlobalTimestamp);

        // Apply unison detune and pan
        auto info = mAllocator.GetUnisonVoiceInfo(u);
        if (info.detuneCents != 0.0) {
            mVoices[idx].ApplyDetuneCents(info.detuneCents);
        }
        mVoices[idx].SetPanPosition(static_cast<float>(info.panPosition));
    }
}
```

### Modified `OnNoteOff()`
```cpp
void OnNoteOff(int note) {
    if (mAllocator.ShouldHold(note)) {
        mAllocator.MarkSustained(note);
        return;
    }
    // Release all voices playing this note
    for (auto& voice : mVoices) {
        if (voice.IsActive() && voice.GetNote() == note) {
            voice.NoteOff();
        }
    }
}
```

### New Methods
```cpp
void OnSustainPedal(bool down) {
    mAllocator.OnSustainPedal(down);
    if (!down) {
        // Release all sustained notes
        int releasedNotes[128];
        int count = mAllocator.ReleaseSustainedNotes(releasedNotes, 128);
        for (int i = 0; i < count; i++) {
            for (auto& voice : mVoices) {
                if (voice.IsActive() && voice.GetNote() == releasedNotes[i]) {
                    voice.NoteOff();
                }
            }
        }
    }
}

// Configuration setters (delegate to allocator)
void SetPolyphonyLimit(int limit) { mAllocator.SetPolyphonyLimit(limit); }
void SetAllocationMode(int mode) {
    mAllocator.SetAllocationMode(static_cast<sea::AllocationMode>(mode));
}
void SetStealPriority(int priority) {
    mAllocator.SetStealPriority(static_cast<sea::StealPriority>(priority));
}
void SetUnisonCount(int count) { mAllocator.SetUnisonCount(count); }
void SetUnisonSpread(double spread) { mAllocator.SetUnisonSpread(spread); }
void SetStereoSpread(double spread) { mAllocator.SetStereoSpread(spread); }
```

### New Voice Method: `ApplyDetuneCents(double cents)`
Add to Voice class:
```cpp
void ApplyDetuneCents(double cents) {
    double factor = std::pow(2.0, cents / 1200.0);
    mFreq *= factor;
    mCurrentPitch = static_cast<float>(mFreq);
    mOscA.SetFrequency(mFreq);
    mOscB.SetFrequency(mFreq * std::pow(2.0, mDetuneB / 1200.0));
}
```

### Acceptance Criteria
- [ ] Existing OnNoteOn/OnNoteOff behavior preserved with default settings
- [ ] Polyphony limit enforcement works
- [ ] Sustain pedal holds and releases correctly
- [ ] Unison allocates multiple voices per note
- [ ] All existing tests pass with default configuration

---

## Task 2.7: Write VoiceAllocator Unit Tests (MockSlot)

### What
Test the allocator in isolation using a minimal MockSlot struct.

### File to Create

**`tests/unit/Test_VoiceAllocator.cpp`**

```cpp
#include <sea_util/sea_voice_allocator.h>
#include "catch.hpp"

// Minimal mock that satisfies VoiceAllocator trait requirements
struct MockSlot {
    bool active = false;
    uint64_t timestamp = 0;
    float pitch = 440.0f;
    bool stolen = false;

    bool IsActive() const { return active; }
    uint64_t GetTimestamp() const { return timestamp; }
    float GetPitch() const { return pitch; }
    void StartSteal() { stolen = true; active = false; }
};

TEST_CASE("VoiceAllocator ResetMode: allocates lowest available", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 8> alloc;
    MockSlot slots[8] = {};

    int idx = alloc.AllocateSlot(slots);
    REQUIRE(idx == 0);
    slots[idx].active = true;

    idx = alloc.AllocateSlot(slots);
    REQUIRE(idx == 1);
    slots[idx].active = true;

    // Free slot 0
    slots[0].active = false;
    idx = alloc.AllocateSlot(slots);
    REQUIRE(idx == 0);  // Reset mode goes back to 0
}

TEST_CASE("VoiceAllocator CycleMode: round-robin allocation", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 4> alloc;
    alloc.SetAllocationMode(sea::AllocationMode::CycleMode);
    MockSlot slots[4] = {};

    int idx0 = alloc.AllocateSlot(slots);
    slots[idx0].active = true;
    int idx1 = alloc.AllocateSlot(slots);
    slots[idx1].active = true;
    int idx2 = alloc.AllocateSlot(slots);
    slots[idx2].active = true;

    // Free slot 0
    slots[0].active = false;
    // Cycle mode should NOT return 0, should continue from where it left off
    int idx3 = alloc.AllocateSlot(slots);
    slots[idx3].active = true;
    REQUIRE(idx3 == 3);  // Next in cycle

    // Now all 4 active. Free slot 1.
    slots[1].active = false;
    int idx4 = alloc.AllocateSlot(slots);
    // Wraps around, finds slot 1
    REQUIRE(idx4 == 1);
}

TEST_CASE("VoiceAllocator: Oldest stealing", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 4> alloc;
    alloc.SetStealPriority(sea::StealPriority::Oldest);
    MockSlot slots[4] = {};

    // Set up 4 active slots with different timestamps
    for (int i = 0; i < 4; i++) {
        slots[i].active = true;
        slots[i].timestamp = (i + 1) * 100;  // 100, 200, 300, 400
    }

    int victim = alloc.FindStealVictim(slots);
    REQUIRE(victim == 0);  // Oldest (timestamp=100)
}

TEST_CASE("VoiceAllocator: LowestPitch stealing", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 4> alloc;
    alloc.SetStealPriority(sea::StealPriority::LowestPitch);
    MockSlot slots[4] = {};

    slots[0] = {true, 100, 440.0f, false};
    slots[1] = {true, 200, 220.0f, false};  // Lowest pitch
    slots[2] = {true, 300, 880.0f, false};
    slots[3] = {true, 400, 330.0f, false};

    int victim = alloc.FindStealVictim(slots);
    REQUIRE(victim == 1);  // Lowest pitch (220Hz)
}

TEST_CASE("VoiceAllocator: polyphony limit respected", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 8> alloc;
    alloc.SetPolyphonyLimit(4);
    MockSlot slots[8] = {};

    // Allocate up to limit
    for (int i = 0; i < 4; i++) {
        int idx = alloc.AllocateSlot(slots);
        REQUIRE(idx >= 0);
        slots[idx].active = true;
    }

    // 5th allocation should fail
    int idx = alloc.AllocateSlot(slots);
    REQUIRE(idx == -1);
}

TEST_CASE("VoiceAllocator: unison voice info symmetric", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 16> alloc;
    alloc.SetUnisonSpread(1.0);  // Max spread = 50 cents
    alloc.SetStereoSpread(1.0);

    // Unison 1: no detune, center pan
    alloc.SetUnisonCount(1);
    auto info = alloc.GetUnisonVoiceInfo(0);
    REQUIRE(info.detuneCents == Approx(0.0));
    REQUIRE(info.panPosition == Approx(0.0));

    // Unison 2: symmetric L/R
    alloc.SetUnisonCount(2);
    auto left = alloc.GetUnisonVoiceInfo(0);
    auto right = alloc.GetUnisonVoiceInfo(1);
    REQUIRE(left.detuneCents == Approx(-right.detuneCents));
    REQUIRE(left.panPosition == Approx(-right.panPosition));

    // Unison 3: center voice has zero detune
    alloc.SetUnisonCount(3);
    auto center = alloc.GetUnisonVoiceInfo(1);
    REQUIRE(center.detuneCents == Approx(0.0));
    REQUIRE(center.panPosition == Approx(0.0));

    // Unison 4: symmetric pairs
    alloc.SetUnisonCount(4);
    auto v0 = alloc.GetUnisonVoiceInfo(0);
    auto v3 = alloc.GetUnisonVoiceInfo(3);
    REQUIRE(v0.detuneCents == Approx(-v3.detuneCents));
    REQUIRE(v0.panPosition == Approx(-v3.panPosition));
}

TEST_CASE("VoiceAllocator: sustain pedal lifecycle", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 8> alloc;

    REQUIRE_FALSE(alloc.IsSustainDown());
    REQUIRE_FALSE(alloc.ShouldHold(60));

    alloc.OnSustainPedal(true);
    REQUIRE(alloc.IsSustainDown());
    REQUIRE(alloc.ShouldHold(60));

    // Mark a note as sustained
    alloc.MarkSustained(60);
    alloc.MarkSustained(64);

    // Release pedal
    alloc.OnSustainPedal(false);
    REQUIRE_FALSE(alloc.IsSustainDown());

    // Get released notes
    int released[128];
    int count = alloc.ReleaseSustainedNotes(released, 128);
    REQUIRE(count == 2);

    // Verify notes
    bool has60 = false, has64 = false;
    for (int i = 0; i < count; i++) {
        if (released[i] == 60) has60 = true;
        if (released[i] == 64) has64 = true;
    }
    REQUIRE(has60);
    REQUIRE(has64);
}

TEST_CASE("VoiceAllocator: EnforcePolyphonyLimit kills excess", "[VoiceAllocator]") {
    sea::VoiceAllocator<MockSlot, 8> alloc;
    alloc.SetPolyphonyLimit(8);
    alloc.SetStealPriority(sea::StealPriority::Oldest);
    MockSlot slots[8] = {};

    // Activate 6 voices
    for (int i = 0; i < 6; i++) {
        slots[i].active = true;
        slots[i].timestamp = (i + 1) * 10;
    }

    // Reduce limit to 4
    alloc.SetPolyphonyLimit(4);

    int killIndices[8];
    int killCount = alloc.EnforcePolyphonyLimit(slots, killIndices, 8);
    REQUIRE(killCount == 2);  // Need to kill 2 excess voices

    // Should have killed the oldest ones
    for (int i = 0; i < killCount; i++) {
        REQUIRE(slots[killIndices[i]].stolen == true);
    }
}
```

### Add to `tests/CMakeLists.txt`
Add `unit/Test_VoiceAllocator.cpp` to `UNIT_TEST_SOURCES`.

---

## Task 2.8: Write VoiceAllocation Integration Tests

### What
Test the allocator integrated with the real Voice class through VoiceManager.

### File to Create

**`tests/unit/Test_VoiceAllocation.cpp`**

```cpp
#include "../../src/core/VoiceManager.h"
#include "catch.hpp"

TEST_CASE("Polyphony limit of 4 respected", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);
    vm.SetPolyphonyLimit(4);

    for (int i = 0; i < 5; i++) {
        vm.OnNoteOn(60 + i, 100);
    }
    // 5th note should steal — total active still 4
    REQUIRE(vm.GetActiveVoiceCount() == 4);
}

TEST_CASE("Polyphony limit reduction kills excess", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    // Start 8 voices
    for (int i = 0; i < 8; i++) {
        vm.OnNoteOn(60 + i, 100);
    }
    REQUIRE(vm.GetActiveVoiceCount() == 8);

    // Reduce limit to 4 — excess voices should be killed
    vm.SetPolyphonyLimit(4);
    // Process a few samples to let stolen voices fade out
    for (int i = 0; i < 200; i++) vm.Process();

    REQUIRE(vm.GetActiveVoiceCount() <= 4);
}

TEST_CASE("Mono mode: polyphony limit 1", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);
    vm.SetPolyphonyLimit(1);

    vm.OnNoteOn(60, 100);
    REQUIRE(vm.GetActiveVoiceCount() == 1);

    vm.OnNoteOn(64, 100);
    // Should steal the first voice
    REQUIRE(vm.GetActiveVoiceCount() == 1);
    REQUIRE(vm.IsNoteActive(64));
}

TEST_CASE("Unison count=3 allocates 3 voices per note", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);
    vm.SetUnisonCount(3);

    vm.OnNoteOn(60, 100);
    REQUIRE(vm.GetActiveVoiceCount() == 3);

    // All 3 voices should be playing note 60
    int noteCount = 0;
    auto states = vm.GetVoiceStates();
    for (const auto& s : states) {
        if (s.note == 60) noteCount++;
    }
    REQUIRE(noteCount == 3);
}

TEST_CASE("NoteOff releases all unison voices for note", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);
    vm.SetUnisonCount(3);

    vm.OnNoteOn(60, 100);
    REQUIRE(vm.GetActiveVoiceCount() == 3);

    vm.OnNoteOff(60);
    // All 3 voices should be in Release state
    auto states = vm.GetVoiceStates();
    int releasingCount = 0;
    for (const auto& s : states) {
        if (s.state == PolySynthCore::VoiceState::Release) releasingCount++;
    }
    REQUIRE(releasingCount == 3);
}

TEST_CASE("Sustain pedal holds and releases through VoiceManager", "[VoiceAllocation]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    vm.OnSustainPedal(true);
    vm.OnNoteOn(60, 100);
    vm.OnNoteOff(60);  // Should be held by sustain

    // Voice should still be active (not released)
    REQUIRE(vm.IsNoteActive(60));

    vm.OnSustainPedal(false);  // Release sustained notes

    // Voice should now be in Release
    auto states = vm.GetVoiceStates();
    bool foundRelease = false;
    for (const auto& s : states) {
        if (s.note == 60 && s.state == PolySynthCore::VoiceState::Release) {
            foundRelease = true;
        }
    }
    REQUIRE(foundRelease);
}
```

### Add to `tests/CMakeLists.txt`
Add `unit/Test_VoiceAllocation.cpp` to `UNIT_TEST_SOURCES`.

---

## Task Order

Execute tasks in this order:
1. **2.7** — Write VoiceAllocator unit tests (MockSlot) — **tests first**
2. **2.8** — Write VoiceAllocation integration tests — **tests first**
3. **2.1** — Implement `sea::VoiceAllocator` until tests pass
4. **2.3** — Add SynthState fields
5. **2.5** — Update SynthState tests, verify pass
6. **2.4** — Update PresetManager serialization
7. **2.2** — Add type aliases/docs
8. **2.6** — Refactor VoiceManager to compose allocator
9. Run all tests: `cd tests/build && cmake .. && make -j && ./run_tests`

## Definition of Done

- [ ] All Sprint 1 tests still pass
- [ ] 8 new VoiceAllocator unit tests pass (MockSlot)
- [ ] 6 new VoiceAllocation integration tests pass (real Voice)
- [ ] SynthState round-trip covers 5 new fields
- [ ] Default settings (limit=16, Reset, unison=1) produce identical behavior to Sprint 1
- [ ] `static_assert(is_aggregate_v<SynthState>)` compiles
- [ ] Golden masters unchanged: `python3 scripts/golden_master.py --verify`
- [ ] No iPlug2 headers in core or library files

## Key File References

| File | Purpose |
|------|---------|
| `src/core/VoiceManager.h` | Refactor target — compose VoiceAllocator |
| `src/core/SynthState.h` | Add 5 new fields (line ~18) |
| `src/core/PresetManager.cpp` | Add SERIALIZE/DESERIALIZE for new fields |
| `src/core/types.h` | Enum aliases for AllocationMode, StealPriority |
| `tests/unit/Test_SynthState.cpp` | Update Reset + round-trip tests |
| `libs/SEA_Util/include/sea_util/sea_voice_allocator.h` | New file to create |
