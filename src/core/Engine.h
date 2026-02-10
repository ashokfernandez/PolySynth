#pragma once

#include "VoiceManager.h"
#include "types.h"
#include <cstdint>
#include <cstring>
#include <iostream>

namespace PolySynthCore {

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
    // Basic param dispatch could go here
    // For now we use direct setters for demos
  }

  // --- High Level Setters ---
  void SetADSR(double a, double d, double s, double r) {
    mVoiceManager.SetADSR(a, d, s, r);
  }
  void SetFilterEnv(double a, double d, double s, double r) {
    mVoiceManager.SetFilterEnv(a, d, s, r);
  }
  void SetFilter(double cutoff, double res, double envAmount) {
    mVoiceManager.SetFilter(cutoff, res, envAmount);
  }
  void SetWaveform(Oscillator::WaveformType type) {
    mVoiceManager.SetWaveform(type);
  }
  void SetWaveformA(Oscillator::WaveformType type) {
    mVoiceManager.SetWaveformA(type);
  }
  void SetWaveformB(Oscillator::WaveformType type) {
    mVoiceManager.SetWaveformB(type);
  }

  void SetPulseWidth(double pw) { mVoiceManager.SetPulseWidth(pw); }
  void SetPulseWidthA(double pw) { mVoiceManager.SetPulseWidthA(pw); }
  void SetPulseWidthB(double pw) { mVoiceManager.SetPulseWidthB(pw); }

  void SetMixer(double mixA, double mixB, double detuneB) {
    mVoiceManager.SetMixer(mixA, mixB, detuneB);
  }

  void SetLFO(int type, double rate, double depth) {
    mVoiceManager.SetLFO(type, rate, depth);
  }
  void SetLFORouting(double pitch, double filter, double amp) {
    mVoiceManager.SetLFORouting(pitch, filter, amp);
  }

  void SetPolyModOscBToFreqA(double amount) {
    mVoiceManager.SetPolyModOscBToFreqA(amount);
  }
  void SetPolyModOscBToPWM(double amount) {
    mVoiceManager.SetPolyModOscBToPWM(amount);
  }
  void SetPolyModOscBToFilter(double amount) {
    mVoiceManager.SetPolyModOscBToFilter(amount);
  }
  void SetPolyModFilterEnvToFreqA(double amount) {
    mVoiceManager.SetPolyModFilterEnvToFreqA(amount);
  }
  void SetPolyModFilterEnvToPWM(double amount) {
    mVoiceManager.SetPolyModFilterEnvToPWM(amount);
  }
  void SetPolyModFilterEnvToFilter(double amount) {
    mVoiceManager.SetPolyModFilterEnvToFilter(amount);
  }

  // --- Audio Processing ---
  void Process(sample_t &left, sample_t &right) {
    sample_t out = mVoiceManager.Process();
    left = out;
    right = out;
  }

  void Process(sample_t **inputs, sample_t **outputs, int nFrames, int nChans) {
    // Basic stereo copy for now
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

} // namespace PolySynthCore
