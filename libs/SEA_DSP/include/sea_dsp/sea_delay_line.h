#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>

namespace sea {

template <typename T>
class DelayLine {
public:
  DelayLine() : buffer_ptr_(nullptr), capacity_(0), write_head_(0), owns_memory_(false) {}

  ~DelayLine() {
    if (owns_memory_ && buffer_ptr_ != nullptr) {
      // Managed buffer cleanup is handled by std::vector
    }
  }

  // Managed mode: Internal allocation
  void Init(size_t max_delay_samples) {
    managed_buffer_.resize(max_delay_samples);
    buffer_ptr_ = managed_buffer_.data();
    capacity_ = max_delay_samples;
    write_head_ = 0;
    owns_memory_ = true;

    // Clear buffer
    std::fill(managed_buffer_.begin(), managed_buffer_.end(), T(0));
  }

  // Unmanaged mode: External buffer
  void Init(T* external_buffer, size_t size) {
    buffer_ptr_ = external_buffer;
    capacity_ = size;
    write_head_ = 0;
    owns_memory_ = false;

    // Clear buffer
    for (size_t i = 0; i < capacity_; ++i) {
      buffer_ptr_[i] = T(0);
    }
  }

  // Write sample to delay line
  void Push(T sample) {
    if (buffer_ptr_ == nullptr || capacity_ == 0) return;

    buffer_ptr_[write_head_] = sample;
    write_head_ = (write_head_ + 1) % capacity_;
  }

  // Read with fractional delay (linear interpolation)
  T Read(T delay_in_samples) const {
    if (buffer_ptr_ == nullptr || capacity_ == 0) return T(0);

    // Clamp delay to valid range
    if (delay_in_samples < T(0)) delay_in_samples = T(0);
    if (delay_in_samples >= T(capacity_)) delay_in_samples = T(capacity_ - 1);

    // Calculate read position (write_head - 1 is the last written sample)
    // Delay of 0 should read the most recent sample (write_head - 1)
    // Delay of N should read the sample N positions back
    T read_pos_float = static_cast<T>(write_head_) - T(1) - delay_in_samples;

    // Handle negative wrap-around
    while (read_pos_float < T(0)) {
      read_pos_float += static_cast<T>(capacity_);
    }

    // Extract integer and fractional parts
    size_t read_pos_int = static_cast<size_t>(read_pos_float) % capacity_;
    T frac = read_pos_float - static_cast<T>(static_cast<size_t>(read_pos_float));

    // Get two samples for interpolation
    size_t read_pos_next = (read_pos_int + 1) % capacity_;
    T sample1 = buffer_ptr_[read_pos_int];
    T sample2 = buffer_ptr_[read_pos_next];

    // Linear interpolation
    return sample1 * (T(1) - frac) + sample2 * frac;
  }

private:
  std::vector<T> managed_buffer_;  // Used only in managed mode
  T* buffer_ptr_;                  // Points to either managed or external buffer
  size_t capacity_;
  size_t write_head_;
  bool owns_memory_;
};

} // namespace sea
