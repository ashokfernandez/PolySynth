#pragma once

#include "ADSRViewModel.h"
#include "IControl.h"
#include "IGraphics.h"
#include <atomic>

#if defined IGRAPHICS_SKIA
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "effects/SkGradientShader.h"
#endif

using namespace iplug;
using namespace igraphics;

class CAnimatedEnvelopeControl : public IControl {
public:
  CAnimatedEnvelopeControl(const IRECT &bounds,
                           std::atomic<float> *voicePhase = nullptr)
      : IControl(bounds), mVoicePhase(voicePhase), mPhaseRef(0.0f),
        mPathIsDirty(true) {
    mIgnoreMouse = true;
  }

  void SetADSR(float a, float d, float s, float r) {
    mViewModel.SetParams(a, d, s, r, 0.0f, 0.0f, 0.0f);
    mPathIsDirty = true;
    SetDirty(false);
  }

  void SetTensions(float at, float dt, float rt) {
    // Just as an example if tensions were used
    mPathIsDirty = true;
    SetDirty(false);
  }

  void SetColors(const IColor &stroke, const IColor &fill) {
    // Since we compute from base, we can just use the stroke color as base
    // indicator For accurate HSL we'd extract HSL from IColor, but simplified
    // here
    mViewModel.SetBaseColorHSL(182.0f, 1.0f, 0.42f); // Default for now
    mPathIsDirty = true;
    SetDirty(false);
  }

  void OnResize() override {
    mViewModel.SetBounds(mRECT.L, mRECT.T, mRECT.W(), mRECT.H());
    mPathIsDirty = true;
    SetDirty(false);
  }

  void OnIdle() override {
    if (mVoicePhase) {
      float phase = mVoicePhase->load(std::memory_order_acquire);
      if (std::abs(phase - mPhaseRef) > 0.005f) {
        mPhaseRef = phase;
        SetDirty(false);
      }
    }
  }

  void Draw(IGraphics &g) override {
#if defined IGRAPHICS_SKIA
    SkCanvas *canvas = static_cast<SkCanvas *>(g.GetDrawContext());
    if (!canvas)
      return;

    if (mPathIsDirty) {
      RecalculatePath();
    }

    g.FillRoundRect(IColor(255, 233, 235, 230), mRECT, 4.f);
    g.DrawRoundRect(IColor(255, 207, 206, 202), mRECT, 4.f, nullptr, 1.f);

    SkPaint paint;
    paint.setAntiAlias(true);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    auto faintColor = mViewModel.GetColor(ADSRViewModel::Theme::Faint);
    paint.setColor(
        SkColorSetARGB(faintColor.A, faintColor.R, faintColor.G, faintColor.B));
    canvas->drawPath(mEnvelopePath, paint);

    float currentX = mViewModel.GetPlayheadX(mPhaseRef);
    canvas->save();
    canvas->clipRect(SkRect::MakeLTRB(mRECT.L, mRECT.T, currentX, mRECT.B),
                     SkClipOp::kIntersect, true);

    paint.setStyle(SkPaint::kFill_Style);
    auto fillColor = mViewModel.GetColor(ADSRViewModel::Theme::Fill);
    paint.setColor(
        SkColorSetARGB(fillColor.A, fillColor.R, fillColor.G, fillColor.B));
    canvas->drawPath(mEnvelopePath, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    auto darkColor = mViewModel.GetColor(ADSRViewModel::Theme::Dark);
    paint.setColor(
        SkColorSetARGB(darkColor.A, darkColor.R, darkColor.G, darkColor.B));
    canvas->drawPath(mEnvelopePath, paint);
    canvas->restore();

    // Scanner Layer
    canvas->save();
    canvas->clipPath(mEnvelopePath, SkClipOp::kIntersect, true);

    SkPoint pts[2] = {SkPoint::Make(currentX - 80.0f, 0),
                      SkPoint::Make(currentX, 0)};
    auto glowColor = mViewModel.GetColor(ADSRViewModel::Theme::Glow);
    SkColor colors[2] = {
        SK_ColorTRANSPARENT,
        SkColorSetARGB(glowColor.A, glowColor.R, glowColor.G, glowColor.B)};

    paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                 SkTileMode::kClamp));
    paint.setStyle(SkPaint::kFill_Style);
    SkRect glowRect =
        SkRect::MakeLTRB(currentX - 80.0f, mRECT.T, currentX, mRECT.B);
    canvas->drawRect(glowRect, paint);

    paint.setShader(nullptr);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(
        SkColorSetARGB(darkColor.A, darkColor.R, darkColor.G, darkColor.B));
    canvas->drawLine(currentX, mRECT.T, currentX, mRECT.B, paint);

    canvas->restore();
#else
    g.DrawText(IText(14.f, COLOR_WHITE), "Skia Required", mRECT);
#endif
  }

private:
#if defined IGRAPHICS_SKIA
  void RecalculatePath() {
    mEnvelopePath.reset();
    auto data = mViewModel.CalculatePathData();

    mEnvelopePath.moveTo(mRECT.L, mRECT.B);
    mEnvelopePath.quadTo(data.attackControlPoint.x, data.attackControlPoint.y,
                         data.attackNode.x, data.attackNode.y);
    mEnvelopePath.quadTo(data.decayControlPoint.x, data.decayControlPoint.y,
                         data.decaySustainNode.x, data.decaySustainNode.y);
    mEnvelopePath.lineTo(data.releaseControlPoint.x, data.decaySustainNode.y);
    mEnvelopePath.quadTo(data.releaseControlPoint.x, data.releaseControlPoint.y,
                         data.releaseNode.x, mRECT.B);
    mEnvelopePath.lineTo(mRECT.L, mRECT.B);
    mEnvelopePath.close();

    mPathIsDirty = false;
  }

  SkPath mEnvelopePath;
#endif

  ADSRViewModel mViewModel;
  std::atomic<float> *mVoicePhase;
  float mPhaseRef;
  bool mPathIsDirty;
};
