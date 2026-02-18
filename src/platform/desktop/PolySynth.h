#pragma once
#include "../../core/SynthState.h"
#include "DemoSequencer.h"
#include "IPlug_include_in_plug_hdr.h"

#if !IPLUG_DSP && !IPLUG_EDITOR
#error "PolySynth requires at least one of IPLUG_DSP or IPLUG_EDITOR"
#endif

#if IPLUG_EDITOR
#include "IGraphics.h"
#endif

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
  kParamPresetSelect,
  // 3 separate params for mutual exclusion via UI/DSP
  kParamDemoMono,
  kParamDemoPoly,
  kParamDemoFX,
  kParamPolyphonyLimit,
  kParamAllocationMode,
  kParamStealPriority,
  kParamUnisonCount,
  kParamUnisonSpread,
  kParamStereoSpread,
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
  kMsgTagPresetSelect = 22,
  kCtrlTagEnvelope,
  // Phase 1: Oscillator controls
  kCtrlTagOscWave,
  kCtrlTagOscBWave,
  kCtrlTagOscMix,
  kCtrlTagPulseWidthA,
  kCtrlTagPulseWidthB,
  // Phase 2: Gain + LFO controls
  kCtrlTagGain,
  kCtrlTagLFOShape,
  kCtrlTagLFORate,
  kCtrlTagLFODepth,
  // Phase 4: Poly-Mod matrix controls
  kCtrlTagPolyModOscBToFreqA,
  kCtrlTagPolyModOscBToPWM,
  kCtrlTagPolyModOscBToFilter,
  kCtrlTagPolyModEnvToFreqA,
  kCtrlTagPolyModEnvToPWM,
  kCtrlTagPolyModEnvToFilter,
  // Phase 5: Demo button controls
  kCtrlTagDemoMono,
  kCtrlTagDemoPoly,
  kCtrlTagDemoFX,
  kCtrlTagPresetSelect,
  kCtrlTagSaveBtn,
  kCtrlTagActiveVoices,
  kCtrlTagChordName,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class PolySynthPlugin final : public Plugin {
public:
  PolySynthPlugin(const InstanceInfo &info);

#if IPLUG_EDITOR
  void OnUIOpen() override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnLayout(IGraphics *pGraphics);
  void PopulatePresetMenu();

private:
  void BuildHeader(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildOscillators(IGraphics *g, const IRECT &bounds,
                        const IVStyle &style);
  void BuildFilter(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildEnvelope(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildLFO(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildPolyMod(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildChorus(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildDelay(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildMaster(IGraphics *g, const IRECT &bounds, const IVStyle &style);
  void BuildFooter(IGraphics *g, const IRECT &bounds, const IVStyle &style);
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
  DemoSequencer mDemoSequencer;
  bool mIsUpdatingUI = false;
#endif

private:
  PolySynthCore::SynthState mState;
  bool mIsDirty = false;
};
