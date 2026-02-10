#pragma once
#include "../../core/SynthState.h"
#include "DemoSequencer.h"
#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams {
  kParamGain = 0,
  kParamNoteGlideTime,
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kParamLFOShape,
  kParamLFORateHz,
  kParamLFORateTempo,
  kParamLFORateMode,
  kParamLFODepth,
  kParamFilterCutoff,
  kParamFilterResonance,
  kParamOscWave,
  kParamOscBWave,
  kParamOscMix,
  kParamOscPulseWidthA,
  kParamOscPulseWidthB,
  kParamFilterEnvAmount,
  kParamPolyModOscBToFreqA,
  kParamPolyModOscBToPWM,
  kParamPolyModOscBToFilter,
  kParamPolyModFilterEnvToFreqA,
  kParamPolyModFilterEnvToPWM,
  kParamPolyModFilterEnvToFilter,
  kParamFilterModel,
  kParamChorusRate,
  kParamChorusDepth,
  kParamChorusMix,
  kParamDelayTime,
  kParamDelayFeedback,
  kParamDelayMix,
  kParamLimiterThreshold,
  kParamDemoMode,
  kNumParams
};

#if IPLUG_DSP
#include "PolySynth_DSP.h"
#endif

enum EControlTags {
  kCtrlTagMeter = 0,
  kCtrlTagLFOVis,
  kCtrlTagScope,
  kCtrlTagRTText,
  kCtrlTagKeyboard,
  kCtrlTagBender,
  kMsgTagTestLoaded = 6,
  kMsgTagDemoMono = 7,
  kMsgTagDemoPoly = 8,
  kMsgTagSavePreset = 9,
  kMsgTagLoadPreset = 10,
  kMsgTagPreset1 = 11,
  kMsgTagPreset2 = 12,
  kMsgTagPreset3 = 13,
  kMsgTagDemoFX = 14,
  kMsgTagNoteOn = 20,
  kMsgTagNoteOff = 21,
  kCtrlTagEnvelope,
  kNumCtrlTags
};

using namespace iplug;

class PolySynthPlugin final : public Plugin {
public:
  PolySynthPlugin(const InstanceInfo &info);

  bool CanNavigateToURL(const char *url);
  bool OnCanDownloadMIMEType(const char *mimeType) override;
  void OnFailedToDownloadFile(const char *path) override;
  void OnDownloadedFile(const char *path) override;
  void OnGetLocalDownloadPathForFile(const char *fileName,
                                     WDL_String &localPath) override;
#if IPLUG_EDITOR
  void OnUIOpen() override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnLayout(IGraphics *pGraphics) override;
#endif

#if IPLUG_DSP
public:
  void ProcessBlock(sample **inputs, sample **outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg &msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize,
                 const void *pData) override;

  void SyncUIState();

private:
  PolySynthDSP mDSP{8};
  PolySynthCore::SynthState mState;
  DemoSequencer mDemoSequencer;
#endif
};
