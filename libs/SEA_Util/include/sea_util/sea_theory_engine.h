#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>

namespace sea {

struct ChordResult {
  bool valid = false;
  int root = 0;             // Pitch class 0-11 (0=C, 1=C#, ..., 11=B)
  int bass = 0;             // Pitch class of lowest note
  int quality = 0;          // Index into quality table
  bool isInversion = false; // true if bass != root
};

class TheoryEngine {
public:
  // Analyze an array of MIDI note numbers. Returns best chord match.
  // Requires at least 3 unique pitch classes for a valid result.
  static ChordResult Analyze(const int *notes, int count) {
    if (!notes || count < 3) {
      return ChordResult{};
    }

    // 1. Copy and sort notes to find bass
    // Stack allocation is safe for small chords, but we need to handle
    // arbitrary count. However, we only care about pitch classes. Let's sort
    // the input to find the bass (lowest note). Since input is const, we might
    // need a local copy. But for performance, maybe we can just scan for min?
    // Actually, the plan says "Sort input MIDI notes ascending".
    // We can limit the copy size to something reasonable, e.g. 16 notes.
    // Most chords are < 10 notes.
    constexpr int kMaxNotes = 32;
    int localNotes[kMaxNotes];
    int numNotes = (count > kMaxNotes) ? kMaxNotes : count;

    for (int i = 0; i < numNotes; ++i) {
      localNotes[i] = notes[i];
    }
    std::sort(localNotes, localNotes + numNotes);

    int bassNote = localNotes[0];
    int bassPC = bassNote % 12;

    // 2. Build 12-bit pitch class bitmask
    uint16_t mask = 0;
    int uniquePCs = 0;
    for (int i = 0; i < numNotes; ++i) {
      int pc = localNotes[i] % 12;
      if (pc < 0)
        pc += 12;
      if (!((mask >> pc) & 1)) {
        uniquePCs++;
        mask |= (1 << pc);
      }
    }

    if (uniquePCs < 3) {
      return ChordResult{};
    }

    // 3. Template table
    // Index | Quality Name | Intervals (semitones above root)  | Bitmask
    struct Template {
      uint16_t mask;
      int intervalCount;
      // int qualityIndex; // Implicit by array index
    };

    static constexpr Template templates[] = {
        {0b000010010001, 3}, // 0: maj {0, 4, 7}
        {0b000010001001, 3}, // 1: min {0, 3, 7}
        {0b000001001001, 3}, // 2: dim {0, 3, 6}
        {0b000100010001, 3}, // 3: aug {0, 4, 8}
        {0b000010000101, 3}, // 4: sus2 {0, 2, 7}
        {0b000010100001, 3}, // 5: sus4 {0, 5, 7}
        {0b010010010001, 4}, // 6: 7 {0, 4, 7, 10}
        {0b100010010001, 4}, // 7: maj7 {0, 4, 7, 11}
        {0b010010001001, 4}, // 8: min7 {0, 3, 7, 10}
        {0b001001001001, 4}, // 9: dim7 {0, 3, 6, 9}
        {0b010001001001, 4}  // 10: m7b5 {0, 3, 6, 10}
    };

    int bestScore = -100;
    ChordResult bestMatch{};

    // 4. Match against templates
    for (int root = 0; root < 12; ++root) {
      // Optimization: root must be present in the chord
      if (!((mask >> root) & 1)) {
        continue;
      }

      // Rotate bitmask so potential root is at 0
      // (bitmask >> root) | (bitmask << (12 - root)) & 0xFFF
      // Note: 12-bit rotation
      uint16_t rotated = (mask >> root) | (mask << (12 - root));
      rotated &= 0xFFF;

      for (int q = 0; q < 11; ++q) {
        const auto &tpl = templates[q];

        // Check if template intervals are a subset of rotated bitmask
        // i.e. (rotated & tpl.mask) == tpl.mask
        // Wait, "Check if template intervals are a subset of rotated bitmask"
        // means the chord MUST contain ALL notes of the template?
        // Plan says: "match = (rotated & template) == template"
        // This implies strict superset match (chord must have at least the
        // template notes). But what if we omit the 5th? For now, strict match
        // as per plan.

        if ((rotated & tpl.mask) == tpl.mask) {
          int score =
              tpl.intervalCount * 2; // Start with number of matched intervals

          // Bonus: Root is bass?
          if (root == bassPC) {
            score += 2;
          }

          // Bonus: 7th chord preference?
          if (tpl.intervalCount >= 4) {
            score += 1;
          }

          // Penalty: Extra notes not in template
          uint16_t extraBits = rotated ^ tpl.mask;
          int extraNotes = 0;
          for (int k = 0; k < 12; ++k) {
            if ((extraBits >> k) & 1)
              extraNotes++;
          }
          score -= extraNotes * 2;

          if (score > bestScore) {
            bestScore = score;
            bestMatch.valid = true;
            bestMatch.root = root;
            bestMatch.bass = bassPC;
            bestMatch.quality = q;
            bestMatch.isInversion = (root != bassPC);
          }
        }
      }
    }

    if (bestScore <= 0) {
      return ChordResult{};
    }

    return bestMatch;
  }

  // Format chord name into buffer. Returns string length.
  // Examples: "Cmaj", "Amin/C", "G7", "F#dim7"
  static int FormatChordName(const ChordResult &result, char *buf,
                             int bufSize) {
    if (!result.valid || !buf || bufSize <= 0) {
      if (buf && bufSize > 0)
        *buf = '\0';
      return 0;
    }

    static const char *rootNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                      "F#", "G",  "G#", "A",  "A#", "B"};

    static const char *qualityNames[] = {"maj",  "min",  "dim", "aug",
                                         "sus2", "sus4", "7",   "maj7",
                                         "min7", "dim7", "m7b5"};

    if (result.root < 0 || result.root >= 12 || result.quality < 0 ||
        result.quality >= 11) {
      *buf = '\0';
      return 0;
    }

    int len = 0;
    if (result.isInversion) {
      // Check bounds for bass as well
      if (result.bass < 0 || result.bass >= 12) {
        *buf = '\0';
        return 0;
      }
      len = std::snprintf(buf, bufSize, "%s%s/%s", rootNames[result.root],
                          qualityNames[result.quality], rootNames[result.bass]);
    } else {
      len = std::snprintf(buf, bufSize, "%s%s", rootNames[result.root],
                          qualityNames[result.quality]);
    }

    if (len < 0)
      return 0; // Error
    if (len >= bufSize)
      return bufSize - 1; // Truncated
    return len;
  }

private:
  // No instances
  TheoryEngine() = delete;
};

} // namespace sea
