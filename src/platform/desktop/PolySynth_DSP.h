#pragma once

#include "../../core/VoiceManager.h"
#include "IPlugConstants.h"

using namespace iplug;

class PolySynthDSP {
public:
  PolySynthDSP(int nVoices) { mVoiceManager.Init(44100.0); }

  void Reset(double sampleRate, int blockSize) {
    mVoiceManager.Init(sampleRate);
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
    } else if (status == IMidiMsg::kControlChange) {
      // Handle MIDI CC?
    }
  }

  void SetParam(int paramIdx, double value) {
    static double a = 0.01, d = 0.1, s = 0.5, r = 0.2;
    static double filterCutoff = 20000.0;
    static double filterRes = 0.707;

    switch (paramIdx) {
    case kParamGain:
      // TODO
      break;
    case kParamAttack:
      a = value / 1000.0;
      mVoiceManager.SetADSR(a, d, s, r);
      break;
    case kParamDecay:
      d = value / 1000.0;
      mVoiceManager.SetADSR(a, d, s, r);
      break;
    case kParamSustain:
      s = value / 100.0;
      mVoiceManager.SetADSR(a, d, s, r);
      break;
    case kParamRelease:
      r = value / 1000.0;
      mVoiceManager.SetADSR(a, d, s, r);
      break;
    case kParamFilterCutoff:
      filterCutoff = value;
      mVoiceManager.SetFilter(filterCutoff, filterRes);
      break;
    case kParamFilterResonance:
      filterRes = value / 100.0; // Assume 0-100%
      mVoiceManager.SetFilter(filterCutoff, filterRes);
      break;
    default:
      break;
    }
  }

  PolySynthCore::VoiceManager mVoiceManager;
};
