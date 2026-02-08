#pragma once

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

  void Reset() {
    // TODO: Reset voices
  }

  // --- Events ---
  void OnNoteOn(int note, int velocity) {
    // TODO: Voice Allocation
  }

  void OnNoteOff(int note) {
    // TODO: Voice Release
  }

  void SetParameter(int paramNum, double value) {
    // TODO: Param dispatch
  }

  // --- Audio Processing ---
  void Process(sample_t **inputs, sample_t **outputs, int nFrames, int nChans) {
    // Silence for now
    for (int c = 0; c < nChans; ++c) {
      std::memset(outputs[c], 0, nFrames * sizeof(sample_t));
    }
  }

private:
  double mSampleRate;
};

} // namespace PolySynth
