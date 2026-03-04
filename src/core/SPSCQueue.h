#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

/// Lock-free single-producer single-consumer ring buffer.
/// Producer and consumer must each be confined to a single thread.
/// Suitable for UI-thread → audio-thread state transfer.
template <typename T, size_t Capacity>
class SPSCQueue {
  static_assert(Capacity > 0, "Capacity must be > 0");
  static_assert(std::is_trivially_copyable<T>::value,
                "T must be trivially copyable for lock-free transfer");

public:
  SPSCQueue() : mHead(0), mTail(0) {}

  /// Push an item (producer thread only).
  /// Returns false if the queue is full.
  bool TryPush(const T& item) {
    const size_t head = mHead.load(std::memory_order_relaxed);
    const size_t next = (head + 1) % (Capacity + 1);
    if (next == mTail.load(std::memory_order_acquire))
      return false; // full
    mBuffer[head] = item;
    mHead.store(next, std::memory_order_release);
    return true;
  }

  /// Pop an item (consumer thread only).
  /// Returns false if the queue is empty.
  bool TryPop(T& item) {
    const size_t tail = mTail.load(std::memory_order_relaxed);
    if (tail == mHead.load(std::memory_order_acquire))
      return false; // empty
    item = mBuffer[tail];
    mTail.store((tail + 1) % (Capacity + 1), std::memory_order_release);
    return true;
  }

private:
  // Buffer has Capacity+1 slots to distinguish full from empty.
  T mBuffer[Capacity + 1];
  alignas(64) std::atomic<size_t> mHead;
  alignas(64) std::atomic<size_t> mTail;
};
