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

      // Apply Gain
      out *= (mGain / 100.0);

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
    switch (paramIdx) {
    case kParamGain:
      mGain = value;
      break;
    case kParamAttack:
      mAttack = value / 1000.0;
      mVoiceManager.SetADSR(mAttack, mDecay, mSustain, mRelease);
      break;
    case kParamDecay:
      mDecay = value / 1000.0;
      mVoiceManager.SetADSR(mAttack, mDecay, mSustain, mRelease);
      break;
    case kParamSustain:
      mSustain = value / 100.0;
      mVoiceManager.SetADSR(mAttack, mDecay, mSustain, mRelease);
      break;
    case kParamRelease:
      mRelease = value / 1000.0;
      mVoiceManager.SetADSR(mAttack, mDecay, mSustain, mRelease);
      break;
    case kParamFilterCutoff:
      mFilterCutoff = value;
      mVoiceManager.SetFilter(mFilterCutoff, mFilterRes);
      break;
    case kParamFilterResonance:
      mFilterRes = value / 100.0;
      mVoiceManager.SetFilter(mFilterCutoff, mFilterRes);
      break;
    case kParamOscWave:
      mVoiceManager.SetWaveform(
          static_cast<PolySynthCore::Oscillator::WaveformType>((int)value));
      break;
    case kParamLFOShape:
      mVoiceManager.SetLFOType((int)value);
      break;
    case kParamLFORateHz:
      mVoiceManager.SetLFORate(value);
      break;
    case kParamLFODepth:
      mVoiceManager.SetLFODepth(value / 100.0);
      break;
    default:
      break;
    }
  }

  PolySynthCore::VoiceManager mVoiceManager;

private:
  double mGain = 100.0;
  double mAttack = 0.01;
  double mDecay = 0.1;
  double mSustain = 0.5;
  double mRelease = 0.2;
  double mFilterCutoff = 20000.0;
  double mFilterRes = 0.707;
};
