#pragma once

#include "VoiceManager.h"
#include "types.h"
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

  void SetParameter(int paramNum, double value) {
    // Basic param dispatch could go here
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
  void SetWaveform(sea::Oscillator::WaveformType type) {
    mVoiceManager.SetWaveform(type);
  }
  void SetWaveformA(sea::Oscillator::WaveformType type) {
    mVoiceManager.SetWaveformA(type);
  }
  void SetWaveformB(sea::Oscillator::WaveformType type) {
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
  void SetLFORouting(double pitch, double filter, double amp,
                     double pan = 0.0) {
    mVoiceManager.SetLFORouting(pitch, filter, amp, pan);
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

  // --- FX Setters ---
  void SetChorus(double rateHz, double depth, double mix) {
    mChorus.SetRate(rateHz);
    mChorus.SetDepth(depth * 5.0); // Map 0-1 to 0-5ms
    mChorus.SetMix(mix);
  }
  void SetDelay(double timeSec, double feedback, double mix) {
    mDelay.SetTime(timeSec * 1000.0);
    mDelay.SetFeedback(feedback * 100.0);
    mDelay.SetMix(mix * 100.0);
  }
  void SetDelayTempo(double bpm, double division) {
    // Basic fallback for now
    double beatSec = 60.0 / bpm;
    SetDelay(beatSec * division, 35.0, 0.0);
  }
  void SetLimiter(double threshold, double lookaheadMs, double releaseMs) {
    // Threshold should be mapped inverted if coming from UI,
    // but here we just pass it through as the engine setter.
    mLimiter.SetParams(threshold, lookaheadMs, releaseMs);
  }

  // --- Audio Processing ---
  void Process(sample_t &left, sample_t &right) {
    double l, r;
    mVoiceManager.ProcessStereo(l, r);
    left = static_cast<sample_t>(l);
    right = static_cast<sample_t>(r);

    sample_t fxL, fxR;
    mChorus.Process(left, right, &fxL, &fxR);
    mDelay.Process(fxL, fxR, &fxL, &fxR);
    mLimiter.Process(fxL, fxR);

    left = fxL;
    right = fxR;
  }

  void Process(sample_t **inputs, sample_t **outputs, int nFrames, int nChans) {
    for (int i = 0; i < nFrames; ++i) {
      double l, r;
      mVoiceManager.ProcessStereo(l, r);
      sample_t left = static_cast<sample_t>(l);
      sample_t right = static_cast<sample_t>(r);

      sample_t fxL, fxR;
      mChorus.Process(left, right, &fxL, &fxR);
      mDelay.Process(fxL, fxR, &fxL, &fxR);
      mLimiter.Process(fxL, fxR);

      if (nChans > 0)
        outputs[0][i] = fxL;
      if (nChans > 1)
        outputs[1][i] = fxR;
    }
  }

private:
  double mSampleRate;
  VoiceManager mVoiceManager;
  sea::VintageChorus<double> mChorus;
  sea::VintageDelay<double> mDelay;
  sea::LookaheadLimiter<double> mLimiter;
};

} // namespace PolySynthCore
