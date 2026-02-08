#pragma once

#include "modulation/ADSREnvelope.h"
#include "oscillator/Oscillator.h"
#include "types.h"
#include <array>
#include <cmath>

namespace PolySynth {

class Voice {
public:
  Voice() = default;

  void Init(double sampleRate) {
    mOsc.Init(sampleRate);
    mOsc.SetFrequency(440.0);

    mAmpEnv.Init(sampleRate);
    mAmpEnv.SetParams(0.01, 0.1, 0.5, 0.2); // Default ADSR

    mActive = false;
  }

  void NoteOn(int note, int velocity) {
    // Simple Midi to Freq
    double freq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
    mOsc.SetFrequency(freq);
    mOsc.Reset(); // Hard sync

    mVelocity = velocity / 127.0;

    mAmpEnv.NoteOn();
    mActive = true;
  }

  void NoteOff() {
    mAmpEnv.NoteOff();
    // mActive remains true until Envelope finishes
  }

  inline sample_t Process() {
    if (!mActive)
      return 0.0;

    sample_t osc = mOsc.Process();
    sample_t env = mAmpEnv.Process();

    // Check if envelope finished
    if (!mAmpEnv.IsActive()) {
      mActive = false;
    }

    return osc * env * mVelocity;
  }

  bool IsActive() const { return mActive; }

private:
  Oscillator mOsc;
  ADSREnvelope mAmpEnv;
  bool mActive = false;
  double mVelocity = 0.0;
};

class VoiceManager {
public:
  VoiceManager() = default;

  void Init(double sampleRate) {
    // Monophonic for now: just 1 voice
    mVoice.Init(sampleRate);
  }

  void Reset() {
    // mVoice.NoteOff(); // Should be silence?
    mVoice.Init(44100.0); // Reset everything
  }

  void OnNoteOn(int note, int velocity) { mVoice.NoteOn(note, velocity); }

  void OnNoteOff(int note) { mVoice.NoteOff(); }

  inline sample_t Process() { return mVoice.Process(); }

private:
  Voice mVoice;
};

} // namespace PolySynth
