#pragma once

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
  kNumParams
};

#if IPLUG_DSP
// will use EParams in PolySynth_DSP.h
#include "PolySynth_DSP.h"
#endif

enum EControlTags {
  kCtrlTagMeter = 0,
  kCtrlTagLFOVis,
  kCtrlTagScope,
  kCtrlTagRTText,
  kCtrlTagKeyboard,
  kCtrlTagBender,
  kNumCtrlTags
};

using namespace iplug;

class PolySynthPlugin final : public Plugin {
public:
  PolySynthPlugin(const InstanceInfo &info);

  // WebView Overrides (Always available)
  bool CanNavigateToURL(const char *url);
  bool OnCanDownloadMIMEType(const char *mimeType) override;
  void OnFailedToDownloadFile(const char *path) override;
  void OnDownloadedFile(const char *path) override;
  void OnGetLocalDownloadPathForFile(const char *fileName,
                                     WDL_String &localPath) override;

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
#endif
};
