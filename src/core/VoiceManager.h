#pragma once

#include "Voice.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sea_util/sea_voice_allocator.h>

namespace PolySynthCore {

class VoiceManager {
public:
  static constexpr int kNumVoices = kMaxVoices;

  // Pre-computed headroom scaling: 1/sqrt(kNumVoices)
  static inline const sample_t kHeadroomScale =
      1.0 / std::sqrt(static_cast<sample_t>(kNumVoices));

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
    return sum * kHeadroomScale;
  }

  inline void ProcessStereo(sample_t &outLeft, sample_t &outRight) {
    outLeft = 0.0;
    outRight = 0.0;

    for (auto &voice : mVoices) {
      sample_t mono = voice.Process();
      if (mono == 0.0)
        continue;

      float pan = std::clamp(voice.GetPanPosition(), -1.0f, 1.0f);
      // Constant-power panning: theta maps [-1,+1] to [0, pi/2]
      sample_t theta = (static_cast<sample_t>(pan) + 1.0) * (kPi / 4.0);
      outLeft += mono * std::cos(theta);
      outRight += mono * std::sin(theta);
    }
    // Headroom scaling
    outLeft *= kHeadroomScale;
    outRight *= kHeadroomScale;
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

  void SetLFORouting(sample_t pitch, sample_t filter, sample_t amp,
                     sample_t pan = 0.0) {
    for (auto &voice : mVoices) {
      voice.SetLFORouting(pitch, filter, amp, pan);
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
