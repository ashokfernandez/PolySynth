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
    mOsc.Init(sampleRate);
    mOsc.SetFrequency(440.0);

    mFilter.Init(sampleRate);
    mFilter.SetParams(FilterType::LowPass, 2000.0, 0.707); // Default open

    mAmpEnv.Init(sampleRate);
    mAmpEnv.SetParams(0.01, 0.1, 0.5, 0.2); // Default ADSR

    mLfo.Init(sampleRate);
    mLfo.SetRate(1.0);
    mLfo.SetDepth(0.0);
    mLfo.SetWaveform(0);

    mActive = false;
    mNote = -1;
    mAge = 0;
    mBaseCutoff = 2000.0;
    mBaseRes = 0.707;
  }

  void NoteOn(int note, int velocity) {
    // Simple Midi to Freq
    double freq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
    mOsc.SetFrequency(freq);
    mOsc.Reset(); // Hard sync

    mVelocity = velocity / 127.0;

    mAmpEnv.NoteOn();
    mActive = true;
    mNote = note;
    mAge = 0; // Reset age
  }

  void NoteOff() {
    mAmpEnv.NoteOff();
    // mActive remains true until Envelope finishes
  }

  inline sample_t Process() {
    if (!mActive)
      return 0.0;

    sample_t osc = mOsc.Process();
    sample_t lfo = mLfo.Process();
    double effectiveCutoff = mBaseCutoff * (1.0 + static_cast<double>(lfo));
    effectiveCutoff = std::clamp(effectiveCutoff, 20.0, 20000.0);
    mFilter.SetParams(FilterType::LowPass, effectiveCutoff, mBaseRes);
    sample_t flt = mFilter.Process(osc);
    sample_t env = mAmpEnv.Process();

    // Update age (samples active)
    mAge++;

    // Check if envelope finished
    if (!mAmpEnv.IsActive()) {
      mActive = false;
      mNote = -1;
      mAge = 0;
    }

    // Velocity sensitivity
    return flt * env * mVelocity;
  }

  bool IsActive() const { return mActive; }
  int GetNote() const { return mNote; }
  uint64_t GetAge() const { return mAge; }

  // For stealing, we might want to release quickly?
  // Or just hard reset?
  // Let's just use NoteOn to reset.

  void SetADSR(double a, double d, double s, double r) {
    mAmpEnv.SetParams(a, d, s, r);
  }

  void SetFilter(double cutoff, double res) {
    mBaseCutoff = cutoff;
    mBaseRes = res;
    mFilter.SetParams(FilterType::LowPass, cutoff, res);
  }

  void SetWaveform(Oscillator::WaveformType type) { mOsc.SetWaveform(type); }

  void SetLFORate(double hz) { mLfo.SetRate(hz); }

  void SetLFODepth(double depth) { mLfo.SetDepth(depth); }

  void SetLFOType(int type) { mLfo.SetWaveform(type); }

private:
  Oscillator mOsc;
  BiquadFilter mFilter;
  ADSREnvelope mAmpEnv;
  LFO mLfo;
  bool mActive = false;
  double mVelocity = 0.0;
  int mNote = -1;
  uint64_t mAge = 0;
  double mBaseCutoff = 2000.0;
  double mBaseRes = 0.707;
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

  void SetFilter(double cutoff, double res) {
    for (auto &voice : mVoices) {
      voice.SetFilter(cutoff, res);
    }
  }

  void SetWaveform(Oscillator::WaveformType type) {
    for (auto &voice : mVoices) {
      voice.SetWaveform(type);
    }
  }

  void SetLFORate(double hz) {
    for (auto &voice : mVoices) {
      voice.SetLFORate(hz);
    }
  }

  void SetLFODepth(double depth) {
    for (auto &voice : mVoices) {
      voice.SetLFODepth(depth);
    }
  }

  void SetLFOType(int type) {
    for (auto &voice : mVoices) {
      voice.SetLFOType(type);
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
