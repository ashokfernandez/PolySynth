#pragma once

#include "IControls.h"
#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams {
  kParamKnobPrimary = 0,
  kParamKnobSecondary,
  kParamToggleMono,
  kParamTogglePoly,
  kParamToggleFX,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class ComponentGallery final : public Plugin {
public:
  ComponentGallery(const InstanceInfo &info);

#if IPLUG_EDITOR
  void OnLayout(IGraphics *pGraphics);
#endif

#if IPLUG_DSP
  void ProcessBlock(sample **inputs, sample **outputs, int nFrames) override;
#endif
};
