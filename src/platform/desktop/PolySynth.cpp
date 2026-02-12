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

#if IPLUG_EDITOR
class SectionFrame final : public IControl {
public:
  SectionFrame(const IRECT &bounds, const char *title, const IColor &borderColor,
               const IColor &textColor, const IColor &bgColor = COLOR_TRANSPARENT)
      : IControl(bounds), mTitle(title), mBorderColor(borderColor),
        mTextColor(textColor), mBgColor(bgColor) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    if (mBgColor.A > 0)
      g.FillRect(mBgColor, mRECT);

    g.DrawRect(mBorderColor, mRECT, nullptr, 1.5f);
    g.DrawText(IText(18.f, &mTextColor, "Roboto-Regular", IText::kStyleNormal,
                     IText::kAlignNear),
               mTitle.Get(), mRECT.GetPadded(-10.f).GetFromTop(24.f));
  }

private:
  WDL_String mTitle;
  IColor mBorderColor;
  IColor mTextColor;
  IColor mBgColor;
};
#endif
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
  const IColor panelBg = IColor(255, 239, 237, 230);
  const IColor groupBorder = IColor(255, 70, 70, 70);
  const IColor textDark = IColor(255, 22, 22, 22);
  const IColor accentRed = IColor(255, 235, 74, 57);
  const IColor accentCyan = IColor(255, 60, 218, 230);

  pGraphics->AttachPanelBackground(panelBg);

  const IVStyle synthStyle = DEFAULT_STYLE
                                 .WithRoundness(0.12f)
                                 .WithShadowOffset(0.0f)
                                 .WithLabelText(IText(15.f, &textDark,
                                                      "Roboto-Regular",
                                                      IText::kStyleNormal,
                                                      IText::kAlignCenter))
                                 .WithValueText(IText(12.f, &textDark,
                                                      "Roboto-Regular",
                                                      IText::kStyleNormal,
                                                      IText::kAlignCenter))
                                 .WithColor(kBG, IColor(255, 247, 245, 240))
                                 .WithColor(kFG, textDark)
                                 .WithColor(kPR, accentRed)
                                 .WithColor(kHL, accentCyan)
                                 .WithColor(kFR, groupBorder);

  IRECT bounds = pGraphics->GetBounds().GetPadded(-14.f);

  const IRECT topHeader = bounds.GetFromTop(58.f);
  const IRECT controlsArea = bounds.GetFromTop(bounds.H() - 74.f);
  const IRECT footerArea = bounds.GetFromBottom(64.f);

  pGraphics->AttachControl(
      new SectionFrame(topHeader, "OSAKA POLY-84", groupBorder, textDark,
                       IColor(255, 246, 244, 238)));

  const IRECT ledRect = topHeader.GetCentredInside(58.f, 36.f).GetTranslated(92.f, 0.f);
  pGraphics->AttachControl(new SectionFrame(ledRect, "REC", accentRed, accentRed,
                                            IColor(255, 34, 8, 8)));

  const float knobSize = 72.f;
  const float smallKnob = 62.f;

  auto attachFrame = [&](const IRECT &r, const char *label) {
    pGraphics->AttachControl(new SectionFrame(r, label, groupBorder, textDark));
    return r.GetPadded(-8.f).GetFromBottom(r.H() - 24.f);
  };

  // 2x4 matrix inspired grouping.
  IRECT oscArea = controlsArea.GetGridCell(0, 0, 2, 4);
  IRECT filterArea = controlsArea.GetGridCell(0, 1, 2, 4);
  IRECT envArea = controlsArea.GetGridCell(0, 2, 2, 4);
  IRECT lfoArea = controlsArea.GetGridCell(0, 3, 2, 4);
  IRECT polyModArea = controlsArea.GetGridCell(1, 0, 2, 4);
  IRECT fxArea = controlsArea.GetGridCell(1, 1, 2, 4);
  IRECT masterArea = controlsArea.GetGridCell(1, 2, 2, 4);
  IRECT demoArea = controlsArea.GetGridCell(1, 3, 2, 4);

  const IRECT oscInner = attachFrame(oscArea, "OSCILLATORS");
  pGraphics->AttachControl(new IVKnobControl(oscInner.GetGridCell(0, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamOscWave, "Wave A", synthStyle),
                           kCtrlTagOscWave);
  pGraphics->AttachControl(new IVKnobControl(oscInner.GetGridCell(0, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamOscBWave, "Wave B", synthStyle),
                           kCtrlTagOscBWave);
  pGraphics->AttachControl(new IVKnobControl(oscInner.GetGridCell(1, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamOscPulseWidthA, "Pulse A", synthStyle),
                           kCtrlTagPulseWidthA);
  pGraphics->AttachControl(new IVKnobControl(oscInner.GetGridCell(1, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamOscPulseWidthB, "Pulse B", synthStyle),
                           kCtrlTagPulseWidthB);

  const IRECT filterInner = attachFrame(filterArea, "FILTER");
  pGraphics->AttachControl(new IVKnobControl(filterInner.GetGridCell(0, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamFilterCutoff, "Cutoff", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(filterInner.GetGridCell(0, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamFilterResonance, "Reso", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(filterInner.GetGridCell(1, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamFilterEnvAmount, "Env Amt", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(filterInner.GetGridCell(1, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamFilterModel, "Model", synthStyle));

  const IRECT envInner = attachFrame(envArea, "AMP ENVELOPE");
  const IRECT envVisualizerArea = envInner.GetFromTop(envInner.H() * 0.45f).GetPadded(-4.f);
  const IRECT envFadersArea = envInner.GetFromBottom(envInner.H() * 0.52f).GetPadded(-4.f);

  Envelope *pEnvelope = new Envelope(envVisualizerArea, synthStyle);
  pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                     GetParam(kParamDecay)->Value() / 1000.f,
                     GetParam(kParamSustain)->Value() / 100.f,
                     GetParam(kParamRelease)->Value() / 1000.f);
  pEnvelope->SetColors(accentCyan, accentCyan.WithOpacity(0.2f));
  pGraphics->AttachControl(pEnvelope, kCtrlTagEnvelope);

  const int nFaders = 4;
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 0, 1, nFaders), kParamAttack, "A",
      synthStyle));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 1, 1, nFaders), kParamDecay, "D",
      synthStyle));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 2, 1, nFaders), kParamSustain, "S",
      synthStyle));
  pGraphics->AttachControl(new IVSliderControl(
      envFadersArea.GetGridCell(0, 3, 1, nFaders), kParamRelease, "R",
      synthStyle));

  const IRECT lfoInner = attachFrame(lfoArea, "LFO");
  pGraphics->AttachControl(new IVKnobControl(lfoInner.GetGridCell(0, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamLFOShape, "Shape", synthStyle),
                           kCtrlTagLFOShape);
  pGraphics->AttachControl(new IVKnobControl(lfoInner.GetGridCell(0, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamLFORateHz, "Rate", synthStyle),
                           kCtrlTagLFORate);
  pGraphics->AttachControl(new IVKnobControl(lfoInner.GetGridCell(1, 0, 2, 2).GetCentredInside(knobSize),
                                             kParamLFODepth, "Depth", synthStyle),
                           kCtrlTagLFODepth);
  pGraphics->AttachControl(new IVKnobControl(lfoInner.GetGridCell(1, 1, 2, 2).GetCentredInside(knobSize),
                                             kParamOscMix, "Osc Mix", synthStyle),
                           kCtrlTagOscMix);

  const IRECT polyInner = attachFrame(polyModArea, "POLY MOD");
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(0, 0, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModOscBToFreqA, "B→Freq A", synthStyle),
                           kCtrlTagPolyModOscBToFreqA);
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(0, 1, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModOscBToPWM, "B→PWM", synthStyle),
                           kCtrlTagPolyModOscBToPWM);
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(0, 2, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModOscBToFilter, "B→Filter", synthStyle),
                           kCtrlTagPolyModOscBToFilter);
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(1, 0, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModFilterEnvToFreqA, "Env→Freq A", synthStyle),
                           kCtrlTagPolyModEnvToFreqA);
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(1, 1, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModFilterEnvToPWM, "Env→PWM", synthStyle),
                           kCtrlTagPolyModEnvToPWM);
  pGraphics->AttachControl(new IVKnobControl(polyInner.GetGridCell(1, 2, 2, 3).GetCentredInside(smallKnob),
                                             kParamPolyModFilterEnvToFilter, "Env→Filter", synthStyle),
                           kCtrlTagPolyModEnvToFilter);

  const IRECT fxInner = attachFrame(fxArea, "FX");
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(0, 0, 2, 3).GetCentredInside(smallKnob),
                                             kParamChorusRate, "Chorus Rt", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(0, 1, 2, 3).GetCentredInside(smallKnob),
                                             kParamChorusDepth, "Chorus Dp", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(0, 2, 2, 3).GetCentredInside(smallKnob),
                                             kParamChorusMix, "Chorus Mix", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(1, 0, 2, 3).GetCentredInside(smallKnob),
                                             kParamDelayTime, "Delay T", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(1, 1, 2, 3).GetCentredInside(smallKnob),
                                             kParamDelayFeedback, "Delay FB", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(fxInner.GetGridCell(1, 2, 2, 3).GetCentredInside(smallKnob),
                                             kParamDelayMix, "Delay Mix", synthStyle));

  const IRECT masterInner = attachFrame(masterArea, "MASTER");
  pGraphics->AttachControl(new IVKnobControl(masterInner.GetGridCell(0, 0, 1, 2).GetCentredInside(knobSize),
                                             kParamGain, "Gain", synthStyle),
                           kCtrlTagGain);
  pGraphics->AttachControl(new IVKnobControl(masterInner.GetGridCell(0, 1, 1, 2).GetCentredInside(knobSize),
                                             kParamLimiterThreshold, "Limiter", synthStyle));
  pGraphics->AttachControl(new IVKnobControl(masterInner.GetGridCell(1, 0, 1, 2).GetCentredInside(knobSize),
                                             kParamNoteGlideTime, "Glide", synthStyle));

  const IRECT demoInner = attachFrame(demoArea, "DEMO");
  IVStyle pillStyle = synthStyle.WithRoundness(1.f);
  pGraphics->AttachControl(new IVSwitchControl(
                             demoInner.GetFromTop(demoInner.H() * 0.32f)
                                 .GetCentredInside(demoInner.W() - 24.f, 30.f),
                             kParamDemoMono, "Mono", pillStyle),
                           kCtrlTagDemoMono);
  pGraphics->AttachControl(new IVSwitchControl(
                             demoInner.GetCentredInside(demoInner.W() - 24.f, 30.f),
                             kParamDemoPoly, "Poly", pillStyle),
                           kCtrlTagDemoPoly);
  pGraphics->AttachControl(new IVSwitchControl(
                             demoInner.GetFromBottom(demoInner.H() * 0.34f)
                                 .GetCentredInside(demoInner.W() - 24.f, 30.f),
                             kParamDemoFX, "FX", pillStyle),
                           kCtrlTagDemoFX);

  pGraphics->AttachControl(new SectionFrame(footerArea, "OUTPUT", groupBorder,
                                            textDark, IColor(255, 8, 8, 8)));
  const IRECT footerText = footerArea.GetPadded(-14.f);
  pGraphics->AttachControl(new ITextControl(
      footerText.GetFromBottom(22.f), "Signal Monitor", IText(14.f, &accentCyan,
                                                                "Roboto-Regular",
                                                                IText::kStyleNormal,
                                                                IText::kAlignNear)));
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
