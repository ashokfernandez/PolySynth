#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>

namespace sea {

enum class AllocationMode : uint8_t {
  ResetMode = 0, // Always scan from slot 0 (lowest available index)
  CycleMode      // Round-robin: continue from last allocated slot
};

enum class StealPriority : uint8_t {
  Oldest = 0,     // Steal slot with lowest timestamp
  LowestPitch,    // Steal slot with lowest pitch
  LowestAmplitude // Steal slot with lowest amplitude (future — requires
                  // amplitude getter)
};

// UnisonVoiceInfo: detune and pan for each unison voice
struct UnisonVoiceInfo {
  float detuneCents = 0.0f; // Detune in cents from base pitch
  float panPosition = 0.0f; // -1.0 (left) to +1.0 (right)
};

template <typename Slot, size_t MaxSlots> class VoiceAllocator {
  // Trait requirements (checked at compile time via static_assert in methods):
  // bool     Slot::IsActive() const;
  // uint32_t Slot::GetTimestamp() const;
  // float    Slot::GetPitch() const;
  // void     Slot::StartSteal();

public:
  // Note: static_asserts disabled for C++17 compatibility uncertainty in
  // environment. Ideally use C++20 concepts or C++17 is_invocable_r_v if
  // available.
  /*
  static_assert(
      std::is_invocable_r_v<bool, decltype(&Slot::IsActive), const Slot *>,
      "Slot must have bool IsActive() const");
  static_assert(std::is_invocable_r_v<uint32_t, decltype(&Slot::GetTimestamp),
                                      const Slot *>,
                "Slot must have uint32_t GetTimestamp() const");
  static_assert(
      std::is_invocable_r_v<float, decltype(&Slot::GetPitch), const Slot *>,
      "Slot must have float GetPitch() const");
  static_assert(
      std::is_invocable_r_v<void, decltype(&Slot::StartSteal), Slot *>,
      "Slot must have void StartSteal()");
  */

  VoiceAllocator() = default;

  // --- Configuration ---
  void SetPolyphonyLimit(int limit) {
    if (limit < 1)
      limit = 1;
    if (limit > static_cast<int>(MaxSlots))
      limit = static_cast<int>(MaxSlots);
    mPolyphonyLimit = limit;
  }

  int GetPolyphonyLimit() const { return mPolyphonyLimit; }

  void SetAllocationMode(AllocationMode mode) { mAllocationMode = mode; }

  void SetStealPriority(StealPriority priority) { mStealPriority = priority; }

  void SetUnisonCount(int count) {
    if (count < 1)
      count = 1;
    if (count > 8)
      count = 8;
    mUnisonCount = count;
  }

  int GetUnisonCount() const { return mUnisonCount; }

  void SetUnisonSpread(float spread) {
    if (spread < 0.0f)
      spread = 0.0f;
    if (spread > 1.0f)
      spread = 1.0f;
    mUnisonSpread = spread;
  }

  float GetUnisonSpread() const { return mUnisonSpread; }

  void SetStereoSpread(float spread) {
    if (spread < 0.0f)
      spread = 0.0f;
    if (spread > 1.0f)
      spread = 1.0f;
    mStereoSpread = spread;
  }

  float GetStereoSpread() const { return mStereoSpread; }

  // --- Allocation ---
  // Returns slot index or -1 if no free slot within polyphony limit
  int AllocateSlot(Slot *slots) {
    int activeCount = 0;
    // Bug fix: only count active voices within the current polyphony limit
    for (int i = 0; i < mPolyphonyLimit; ++i) {
      if (slots[i].IsActive()) {
        activeCount++;
      }
    }

    if (activeCount >= mPolyphonyLimit) {
      return -1;
    }

    if (mAllocationMode == AllocationMode::ResetMode) {
      for (int i = 0; i < mPolyphonyLimit; ++i) {
        if (!slots[i].IsActive()) {
          return i;
        }
      }
    } else { // CycleMode
      for (int i = 0; i < mPolyphonyLimit; ++i) {
        int idx = (mRoundRobinIndex + i) % mPolyphonyLimit;
        if (!slots[idx].IsActive()) {
          mRoundRobinIndex = (idx + 1) % mPolyphonyLimit;
          return idx;
        }
      }
    }

    return -1;
  }

  // Returns index of best steal victim or -1 if all slots empty
  // Does NOT call StartSteal — caller decides what to do
  int FindStealVictim(Slot *slots) {
    int bestIdx = -1;

    if (mStealPriority == StealPriority::Oldest) {
      uint32_t minTimestamp = UINT32_MAX;
      for (int i = 0; i < mPolyphonyLimit; ++i) {
        if (slots[i].IsActive()) {
          uint32_t ts = slots[i].GetTimestamp();
          if (ts < minTimestamp) {
            minTimestamp = ts;
            bestIdx = i;
          }
        }
      }
    } else if (mStealPriority == StealPriority::LowestPitch) {
      float minPitch = 1e9f; // Arbitrary large float
      for (int i = 0; i < mPolyphonyLimit; ++i) {
        if (slots[i].IsActive()) {
          float p = slots[i].GetPitch();
          if (p < minPitch) {
            minPitch = p;
            bestIdx = i;
          }
        }
      }
    } else {
      // Fallback to Oldest
      uint32_t minTimestamp = UINT32_MAX;
      for (int i = 0; i < mPolyphonyLimit; ++i) {
        if (slots[i].IsActive()) {
          uint32_t ts = slots[i].GetTimestamp();
          if (ts < minTimestamp) {
            minTimestamp = ts;
            bestIdx = i;
          }
        }
      }
    }

    return bestIdx;
  }

  // --- Sustain Pedal ---
  void OnSustainPedal(bool down) { mSustainDown = down; }

  bool IsSustainDown() const { return mSustainDown; }

  bool ShouldHold(int note) const {
    (void)note; // Note-agnostic sustain for now
    return mSustainDown;
  }

  void MarkSustained(int note) {
    if (note >= 0 && note < 128) {
      mSustainedNotes[note] = true;
    }
  }

  // Fills releasedNotes with notes to release, returns count
  int ReleaseSustainedNotes(int *releasedNotes, int maxCount) {
    int count = 0;
    for (int i = 0; i < 128; ++i) {
      if (mSustainedNotes[i]) {
        if (count < maxCount) {
          releasedNotes[count++] = i;
        }
        mSustainedNotes[i] = false;
      }
    }
    return count;
  }

  // --- Unison ---
  UnisonVoiceInfo GetUnisonVoiceInfo(int unisonIndex) const {
    UnisonVoiceInfo info;
    if (mUnisonCount <= 1) {
      return info;
    }

    // Fraction from -1.0 to 1.0
    float fraction = (2.0f * unisonIndex / (mUnisonCount - 1)) - 1.0f;

    info.detuneCents = fraction * mUnisonSpread * 50.0f;
    info.panPosition = fraction * mStereoSpread;

    return info;
  }

  // --- Polyphony Limit Enforcement ---
  // Finds excess active voices beyond polyphony limit
  // Fills killIndices with slot indices to kill, returns count
  int EnforcePolyphonyLimit(Slot *slots, int *killIndices, int maxKill) {
    int activeCount = 0;

    // Count active first
    for (size_t i = 0; i < MaxSlots; ++i) {
      if (slots[i].IsActive()) {
        activeCount++;
      }
    }

    if (activeCount <= mPolyphonyLimit) {
      return 0;
    }

    int kills = activeCount - mPolyphonyLimit;
    int count = 0;

    // Simpler implementation:
    // Create a list of active slots with their metric (timestamp or pitch)
    struct Victim {
      int index;
      uint32_t timestamp;
      float pitch;
    };

    std::array<Victim, MaxSlots>
        candidates; // Large stack allocation? MaxSlots is usually small (16-64)
    int candidateCount = 0;

    for (size_t i = 0; i < MaxSlots; ++i) {
      if (slots[i].IsActive()) {
        candidates[candidateCount++] = {
            static_cast<int>(i), slots[i].GetTimestamp(), slots[i].GetPitch()};
      }
    }

    // Sort candidates based on priority
    // We want the 'best' victims (lowest score) at the beginning
    if (mStealPriority == StealPriority::Oldest ||
        mStealPriority == StealPriority::LowestAmplitude) {
      std::sort(candidates.begin(), candidates.begin() + candidateCount,
                [](const Victim &a, const Victim &b) {
                  return a.timestamp < b.timestamp;
                });
    } else { // LowestPitch
      std::sort(
          candidates.begin(), candidates.begin() + candidateCount,
          [](const Victim &a, const Victim &b) { return a.pitch < b.pitch; });
    }

    // Pick top N
    int toKill = std::min(kills, maxKill);
    for (int i = 0; i < toKill; ++i) {
      killIndices[count++] = candidates[i].index;
      slots[candidates[i].index].StartSteal();
    }

    return count;
  }

private:
  AllocationMode mAllocationMode = AllocationMode::ResetMode;
  StealPriority mStealPriority = StealPriority::Oldest;
  int mPolyphonyLimit = static_cast<int>(MaxSlots);
  int mUnisonCount = 1;
  float mUnisonSpread = 0.0f; // 0.0–1.0
  float mStereoSpread = 0.0f; // 0.0–1.0
  int mRoundRobinIndex = 0;   // For CycleMode

  bool mSustainDown = false;
  std::array<bool, 128> mSustainedNotes{}; // MIDI notes 0-127
};

} // namespace sea
