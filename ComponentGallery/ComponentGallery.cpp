#include "ComponentGallery.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
// Include Envelope (custom control - no built-in equivalent)
#include "Envelope.h"
#endif

#include <cstring>

ComponentGallery::ComponentGallery(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  GetParam(kParamKnob)->InitDouble("Knob", 0.5, 0., 1., 0.01);
  GetParam(kParamFader)->InitDouble("Fader", 0.5, 0., 1., 0.01);
  GetParam(kParamButton)->InitBool("Button", false);
  GetParam(kParamSwitch)->InitEnum("Switch", 0, 3);
  GetParam(kParamToggle)->InitBool("Toggle", false);
  GetParam(kParamSlideSwitch)->InitBool("Slide Switch", false);
  GetParam(kParamTabSwitch)
      ->InitEnum("Tab Switch", 0, {"Tab 1", "Tab 2", "Tab 3"});
  GetParam(kParamRadioButton)
      ->InitEnum("Radio Button", 0, {"Option 1", "Option 2", "Option 3"});

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
#if defined(GALLERY_COMPONENT_KNOB)
  const float knobSize = 100.0f;
  pGraphics->AttachControl(new IVKnobControl(
      IRECT(padding, padding, padding + knobSize, padding + knobSize),
      kParamKnob));

#elif defined(GALLERY_COMPONENT_FADER)
  const float faderWidth = 60.0f;
  const float faderHeight = 200.0f;
  pGraphics->AttachControl(new IVSliderControl(
      IRECT(padding, padding, padding + faderWidth, padding + faderHeight),
      kParamFader));

#elif defined(GALLERY_COMPONENT_ENVELOPE)
  const float envelopeWidth = 400.0f;
  const float envelopeHeight = 150.0f;
  Envelope *pEnvelope = new Envelope(IRECT(
      padding, padding, padding + envelopeWidth, padding + envelopeHeight));
  pEnvelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
  pGraphics->AttachControl(pEnvelope);

#elif defined(GALLERY_COMPONENT_BUTTON)
  // Pill-shaped button using roundness = 1.0f
  const float buttonWidth = 120.0f;
  const float buttonHeight = 40.0f;
  IVStyle pillStyle = DEFAULT_STYLE.WithRoundness(1.0f);
  pGraphics->AttachControl(
      new IVButtonControl(IRECT(padding, padding, padding + buttonWidth,
                                padding + buttonHeight),
                          nullptr, "Button", pillStyle),
      kNoTag);

#elif defined(GALLERY_COMPONENT_SWITCH)
  const float switchSize = 60.0f;
  pGraphics->AttachControl(new IVSwitchControl(
      IRECT(padding, padding, padding + switchSize, padding + switchSize),
      kParamSwitch));

#elif defined(GALLERY_COMPONENT_TOGGLE)
  const float toggleWidth = 80.0f;
  const float toggleHeight = 40.0f;
  pGraphics->AttachControl(new IVToggleControl(
      IRECT(padding, padding, padding + toggleWidth, padding + toggleHeight),
      kParamToggle));

#elif defined(GALLERY_COMPONENT_SLIDESWITCH)
  const float slideWidth = 100.0f;
  const float slideHeight = 40.0f;
  pGraphics->AttachControl(new IVSlideSwitchControl(
      IRECT(padding, padding, padding + slideWidth, padding + slideHeight),
      kParamSlideSwitch));

#elif defined(GALLERY_COMPONENT_TABSWITCH)
  const float tabWidth = 300.0f;
  const float tabHeight = 40.0f;
  pGraphics->AttachControl(new IVTabSwitchControl(
      IRECT(padding, padding, padding + tabWidth, padding + tabHeight),
      kParamTabSwitch, {"One", "Two", "Three"}));

#elif defined(GALLERY_COMPONENT_RADIOBUTTON)
  const float radioWidth = 150.0f;
  const float radioHeight = 120.0f;
  pGraphics->AttachControl(new IVRadioButtonControl(
      IRECT(padding, padding, padding + radioWidth, padding + radioHeight),
      kParamRadioButton, {"Choice A", "Choice B", "Choice C"}));

#else
  // Default: show knob
  const float knobSize = 100.0f;
  pGraphics->AttachControl(new IVKnobControl(
      IRECT(padding, padding, padding + knobSize, padding + knobSize),
      kParamKnob));
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
