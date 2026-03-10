#pragma once

#include "DspConstants.h"
#include "SynthState.h"
#include "VoiceManager.h"
#include "types.h"
#include <array>
#include <atomic>
#include <cstdint>
// Deploy guards: default ON so desktop/WASM builds are unaffected.
// Pico CMakeLists sets these to 0 to save SRAM.
#ifndef POLYSYNTH_DEPLOY_CHORUS
#define POLYSYNTH_DEPLOY_CHORUS 1
#endif
#ifndef POLYSYNTH_DEPLOY_DELAY
#define POLYSYNTH_DEPLOY_DELAY 1
#endif
#ifndef POLYSYNTH_DEPLOY_LIMITER
#define POLYSYNTH_DEPLOY_LIMITER 1
#endif

#if POLYSYNTH_DEPLOY_CHORUS
#include <sea_dsp/effects/sea_vintage_chorus.h>
#endif
#if POLYSYNTH_DEPLOY_DELAY
#include <sea_dsp/effects/sea_vintage_delay.h>
#endif
#if POLYSYNTH_DEPLOY_LIMITER
#include <sea_dsp/sea_limiter.h>
#endif
#include <string.h>

namespace PolySynthCore {

class Engine {
public:
  Engine() : mSampleRate(44100.0) {}
  ~Engine() = default;

  // --- Lifecycle ---
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    mVoiceManager.Init(sampleRate);
#if POLYSYNTH_DEPLOY_CHORUS
    mChorus.Init(sampleRate);
#endif
#if POLYSYNTH_DEPLOY_DELAY
    mDelay.Init(sampleRate, kMaxDelayMs);
#endif
#if POLYSYNTH_DEPLOY_LIMITER
    mLimiter.Init(sampleRate);
#endif
    Reset();
  }

  void Reset() {
    mVoiceManager.Reset();
#if POLYSYNTH_DEPLOY_DELAY
    mDelay.Clear();
#endif
#if POLYSYNTH_DEPLOY_LIMITER
    mLimiter.Reset();
#endif
  }

  // --- Events ---
  void OnNoteOn(int note, int velocity) {
    mVoiceManager.OnNoteOn(note, velocity);
  }

  void OnNoteOff(int note) { mVoiceManager.OnNoteOff(note); }

  void SetParameter(int /*paramNum*/, sample_t /*value*/) {
    // Basic param dispatch could go here
  }

  // --- State Update (parameter fan-out) ---
  void UpdateState(const SynthState& state) {
    mGain = state.masterGain;

    mVoiceManager.SetADSR(state.ampAttack, state.ampDecay, state.ampSustain,
                          state.ampRelease);
    mVoiceManager.SetFilterEnv(state.filterAttack, state.filterDecay,
                               state.filterSustain, state.filterRelease);
    mVoiceManager.SetFilter(state.filterCutoff, state.filterResonance,
                            state.filterEnvAmount);
    mVoiceManager.SetFilterModel(state.filterModel);

    mVoiceManager.SetWaveformA(
        static_cast<sea::Oscillator::WaveformType>(state.oscAWaveform));
    mVoiceManager.SetWaveformB(
        static_cast<sea::Oscillator::WaveformType>(state.oscBWaveform));
    mVoiceManager.SetPulseWidthA(state.oscAPulseWidth);
    mVoiceManager.SetPulseWidthB(state.oscBPulseWidth);
    mVoiceManager.SetMixer(state.mixOscA, state.mixOscB,
                           state.oscBFineTune * kFineTuneToCents);

    mVoiceManager.SetLFO(state.lfoShape, state.lfoRate, state.lfoDepth);

    mVoiceManager.SetPolyModOscBToFreqA(state.polyModOscBToFreqA);
    mVoiceManager.SetPolyModOscBToPWM(state.polyModOscBToPWM);
    mVoiceManager.SetPolyModOscBToFilter(state.polyModOscBToFilter);
    mVoiceManager.SetPolyModFilterEnvToFreqA(state.polyModFilterEnvToFreqA);
    mVoiceManager.SetPolyModFilterEnvToPWM(state.polyModFilterEnvToPWM);
    mVoiceManager.SetPolyModFilterEnvToFilter(state.polyModFilterEnvToFilter);

    mVoiceManager.SetGlideTime(state.glideTime);
    mVoiceManager.SetPolyphonyLimit(state.polyphony);
    mVoiceManager.SetAllocationMode(state.allocationMode);
    mVoiceManager.SetStealPriority(state.stealPriority);
    mVoiceManager.SetUnisonCount(state.unisonCount);
    mVoiceManager.SetUnisonSpread(state.unisonSpread);
    mVoiceManager.SetStereoSpread(state.stereoSpread);

#if POLYSYNTH_DEPLOY_CHORUS
    SetChorus(state.fxChorusRate, state.fxChorusDepth, state.fxChorusMix);
#endif
#if POLYSYNTH_DEPLOY_DELAY
    SetDelay(state.fxDelayTime, state.fxDelayFeedback, state.fxDelayMix);
#endif
#if POLYSYNTH_DEPLOY_LIMITER
    SetLimiter(state.fxLimiterThreshold, kLimiterLookaheadMs, kLimiterReleaseMs);
#endif
  }

  // --- High Level Setters ---
  void SetADSR(sample_t a, sample_t d, sample_t s, sample_t r) {
    mVoiceManager.SetADSR(a, d, s, r);
  }
  void SetFilterEnv(sample_t a, sample_t d, sample_t s, sample_t r) {
    mVoiceManager.SetFilterEnv(a, d, s, r);
  }
  void SetFilter(sample_t cutoff, sample_t res, sample_t envAmount) {
    mVoiceManager.SetFilter(cutoff, res, envAmount);
  }
  void SetWaveform(sea::Oscillator::WaveformType type) {
    mVoiceManager.SetWaveform(type);
  }
  void SetWaveformA(sea::Oscillator::WaveformType type) {
    mVoiceManager.SetWaveformA(type);
  }
  void SetWaveformB(sea::Oscillator::WaveformType type) {
    mVoiceManager.SetWaveformB(type);
  }

  void SetPulseWidth(sample_t pw) { mVoiceManager.SetPulseWidth(pw); }
  void SetPulseWidthA(sample_t pw) { mVoiceManager.SetPulseWidthA(pw); }
  void SetPulseWidthB(sample_t pw) { mVoiceManager.SetPulseWidthB(pw); }

  void SetMixer(sample_t mixA, sample_t mixB, sample_t detuneB) {
    mVoiceManager.SetMixer(mixA, mixB, detuneB);
  }

  void SetLFO(int type, sample_t rate, sample_t depth) {
    mVoiceManager.SetLFO(type, rate, depth);
  }
  void SetLFORouting(sample_t pitch, sample_t filter, sample_t amp,
                     sample_t pan = 0.0) {
    mVoiceManager.SetLFORouting(pitch, filter, amp, pan);
  }

  void SetPolyModOscBToFreqA(sample_t amount) {
    mVoiceManager.SetPolyModOscBToFreqA(amount);
  }
  void SetPolyModOscBToPWM(sample_t amount) {
    mVoiceManager.SetPolyModOscBToPWM(amount);
  }
  void SetPolyModOscBToFilter(sample_t amount) {
    mVoiceManager.SetPolyModOscBToFilter(amount);
  }
  void SetPolyModFilterEnvToFreqA(sample_t amount) {
    mVoiceManager.SetPolyModFilterEnvToFreqA(amount);
  }
  void SetPolyModFilterEnvToPWM(sample_t amount) {
    mVoiceManager.SetPolyModFilterEnvToPWM(amount);
  }
  void SetPolyModFilterEnvToFilter(sample_t amount) {
    mVoiceManager.SetPolyModFilterEnvToFilter(amount);
  }

  // --- VoiceManager Forwarding Setters ---
  void SetGlideTime(sample_t t) { mVoiceManager.SetGlideTime(t); }
  void SetPolyphonyLimit(int n) { mVoiceManager.SetPolyphonyLimit(n); }
  void SetAllocationMode(int mode) { mVoiceManager.SetAllocationMode(mode); }
  void SetStealPriority(int priority) { mVoiceManager.SetStealPriority(priority); }
  void SetUnisonCount(int count) { mVoiceManager.SetUnisonCount(count); }
  void SetUnisonSpread(sample_t spread) { mVoiceManager.SetUnisonSpread(spread); }
  void SetStereoSpread(sample_t spread) { mVoiceManager.SetStereoSpread(spread); }
  void OnSustainPedal(bool down) { mVoiceManager.OnSustainPedal(down); }
  void SetFilterModel(int model) { mVoiceManager.SetFilterModel(model); }

  // --- FX Setters ---
#if POLYSYNTH_DEPLOY_CHORUS
  void SetChorus(sample_t rateHz, sample_t depth, sample_t mix) {
    mChorus.SetRate(rateHz);
    mChorus.SetDepth(depth * kChorusDepthMs);
    mChorus.SetMix(mix);
  }
#endif
#if POLYSYNTH_DEPLOY_DELAY
  void SetDelay(sample_t timeSec, sample_t feedback, sample_t mix) {
    mDelay.SetTime(timeSec * kDelayTimeToMs);
    mDelay.SetFeedback(feedback * kDelayFeedbackScale);
    mDelay.SetMix(mix * kDelayMixScale);
  }
  void SetDelayTempo(sample_t bpm, sample_t division) {
    // Basic fallback for now
    sample_t beatSec = 60.0 / bpm;
    SetDelay(beatSec * division, kTempoDelayFeedbackPct, 0.0);
  }
#endif
#if POLYSYNTH_DEPLOY_LIMITER
  void SetLimiter(sample_t threshold, sample_t lookaheadMs, sample_t releaseMs) {
    mLimiter.SetParams(threshold, lookaheadMs, releaseMs);
  }
#endif

  // --- Visualization Accessors ---
  int GetActiveVoiceCount() const {
    return mVisualActiveVoiceCount.load(std::memory_order_relaxed);
  }

  int GetHeldNotes(std::array<int, kMaxVoices>& buf) const {
    uint64_t low = mVisualHeldNotesLow.load(std::memory_order_relaxed);
    uint64_t high = mVisualHeldNotesHigh.load(std::memory_order_relaxed);
    int count = 0;
    for (int i = 0; i < 64; ++i) {
      if ((low >> i) & 1) {
        if (count < kMaxVoices) buf[count++] = i;
      }
    }
    for (int i = 0; i < 64; ++i) {
      if ((high >> i) & 1) {
        if (count < kMaxVoices) buf[count++] = i + 64;
      }
    }
    return count;
  }

  void UpdateVisualization() {
    mVisualActiveVoiceCount.store(mVoiceManager.GetActiveVoiceCount(),
                                  std::memory_order_relaxed);
    std::array<int, kMaxVoices> notes{};
    int count = mVoiceManager.GetHeldNotes(notes);
    uint64_t low = 0, high = 0;
    for (int i = 0; i < count; ++i) {
      int note = notes[i];
      if (note >= 0 && note < 64)
        low |= (1ULL << note);
      else if (note >= 64 && note < 128)
        high |= (1ULL << (note - 64));
    }
    mVisualHeldNotesLow.store(low, std::memory_order_relaxed);
    mVisualHeldNotesHigh.store(high, std::memory_order_relaxed);
  }

  // --- Audio Processing ---
  void Process(sample_t &left, sample_t &right) {
    sample_t l, r;
    mVoiceManager.ProcessStereo(l, r);

    l *= mGain;
    r *= mGain;

#if POLYSYNTH_DEPLOY_CHORUS || POLYSYNTH_DEPLOY_DELAY || POLYSYNTH_DEPLOY_LIMITER
    sample_t fxL = l, fxR = r;
#if POLYSYNTH_DEPLOY_CHORUS
    mChorus.Process(fxL, fxR, &fxL, &fxR);
#endif
#if POLYSYNTH_DEPLOY_DELAY
    mDelay.Process(fxL, fxR, &fxL, &fxR);
#endif
#if POLYSYNTH_DEPLOY_LIMITER
    mLimiter.Process(fxL, fxR);
#endif
    left = fxL;
    right = fxR;
#else
    left = l;
    right = r;
#endif
  }

  void Process(sample_t ** /*inputs*/, sample_t **outputs, int nFrames,
               int nChans) {
    for (int i = 0; i < nFrames; ++i) {
      sample_t l, r;
      mVoiceManager.ProcessStereo(l, r);

      l *= mGain;
      r *= mGain;

#if POLYSYNTH_DEPLOY_CHORUS || POLYSYNTH_DEPLOY_DELAY || POLYSYNTH_DEPLOY_LIMITER
      sample_t fxL = l, fxR = r;
#if POLYSYNTH_DEPLOY_CHORUS
      mChorus.Process(fxL, fxR, &fxL, &fxR);
#endif
#if POLYSYNTH_DEPLOY_DELAY
      mDelay.Process(fxL, fxR, &fxL, &fxR);
#endif
#if POLYSYNTH_DEPLOY_LIMITER
      mLimiter.Process(fxL, fxR);
#endif
      if (nChans > 0)
        outputs[0][i] = fxL;
      if (nChans > 1)
        outputs[1][i] = fxR;
#else
      if (nChans > 0)
        outputs[0][i] = l;
      if (nChans > 1)
        outputs[1][i] = r;
#endif
    }

    UpdateVisualization();
  }

private:
  double mSampleRate;
  sample_t mGain = 1.0;
  VoiceManager mVoiceManager;
#if POLYSYNTH_DEPLOY_CHORUS
  sea::VintageChorus<sample_t> mChorus;
#endif
#if POLYSYNTH_DEPLOY_DELAY
  sea::VintageDelay<sample_t> mDelay;
#endif
#if POLYSYNTH_DEPLOY_LIMITER
  sea::LookaheadLimiter<sample_t> mLimiter;
#endif

  // Visualization state (written by Audio thread, read by UI thread)
  std::atomic<int> mVisualActiveVoiceCount{0};
  std::atomic<uint64_t> mVisualHeldNotesLow{0};
  std::atomic<uint64_t> mVisualHeldNotesHigh{0};
};

} // namespace PolySynthCore
