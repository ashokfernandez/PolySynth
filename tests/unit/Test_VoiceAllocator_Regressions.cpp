#include "catch.hpp"
#include <sea_util/sea_voice_allocator.h>

struct MockSlot {
  bool active = false;
  uint64_t timestamp = 0;
  float pitch = 440.0f;
  bool stolen = false;

  bool IsActive() const { return active; }
  uint64_t GetTimestamp() const { return timestamp; }
  float GetPitch() const { return pitch; }
  void StartSteal() {
    stolen = true;
    active = false;
  }
};

TEST_CASE("VoiceAllocator Regression: Active voices outside limit should not "
          "block allocation",
          "[VoiceAllocator]") {
  sea::VoiceAllocator<MockSlot, 8> alloc;
  MockSlot slots[8] = {};

  // 1. Set limit to max (8)
  alloc.SetPolyphonyLimit(8);

  // 2. Activate voices in the upper half (4, 5, 6, 7)
  for (int i = 4; i < 8; ++i) {
    slots[i].active = true;
  }

  // 3. Reduce limit to 4
  // Note: We are NOT calling EnforcePolyphonyLimit here to simulate the state
  // where "voices above polyphony limit are somehow active".
  alloc.SetPolyphonyLimit(4);

  // 4. Try to allocate.
  // Expected:
  // - Active count within limit (0..3) is 0.
  // - Limit is 4.
  // - Should allocate successfully in range 0..3.
  //
  // Failure mode (Old code):
  // - Active count scans 0..7 -> finds 4 active.
  // - 4 >= 4 (limit) -> Allocator assumes full -> returns -1.
  int idx = alloc.AllocateSlot(slots);

  REQUIRE(idx != -1);
  REQUIRE(idx < 4);
  REQUIRE(slots[idx].active == false); // Should find a free slot
}

TEST_CASE("VoiceAllocator Regression: CycleMode respects reduced limit",
          "[VoiceAllocator]") {
  sea::VoiceAllocator<MockSlot, 8> alloc;
  alloc.SetAllocationMode(sea::AllocationMode::CycleMode);
  MockSlot slots[8] = {};

  // 1. Limit 4
  alloc.SetPolyphonyLimit(4);

  // 2. Allocate 0, 1
  int idx0 = alloc.AllocateSlot(slots);
  slots[idx0].active = true;
  int idx1 = alloc.AllocateSlot(slots);
  slots[idx1].active = true;

  // 3. Round robin index should be at 2.

  // 4. Allocation should return 2.
  int idx2 = alloc.AllocateSlot(slots);
  REQUIRE(idx2 == 2);
  slots[idx2].active = true;
}
