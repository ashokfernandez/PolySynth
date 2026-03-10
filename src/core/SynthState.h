#pragma once

#include "types.h"
#include <stdint.h>
#include <string.h>
#include <type_traits>

namespace PolySynthCore {

// The Single Source of Truth for the Synth Engine
struct SynthState {
  // --- Global -------------------------------
  float masterGain = 0.75f; // 0.0 to 1.0
  int polyphony = 8;        // 1 to 16
  bool unison = false;
  float unisonDetune = 0.0f; // 0.0 to 1.0 (Hz spread)
  float glideTime = 0.0f;    // Seconds

  // --- Voice Allocation (Sprint 2) --------
  int allocationMode = 0;    // 0=Reset (scan from 0), 1=Cycle (round-robin)
  int stealPriority = 0;     // 0=Oldest, 1=LowestPitch, 2=LowestAmplitude
  int unisonCount = 1;       // 1-8 voices per note
  float unisonSpread = 0.0f; // 0.0-1.0 (detune amount)
  float stereoSpread = 0.0f; // 0.0-1.0 (stereo width)

  // --- Oscillator A -------------------------
  int oscAWaveform = 0;        // 0=Saw, 1=Square
  float oscAFreq = 440.0f;     // Hz (Base)
  float oscAPulseWidth = 0.5f; // 0.0 to 1.0
  bool oscASync = false;       // Hard Sync to B?

  // --- Oscillator B -------------------------
  int oscBWaveform = 0;      // 0=Saw, 1=Square, 2=Tri
  float oscBFreq = 440.0f;   // Hz
  float oscBFineTune = 0.0f; // Semitones
  float oscBPulseWidth = 0.5f;
  bool oscBLoFreq = false;        // LFO Mode?
  bool oscBKeyboardPoints = true; // Key tracking

  // --- Mixer --------------------------------
  float mixOscA = 1.0f;
  float mixOscB = 0.0f;
  float mixNoise = 0.0f;

  // --- Filter (VCF) -------------------------
  float filterCutoff = 2000.0f; // Hz
  float filterResonance = 0.0f; // 0.0 to 1.0
  float filterEnvAmount = 0.0f; // 0.0 to 1.0
  int filterModel = 1;          // 0=Classic, 1=Ladder, 2=Cascade12, 3=Cascade24
  bool filterKeyboardTrack =
      false; // Full/Half/Off in hardware, uses float 0-1 for amount maybe?

  // --- Filter Envelope (ADSR) ---------------
  float filterAttack = 0.01f;
  float filterDecay = 0.1f;
  float filterSustain = 0.5f;
  float filterRelease = 0.2f;

  // --- Amp Envelope (ADSR) ------------------
  float ampAttack = 0.01f;
  float ampDecay = 0.1f;
  float ampSustain = 1.0f;
  float ampRelease = 0.1f;

  // --- LFO (Global) -------------------------
  int lfoShape = 0;      // 0=Sine, 1=Tri, 2=Square, 3=Saw, 4=Rnd
  float lfoRate = 1.0f;  // Hz
  float lfoDepth = 0.0f; // Global scale factor

  // --- Poly-Mod (Modulation Matrix) ---------
  // Sources are Osc B and Filter Env
  float polyModOscBToFreqA = 0.0f; // FM amount
  float polyModOscBToPWM = 0.0f;
  float polyModOscBToFilter = 0.0f;

  float polyModFilterEnvToFreqA = 0.0f;
  float polyModFilterEnvToPWM = 0.0f;
  float polyModFilterEnvToFilter = 0.0f; // This is usually "Filter Env Amount"

  // --- FX (Epic 4.1) ------------------------
  float fxChorusRate = 0.25f;       // Hz
  float fxChorusDepth = 0.5f;       // 0.0 to 1.0
  float fxChorusMix = 0.0f;         // 0.0 to 1.0
  float fxDelayTime = 0.35f;        // Seconds
  float fxDelayFeedback = 0.35f;    // 0.0 to 0.95
  float fxDelayMix = 0.0f;          // 0.0 to 1.0
  float fxLimiterThreshold = 0.95f; // 0.0 to 1.0

  // --- Helper Methods -----------------------
  void Reset() { *this = SynthState{}; }
};

// SynthState must remain an aggregate so Reset() via value-initialization
// works.
static_assert(std::is_aggregate_v<SynthState>,
              "SynthState must remain an aggregate for Reset() to work");

} // namespace PolySynthCore
