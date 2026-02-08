#pragma once

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
    mActive = false;
  }

  void NoteOn(int note, int velocity) {
    // Simple Midi to Freq
    double freq = 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
    mOsc.SetFrequency(freq);
    mActive = true;
    mVelocity = velocity / 127.0;
    mOsc.Reset(); // Hard sync on note on
  }

  void NoteOff() { mActive = false; }

  inline sample_t Process() {
    if (!mActive)
      return 0.0;
    return mOsc.Process() * mVelocity;
  }

  bool IsActive() const { return mActive; }

private:
  Oscillator mOsc;
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

  void Reset() { mVoice.NoteOff(); }

  void OnNoteOn(int note, int velocity) { mVoice.NoteOn(note, velocity); }

  void OnNoteOff(int note) {
    // For mono, we might want to check if it's the SAME note, but simple gate
    // for now
    mVoice.NoteOff();
  }

  inline sample_t Process() { return mVoice.Process(); }

private:
  Voice mVoice;
};

} // namespace PolySynth
