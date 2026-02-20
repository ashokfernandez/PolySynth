#include "ComponentGallery.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
// Custom controls from UI/Controls
#include "Envelope.h"
#include "LCDPanel.h"
#include "PolyKnob.h"
#include "PolySection.h"
#include "PolyToggleButton.h"
#include "PresetSaveButton.h"
#include "SectionFrame.h"
#endif

#include <cstring>

ComponentGallery::ComponentGallery(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  GetParam(kParamKnobPrimary)->InitDouble("Knob Primary", 0.5, 0., 1., 0.01);
  GetParam(kParamKnobSecondary)
      ->InitDouble("Knob Secondary", 0.5, 0., 1., 0.01);
  GetParam(kParamToggleMono)->InitBool("Toggle Mono", false);
  GetParam(kParamTogglePoly)->InitBool("Toggle Poly", false);
  GetParam(kParamToggleFX)->InitBool("Toggle FX", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [this](IGraphics *pGraphics) { OnLayout(pGraphics); };
#endif
}

#if IPLUG_EDITOR
void ComponentGallery::OnLayout(IGraphics *pGraphics) {
  if (pGraphics->NControls())
    return;

  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);

  const float padding = 20.0f;

  // Build-time component selection via preprocessor defines
#if defined(GALLERY_COMPONENT_ENVELOPE)
  const float envelopeWidth = 400.0f;
  const float envelopeHeight = 150.0f;
  Envelope *pEnvelope = new Envelope(IRECT(
      padding, padding, padding + envelopeWidth, padding + envelopeHeight));
  pEnvelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
  pGraphics->AttachControl(pEnvelope);

#elif defined(GALLERY_COMPONENT_POLYKNOB)
  // PolyKnob: custom themed knob with arc indicator, label, and value readout
  const float polyKnobSize = 80.0f;
  const float polyKnobSpacing = 20.0f;

  // Default accent (red)
  pGraphics->AttachControl(new PolyKnob(
      IRECT(padding, padding, padding + polyKnobSize, padding + polyKnobSize + 36.f),
      kParamKnobPrimary, "Cutoff"));

  // Cyan accent variant
  auto* cyanKnob = new PolyKnob(
      IRECT(padding + polyKnobSize + polyKnobSpacing, padding,
            padding + 2.f * polyKnobSize + polyKnobSpacing,
            padding + polyKnobSize + 36.f),
      kParamKnobSecondary, "Reso");
  cyanKnob->SetAccent(PolyTheme::AccentCyan);
  pGraphics->AttachControl(cyanKnob);

  // No-label/no-value variant (as used in stacked controls)
  auto* bareKnob = new PolyKnob(
      IRECT(padding + 2.f * (polyKnobSize + polyKnobSpacing), padding + 18.f,
            padding + 2.f * (polyKnobSize + polyKnobSpacing) + polyKnobSize,
            padding + 18.f + polyKnobSize),
      kParamKnobPrimary, "");
  bareKnob->WithShowLabel(false).WithShowValue(false);
  pGraphics->AttachControl(bareKnob);

#elif defined(GALLERY_COMPONENT_POLYSECTION)
  // PolySection: cached panel with title, sheen, scanlines, and depth border
  const float sectionWidth = 300.0f;
  const float sectionHeight = 200.0f;
  pGraphics->AttachControl(new PolySection(
      IRECT(padding, padding, padding + sectionWidth, padding + sectionHeight),
      "FILTER"));

  // Second section to show side-by-side appearance
  pGraphics->AttachControl(new PolySection(
      IRECT(padding + sectionWidth + 20.f, padding,
            padding + 2.f * sectionWidth + 20.f, padding + sectionHeight),
      "LFO"));

#elif defined(GALLERY_COMPONENT_POLYTOGGLE)
  // PolyToggleButton: latching toggle with themed active/inactive states
  const float toggleWidth = 80.0f;
  const float toggleHeight = 28.0f;
  const float toggleGap = 10.0f;

  // Inactive state (default value = 0)
  pGraphics->AttachControl(new PolyToggleButton(
      IRECT(padding, padding, padding + toggleWidth, padding + toggleHeight),
      kParamToggleMono, "MONO"));

  // A second toggle wired to a different param to show both states simultaneously
  pGraphics->AttachControl(new PolyToggleButton(
      IRECT(padding + toggleWidth + toggleGap, padding,
            padding + 2.f * toggleWidth + toggleGap, padding + toggleHeight),
      kParamTogglePoly, "POLY"));

  // A third toggle for the row layout
  pGraphics->AttachControl(new PolyToggleButton(
      IRECT(padding + 2.f * (toggleWidth + toggleGap), padding,
            padding + 3.f * toggleWidth + 2.f * toggleGap, padding + toggleHeight),
      kParamToggleFX, "FX"));

#elif defined(GALLERY_COMPONENT_SECTIONFRAME)
  // SectionFrame: generic framed group container with optional background
  const float frameWidth = 380.0f;
  const float frameHeight = 220.0f;
  pGraphics->AttachControl(new SectionFrame(
      IRECT(padding, padding, padding + frameWidth, padding + frameHeight),
      "MOD MATRIX", PolyTheme::SectionBorder, PolyTheme::TextDark,
      PolyTheme::ControlBG));

#elif defined(GALLERY_COMPONENT_LCDPANEL)
  // LCDPanel: dark "display" panel treatment used in the header preset area
  const float panelWidth = 320.0f;
  const float panelHeight = 72.0f;
  const IRECT panelRect(padding, padding, padding + panelWidth,
                        padding + panelHeight);
  pGraphics->AttachControl(new LCDPanel(panelRect, PolyTheme::LCDBackground));
  pGraphics->AttachControl(new ITextControl(
      panelRect.GetPadded(-14.f), "PRESET: INIT",
      IText(PolyTheme::FontControl + 1.f, PolyTheme::LCDText, "Bold",
            EAlign::Center)));

#elif defined(GALLERY_COMPONENT_PRESETSAVEBUTTON)
  // PresetSaveButton: dirty/clean state variants side-by-side
  const float buttonWidth = 120.0f;
  const float buttonHeight = 34.0f;
  const float buttonGap = 16.0f;
  const float labelHeight = 18.0f;

  auto *dirty = new PresetSaveButton(
      IRECT(padding, padding + labelHeight, padding + buttonWidth,
            padding + labelHeight + buttonHeight),
      [](IControl *ctrl) { (void)ctrl; });
  dirty->SetHasUnsavedChanges(true);
  pGraphics->AttachControl(dirty);
  pGraphics->AttachControl(new ITextControl(
      IRECT(padding, padding, padding + buttonWidth, padding + labelHeight),
      "DIRTY", IText(PolyTheme::FontControl - 1.f, PolyTheme::TextDark,
                     "Bold", EAlign::Center)));

  const float cleanX = padding + buttonWidth + buttonGap;
  pGraphics->AttachControl(new PresetSaveButton(
      IRECT(cleanX, padding + labelHeight, cleanX + buttonWidth,
            padding + labelHeight + buttonHeight),
      [](IControl *ctrl) { (void)ctrl; }));
  pGraphics->AttachControl(new ITextControl(
      IRECT(cleanX, padding, cleanX + buttonWidth, padding + labelHeight),
      "CLEAN", IText(PolyTheme::FontControl - 1.f, PolyTheme::TextDark,
                     "Bold", EAlign::Center)));

#else
  // Default: show PolyKnob (gallery focuses on PolySynth UI controls)
  pGraphics->AttachControl(new PolyKnob(
      IRECT(padding, padding, padding + 80.f, padding + 116.f),
      kParamKnobPrimary, "Cutoff"));
#endif
}
#endif

#if IPLUG_DSP
void ComponentGallery::ProcessBlock(sample **inputs, sample **outputs,
                                    int nFrames) {
  const int nInChans = NInChansConnected();
  const int nOutChans = NOutChansConnected();

  for (int chan = 0; chan < nOutChans; ++chan) {
    if (chan < nInChans && inputs[chan] && outputs[chan])
      std::memcpy(outputs[chan], inputs[chan], nFrames * sizeof(sample));
    else if (outputs[chan])
      std::memset(outputs[chan], 0, nFrames * sizeof(sample));
  }
}
#endif
