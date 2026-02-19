#include "catch.hpp"
#include <sea_util/sea_voice_allocator.h>

// Minimal mock that satisfies VoiceAllocator trait requirements
struct MockSlot {
  bool active = false;
  uint32_t timestamp = 0;
  float pitch = 440.0f;
  bool stolen = false;

  bool IsActive() const { return active; }
  uint32_t GetTimestamp() const { return timestamp; }
  float GetPitch() const { return pitch; }
  void StartSteal() {
    stolen = true;
    active = false;
  }
};

TEST_CASE("VoiceAllocator ResetMode: allocates lowest available",
          "[VoiceAllocator]") {
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
  REQUIRE(idx == 0); // Reset mode goes back to 0
}

TEST_CASE("VoiceAllocator CycleMode: round-robin allocation",
          "[VoiceAllocator]") {
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
  REQUIRE(idx3 == 3); // Next in cycle

  // Now ensure slot 0 is active so we skip it
  slots[0].active = true;

  // Free slot 1.
  slots[1].active = false;
  int idx4 = alloc.AllocateSlot(slots);
  // Wraps around, checks 0 (active), checks 1 (inactive) -> returns 1
  REQUIRE(idx4 == 1);
}

TEST_CASE("VoiceAllocator: Oldest stealing", "[VoiceAllocator]") {
  sea::VoiceAllocator<MockSlot, 4> alloc;
  alloc.SetStealPriority(sea::StealPriority::Oldest);
  MockSlot slots[4] = {};

  // Set up 4 active slots with different timestamps
  for (int i = 0; i < 4; i++) {
    slots[i].active = true;
    slots[i].timestamp = (i + 1) * 100; // 100, 200, 300, 400
  }

  int victim = alloc.FindStealVictim(slots);
  REQUIRE(victim == 0); // Oldest (timestamp=100)
}

TEST_CASE("VoiceAllocator: LowestPitch stealing", "[VoiceAllocator]") {
  sea::VoiceAllocator<MockSlot, 4> alloc;
  alloc.SetStealPriority(sea::StealPriority::LowestPitch);
  MockSlot slots[4] = {};

  slots[0] = {true, 100, 440.0f, false};
  slots[1] = {true, 200, 220.0f, false}; // Lowest pitch
  slots[2] = {true, 300, 880.0f, false};
  slots[3] = {true, 400, 330.0f, false};

  int victim = alloc.FindStealVictim(slots);
  REQUIRE(victim == 1); // Lowest pitch (220Hz)
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
  alloc.SetUnisonSpread(1.0); // Max spread = 50 cents
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
    if (released[i] == 60)
      has60 = true;
    if (released[i] == 64)
      has64 = true;
  }
  REQUIRE(has60);
  REQUIRE(has64);
}

TEST_CASE("VoiceAllocator: EnforcePolyphonyLimit kills excess",
          "[VoiceAllocator]") {
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
  REQUIRE(killCount == 2); // Need to kill 2 excess voices

  // Should have killed the oldest ones
  for (int i = 0; i < killCount; i++) {
    REQUIRE(slots[killIndices[i]].stolen == true);
  }
}
