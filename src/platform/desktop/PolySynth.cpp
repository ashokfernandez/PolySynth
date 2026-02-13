#include "PolySynth.h"
#include "../../../UI/Controls/PolyTheme.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#if IPLUG_EDITOR
#include "Envelope.h"
#include "IControls.h"
#include "PolyKnob.h"
#include "PolyToggleButton.h"
#include "PolySection.h"
#endif
#include <algorithm>
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

// Stacked control helper: label -> control -> value.
void AttachStackedControl(IGraphics *pGraphics, IRECT bounds, int paramIdx,
                          const char *label, const IVStyle &style,
                          bool isSlider = false) {
  float knobSize = std::min(bounds.W(), bounds.H() * 0.7f) * 0.82f;
  if (isSlider)
    knobSize = std::min(bounds.W() * 0.75f, bounds.H() * 0.75f);

  IRECT controlRect = bounds.GetCentredInside(knobSize);
  if (isSlider)
    controlRect = bounds.GetCentredInside(bounds.W() * 0.7f, bounds.H() * 0.7f);

  const float labelH = PolyTheme::LabelH;
  const float valueH = PolyTheme::ValueH;

  IText labelTextBold =
      IText(PolyTheme::FontLabel, style.labelText.mFGColor, "Roboto-Bold", EAlign::Center);
  IRECT labelRect = IRECT(bounds.L, controlRect.T - labelH - 1.f, bounds.R,
                          controlRect.T - 1.f);

  IText valueTextBold =
      IText(PolyTheme::FontValue, style.valueText.mFGColor, "Roboto-Regular", EAlign::Center);
  IRECT valueRect = IRECT(bounds.L, controlRect.B + 1.f, bounds.R,
                          controlRect.B + valueH + 1.f);

  pGraphics->AttachControl(new ITextControl(labelRect, label, labelTextBold));
  if (isSlider) {
    pGraphics->AttachControl(
        new IVSliderControl(controlRect, paramIdx, "",
                            style.WithShowLabel(false).WithShowValue(false)));
  } else {
    auto *knob = new PolyKnob(controlRect, paramIdx, "");
    knob->WithShowLabel(false).WithShowValue(false);
    pGraphics->AttachControl(knob);
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
#endif

PolySynthPlugin::PolySynthPlugin(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
#if IPLUG_DSP
  mState.Reset(); // Single source of truth for synth defaults
#endif

  GetParam(kParamGain)
      ->InitDouble("Gain", mState.masterGain * kToPercentage, 0., 100., 1.25,
                   "%");
  GetParam(kParamNoteGlideTime)
      ->InitMilliseconds("Glide", mState.glideTime * kToMs, 0.0, 30.);
  GetParam(kParamAttack)
      ->InitDouble("Attack", mState.ampAttack * kToMs, 1., 1000., 0.1, "ms",
                   IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)
      ->InitDouble("Decay", mState.ampDecay * kToMs, 1., 1000., 0.1, "ms",
                   IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)
      ->InitDouble("Sustain", mState.ampSustain * kToPercentage, 0., 100., 1,
                   "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)
      ->InitDouble("Release", mState.ampRelease * kToMs, 2., 1000., 0.1, "ms",
                   IParam::kFlagsNone,
                   "ADSR");

  GetParam(kParamLFOShape)
      ->InitEnum("LFO", mState.lfoShape, {"Sin", "Tri", "Sqr", "Saw"});
  GetParam(kParamLFORateHz)->InitFrequency("Rate", mState.lfoRate, 0.01, 40.);
  GetParam(kParamLFODepth)->InitDouble("Dep", mState.lfoDepth * kToPercentage,
                                       0., 100., 1., "%");

  GetParam(kParamFilterCutoff)
      ->InitDouble("Cutoff", mState.filterCutoff, 20., 20000., 1., "Hz",
                   IParam::kFlagsNone,
                   "Filter", IParam::ShapeExp());
  GetParam(kParamFilterResonance)
      ->InitDouble("Reso", mState.filterResonance * kToPercentage, 0., 100.,
                   1., "%");
  GetParam(kParamFilterEnvAmount)
      ->InitDouble("Contour", mState.filterEnvAmount * kToPercentage, 0.,
                   100., 1., "%");
  GetParam(kParamFilterModel)
      ->InitEnum("Model", mState.filterModel, {"CL", "LD", "P12", "P24"});

  GetParam(kParamOscWave)
      ->InitEnum("OscA", mState.oscAWaveform, {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamOscBWave)
      ->InitEnum("OscB", mState.oscBWaveform, {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamOscMix)
      ->InitDouble("Osc Bal", mState.mixOscB * kToPercentage, 0., 100., 1.,
                   "%");
  GetParam(kParamOscPulseWidthA)
      ->InitDouble("PWA", mState.oscAPulseWidth * kToPercentage, 0., 100., 1.,
                   "%");
  GetParam(kParamOscPulseWidthB)
      ->InitDouble("PWB", mState.oscBPulseWidth * kToPercentage, 0., 100., 1.,
                   "%");

  GetParam(kParamPolyModOscBToFreqA)
      ->InitDouble("FM Depth", mState.polyModOscBToFreqA * kToPercentage, 0.,
                   100., 1., "%");
  GetParam(kParamPolyModOscBToPWM)
      ->InitDouble("PWM Mod", mState.polyModOscBToPWM * kToPercentage, 0.,
                   100., 1., "%");
  GetParam(kParamPolyModOscBToFilter)
      ->InitDouble("B-V", mState.polyModOscBToFilter * kToPercentage, 0.,
                   100., 1., "%");
  GetParam(kParamPolyModFilterEnvToFreqA)
      ->InitDouble("Env FM", mState.polyModFilterEnvToFreqA * kToPercentage,
                   0., 100., 1., "%");
  GetParam(kParamPolyModFilterEnvToPWM)
      ->InitDouble("Env PWM", mState.polyModFilterEnvToPWM * kToPercentage,
                   0., 100., 1., "%");
  GetParam(kParamPolyModFilterEnvToFilter)
      ->InitDouble("Env VCF", mState.polyModFilterEnvToFilter * kToPercentage,
                   0., 100., 1., "%");

  GetParam(kParamChorusRate)->InitFrequency("Rate", mState.fxChorusRate, 0.05,
                                            2.0);
  GetParam(kParamChorusDepth)
      ->InitDouble("Dep", mState.fxChorusDepth * kToPercentage, 0., 100., 1.,
                   "%");
  GetParam(kParamChorusMix)
      ->InitDouble("Mix", mState.fxChorusMix * kToPercentage, 0., 100., 1.,
                   "%");
  GetParam(kParamDelayTime)
      ->InitMilliseconds("Time", mState.fxDelayTime * kToMs, 50., 1200.);
  GetParam(kParamDelayFeedback)
      ->InitDouble("Fbk", mState.fxDelayFeedback * kToPercentage, 0., 95., 1.,
                   "%");
  GetParam(kParamDelayMix)
      ->InitDouble("Mix", mState.fxDelayMix * kToPercentage, 0., 100., 1.,
                   "%");
  GetParam(kParamLimiterThreshold)
      ->InitDouble("Lmt", (1.0 - mState.fxLimiterThreshold) * kToPercentage,
                   0., 100., 1., "%");

  GetParam(kParamPresetSelect)->InitEnum("Patch", 0,
      {"Init", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5",
       "Slot 6", "Slot 7", "Slot 8", "Slot 9", "Slot 10",
       "Slot 11", "Slot 12", "Slot 13", "Slot 14", "Slot 15"});
  GetParam(kParamDemoMono)->InitBool("MONO", false);
  GetParam(kParamDemoPoly)->InitBool("POLY", false);
  GetParam(kParamDemoFX)->InitBool("FX", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  mLayoutFunc = [this](IGraphics *pGraphics) { OnLayout(pGraphics); };
#endif

#if IPLUG_DSP
  mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate(), mDSP);
  SyncUIState();
#endif
}

#if IPLUG_EDITOR
void PolySynthPlugin::OnUIOpen() {
  PopulatePresetMenu();
#if IPLUG_DSP
  SyncUIState();
#endif
}

void PolySynthPlugin::PopulatePresetMenu() {
#if !defined(WEB_API)
  WDL_String basePath;
  DesktopPath(basePath);
  for (int i = 1; i < 16; i++) {
    WDL_String filePath;
    filePath.SetFormatted(512, "%s/PolySynth_Preset_%d.json", basePath.Get(), i);
    FILE *f = fopen(filePath.Get(), "r");
    if (f) {
      fclose(f);
      WDL_String label;
      label.SetFormatted(64, "* Slot %d", i);
      GetParam(kParamPresetSelect)->SetDisplayText(i, label.Get());
    }
  }
#endif
}

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
  pGraphics->AttachPanelBackground(PolyTheme::PanelBG);

  const IColor textDark = PolyTheme::TextDark;
  const IVStyle synthStyle =
      DEFAULT_STYLE.WithRoundness(0.04f)
          .WithShadowOffset(2.0f)
          .WithShowValue(false)
          .WithLabelText(IText(PolyTheme::FontStyleLabel, textDark, "Roboto-Regular", EAlign::Center))
          .WithValueText(IText(PolyTheme::FontStyleValue, textDark, "Roboto-Regular", EAlign::Center))
          .WithColor(kBG, PolyTheme::ControlBG)
          .WithColor(kFG, PolyTheme::ControlFace)
          .WithColor(kPR, PolyTheme::AccentRed)
          .WithColor(kHL, PolyTheme::AccentCyan)
          .WithColor(kFR, PolyTheme::SectionBorder);

  IRECT bounds = pGraphics->GetBounds().GetPadded(-PolyTheme::Padding);
  const IRECT header = bounds.GetFromTop(72.f);
  const IRECT main = bounds.GetReducedFromTop(76.f);
  float w = main.W();

  IRECT topRow = main.GetFromTop(main.H() * 0.55f);
  IRECT bottomRow = main.GetFromBottom(main.H() * 0.42f);

  BuildHeader(pGraphics, header, synthStyle);
  BuildOscillators(pGraphics, topRow.GetFromLeft(w * 0.23f), synthStyle);
  IRECT filterArea(topRow.L + w * 0.23f, topRow.T, topRow.L + w * 0.48f, topRow.B);
  BuildFilter(pGraphics, filterArea, synthStyle);
  BuildEnvelope(pGraphics, IRECT(filterArea.R, topRow.T, topRow.R, topRow.B), synthStyle);
  BuildLFO(pGraphics, bottomRow.GetFromLeft(w * 0.15f), synthStyle);
  IRECT polyModArea(bottomRow.L + w * 0.15f, bottomRow.T, bottomRow.L + w * 0.35f, bottomRow.B);
  BuildPolyMod(pGraphics, polyModArea, synthStyle);
  IRECT chorusArea(polyModArea.R, bottomRow.T, polyModArea.R + w * 0.20f, bottomRow.B);
  BuildChorus(pGraphics, chorusArea, synthStyle);
  IRECT delayArea(chorusArea.R, bottomRow.T, chorusArea.R + w * 0.20f, bottomRow.B);
  BuildDelay(pGraphics, delayArea, synthStyle);
  BuildMaster(pGraphics, IRECT(delayArea.R, bottomRow.T, bottomRow.R, bottomRow.B), synthStyle);
}

void PolySynthPlugin::BuildHeader(IGraphics *g, const IRECT &bounds,
                                   const IVStyle &style) {
  const IColor textDark = PolyTheme::TextDark;
  g->AttachControl(new SectionFrame(bounds, "", PolyTheme::SectionBorder,
                                    textDark, PolyTheme::HeaderBG));
  g->AttachControl(new ITextControl(bounds.GetPadded(-18.f).GetFromTop(40.f),
                                    "PolySynth",
                                    IText(PolyTheme::FontTitle, textDark, "Roboto-Bold", EAlign::Near)));

  const IRECT presetArea =
      bounds.GetFromRight(280.f).GetCentredInside(260.f, 44.f).GetTranslated(-10.f, 0.f);
  g->AttachControl(new IVMenuButtonControl(presetArea.GetFromLeft(160.f),
                                           kParamPresetSelect, "Select Patch", style),
                   kCtrlTagPresetSelect);

  IVStyle saveStyle = style.WithColor(kBG, PolyTheme::AccentRed).WithColor(kFG, COLOR_WHITE);
  saveStyle.labelText.WithFont("Roboto-Bold").WithSize(18.f);
  g->AttachControl(
      new IVButtonControl(
          presetArea.GetFromRight(70.f),
          [&](IControl *pCaller) {
            mIsDirty = false;
            OnMessage(kMsgTagSavePreset,
                      (int)GetParam(kParamPresetSelect)->Value(), 0, nullptr);
            pCaller->SetDirty(false);
          },
          "SAVE", saveStyle),
      kCtrlTagSaveBtn);

  const IRECT demoArea = bounds.GetFromRight(bounds.W() * 0.45f)
                             .GetFromLeft(180.f)
                             .GetCentredInside(180.f, 30.f)
                             .GetTranslated(-20.f, 0.f);
  g->AttachControl(new PolyToggleButton(
      demoArea.GetGridCell(0, 0, 1, 3).GetPadded(-2.f), kParamDemoMono, "MONO"));
  g->AttachControl(new PolyToggleButton(
      demoArea.GetGridCell(0, 1, 1, 3).GetPadded(-2.f), kParamDemoPoly, "POLY"));
  g->AttachControl(new PolyToggleButton(
      demoArea.GetGridCell(0, 2, 1, 3).GetPadded(-2.f), kParamDemoFX, "FX"));
}

void PolySynthPlugin::BuildOscillators(IGraphics *g, const IRECT &bounds,
                                        const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "OSCILLATORS"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 2), kParamOscWave, "WAVE A", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 2), kParamOscBWave, "WAVE B", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 2), kParamOscPulseWidthA, "PULSE A", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 2), kParamOscPulseWidthB, "PULSE B", style);
}

void PolySynthPlugin::BuildFilter(IGraphics *g, const IRECT &bounds,
                                   const IVStyle &style) {
  const IColor textDark = PolyTheme::TextDark;
  g->AttachControl(new PolySection(bounds, "FILTER"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);

  IRECT modelArea = inner.GetFromBottom(40.f).GetPadded(-4.f);
  const IVStyle tabStyle = style
      .WithColor(kFG, PolyTheme::TabInactiveBG)
      .WithColor(kPR, PolyTheme::AccentRed)
      .WithValueText(IText(PolyTheme::FontTabSwitch, textDark, "Roboto-Bold"));
  g->AttachControl(new IVTabSwitchControl(
      modelArea, kParamFilterModel, {"LP", "BP", "HP", "NT"}, "", tabStyle));

  IRECT topFilter = inner.GetFromTop(inner.H() - 50.f);
  IRECT cutoffArea = topFilter.GetFromTop(topFilter.H() * 0.65f).GetCentredInside(90.f);
  auto *cutoffKnob = new PolyKnob(cutoffArea, kParamFilterCutoff, "Cutoff");
  cutoffKnob->WithShowValue(false);
  g->AttachControl(cutoffKnob);

  IRECT cutoffValueArea(cutoffArea.L, cutoffArea.B + 2.f, cutoffArea.R, cutoffArea.B + 18.f);
  g->AttachControl(new ICaptionControl(cutoffValueArea, kParamFilterCutoff,
                                       IText(PolyTheme::FontValue, textDark, "Roboto-Bold", EAlign::Center), false));

  IRECT secondaryArea(inner.L, cutoffValueArea.B, inner.R, modelArea.T);
  AttachStackedControl(g, secondaryArea.GetGridCell(0, 0, 1, 2), kParamFilterResonance, "RESO", style);
  AttachStackedControl(g, secondaryArea.GetGridCell(0, 1, 1, 2), kParamFilterEnvAmount, "CONTOUR", style);
}

void PolySynthPlugin::BuildEnvelope(IGraphics *g, const IRECT &bounds,
                                     const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "AMP ENVELOPE"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  const IRECT vizArea = inner.GetFromTop(inner.H() * 0.5f).GetPadded(-8.f).GetTranslated(0, 6.f);
  const IRECT sliderArea = inner.GetFromBottom(inner.H() * 0.45f);

  Envelope *pEnvelope = new Envelope(vizArea, style);
  pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                     GetParam(kParamDecay)->Value() / 1000.f,
                     GetParam(kParamSustain)->Value() / 100.f,
                     GetParam(kParamRelease)->Value() / 1000.f);
  pEnvelope->SetColors(PolyTheme::AccentCyan, PolyTheme::AccentCyan.WithOpacity(0.15f));
  g->AttachControl(pEnvelope, kCtrlTagEnvelope);

  AttachStackedControl(g, sliderArea.GetGridCell(0, 0, 1, 4), kParamAttack, "A", style, true);
  AttachStackedControl(g, sliderArea.GetGridCell(0, 1, 1, 4), kParamDecay, "D", style, true);
  AttachStackedControl(g, sliderArea.GetGridCell(0, 2, 1, 4), kParamSustain, "S", style, true);
  AttachStackedControl(g, sliderArea.GetGridCell(0, 3, 1, 4), kParamRelease, "R", style, true);
}

void PolySynthPlugin::BuildLFO(IGraphics *g, const IRECT &bounds,
                                const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "LFO"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 2), kParamLFOShape, "SHAPE", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 2), kParamLFORateHz, "RATE", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 2), kParamLFODepth, "DEPTH", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 2), kParamOscMix, "MIX", style);
}

void PolySynthPlugin::BuildPolyMod(IGraphics *g, const IRECT &bounds,
                                    const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "POLY MOD"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 3), kParamPolyModOscBToFreqA, "B-FREQ", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 3), kParamPolyModOscBToPWM, "B-PWM", style);
  AttachStackedControl(g, inner.GetGridCell(0, 2, 2, 3), kParamPolyModOscBToFilter, "B-FILT", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 3), kParamPolyModFilterEnvToFreqA, "E-FREQ", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 3), kParamPolyModFilterEnvToPWM, "E-PWM", style);
  AttachStackedControl(g, inner.GetGridCell(1, 2, 2, 3), kParamPolyModFilterEnvToFilter, "E-FILT", style);
}

void PolySynthPlugin::BuildChorus(IGraphics *g, const IRECT &bounds,
                                   const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "CHORUS"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  auto pad = [](IRECT r, int row, int col, int nr, int nc) {
    return r.GetGridCell(row, col, nr, nc).GetPadded(-2.f);
  };
  AttachStackedControl(g, pad(inner, 0, 0, 1, 3), kParamChorusRate, "RATE", style);
  AttachStackedControl(g, pad(inner, 0, 1, 1, 3), kParamChorusDepth, "DEPTH", style);
  AttachStackedControl(g, pad(inner, 0, 2, 1, 3), kParamChorusMix, "MIX", style);
}

void PolySynthPlugin::BuildDelay(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "DELAY"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  auto pad = [](IRECT r, int row, int col, int nr, int nc) {
    return r.GetGridCell(row, col, nr, nc).GetPadded(-2.f);
  };
  AttachStackedControl(g, pad(inner, 0, 0, 1, 3), kParamDelayTime, "TIME", style);
  AttachStackedControl(g, pad(inner, 0, 1, 1, 3), kParamDelayFeedback, "FDBK", style);
  AttachStackedControl(g, pad(inner, 0, 2, 1, 3), kParamDelayMix, "MIX", style);
}

void PolySynthPlugin::BuildMaster(IGraphics *g, const IRECT &bounds,
                                   const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "MASTER"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding).GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 1), kParamGain, "GAIN", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 1), kParamLimiterThreshold, "LIMIT", style);
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
  }
#if IPLUG_EDITOR && !defined(WEB_API)
  else if (msgTag == kMsgTagSavePreset) {
    WDL_String path;
    DesktopPath(path);
    path.AppendFormatted(512, "/PolySynth_Preset_%d.json", ctrlTag);
    bool success = PolySynthCore::PresetManager::SaveToFile(mState, path.Get());
    mIsDirty = !success;
    if (success)
      PopulatePresetMenu();
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
  }
#endif
  else if (msgTag == kMsgTagNoteOn) {
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
