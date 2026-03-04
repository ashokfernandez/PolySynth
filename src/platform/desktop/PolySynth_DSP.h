#pragma once

#include "../../core/Engine.h"
#include "../../core/SynthState.h"
#include "IPlugConstants.h"
#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;

class PolySynthDSP {
public:
  PolySynthDSP(int /*nVoices*/) {}

  void Reset(double sampleRate, int blockSize) {
    mEngine.Init(sampleRate);
  }

  void UpdateState(const PolySynthCore::SynthState &state) {
    mEngine.UpdateState(state);
  }

  void ProcessBlock(sample **inputs, sample **outputs, int nOutputs,
                    int nFrames, double qnPos = 0.,
                    bool transportIsRunning = false) {
    mEngine.Process(inputs, outputs, nFrames, nOutputs);
    mEngine.UpdateVisualization();
  }

  void ProcessMidiMsg(const IMidiMsg &msg) {
    int status = msg.StatusMsg();
    if (status == IMidiMsg::kNoteOn) {
      mEngine.OnNoteOn(msg.NoteNumber(), msg.Velocity());
    } else if (status == IMidiMsg::kNoteOff) {
      mEngine.OnNoteOff(msg.NoteNumber());
    } else if (status == IMidiMsg::kControlChange) {
      if (msg.mData1 == 64) {
        mEngine.OnSustainPedal(msg.mData2 >= 64);
      }
    }
  }

  int GetActiveVoiceCount() const {
    return mEngine.GetActiveVoiceCount();
  }

  int GetHeldNotes(std::array<int, PolySynthCore::kMaxVoices> &buf) const {
    return mEngine.GetHeldNotes(buf);
  }

private:
  PolySynthCore::Engine mEngine;
};
