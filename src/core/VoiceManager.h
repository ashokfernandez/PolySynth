#pragma once

#include "dsp/BiquadFilter.h"
#include "modulation/ADSREnvelope.h"
#include "modulation/LFO.h"
#include "oscillator/Oscillator.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace PolySynthCore {

class Voice {
public:
  Voice() = default;

  void Init(double sampleRate) {
    mOscA.Init(sampleRate);
    mOscB.Init(sampleRate);
    mOscA.SetFrequency(440.0);
    mOscB.SetFrequency(440.0 * std::pow(2.0, mDetuneB / 1200.0));
    mBasePulseWidthA = 0.5;
    mBasePulseWidthB = 0.5;
    mOscA.SetPulseWidth(mBasePulseWidthA);
    mOscB.SetPulseWidth(mBasePulseWidthB);

    mFilter.Init(sampleRate);
    mFilter.SetParams(FilterType::LowPass, 2000.0, 0.707);

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
    mBaseCutoff = 2000.0;
    mBaseRes = 0.707;
    mPolyModOscBToFreqA = 0.0;
    mPolyModOscBToPWM = 0.0;
    mPolyModOscBToFilter = 0.0;
    mPolyModFilterEnvToFreqA = 0.0;
    mPolyModFilterEnvToPWM = 0.0;
    mPolyModFilterEnvToFilter = 0.0;
  }

  void NoteOn(int note, int velocity) {
    mFreq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
    mOscA.SetFrequency(mFreq);
    mOscB.SetFrequency(mFreq * std::pow(2.0, mDetuneB / 1200.0));
    mOscA.Reset();
    mOscB.Reset();

    mVelocity = velocity / 127.0;

    mAmpEnv.NoteOn();
    mFilterEnv.NoteOn();
    mActive = true;
    mNote = note;
    mAge = 0;
  }

  void NoteOff() {
    mAmpEnv.NoteOff();
    mFilterEnv.NoteOff();
  }

  inline sample_t Process() {
    if (!mActive)
      return 0.0;

    sample_t lfoVal = mLfo.Process();
    sample_t filterEnvVal = mFilterEnv.Process();

    // Pitch Modulation (Vibrato)
    double modFreqA = mFreq;
    double modFreqB = mFreq * std::pow(2.0, mDetuneB / 1200.0);

    if (mLfoPitchDepth > 0.0) {
      double modMult = (1.0 + lfoVal * mLfoPitchDepth * 0.05);
      modFreqA *= modMult;
      modFreqB *= modMult;
    }

    mOscB.SetFrequency(modFreqB);
    sample_t oscB = mOscB.Process();

    if (mPolyModOscBToFreqA != 0.0 || mPolyModFilterEnvToFreqA != 0.0) {
      double freqMod =
          (oscB * mPolyModOscBToFreqA) +
          (filterEnvVal * mPolyModFilterEnvToFreqA);
      modFreqA *= (1.0 + freqMod);
      modFreqA = std::max(1.0, modFreqA);
    }

    mOscA.SetFrequency(modFreqA);

    if (mPolyModOscBToPWM != 0.0 || mPolyModFilterEnvToPWM != 0.0) {
      double pwmMod = (oscB * mPolyModOscBToPWM) +
                      (filterEnvVal * mPolyModFilterEnvToPWM);
      double pwmA = mBasePulseWidthA + (pwmMod * 0.5);
      mOscA.SetPulseWidth(std::clamp(pwmA, 0.01, 0.99));
    } else {
      mOscA.SetPulseWidth(mBasePulseWidthA);
    }

    sample_t oscA = mOscA.Process();
    sample_t mixed = (oscA * mMixA) + (oscB * mMixB);

    // Filter Modulation: Base + Keyboard + Env + LFO
    double cutoff = mBaseCutoff;
    cutoff += filterEnvVal * (mFilterEnvAmount + mPolyModFilterEnvToFilter) *
              10000.0;
    if (mPolyModOscBToFilter != 0.0) {
      cutoff += oscB * mPolyModOscBToFilter * mBaseCutoff;
    }
    cutoff *= (1.0 + lfoVal * mLfoFilterDepth);
    cutoff = std::clamp(cutoff, 20.0, 20000.0);

    mFilter.SetParams(FilterType::LowPass, cutoff, mBaseRes);
    sample_t flt = mFilter.Process(mixed);

    sample_t ampEnvVal = mAmpEnv.Process();

    // Amp Modulation (Tremolo)
    double ampMod = 1.0;
    if (mLfoAmpDepth > 0.0) {
      ampMod = 1.0 + lfoVal * mLfoAmpDepth;
      ampMod = std::clamp(ampMod, 0.0, 2.0);
    }

    mAge++;

    if (!mAmpEnv.IsActive() && !mFilterEnv.IsActive()) {
      mActive = false;
      mNote = -1;
      mAge = 0;
    }

    return flt * ampEnvVal * mVelocity * ampMod;
  }

  bool IsActive() const { return mActive; }
  int GetNote() const { return mNote; }
  uint64_t GetAge() const { return mAge; }

  void SetADSR(double a, double d, double s, double r) {
    mAmpEnv.SetParams(a, d, s, r);
  }

  void SetFilterEnv(double a, double d, double s, double r) {
    mFilterEnv.SetParams(a, d, s, r);
  }

  void SetFilter(double cutoff, double res, double envAmount) {
    mBaseCutoff = cutoff;
    mBaseRes = res;
    mFilterEnvAmount = envAmount;
  }

  void SetWaveform(Oscillator::WaveformType type) { SetWaveformA(type); }
  void SetWaveformA(Oscillator::WaveformType type) { mOscA.SetWaveform(type); }
  void SetWaveformB(Oscillator::WaveformType type) { mOscB.SetWaveform(type); }

  void SetPulseWidth(double pw) { SetPulseWidthA(pw); }
  void SetPulseWidthA(double pw) {
    mBasePulseWidthA = std::clamp(pw, 0.01, 0.99);
    mOscA.SetPulseWidth(mBasePulseWidthA);
  }
  void SetPulseWidthB(double pw) {
    mBasePulseWidthB = std::clamp(pw, 0.01, 0.99);
    mOscB.SetPulseWidth(mBasePulseWidthB);
  }

  void SetMixer(double mixA, double mixB, double detuneB) {
    mMixA = std::clamp(mixA, 0.0, 1.0);
    mMixB = std::clamp(mixB, 0.0, 1.0);
    mDetuneB = detuneB;
  }

  void SetLFO(int type, double rate, double depth) {
    mLfo.SetWaveform(type);
    mLfo.SetRate(rate);
    mLfo.SetDepth(depth);
  }

  void SetLFORouting(double pitch, double filter, double amp) {
    mLfoPitchDepth = pitch;
    mLfoFilterDepth = filter;
    mLfoAmpDepth = amp;
  }

  void SetPolyModOscBToFreqA(double amount) { mPolyModOscBToFreqA = amount; }
  void SetPolyModOscBToPWM(double amount) { mPolyModOscBToPWM = amount; }
  void SetPolyModOscBToFilter(double amount) { mPolyModOscBToFilter = amount; }
  void SetPolyModFilterEnvToFreqA(double amount) {
    mPolyModFilterEnvToFreqA = amount;
  }
  void SetPolyModFilterEnvToPWM(double amount) {
    mPolyModFilterEnvToPWM = amount;
  }
  void SetPolyModFilterEnvToFilter(double amount) {
    mPolyModFilterEnvToFilter = amount;
  }

private:
  Oscillator mOscA;
  Oscillator mOscB;
  BiquadFilter mFilter;
  ADSREnvelope mAmpEnv;
  ADSREnvelope mFilterEnv;
  LFO mLfo;

  bool mActive = false;
  double mVelocity = 0.0;
  int mNote = -1;
  uint64_t mAge = 0;

  double mFreq = 440.0;
  double mBaseCutoff = 2000.0;
  double mBaseRes = 0.707;
  double mFilterEnvAmount = 0.0;

  double mMixA = 1.0;
  double mMixB = 0.0;
  double mDetuneB = 0.0;
  double mBasePulseWidthA = 0.5;
  double mBasePulseWidthB = 0.5;

  double mLfoPitchDepth = 0.0;
  double mLfoFilterDepth = 0.0;
  double mLfoAmpDepth = 0.0;

  double mPolyModOscBToFreqA = 0.0;
  double mPolyModOscBToPWM = 0.0;
  double mPolyModOscBToFilter = 0.0;
  double mPolyModFilterEnvToFreqA = 0.0;
  double mPolyModFilterEnvToPWM = 0.0;
  double mPolyModFilterEnvToFilter = 0.0;
};

class VoiceManager {
public:
  static constexpr int kNumVoices = 8;

  VoiceManager() = default;

  void Init(double sampleRate) {
    for (auto &voice : mVoices) {
      voice.Init(sampleRate);
    }
  }

  void Reset() {
    for (auto &voice : mVoices) {
      voice.Init(44100.0); // Reset defaults
    }
  }

  void OnNoteOn(int note, int velocity) {
    Voice *voice = FindFreeVoice();
    if (!voice) {
      voice = FindVoiceToSteal();
    }

    if (voice) {
      voice->NoteOn(note, velocity);
    }
  }

  void OnNoteOff(int note) {
    // Find voice playing this note
    for (auto &voice : mVoices) {
      if (voice.IsActive() && voice.GetNote() == note) {
        voice.NoteOff();
        // Don't break, in case multiple voices have same note (unlikely but
        // possible in some midi scenarios) Actually usually we want to note off
        // all of them? Or just one? Standard behavior: note off all matching.
      }
    }
  }

  inline sample_t Process() {
    sample_t sum = 0.0;
    for (auto &voice : mVoices) {
      sum += voice.Process();
    }
    return sum * 0.25; // Headroom scaling (1/sqrt(N) or similar, 0.25 is safe
                       // for 8 voices)
  }

  void SetADSR(double a, double d, double s, double r) {
    for (auto &voice : mVoices) {
      voice.SetADSR(a, d, s, r);
    }
  }

  void SetFilterEnv(double a, double d, double s, double r) {
    for (auto &voice : mVoices) {
      voice.SetFilterEnv(a, d, s, r);
    }
  }

  void SetFilter(double cutoff, double res, double envAmount) {
    for (auto &voice : mVoices) {
      voice.SetFilter(cutoff, res, envAmount);
    }
  }

  void SetWaveform(Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveform(type);
    }
  }

  void SetWaveformA(Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveformA(type);
    }
  }

  void SetWaveformB(Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveformB(type);
    }
  }

  void SetPulseWidth(double pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidth(pw);
    }
  }

  void SetPulseWidthA(double pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidthA(pw);
    }
  }

  void SetPulseWidthB(double pw) {
    for (auto &voice : mVoices) {
      voice.SetPulseWidthB(pw);
    }
  }

  void SetMixer(double mixA, double mixB, double detuneB) {
    for (auto &voice : mVoices) {
      voice.SetMixer(mixA, mixB, detuneB);
    }
  }

  void SetLFO(int type, double rate, double depth) {
    for (auto &voice : mVoices) {
      voice.SetLFO(type, rate, depth);
    }
  }

  void SetLFORouting(double pitch, double filter, double amp) {
    for (auto &voice : mVoices) {
      voice.SetLFORouting(pitch, filter, amp);
    }
  }

  void SetPolyModOscBToFreqA(double amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToFreqA(amount);
    }
  }

  void SetPolyModOscBToPWM(double amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToPWM(amount);
    }
  }

  void SetPolyModOscBToFilter(double amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModOscBToFilter(amount);
    }
  }

  void SetPolyModFilterEnvToFreqA(double amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModFilterEnvToFreqA(amount);
    }
  }

  void SetPolyModFilterEnvToPWM(double amount) {
    for (auto &voice : mVoices) {
      voice.SetPolyModFilterEnvToPWM(amount);
    }
  }

  void SetPolyModFilterEnvToFilter(double amount) {
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

private:
  Voice *FindFreeVoice() {
    for (auto &voice : mVoices) {
      if (!voice.IsActive())
        return &voice;
    }
    return nullptr;
  }

  Voice *FindVoiceToSteal() {
    // Steal oldest voice
    Voice *oldest = nullptr;
    uint64_t maxAge = 0;

    for (auto &voice : mVoices) {
      if (voice.GetAge() >= maxAge) {
        maxAge = voice.GetAge();
        oldest = &voice;
      }
    }
    return oldest;
  }

  std::array<Voice, kNumVoices> mVoices;
};

} // namespace PolySynthCore
