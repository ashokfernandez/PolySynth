#include "catch.hpp"
#include <sea_util/sea_theory_engine.h>
#include <vector>

// Helper: format chord result to string
static std::string ChordStr(const int *notes, int count) {
  auto result = sea::TheoryEngine::Analyze(notes, count);
  if (!result.valid)
    return "";
  char buf[32];
  sea::TheoryEngine::FormatChordName(result, buf, sizeof(buf));
  return std::string(buf);
}

// ============ Basic Triads ============

TEST_CASE("TheoryEngine: C major triad", "[TheoryEngine]") {
  int notes[] = {60, 64, 67}; // C4, E4, G4
  REQUIRE(ChordStr(notes, 3) == "Cmaj");
}

TEST_CASE("TheoryEngine: A minor triad", "[TheoryEngine]") {
  int notes[] = {57, 60, 64}; // A3, C4, E4
  REQUIRE(ChordStr(notes, 3) == "Amin");
}

TEST_CASE("TheoryEngine: B diminished triad", "[TheoryEngine]") {
  int notes[] = {59, 62, 65}; // B3, D4, F4
  REQUIRE(ChordStr(notes, 3) == "Bdim");
}

TEST_CASE("TheoryEngine: C augmented triad", "[TheoryEngine]") {
  int notes[] = {60, 64, 68}; // C4, E4, G#4
  REQUIRE(ChordStr(notes, 3) == "Caug");
}

TEST_CASE("TheoryEngine: D sus2", "[TheoryEngine]") {
  int notes[] = {62, 64, 69}; // D4, E4, A4
  REQUIRE(ChordStr(notes, 3) == "Dsus2");
}

TEST_CASE("TheoryEngine: D sus4", "[TheoryEngine]") {
  int notes[] = {62, 67, 69}; // D4, G4, A4
  REQUIRE(ChordStr(notes, 3) == "Dsus4");
}

// ============ Seventh Chords ============

TEST_CASE("TheoryEngine: C dominant 7", "[TheoryEngine]") {
  int notes[] = {60, 64, 67, 70}; // C4, E4, G4, Bb4
  REQUIRE(ChordStr(notes, 4) == "C7");
}

TEST_CASE("TheoryEngine: C major 7", "[TheoryEngine]") {
  int notes[] = {60, 64, 67, 71}; // C4, E4, G4, B4
  REQUIRE(ChordStr(notes, 4) == "Cmaj7");
}

TEST_CASE("TheoryEngine: C minor 7", "[TheoryEngine]") {
  int notes[] = {60, 63, 67, 70}; // C4, Eb4, G4, Bb4
  REQUIRE(ChordStr(notes, 4) == "Cmin7");
}

TEST_CASE("TheoryEngine: B diminished 7", "[TheoryEngine]") {
  int notes[] = {59, 62, 65, 68}; // B3, D4, F4, Ab4
  REQUIRE(ChordStr(notes, 4) == "Bdim7");
}

TEST_CASE("TheoryEngine: B half-diminished 7", "[TheoryEngine]") {
  int notes[] = {59, 62, 65, 69}; // B3, D4, F4, A4
  REQUIRE(ChordStr(notes, 4) == "Bm7b5");
}

// ============ Inversions ============

TEST_CASE("TheoryEngine: C major first inversion", "[TheoryEngine]") {
  int notes[] = {64, 67, 72}; // E4, G4, C5
  auto result = sea::TheoryEngine::Analyze(notes, 3);
  REQUIRE(result.valid);
  REQUIRE(result.root == 0); // C
  REQUIRE(result.bass == 4); // E
  REQUIRE(result.isInversion == true);

  char buf[32];
  sea::TheoryEngine::FormatChordName(result, buf, sizeof(buf));
  REQUIRE(std::string(buf) == "Cmaj/E");
}

TEST_CASE("TheoryEngine: C major second inversion", "[TheoryEngine]") {
  int notes[] = {67, 72, 76}; // G4, C5, E5
  auto result = sea::TheoryEngine::Analyze(notes, 3);
  REQUIRE(result.valid);
  REQUIRE(result.root == 0); // C
  REQUIRE(result.bass == 7); // G
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
  int notes[] = {48, 52, 55, 60}; // C3, E3, G3, C4
  REQUIRE(ChordStr(notes, 4) == "Cmaj");
}

TEST_CASE("TheoryEngine: chromatic cluster is invalid", "[TheoryEngine]") {
  int notes[] = {60, 61, 62}; // C, C#, D
  auto result = sea::TheoryEngine::Analyze(notes, 3);
  REQUIRE_FALSE(result.valid);
}

// ============ All 12 Chromatic Roots ============

TEST_CASE("TheoryEngine: all 12 major triads", "[TheoryEngine]") {
  // Major triad intervals: root, +4, +7
  const char *names[] = {"Cmaj",  "C#maj", "Dmaj",  "D#maj", "Emaj",  "Fmaj",
                         "F#maj", "Gmaj",  "G#maj", "Amaj",  "A#maj", "Bmaj"};
  for (int root = 0; root < 12; root++) {
    int notes[] = {60 + root, 64 + root, 67 + root};
    INFO("Root: " << root);
    REQUIRE(ChordStr(notes, 3) == names[root]);
  }
}

// ============ Format Function ============

TEST_CASE("TheoryEngine: FormatChordName with invalid result",
          "[TheoryEngine]") {
  sea::ChordResult invalid{};
  invalid.valid = false;
  char buf[32];
  int len = sea::TheoryEngine::FormatChordName(invalid, buf, sizeof(buf));
  REQUIRE(len == 0);
  REQUIRE(std::string(buf) == "");
}

TEST_CASE("TheoryEngine: FormatChordName buffer size handling",
          "[TheoryEngine]") {
  int notes[] = {60, 64, 67};
  auto result = sea::TheoryEngine::Analyze(notes, 3);
  // Tiny buffer — should not overflow
  char tiny[4];
  int len = sea::TheoryEngine::FormatChordName(result, tiny, sizeof(tiny));
  // Should truncate but not crash
  REQUIRE(len >= 0);
}

// ============ Robustness & Edge Cases ============

TEST_CASE("TheoryEngine: Robustness - Negative Notes", "[TheoryEngine]") {
  // Negative numbers can occur if raw data is passed improperly.
  // -1 % 12 = -1 in C++, which causes UB in bit shifting.
  // Engine should handle this gracefully (modulo 12 logic or ignoring).
  int notes[] = {-12, 60, 64, 67}; // -12 is technically C-2
  // If we handle modulo correctly, -12 % 12 == 0 (or -0).
  // Let's see if it crashes or works.
  auto result = sea::TheoryEngine::Analyze(notes, 4);
  REQUIRE(result.valid); // Should still be Cmaj if -12 wraps to 0
  REQUIRE(result.root == 0);
}

TEST_CASE("TheoryEngine: Robustness - Large Note Count", "[TheoryEngine]") {
  // Engine has internal limit (e.g. 32).
  // Let's pass 40 notes (chromatic scale repeated).
  std::vector<int> lotsOfNotes;
  for (int i = 0; i < 40; ++i)
    lotsOfNotes.push_back(i + 40);

  // Should return invalid because it's a cluster, but NOT crash
  auto result =
      sea::TheoryEngine::Analyze(lotsOfNotes.data(), (int)lotsOfNotes.size());
  REQUIRE_FALSE(result.valid);
}

TEST_CASE("TheoryEngine: Robustness - Large Inputs", "[TheoryEngine]") {
  // 60 + 1200 = 1260. Large numbers.
  int notes[] = {1260, 1264, 1267};
  auto result = sea::TheoryEngine::Analyze(notes, 3);
  REQUIRE(result.valid);     // Should be valid Cmaj
  REQUIRE(result.root == 0); // Still C
}
