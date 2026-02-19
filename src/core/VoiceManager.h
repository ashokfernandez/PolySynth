#pragma once

#include "types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sea_dsp/sea_adsr.h>
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_cascade_filter.h>
#include <sea_dsp/sea_ladder_filter.h>
#include <sea_dsp/sea_lfo.h>
#include <sea_dsp/sea_oscillator.h>
#include <sea_util/sea_voice_allocator.h>

namespace PolySynthCore {

class Voice {
public:
  enum class FilterModel { Classic, Ladder, Cascade12, Cascade24 };

  Voice() = default;

  void Init(sample_t sampleRate, uint8_t voiceID = 0) {
    mOscA.Init(sampleRate);
    mOscB.Init(sampleRate);
    mOscA.SetFrequency(440.0);
    mOscB.SetFrequency(440.0 * std::pow(2.0, mDetuneB / 1200.0));
    mTargetFreq = 440.0;
    mGlideTime = 0.0;

    mBasePulseWidthA = 0.5;
    mBasePulseWidthB = 0.5;
    mOscA.SetPulseWidth(mBasePulseWidthA);
    mOscB.SetPulseWidth(mBasePulseWidthB);

    mFilter.Init(sampleRate);
    mFilter.SetParams(sea::FilterType::LowPass, 2000.0, 0.707);
    mLadderFilter.Init(sampleRate);
    mCascadeFilter.Init(sampleRate);

    mAmpEnv.Init(sampleRate);
    mAmpEnv.SetParams(0.01, 0.1, 1.0, 0.2);

    mFilterEnv.Init(sampleRate);
    mFilterEnv.SetParams(0.01, 0.1, 0.5, 0.2);

    mLfo.Init(sampleRate);
    mLfo.SetRate(5.0);
    mLfo.SetDepth(0.0);
    mLfo.SetWaveform(0);

    mActive = false;
    mNote = -1;
    mAge = 0;
    mSampleRate = sampleRate;
    mVoiceID = voiceID;
    mVoiceState = VoiceState::Idle;
    mTimestamp = 0;
    mCurrentPitch = 0.0f;
    mPanPosition = 0.0f;
    mStolenFadeGain = 0.0f;
    mStolenFadeDelta = 0.0;
    mLastAmpEnvVal = 0.0f;
    mBaseCutoff = 2000.0;
    mBaseRes = 0.707;
    mPolyModOscBToFreqA = 0.0;
    mPolyModOscBToPWM = 0.0;
    mPolyModOscBToFilter = 0.0;
    mPolyModFilterEnvToFreqA = 0.0;
    mPolyModFilterEnvToPWM = 0.0;
    mPolyModFilterEnvToFilter = 0.0;
    mFilterModel = FilterModel::Ladder;
  }

  void NoteOn(int note, int velocity, uint32_t timestamp = 0) {
    sample_t newFreq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
    mTargetFreq = newFreq;

    if (mGlideTime > 0.0 && mActive) {
      // Legato glide: keep current frequency, glide to target
      // Don't reset oscillators — smooth transition
    } else {
      // Normal retrigger: jump to frequency immediately
      mFreq = newFreq;
      mOscA.SetFrequency(mFreq);
      mOscB.SetFrequency(mFreq * std::pow(2.0, mDetuneB / 1200.0));
      mOscA.Reset();
      mOscB.Reset();
    }

    mVelocity = velocity / 127.0;

    mAmpEnv.NoteOn();
    mFilterEnv.NoteOn();
    mActive = true;
    mNote = note;
    mAge = 0;
    mVoiceState = VoiceState::Attack;
    mTimestamp = timestamp;
    mCurrentPitch = static_cast<float>(mFreq);
  }

  void NoteOff() {
    mAmpEnv.NoteOff();
    mFilterEnv.NoteOff();
    mVoiceState = VoiceState::Release;
  }

  void ApplyDetuneCents(sample_t cents) {
    sample_t factor = std::pow(2.0, cents / 1200.0);
    mFreq *= factor;
    mCurrentPitch = static_cast<float>(mFreq);
    mOscA.SetFrequency(mFreq);
    mOscB.SetFrequency(mFreq * std::pow(2.0, mDetuneB / 1200.0));
  }

  inline sample_t Process() {
    if (!mActive)
      return 0.0;

    if (mVoiceState == VoiceState::Stolen && mStolenFadeGain <= 0.0f) {
      mActive = false;
      mNote = -1;
      mVoiceState = VoiceState::Idle;
      mAge = 0;
      mLastAmpEnvVal = 0.0f;
      return 0.0;
    }

    sample_t lfoVal = mLfo.Process();
    sample_t filterEnvVal = mFilterEnv.Process();

    // Portamento: glide toward target frequency
    if (mGlideTime > 0.0 && std::abs(mFreq - mTargetFreq) > 0.01) {
      sample_t alpha = 1.0 - std::exp(-3.0 / (mGlideTime * mSampleRate));
      mFreq += (mTargetFreq - mFreq) * alpha;
      // Snap to target when close enough
      if (std::abs(mFreq - mTargetFreq) < 0.01) {
        mFreq = mTargetFreq;
      }
      mCurrentPitch = static_cast<float>(mFreq);
      // Update oscillator base frequencies (will be modulated below)
      // Note: We don't set mOscA/B immediate here because they get set below
      // with modulation
    }

    // Pitch Modulation (Vibrato)
    sample_t modFreqA = mFreq;
    sample_t modFreqB = mFreq * std::pow(2.0, mDetuneB / 1200.0);

    if (mLfoPitchDepth > 0.0) {
      sample_t modMult = (1.0 + lfoVal * mLfoPitchDepth * 0.05);
      modFreqA *= modMult;
      modFreqB *= modMult;
    }

    mOscB.SetFrequency(modFreqB);
    sample_t oscB = mOscB.Process();

    if (mPolyModOscBToFreqA != 0.0 || mPolyModFilterEnvToFreqA != 0.0) {
      sample_t freqMod = (oscB * mPolyModOscBToFreqA) +
                         (filterEnvVal * mPolyModFilterEnvToFreqA);
      modFreqA *= (1.0 + freqMod);
      modFreqA = std::max(1.0, static_cast<double>(modFreqA));
    }

    mOscA.SetFrequency(modFreqA);

    if (mPolyModOscBToPWM != 0.0 || mPolyModFilterEnvToPWM != 0.0) {
      sample_t pwmMod =
          (oscB * mPolyModOscBToPWM) + (filterEnvVal * mPolyModFilterEnvToPWM);
      sample_t pwmA = mBasePulseWidthA + (pwmMod * 0.5);
      mOscA.SetPulseWidth(std::clamp(pwmA, 0.01, 0.99));
    } else {
      mOscA.SetPulseWidth(mBasePulseWidthA);
    }

    sample_t oscA = mOscA.Process();
    sample_t mixed = (oscA * mMixA) + (oscB * mMixB);

    // Filter Modulation: Base + Keyboard + Env + LFO
    sample_t cutoff = mBaseCutoff;
    cutoff +=
        filterEnvVal * (mFilterEnvAmount + mPolyModFilterEnvToFilter) * 10000.0;
    if (mPolyModOscBToFilter != 0.0) {
      cutoff += oscB * mPolyModOscBToFilter * mBaseCutoff;
    }
    cutoff *= (1.0 + lfoVal * mLfoFilterDepth);
    cutoff = std::clamp(cutoff, 20.0, 20000.0);

    sample_t flt = 0.0;
    switch (mFilterModel) {
    case FilterModel::Ladder:
      mLadderFilter.SetParams(sea::LadderFilter<sample_t>::Model::Transistor,
                              cutoff, mBaseRes);
      flt = mLadderFilter.Process(mixed);
      break;
    case FilterModel::Cascade12:
      mCascadeFilter.SetParams(cutoff, mBaseRes,
                               sea::CascadeFilter<sample_t>::Slope::dB12);
      flt = mCascadeFilter.Process(mixed);
      break;
    case FilterModel::Cascade24:
      mCascadeFilter.SetParams(cutoff, mBaseRes,
                               sea::CascadeFilter<sample_t>::Slope::dB24);
      flt = mCascadeFilter.Process(mixed);
      break;
    case FilterModel::Classic:
    default:
      mFilter.SetParams(sea::FilterType::LowPass, cutoff, mBaseRes);
      flt = mFilter.Process(mixed);
      break;
    }

    sample_t ampEnvVal = mAmpEnv.Process();
    mLastAmpEnvVal = static_cast<float>(ampEnvVal);

    // Amp Modulation (Tremolo)
    sample_t ampMod = 1.0;
    if (mLfoAmpDepth > 0.0) {
      ampMod = 1.0 + lfoVal * mLfoAmpDepth;
      ampMod = std::clamp(ampMod, 0.0, 2.0);
    }

    mAge++;

    if (!mAmpEnv.IsActive() && !mFilterEnv.IsActive()) {
      mActive = false;
      mNote = -1;
      mAge = 0;
      mVoiceState = VoiceState::Idle;
    }

    sample_t out = flt * ampEnvVal * mVelocity * ampMod;
    if (mVoiceState == VoiceState::Stolen) {
      const float gain = std::max(0.0f, mStolenFadeGain);
      out *= gain;
      mStolenFadeGain = std::max(
          0.0f, mStolenFadeGain - static_cast<float>(mStolenFadeDelta));
      if (mStolenFadeGain <= 0.0f) {
        mActive = false;
        mNote = -1;
        mVoiceState = VoiceState::Idle;
        mAge = 0;
        mLastAmpEnvVal = 0.0f;
      }
    }
    return out;
  }

  bool IsActive() const { return mActive; }
  int GetNote() const { return mNote; }
  uint32_t GetAge() const { return mAge; }

  void StartSteal() {
    mStolenFadeGain = 1.0f;
    // Increase fade to 20ms to prevent clicks/crunch when reducing polyphony
    const sample_t fadeSamples = std::max(1.0, 0.020 * mSampleRate);
    mStolenFadeDelta = 1.0 / fadeSamples;
    mVoiceState = VoiceState::Stolen;
  }

  VoiceState GetVoiceState() const { return mVoiceState; }
  uint8_t GetVoiceID() const { return mVoiceID; }
  uint32_t GetTimestamp() const { return mTimestamp; }
  float GetPitch() const { return mCurrentPitch; }
  float GetPanPosition() const { return mPanPosition; }
  void SetPanPosition(float pan) {
    mPanPosition = std::clamp(pan, -1.0f, 1.0f);
  }

  VoiceRenderState GetRenderState() const {
    VoiceRenderState rs;
    rs.voiceID = mVoiceID;
    rs.state = mVoiceState;
    rs.note = mNote;
    rs.velocity = static_cast<int>(mVelocity * 127.0);
    rs.currentPitch = mCurrentPitch;
    rs.panPosition = mPanPosition;
    rs.amplitude = mLastAmpEnvVal;
    return rs;
  }

  void SetGlideTime(sample_t seconds) { mGlideTime = std::max(0.0, seconds); }

  void SetADSR(sample_t a, sample_t d, sample_t s, sample_t r) {
    mAmpEnv.SetParams(a, d, s, r);
  }

  void SetFilterEnv(sample_t a, sample_t d, sample_t s, sample_t r) {
    mFilterEnv.SetParams(a, d, s, r);
  }

  void SetFilter(sample_t cutoff, sample_t res, sample_t envAmount) {
    mBaseCutoff = cutoff;
    // Map 0-1 resonance to 0.5-20.5 Q for the biquad filter
    mBaseRes = 0.5 + (res * res * 20.0);
    mFilterEnvAmount = envAmount;
  }

  void SetFilterModel(FilterModel model) { mFilterModel = model; }

  void SetWaveform(sea::Oscillator::WaveformType type) { SetWaveformA(type); }
  void SetWaveformA(sea::Oscillator::WaveformType type) {
    mOscA.SetWaveform(type);
  }
  void SetWaveformB(sea::Oscillator::WaveformType type) {
    mOscB.SetWaveform(type);
  }

  void SetPulseWidth(sample_t pw) { SetPulseWidthA(pw); }
  void SetPulseWidthA(sample_t pw) {
    mBasePulseWidthA = std::clamp(pw, 0.01, 0.99);
    mOscA.SetPulseWidth(mBasePulseWidthA);
  }
  void SetPulseWidthB(sample_t pw) {
    mBasePulseWidthB = std::clamp(pw, 0.01, 0.99);
    mOscB.SetPulseWidth(mBasePulseWidthB);
  }

  void SetMixer(sample_t mixA, sample_t mixB, sample_t detuneB) {
    mMixA = std::clamp(mixA, 0.0, 1.0);
    mMixB = std::clamp(mixB, 0.0, 1.0);
    mDetuneB = detuneB;
  }

  void SetLFO(int type, sample_t rate, sample_t depth) {
    mLfo.SetWaveform(type);
    mLfo.SetRate(rate);
    mLfo.SetDepth(depth);
  }

  void SetLFORouting(sample_t pitch, sample_t filter, sample_t amp) {
    mLfoPitchDepth = pitch;
    mLfoFilterDepth = filter;
    mLfoAmpDepth = amp;
  }

  void SetPolyModOscBToFreqA(sample_t amount) { mPolyModOscBToFreqA = amount; }
  void SetPolyModOscBToPWM(sample_t amount) { mPolyModOscBToPWM = amount; }
  void SetPolyModOscBToFilter(sample_t amount) {
    mPolyModOscBToFilter = amount;
  }
  void SetPolyModFilterEnvToFreqA(sample_t amount) {
    mPolyModFilterEnvToFreqA = amount;
  }
  void SetPolyModFilterEnvToPWM(sample_t amount) {
    mPolyModFilterEnvToPWM = amount;
  }
  void SetPolyModFilterEnvToFilter(sample_t amount) {
    mPolyModFilterEnvToFilter = amount;
  }

private:
  sea::Oscillator mOscA;
  sea::Oscillator mOscB;
  sea::BiquadFilter<sample_t> mFilter;
  sea::LadderFilter<sample_t> mLadderFilter;
  sea::CascadeFilter<sample_t> mCascadeFilter;
  sea::ADSREnvelope mAmpEnv;
  sea::ADSREnvelope mFilterEnv;
  sea::LFO mLfo;

  bool mActive = false;
  sample_t mVelocity = 0.0;
  int mNote = -1;
  uint32_t mAge = 0;

  sample_t mFreq = 440.0;
  sample_t mBaseCutoff = 2000.0;
  sample_t mBaseRes = 0.707;
  sample_t mFilterEnvAmount = 0.0;
  FilterModel mFilterModel = FilterModel::Classic;

  sample_t mMixA = 1.0;
  sample_t mMixB = 0.0;
  sample_t mDetuneB = 0.0;
  sample_t mBasePulseWidthA = 0.5;
  sample_t mBasePulseWidthB = 0.5;

  sample_t mLfoPitchDepth = 0.0;
  sample_t mLfoFilterDepth = 0.0;
  sample_t mLfoAmpDepth = 0.0;

  sample_t mPolyModOscBToFreqA = 0.0;
  sample_t mPolyModOscBToPWM = 0.0;
  sample_t mPolyModOscBToFilter = 0.0;
  sample_t mPolyModFilterEnvToFreqA = 0.0;
  sample_t mPolyModFilterEnvToPWM = 0.0;
  sample_t mPolyModFilterEnvToFilter = 0.0;

  uint8_t mVoiceID = 0;
  VoiceState mVoiceState = VoiceState::Idle;
  uint32_t mTimestamp = 0;
  float mCurrentPitch = 0.0f;
  float mPanPosition = 0.0f;
  float mStolenFadeGain = 0.0f;
  sample_t mStolenFadeDelta = 0.0;
  float mLastAmpEnvVal = 0.0f;

  sample_t mTargetFreq = 440.0;
  sample_t mGlideTime = 0.0;

  sample_t mSampleRate = 48000.0;
};

class VoiceManager {
public:
  static constexpr int kNumVoices = kMaxVoices;

  VoiceManager() = default;

  void Init(sample_t sampleRate) {
    mSampleRate = sampleRate;
    mGlobalTimestamp = 0;
    for (int i = 0; i < kNumVoices; i++) {
      mVoices[i].Init(sampleRate, static_cast<uint8_t>(i));
    }
    mAllocator.SetPolyphonyLimit(kNumVoices);
  }

  void Reset() {
    mGlobalTimestamp = 0;
    for (int i = 0; i < kNumVoices; i++) {
      mVoices[i].Init(mSampleRate, static_cast<uint8_t>(i));
    }
  }

  void OnNoteOn(int note, int velocity) {
    int unisonCount = mAllocator.GetUnisonCount();
    for (int u = 0; u < unisonCount; u++) {
      int idx = mAllocator.AllocateSlot(mVoices.data());
      if (idx < 0) {
        idx = mAllocator.FindStealVictim(mVoices.data());
      }
      if (idx < 0)
        break; // No voice available

      mVoices[idx].NoteOn(note, velocity, ++mGlobalTimestamp);

      // Apply unison detune and pan
      auto info = mAllocator.GetUnisonVoiceInfo(u);

      // If we are in non-unison mode, apply stereo spread based on voice index
      // to spread voices across the stereo field.
      if (mAllocator.GetUnisonCount() <= 1 &&
          mAllocator.GetStereoSpread() > 0.0) {
        sample_t spread = mAllocator.GetStereoSpread();
        // Alternate L/R panning per voice slot
        sample_t pan = (idx % 2 == 0) ? -spread : spread;
        info.panPosition = pan;
      }

      if (info.detuneCents != 0.0) {
        mVoices[idx].ApplyDetuneCents(info.detuneCents);
      }
      mVoices[idx].SetPanPosition(static_cast<float>(info.panPosition));
    }
  }

  void OnNoteOff(int note) {
    if (mAllocator.ShouldHold(note)) {
      mAllocator.MarkSustained(note);
      return;
    }
    // Release all voices playing this note
    for (auto &voice : mVoices) {
      if (voice.IsActive() && voice.GetNote() == note) {
        voice.NoteOff();
      }
    }
  }

  void OnSustainPedal(bool down) {
    mAllocator.OnSustainPedal(down);
    if (!down) {
      // Release all sustained notes
      int releasedNotes[128];
      int count = mAllocator.ReleaseSustainedNotes(releasedNotes, 128);
      for (int i = 0; i < count; i++) {
        for (auto &voice : mVoices) {
          if (voice.IsActive() && voice.GetNote() == releasedNotes[i]) {
            voice.NoteOff();
          }
        }
      }
    }
  }

  // Configuration setters (delegate to allocator)
  void SetPolyphonyLimit(int limit) {
    mAllocator.SetPolyphonyLimit(limit);
    // Immediately kill excess voices if active count exceeds new limit
    int killIndices[kMaxVoices];
    mAllocator.EnforcePolyphonyLimit(mVoices.data(), killIndices, kMaxVoices);
  }

  void SetAllocationMode(int mode) {
    mAllocator.SetAllocationMode(static_cast<sea::AllocationMode>(mode));
  }
  void SetStealPriority(int priority) {
    mAllocator.SetStealPriority(static_cast<sea::StealPriority>(priority));
  }
  void SetUnisonCount(int count) { mAllocator.SetUnisonCount(count); }
  void SetUnisonSpread(sample_t spread) { mAllocator.SetUnisonSpread(spread); }
  void SetStereoSpread(sample_t spread) { mAllocator.SetStereoSpread(spread); }

  inline sample_t Process() {
    sample_t sum = 0.0;
    for (auto &voice : mVoices) {
      sum += voice.Process();
    }
    return sum * (1.0 / std::sqrt(static_cast<double>(kNumVoices)));
  }

  inline void ProcessStereo(double &outLeft, double &outRight) {
    outLeft = 0.0;
    outRight = 0.0;

    for (auto &voice : mVoices) {
      sample_t mono = voice.Process();
      if (mono == 0.0)
        continue;

      float pan = voice.GetPanPosition();
      // Constant-power panning: theta maps [-1,+1] to [0, pi/2]
      double theta = (static_cast<double>(pan) + 1.0) * kPi * 0.25;
      outLeft += mono * std::cos(theta);
      outRight += mono * std::sin(theta);
    }
    // Headroom scaling
    double scale = 1.0 / std::sqrt(static_cast<double>(kNumVoices));
    outLeft *= scale;
    outRight *= scale;
  }

  void SetGlideTime(sample_t seconds) {
    for (auto &voice : mVoices) {
      voice.SetGlideTime(seconds);
    }
  }

  void SetADSR(sample_t a, sample_t d, sample_t s, sample_t r) {
    for (auto &voice : mVoices) {
      voice.SetADSR(a, d, s, r);
    }
  }

  void SetFilterEnv(sample_t a, sample_t d, sample_t s, sample_t r) {
    for (auto &voice : mVoices) {
      voice.SetFilterEnv(a, d, s, r);
    }
  }

  void SetFilter(sample_t cutoff, sample_t res, sample_t envAmount) {
    for (auto &voice : mVoices) {
      voice.SetFilter(cutoff, res, envAmount);
    }
  }

  void SetFilterModel(int model) {
    int clamped = std::clamp(model, 0, 3);
    auto filterModel = static_cast<Voice::FilterModel>(clamped);
    for (auto &voice : mVoices) {
      voice.SetFilterModel(filterModel);
    }
  }

  void SetWaveform(sea::Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveform(type);
    }
  }

  void SetWaveformA(sea::Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveformA(type);
    }
  }

  void SetWaveformB(sea::Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveformB(type);
    }
  }

  void SetPulseWidth(sample_t pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidth(pw);
    }
  }

  void SetPulseWidthA(sample_t pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidthA(pw);
    }
  }

  void SetPulseWidthB(sample_t pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidthB(pw);
    }
  }

  void SetMixer(sample_t mixA, sample_t mixB, sample_t detuneB) {
    for (auto &voice : mVoices) {
      voice.SetMixer(mixA, mixB, detuneB);
    }
  }

  void SetLFO(int type, sample_t rate, sample_t depth) {
    for (auto &voice : mVoices) {
      voice.SetLFO(type, rate, depth);
    }
  }

  void SetLFORouting(sample_t pitch, sample_t filter, sample_t amp) {
    for (auto &voice : mVoices) {
      voice.SetLFORouting(pitch, filter, amp);
    }
  }

  void SetPolyModOscBToFreqA(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToFreqA(amount);
    }
  }

  void SetPolyModOscBToPWM(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToPWM(amount);
    }
  }

  void SetPolyModOscBToFilter(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToFilter(amount);
    }
  }

  void SetPolyModFilterEnvToFreqA(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModFilterEnvToFreqA(amount);
    }
  }

  void SetPolyModFilterEnvToPWM(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModFilterEnvToPWM(amount);
    }
  }

  void SetPolyModFilterEnvToFilter(sample_t amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModFilterEnvToFilter(amount);
    }
  }

  int GetActiveVoiceCount() const {
    int count = 0;
    for (const auto &voice : mVoices) {
      if (voice.IsActive()) {
        ++count;
      }
    }
    return count;
  }

  bool IsNoteActive(int note) const {
    for (const auto &voice : mVoices) {
      if (voice.IsActive() && voice.GetNote() == note) {
        return true;
      }
    }
    return false;
  }

  uint32_t GetGlobalTimestamp() const { return mGlobalTimestamp; }

  // --- Sprint 1: Voice state query helpers ---
  std::array<VoiceRenderState, kMaxVoices> GetVoiceStates() const {
    std::array<VoiceRenderState, kMaxVoices> states{};
    for (int i = 0; i < kNumVoices; i++) {
      states[i] = mVoices[i].GetRenderState();
    }
    return states;
  }

  int GetHeldNotes(std::array<int, kMaxVoices> &buf) const {
    int count = 0;
    for (const auto &voice : mVoices) {
      if (voice.IsActive() && voice.GetNote() >= 0) {
        const int note = voice.GetNote();
        bool found = false;
        for (int j = 0; j < count; j++) {
          if (buf[j] == note) {
            found = true;
            break;
          }
        }
        if (!found && count < kMaxVoices) {
          buf[count++] = note;
        }
      }
    }
    return count;
  }

private:
  std::array<Voice, kNumVoices> mVoices;
  sea::VoiceAllocator<Voice, kMaxVoices> mAllocator;
  sample_t mSampleRate = 44100.0;
  uint32_t mGlobalTimestamp = 0;
};

} // namespace PolySynthCore
