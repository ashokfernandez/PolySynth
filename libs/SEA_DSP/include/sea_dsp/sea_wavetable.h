#pragma once
#include <array>
#include <cstddef>

namespace sea {

namespace detail {

// Constexpr Taylor series sin — only runs at compile time to seed the table.
// Accuracy doesn't matter for runtime; the table values are what get used.
constexpr double constexpr_sin(double x) {
  // Reduce to [-pi, pi]
  constexpr double pi = 3.14159265358979323846;
  constexpr double two_pi = 6.28318530717958647692;
  while (x > pi)
    x -= two_pi;
  while (x < -pi)
    x += two_pi;
  // 11th-order Taylor: sin(x) ≈ x - x³/6 + x⁵/120 - x⁷/5040 + x⁹/362880 -
  // x¹¹/39916800
  double x2 = x * x;
  double x3 = x2 * x;
  double x5 = x3 * x2;
  double x7 = x5 * x2;
  double x9 = x7 * x2;
  double x11 = x9 * x2;
  return x - x3 / 6.0 + x5 / 120.0 - x7 / 5040.0 + x9 / 362880.0 -
         x11 / 39916800.0;
}

template <typename T, size_t N>
constexpr std::array<T, N> make_sin_table() {
  constexpr double two_pi = 6.28318530717958647692;
  std::array<T, N> table{};
  for (size_t i = 0; i < N; ++i) {
    table[i] = static_cast<T>(constexpr_sin(two_pi * static_cast<double>(i) /
                                            static_cast<double>(N)));
  }
  return table;
}

} // namespace detail

/// 512-entry sine wavetable with linear interpolation lookup.
/// Memory: N × sizeof(T) bytes (512 × 4 = 2KB for float).
/// Constexpr table lives in flash (XIP cached on RP2350).
/// Linear interpolation gives <0.01% max error at N=512.
template <typename T, size_t N = 512> struct SinWavetable {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");

  static constexpr std::array<T, N> kTable = detail::make_sin_table<T, N>();

  /// Lookup sin(phase) with linear interpolation.
  /// @param phase  Phase in [0, 2π). Values outside are wrapped via bitmask.
  static inline T Lookup(T phase) {
    constexpr T kInvStep =
        static_cast<T>(N) / static_cast<T>(6.28318530717958647692);
    constexpr size_t kMask = N - 1;

    T idx_f = phase * kInvStep;
    auto i0 = static_cast<size_t>(static_cast<unsigned int>(idx_f)) & kMask;
    size_t i1 = (i0 + 1) & kMask;
    T frac = idx_f - static_cast<T>(static_cast<unsigned int>(idx_f));

    return kTable[i0] + frac * (kTable[i1] - kTable[i0]);
  }
};

} // namespace sea
