#pragma once

#include "SynthState.h"
#include "VoiceManager.h"
#include "types.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <sea_dsp/effects/sea_vintage_chorus.h>
#include <sea_dsp/effects/sea_vintage_delay.h>
#include <sea_dsp/sea_limiter.h>
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
    mChorus.Init(sampleRate);
    mDelay.Init(sampleRate, 2000.0);
    mLimiter.Init(sampleRate);
    Reset();
  }

  void Reset() {
    mVoiceManager.Reset();
    mDelay.Clear();
    mLimiter.Reset();
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
                           state.oscBFineTune * 100.0);

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

    SetChorus(state.fxChorusRate, state.fxChorusDepth, state.fxChorusMix);
    SetDelay(state.fxDelayTime, state.fxDelayFeedback, state.fxDelayMix);
    SetLimiter(state.fxLimiterThreshold, 5.0, 50.0);
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
  void SetChorus(sample_t rateHz, sample_t depth, sample_t mix) {
    mChorus.SetRate(rateHz);
    mChorus.SetDepth(depth * 5.0); // Map 0-1 to 0-5ms
    mChorus.SetMix(mix);
  }
  void SetDelay(sample_t timeSec, sample_t feedback, sample_t mix) {
    mDelay.SetTime(timeSec * 1000.0);
    mDelay.SetFeedback(feedback * 100.0);
    mDelay.SetMix(mix * 100.0);
  }
  void SetDelayTempo(sample_t bpm, sample_t division) {
    // Basic fallback for now
    sample_t beatSec = 60.0 / bpm;
    SetDelay(beatSec * division, 35.0, 0.0);
  }
  void SetLimiter(sample_t threshold, sample_t lookaheadMs, sample_t releaseMs) {
    // Threshold should be mapped inverted if coming from UI,
    // but here we just pass it through as the engine setter.
    mLimiter.SetParams(threshold, lookaheadMs, releaseMs);
  }

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

    left = l;
    right = r;

    sample_t fxL, fxR;
    mChorus.Process(left, right, &fxL, &fxR);
    mDelay.Process(fxL, fxR, &fxL, &fxR);
    mLimiter.Process(fxL, fxR);

    left = fxL;
    right = fxR;
  }

  void Process(sample_t ** /*inputs*/, sample_t **outputs, int nFrames,
               int nChans) {
    for (int i = 0; i < nFrames; ++i) {
      sample_t l, r;
      mVoiceManager.ProcessStereo(l, r);

      l *= mGain;
      r *= mGain;

      sample_t left = l;
      sample_t right = r;

      sample_t fxL, fxR;
      mChorus.Process(left, right, &fxL, &fxR);
      mDelay.Process(fxL, fxR, &fxL, &fxR);
      mLimiter.Process(fxL, fxR);

      if (nChans > 0)
        outputs[0][i] = fxL;
      if (nChans > 1)
        outputs[1][i] = fxR;
    }

    UpdateVisualization();
  }

private:
  double mSampleRate;
  sample_t mGain = 1.0;
  VoiceManager mVoiceManager;
  sea::VintageChorus<sample_t> mChorus;
  sea::VintageDelay<sample_t> mDelay;
  sea::LookaheadLimiter<sample_t> mLimiter;

  // Visualization state (written by Audio thread, read by UI thread)
  std::atomic<int> mVisualActiveVoiceCount{0};
  std::atomic<uint64_t> mVisualHeldNotesLow{0};
  std::atomic<uint64_t> mVisualHeldNotesHigh{0};
};

} // namespace PolySynthCore
