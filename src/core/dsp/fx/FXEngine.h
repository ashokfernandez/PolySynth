#pragma once

#include "../../types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>

namespace PolySynthCore {

class Chorus {
public:
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    const int maxDelaySamples = static_cast<int>(sampleRate * 0.05); // 50 ms
    mBufferL.assign(maxDelaySamples + 1, 0.0);
    mBufferR.assign(maxDelaySamples + 1, 0.0);
    mWriteIndex = 0;
    mPhase = 0.0;
  }

  void Reset() {
    std::fill(mBufferL.begin(), mBufferL.end(), 0.0);
    std::fill(mBufferR.begin(), mBufferR.end(), 0.0);
    mWriteIndex = 0;
    mPhase = 0.0;
  }

  void SetParams(double rateHz, double depth, double mix) {
    mRateHz = std::max(0.0, rateHz);
    mDepth = std::clamp(depth, 0.0, 1.0);
    mMix = std::clamp(mix, 0.0, 1.0);
  }

  void Process(sample_t &left, sample_t &right) {
    if (mBufferL.empty() || mBufferR.empty()) {
      return;
    }

    constexpr double kTwoPi = 6.283185307179586;
    constexpr double kHalfPi = 1.5707963267948966;
    const double baseDelayMs = 15.0;
    const double depthMs = 8.0 * mDepth;
    const double phaseInc = (kTwoPi * mRateHz) / mSampleRate;

    double lfoL = std::sin(mPhase);
    double lfoR = std::sin(mPhase + kHalfPi);

    double delaySamplesL =
        (baseDelayMs + depthMs * (0.5 * (1.0 + lfoL))) * 0.001 * mSampleRate;
    double delaySamplesR =
        (baseDelayMs + depthMs * (0.5 * (1.0 + lfoR))) * 0.001 * mSampleRate;

    sample_t delayedL = ReadDelayedSample(mBufferL, delaySamplesL, mWriteIndex);
    sample_t delayedR = ReadDelayedSample(mBufferR, delaySamplesR, mWriteIndex);

    const double dryMix = 1.0 - mMix;
    left = static_cast<sample_t>(left * dryMix + delayedL * mMix);
    right = static_cast<sample_t>(right * dryMix + delayedR * mMix);

    mBufferL[mWriteIndex] = left;
    mBufferR[mWriteIndex] = right;

    mWriteIndex = (mWriteIndex + 1) % static_cast<int>(mBufferL.size());
    mPhase += phaseInc;
    if (mPhase >= kTwoPi) {
      mPhase -= kTwoPi;
    }
  }

private:
  static sample_t ReadDelayedSample(const std::vector<double> &buffer,
                                    double delaySamples, int writeIndex) {
    const int size = static_cast<int>(buffer.size());
    double readPos = static_cast<double>(writeIndex) - delaySamples;
    while (readPos < 0.0) {
      readPos += size;
    }

    int idx0 = static_cast<int>(readPos) % size;
    int idx1 = (idx0 + 1) % size;
    double frac = readPos - static_cast<double>(idx0);
    return static_cast<sample_t>(buffer[idx0] * (1.0 - frac) +
                                 buffer[idx1] * frac);
  }

  double mSampleRate = 44100.0;
  double mRateHz = 0.25;
  double mDepth = 0.5;
  double mMix = 0.0;
  double mPhase = 0.0;
  int mWriteIndex = 0;
  std::vector<double> mBufferL;
  std::vector<double> mBufferR;
};

class StereoDelay {
public:
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    const int maxDelaySamples = static_cast<int>(sampleRate * 2.0); // 2 sec
    mBufferL.assign(maxDelaySamples + 1, 0.0);
    mBufferR.assign(maxDelaySamples + 1, 0.0);
    mWriteIndex = 0;
  }

  void Reset() {
    std::fill(mBufferL.begin(), mBufferL.end(), 0.0);
    std::fill(mBufferR.begin(), mBufferR.end(), 0.0);
    mWriteIndex = 0;
  }

  void SetParams(double delayTimeSec, double feedback, double mix) {
    mDelayTimeSec = std::clamp(delayTimeSec, 0.01, 2.0);
    mFeedback = std::clamp(feedback, 0.0, 0.95);
    mMix = std::clamp(mix, 0.0, 1.0);
  }

  void SetTempoSync(double bpm, double division) {
    if (bpm <= 0.0) {
      return;
    }
    double beatSec = 60.0 / bpm;
    SetParams(beatSec * division, mFeedback, mMix);
  }

  void Process(sample_t &left, sample_t &right) {
    if (mBufferL.empty() || mBufferR.empty()) {
      return;
    }

    const int delaySamples = static_cast<int>(mDelayTimeSec * mSampleRate);
    const int size = static_cast<int>(mBufferL.size());
    const int readIndex = (mWriteIndex - delaySamples + size) % size;

    sample_t delayedL = static_cast<sample_t>(mBufferL[readIndex]);
    sample_t delayedR = static_cast<sample_t>(mBufferR[readIndex]);

    const double dryMix = 1.0 - mMix;
    sample_t outL = static_cast<sample_t>(left * dryMix + delayedL * mMix);
    sample_t outR = static_cast<sample_t>(right * dryMix + delayedR * mMix);

    mBufferL[mWriteIndex] = left + delayedL * mFeedback;
    mBufferR[mWriteIndex] = right + delayedR * mFeedback;

    left = outL;
    right = outR;
    mWriteIndex = (mWriteIndex + 1) % size;
  }

private:
  double mSampleRate = 44100.0;
  double mDelayTimeSec = 0.35;
  double mFeedback = 0.35;
  double mMix = 0.0;
  int mWriteIndex = 0;
  std::vector<double> mBufferL;
  std::vector<double> mBufferR;
};

class LookaheadLimiter {
public:
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    UpdateLookaheadBuffer();
    Reset();
  }

  void Reset() {
    std::fill(mBufferL.begin(), mBufferL.end(), 0.0);
    std::fill(mBufferR.begin(), mBufferR.end(), 0.0);
    mWriteIndex = 0;
    mSampleIndex = 0;
    mPeakWindow.clear();
    mGain = 1.0;
  }

  void SetParams(double threshold, double lookaheadMs, double releaseMs) {
    mThreshold = std::clamp(threshold, 0.01, 1.0);
    mReleaseMs = std::clamp(releaseMs, 1.0, 500.0);

    double newLookahead = std::clamp(lookaheadMs, 0.0, 50.0);
    if (newLookahead != mLookaheadMs) {
      mLookaheadMs = newLookahead;
      UpdateLookaheadBuffer();
    }
  }

  void Process(sample_t &left, sample_t &right) {
    if (mBufferL.empty() || mBufferR.empty()) {
      return;
    }

    const double peak = std::max(std::abs(left), std::abs(right));
    const int window = static_cast<int>(mBufferL.size());

    while (!mPeakWindow.empty() && mPeakWindow.back().value <= peak) {
      mPeakWindow.pop_back();
    }
    mPeakWindow.push_back({mSampleIndex, peak});

    const int64_t minIndex = mSampleIndex - (window - 1);
    while (!mPeakWindow.empty() && mPeakWindow.front().index < minIndex) {
      mPeakWindow.pop_front();
    }

    const double maxPeak =
        mPeakWindow.empty() ? 0.0 : mPeakWindow.front().value;
    const double targetGain =
        maxPeak > mThreshold ? (mThreshold / maxPeak) : 1.0;

    if (targetGain < mGain) {
      mGain = targetGain;
    } else {
      const double releaseCoeff =
          1.0 - std::exp(-1.0 / (mReleaseMs * 0.001 * mSampleRate));
      mGain = mGain + (targetGain - mGain) * releaseCoeff;
    }

    const int size = static_cast<int>(mBufferL.size());
    const int readIndex = (mWriteIndex + 1) % size;

    sample_t outL = static_cast<sample_t>(mBufferL[readIndex] * mGain);
    sample_t outR = static_cast<sample_t>(mBufferR[readIndex] * mGain);

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
    double value;
  };

  void UpdateLookaheadBuffer() {
    const int samples =
        std::max(1, static_cast<int>(mLookaheadMs * 0.001 * mSampleRate));
    mBufferL.assign(samples, 0.0);
    mBufferR.assign(samples, 0.0);
  }

  double mSampleRate = 44100.0;
  double mThreshold = 0.95;
  double mLookaheadMs = 5.0;
  double mReleaseMs = 50.0;
  int mWriteIndex = 0;
  int64_t mSampleIndex = 0;
  double mGain = 1.0;
  std::deque<PeakEntry> mPeakWindow;
  std::vector<double> mBufferL;
  std::vector<double> mBufferR;
};

class FXEngine {
public:
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    mChorus.Init(sampleRate);
    mDelay.Init(sampleRate);
    mLimiter.Init(sampleRate);
  }

  void Reset() {
    mChorus.Reset();
    mDelay.Reset();
    mLimiter.Reset();
  }

  void SetChorus(double rateHz, double depth, double mix) {
    mChorus.SetParams(rateHz, depth, mix);
  }

  void SetDelay(double timeSec, double feedback, double mix) {
    mDelay.SetParams(timeSec, feedback, mix);
  }

  void SetDelayTempo(double bpm, double division) {
    mDelay.SetTempoSync(bpm, division);
  }

  void SetLimiter(double threshold, double lookaheadMs, double releaseMs) {
    mLimiter.SetParams(threshold, lookaheadMs, releaseMs);
  }

  void Process(sample_t &left, sample_t &right) {
    mChorus.Process(left, right);
    mDelay.Process(left, right);
    mLimiter.Process(left, right);
  }

private:
  double mSampleRate = 44100.0;
  Chorus mChorus;
  StereoDelay mDelay;
  LookaheadLimiter mLimiter;
};

} // namespace PolySynthCore
