#include "ComponentGallery.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
// Include Envelope (custom control - no built-in equivalent)
#include "Envelope.h"
#endif

#include <cstring>

ComponentGallery::ComponentGallery(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Initialize knob parameters
  GetParam(kParamKnob1)->InitDouble("Knob 1", 0.3, 0., 1., 0.001);
  GetParam(kParamKnob2)->InitDouble("Knob 2", 0.5, 0., 1., 0.001);
  GetParam(kParamKnob3)->InitDouble("Knob 3", 0.7, 0., 1., 0.001);

  // Initialize fader parameters
  GetParam(kParamFader1)->InitDouble("Fader 1", 0.25, 0., 1., 0.001);
  GetParam(kParamFader2)->InitDouble("Fader 2", 0.5, 0., 1., 0.001);
  GetParam(kParamFader3)->InitDouble("Fader 3", 0.75, 0., 1., 0.001);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [this](IGraphics* pGraphics) {
    OnLayout(pGraphics);
  };
#endif
}

#if IPLUG_EDITOR
void ComponentGallery::OnLayout(IGraphics* pGraphics)
{
  if (pGraphics->NControls())
    return;

  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);

  const float padding = 20.0f;

  // Build-time component selection via preprocessor defines
  // Set GALLERY_COMPONENT=KNOB, FADER, or ENVELOPE at compile time

#if defined(GALLERY_COMPONENT_KNOB)
  // Small knob in top-left corner
  const float knobSize = 100.0f;
  pGraphics->AttachControl(new IVKnobControl(
    IRECT(padding, padding, padding + knobSize, padding + knobSize),
    kParamKnob1));

#elif defined(GALLERY_COMPONENT_FADER)
  // Small fader in top-left corner
  const float faderWidth = 60.0f;
  const float faderHeight = 200.0f;
  pGraphics->AttachControl(new IVSliderControl(
    IRECT(padding, padding, padding + faderWidth, padding + faderHeight),
    kParamFader1));

#elif defined(GALLERY_COMPONENT_ENVELOPE)
  // Smaller envelope in top-left corner
  const float envelopeWidth = 400.0f;
  const float envelopeHeight = 150.0f;
  Envelope* pEnvelope = new Envelope(IRECT(
    padding, padding, padding + envelopeWidth, padding + envelopeHeight));
  pEnvelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
  pGraphics->AttachControl(pEnvelope);

#else
  // Default: show small knob in top-left corner if no component specified
  const float knobSize = 100.0f;
  pGraphics->AttachControl(new IVKnobControl(
    IRECT(padding, padding, padding + knobSize, padding + knobSize),
    kParamKnob1));
#endif
}
#endif

#if IPLUG_DSP
void ComponentGallery::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nInChans = NInChansConnected();
  const int nOutChans = NOutChansConnected();

  for (int chan = 0; chan < nOutChans; ++chan)
  {
    if (chan < nInChans && inputs[chan] && outputs[chan])
      std::memcpy(outputs[chan], inputs[chan], nFrames * sizeof(sample));
    else if (outputs[chan])
      std::memset(outputs[chan], 0, nFrames * sizeof(sample));
  }
}
#endif
