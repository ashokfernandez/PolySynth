#include "PolySynth.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include "LFO.h"

PolySynthPlugin::PolySynthPlugin(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)
      ->InitMilliseconds("Note Glide Time", 0., 0.0, 30.);
  GetParam(kParamAttack)
      ->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)
      ->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)
      ->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone,
                   "ADSR");
  GetParam(kParamRelease)
      ->InitDouble("Release", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR");

  // LFO Params
  GetParam(kParamLFOShape)
      ->InitEnum("LFO Shape", LFO<>::kTriangle, {LFO_SHAPE_VALIST});
  GetParam(kParamLFORateHz)->InitFrequency("LFO Rate", 1., 0.01, 40.);
  GetParam(kParamLFORateTempo)
      ->InitEnum("LFO Rate", LFO<>::k1, {LFO_TEMPODIV_VALIST});
  GetParam(kParamLFORateMode)->InitBool("LFO Sync", true);
  GetParam(kParamLFODepth)->InitPercentage("LFO Depth");

  // Filter Params
  GetParam(kParamFilterCutoff)
      ->InitDouble("Cutoff", 20000., 20., 20000., 1., "Hz", IParam::kFlagsNone,
                   "Filter", IParam::ShapeExp());
  GetParam(kParamFilterResonance)
      ->InitDouble("Resonance", 0., 0., 100., 1., "%");

#if IPLUG_EDITOR
  mEditorInitFunc = [&]() {
    LoadFile("index.html", GetBundleID());
    EnableScroll(false);
  };
#endif
}

#if IPLUG_DSP
void PolySynthPlugin::ProcessBlock(sample **inputs, sample **outputs,
                                   int nFrames) {
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}

void PolySynthPlugin::OnIdle() {}

void PolySynthPlugin::OnReset() { mDSP.Reset(GetSampleRate(), GetBlockSize()); }

void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  mDSP.ProcessMidiMsg(msg);
}

void PolySynthPlugin::OnParamChange(int paramIdx) {
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

bool PolySynthPlugin::OnMessage(int msgTag, int ctrlTag, int dataSize,
                                const void *pData) {
  return false;
}

bool PolySynthPlugin::CanNavigateToURL(const char *url) { return true; }

bool PolySynthPlugin::OnCanDownloadMIMEType(const char *mimeType) {
  return std::string_view(mimeType) != "text/html";
}

void PolySynthPlugin::OnDownloadedFile(const char *path) {
  WDL_String str;
  str.SetFormatted(64, "Downloaded file to %s\n", path);
  LoadHTML(str.Get());
}

void PolySynthPlugin::OnFailedToDownloadFile(const char *path) {
  WDL_String str;
  str.SetFormatted(64, "Failed to download file to %s\n", path);
  LoadHTML(str.Get());
}

void PolySynthPlugin::OnGetLocalDownloadPathForFile(const char *fileName,
                                                    WDL_String &localPath) {
  DesktopPath(localPath);
  localPath.AppendFormatted(MAX_WIN32_PATH_LEN, "/%s", fileName);
}
#endif
