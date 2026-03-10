#pragma once

#include "DspConstants.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <sea_dsp/sea_adsr.h>
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_cascade_filter.h>
#include <sea_dsp/sea_ladder_filter.h>
#include <sea_dsp/sea_lfo.h>
#include <sea_dsp/sea_oscillator.h>

namespace PolySynthCore {

class Voice {
public:
  enum class FilterModel { Classic, Ladder, Cascade12, Cascade24 };

  Voice() = default;

  void Init(sample_t sampleRate, uint8_t voiceID = 0) {
    mOscA.Init(sampleRate);
    mOscB.Init(sampleRate);
    mOscA.SetFrequency(sample_t(440));
    mDetuneFactor = std::pow(sample_t(2), mDetuneB / sample_t(1200));
    mOscB.SetFrequency(sample_t(440) * mDetuneFactor);
    mTargetFreq = sample_t(440);
    mGlideTime = sample_t(0);
    mGlideAlpha = sample_t(1);

    mBasePulseWidthA = sample_t(0.5);
    mBasePulseWidthB = sample_t(0.5);
    mOscA.SetPulseWidth(mBasePulseWidthA);
    mOscB.SetPulseWidth(mBasePulseWidthB);

    mFilter.Init(sampleRate);
    mFilter.SetParams(sea::FilterType::LowPass, sample_t(2000), sample_t(0.707));
    mLadderFilter.Init(sampleRate);
    mCascadeFilter.Init(sampleRate);

    mAmpEnv.Init(sampleRate);
    mAmpEnv.SetParams(sample_t(0.01), sample_t(0.1), sample_t(1), sample_t(0.2));

    mFilterEnv.Init(sampleRate);
    mFilterEnv.SetParams(sample_t(0.01), sample_t(0.1), sample_t(0.5), sample_t(0.2));

    mLfo.Init(sampleRate);
    mLfo.SetRate(sample_t(5));
    mLfo.SetDepth(sample_t(0));
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
    mStolenFadeDelta = sample_t(0);
    mLastAmpEnvVal = 0.0f;
    mBaseCutoff = sample_t(2000);
    mBaseRes = sample_t(0.707);
    mPolyModOscBToFreqA = sample_t(0);
    mPolyModOscBToPWM = sample_t(0);
    mPolyModOscBToFilter = sample_t(0);
    mPolyModFilterEnvToFreqA = sample_t(0);
    mPolyModFilterEnvToPWM = sample_t(0);
    mPolyModFilterEnvToFilter = sample_t(0);
    mFilterModel = FilterModel::Ladder;
  }

  void NoteOn(int note, int velocity, uint32_t timestamp = 0) {
    sample_t newFreq = sample_t(440) * std::pow(sample_t(2), (note - sample_t(69)) / sample_t(12));
    mTargetFreq = newFreq;

    if (mGlideTime > sample_t(0) && mActive) {
      // Legato glide: keep current frequency, glide to target
      // Don't reset oscillators — smooth transition
    } else {
      // Normal retrigger: jump to frequency immediately
      mFreq = newFreq;
      mOscA.SetFrequency(mFreq);
      mOscB.SetFrequency(mFreq * mDetuneFactor);
      mOscA.Reset();
      mOscB.Reset();
    }

    mVelocity = velocity / sample_t(127);

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
    sample_t factor = std::pow(sample_t(2), cents / sample_t(1200));
    mFreq *= factor;
    mCurrentPitch = static_cast<float>(mFreq);
    mOscA.SetFrequency(mFreq);
    mOscB.SetFrequency(mFreq * mDetuneFactor);
  }

  inline sample_t Process() {
    // ─── Voice Signal Flow ────────────────────────────────────────────
    // 1. Early exit if voice is idle or stolen-and-faded
    // 2. LFO + Filter Envelope generation
    // 3. Portamento: exponential glide toward target frequency
    // 4. Pitch modulation: base freq ← LFO vibrato + poly-mod (OscB→FreqA, FilterEnv→FreqA)
    // 5. OscB synthesis → used as poly-mod source
    // 6. OscA synthesis → modulated frequency, pulse width from poly-mod
    // 7. Mixer: OscA × mixA + OscB × mixB
    // 8. Filter: base cutoff + filter env + poly-mod + LFO modulation
    // 9. Amp envelope × velocity × tremolo (LFO amp mod)
    // 10. Voice stealing fade (if state == Stolen)
    // ──────────────────────────────────────────────────────────────────

    // ── Step 1: Early exit ──
    if (!mActive)
      return sample_t(0);

    if (mVoiceState == VoiceState::Stolen && mStolenFadeGain <= 0.0f) {
      mActive = false;
      mNote = -1;
      mVoiceState = VoiceState::Idle;
      mAge = 0;
      mLastAmpEnvVal = 0.0f;
      return sample_t(0);
    }

    // ── Step 2: LFO & Filter Envelope ──
    sample_t lfoVal = sample_t(0);
    if (mLfoPitchDepth != sample_t(0) || mLfoFilterDepth != sample_t(0) ||
        mLfoAmpDepth != sample_t(0) || mLfoPanDepth != sample_t(0)) {
      lfoVal = mLfo.Process();
    }
    sample_t filterEnvVal = mFilterEnv.Process();

    // ── Step 3: Portamento ──
    if (mGlideTime > sample_t(0) && std::abs(mFreq - mTargetFreq) > kGlideSnapThresholdHz) {
      mFreq += (mTargetFreq - mFreq) * mGlideAlpha;
      // Snap to target when close enough
      if (std::abs(mFreq - mTargetFreq) < kGlideSnapThresholdHz) {
        mFreq = mTargetFreq;
      }
      mCurrentPitch = static_cast<float>(mFreq);
      // Update oscillator base frequencies (will be modulated below)
      // Note: We don't set mOscA/B immediate here because they get set below
      // with modulation
    }

    // ── Step 4-6: Oscillator Synthesis & Modulation ──
    sample_t modFreqA = mFreq;
    sample_t modFreqB = mFreq * mDetuneFactor;

    if (mLfoPitchDepth > sample_t(0)) {
      sample_t modMult = (sample_t(1) + lfoVal * mLfoPitchDepth * kLfoPitchScale);
      modFreqA *= modMult;
      modFreqB *= modMult;
    }

    mOscB.SetFrequency(modFreqB);
    sample_t oscB = mOscB.Process();

    if (mPolyModOscBToFreqA != sample_t(0) || mPolyModFilterEnvToFreqA != sample_t(0)) {
      sample_t freqMod = (oscB * mPolyModOscBToFreqA) +
                         (filterEnvVal * mPolyModFilterEnvToFreqA);
      modFreqA *= (sample_t(1) + freqMod);
      modFreqA = std::max(sample_t(1), modFreqA);
    }

    mOscA.SetFrequency(modFreqA);

    if (mPolyModOscBToPWM != sample_t(0) || mPolyModFilterEnvToPWM != sample_t(0)) {
      sample_t pwmMod =
          (oscB * mPolyModOscBToPWM) + (filterEnvVal * mPolyModFilterEnvToPWM);
      sample_t pwmA = mBasePulseWidthA + (pwmMod * kPwmModScale);
      mOscA.SetPulseWidth(std::clamp(pwmA, sample_t(0.01), sample_t(0.99)));
    } else {
      mOscA.SetPulseWidth(mBasePulseWidthA);
    }

    sample_t oscA = mOscA.Process();
    // ── Step 7: Mixer ──
    sample_t mixed = (oscA * mMixA) + (oscB * mMixB);

    // ── Step 8: Filter (model dispatch) ──
    sample_t cutoff = mBaseCutoff;
    cutoff +=
        filterEnvVal * (mFilterEnvAmount + mPolyModFilterEnvToFilter) * kFilterEnvMaxHz;
    if (mPolyModOscBToFilter != sample_t(0)) {
      cutoff += oscB * mPolyModOscBToFilter * mBaseCutoff;
    }
    cutoff *= (sample_t(1) + lfoVal * mLfoFilterDepth);
    cutoff = std::clamp(cutoff, sample_t(20.0), sample_t(20000.0));

    sample_t flt = sample_t(0);
    bool filterDirty = (cutoff != mLastFilterCutoff || mBaseRes != mLastFilterRes);
    if (filterDirty) {
      mLastFilterCutoff = cutoff;
      mLastFilterRes = mBaseRes;
    }
    switch (mFilterModel) {
    case FilterModel::Ladder:
      if (filterDirty)
        mLadderFilter.SetParams(sea::LadderFilter<sample_t>::Model::Transistor,
                                cutoff, mBaseRes);
      flt = mLadderFilter.Process(mixed);
      break;
    case FilterModel::Cascade12:
      if (filterDirty)
        mCascadeFilter.SetParams(cutoff, mBaseRes,
                                 sea::CascadeFilter<sample_t>::Slope::dB12);
      flt = mCascadeFilter.Process(mixed);
      break;
    case FilterModel::Cascade24:
      if (filterDirty)
        mCascadeFilter.SetParams(cutoff, mBaseRes,
                                 sea::CascadeFilter<sample_t>::Slope::dB24);
      flt = mCascadeFilter.Process(mixed);
      break;
    case FilterModel::Classic:
    default:
      if (filterDirty)
        mFilter.SetParams(sea::FilterType::LowPass, cutoff, mBaseRes);
      flt = mFilter.Process(mixed);
      break;
    }

    // ── Step 9: Amplitude Envelope & Tremolo ──
    sample_t ampEnvVal = mAmpEnv.Process();
    mLastAmpEnvVal = static_cast<float>(ampEnvVal);
    sample_t ampMod = sample_t(1);
    if (mLfoAmpDepth > sample_t(0)) {
      ampMod = sample_t(1) + lfoVal * mLfoAmpDepth;
      ampMod = std::clamp(ampMod, sample_t(0.0), sample_t(2.0));
    }

    // Update pan cache with LFO modulation (only recomputes sin/cos if pan changed)
    mLastLfoVal = static_cast<float>(lfoVal);
    if (mLfoPanDepth != sample_t(0)) {
      float modulatedPan = mPanPosition + (mLastLfoVal * mLfoPanDepth);
      modulatedPan = std::clamp(modulatedPan, -1.0f, 1.0f);
      UpdatePanCache(modulatedPan);
    }

    mAge++;

    if (!mAmpEnv.IsActive() && !mFilterEnv.IsActive()) {
      mActive = false;
      mNote = -1;
      mAge = 0;
      mVoiceState = VoiceState::Idle;
    }

    sample_t out = flt * ampEnvVal * mVelocity * ampMod;
    // ── Step 10: Voice Stealing Fade ──
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
    const sample_t fadeSamples = std::max(sample_t(1.0), kStolenFadeTimeSec * mSampleRate);
    mStolenFadeDelta = sample_t(1.0) / fadeSamples;
    mVoiceState = VoiceState::Stolen;
  }

  VoiceState GetVoiceState() const { return mVoiceState; }
  uint8_t GetVoiceID() const { return mVoiceID; }
  uint32_t GetTimestamp() const { return mTimestamp; }
  float GetPitch() const { return mCurrentPitch; }

  float GetPanPosition() const {
    float modulatedPan = mPanPosition + (mLastLfoVal * mLfoPanDepth);
    return std::clamp(modulatedPan, -1.0f, 1.0f);
  }

  void GetPanCoefficients(sample_t &left, sample_t &right) const {
    left = mPanLeft;
    right = mPanRight;
  }

  void SetPanPosition(float pan) {
    mPanPosition = std::clamp(pan, -1.0f, 1.0f);
    UpdatePanCache(mPanPosition);
  }

  VoiceRenderState GetRenderState() const {
    VoiceRenderState rs;
    rs.voiceID = mVoiceID;
    rs.state = mVoiceState;
    rs.note = mNote;
    rs.velocity = static_cast<int>(mVelocity * sample_t(127));
    rs.currentPitch = mCurrentPitch;
    rs.panPosition = mPanPosition;
    rs.amplitude = mLastAmpEnvVal;
    return rs;
  }

  void SetGlideTime(sample_t seconds) {
    mGlideTime = std::max(sample_t(0.0), seconds);
    if (mGlideTime > sample_t(0)) {
      mGlideAlpha = sample_t(1) - std::exp(-kGlideTimeConstant / (mGlideTime * mSampleRate));
    } else {
      mGlideAlpha = sample_t(1); // Instant snap
    }
  }

  void SetADSR(sample_t a, sample_t d, sample_t s, sample_t r) {
    mAmpEnv.SetParams(a, d, s, r);
  }

  void SetFilterEnv(sample_t a, sample_t d, sample_t s, sample_t r) {
    mFilterEnv.SetParams(a, d, s, r);
  }

  void SetFilter(sample_t cutoff, sample_t res, sample_t envAmount) {
    mBaseCutoff = cutoff;
    // Map 0-1 resonance to 0.5-20.5 Q for the biquad filter
    mBaseRes = kResonanceBaseQ + (res * res * kResonanceScale);
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
    mBasePulseWidthA = std::clamp(pw, sample_t(0.01), sample_t(0.99));
    mOscA.SetPulseWidth(mBasePulseWidthA);
  }
  void SetPulseWidthB(sample_t pw) {
    mBasePulseWidthB = std::clamp(pw, sample_t(0.01), sample_t(0.99));
    mOscB.SetPulseWidth(mBasePulseWidthB);
  }

  void SetMixer(sample_t mixA, sample_t mixB, sample_t detuneB) {
    mMixA = std::clamp(mixA, sample_t(0.0), sample_t(1.0));
    mMixB = std::clamp(mixB, sample_t(0.0), sample_t(1.0));
    mDetuneB = detuneB;
    mDetuneFactor = std::pow(sample_t(2), mDetuneB / sample_t(1200));
  }

  void SetLFO(int type, sample_t rate, sample_t depth) {
    mLfo.SetWaveform(type);
    mLfo.SetRate(rate);
    mLfo.SetDepth(depth);
  }

  void SetLFORouting(sample_t pitch, sample_t filter, sample_t amp,
                     sample_t pan = sample_t(0)) {
    mLfoPitchDepth = pitch;
    mLfoFilterDepth = filter;
    mLfoAmpDepth = amp;
    mLfoPanDepth = pan;
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
  void UpdatePanCache(float pan) {
    if (pan != mCachedPan) {
      mCachedPan = pan;
      sample_t theta =
          (static_cast<sample_t>(pan) + sample_t(1)) * (kPi / sample_t(4));
      mPanLeft = sea::Math::Cos(theta);
      mPanRight = sea::Math::Sin(theta);
    }
  }

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
  sample_t mLfoPanDepth = 0.0;

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
  float mLastLfoVal = 0.0f;
  float mStolenFadeGain = 0.0f;
  sample_t mStolenFadeDelta = 0.0;
  float mLastAmpEnvVal = 0.0f;

  sample_t mTargetFreq = 440.0;
  sample_t mGlideTime = 0.0;

  // --- Cached values (recomputed in setters, not per-sample) ---
  sample_t mDetuneFactor = 1.0;  // = pow(2.0, mDetuneB / 1200.0)
  sample_t mGlideAlpha = 1.0;   // = 1.0 - exp(-kGlideTimeConstant / (mGlideTime * mSampleRate))
  sample_t mLastFilterCutoff = sample_t(-1);  // impossible value forces first SetParams
  sample_t mLastFilterRes = sample_t(-1);

  sample_t mSampleRate = 48000.0;

  // --- Cached pan coefficients (recomputed only when pan changes) ---
  float mCachedPan = 0.0f;
  sample_t mPanLeft = sample_t(0.70710678118654752);   // cos(pi/4) — center pan
  sample_t mPanRight = sample_t(0.70710678118654752);  // sin(pi/4) — center pan
};

} // namespace PolySynthCore
