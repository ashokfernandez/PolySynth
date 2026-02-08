#pragma once

#include "VoiceManager.h"
#include "types.h"
#include <cstdint>
#include <cstring>
#include <iostream>

namespace PolySynth {

class Engine {
public:
  Engine() : mSampleRate(44100.0) {}
  ~Engine() = default;

  // --- Lifecycle ---
  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    Reset();
  }

  void Reset() { mVoiceManager.Reset(); }

  // --- Events ---
  void OnNoteOn(int note, int velocity) {
    mVoiceManager.OnNoteOn(note, velocity);
  }

  void OnNoteOff(int note) { mVoiceManager.OnNoteOff(note); }

  void SetParameter(int paramNum, double value) {
    // TODO: Param dispatch
  }

  // --- Audio Processing ---
  void Process(sample_t **inputs, sample_t **outputs, int nFrames, int nChans) {
    for (int i = 0; i < nFrames; ++i) {
      sample_t out = mVoiceManager.Process();
      for (int c = 0; c < nChans; ++c) {
        outputs[c][i] = out;
      }
    }
  }

private:
  double mSampleRate;
  VoiceManager mVoiceManager;
};

} // namespace PolySynth
