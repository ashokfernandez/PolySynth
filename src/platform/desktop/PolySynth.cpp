#include "PolySynth.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#if IPLUG_EDITOR
#include "Envelope.h"
#include "IControls.h"
#endif
#include <cstdlib>
#include <vector>

namespace {
static const double kToPercentage = 100.0;
static const double kToMs = 1000.0;

#if IPLUG_EDITOR
class SectionFrame final : public IControl {
public:
  SectionFrame(const IRECT &bounds, const char *title,
               const IColor &borderColor, const IColor &textColor,
               const IColor &bgColor = COLOR_TRANSPARENT)
      : IControl(bounds), mTitle(title), mBorderColor(borderColor),
        mTextColor(textColor), mBgColor(bgColor) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    if (mBgColor.A > 0)
      g.FillRect(mBgColor, mRECT);
    g.DrawRect(mBorderColor, mRECT, nullptr, 1.25f);
    if (mTitle.GetLength()) {
      // Larger, bolder section headers, well-offset from corners
      g.DrawText(
          IText(18.f, mTextColor, "Roboto-Bold", EAlign::Near), mTitle.Get(),
          mRECT.GetPadded(-12.f).GetFromTop(24.f).GetTranslated(4.f, 4.f));
    }
  }

private:
  WDL_String mTitle;
  IColor mBorderColor;
  IColor mTextColor;
  IColor mBgColor;
};

// Precisely Stacked Control (Label -> Knob -> Value)
void AttachStackedControl(IGraphics *pGraphics, IRECT bounds, int paramIdx,
                          const char *label, const IVStyle &style,
                          bool isSlider = false) {
  // Relative sizing based on cell constraints
  float knobSize = std::min(bounds.W(), bounds.H() * 0.7f) * 0.82f;
  if (isSlider)
    knobSize = std::min(bounds.W() * 0.75f, bounds.H() * 0.75f);

  IRECT controlRect = bounds.GetCentredInside(knobSize);
  if (isSlider)
    controlRect = bounds.GetCentredInside(bounds.W() * 0.7f, bounds.H() * 0.7f);

  // Tight label positioning relative to the control
  const float labelH = 18.f;
  const float valueH = 16.f;

  // Label Above (Bold, closer to knob)
  IText labelTextBold =
      IText(15.f, style.labelText.mFGColor, "Roboto-Bold", EAlign::Center);
  IRECT labelRect = IRECT(bounds.L, controlRect.T - labelH - 1.f, bounds.R,
                          controlRect.T - 1.f);

  // Value Below (Larger, closer to knob)
  IText valueTextBold =
      IText(14.f, style.valueText.mFGColor, "Roboto-Regular", EAlign::Center);
  IRECT valueRect = IRECT(bounds.L, controlRect.B + 1.f, bounds.R,
                          controlRect.B + valueH + 1.f);

  pGraphics->AttachControl(new ITextControl(labelRect, label, labelTextBold));

  if (isSlider) {
    pGraphics->AttachControl(
        new IVSliderControl(controlRect, paramIdx, "",
                            style.WithShowLabel(false).WithShowValue(false)));
  } else {
    pGraphics->AttachControl(
        new IVKnobControl(controlRect, paramIdx, "",
                          style.WithShowLabel(false).WithShowValue(false)));
  }

  pGraphics->AttachControl(
      new ICaptionControl(valueRect, paramIdx, valueTextBold, false));
}
#endif
} // namespace

#if IPLUG_DSP
void PolySynthPlugin::SyncUIState() {
  mIsUpdatingUI = true;
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
  GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
  GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * kToPercentage);
  GetParam(kParamChorusMix)->Set(mState.fxChorusMix * kToPercentage);
  GetParam(kParamDelayTime)->Set(mState.fxDelayTime * kToMs);
  GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * kToPercentage);
  GetParam(kParamDelayMix)->Set(mState.fxDelayMix * kToPercentage);
  GetParam(kParamLimiterThreshold)
      ->Set((1.0 - mState.fxLimiterThreshold) * kToPercentage);
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

  // Sync demo buttons based on current sequencer mode
  GetParam(kParamDemoMono)
      ->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::Mono ? 1.0 : 0.0);
  GetParam(kParamDemoPoly)
      ->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::Poly ? 1.0 : 0.0);
  GetParam(kParamDemoFX)
      ->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::FX ? 1.0 : 0.0);

  for (int i = 0; i < kNumParams; ++i) {
    SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
  }
  mIsUpdatingUI = false;
}

PolySynthPlugin::PolySynthPlugin(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  mState.Reset(); // Ensure audible defaults (Gain 1.0, etc.)

  GetParam(kParamGain)->InitDouble("Gain", 75., 0., 100., 1.25, "%");
  GetParam(kParamNoteGlideTime)->InitMilliseconds("Glide", 0., 0.0, 30.);
  GetParam(kParamAttack)
      ->InitDouble("Att", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR",
                   IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)
      ->InitDouble("Dec", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR",
                   IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)
      ->InitDouble("Sus", 50., 0., 100., 1, "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)
      ->InitDouble("Rel", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR");

  GetParam(kParamLFOShape)->InitEnum("LFO", 1, {"Sin", "Tri", "Sqr", "Saw"});
  GetParam(kParamLFORateHz)->InitFrequency("Rate", 1., 0.01, 40.);
  GetParam(kParamLFODepth)->InitPercentage("Dep");

  GetParam(kParamFilterCutoff)
      ->InitDouble("Cutoff", 2000.0, 20., 20000., 1., "Hz", IParam::kFlagsNone,
                   "Filter", IParam::ShapeExp());
  GetParam(kParamFilterResonance)->InitDouble("Reso", 0., 0., 100., 1., "%");
  GetParam(kParamFilterEnvAmount)->InitPercentage("Env");
  GetParam(kParamFilterModel)->InitEnum("Model", 0, {"CL", "LD", "P12", "P24"});

  GetParam(kParamOscWave)
      ->InitEnum("OscA", (int)sea::Oscillator::WaveformType::Saw,
                 {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamOscBWave)
      ->InitEnum("OscB", (int)sea::Oscillator::WaveformType::Sine,
                 {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamOscMix)->InitDouble("Mix", 0., 0., 100., 1., "%");
  GetParam(kParamOscPulseWidthA)->InitPercentage("PWA");
  GetParam(kParamOscPulseWidthB)->InitPercentage("PWB");

  GetParam(kParamPolyModOscBToFreqA)->InitPercentage("B-F");
  GetParam(kParamPolyModOscBToPWM)->InitPercentage("B-P");
  GetParam(kParamPolyModOscBToFilter)->InitPercentage("B-V");
  GetParam(kParamPolyModFilterEnvToFreqA)->InitPercentage("E-F");
  GetParam(kParamPolyModFilterEnvToPWM)->InitPercentage("E-P");
  GetParam(kParamPolyModFilterEnvToFilter)->InitPercentage("E-V");

  GetParam(kParamChorusRate)->InitFrequency("Rate", 0.25, 0.05, 2.0);
  GetParam(kParamChorusDepth)->InitDouble("Dep", 50., 0., 100., 1., "%");
  GetParam(kParamChorusMix)->InitDouble("Mix", 0., 0., 100., 1., "%");
  GetParam(kParamDelayTime)->InitMilliseconds("Time", 350., 50., 1200.);
  GetParam(kParamDelayFeedback)->InitDouble("Fbk", 35., 0., 95., 1., "%");
  GetParam(kParamDelayMix)->InitDouble("Mix", 0., 0., 100., 1., "%");
  GetParam(kParamLimiterThreshold)->InitPercentage("Lmt", 0.0);

  GetParam(kParamPresetSelect)->InitEnum("Patch", 0, 16);
  GetParam(kParamDemoMono)->InitBool("MONO", true);
  GetParam(kParamDemoPoly)->InitBool("POLY", false);
  GetParam(kParamDemoFX)->InitBool("FX", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  mLayoutFunc = [this](IGraphics *pGraphics) { OnLayout(pGraphics); };
#endif

  // Default to Mono Demo ON for debugging
  mDemoSequencer.SetMode(DemoSequencer::Mode::Mono, GetSampleRate(), mDSP);
}
#endif

#if IPLUG_EDITOR
void PolySynthPlugin::OnUIOpen() {}

void PolySynthPlugin::OnParamChangeUI(int paramIdx, EParamSource source) {
  if (paramIdx != kParamPresetSelect && paramIdx != kParamDemoMono &&
      paramIdx != kParamDemoPoly && paramIdx != kParamDemoFX &&
      source != kPresetRecall) {
    mIsDirty = true;
    if (GetUI())
      GetUI()->GetControlWithTag(kCtrlTagSaveBtn)->SetDirty(false);
  }

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
  pGraphics->LoadFont("Roboto-Bold", ROBOTO_BOLD_FN);

  // Clean Aesthetic Colors
  const IColor panelBg = IColor(255, 239, 237, 230);
  const IColor groupBorder = IColor(255, 120, 120, 120);
  const IColor textDark = IColor(255, 22, 22, 22);
  const IColor accentRed = IColor(255, 235, 74, 57);
  const IColor accentCyan = IColor(255, 60, 218, 230);

  pGraphics->AttachPanelBackground(panelBg);

  const IVStyle synthStyle =
      DEFAULT_STYLE.WithRoundness(0.04f)
          .WithShadowOffset(2.0f)
          .WithShowValue(false)
          .WithLabelText(
              IText(13.f, textDark, "Roboto-Regular", EAlign::Center))
          .WithValueText(
              IText(11.f, textDark, "Roboto-Regular", EAlign::Center))
          .WithColor(kBG, IColor(255, 240, 238, 232))
          .WithColor(kFG, textDark)
          .WithColor(kPR, accentRed)
          .WithColor(kHL, accentCyan)
          .WithColor(kFR, groupBorder);

  IRECT bounds = pGraphics->GetBounds().GetPadded(-12.f);
  const IRECT topHeader = bounds.GetFromTop(72.f);
  const IRECT mainArea = bounds.GetReducedFromTop(76.f);
  const IRECT footerArea = mainArea.GetFromBottom(40.f);
  const IRECT controlsArea = mainArea.GetReducedFromBottom(44.f);

  pGraphics->AttachControl(new SectionFrame(
      topHeader, "", groupBorder, textDark, IColor(255, 246, 244, 238)));
  pGraphics->AttachControl(new ITextControl(
      topHeader.GetPadded(-18.f).GetFromTop(40.f), "OSAKA POLY-84",
      IText(40.f, textDark, "Roboto-Bold", EAlign::Near)));

  // Preset Selection
  const IRECT presetArea = topHeader.GetFromRight(280.f)
                               .GetCentredInside(260.f, 44.f)
                               .GetTranslated(-10.f, 0.f);
  pGraphics->AttachControl(
      new IVMenuButtonControl(presetArea.GetFromLeft(160.f), kParamPresetSelect,
                              "Select Patch", synthStyle),
      kCtrlTagPresetSelect);

  // Premium SAVE button
  IVStyle saveButtonStyle =
      synthStyle.WithColor(kBG, accentRed).WithColor(kFG, COLOR_WHITE);
  saveButtonStyle.labelText.WithFont("Roboto-Bold").WithSize(18.f);
  pGraphics->AttachControl(
      new IVButtonControl(
          presetArea.GetFromRight(70.f),
          [&](IControl *pCaller) {
            mIsDirty = false;
            OnMessage(kMsgTagSavePreset,
                      (int)GetParam(kParamPresetSelect)->Value(), 0, nullptr);
            pCaller->SetDirty(false);
          },
          "SAVE", saveButtonStyle),
      kCtrlTagSaveBtn);

  auto attachFrame = [&](const IRECT &r, const char *label) {
    pGraphics->AttachControl(new SectionFrame(r, label, groupBorder, textDark));
    return r.GetPadded(-8.f).GetFromBottom(
        r.H() - 36.f); // Significant space for bold header
  };

  // Proportional row division (Top is taller for Envelope)
  IRECT topRow = controlsArea.GetFromTop(controlsArea.H() * 0.58f);
  IRECT bottomRow = controlsArea.GetFromBottom(controlsArea.H() * 0.40f);
  float w = controlsArea.W();

  IRECT oscArea = topRow.GetFromLeft(w * 0.24f);
  IRECT filterArea =
      IRECT(oscArea.R, topRow.T, oscArea.R + w * 0.24f, topRow.B);
  IRECT envArea =
      IRECT(filterArea.R, topRow.T, filterArea.R + w * 0.32f, topRow.B);
  IRECT lfoArea = IRECT(envArea.R, topRow.T, topRow.R, topRow.B);

  IRECT polyModArea = bottomRow.GetFromLeft(w * 0.24f);
  IRECT fxArea =
      IRECT(polyModArea.R, bottomRow.T, polyModArea.R + w * 0.24f, bottomRow.B);
  IRECT masterArea =
      IRECT(fxArea.R, bottomRow.T, fxArea.R + w * 0.20f, bottomRow.B);
  IRECT demoArea = IRECT(masterArea.R, bottomRow.T, bottomRow.R, bottomRow.B);

  const IRECT oscInner = attachFrame(oscArea, "OSCILLATORS");
  AttachStackedControl(pGraphics, oscInner.GetGridCell(0, 0, 2, 2),
                       kParamOscWave, "WAVE A", synthStyle);
  AttachStackedControl(pGraphics, oscInner.GetGridCell(0, 1, 2, 2),
                       kParamOscBWave, "WAVE B", synthStyle);
  AttachStackedControl(pGraphics, oscInner.GetGridCell(1, 0, 2, 2),
                       kParamOscPulseWidthA, "PULSE A", synthStyle);
  AttachStackedControl(pGraphics, oscInner.GetGridCell(1, 1, 2, 2),
                       kParamOscPulseWidthB, "PULSE B", synthStyle);

  const IRECT filterInner = attachFrame(filterArea, "FILTER");
  AttachStackedControl(pGraphics, filterInner.GetGridCell(0, 0, 2, 2),
                       kParamFilterCutoff, "CUTOFF", synthStyle);
  AttachStackedControl(pGraphics, filterInner.GetGridCell(0, 1, 2, 2),
                       kParamFilterResonance, "RESO", synthStyle);
  AttachStackedControl(pGraphics, filterInner.GetGridCell(1, 0, 2, 2),
                       kParamFilterEnvAmount, "ENV AMT", synthStyle);
  AttachStackedControl(pGraphics, filterInner.GetGridCell(1, 1, 2, 2),
                       kParamFilterModel, "MODEL", synthStyle);

  const IRECT envInner = attachFrame(envArea, "AMP ENVELOPE");
  // Visualization shifted down significantly (12px translation)
  const IRECT envVisualArea = envInner.GetFromTop(envInner.H() * 0.5f)
                                  .GetTranslated(0, 12.f)
                                  .GetPadded(-4.f);
  const IRECT envSliderArea = envInner.GetFromBottom(envInner.H() * 0.45f);
  Envelope *pEnvelope = new Envelope(envVisualArea, synthStyle);
  pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                     GetParam(kParamDecay)->Value() / 1000.f,
                     GetParam(kParamSustain)->Value() / 100.f,
                     GetParam(kParamRelease)->Value() / 1000.f);
  pEnvelope->SetColors(accentCyan, accentCyan.WithOpacity(0.15f));
  pGraphics->AttachControl(pEnvelope, kCtrlTagEnvelope);
  AttachStackedControl(pGraphics, envSliderArea.GetGridCell(0, 0, 1, 4),
                       kParamAttack, "A", synthStyle, true);
  AttachStackedControl(pGraphics, envSliderArea.GetGridCell(0, 1, 1, 4),
                       kParamDecay, "D", synthStyle, true);
  AttachStackedControl(pGraphics, envSliderArea.GetGridCell(0, 2, 1, 4),
                       kParamSustain, "S", synthStyle, true);
  AttachStackedControl(pGraphics, envSliderArea.GetGridCell(0, 3, 1, 4),
                       kParamRelease, "R", synthStyle, true);

  const IRECT lfoInner = attachFrame(lfoArea, "LFO");
  AttachStackedControl(pGraphics, lfoInner.GetGridCell(0, 0, 2, 2),
                       kParamLFOShape, "SHAPE", synthStyle);
  AttachStackedControl(pGraphics, lfoInner.GetGridCell(0, 1, 2, 2),
                       kParamLFORateHz, "RATE", synthStyle);
  AttachStackedControl(pGraphics, lfoInner.GetGridCell(1, 0, 2, 2),
                       kParamLFODepth, "DEPTH", synthStyle);
  AttachStackedControl(pGraphics, lfoInner.GetGridCell(1, 1, 2, 2),
                       kParamOscMix, "MIX", synthStyle);

  const IRECT polyInner = attachFrame(polyModArea, "POLY MOD");
  AttachStackedControl(pGraphics, polyInner.GetGridCell(0, 0, 2, 3),
                       kParamPolyModOscBToFreqA, "B-FREQ", synthStyle);
  AttachStackedControl(pGraphics, polyInner.GetGridCell(0, 1, 2, 3),
                       kParamPolyModOscBToPWM, "B-PWM", synthStyle);
  AttachStackedControl(pGraphics, polyInner.GetGridCell(0, 2, 2, 3),
                       kParamPolyModOscBToFilter, "B-FILT", synthStyle);
  AttachStackedControl(pGraphics, polyInner.GetGridCell(1, 0, 2, 3),
                       kParamPolyModFilterEnvToFreqA, "E-FREQ", synthStyle);
  AttachStackedControl(pGraphics, polyInner.GetGridCell(1, 1, 2, 3),
                       kParamPolyModFilterEnvToPWM, "E-PWM", synthStyle);
  AttachStackedControl(pGraphics, polyInner.GetGridCell(1, 2, 2, 3),
                       kParamPolyModFilterEnvToFilter, "E-FILT", synthStyle);

  const IRECT fxInner = attachFrame(fxArea, "FX");
  AttachStackedControl(pGraphics, fxInner.GetGridCell(0, 0, 2, 3),
                       kParamChorusRate, "RT", synthStyle);
  AttachStackedControl(pGraphics, fxInner.GetGridCell(0, 1, 2, 3),
                       kParamChorusDepth, "DP", synthStyle);
  AttachStackedControl(pGraphics, fxInner.GetGridCell(0, 2, 2, 3),
                       kParamChorusMix, "MX", synthStyle);
  AttachStackedControl(pGraphics, fxInner.GetGridCell(1, 0, 2, 3),
                       kParamDelayTime, "TM", synthStyle);
  AttachStackedControl(pGraphics, fxInner.GetGridCell(1, 1, 2, 3),
                       kParamDelayFeedback, "FB", synthStyle);
  AttachStackedControl(pGraphics, fxInner.GetGridCell(1, 2, 2, 3),
                       kParamDelayMix, "MX", synthStyle);

  // Master Section Fix: Divided into two sub-rows to prevent crowding
  const IRECT masterInner = attachFrame(masterArea, "MASTER");
  IRECT masterTop = masterInner.GetFromTop(masterInner.H() * 0.5f);
  IRECT masterBottom = masterInner.GetFromBottom(masterInner.H() * 0.5f);
  AttachStackedControl(pGraphics, masterTop.GetGridCell(0, 0, 1, 2), kParamGain,
                       "GAIN", synthStyle);
  AttachStackedControl(pGraphics, masterTop.GetGridCell(0, 1, 1, 2),
                       kParamLimiterThreshold, "LMT", synthStyle);
  AttachStackedControl(pGraphics, masterBottom.GetGridCell(0, 0, 1, 2),
                       kParamNoteGlideTime, "GLIDE", synthStyle);

  const IRECT demoInner = attachFrame(demoArea, "DEMO");
  IVStyle demoStyle = synthStyle.WithRoundness(0.08f);

  pGraphics->AttachControl(
      new IVSwitchControl(demoInner.GetGridCell(0, 0, 3, 1).GetPadded(-4.f),
                          kParamDemoMono, "MONO SEQ", demoStyle));
  pGraphics->AttachControl(
      new IVSwitchControl(demoInner.GetGridCell(1, 0, 3, 1).GetPadded(-4.f),
                          kParamDemoPoly, "POLY SEQ", demoStyle));
  pGraphics->AttachControl(
      new IVSwitchControl(demoInner.GetGridCell(2, 0, 3, 1).GetPadded(-4.f),
                          kParamDemoFX, "FX SHOW", demoStyle));

  pGraphics->AttachControl(new SectionFrame(footerArea, "OUTPUT", groupBorder,
                                            textDark, IColor(255, 10, 10, 10)));
  pGraphics->AttachControl(new ITextControl(
      footerArea.GetPadded(-10.f), "Ready",
      IText(14.f, accentCyan, "Roboto-Regular", EAlign::Near)));
}
#endif

#if IPLUG_DSP
void PolySynthPlugin::ProcessBlock(sample **inputs, sample **outputs,
                                   int nFrames) {
  static int blockCounter = 0;
  if (blockCounter++ % 100 == 0) {
    // Log every 100 blocks (~2 seconds)
    printf("[DSP] ProcessBlock: Mode=%d, SampleRate=%.1f\n",
           (int)mDemoSequencer.GetMode(), GetSampleRate());
  }

  mDemoSequencer.Process(nFrames, GetSampleRate(), mDSP);
  mDSP.UpdateState(mState);
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}
void PolySynthPlugin::OnIdle() {}
void PolySynthPlugin::OnReset() {
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mDSP.UpdateState(mState);
}
void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  mDSP.ProcessMidiMsg(msg);
}

void PolySynthPlugin::OnParamChange(int paramIdx) {
  if (mIsUpdatingUI)
    return;
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
    mState.fxLimiterThreshold = 1.0 - (value / kToPercentage);
    break;
  case kParamPresetSelect:
    mIsDirty = false;
    OnMessage(kMsgTagLoadPreset, (int)value, 0, nullptr);
    break;

  case kParamDemoMono:
    if (value > 0.5) {
      if (mDemoSequencer.GetMode() != DemoSequencer::Mode::Mono) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::Mono, GetSampleRate(),
                               mDSP);
        mIsUpdatingUI = true;
        GetParam(kParamDemoPoly)->Set(0.0);
        GetParam(kParamDemoFX)->Set(0.0);
        SendParameterValueFromDelegate(kParamDemoPoly, 0.0, true);
        SendParameterValueFromDelegate(kParamDemoFX, 0.0, true);
        mIsUpdatingUI = false;
      }
    } else {
      if (mDemoSequencer.GetMode() == DemoSequencer::Mode::Mono) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
      }
    }
    break;

  case kParamDemoPoly:
    if (value > 0.5) {
      if (mDemoSequencer.GetMode() != DemoSequencer::Mode::Poly) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::Poly, GetSampleRate(),
                               mDSP);
        mIsUpdatingUI = true;
        GetParam(kParamDemoMono)->Set(0.0);
        GetParam(kParamDemoFX)->Set(0.0);
        SendParameterValueFromDelegate(kParamDemoMono, 0.0, true);
        SendParameterValueFromDelegate(kParamDemoFX, 0.0, true);
        mIsUpdatingUI = false;
      }
    } else {
      if (mDemoSequencer.GetMode() == DemoSequencer::Mode::Poly) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
      }
    }
    break;

  case kParamDemoFX:
    if (value > 0.5) {
      if (mDemoSequencer.GetMode() != DemoSequencer::Mode::FX) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::FX, GetSampleRate(), mDSP);
        mIsUpdatingUI = true;
        GetParam(kParamDemoMono)->Set(0.0);
        GetParam(kParamDemoPoly)->Set(0.0);
        SendParameterValueFromDelegate(kParamDemoMono, 0.0, true);
        SendParameterValueFromDelegate(kParamDemoPoly, 0.0, true);
        mIsUpdatingUI = false;

        mState.fxChorusMix = 0.35;
        mState.fxDelayMix = 0.35;
        SyncUIState();
      }
    } else {
      if (mDemoSequencer.GetMode() == DemoSequencer::Mode::FX) {
        mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
        mState.fxChorusMix = 0.0;
        mState.fxDelayMix = 0.0;
        SyncUIState();
      }
    }
    break;
  default:
    break;
  }

#if IPLUG_EDITOR
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
#endif
}

bool PolySynthPlugin::OnMessage(int msgTag, int ctrlTag, int dataSize,
                                const void *pData) {
  if (msgTag == kMsgTagTestLoaded) {
    if (std::getenv("POLYSYNTH_TEST_UI")) {
      printf("TEST_PASS: UI Loaded\n");
      exit(0);
    }
  } else if (msgTag == kMsgTagSavePreset) {
    WDL_String path;
    DesktopPath(path);
    path.AppendFormatted(512, "/PolySynth_Preset_%d.json", ctrlTag);
    bool success = PolySynthCore::PresetManager::SaveToFile(mState, path.Get());
    mIsDirty = !success;
    return true;
  } else if (msgTag == kMsgTagLoadPreset) {
    WDL_String path;
    DesktopPath(path);
    path.AppendFormatted(512, "/PolySynth_Preset_%d.json", ctrlTag);
    PolySynthCore::SynthState loaded;
    if (PolySynthCore::PresetManager::LoadFromFile(path.Get(), loaded)) {
      mState = loaded;
      SyncUIState();
      mIsDirty = false;
      return true;
    }
    return false;
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
