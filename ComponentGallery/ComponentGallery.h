#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

const int kNumPresets = 1;

enum EParams
{
  kParamKnob1 = 0,
  kParamKnob2,
  kParamKnob3,
  kParamFader1,
  kParamFader2,
  kParamFader3,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class ComponentGallery final : public Plugin
{
public:
  ComponentGallery(const InstanceInfo& info);

#if IPLUG_EDITOR
  void OnLayout(IGraphics* pGraphics);
#endif

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
