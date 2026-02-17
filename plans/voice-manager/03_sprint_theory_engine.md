# Sprint 3: Theory Engine — Chord Detection

**Depends on:** Sprint 1 (for SEA_Util library structure)
**Blocks:** Sprint 4 (UI chord display)
**Can run in parallel with:** Sprint 2
**Approach:** TDD — write all tests first, then implement until green

---

## Task 3.1: Write Theory Engine Tests

### What
Write comprehensive chord detection tests before implementing the engine. These tests define the expected behavior.

### File to Create FIRST

**`tests/unit/Test_TheoryEngine.cpp`**

```cpp
#include <sea_util/sea_theory_engine.h>
#include "catch.hpp"

// Helper: format chord result to string
static std::string ChordStr(const int* notes, int count) {
    auto result = sea::TheoryEngine::Analyze(notes, count);
    if (!result.valid) return "";
    char buf[32];
    sea::TheoryEngine::FormatChordName(result, buf, sizeof(buf));
    return std::string(buf);
}

// ============ Basic Triads ============

TEST_CASE("TheoryEngine: C major triad", "[TheoryEngine]") {
    int notes[] = {60, 64, 67};  // C4, E4, G4
    REQUIRE(ChordStr(notes, 3) == "Cmaj");
}

TEST_CASE("TheoryEngine: A minor triad", "[TheoryEngine]") {
    int notes[] = {57, 60, 64};  // A3, C4, E4
    REQUIRE(ChordStr(notes, 3) == "Amin");
}

TEST_CASE("TheoryEngine: B diminished triad", "[TheoryEngine]") {
    int notes[] = {59, 62, 65};  // B3, D4, F4
    REQUIRE(ChordStr(notes, 3) == "Bdim");
}

TEST_CASE("TheoryEngine: C augmented triad", "[TheoryEngine]") {
    int notes[] = {60, 64, 68};  // C4, E4, G#4
    REQUIRE(ChordStr(notes, 3) == "Caug");
}

TEST_CASE("TheoryEngine: D sus2", "[TheoryEngine]") {
    int notes[] = {62, 64, 69};  // D4, E4, A4
    REQUIRE(ChordStr(notes, 3) == "Dsus2");
}

TEST_CASE("TheoryEngine: D sus4", "[TheoryEngine]") {
    int notes[] = {62, 67, 69};  // D4, G4, A4
    REQUIRE(ChordStr(notes, 3) == "Dsus4");
}

// ============ Seventh Chords ============

TEST_CASE("TheoryEngine: C dominant 7", "[TheoryEngine]") {
    int notes[] = {60, 64, 67, 70};  // C4, E4, G4, Bb4
    REQUIRE(ChordStr(notes, 4) == "C7");
}

TEST_CASE("TheoryEngine: C major 7", "[TheoryEngine]") {
    int notes[] = {60, 64, 67, 71};  // C4, E4, G4, B4
    REQUIRE(ChordStr(notes, 4) == "Cmaj7");
}

TEST_CASE("TheoryEngine: C minor 7", "[TheoryEngine]") {
    int notes[] = {60, 63, 67, 70};  // C4, Eb4, G4, Bb4
    REQUIRE(ChordStr(notes, 4) == "Cmin7");
}

TEST_CASE("TheoryEngine: B diminished 7", "[TheoryEngine]") {
    int notes[] = {59, 62, 65, 68};  // B3, D4, F4, Ab4
    REQUIRE(ChordStr(notes, 4) == "Bdim7");
}

TEST_CASE("TheoryEngine: B half-diminished 7", "[TheoryEngine]") {
    int notes[] = {59, 62, 65, 70};  // B3, D4, F4, Bb4
    REQUIRE(ChordStr(notes, 4) == "Bm7b5");
}

// ============ Inversions ============

TEST_CASE("TheoryEngine: C major first inversion", "[TheoryEngine]") {
    int notes[] = {64, 67, 72};  // E4, G4, C5
    auto result = sea::TheoryEngine::Analyze(notes, 3);
    REQUIRE(result.valid);
    REQUIRE(result.root == 0);   // C
    REQUIRE(result.bass == 4);   // E
    REQUIRE(result.isInversion == true);

    char buf[32];
    sea::TheoryEngine::FormatChordName(result, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "Cmaj/E");
}

TEST_CASE("TheoryEngine: C major second inversion", "[TheoryEngine]") {
    int notes[] = {67, 72, 76};  // G4, C5, E5
    auto result = sea::TheoryEngine::Analyze(notes, 3);
    REQUIRE(result.valid);
    REQUIRE(result.root == 0);   // C
    REQUIRE(result.bass == 7);   // G
    REQUIRE(result.isInversion == true);

    char buf[32];
    sea::TheoryEngine::FormatChordName(result, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "Cmaj/G");
}

// ============ Edge Cases ============

TEST_CASE("TheoryEngine: single note is invalid", "[TheoryEngine]") {
    int notes[] = {60};
    auto result = sea::TheoryEngine::Analyze(notes, 1);
    REQUIRE_FALSE(result.valid);
}

TEST_CASE("TheoryEngine: zero notes is invalid", "[TheoryEngine]") {
    auto result = sea::TheoryEngine::Analyze(nullptr, 0);
    REQUIRE_FALSE(result.valid);
}

TEST_CASE("TheoryEngine: two notes (interval) is invalid", "[TheoryEngine]") {
    int notes[] = {60, 64};
    auto result = sea::TheoryEngine::Analyze(notes, 2);
    REQUIRE_FALSE(result.valid);
}

TEST_CASE("TheoryEngine: octave doubling recognized", "[TheoryEngine]") {
    int notes[] = {48, 52, 55, 60};  // C3, E3, G3, C4
    REQUIRE(ChordStr(notes, 4) == "Cmaj");
}

TEST_CASE("TheoryEngine: chromatic cluster is invalid", "[TheoryEngine]") {
    int notes[] = {60, 61, 62};  // C, C#, D
    auto result = sea::TheoryEngine::Analyze(notes, 3);
    REQUIRE_FALSE(result.valid);
}

// ============ All 12 Chromatic Roots ============

TEST_CASE("TheoryEngine: all 12 major triads", "[TheoryEngine]") {
    // Major triad intervals: root, +4, +7
    const char* names[] = {"Cmaj", "C#maj", "Dmaj", "D#maj", "Emaj", "Fmaj",
                           "F#maj", "Gmaj", "G#maj", "Amaj", "A#maj", "Bmaj"};
    for (int root = 0; root < 12; root++) {
        int notes[] = {60 + root, 64 + root, 67 + root};
        INFO("Root: " << root);
        REQUIRE(ChordStr(notes, 3) == names[root]);
    }
}

// ============ Format Function ============

TEST_CASE("TheoryEngine: FormatChordName with invalid result", "[TheoryEngine]") {
    sea::ChordResult invalid{};
    invalid.valid = false;
    char buf[32];
    int len = sea::TheoryEngine::FormatChordName(invalid, buf, sizeof(buf));
    REQUIRE(len == 0);
    REQUIRE(std::string(buf) == "");
}

TEST_CASE("TheoryEngine: FormatChordName buffer size handling", "[TheoryEngine]") {
    int notes[] = {60, 64, 67};
    auto result = sea::TheoryEngine::Analyze(notes, 3);
    // Tiny buffer — should not overflow
    char tiny[4];
    int len = sea::TheoryEngine::FormatChordName(result, tiny, sizeof(tiny));
    // Should truncate but not crash
    REQUIRE(len >= 0);
}
```

### Add to `tests/CMakeLists.txt`
Add `unit/Test_TheoryEngine.cpp` to `UNIT_TEST_SOURCES`.

### Acceptance Criteria
- [ ] Test file compiles (after implementation)
- [ ] All 20+ test cases defined

---

## Task 3.2: Implement `sea::TheoryEngine`

### What
A stateless, all-static chord detection engine using pitch class bitmask matching.

### File to Create

**`libs/SEA_Util/include/sea_util/sea_theory_engine.h`**

### Data Structures
```cpp
namespace sea {

struct ChordResult {
    bool valid = false;
    int root = 0;          // Pitch class 0-11 (0=C, 1=C#, ..., 11=B)
    int bass = 0;          // Pitch class of lowest note
    int quality = 0;       // Index into quality table
    bool isInversion = false;  // true if bass != root
};

class TheoryEngine {
public:
    // Analyze an array of MIDI note numbers. Returns best chord match.
    // Requires at least 3 unique pitch classes for a valid result.
    static ChordResult Analyze(const int* notes, int count);

    // Format chord name into buffer. Returns string length.
    // Examples: "Cmaj", "Amin/C", "G7", "F#dim7"
    static int FormatChordName(const ChordResult& result, char* buf, int bufSize);

private:
    // No instances
    TheoryEngine() = delete;
};

} // namespace sea
```

### Algorithm: `Analyze()`

1. **Validate**: Return invalid if `count < 3` or `notes == nullptr`

2. **Sort and extract pitch classes**:
   ```
   Sort notes ascending
   bass = notes[0] % 12
   Build 12-bit pitch class set (bitmask): for each note, set bit (note % 12)
   Count unique pitch classes — if < 3, return invalid
   ```

3. **Template table** (constexpr array):
   ```
   Index | Quality Name | Intervals (semitones above root)  | Bitmask
   ------|-------------|-------------------------------------|--------
   0     | maj         | {0, 4, 7}                          | 0b000010010001
   1     | min         | {0, 3, 7}                          | 0b000010001001
   2     | dim         | {0, 3, 6}                          | 0b000001001001
   3     | aug         | {0, 4, 8}                          | 0b000100010001
   4     | sus2        | {0, 2, 7}                          | 0b000010000101
   5     | sus4        | {0, 5, 7}                          | 0b000010100001
   6     | 7           | {0, 4, 7, 10}                      | 0b010010010001
   7     | maj7        | {0, 4, 7, 11}                      | 0b100010010001
   8     | min7        | {0, 3, 7, 10}                      | 0b010010001001
   9     | dim7        | {0, 3, 6, 9}                       | 0b001001001001
   10    | m7b5        | {0, 3, 6, 10}                      | 0b010001001001
   ```

4. **Match against templates**:
   ```
   For each of the 12 possible roots (0-11):
       If root pitch class is not in the original bitmask, skip
       Rotate bitmask so root is at bit 0:
           rotated = (bitmask >> root) | (bitmask << (12 - root)) & 0xFFF
       For each template in table:
           Check if template intervals are a subset of rotated bitmask:
               match = (rotated & template) == template
           If match:
               Score = popcount(template)  // More intervals = better match
               Bonus +1 if root == bass (root position preferred)
               Bonus +1 if template has 4+ intervals (7th chords preferred over triads when all notes present)
               Track best score
   ```

5. **Return best match**:
   ```
   result.root = bestRoot
   result.bass = bass
   result.quality = bestQualityIndex
   result.isInversion = (bestRoot != bass)
   result.valid = (bestScore > 0)
   ```

### `FormatChordName()`

```
Note names: {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}
Quality names: {"maj", "min", "dim", "aug", "sus2", "sus4", "7", "maj7", "min7", "dim7", "m7b5"}

Format: "<RootName><QualityName>" or "<RootName><QualityName>/<BassName>"
Example: "Cmaj" or "Cmaj/E"

If !result.valid: write "" to buf, return 0
If result.isInversion: append "/<BassName>"
Use snprintf for safe buffer handling
```

### Only Includes
- `<algorithm>` — for std::sort
- `<array>` — for template table
- `<cstdint>` — for uint16_t bitmask
- `<cstdio>` — for snprintf (FormatChordName only)
- No DSP, no iPlug2, no platform headers

### Acceptance Criteria
- [ ] All basic triad tests pass (major, minor, dim, aug, sus2, sus4)
- [ ] All seventh chord tests pass (dom7, maj7, min7, dim7, m7b5)
- [ ] Inversion detection works (first and second inversion)
- [ ] Edge cases handled (single note, zero notes, clusters)
- [ ] Octave doubling recognized
- [ ] All 12 chromatic roots detected
- [ ] FormatChordName produces correct strings
- [ ] Buffer overflow doesn't crash
- [ ] No heap allocations
- [ ] No DSP or iPlug2 dependencies

---

## Task Order

Execute tasks in this order:
1. **3.1** — Write all theory engine tests — **tests first**
2. Add test file to `tests/CMakeLists.txt`
3. **3.2** — Implement `sea::TheoryEngine` until all tests pass
4. Run all tests: `cd tests/build && cmake .. && make -j && ./run_tests`

## Definition of Done

- [ ] All 20+ chord detection tests pass
- [ ] `sea_theory_engine.h` has zero DSP or iPlug2 dependencies
- [ ] Only standard library includes used
- [ ] All Sprint 1 tests still pass
- [ ] Golden masters unchanged: `python3 scripts/golden_master.py --verify`
- [ ] Header compiles standalone (include it in an empty .cpp to verify)

## Key File References

| File | Purpose |
|------|---------|
| `libs/SEA_Util/include/sea_util/sea_theory_engine.h` | New file to create |
| `tests/unit/Test_TheoryEngine.cpp` | New test file (write first) |
| `tests/CMakeLists.txt` | Add test source file |
| `libs/SEA_Util/CMakeLists.txt` | Already created in Sprint 1 |
