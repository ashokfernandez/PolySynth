#include "ComponentGallery.h"
#include "IPlug_include_in_plug_src.h"

#include <cstring>

ComponentGallery::ComponentGallery(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamTestKnob)->InitDouble("Test Knob", 0.5, 0., 1., 0.001);

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

  // iPlug2 currently exposes this control as IVKnobControl.
  using IVectorKnobControl = IVKnobControl;

  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);
  pGraphics->AttachControl(new IVectorKnobControl(IRECT(100.f, 100.f, 200.f, 200.f), kParamTestKnob), kCtrlTag_TestKnob);
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
