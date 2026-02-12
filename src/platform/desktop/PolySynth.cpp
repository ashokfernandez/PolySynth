#include "PolySynth.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#if IPLUG_EDITOR
#include "Envelope.h"
#include "IControls.h"
#endif
#include <cstdlib>

namespace {
static const double kToPercentage = 100.0;
static const double kToMs = 1000.0;
} // namespace

void PolySynthPlugin::SyncUIState() {
  GetParam(kParamGain)->Set(mState.masterGain * kToPercentage);
  GetParam(kParamNoteGlideTime)->Set(mState.glideTime * kToMs);
  GetParam(kParamAttack)->Set(mState.ampAttack * kToMs);
  GetParam(kParamDecay)->Set(mState.ampDecay * kToMs);
  GetParam(kParamSustain)->Set(mState.ampSustain * kToPercentage);
  GetParam(kParamRelease)->Set(mState.ampRelease * kToMs);
  GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
  GetParam(kParamLFORateHz)->Set(mState.lfoRate);
  GetParam(kParamLFODepth)->Set(mState.lfoDepth * kToPercentage);
  GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
  GetParam(kParamFilterResonance)->Set(mState.filterResonance * kToPercentage);
  GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * kToPercentage);
  GetParam(kParamFilterModel)->Set((double)mState.filterModel);
  GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
  GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
  GetParam(kParamOscMix)->Set(mState.mixOscB * kToPercentage);
  GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * kToPercentage);
  GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * kToPercentage);
  GetParam(kParamPolyModOscBToFreqA)
      ->Set(mState.polyModOscBToFreqA * kToPercentage);
  GetParam(kParamPolyModOscBToPWM)
      ->Set(mState.polyModOscBToPWM * kToPercentage);
  GetParam(kParamPolyModOscBToFilter)
      ->Set(mState.polyModOscBToFilter * kToPercentage);
  GetParam(kParamPolyModFilterEnvToFreqA)
      ->Set(mState.polyModFilterEnvToFreqA * kToPercentage);
  GetParam(kParamPolyModFilterEnvToPWM)
      ->Set(mState.polyModFilterEnvToPWM * kToPercentage);
  GetParam(kParamPolyModFilterEnvToFilter)
      ->Set(mState.polyModFilterEnvToFilter * kToPercentage);
  GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
  GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * kToPercentage);
  GetParam(kParamChorusMix)->Set(mState.fxChorusMix * kToPercentage);
  GetParam(kParamDelayTime)->Set(mState.fxDelayTime * kToMs);
  GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * kToPercentage);
  GetParam(kParamDelayMix)->Set(mState.fxDelayMix * kToPercentage);
  GetParam(kParamLimiterThreshold)
      ->Set(mState.fxLimiterThreshold * kToPercentage);

  // Phase 5: Sync demo mode buttons based on sequencer state
  GetParam(kParamDemoMono)->Set(
      mDemoSequencer.GetMode() == DemoSequencer::Mode::Mono ? 1.0 : 0.0);
  GetParam(kParamDemoPoly)->Set(
      mDemoSequencer.GetMode() == DemoSequencer::Mode::Poly ? 1.0 : 0.0);
  GetParam(kParamDemoFX)->Set(
      mDemoSequencer.GetMode() == DemoSequencer::Mode::FX ? 1.0 : 0.0);

  for (int i = 0; i < kNumParams; ++i) {
    SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
  }
}

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
      ->InitEnum("LFO Shape", 1, {"Sine", "Triangle", "Square", "Saw"});
  GetParam(kParamLFORateHz)->InitFrequency("LFO Rate", 1., 0.01, 40.);
  GetParam(kParamLFORateTempo)
      ->InitEnum("LFO Rate", 0, {"1/1", "1/2", "1/4", "1/8", "1/16"});
  GetParam(kParamLFORateMode)->InitBool("LFO Sync", true);
  GetParam(kParamLFODepth)->InitPercentage("LFO Depth");

  // Filter Params
  GetParam(kParamFilterCutoff)
      ->InitDouble("Cutoff", 20000., 20., 20000., 1., "Hz", IParam::kFlagsNone,
                   "Filter", IParam::ShapeExp());
  GetParam(kParamFilterResonance)
      ->InitDouble("Resonance", 0., 0., 100., 1., "%");
  GetParam(kParamFilterEnvAmount)->InitPercentage("Filter Env");
  GetParam(kParamFilterModel)
      ->InitEnum("Filter Model", 0,
                 {"Classic", "Ladder", "Prophet 12", "Prophet 24"});

  // Oscillator Params
  GetParam(kParamOscWave)
      ->InitEnum("Osc Waveform", (int)sea::Oscillator::WaveformType::Saw,
                 {"Saw", "Square", "Triangle", "Sine"});
  GetParam(kParamOscBWave)
      ->InitEnum("Osc B Waveform", (int)sea::Oscillator::WaveformType::Sine,
                 {"Saw", "Square", "Triangle", "Sine"});
  GetParam(kParamOscMix)->InitDouble("Osc Mix", 0., 0., 100., 1., "%");
  GetParam(kParamOscPulseWidthA)->InitPercentage("Pulse Width A");
  GetParam(kParamOscPulseWidthB)->InitPercentage("Pulse Width B");

  // Poly-Mod Params
  GetParam(kParamPolyModOscBToFreqA)->InitPercentage("B -> Freq A");
  GetParam(kParamPolyModOscBToPWM)->InitPercentage("B -> PWM A");
  GetParam(kParamPolyModOscBToFilter)->InitPercentage("B -> Filter");
  GetParam(kParamPolyModFilterEnvToFreqA)->InitPercentage("Env -> Freq A");
  GetParam(kParamPolyModFilterEnvToPWM)->InitPercentage("Env -> PWM A");
  GetParam(kParamPolyModFilterEnvToFilter)->InitPercentage("Env -> Filter");

  // FX Params
  GetParam(kParamChorusRate)->InitFrequency("Chorus Rate", 0.25, 0.05, 2.0);
  GetParam(kParamChorusDepth)
      ->InitDouble("Chorus Depth", 50., 0., 100., 1., "%");
  GetParam(kParamChorusMix)->InitDouble("Chorus Mix", 0., 0., 100., 1., "%");
  GetParam(kParamDelayTime)->InitMilliseconds("Delay Time", 350., 50., 1200.);
  GetParam(kParamDelayFeedback)
      ->InitDouble("Delay Feedback", 35., 0., 95., 1., "%");
  GetParam(kParamDelayMix)->InitDouble("Delay Mix", 0., 0., 100., 1., "%");

  // Phase 5: Demo mode buttons
  GetParam(kParamDemoMono)->InitBool("Demo Mono", false);
  GetParam(kParamDemoPoly)->InitBool("Demo Poly", false);
  GetParam(kParamDemoFX)->InitBool("Demo FX", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [this](IGraphics *pGraphics) { OnLayout(pGraphics); };
#endif
}

#if IPLUG_EDITOR
void PolySynthPlugin::OnUIOpen() {
  // The base class already syncs parameter values to the UI on open.
  // No need to manually re-send them here.
}

void PolySynthPlugin::OnParamChangeUI(int paramIdx, EParamSource source) {
  // Update the envelope visualizer when ADSR params change from the UI
  if (paramIdx == kParamAttack || paramIdx == kParamDecay ||
      paramIdx == kParamSustain || paramIdx == kParamRelease) {
    if (GetUI()) {
      Envelope *pEnvelope = dynamic_cast<Envelope *>(
          GetUI()->GetControlWithTag(kCtrlTagEnvelope));
      if (pEnvelope) {
        pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                           GetParam(kParamDecay)->Value() / 1000.f,
                           GetParam(kParamSustain)->Value() / 100.f,
                           GetParam(kParamRelease)->Value() / 1000.f);
      }
    }
  }
}

void PolySynthPlugin::OnLayout(IGraphics *pGraphics) {
  if (pGraphics->NControls())
    return;

  pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);

  IRECT b = pGraphics->GetBounds();
  const float footerH = 50.f;
  const float polyModH = 120.f;  // Phase 4: Space for poly-mod section
  const IRECT footerArea = b.ReduceFromBottom(footerH);
  const IRECT polyModArea = b.ReduceFromBottom(polyModH);
  const IRECT mainArea = b;

  // 3-column layout
  const int nCols = 3;
  const IRECT oscCol = mainArea.GetGridCell(0, 0, 1, nCols);
  const IRECT filterCol = mainArea.GetGridCell(0, 1, 1, nCols);
  const IRECT envCol = mainArea.GetGridCell(0, 2, 1, nCols);

  // Phase 1: Oscillators Section - Groups horizontal, controls within groups horizontal
  const IRECT oscKnobs = oscCol.GetPadded(-10.f);
  const float knobSize = 75.f;

  // Top row: Waveform group (Osc A Wave | Osc B Wave)
  const IRECT waveRow = oscKnobs.FracRectVertical(0.4f, true).GetPadded(-5.f);
  pGraphics->AttachControl(new IVKnobControl(
      waveRow.GetGridCell(0, 0, 1, 2).GetCentredInside(knobSize),
      kParamOscWave, "Osc A"), kCtrlTagOscWave);
  pGraphics->AttachControl(new IVKnobControl(
      waveRow.GetGridCell(0, 1, 1, 2).GetCentredInside(knobSize),
      kParamOscBWave, "Osc B"), kCtrlTagOscBWave);

  // Middle row: Pulse Width group (PW A | PW B)
  const IRECT pwRow = oscKnobs.FracRectVertical(0.4f, false).FracRectVertical(0.6f, true).GetPadded(-5.f);
  pGraphics->AttachControl(new IVKnobControl(
      pwRow.GetGridCell(0, 0, 1, 2).GetCentredInside(knobSize),
      kParamOscPulseWidthA, "PW A"), kCtrlTagPulseWidthA);
  pGraphics->AttachControl(new IVKnobControl(
      pwRow.GetGridCell(0, 1, 1, 2).GetCentredInside(knobSize),
      kParamOscPulseWidthB, "PW B"), kCtrlTagPulseWidthB);

  // Bottom row: Mix (centered)
  const IRECT mixRow = oscKnobs.FracRectVertical(0.4f, false).FracRectVertical(0.6f, false).GetPadded(-5.f);
  pGraphics->AttachControl(new IVKnobControl(
      mixRow.GetCentredInside(knobSize),
      kParamOscMix, "Mix"), kCtrlTagOscMix);

  // Phase 2: LFO Section (in former filter column, top portion)
  const IRECT lfoArea = filterCol.FracRectVertical(0.5f, true).GetPadded(-10.f);
  pGraphics->AttachControl(new IVKnobControl(
      lfoArea.GetGridCell(0, 0, 2, 2).GetCentredInside(knobSize),
      kParamLFOShape, "LFO Shape"), kCtrlTagLFOShape);
  pGraphics->AttachControl(new IVKnobControl(
      lfoArea.GetGridCell(0, 1, 2, 2).GetCentredInside(knobSize),
      kParamLFORateHz, "LFO Rate"), kCtrlTagLFORate);
  pGraphics->AttachControl(new IVKnobControl(
      lfoArea.GetGridCell(1, 0, 2, 2).GetCentredInside(knobSize),
      kParamLFODepth, "LFO Depth"), kCtrlTagLFODepth);

  // Phase 2: Master Gain (in LFO area, bottom right)
  pGraphics->AttachControl(new IVKnobControl(
      lfoArea.GetGridCell(1, 1, 2, 2).GetCentredInside(knobSize),
      kParamGain, "Gain"), kCtrlTagGain);

  // Filter Section (moved to bottom half of filter column)
  const IRECT filterArea = filterCol.FracRectVertical(0.5f, false).GetPadded(-10.f);
  pGraphics->AttachControl(new IVKnobControl(
      filterArea.GetGridCell(0, 0, 2, 1).GetCentredInside(knobSize),
      kParamFilterCutoff, "Cutoff"));
  pGraphics->AttachControl(new IVKnobControl(
      filterArea.GetGridCell(1, 0, 2, 1).GetCentredInside(knobSize),
      kParamFilterResonance, "Resonance"));

  // Envelope Section
  const IRECT envVisualizerArea =
      envCol.FracRectVertical(0.4f, true).GetPadded(-10.f);
  const IRECT envFadersArea =
      envCol.FracRectVertical(0.6f, false).GetPadded(-10.f);

  Envelope *pEnvelope = new Envelope(envVisualizerArea);
  pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                     GetParam(kParamDecay)->Value() / 1000.f,
                     GetParam(kParamSustain)->Value() / 100.f,
                     GetParam(kParamRelease)->Value() / 1000.f);
  pGraphics->AttachControl(pEnvelope, kCtrlTagEnvelope);

  const int nFaders = 4;
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 0, 1, nFaders), kParamAttack, "A"));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 1, 1, nFaders), kParamDecay, "D"));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 2, 1, nFaders), kParamSustain, "S"));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 3, 1, nFaders), kParamRelease, "R"));

  // Phase 4: Poly-Mod Matrix Section (6 knobs in 2 rows x 3 cols)
  const IRECT polyModKnobs = polyModArea.GetPadded(-10.f);
  const float polyModKnobSize = 65.f;

  // Row 1: Osc B modulation sources (B→Freq A, B→PWM, B→Filter)
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(0, 0, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModOscBToFreqA, "B→Freq A"), kCtrlTagPolyModOscBToFreqA);
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(0, 1, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModOscBToPWM, "B→PWM"), kCtrlTagPolyModOscBToPWM);
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(0, 2, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModOscBToFilter, "B→Filter"), kCtrlTagPolyModOscBToFilter);

  // Row 2: Envelope modulation sources (Env→Freq A, Env→PWM, Env→Filter)
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(1, 0, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModFilterEnvToFreqA, "Env→Freq A"), kCtrlTagPolyModEnvToFreqA);
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(1, 1, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModFilterEnvToPWM, "Env→PWM"), kCtrlTagPolyModEnvToPWM);
  pGraphics->AttachControl(new IVKnobControl(
      polyModKnobs.GetGridCell(1, 2, 2, 3).GetCentredInside(polyModKnobSize),
      kParamPolyModFilterEnvToFilter, "Env→Filter"), kCtrlTagPolyModEnvToFilter);

  // Preset save/load buttons (restores missing React functionality)
  const float presetButtonW = 90.f;
  const float presetButtonH = 32.f;
  const float presetButtonGap = 8.f;
  const IRECT presetButtonsArea =
      footerArea.GetFromLeft(2 * presetButtonW + presetButtonGap + 20.f)
          .GetMidVPadded(presetButtonH);

  pGraphics->AttachControl(new IVButtonControl(
      presetButtonsArea.GetGridCell(0, 0, 1, 2),
      [this](IControl *) { SendArbitraryMsgFromUI(kMsgTagSavePreset); },
      "Save"));
  pGraphics->AttachControl(new IVButtonControl(
      presetButtonsArea.GetGridCell(0, 1, 1, 2),
      [this](IControl *) { SendArbitraryMsgFromUI(kMsgTagLoadPreset); },
      "Load"));

  // Phase 5: Footer - 3 demo toggle buttons
  IVStyle pillStyle = DEFAULT_STYLE.WithRoundness(1.0f);
  const float buttonW = 100.f, buttonH = 35.f, spacing = 10.f;
  const IRECT demoArea =
      footerArea.GetFromRight(3 * buttonW + 2 * spacing + 10.f)
          .GetMidVPadded(buttonH);

  pGraphics->AttachControl(
      new IVSwitchControl(demoArea.GetGridCell(0, 0, 1, 3),
                          kParamDemoMono, "Mono", pillStyle),
      kCtrlTagDemoMono);
  pGraphics->AttachControl(
      new IVSwitchControl(demoArea.GetGridCell(0, 1, 1, 3),
                          kParamDemoPoly, "Poly", pillStyle),
      kCtrlTagDemoPoly);
  pGraphics->AttachControl(
      new IVSwitchControl(demoArea.GetGridCell(0, 2, 1, 3),
                          kParamDemoFX, "FX", pillStyle),
      kCtrlTagDemoFX);
}
#endif

#if IPLUG_DSP
void PolySynthPlugin::ProcessBlock(sample **inputs, sample **outputs,
                                   int nFrames) {
  mDemoSequencer.Process(nFrames, GetSampleRate(), mDSP);

  mDSP.UpdateState(mState);
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}

void PolySynthPlugin::OnIdle() {}

void PolySynthPlugin::OnReset() {
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  // Ensure DSP has latest state on reset
  mDSP.UpdateState(mState);
}

// Log note on messages
void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  mDSP.ProcessMidiMsg(msg);
}

void PolySynthPlugin::OnParamChange(int paramIdx) {
  double value = GetParam(paramIdx)->Value();

  switch (paramIdx) {
  case kParamGain:
    mState.masterGain = value / kToPercentage;
    break;
  case kParamNoteGlideTime:
    mState.glideTime = value / kToMs;
    break;
  case kParamAttack:
    mState.ampAttack = value / kToMs;
    break;
  case kParamDecay:
    mState.ampDecay = value / kToMs;
    break;
  case kParamSustain:
    mState.ampSustain = value / kToPercentage;
    break;
  case kParamRelease:
    mState.ampRelease = value / kToMs;
    break;
  case kParamLFOShape:
    mState.lfoShape = static_cast<int>(value);
    break;
  case kParamLFORateHz:
    mState.lfoRate = value;
    break;
  case kParamLFODepth:
    mState.lfoDepth = value / kToPercentage;
    break;
  case kParamFilterCutoff:
    mState.filterCutoff = value;
    break;
  case kParamFilterResonance:
    mState.filterResonance = value / kToPercentage;
    break;
  case kParamFilterEnvAmount:
    mState.filterEnvAmount = value / kToPercentage;
    break;
  case kParamFilterModel:
    mState.filterModel = static_cast<int>(value);
    break;
  case kParamOscWave:
    mState.oscAWaveform = static_cast<int>(value);
    break;
  case kParamOscBWave:
    mState.oscBWaveform = static_cast<int>(value);
    break;
  case kParamOscMix:
    mState.mixOscA = 1.0 - (value / kToPercentage);
    mState.mixOscB = value / kToPercentage;
    break;
  case kParamOscPulseWidthA:
    mState.oscAPulseWidth = value / kToPercentage;
    break;
  case kParamOscPulseWidthB:
    mState.oscBPulseWidth = value / kToPercentage;
    break;
  case kParamPolyModOscBToFreqA:
    mState.polyModOscBToFreqA = value / kToPercentage;
    break;
  case kParamPolyModOscBToPWM:
    mState.polyModOscBToPWM = value / kToPercentage;
    break;
  case kParamPolyModOscBToFilter:
    mState.polyModOscBToFilter = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToFreqA:
    mState.polyModFilterEnvToFreqA = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToPWM:
    mState.polyModFilterEnvToPWM = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToFilter:
    mState.polyModFilterEnvToFilter = value / kToPercentage;
    break;
  case kParamChorusRate:
    mState.fxChorusRate = value;
    break;
  case kParamChorusDepth:
    mState.fxChorusDepth = value / kToPercentage;
    break;
  case kParamChorusMix:
    mState.fxChorusMix = value / kToPercentage;
    break;
  case kParamDelayTime:
    mState.fxDelayTime = value / kToMs;
    break;
  case kParamDelayFeedback:
    mState.fxDelayFeedback = value / kToPercentage;
    break;
  case kParamDelayMix:
    mState.fxDelayMix = value / kToPercentage;
    break;
  case kParamLimiterThreshold:
    mState.fxLimiterThreshold = value / kToPercentage;
    break;
  // Phase 5: Demo mode mutual exclusion
  case kParamDemoMono:
    if (value > 0.5) {
      GetParam(kParamDemoPoly)->Set(0.0);
      GetParam(kParamDemoFX)->Set(0.0);
      mDemoSequencer.SetMode(DemoSequencer::Mode::Mono, GetSampleRate(), mDSP);
    } else {
      mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
    }
    break;
  case kParamDemoPoly:
    if (value > 0.5) {
      GetParam(kParamDemoMono)->Set(0.0);
      GetParam(kParamDemoFX)->Set(0.0);
      mDemoSequencer.SetMode(DemoSequencer::Mode::Poly, GetSampleRate(), mDSP);
    } else {
      mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
    }
    break;
  case kParamDemoFX:
    if (value > 0.5) {
      GetParam(kParamDemoMono)->Set(0.0);
      GetParam(kParamDemoPoly)->Set(0.0);
      mDemoSequencer.SetMode(DemoSequencer::Mode::FX, GetSampleRate(), mDSP);
      // Set FX parameters (matching OnMessage kMsgTagDemoFX behavior)
      mState.fxChorusRate = 0.35;
      mState.fxChorusDepth = 0.65;
      mState.fxChorusMix = 0.35;
      mState.fxDelayTime = 0.45;
      mState.fxDelayFeedback = 0.45;
      mState.fxDelayMix = 0.35;
      mState.fxLimiterThreshold = 0.7;
      SyncUIState();
    } else {
      mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
      // Reset FX to defaults
      mState.fxChorusMix = 0.0;
      mState.fxDelayMix = 0.0;
      mState.fxLimiterThreshold = 0.95;
      SyncUIState();
    }
    break;
  default:
    break;
  }

  // Feedback loop for Envelope visualizer
  if (paramIdx == kParamAttack || paramIdx == kParamDecay ||
      paramIdx == kParamSustain || paramIdx == kParamRelease) {
    if (GetUI()) {
      Envelope *pEnvelope = dynamic_cast<Envelope *>(
          GetUI()->GetControlWithTag(kCtrlTagEnvelope));
      if (pEnvelope) {
        pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                           GetParam(kParamDecay)->Value() / 1000.f,
                           GetParam(kParamSustain)->Value() / 100.f,
                           GetParam(kParamRelease)->Value() / 1000.f);
      }
    }
  }
}

bool PolySynthPlugin::OnMessage(int msgTag, int ctrlTag, int dataSize,
                                const void *pData) {
  if (msgTag == kMsgTagTestLoaded) {
    if (std::getenv("POLYSYNTH_TEST_UI")) {
      printf("TEST_PASS: UI Loaded\n");
      exit(0);
    }
  } else if (msgTag == kMsgTagDemoMono) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::Mono, GetSampleRate(), mDSP);
    return true;
  } else if (msgTag == kMsgTagDemoPoly) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::Poly, GetSampleRate(), mDSP);
    return true;
  } else if (msgTag == kMsgTagDemoFX) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::FX, GetSampleRate(), mDSP);
    if (mDemoSequencer.GetMode() == DemoSequencer::Mode::FX) {
      mState.fxChorusRate = 0.35;
      mState.fxChorusDepth = 0.65;
      mState.fxChorusMix = 0.35;
      mState.fxDelayTime = 0.45;
      mState.fxDelayFeedback = 0.45;
      mState.fxDelayMix = 0.35;
      mState.fxLimiterThreshold = 0.7;
    } else {
      mState.fxChorusMix = 0.0;
      mState.fxDelayMix = 0.0;
      mState.fxLimiterThreshold = 0.95;
    }
    // Update UI and DSP
    SyncUIState();
    return true;
  } else if (msgTag == kMsgTagSavePreset) {
    // Save current state to demo preset file
    WDL_String presetPath;
    DesktopPath(presetPath);
    presetPath.Append("/PolySynth_DemoPreset.json");
    bool success =
        PolySynthCore::PresetManager::SaveToFile(mState, presetPath.Get());
    printf("PRESET_SAVE: %s (%s)\n", presetPath.Get(), success ? "OK" : "FAIL");
    return true;
  } else if (msgTag == kMsgTagLoadPreset) {
    // Load state from demo preset file
    WDL_String presetPath;
    DesktopPath(presetPath);
    presetPath.Append("/PolySynth_DemoPreset.json");
    PolySynthCore::SynthState loadedState;
    bool success = PolySynthCore::PresetManager::LoadFromFile(presetPath.Get(),
                                                              loadedState);
    if (success) {
      mState = loadedState;
      // Sync UI with loaded state by updating all parameters
      GetParam(kParamGain)->Set(mState.masterGain * 100.0);
      GetParam(kParamAttack)->Set(mState.ampAttack * 1000.0);
      GetParam(kParamDecay)->Set(mState.ampDecay * 1000.0);
      GetParam(kParamSustain)->Set(mState.ampSustain * 100.0);
      GetParam(kParamRelease)->Set(mState.ampRelease * 1000.0);
      GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
      GetParam(kParamFilterResonance)->Set(mState.filterResonance * 100.0);
      GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * 100.0);
      GetParam(kParamFilterModel)->Set((double)mState.filterModel);
      GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
      GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
      GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
      GetParam(kParamLFORateHz)->Set(mState.lfoRate);
      GetParam(kParamLFODepth)->Set(mState.lfoDepth * 100.0);
      GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * 100.0);
      GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * 100.0);
      GetParam(kParamPolyModOscBToFreqA)
          ->Set(mState.polyModOscBToFreqA * 100.0);
      GetParam(kParamPolyModOscBToPWM)->Set(mState.polyModOscBToPWM * 100.0);
      GetParam(kParamPolyModOscBToFilter)
          ->Set(mState.polyModOscBToFilter * 100.0);
      GetParam(kParamPolyModFilterEnvToFreqA)
          ->Set(mState.polyModFilterEnvToFreqA * 100.0);
      GetParam(kParamPolyModFilterEnvToPWM)
          ->Set(mState.polyModFilterEnvToPWM * 100.0);
      GetParam(kParamPolyModFilterEnvToFilter)
          ->Set(mState.polyModFilterEnvToFilter * 100.0);
      GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
      GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * 100.0);
      GetParam(kParamChorusMix)->Set(mState.fxChorusMix * 100.0);
      GetParam(kParamDelayTime)->Set(mState.fxDelayTime * 1000.0);
      GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * 100.0);
      GetParam(kParamDelayMix)->Set(mState.fxDelayMix * 100.0);
      GetParam(kParamLimiterThreshold)->Set(mState.fxLimiterThreshold * 100.0);

      // Notify UI of all param changes
      for (int i = 0; i < kNumParams; ++i) {
        SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
      }
      printf("PRESET_LOAD: %s (OK)\n", presetPath.Get());
    } else {
      printf("PRESET_LOAD: %s (FAIL - file not found or invalid)\n",
             presetPath.Get());
    }
    return true;
  } else if (msgTag == kMsgTagPreset1 || msgTag == kMsgTagPreset2 ||
             msgTag == kMsgTagPreset3) {
    // Factory Presets with distinct sounds
    if (msgTag == kMsgTagPreset1) {
      // Warm Pad - Slow attack, low cutoff, no resonance
      mState.masterGain = 0.8;
      mState.ampAttack = 0.5; // 500ms
      mState.ampDecay = 0.3;
      mState.ampSustain = 0.7;
      mState.ampRelease = 1.0; // 1 second
      mState.filterCutoff = 800.0;
      mState.filterResonance = 0.1;
      mState.filterModel = 0;
      mState.oscAWaveform = 0; // Saw
      mState.oscBWaveform = 3; // Sine
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 0; // Sine
      mState.lfoRate = 0.5;
      mState.lfoDepth = 0.3;
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Warm Pad loaded\n");
    } else if (msgTag == kMsgTagPreset2) {
      // Bright Lead - Fast attack, high cutoff, high resonance
      mState.masterGain = 1.0;
      mState.ampAttack = 0.005; // 5ms
      mState.ampDecay = 0.1;
      mState.ampSustain = 0.6;
      mState.ampRelease = 0.2;
      mState.filterCutoff = 18000.0;
      mState.filterResonance = 0.7;
      mState.filterModel = 2;
      mState.oscAWaveform = 1; // Square
      mState.oscBWaveform = 1; // Square
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 2; // Square LFO
      mState.lfoRate = 6.0;
      mState.lfoDepth = 0.0; // No LFO
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Bright Lead loaded\n");
    } else if (msgTag == kMsgTagPreset3) {
      // Dark Bass - Medium attack, very low cutoff, medium resonance
      mState.masterGain = 0.9;
      mState.ampAttack = 0.02; // 20ms
      mState.ampDecay = 0.5;
      mState.ampSustain = 0.4;
      mState.ampRelease = 0.15;
      mState.filterCutoff = 300.0;
      mState.filterResonance = 0.5;
      mState.filterModel = 1;
      mState.oscAWaveform = 0; // Saw
      mState.oscBWaveform = 2; // Triangle
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 1; // Triangle
      mState.lfoRate = 2.0;
      mState.lfoDepth = 0.5;
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Dark Bass loaded\n");
    }

    mState.fxChorusRate = 0.25;
    mState.fxChorusDepth = 0.5;
    mState.fxChorusMix = 0.0;
    mState.fxDelayTime = 0.35;
    mState.fxDelayFeedback = 0.35;
    mState.fxDelayMix = 0.0;
    mState.fxLimiterThreshold = 0.95;

    // Sync UI with loaded state by updating all parameters
    GetParam(kParamGain)->Set(mState.masterGain * 100.0);
    GetParam(kParamAttack)->Set(mState.ampAttack * 1000.0);
    GetParam(kParamDecay)->Set(mState.ampDecay * 1000.0);
    GetParam(kParamSustain)->Set(mState.ampSustain * 100.0);
    GetParam(kParamRelease)->Set(mState.ampRelease * 1000.0);
    GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
    GetParam(kParamFilterResonance)->Set(mState.filterResonance * 100.0);
    GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * 100.0);
    GetParam(kParamFilterModel)->Set((double)mState.filterModel);
    GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
    GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
    GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
    GetParam(kParamLFORateHz)->Set(mState.lfoRate);
    GetParam(kParamLFODepth)->Set(mState.lfoDepth * 100.0);
    GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * 100.0);
    GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * 100.0);
    GetParam(kParamPolyModOscBToFreqA)->Set(mState.polyModOscBToFreqA * 100.0);
    GetParam(kParamPolyModOscBToPWM)->Set(mState.polyModOscBToPWM * 100.0);
    GetParam(kParamPolyModOscBToFilter)
        ->Set(mState.polyModOscBToFilter * 100.0);
    GetParam(kParamPolyModFilterEnvToFreqA)
        ->Set(mState.polyModFilterEnvToFreqA * 100.0);
    GetParam(kParamPolyModFilterEnvToPWM)
        ->Set(mState.polyModFilterEnvToPWM * 100.0);
    GetParam(kParamPolyModFilterEnvToFilter)
        ->Set(mState.polyModFilterEnvToFilter * 100.0);
    GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
    GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * 100.0);
    GetParam(kParamChorusMix)->Set(mState.fxChorusMix * 100.0);
    GetParam(kParamDelayTime)->Set(mState.fxDelayTime * 1000.0);
    GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * 100.0);
    GetParam(kParamDelayMix)->Set(mState.fxDelayMix * 100.0);
    GetParam(kParamLimiterThreshold)->Set(mState.fxLimiterThreshold * 100.0);

    // Notify UI of all param changes
    for (int i = 0; i < kNumParams; ++i) {
      SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }
    return true;
  } else if (msgTag == kMsgTagNoteOn) {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(ctrlTag, 100, 0);
    mDSP.ProcessMidiMsg(msg);
    return true;
  } else if (msgTag == kMsgTagNoteOff) {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(ctrlTag, 0);
    mDSP.ProcessMidiMsg(msg);
    return true;
  }
  return false;
}

#endif
