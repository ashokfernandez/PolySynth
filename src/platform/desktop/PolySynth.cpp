#include "PolySynth.h"

using namespace iplug;
using namespace igraphics;
#include "../../../UI/Controls/PolyTheme.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#if IPLUG_EDITOR
#include "Envelope.h"
#include "IControls.h"
#include "LCDPanel.h"
#include "PolyKnob.h"
#include "PolySection.h"
#include "PolyToggleButton.h"
#include "PresetSaveButton.h"
#include "SectionFrame.h"
#endif
#include "ParamMeta.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sea_util/sea_theory_engine.h>
#include <vector>

namespace {
static const double kToMs = 1000.0;

#if IPLUG_EDITOR
// LCDPanel and SectionFrame now in separate headers

// Stacked control helper: label -> control -> value.
void AttachStackedControl(IGraphics *pGraphics, IRECT bounds, int paramIdx,
                          const char *label, const IVStyle &style,
                          bool isSlider = false) {
  float knobSize = std::min(bounds.W(), bounds.H() * 0.7f) * 0.82f;
  if (isSlider)
    knobSize = std::min(bounds.W() * 0.75f, bounds.H() * 0.9f);

  IRECT controlRect = bounds.GetCentredInside(knobSize);
  if (isSlider)
    controlRect = bounds.GetCentredInside(bounds.W() * 0.7f, bounds.H() * 0.9f);

  const float labelH = PolyTheme::LabelH;
  const float valueH = PolyTheme::ValueH;

  IText labelTextBold = IText(PolyTheme::FontLabel, style.labelText.mFGColor,
                              "Bold", EAlign::Center);
  IRECT labelRect = IRECT(bounds.L, controlRect.T - labelH - 1.f, bounds.R,
                          controlRect.T - 1.f);

  IText valueTextBold = IText(PolyTheme::FontValue, style.valueText.mFGColor,
                              "Regular", EAlign::Center);
  IRECT valueRect = IRECT(bounds.L, controlRect.B + 1.f, bounds.R,
                          controlRect.B + valueH + 1.f);

  pGraphics->AttachControl(new ITextControl(labelRect, label, labelTextBold));
  if (isSlider) {
    pGraphics->AttachControl(new IVSliderControl(
        controlRect, paramIdx, "",
        style.WithShowLabel(false).WithShowValue(false).WithWidgetFrac(0.3f)));
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

  // Table-driven sync
  for (int i = 0; i < kParamTableSize; ++i) {
    const auto &m = kParamTable[i];
    GetParam(m.paramId)->Set(StateToUIValue(m, mState));
  }

  // Special cases: enums, frequency, and milliseconds params
  GetParam(kParamLFOShape)->Set(static_cast<double>(mState.lfoShape));
  GetParam(kParamLFORateHz)->Set(mState.lfoRate);
  GetParam(kParamOscWave)->Set(static_cast<double>(mState.oscAWaveform));
  GetParam(kParamOscBWave)->Set(static_cast<double>(mState.oscBWaveform));
  GetParam(kParamFilterModel)->Set(static_cast<double>(mState.filterModel));
  GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
  GetParam(kParamDelayTime)->Set(mState.fxDelayTime * kToMs);
  GetParam(kParamAllocationMode)
      ->Set(static_cast<double>(mState.allocationMode));
  GetParam(kParamStealPriority)
      ->Set(static_cast<double>(mState.stealPriority));

  // Demo buttons
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
  mStateQueue.TryPush(mState);
}
#endif

PolySynthPlugin::PolySynthPlugin(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  mState.Reset();
  PolySynthCore::SynthState &state = mState;

  // Table-driven parameter registration
  for (int i = 0; i < kParamTableSize; ++i) {
    const auto &m = kParamTable[i];
    switch (m.initKind) {
    case ParamMeta::InitKind::kInitDouble:
      switch (m.shape) {
      case ParamMeta::ShapeKind::kExp:
        GetParam(m.paramId)
            ->InitDouble(m.name, m.defaultVal, m.min, m.max, m.step,
                         m.unit ? m.unit : "", IParam::kFlagsNone,
                         m.group ? m.group : "", IParam::ShapeExp());
        break;
      case ParamMeta::ShapeKind::kPow3:
        GetParam(m.paramId)
            ->InitDouble(m.name, m.defaultVal, m.min, m.max, m.step,
                         m.unit ? m.unit : "", IParam::kFlagsNone,
                         m.group ? m.group : "", IParam::ShapePowCurve(3.));
        break;
      case ParamMeta::ShapeKind::kLinear:
        if (m.group) {
          GetParam(m.paramId)
              ->InitDouble(m.name, m.defaultVal, m.min, m.max, m.step,
                           m.unit ? m.unit : "", IParam::kFlagsNone, m.group);
        } else {
          GetParam(m.paramId)
              ->InitDouble(m.name, m.defaultVal, m.min, m.max, m.step,
                           m.unit ? m.unit : "");
        }
        break;
      }
      break;
    case ParamMeta::InitKind::kInitInt:
      GetParam(m.paramId)
          ->InitInt(m.name, static_cast<int>(m.defaultVal),
                    static_cast<int>(m.min), static_cast<int>(m.max));
      break;
    case ParamMeta::InitKind::kInitMilliseconds:
      GetParam(m.paramId)
          ->InitMilliseconds(m.name, m.defaultVal, m.min, m.max);
      break;
    }
  }

  // Special cases: enum, frequency, and bool params
  GetParam(kParamLFOShape)
      ->InitEnum("LFO", state.lfoShape, {"Sin", "Tri", "Sqr", "Saw"});
  GetParam(kParamLFORateHz)->InitFrequency("Rate", state.lfoRate, 0.01, 40.);
  GetParam(kParamOscWave)
      ->InitEnum("OscA", state.oscAWaveform, {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamOscBWave)
      ->InitEnum("OscB", state.oscBWaveform, {"SAW", "SQR", "TRI", "SIN"});
  GetParam(kParamFilterModel)
      ->InitEnum("Model", state.filterModel, {"CL", "LD", "P12", "P24"});
  GetParam(kParamChorusRate)
      ->InitFrequency("Rate", state.fxChorusRate, 0.05, 2.0);
  GetParam(kParamDelayTime)
      ->InitMilliseconds("Time", state.fxDelayTime * kToMs, 50., 1200.);
  GetParam(kParamAllocationMode)
      ->InitEnum("Alloc", state.allocationMode,
                 {"Oldest", "Lowest", "Highest"});
  GetParam(kParamStealPriority)
      ->InitEnum("Steal", state.stealPriority,
                 {"Oldest", "Lowest", "Quietest"});
  GetParam(kParamPresetSelect)
      ->InitEnum("Patch", 0,
                 {"Init", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5",
                  "Slot 6", "Slot 7", "Slot 8", "Slot 9", "Slot 10", "Slot 11",
                  "Slot 12", "Slot 13", "Slot 14", "Slot 15"});
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
  mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate());
  SyncUIState();
#endif
}

#if IPLUG_EDITOR
void PolySynthPlugin::OnUIOpen() {
  // Initialise voice-tracking tables so every slot starts free.
  std::fill(mNoteToUISlot, mNoteToUISlot + 128, -1);
  std::fill(mUISlotNote,    mUISlotNote    + kUIMaxVoices, -1);
  std::fill(mUISlotReleased, mUISlotReleased + kUIMaxVoices, false);
  mUIMaxVoices = GetParam(kParamPolyphonyLimit)->Int();

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
    filePath.SetFormatted(512, "%s/PolySynth_Preset_%d.json", basePath.Get(),
                          i);
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
      paramIdx == kParamSustain || paramIdx == kParamRelease ||
      paramIdx == kParamPolyphonyLimit) {
    if (GetUI()) {
      Envelope *pEnvelope = dynamic_cast<Envelope *>(
          GetUI()->GetControlWithTag(kCtrlTagEnvelope));
      if (pEnvelope) {
        pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                           GetParam(kParamDecay)->Value() / 1000.f,
                           GetParam(kParamSustain)->Value() / 100.f,
                           GetParam(kParamRelease)->Value() / 1000.f);
        if (paramIdx == kParamPolyphonyLimit)
          mUIMaxVoices = GetParam(kParamPolyphonyLimit)->Int();
      }
    }
  }
}

void PolySynthPlugin::OnLayout(IGraphics *pGraphics) {
  IGraphics *g = pGraphics;
  if (pGraphics->NControls()) {
    return;
  }

  bool regularLoaded = false;
  bool boldLoaded = false;
  bool robotoRegularLoaded = false;
  bool robotoBoldLoaded = false;

  WDL_String resourcePath;
#if defined(OS_MAC)
  // macOS: PluginIDType is const char*, try multiple bundle ID candidates.
  const char *bundleIDCandidates[] = {
      GetBundleID(), BUNDLE_ID, "com." PLUG_NAME ".app." BUNDLE_NAME};

  for (const char *bundleID : bundleIDCandidates) {
    if (!bundleID || !bundleID[0]) {
      continue;
    }

    WDL_String candidatePath;
    BundleResourcePath(candidatePath, bundleID);
    if (!candidatePath.GetLength()) {
      continue;
    }

    WDL_String regularFontPath(candidatePath);
    regularFontPath.Append("/");
    regularFontPath.Append(ROBOTO_FN);

    WDL_String boldFontPath(candidatePath);
    boldFontPath.Append("/");
    boldFontPath.Append(ROBOTO_BOLD_FN);

    regularLoaded = regularLoaded ||
                    pGraphics->LoadFont("Regular", regularFontPath.Get());
    boldLoaded = boldLoaded || pGraphics->LoadFont("Bold", boldFontPath.Get());
    robotoRegularLoaded =
        robotoRegularLoaded ||
        pGraphics->LoadFont("Roboto-Regular", regularFontPath.Get());
    robotoBoldLoaded = robotoBoldLoaded ||
                       pGraphics->LoadFont("Roboto-Bold", boldFontPath.Get());

    if (regularLoaded && boldLoaded && robotoRegularLoaded &&
        robotoBoldLoaded) {
      resourcePath.Set(candidatePath.Get());
      break;
    }
  }
#elif defined(OS_WIN)
  // Windows: PluginIDType is HMODULE; use the plugin DLL handle.
  {
    extern HINSTANCE gHINSTANCE;
    WDL_String candidatePath;
    BundleResourcePath(candidatePath, gHINSTANCE);
    if (candidatePath.GetLength()) {
      WDL_String regularFontPath(candidatePath);
      regularFontPath.Append("\\");
      regularFontPath.Append(ROBOTO_FN);

      WDL_String boldFontPath(candidatePath);
      boldFontPath.Append("\\");
      boldFontPath.Append(ROBOTO_BOLD_FN);

      regularLoaded = pGraphics->LoadFont("Regular", regularFontPath.Get());
      boldLoaded = pGraphics->LoadFont("Bold", boldFontPath.Get());
      robotoRegularLoaded =
          pGraphics->LoadFont("Roboto-Regular", regularFontPath.Get());
      robotoBoldLoaded =
          pGraphics->LoadFont("Roboto-Bold", boldFontPath.Get());
      if (regularLoaded && boldLoaded)
        resourcePath.Set(candidatePath.Get());
    }
  }
#endif // OS_MAC / OS_WIN

  // Fallback for edge cases where bundle resource discovery fails.
  if (!regularLoaded)
    regularLoaded = pGraphics->LoadFont("Regular", ROBOTO_FN);
  if (!boldLoaded)
    boldLoaded = pGraphics->LoadFont("Bold", ROBOTO_BOLD_FN);
  if (!robotoRegularLoaded)
    robotoRegularLoaded = pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
  if (!robotoBoldLoaded)
    robotoBoldLoaded = pGraphics->LoadFont("Roboto-Bold", ROBOTO_BOLD_FN);

  if (!regularLoaded || !boldLoaded || !robotoRegularLoaded ||
      !robotoBoldLoaded) {
    std::fprintf(stderr,
                 "[FontLoad] Failed! Regular=%d Bold=%d Roboto-Regular=%d "
                 "Roboto-Bold=%d path='%s'\n",
                 regularLoaded, boldLoaded, robotoRegularLoaded,
                 robotoBoldLoaded, resourcePath.Get());
  }

  pGraphics->AttachPanelBackground(PolyTheme::PanelBG);

  const IColor textDark = PolyTheme::TextDark;
  const IVStyle synthStyle =
      IVStyle()
          .WithShowLabel(true)
          .WithShowValue(false)
          .WithWidgetFrac(0.5f)
          .WithDrawFrame(false)
          .WithColor(kBG, PolyTheme::ControlBG)
          .WithColor(kFG, PolyTheme::ControlFace)
          //.WithColor(kPR, PolyTheme::AccentCyan) // Knob ring
          .WithColor(kFR, PolyTheme::SectionBorder)
          .WithLabelText(
              IText(PolyTheme::FontStyleLabel, PolyTheme::TextDark, "Bold"))
          .WithValueText(
              IText(PolyTheme::FontStyleValue, PolyTheme::TextDark, "Regular"));

  IRECT bounds = pGraphics->GetBounds().GetPadded(-PolyTheme::Padding);
  // Layout Constants
  const float headerHeight = 80.f;
  const float footerHeight = 60.f;

  IRECT headerArea = bounds.GetFromTop(headerHeight);
  IRECT footerArea = bounds.GetFromBottom(footerHeight);
  IRECT contentArea =
      bounds.GetReducedFromTop(headerHeight).GetReducedFromBottom(footerHeight);

  // Background
  // g->FillRect(PolyTheme::PanelBG, bounds); // Removed: Use
  // AttachPanelBackground

  // Build Sections
  BuildHeader(g, headerArea, synthStyle);
  BuildFooter(g, footerArea, synthStyle);

  // Content Layout
  // 3 Rows:
  // 1. Oscillators | Filter | Envelope
  // 2. LFO | Poly Mod | Chorus | Delay | Master

  const float topRowHeight = contentArea.H() * 0.65f;
  IRECT topRow =
      contentArea.GetFromTop(topRowHeight).GetPadded(-PolyTheme::Padding);
  IRECT bottomRow = contentArea.GetFromBottom(contentArea.H() - topRowHeight)
                        .GetPadded(-PolyTheme::Padding);

  // Top Row: Osc (30%), Filter (30%), Env (40%)
  BuildOscillators(g, topRow.GetFromLeft(topRow.W() * 0.30f).GetPadded(-4.f),
                   synthStyle);
  BuildFilter(g,
              topRow.GetFromLeft(topRow.W() * 0.30f)
                  .GetTranslated(topRow.W() * 0.30f, 0)
                  .GetPadded(-4.f),
              synthStyle);
  BuildEnvelope(g, topRow.GetFromRight(topRow.W() * 0.40f).GetPadded(-4.f),
                synthStyle);

  // Bottom Row
  // LFO(15%), PolyMod(20%), Chorus(20%), Delay(20%), Master(25%)
  float x = 0;
  float w = bottomRow.W();

  BuildLFO(g, bottomRow.GetFromLeft(w * 0.15f).GetPadded(-4.f), synthStyle);
  x += w * 0.15f;

  BuildPolyMod(
      g, bottomRow.GetFromLeft(w * 0.20f).GetTranslated(x, 0).GetPadded(-4.f),
      synthStyle);
  x += w * 0.20f;

  BuildChorus(
      g, bottomRow.GetFromLeft(w * 0.20f).GetTranslated(x, 0).GetPadded(-4.f),
      synthStyle);
  x += w * 0.20f;

  BuildDelay(
      g, bottomRow.GetFromLeft(w * 0.20f).GetTranslated(x, 0).GetPadded(-4.f),
      synthStyle);
  x += w * 0.20f;

  BuildMaster(
      g, bottomRow.GetFromLeft(w * 0.25f).GetTranslated(x, 0).GetPadded(-4.f),
      synthStyle);

  if (std::getenv("POLYSYNTH_TEST_UI")) {
    OnMessage(kMsgTagTestLoaded, 0, 0, nullptr);
  }
}

void PolySynthPlugin::BuildHeader(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  g->AttachControl(new SectionFrame(bounds, "", PolyTheme::SectionBorder,
                                    PolyTheme::TextDark, PolyTheme::HeaderBG));

  IRECT inner = bounds.GetPadded(-PolyTheme::Padding);

  // 1. Voice Controls (Left)
  // 1x6 Grid: POLY | UNI | ALLOC | STEAL | DETUNE | WIDTH
  IRECT voiceArea = inner.GetFromLeft(420.f).GetCentredInside(420.f, 60.f);

  AttachStackedControl(g, voiceArea.GetGridCell(0, 0, 1, 6),
                       kParamPolyphonyLimit, "POLY", style);
  AttachStackedControl(g, voiceArea.GetGridCell(0, 1, 1, 6), kParamUnisonCount,
                       "UNI", style);
  AttachStackedControl(g, voiceArea.GetGridCell(0, 2, 1, 6),
                       kParamAllocationMode, "ALLOC", style);
  AttachStackedControl(g, voiceArea.GetGridCell(0, 3, 1, 6),
                       kParamStealPriority, "STEAL", style);
  AttachStackedControl(g, voiceArea.GetGridCell(0, 4, 1, 6), kParamUnisonSpread,
                       "DETUNE", style);
  AttachStackedControl(g, voiceArea.GetGridCell(0, 5, 1, 6), kParamStereoSpread,
                       "WIDTH", style);

  // 2. LCD Display (Center)
  // Displays Active Voices and Chord Name
  IRECT lcdArea =
      inner.GetFromLeft(200.f).GetTranslated(440.f, 0).GetCentredInside(180.f,
                                                                        50.f);

  // LCD Background
  g->AttachControl(new LCDPanel(lcdArea, PolyTheme::LCDBackground));

  // Active Voices (Top Left of LCD)
  IRECT voicesText = lcdArea.GetPadded(-4.f).GetFromTop(16.f);
  g->AttachControl(
      new ITextControl(voicesText, "0/0",
                       IText(14.f, PolyTheme::LCDText, "Bold", EAlign::Near)),
      kCtrlTagActiveVoices);

  // Chord Name (Bottom Center of LCD)
  IRECT chordText = lcdArea.GetPadded(-4.f).GetFromBottom(24.f);
  g->AttachControl(
      new ITextControl(chordText, "--",
                       IText(20.f, PolyTheme::LCDText, "Bold", EAlign::Center)),
      kCtrlTagChordName);

  // 3. Patch Management (Right)
  IRECT presetArea = inner.GetFromRight(260.f).GetCentredInside(260.f, 40.f);

  // Preset Selector (Left side of area)
  // Use a consistent height for both controls to ensure alignment
  float controlH = 24.f;

  // Preset Display
  // Preset Display
  IRECT dropdownRect =
      presetArea.GetFromLeft(180.f).GetCentredInside(180.f, controlH);
  g->AttachControl(
      new IPanelControl(dropdownRect, PolyTheme::ControlFace)); // Background
  // Actually, IPanelControl doesn't draw border by default. Let's use a lambda
  // or just a background. Better: specialized control or just Panel with
  // border.
  g->AttachControl(
      new ICaptionControl(dropdownRect, kParamPresetSelect,
                          IText(14.f, PolyTheme::TextDark, "Regular"), true));

  // Save Button
  g->AttachControl(
      new PresetSaveButton(
          presetArea.GetFromRight(60.f).GetCentredInside(60.f, controlH),
          [&](IControl *pCaller) {
            // Action only triggers if dirty (logic in control), but here we
            // handle the save
            OnMessage(kMsgTagSavePreset,
                      (int)GetParam(kParamPresetSelect)->Value(), 0, nullptr);
          }),
      kCtrlTagSaveBtn);
}

void PolySynthPlugin::BuildOscillators(IGraphics *g, const IRECT &bounds,
                                       const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "OSCILLATORS"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 2), kParamOscWave,
                       "WAVE A", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 2), kParamOscBWave,
                       "WAVE B", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 2), kParamOscPulseWidthA,
                       "PULSE A", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 2), kParamOscPulseWidthB,
                       "PULSE B", style);
}

void PolySynthPlugin::BuildFilter(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  const IColor textDark = PolyTheme::TextDark;
  g->AttachControl(new PolySection(bounds, "FILTER"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);

  IRECT modelArea = inner.GetFromBottom(60.f).GetPadded(-4.f);
  const IVStyle tabStyle = style.WithColor(kFG, PolyTheme::TabInactiveBG)
                               .WithColor(kPR, PolyTheme::AccentRed)
                               .WithValueText(IText(14.f, textDark, "Bold"));
  g->AttachControl(new IVTabSwitchControl(
      modelArea, kParamFilterModel, {"CL", "LD", "P12", "P24"}, "", tabStyle));

  IRECT topFilter = inner.GetFromTop(inner.H() - 70.f);
  IRECT cutoffArea =
      topFilter.GetFromTop(topFilter.H() * 0.55f).GetCentredInside(90.f);
  auto *cutoffKnob = new PolyKnob(cutoffArea, kParamFilterCutoff, "Cutoff");
  cutoffKnob->WithShowValue(false);
  g->AttachControl(cutoffKnob);

  IRECT cutoffValueArea(cutoffArea.L, cutoffArea.B + 2.f, cutoffArea.R,
                        cutoffArea.B + 18.f);
  g->AttachControl(new ICaptionControl(
      cutoffValueArea, kParamFilterCutoff,
      IText(PolyTheme::FontValue, textDark, "Bold", EAlign::Center), false));

  IRECT secondaryArea(inner.L, cutoffValueArea.B, inner.R, modelArea.T);
  AttachStackedControl(g, secondaryArea.GetGridCell(0, 0, 1, 2),
                       kParamFilterResonance, "RESO", style);
  AttachStackedControl(g, secondaryArea.GetGridCell(0, 1, 1, 2),
                       kParamFilterEnvAmount, "CONTOUR", style);
}

void PolySynthPlugin::BuildEnvelope(IGraphics *g, const IRECT &bounds,
                                    const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "AMP ENVELOPE"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  // Increase envelope visualizer height (40%)
  const IRECT vizArea =
      inner.GetFromTop(inner.H() * 0.40f).GetPadded(-8.f).GetTranslated(0, 6.f);

  // Decrease slider area slightly (60%) but optimize layout
  const IRECT sliderArea = inner.GetFromBottom(inner.H() * 0.60f);

  Envelope *pEnvelope = new Envelope(vizArea, style);
  pEnvelope->SetADSR(GetParam(kParamAttack)->Value() / 1000.f,
                     GetParam(kParamDecay)->Value() / 1000.f,
                     GetParam(kParamSustain)->Value() / 100.f,
                     GetParam(kParamRelease)->Value() / 1000.f);
  pEnvelope->SetColors(PolyTheme::AccentCyan,
                       PolyTheme::AccentCyan.WithOpacity(0.15f));
  g->AttachControl(pEnvelope, kCtrlTagEnvelope);

  // Manual ADSR Layout for maximum slider length
  const int nParams = 4;
  int params[4] = {kParamAttack, kParamDecay, kParamSustain, kParamRelease};
  const char *labels[4] = {"A", "D", "S", "R"};

  for (int i = 0; i < nParams; i++) {
    IRECT cell = sliderArea.GetGridCell(0, i, 1, nParams);

    // Calculate specific rects to maximize slider travel
    // Use tight heights from theme
    const float labelH = PolyTheme::LabelH; // 18.f
    const float valueH = PolyTheme::ValueH; // 16.f

    IRECT labelRect = cell.GetFromTop(labelH);
    IRECT valueRect = cell.GetFromBottom(valueH);

    // Slider fills the space between label and value
    // Changed from GetMidHPadded (which made it 2px wide) to GetHPadded(2.f)
    // for full width minus small padding
    IRECT sliderRect =
        cell.GetReducedFromTop(labelH).GetReducedFromBottom(valueH).GetHPadded(
            2.f);

    // Label
    g->AttachControl(
        new ITextControl(labelRect, labels[i],
                         IText(PolyTheme::FontLabel, style.labelText.mFGColor,
                               "Bold", EAlign::Center)));

    // Slider
    // Revert WidgetFrac to 0.5f (or default) for normal handle size
    g->AttachControl(new IVSliderControl(
        sliderRect, params[i], "",
        style.WithShowLabel(false).WithShowValue(false).WithWidgetFrac(0.5f)));

    // Value
    g->AttachControl(new ICaptionControl(valueRect, params[i],
                                         IText(PolyTheme::FontValue,
                                               style.valueText.mFGColor,
                                               "Regular", EAlign::Center),
                                         false));
  }
}

void PolySynthPlugin::BuildLFO(IGraphics *g, const IRECT &bounds,
                               const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "LFO"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 2), kParamLFOShape,
                       "SHAPE", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 2), kParamLFORateHz,
                       "RATE", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 2), kParamLFODepth,
                       "DEPTH", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 2), kParamOscMix, "MIX",
                       style);
}

void PolySynthPlugin::BuildPolyMod(IGraphics *g, const IRECT &bounds,
                                   const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "POLY MOD"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 3),
                       kParamPolyModOscBToFreqA, "B-FREQ", style);
  AttachStackedControl(g, inner.GetGridCell(0, 1, 2, 3), kParamPolyModOscBToPWM,
                       "B-PWM", style);
  AttachStackedControl(g, inner.GetGridCell(0, 2, 2, 3),
                       kParamPolyModOscBToFilter, "B-FILT", style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 3),
                       kParamPolyModFilterEnvToFreqA, "E-FREQ", style);
  AttachStackedControl(g, inner.GetGridCell(1, 1, 2, 3),
                       kParamPolyModFilterEnvToPWM, "E-PWM", style);
  AttachStackedControl(g, inner.GetGridCell(1, 2, 2, 3),
                       kParamPolyModFilterEnvToFilter, "E-FILT", style);
}

void PolySynthPlugin::BuildChorus(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "CHORUS"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  auto pad = [](IRECT r, int row, int col, int nr, int nc) {
    return r.GetGridCell(row, col, nr, nc).GetPadded(-2.f);
  };
  AttachStackedControl(g, pad(inner, 0, 0, 1, 3), kParamChorusRate, "RATE",
                       style);
  AttachStackedControl(g, pad(inner, 0, 1, 1, 3), kParamChorusDepth, "DEPTH",
                       style);
  AttachStackedControl(g, pad(inner, 0, 2, 1, 3), kParamChorusMix, "MIX",
                       style);
}

void PolySynthPlugin::BuildDelay(IGraphics *g, const IRECT &bounds,
                                 const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "DELAY"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  auto pad = [](IRECT r, int row, int col, int nr, int nc) {
    return r.GetGridCell(row, col, nr, nc).GetPadded(-2.f);
  };
  AttachStackedControl(g, pad(inner, 0, 0, 1, 3), kParamDelayTime, "TIME",
                       style);
  AttachStackedControl(g, pad(inner, 0, 1, 1, 3), kParamDelayFeedback, "FDBK",
                       style);
  AttachStackedControl(g, pad(inner, 0, 2, 1, 3), kParamDelayMix, "MIX", style);
}

void PolySynthPlugin::BuildMaster(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  g->AttachControl(new PolySection(bounds, "MASTER"));
  const IRECT inner = bounds.GetPadded(-PolyTheme::SectionPadding)
                          .GetFromBottom(bounds.H() - PolyTheme::SectionTitleH);
  AttachStackedControl(g, inner.GetGridCell(0, 0, 2, 1), kParamGain, "GAIN",
                       style);
  AttachStackedControl(g, inner.GetGridCell(1, 0, 2, 1), kParamLimiterThreshold,
                       "LIMIT", style);
}

void PolySynthPlugin::BuildFooter(IGraphics *g, const IRECT &bounds,
                                  const IVStyle &style) {
  g->AttachControl(new SectionFrame(bounds, "", PolyTheme::SectionBorder,
                                    PolyTheme::TextDark, PolyTheme::HeaderBG));

  const IRECT inner = bounds.GetPadded(-10.f);

  // 1. Demo Controls (Left)
  g->AttachControl(
      new ITextControl(inner.GetFromLeft(50.f).GetMidVPadded(10.f), "DEMO",
                       IText(14.f, PolyTheme::TextDark, "Bold", EAlign::Near)));

  IRECT demoArea =
      inner.GetFromLeft(200.f).GetTranslated(50.f, 0).GetCentredInside(180.f,
                                                                       30.f);
  g->AttachControl(
      new PolyToggleButton(demoArea.GetGridCell(0, 0, 1, 3).GetPadded(-2.f),
                           kParamDemoMono, "MONO"));
  g->AttachControl(
      new PolyToggleButton(demoArea.GetGridCell(0, 1, 1, 3).GetPadded(-2.f),
                           kParamDemoPoly, "POLY"));
  g->AttachControl(new PolyToggleButton(
      demoArea.GetGridCell(0, 2, 1, 3).GetPadded(-2.f), kParamDemoFX, "FX"));

  // 2. Logo (Right)
  IRECT logoArea = inner.GetFromRight(200.f);
  g->AttachControl(
      new ITextControl(logoArea.GetCentredInside(200.f, 32.f), "PolySynth",
                       IText(26.f, PolyTheme::TextDark, "Bold", EAlign::Far)));
}
#endif

#if IPLUG_DSP
void PolySynthPlugin::ProcessBlock(sample **inputs, sample **outputs,
                                   int nFrames) {
  if (mPendingDSPReset.exchange(false, std::memory_order_acquire)) {
    mEngine.Init(GetSampleRate());
  }
  mDemoSequencer.Process(nFrames, GetSampleRate(),
                         [this](const IMidiMsg& msg) { DispatchMidiToEngine(msg); },
                         [this](const IMidiMsg& msg) { SendMidiMsgFromDelegate(msg); });
  PolySynthCore::SynthState tmp;
  while (mStateQueue.TryPop(tmp)) mAudioState = tmp;
  mEngine.UpdateState(mAudioState);
  mEngine.Process(inputs, outputs, nFrames, 2);
}
void PolySynthPlugin::OnIdle() {
#if IPLUG_EDITOR
  // Update active voice count display
  if (GetUI()) {
    if (auto *pControl = GetUI()->GetControlWithTag(kCtrlTagActiveVoices)) {
      int active = mEngine.GetActiveVoiceCount();
      int limit = mState.polyphony;
      char buf[16];
      snprintf(buf, sizeof(buf), "%d/%d", active, limit);
      static_cast<ITextControl *>(pControl)->SetStr(buf);
      pControl->SetDirty(false);
    }

    // Update chord name display
    if (auto *pControl = GetUI()->GetControlWithTag(kCtrlTagChordName)) {
      std::array<int, PolySynthCore::kMaxVoices> notes{};
      int noteCount = mEngine.GetHeldNotes(notes);

      char chordBuf[32] = "";
      if (noteCount >= 3) {
        auto result = sea::TheoryEngine::Analyze(notes.data(), noteCount);
        if (result.valid) {
          sea::TheoryEngine::FormatChordName(result, chordBuf,
                                             sizeof(chordBuf));
        }
      }
      static_cast<ITextControl *>(pControl)->SetStr(chordBuf);
      pControl->SetDirty(false);
    }
  }
#endif
}
void PolySynthPlugin::OnReset() {
  mEngine.Init(GetSampleRate());
  mAudioState = mState;
  // Drain any stale queued states
  PolySynthCore::SynthState tmp;
  while (mStateQueue.TryPop(tmp)) {}
  mEngine.UpdateState(mAudioState);
}
void PolySynthPlugin::DispatchMidiToEngine(const IMidiMsg &msg) {
  int status = msg.StatusMsg();
  if (status == IMidiMsg::kNoteOn) {
    mEngine.OnNoteOn(msg.NoteNumber(), msg.Velocity());
  } else if (status == IMidiMsg::kNoteOff) {
    mEngine.OnNoteOff(msg.NoteNumber());
  } else if (status == IMidiMsg::kControlChange) {
    if (msg.mData1 == 64) {
      mEngine.OnSustainPedal(msg.mData2 >= 64);
    }
  }
}
void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  DispatchMidiToEngine(msg);
#if IPLUG_EDITOR
  SendMidiMsgFromDelegate(msg); // Forward to UI thread for envelope animation
#endif
}
#endif // IPLUG_DSP

#if IPLUG_EDITOR
void PolySynthPlugin::OnMidiMsgUI(const IMidiMsg &msg) {
  auto *pUI = GetUI();
  if (!pUI) return;
  auto *pEnv = dynamic_cast<Envelope *>(pUI->GetControlWithTag(kCtrlTagEnvelope));
  if (!pEnv) return;

  using Clock = std::chrono::steady_clock;
  const int status = msg.StatusMsg();

  if (status == IMidiMsg::kNoteOn && msg.Velocity() > 0) {
    const int note = msg.NoteNumber();

    // Count currently occupied slots (sustaining + releasing).
    int count = 0;
    for (int i = 0; i < kUIMaxVoices; i++)
      if (mUISlotNote[i] >= 0) count++;

    // Voice steal: keep occupied slots within the polyphony limit.
    // Prefer stealing the oldest-released slot first (it's already fading),
    // then fall back to the oldest-sustaining slot.
    while (mUIMaxVoices > 0 && count >= mUIMaxVoices) {
      int toSteal = -1;

      // Pass 1 — oldest released
      for (int i = 0; i < kUIMaxVoices; i++) {
        if (mUISlotNote[i] < 0 || !mUISlotReleased[i]) continue;
        if (toSteal < 0 || mUISlotOffTime[i] < mUISlotOffTime[toSteal])
          toSteal = i;
      }
      // Pass 2 — oldest sustaining
      if (toSteal < 0) {
        for (int i = 0; i < kUIMaxVoices; i++) {
          if (mUISlotNote[i] < 0) continue;
          if (toSteal < 0 || mUISlotOnTime[i] < mUISlotOnTime[toSteal])
            toSteal = i;
        }
      }
      if (toSteal < 0) break;

      const int stolenNote = mUISlotNote[toSteal];
      if (stolenNote >= 0 && stolenNote < 128 && mNoteToUISlot[stolenNote] == toSteal)
        mNoteToUISlot[stolenNote] = -1;
      mUISlotNote[toSteal] = -1;
      count--;
    }

    // Reuse this note's existing slot if it was retriggered, otherwise find a free one.
    int slot = (note >= 0 && note < 128) ? mNoteToUISlot[note] : -1;
    if (slot < 0) {
      for (int i = 0; i < kUIMaxVoices; i++) {
        if (mUISlotNote[i] < 0) { slot = i; break; }
      }
    }
    if (slot < 0) return; // No slot available (shouldn't happen after stealing)

    // Evict any previous occupant of this slot from the note→slot map.
    const int prevNote = mUISlotNote[slot];
    if (prevNote >= 0 && prevNote < 128 && prevNote != note)
      mNoteToUISlot[prevNote] = -1;

    mNoteToUISlot[note]   = slot;
    mUISlotNote[slot]     = note;
    mUISlotReleased[slot] = false;
    mUISlotOnTime[slot]   = Clock::now();
    pEnv->OnVoiceOn(slot);

  } else if (status == IMidiMsg::kNoteOff ||
             (status == IMidiMsg::kNoteOn && msg.Velocity() == 0)) {
    const int note = msg.NoteNumber();
    const int slot = (note >= 0 && note < 128) ? mNoteToUISlot[note] : -1;
    if (slot < 0) return;

    mUISlotReleased[slot] = true;
    mUISlotOffTime[slot]  = Clock::now();
    mNoteToUISlot[note]   = -1;
    // Keep mUISlotNote[slot] set so the slot is visible as "released"
    // and is preferred for stealing over sustaining slots.
    pEnv->OnVoiceOff(slot);
  }
}
#endif // IPLUG_EDITOR

#if IPLUG_DSP
void PolySynthPlugin::OnParamChange(int paramIdx) {
  if (mIsUpdatingUI)
    return;

  // Mark as dirty and update save button
  mIsDirty = true;
#if IPLUG_EDITOR
  if (auto *pUI = GetUI()) {
    if (auto *pBtn = pUI->GetControlWithTag(kCtrlTagSaveBtn)) {
      ((PresetSaveButton *)pBtn)->SetHasUnsavedChanges(true);
    }
  }
#endif

  double value = GetParam(paramIdx)->Value();

  // ── Table-driven parameters ──
  const ParamMeta *meta = FindParamMeta(paramIdx);
  if (meta) {
    ApplyParamToState(*meta, value, mState);
    // OscMix also sets mixOscA (dual-field update)
    if (paramIdx == kParamOscMix)
      mState.mixOscA = 1.0 - mState.mixOscB;
  } else {
    // ── Special cases (enums, frequency, milliseconds, demos, presets) ──
    switch (paramIdx) {
    case kParamLFOShape:
      mState.lfoShape = static_cast<int>(value);
      break;
    case kParamLFORateHz:
      mState.lfoRate = value;
      break;
    case kParamOscWave:
      mState.oscAWaveform = static_cast<int>(value);
      break;
    case kParamOscBWave:
      mState.oscBWaveform = static_cast<int>(value);
      break;
    case kParamFilterModel:
      mState.filterModel = static_cast<int>(value);
      break;
    case kParamChorusRate:
      mState.fxChorusRate = value;
      break;
    case kParamDelayTime:
      mState.fxDelayTime = value / kToMs;
      break;
    case kParamAllocationMode:
      mState.allocationMode = static_cast<int>(value);
      break;
    case kParamStealPriority:
      mState.stealPriority = static_cast<int>(value);
      break;
    case kParamPresetSelect:
      mIsDirty = false;
      OnMessage(kMsgTagLoadPreset, static_cast<int>(value), 0, nullptr);
      break;
    case kParamDemoMono:
    case kParamDemoPoly:
    case kParamDemoFX:
      HandleDemoButton(paramIdx, value);
      break;
    default:
      break;
    }
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

  mStateQueue.TryPush(mState);
}

void PolySynthPlugin::HandleDemoButton(int paramIdx, double value) {
  if (value <= 0.5)
    return; // Ignore release

  DemoSequencer::Mode targetMode;
  switch (paramIdx) {
  case kParamDemoMono:
    targetMode = DemoSequencer::Mode::Mono;
    break;
  case kParamDemoPoly:
    targetMode = DemoSequencer::Mode::Poly;
    break;
  case kParamDemoFX:
    targetMode = DemoSequencer::Mode::FX;
    break;
  default:
    return;
  }

  if (mDemoSequencer.GetMode() == targetMode) {
    // Toggle off
    mDemoSequencer.SetMode(DemoSequencer::Mode::Off, GetSampleRate());
    mPendingDSPReset.store(true, std::memory_order_release);
    GetParam(paramIdx)->Set(0.0);
    if (targetMode == DemoSequencer::Mode::FX) {
      mState.fxChorusMix = 0.0;
      mState.fxDelayMix = 0.0;
    }
  } else {
    // Switch to target mode
    mDemoSequencer.SetMode(targetMode, GetSampleRate());
    mPendingDSPReset.store(true, std::memory_order_release);
    // Clear the other demo buttons
    if (paramIdx != kParamDemoMono)
      GetParam(kParamDemoMono)->Set(0.0);
    if (paramIdx != kParamDemoPoly)
      GetParam(kParamDemoPoly)->Set(0.0);
    if (paramIdx != kParamDemoFX)
      GetParam(kParamDemoFX)->Set(0.0);
    if (targetMode == DemoSequencer::Mode::FX) {
      mState.fxChorusMix = 0.35;
      mState.fxDelayMix = 0.35;
    }
  }
  SyncUIState();
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
    mIsDirty = !success; // Remains dirty if save failed
    if (GetUI()) {
      if (auto *pBtn = GetUI()->GetControlWithTag(kCtrlTagSaveBtn)) {
        ((PresetSaveButton *)pBtn)->SetHasUnsavedChanges(mIsDirty);
      }
    }
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
      if (GetUI()) {
        if (auto *pBtn = GetUI()->GetControlWithTag(kCtrlTagSaveBtn)) {
          ((PresetSaveButton *)pBtn)->SetHasUnsavedChanges(false);
        }
      }
      return true;
    }
    return false;
  }
#endif
  else if (msgTag == kMsgTagNoteOn) {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(ctrlTag, 100, 0);
    DispatchMidiToEngine(msg);
#if IPLUG_EDITOR
    OnMidiMsgUI(msg); // routes through plugin voice tracking → OnVoiceOn(slot)
#endif
    return true;
  } else if (msgTag == kMsgTagNoteOff) {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(ctrlTag, 0);
    DispatchMidiToEngine(msg);
#if IPLUG_EDITOR
    OnMidiMsgUI(msg); // routes through plugin voice tracking → OnVoiceOff(slot)
#endif
    return true;
  }
  return false;
}
#endif
