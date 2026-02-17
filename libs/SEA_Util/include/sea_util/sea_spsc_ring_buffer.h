#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sea {

template <typename T, uint32_t Capacity>
class SPSCRingBuffer {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
  static_assert(Capacity > 0, "Capacity must be > 0");

public:
  SPSCRingBuffer() = default;

  bool TryPush(const T &item) {
    const uint32_t head = mHead.load(std::memory_order_relaxed);
    const uint32_t tail = mTail.load(std::memory_order_acquire);

    if ((head - tail) == Capacity) {
      return false;
    }

    mBuffer[head & (Capacity - 1)] = item;
    mHead.store(head + 1, std::memory_order_release);
    return true;
  }

  bool TryPop(T &item) {
    const uint32_t tail = mTail.load(std::memory_order_relaxed);
    const uint32_t head = mHead.load(std::memory_order_acquire);

    if (head == tail) {
      return false;
    }

    item = mBuffer[tail & (Capacity - 1)];
    mTail.store(tail + 1, std::memory_order_release);
    return true;
  }

  uint32_t Size() const {
    const uint32_t head = mHead.load(std::memory_order_acquire);
    const uint32_t tail = mTail.load(std::memory_order_acquire);
    return head - tail;
  }

  void Clear() {
    const uint32_t head = mHead.load(std::memory_order_acquire);
    mTail.store(head, std::memory_order_release);
  }

private:
  std::array<T, Capacity> mBuffer{};
  std::atomic<uint32_t> mHead{0};
  std::atomic<uint32_t> mTail{0};
};

} // namespace sea
