#pragma once

#include "types.h"
#include <stdint.h>
#include <string.h>

namespace PolySynthCore {

// The Single Source of Truth for the Synth Engine
struct SynthState {
  // --- Global -------------------------------
  double masterGain = 1.0; // 0.0 to 1.0
  int polyphony = 8;       // 1 to 16
  bool unison = false;
  double unisonDetune = 0.0; // 0.0 to 1.0 (Hz spread)
  double glideTime = 0.0;    // Seconds

  // --- Oscillator A -------------------------
  int oscAWaveform = 0;        // 0=Saw, 1=Square
  double oscAFreq = 440.0;     // Hz (Base)
  double oscAPulseWidth = 0.5; // 0.0 to 1.0
  bool oscASync = false;       // Hard Sync to B?

  // --- Oscillator B -------------------------
  int oscBWaveform = 0;      // 0=Saw, 1=Square, 2=Tri
  double oscBFreq = 440.0;   // Hz
  double oscBFineTune = 0.0; // Semitones
  double oscBPulseWidth = 0.5;
  bool oscBLoFreq = false;        // LFO Mode?
  bool oscBKeyboardPoints = true; // Key tracking

  // --- Mixer --------------------------------
  double mixOscA = 1.0;
  double mixOscB = 0.0;
  double mixNoise = 0.0;

  // --- Filter (VCF) -------------------------
  double filterCutoff = 2000.0; // Hz
  double filterResonance = 0.0; // 0.0 to 1.0
  double filterEnvAmount = 0.0; // 0.0 to 1.0
  int filterModel = 0;          // 0=Classic, 1=Ladder, 2=Prophet12, 3=Prophet24
  bool filterKeyboardTrack =
      false; // Full/Half/Off in hardware, uses float 0-1 for amount maybe?

  // --- Filter Envelope (ADSR) ---------------
  double filterAttack = 0.01;
  double filterDecay = 0.1;
  double filterSustain = 0.5;
  double filterRelease = 0.2;

  // --- Amp Envelope (ADSR) ------------------
  double ampAttack = 0.01;
  double ampDecay = 0.1;
  double ampSustain = 1.0;
  double ampRelease = 0.1;

  // --- LFO (Global) -------------------------
  int lfoShape = 0;      // 0=Sine, 1=Tri, 2=Square, 3=Saw, 4=Rnd
  double lfoRate = 1.0;  // Hz
  double lfoDepth = 0.0; // Global scale factor

  // --- Poly-Mod (Modulation Matrix) ---------
  // Sources are Osc B and Filter Env
  double polyModOscBToFreqA = 0.0; // FM amount
  double polyModOscBToPWM = 0.0;
  double polyModOscBToFilter = 0.0;

  double polyModFilterEnvToFreqA = 0.0;
  double polyModFilterEnvToPWM = 0.0;
  double polyModFilterEnvToFilter = 0.0; // This is usually "Filter Env Amount"

  // --- FX (Epic 4.1) ------------------------
  double fxChorusRate = 0.25;   // Hz
  double fxChorusDepth = 0.5;   // 0.0 to 1.0
  double fxChorusMix = 0.0;     // 0.0 to 1.0
  double fxDelayTime = 0.35;    // Seconds
  double fxDelayFeedback = 0.35; // 0.0 to 0.95
  double fxDelayMix = 0.0;      // 0.0 to 1.0
  double fxLimiterThreshold = 0.95; // 0.0 to 1.0

  // --- Helper Methods -----------------------
  void Reset() {
    masterGain = 1.0;
    oscAWaveform = 0;
    oscAFreq = 440.0;
    // ... set defaults matching hardware or sensible init
    filterCutoff = 20000.0;
    filterResonance = 0.0;
    filterModel = 0;
    polyphony = 8;
    fxChorusRate = 0.25;
    fxChorusDepth = 0.5;
    fxChorusMix = 0.0;
    fxDelayTime = 0.35;
    fxDelayFeedback = 0.35;
    fxDelayMix = 0.0;
    fxLimiterThreshold = 0.95;
  }
};

} // namespace PolySynthCore
