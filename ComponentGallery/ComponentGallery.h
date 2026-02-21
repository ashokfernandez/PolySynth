#pragma once

#include "IControls.h"
#include "IPlug_include_in_plug_hdr.h"

#include <string>
#include <vector>

const int kNumPresets = 1;

enum EGalleryComponent {
  kComponentEnvelope = 0,
  kComponentPolyKnob,
  kComponentPolySection,
  kComponentPolyToggle,
  kComponentSectionFrame,
  kComponentLCDPanel,
  kComponentPresetSaveButton,
  kNumComponents
};

enum EParams {
  kParamKnobPrimary = 0,
  kParamKnobSecondary,
  kParamToggleMono,
  kParamTogglePoly,
  kParamToggleFX,
  kParamStateSlider,  // master 0-1 state injection for the visible component
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class ComponentGallery final : public Plugin {
public:
  ComponentGallery(const InstanceInfo& info);

  void OnIdle() override;

#if IPLUG_EDITOR
  void OnLayout(IGraphics* pGraphics);
  void OnParamChange(int paramIdx, EParamSource source, int sampleOffset) override;
#endif

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
  // VRT capture flags — initialised in constructor, acted on in OnIdle.
  bool        mVRTCaptureMode    = false;
  std::string mVRTOutputDir;
  bool        mVRTCaptureStarted = false;
  int         mVRTWarmupFrames   = 0;

#if IPLUG_EDITOR
  void ShowComponent(int idx);
  void RunVRTCapture();
  void SaveComponentPNG(int idx, const std::string& filePath);

  int                    mCurrentComponent = kComponentPolyKnob;
  std::vector<IControl*> mComponentControls[kNumComponents];
  IRECT                  mComponentBounds[kNumComponents] = {};
  std::vector<IControl*> mSidebarButtons;
#endif
};
