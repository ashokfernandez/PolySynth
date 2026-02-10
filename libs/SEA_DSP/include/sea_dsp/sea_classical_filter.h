#pragma once
#include "sea_svf.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "sea_math.h"

namespace sea {

template <typename T> class ClassicalFilter {
public:
  enum class Type { Butterworth, Chebyshev1, Chebyshev2 };

  ClassicalFilter() = default;

  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    for (auto &f : stages)
      f.Init(sampleRate);
    Reset();
  }

  void Reset() {
    for (auto &f : stages)
      f.Reset();
  }

  void SetConfig(Type type, int order, T rippleDb) {
    mType = type;
    mOrder = (order < 2) ? 2 : order;
    if (mOrder % 2 != 0)
      mOrder += 1;

    mRippleDb =
        (rippleDb < static_cast<T>(0.01)) ? static_cast<T>(0.01) : rippleDb;
    CalculatePoles();
  }

  void SetCutoff(T cutoff) {
    // ClampCutoff
    if (mSampleRate <= static_cast<T>(0.0)) {
      mCutoff = static_cast<T>(0.0);
    } else {
      T nyquist = static_cast<T>(0.5) * mSampleRate;
      T maxCutoff = nyquist * static_cast<T>(0.49);
      mCutoff = Math::Clamp(cutoff, static_cast<T>(0.0), maxCutoff);
    }

    for (size_t i = 0; i < stages.size(); ++i) {
      stages[i].SetParams(mCutoff, mQFactors[i]);
    }
  }

  SEA_INLINE T Process(T in) {
    T out = in;
    for (auto &stage : stages) {
      out = stage.ProcessLP(out);
    }
    return out;
  }

private:
  void CalculatePoles() {
    // TODO: Optimize for embedded (remove allocation)
    stages.clear();
    mQFactors.clear();

    int numPairs = mOrder / 2;

    if (mType == Type::Butterworth || mType == Type::Chebyshev2) {
      for (int k = 1; k <= numPairs; ++k) {
        T angle = (static_cast<T>(2.0) * static_cast<T>(k) +
                   static_cast<T>(mOrder) - static_cast<T>(1.0)) *
                  static_cast<T>(kPi) / (static_cast<T>(2.0) * static_cast<T>(mOrder));
        T q = static_cast<T>(-1.0) / (static_cast<T>(2.0) * Math::Cos(angle));
        mQFactors.push_back(q);
        stages.emplace_back();
      }
    } else if (mType == Type::Chebyshev1) {
      T eps = Math::Sqrt(
          Math::Pow(static_cast<T>(10.0), mRippleDb / static_cast<T>(10.0)) -
          static_cast<T>(1.0));

      // asinh(x) = log(x + sqrt(x^2 + 1))
      T x_val = static_cast<T>(1.0) / eps;
      T a = Math::Log(x_val + Math::Sqrt(x_val * x_val + static_cast<T>(1.0))) /
            static_cast<T>(mOrder);

      T sinh_a = (Math::Exp(a) - Math::Exp(-a)) / static_cast<T>(2.0);
      T cosh_a = (Math::Exp(a) + Math::Exp(-a)) / static_cast<T>(2.0);

      for (int k = 1; k <= numPairs; ++k) {
        T theta =
            (static_cast<T>(2.0) * static_cast<T>(k) - static_cast<T>(1.0)) *
            static_cast<T>(kPi) / (static_cast<T>(2.0) * static_cast<T>(mOrder));
        T re = -sinh_a * Math::Sin(theta);
        T im = cosh_a * Math::Cos(theta);

        T mag2 = re * re + im * im;
        T mag = Math::Sqrt(mag2);
        T q = mag / (static_cast<T>(-2.0) * re);

        mQFactors.push_back(q);
        stages.emplace_back();
      }
    }

    // Initialize the new stages
    for (size_t i = 0; i < stages.size(); ++i) {
      stages[i].Init(mSampleRate);
      stages[i].SetParams(mCutoff, mQFactors[i]);
    }
  }

  T mSampleRate = static_cast<T>(44100.0);
  Type mType = Type::Butterworth;
  int mOrder = 4;
  T mRippleDb = static_cast<T>(1.0);
  T mCutoff = static_cast<T>(1000.0);

  // TODO: Optimize for embedded (remove allocation)
  std::vector<SVFilter<T>> stages;
  std::vector<T> mQFactors;
};

} // namespace sea
