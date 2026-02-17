#pragma once

#include "../../core/SynthState.h"
#include "../../core/VoiceManager.h"
#include "IPlugConstants.h"
#include "IPlug_include_in_plug_hdr.h"
#include <sea_dsp/effects/sea_vintage_chorus.h>
#include <sea_dsp/effects/sea_vintage_delay.h>
#include <sea_dsp/sea_limiter.h>

using namespace iplug;

class PolySynthDSP {
public:
  PolySynthDSP(int nVoices) {
    mVoiceManager.Init(44100.0);
    mChorus.Init(44100.0);
    mDelay.Init(44100.0, 2000.0);
    mLimiter.Init(44100.0);
  }

  void Reset(double sampleRate, int blockSize) {
    mVoiceManager.Init(sampleRate);
    mChorus.Init(sampleRate);
    mDelay.Init(sampleRate, 2000.0);
    mDelay.Clear();
    mLimiter.Init(sampleRate);
  }

  // Called from ProcessBlock in the plugin, before audio processing
  void UpdateState(const PolySynthCore::SynthState &state) {
    mGain = state.masterGain;

    mVoiceManager.SetADSR(state.ampAttack, state.ampDecay, state.ampSustain,
                          state.ampRelease);
    mVoiceManager.SetFilterEnv(state.filterAttack, state.filterDecay,
                               state.filterSustain, state.filterRelease);
    mVoiceManager.SetFilter(state.filterCutoff, state.filterResonance,
                            state.filterEnvAmount);
    mVoiceManager.SetFilterModel(state.filterModel);

    mVoiceManager.SetWaveformA(
        static_cast<sea::Oscillator::WaveformType>(state.oscAWaveform));
    mVoiceManager.SetWaveformB(
        static_cast<sea::Oscillator::WaveformType>(state.oscBWaveform));
    mVoiceManager.SetPulseWidthA(state.oscAPulseWidth);
    mVoiceManager.SetPulseWidthB(state.oscBPulseWidth);
    mVoiceManager.SetMixer(state.mixOscA, state.mixOscB,
                           state.oscBFineTune * 100.0);

    mVoiceManager.SetLFO(state.lfoShape, state.lfoRate, state.lfoDepth);

    mVoiceManager.SetPolyModOscBToFreqA(state.polyModOscBToFreqA);
    mVoiceManager.SetPolyModOscBToPWM(state.polyModOscBToPWM);
    mVoiceManager.SetPolyModOscBToFilter(state.polyModOscBToFilter);
    mVoiceManager.SetPolyModFilterEnvToFreqA(state.polyModFilterEnvToFreqA);
    mVoiceManager.SetPolyModFilterEnvToPWM(state.polyModFilterEnvToPWM);
    mVoiceManager.SetPolyModFilterEnvToFilter(state.polyModFilterEnvToFilter);

    mVoiceManager.SetGlideTime(state.glideTime);
    mVoiceManager.SetPolyphonyLimit(state.polyphony);
    mVoiceManager.SetAllocationMode(state.allocationMode);
    mVoiceManager.SetStealPriority(state.stealPriority);
    mVoiceManager.SetUnisonCount(state.unisonCount);
    mVoiceManager.SetUnisonSpread(state.unisonSpread);
    mVoiceManager.SetStereoSpread(state.stereoSpread);

    // Chorus
    mChorus.SetRate(state.fxChorusRate);
    mChorus.SetDepth(state.fxChorusDepth * 5.0); // Map 0-1 to 0-5ms
    mChorus.SetMix(state.fxChorusMix);

    // Delay
    mDelay.SetTime(state.fxDelayTime * 1000.0);        // Seconds to ms
    mDelay.SetFeedback(state.fxDelayFeedback * 100.0); // 0-1 to percent
    mDelay.SetMix(state.fxDelayMix * 100.0);           // 0-1 to percent

    // Limiter
    mLimiter.SetParams(state.fxLimiterThreshold, 5.0, 50.0);
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
      double left = 0.0;
      double right = 0.0;
      mVoiceManager.ProcessStereo(left, right);

      left *= mGain;
      right *= mGain;

      // Process Chorus
      double cOutL, cOutR;
      mChorus.Process(left, right, &cOutL, &cOutR);
      left = cOutL;
      right = cOutR;

      // Process Delay
      mDelay.Process(left, right, &cOutL, &cOutR);
      left = cOutL;
      right = cOutR;

      // Process Limiter
      mLimiter.Process(left, right);

      if (nOutputs > 0)
        outputs[0][s] = static_cast<sample>(left);
      if (nOutputs > 1)
        outputs[1][s] = static_cast<sample>(right);
    }
  }

  void ProcessMidiMsg(const IMidiMsg &msg) {
    int status = msg.StatusMsg();

    if (status == IMidiMsg::kNoteOn) {
      mVoiceManager.OnNoteOn(msg.NoteNumber(), msg.Velocity());
    } else if (status == IMidiMsg::kNoteOff) {
      mVoiceManager.OnNoteOff(msg.NoteNumber());
    } else if (status == IMidiMsg::kControlChange) {
      if (msg.mData1 == 64) { // Sustain Pedal
        mVoiceManager.OnSustainPedal(msg.mData2 >= 64);
      }
    }
  }

  int GetActiveVoiceCount() const {
    return mVoiceManager.GetActiveVoiceCount();
  }

  int GetHeldNotes(std::array<int, PolySynthCore::kMaxVoices> &buf) const {
    return mVoiceManager.GetHeldNotes(buf);
  }

  void SetParam(int paramIdx, double value) {
    // Legacy
  }

  PolySynthCore::VoiceManager mVoiceManager;
  sea::VintageChorus<double> mChorus;
  sea::VintageDelay<double> mDelay;
  sea::LookaheadLimiter<double> mLimiter;

private:
  double mGain = 1.0;
};
