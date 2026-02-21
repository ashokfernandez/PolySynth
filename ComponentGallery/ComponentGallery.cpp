#include "ComponentGallery.h"
#include "IPlug_include_in_plug_src.h"
#include "VRTCaptureHelper.h"

#if IPLUG_EDITOR
#include "Envelope.h"
#include "LCDPanel.h"
#include "PolyKnob.h"
#include "PolySection.h"
#include "PolyToggleButton.h"
#include "PresetSaveButton.h"
#include "SectionFrame.h"
#endif

#include <cstdlib>
#include <cstring>

// Layout constants
static constexpr float kSidebarW = 180.f;
static constexpr float kBottomBarH = 50.f;
static constexpr float kPad = 20.f;

static const char *kComponentNames[kNumComponents] = {
    "Envelope",     "PolyKnob", "PolySection",     "PolyToggle",
    "SectionFrame", "LCDPanel", "PresetSaveButton"};

ComponentGallery::ComponentGallery(const InstanceInfo &info)
    : iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  GetParam(kParamKnobPrimary)->InitDouble("Knob Primary", 0.5, 0., 1., 0.01);
  GetParam(kParamKnobSecondary)
      ->InitDouble("Knob Secondary", 0.5, 0., 1., 0.01);
  GetParam(kParamToggleMono)->InitBool("Toggle Mono", false);
  GetParam(kParamTogglePoly)->InitBool("Toggle Poly", false);
  GetParam(kParamToggleFX)->InitBool("Toggle FX", false);
  GetParam(kParamStateSlider)->InitDouble("State", 0., 0., 1., 0.01);

  // VRT capture mode: activated via environment variable for CLI/CI use.
  const char *vrtMode = std::getenv("POLYSYNTH_VRT_MODE");
  if (vrtMode && std::strcmp(vrtMode, "1") == 0) {
    mVRTCaptureMode = true;
    const char *outDir = std::getenv("POLYSYNTH_VRT_OUTPUT_DIR");
    mVRTOutputDir = outDir ? outDir : "tests/Visual/current";
  }

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [this](IGraphics *pGraphics) { OnLayout(pGraphics); };
#endif
}

void ComponentGallery::OnIdle() {
#if IPLUG_EDITOR
  if (!mVRTCaptureMode || mVRTCaptureFinished)
    return;

  IGraphics *pGraphics = GetUI();
  if (!pGraphics)
    return; // Editor not yet open

  // Accumulate warmup frames so all controls have rendered at least once
  // before we attempt pixel readback.
  if (mVRTWarmupFrames < 10) {
    // Switch to target component on first warmup frame
    if (mVRTWarmupFrames == 0) {
      ShowComponent(mCurrentVRTComponent);
    }
    pGraphics->SetAllControlsDirty();
    ++mVRTWarmupFrames;
    return;
  }

  // Capture current component!
  const std::string outDir =
      mVRTOutputDir.empty() ? "tests/Visual/current" : mVRTOutputDir;
  VRTCapture::EnsureDir(outDir);
  SaveComponentPNG(mCurrentVRTComponent,
                   outDir + "/" + kComponentNames[mCurrentVRTComponent] +
                       ".png");

  // Advance to next component
  mCurrentVRTComponent++;
  mVRTWarmupFrames = 0;

  if (mCurrentVRTComponent >= kNumComponents) {
    mVRTCaptureFinished = true;
    std::fprintf(stdout, "[VRTCapture] Finished capturing %d components.\n",
                 kNumComponents);
    std::exit(0);
  }
#endif
}

#if IPLUG_EDITOR

void ComponentGallery::OnLayout(IGraphics *pGraphics) {
  if (pGraphics->NControls())
    return;

  // Load bundled fonts (must precede any IText that references them).
  bool f1 = pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
  bool f2 = pGraphics->LoadFont("Roboto-Bold", ROBOTO_BOLD_FN);
  bool f3 = pGraphics->LoadFont("Regular", ROBOTO_FN);
  bool f4 = pGraphics->LoadFont("Bold", ROBOTO_BOLD_FN);
  if (!f1 || !f2 || !f3 || !f4) {
    std::fprintf(stderr, "[FontLoad] Failed! f1=%d, f2=%d, f3=%d, f4=%d\n", f1,
                 f2, f3, f4);
  }

  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);

  const IRECT sidebarBounds(0.f, 0.f, kSidebarW, PLUG_HEIGHT - kBottomBarH);
  const IRECT mainBounds(kSidebarW, 0.f, PLUG_WIDTH, PLUG_HEIGHT - kBottomBarH);

  // ── Sidebar navigation buttons ───────────────────────────────────────────
  const float btnH = 36.f;
  const float btnPad = 4.f;

  for (int i = 0; i < kNumComponents; ++i) {
    const int idx = i;
    IRECT btnRect(btnPad, btnPad + i * (btnH + btnPad), kSidebarW - btnPad,
                  btnPad + i * (btnH + btnPad) + btnH);

    auto *btn = new IVButtonControl(
        btnRect, [this, idx](IControl *) { ShowComponent(idx); },
        kComponentNames[i]);
    pGraphics->AttachControl(btn);
    mSidebarButtons.push_back(btn);
  }

  // ── Main viewing area origin ─────────────────────────────────────────────
  const float mx = mainBounds.L + kPad;
  const float my = mainBounds.T + kPad;

  // ── Envelope (400 × 150) ─────────────────────────────────────────────────
  {
    const float ew = 400.f, eh = 150.f;
    IRECT r(mx, my, mx + ew, my + eh);
    mComponentBounds[kComponentEnvelope] = r;
    auto *pEnv = new Envelope(r);
    pEnv->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
    pGraphics->AttachControl(pEnv);
    mComponentControls[kComponentEnvelope].push_back(pEnv);
  }

  // ── PolyKnob (3 variants) ─────────────────────────────────────────────────
  {
    const float ks = 80.f, kg = 20.f;
    IRECT r(mx, my, mx + 3.f * ks + 2.f * kg, my + ks + 36.f);
    mComponentBounds[kComponentPolyKnob] = r;

    auto *k1 = new PolyKnob(IRECT(mx, my, mx + ks, my + ks + 36.f),
                            kParamKnobPrimary, "Cutoff");
    pGraphics->AttachControl(k1);
    mComponentControls[kComponentPolyKnob].push_back(k1);

    auto *k2 = new PolyKnob(
        IRECT(mx + ks + kg, my, mx + 2.f * ks + kg, my + ks + 36.f),
        kParamKnobSecondary, "Reso");
    k2->SetAccent(PolyTheme::AccentCyan);
    pGraphics->AttachControl(k2);
    mComponentControls[kComponentPolyKnob].push_back(k2);

    auto *k3 = new PolyKnob(IRECT(mx + 2.f * (ks + kg), my + 18.f,
                                  mx + 2.f * (ks + kg) + ks, my + 18.f + ks),
                            kParamKnobPrimary, "");
    k3->WithShowLabel(false).WithShowValue(false);
    pGraphics->AttachControl(k3);
    mComponentControls[kComponentPolyKnob].push_back(k3);
  }

  // ── PolySection (2 panels) ───────────────────────────────────────────────
  {
    const float sw = 300.f, sh = 200.f, sg = 20.f;
    IRECT r(mx, my, mx + 2.f * sw + sg, my + sh);
    mComponentBounds[kComponentPolySection] = r;

    auto *s1 = new PolySection(IRECT(mx, my, mx + sw, my + sh), "FILTER");
    pGraphics->AttachControl(s1);
    mComponentControls[kComponentPolySection].push_back(s1);

    auto *s2 = new PolySection(
        IRECT(mx + sw + sg, my, mx + 2.f * sw + sg, my + sh), "LFO");
    pGraphics->AttachControl(s2);
    mComponentControls[kComponentPolySection].push_back(s2);
  }

  // ── PolyToggle (3 buttons) ───────────────────────────────────────────────
  {
    const float tw = 80.f, th = 28.f, tg = 10.f;
    IRECT r(mx, my, mx + 3.f * tw + 2.f * tg, my + th);
    mComponentBounds[kComponentPolyToggle] = r;

    auto *t1 = new PolyToggleButton(IRECT(mx, my, mx + tw, my + th),
                                    kParamToggleMono, "MONO");
    pGraphics->AttachControl(t1);
    mComponentControls[kComponentPolyToggle].push_back(t1);

    auto *t2 = new PolyToggleButton(
        IRECT(mx + tw + tg, my, mx + 2.f * tw + tg, my + th), kParamTogglePoly,
        "POLY");
    pGraphics->AttachControl(t2);
    mComponentControls[kComponentPolyToggle].push_back(t2);

    auto *t3 = new PolyToggleButton(
        IRECT(mx + 2.f * (tw + tg), my, mx + 3.f * tw + 2.f * tg, my + th),
        kParamToggleFX, "FX");
    pGraphics->AttachControl(t3);
    mComponentControls[kComponentPolyToggle].push_back(t3);
  }

  // ── SectionFrame (380 × 220) ─────────────────────────────────────────────
  {
    const float fw = 380.f, fh = 220.f;
    IRECT r(mx, my, mx + fw, my + fh);
    mComponentBounds[kComponentSectionFrame] = r;

    auto *sf = new SectionFrame(r, "MOD MATRIX", PolyTheme::SectionBorder,
                                PolyTheme::TextDark, PolyTheme::ControlBG);
    pGraphics->AttachControl(sf);
    mComponentControls[kComponentSectionFrame].push_back(sf);
  }

  // ── LCDPanel (320 × 72) ──────────────────────────────────────────────────
  {
    const float lw = 320.f, lh = 72.f;
    IRECT r(mx, my, mx + lw, my + lh);
    mComponentBounds[kComponentLCDPanel] = r;

    auto *lcd = new LCDPanel(r, PolyTheme::LCDBackground);
    pGraphics->AttachControl(lcd);
    mComponentControls[kComponentLCDPanel].push_back(lcd);

    auto *txt =
        new ITextControl(r.GetPadded(-14.f), "PRESET: INIT",
                         IText(PolyTheme::FontControl + 1.f, PolyTheme::LCDText,
                               "Regular", EAlign::Center));
    pGraphics->AttachControl(txt);
    mComponentControls[kComponentLCDPanel].push_back(txt);
  }

  // ── PresetSaveButton (dirty + clean states) ───────────────────────────────
  {
    const float bw = 120.f, bh = 34.f, bg = 16.f, lblH = 18.f;
    IRECT r(mx, my, mx + 2.f * bw + bg, my + lblH + bh);
    mComponentBounds[kComponentPresetSaveButton] = r;

    auto *lbl1 =
        new ITextControl(IRECT(mx, my, mx + bw, my + lblH), "DIRTY",
                         IText(PolyTheme::FontControl - 1.f,
                               PolyTheme::TextDark, "Regular", EAlign::Center));
    pGraphics->AttachControl(lbl1);
    mComponentControls[kComponentPresetSaveButton].push_back(lbl1);

    auto *dirty = new PresetSaveButton(
        IRECT(mx, my + lblH, mx + bw, my + lblH + bh), [](IControl *) {});
    dirty->SetHasUnsavedChanges(true);
    pGraphics->AttachControl(dirty);
    mComponentControls[kComponentPresetSaveButton].push_back(dirty);

    const float cx = mx + bw + bg;
    auto *lbl2 =
        new ITextControl(IRECT(cx, my, cx + bw, my + lblH), "CLEAN",
                         IText(PolyTheme::FontControl - 1.f,
                               PolyTheme::TextDark, "Regular", EAlign::Center));
    pGraphics->AttachControl(lbl2);
    mComponentControls[kComponentPresetSaveButton].push_back(lbl2);

    auto *clean = new PresetSaveButton(
        IRECT(cx, my + lblH, cx + bw, my + lblH + bh), [](IControl *) {});
    pGraphics->AttachControl(clean);
    mComponentControls[kComponentPresetSaveButton].push_back(clean);
  }

  // ── Default view: PolyKnob ───────────────────────────────────────────────
  ShowComponent(kComponentPolyKnob);
}

void ComponentGallery::ShowComponent(int idx) {
  for (int i = 0; i < kNumComponents; ++i) {
    const bool hide = (i != idx);
    for (auto *pCtrl : mComponentControls[i])
      pCtrl->Hide(hide);
  }
  mCurrentComponent = idx;

  IGraphics *pGraphics = GetUI();
  if (pGraphics)
    pGraphics->SetAllControlsDirty();
}

void ComponentGallery::OnParamChange(int paramIdx, EParamSource /*source*/,
                                     int /*sampleOffset*/) {
  if (paramIdx == kParamStateSlider) {
    IGraphics *pGraphics = GetUI();
    if (pGraphics)
      pGraphics->SetAllControlsDirty();
  }
}

void ComponentGallery::RunVRTCapture() {
  // Deprecated: capture is now handled incrementally in OnIdle to allow
  // for OS window updates between component switches.
}

void ComponentGallery::SaveComponentPNG(int idx, const std::string &filePath) {
  IGraphics *pGraphics = GetUI();
  if (!pGraphics)
    return;

  if (!VRTCapture::SaveRegionAsPNG(pGraphics, mComponentBounds[idx],
                                   filePath)) {
    std::fprintf(stderr, "[VRTCapture] Failed to save '%s'.\n",
                 filePath.c_str());
  }
}

#endif // IPLUG_EDITOR

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
