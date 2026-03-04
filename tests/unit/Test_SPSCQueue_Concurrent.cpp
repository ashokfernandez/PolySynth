#include "../../src/core/SPSCQueue.h"
#include "../../src/core/SynthState.h"
#include "catch.hpp"

#include <atomic>
#include <thread>
#include <vector>

using PolySynthCore::SynthState;

TEST_CASE("SPSCQueue concurrent push/pop preserves FIFO order",
          "[SPSCQueue][concurrent]") {
  SPSCQueue<int, 64> queue;
  constexpr int kItemCount = 100000;

  std::atomic<bool> producerDone{false};
  std::vector<int> received;
  received.reserve(kItemCount);

  // Producer thread: push 0..kItemCount-1
  std::thread producer([&]() {
    for (int i = 0; i < kItemCount; ++i) {
      while (!queue.TryPush(i)) {
        // Queue full, spin
      }
    }
    producerDone.store(true, std::memory_order_release);
  });

  // Consumer thread: pop all items
  std::thread consumer([&]() {
    int item;
    while (true) {
      if (queue.TryPop(item)) {
        received.push_back(item);
        if (received.size() == kItemCount)
          break;
      } else if (producerDone.load(std::memory_order_acquire)) {
        // Drain remaining
        while (queue.TryPop(item)) {
          received.push_back(item);
        }
        break;
      }
    }
  });

  producer.join();
  consumer.join();

  REQUIRE(received.size() == kItemCount);

  // Verify FIFO order
  for (int i = 0; i < kItemCount; ++i) {
    REQUIRE(received[i] == i);
  }
}

TEST_CASE("SPSCQueue small queue with SynthState — no torn reads",
          "[SPSCQueue][concurrent][SynthState]") {
  SPSCQueue<SynthState, 4> queue;
  constexpr int kItemCount = 10000;

  std::atomic<bool> producerDone{false};
  std::atomic<bool> tornReadDetected{false};

  // Producer: push SynthState with monotonically increasing masterGain
  std::thread producer([&]() {
    for (int i = 0; i < kItemCount; ++i) {
      SynthState state{};
      state.masterGain = static_cast<double>(i);
      while (!queue.TryPush(state)) {
        // Queue full, spin
      }
    }
    producerDone.store(true, std::memory_order_release);
  });

  // Consumer: verify monotonicity of masterGain (no torn reads)
  std::thread consumer([&]() {
    SynthState state{};
    double lastGain = -1.0;
    int consumed = 0;

    while (true) {
      if (queue.TryPop(state)) {
        if (state.masterGain <= lastGain) {
          tornReadDetected.store(true, std::memory_order_relaxed);
        }
        lastGain = state.masterGain;
        ++consumed;
        if (consumed == kItemCount)
          break;
      } else if (producerDone.load(std::memory_order_acquire)) {
        // Drain remaining
        while (queue.TryPop(state)) {
          if (state.masterGain <= lastGain) {
            tornReadDetected.store(true, std::memory_order_relaxed);
          }
          lastGain = state.masterGain;
          ++consumed;
        }
        break;
      }
    }

    REQUIRE(consumed == kItemCount);
  });

  producer.join();
  consumer.join();

  REQUIRE_FALSE(tornReadDetected.load());
}
