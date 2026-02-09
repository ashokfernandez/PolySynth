#pragma once

#include "../../core/SynthState.h"
#include "../../core/VoiceManager.h"
#include "IPlugConstants.h"

using namespace iplug;

class PolySynthDSP {
public:
  PolySynthDSP(int nVoices) { mVoiceManager.Init(44100.0); }

  void Reset(double sampleRate, int blockSize) {
    mVoiceManager.Init(sampleRate);
  }

  // Called from ProcessBlock in the plugin, before audio processing
  void UpdateState(const PolySynthCore::SynthState &state) {
    // Global
    mGain = state.masterGain;

    // Dispatch to VoiceManager which currently handles all voices uniformly
    // This is a temporary bridge until VoiceManager reads state directly
    mVoiceManager.SetADSR(state.ampAttack, state.ampDecay, state.ampSustain,
                          state.ampRelease);
    mVoiceManager.SetFilter(state.filterCutoff, state.filterResonance);

    // Map Oscillator types
    // SynthState uses: 0=Saw, 1=Square (Osc A)
    // Oscillator.h uses: Saw=0, Square=1. Matches.
    mVoiceManager.SetWaveform(
        static_cast<PolySynthCore::Oscillator::WaveformType>(
            state.oscAWaveform));

    // LFO
    mVoiceManager.SetLFOType(state.lfoShape);
    mVoiceManager.SetLFORate(state.lfoRate);
    mVoiceManager.SetLFODepth(state.lfoDepth);
  }

  void ProcessBlock(sample **inputs, sample **outputs, int nOutputs,
                    int nFrames, double qnPos = 0.,
                    bool transportIsRunning = false) {
    // Clear outputs
    for (int i = 0; i < nOutputs; i++) {
      memset(outputs[i], 0, nFrames * sizeof(sample));
    }

    // Process one sample at a time
    for (int s = 0; s < nFrames; s++) {
      PolySynthCore::sample_t out = mVoiceManager.Process();

      // Apply Gain
      out *= mGain; // mGain is now 0.0-1.0 from state

      // Mono to Stereo copy
      if (nOutputs > 0)
        outputs[0][s] = (sample)out;
      if (nOutputs > 1)
        outputs[1][s] = (sample)out;
    }
  }

  void ProcessMidiMsg(const IMidiMsg &msg) {
    int status = msg.StatusMsg();

    if (status == IMidiMsg::kNoteOn) {
      mVoiceManager.OnNoteOn(msg.NoteNumber(), msg.Velocity());
    } else if (status == IMidiMsg::kNoteOff) {
      mVoiceManager.OnNoteOff(msg.NoteNumber());
    }
  }

  // SetParam is legacy now, used only if direct param access is needed,
  // but we prefer UpdateState. Leaving empty or routing to a "Pending Change"
  // queue if strictly necessary, but for this Epic we switch to State.
  void SetParam(int paramIdx, double value) {
    // No-op or TODO: remove once Plugin calls are removed
  }

  PolySynthCore::VoiceManager mVoiceManager;

private:
  double mGain = 1.0;
};
