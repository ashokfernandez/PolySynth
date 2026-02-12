#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>

namespace sea {

template <typename T> class LookaheadLimiter {
public:
  void Init(T sampleRate) {
    mSampleRate = sampleRate;
    UpdateLookaheadBuffer();
    Reset();
  }

  void Reset() {
    std::fill(mBufferL.begin(), mBufferL.end(), T(0.0));
    std::fill(mBufferR.begin(), mBufferR.end(), T(0.0));
    mWriteIndex = 0;
    mSampleIndex = 0;
    mPeakWindow.clear();
    mGain = T(1.0);
  }

  void SetParams(T threshold, T lookaheadMs, T releaseMs) {
    mThreshold = std::max(T(0.01), std::min(T(1.0), threshold));
    mReleaseMs = std::max(T(1.0), std::min(T(500.0), releaseMs));

    T newLookahead = std::max(T(0.0), std::min(T(50.0), lookaheadMs));
    if (newLookahead != mLookaheadMs) {
      mLookaheadMs = newLookahead;
      UpdateLookaheadBuffer();
    }
  }

  void Process(T &left, T &right) {
    if (mBufferL.empty() || mBufferR.empty()) {
      return;
    }

    const T peak = std::max(std::abs(left), std::abs(right));
    const int window = static_cast<int>(mBufferL.size());

    while (!mPeakWindow.empty() && mPeakWindow.back().value <= peak) {
      mPeakWindow.pop_back();
    }
    mPeakWindow.push_back({mSampleIndex, peak});

    const int64_t minIndex = mSampleIndex - (window - 1);
    while (!mPeakWindow.empty() && mPeakWindow.front().index < minIndex) {
      mPeakWindow.pop_front();
    }

    const T maxPeak = mPeakWindow.empty() ? T(0.0) : mPeakWindow.front().value;
    const T targetGain = maxPeak > mThreshold ? (mThreshold / maxPeak) : T(1.0);

    if (targetGain < mGain) {
      mGain = targetGain;
    } else {
      const T releaseCoeff =
          T(1.0) - std::exp(-T(1.0) / (mReleaseMs * T(0.001) * mSampleRate));
      mGain = mGain + (targetGain - mGain) * releaseCoeff;
    }

    const int size = static_cast<int>(mBufferL.size());
    const int readIndex = (mWriteIndex + 1) % size;

    T outL = static_cast<T>(mBufferL[readIndex] * mGain);
    T outR = static_cast<T>(mBufferR[readIndex] * mGain);

    mBufferL[mWriteIndex] = left;
    mBufferR[mWriteIndex] = right;

    left = outL;
    right = outR;

    mWriteIndex = readIndex;
    ++mSampleIndex;
  }

private:
  struct PeakEntry {
    int64_t index;
    T value;
  };

  void UpdateLookaheadBuffer() {
    const int samples =
        std::max(1, static_cast<int>(mLookaheadMs * T(0.001) * mSampleRate));
    mBufferL.assign(samples, T(0.0));
    mBufferR.assign(samples, T(0.0));
  }

  T mSampleRate = T(44100.0);
  T mThreshold = T(0.95);
  T mLookaheadMs = T(5.0);
  T mReleaseMs = T(50.0);
  int mWriteIndex = 0;
  int64_t mSampleIndex = 0;
  T mGain = T(1.0);
  std::deque<PeakEntry> mPeakWindow;
  std::vector<T> mBufferL;
  std::vector<T> mBufferR;
};

} // namespace sea
