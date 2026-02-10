#pragma once
#include "../../core/SynthState.h"
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
  kMsgTagTestLoaded,
  kMsgTagDemoMono,
  kMsgTagDemoPoly,
  kMsgTagSavePreset,
  kMsgTagLoadPreset,
  kMsgTagPreset1, // Warm Pad
  kMsgTagPreset2, // Bright Lead
  kMsgTagPreset3, // Dark Bass
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

private:
  PolySynthDSP mDSP{8};
  PolySynthCore::SynthState mState;
  int mDemoMode = 0; // 0 = Off, 1 = Mono, 2 = Poly
  long long mDemoSampleCounter = 0;
  int mDemoNoteIndex = 0;
#endif
};
