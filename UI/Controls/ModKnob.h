#pragma once

// EXPERIMENTAL: ModKnob is a work-in-progress control. Not yet wired into
// the plugin UI. Will be integrated in a future sprint.

#include "IControl.h"
#include "PolyTheme.h"
#include <algorithm>
#include <cmath>

class ModKnob : public IControl {
public:
  ModKnob(const IRECT &bounds, int paramIdx, const char *label,
          float scale = 1.0f)
      : IControl(bounds, paramIdx), mLabelStr(label), mScale(scale) {
    mTooltip.Set("");
    UpdateLayout();
  }

  // ── Public API ──────────────────────────────────────────────────

  void SetScale(float scale) {
    mScale = scale;
    UpdateLayout();
    mCachedLayer = nullptr;
    SetDirty(false);
  }

  void SetAccent(const IColor &color) {
    mAccentColor = color;
    SetDirty(false);
  }

  void SetModulationBounds(double start, double end) {
    if (start > end)
      std::swap(start, end);
    mModStart = std::clamp(start, 0.0, 1.0);
    mModEnd = std::clamp(end, 0.0, 1.0);
    SetDirty(false);
  }

  void SetModCurrent(double val) {
    mModCurrent = std::clamp(val, 0.0, 1.0);
    SetDirty(false);
  }

  void ShowModulation(bool show) {
    mShowModulation = show;
    SetDirty(false);
  }

  double GetValue() const { return mValue; }
  void SetValue(double v) { mValue = std::clamp(v, 0.0, 1.0); }

  double GetModStart() const { return mModStart; }
  double GetModEnd() const { return mModEnd; }
  double GetModCurrent() const { return mModCurrent; }
  bool GetShowModulation() const { return mShowModulation; }

  // ── Drawing ─────────────────────────────────────────────────────

  void Draw(IGraphics &g) override {
    // Label above
    g.DrawText(IText(PolyTheme::FontControl, PolyTheme::TextDark, "Bold",
                     EAlign::Center),
               mLabelStr.Get(), mLabelRect);

    const float cx = mDrawRECT.MW();
    const float cy = mDrawRECT.MH();
    const float halfW = mDrawRECT.W() * 0.5f;

    const float rInner = halfW * kInnerTrackRatio;
    const float rDial = halfW * kDialRatio;
    const float rOuter = halfW * kOuterTrackRatio;

    // ── Static elements (layer-cached) ───────────────────────────
    if (!mCachedLayer) {
      g.StartLayer(this, mRECT);

      // Neumorphic dial body
      g.FillCircle(PolyTheme::ShadowDark, cx + 1.f, cy + 2.f, rDial - 1.f);
      g.FillCircle(PolyTheme::KnobBody, cx, cy, rDial - 1.5f);
      g.DrawCircle(PolyTheme::InnerGlow, cx, cy - 0.5f, rDial - 2.0f, nullptr,
                   1.f);
      g.DrawCircle(PolyTheme::ShadowLight, cx, cy + 1.f, rDial - 2.0f,
                   nullptr, 1.f);

      // Inner track ring (inactive arc)
      g.DrawArc(PolyTheme::KnobRingOff, cx, cy, rInner, kStartAngle,
                kEndAngle, nullptr, 3.f);

      mCachedLayer = g.EndLayer();
    }
    g.DrawLayer(mCachedLayer);

    // ── Dynamic elements (every frame) ───────────────────────────

    // Value arc
    const float valueAngle = kStartAngle + static_cast<float>(mValue) * kSweep;
    g.DrawArc(IColor(70, mAccentColor.R, mAccentColor.G, mAccentColor.B), cx,
              cy, rInner, kStartAngle, valueAngle, nullptr, 6.f);
    g.DrawArc(mAccentColor, cx, cy, rInner, kStartAngle, valueAngle, nullptr,
              3.f);

    // Indicator line + dot
    const float indicatorAngle = DegToRad(valueAngle - 90.f);
    const float ix = cx + std::cos(indicatorAngle) * rDial * 0.7f;
    const float iy = cy + std::sin(indicatorAngle) * rDial * 0.7f;
    const float ox = cx + std::cos(indicatorAngle) * rInner;
    const float oy = cy + std::sin(indicatorAngle) * rInner;
    g.DrawLine(mAccentColor, ix, iy, ox, oy, nullptr, 2.f);
    g.FillCircle(PolyTheme::Highlight, ox, oy, 1.8f);

    // ── Modulation overlay ───────────────────────────────────────
    if (mShowModulation) {
      // Deviation strip on outer track
      const float devMinVal =
          static_cast<float>(std::min(mValue, mModCurrent));
      const float devMaxVal =
          static_cast<float>(std::max(mValue, mModCurrent));
      const float devStartAngle = kStartAngle + devMinVal * kSweep;
      const float devEndAngle = kStartAngle + devMaxVal * kSweep;

      // Semi-transparent accent glow
      g.DrawArc(IColor(50, mAccentColor.R, mAccentColor.G, mAccentColor.B), cx,
                cy, rOuter, devStartAngle, devEndAngle, nullptr, 6.f);
      // Solid deviation strip
      g.DrawArc(IColor(180, mAccentColor.R, mAccentColor.G, mAccentColor.B),
                cx, cy, rOuter, devStartAngle, devEndAngle, nullptr, 3.f);

      // Mod handles
      const float handleRadius =
          3.f + 1.5f * mHoverAnimPhase; // grows on hover
      const float handleStroke = 1.5f + 0.5f * mHoverAnimPhase;

      const float modStartAngle =
          DegToRad(kStartAngle + static_cast<float>(mModStart) * kSweep -
                   90.f);
      const float msX = cx + std::cos(modStartAngle) * rOuter;
      const float msY = cy + std::sin(modStartAngle) * rOuter;
      g.DrawCircle(mAccentColor, msX, msY, handleRadius, nullptr,
                   handleStroke);

      const float modEndAngle =
          DegToRad(kStartAngle + static_cast<float>(mModEnd) * kSweep - 90.f);
      const float meX = cx + std::cos(modEndAngle) * rOuter;
      const float meY = cy + std::sin(modEndAngle) * rOuter;
      g.DrawCircle(mAccentColor, meX, meY, handleRadius, nullptr,
                   handleStroke);
    }

    // Value readout below
    WDL_String valStr;
    GetParam()->GetDisplay(valStr);
    g.DrawText(IText(PolyTheme::FontControl, PolyTheme::TextDark, "Regular",
                     EAlign::Center),
               valStr.Get(), mValueRect);

    // Animate hover phase
    AnimateHoverPhase();
  }

  // ── Interaction ─────────────────────────────────────────────────

  void OnMouseDown(float x, float y, const IMouseMod &mod) override {
    const float cx = mDrawRECT.MW();
    const float cy = mDrawRECT.MH();
    const float halfW = mDrawRECT.W() * 0.5f;
    const float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));

    mActiveDragZone = DragZone::None;

    if (mShowModulation) {
      // Check handle proximity (10px padding)
      const float rOuter = halfW * kOuterTrackRatio;
      const float handlePad = 10.f;

      const float msAngle =
          DegToRad(kStartAngle + static_cast<float>(mModStart) * kSweep -
                   90.f);
      const float msX = cx + std::cos(msAngle) * rOuter;
      const float msY = cy + std::sin(msAngle) * rOuter;
      if (std::sqrt((x - msX) * (x - msX) + (y - msY) * (y - msY)) <
          handlePad) {
        mActiveDragZone = DragZone::HandleStart;
        mDragStartY = y;
        mDragStartModStart = mModStart;
        return;
      }

      const float meAngle =
          DegToRad(kStartAngle + static_cast<float>(mModEnd) * kSweep - 90.f);
      const float meX = cx + std::cos(meAngle) * rOuter;
      const float meY = cy + std::sin(meAngle) * rOuter;
      if (std::sqrt((x - meX) * (x - meX) + (y - meY) * (y - meY)) <
          handlePad) {
        mActiveDragZone = DragZone::HandleEnd;
        mDragStartY = y;
        mDragStartModEnd = mModEnd;
        return;
      }
    }

    // Base knob drag
    const float rBase = halfW * kBaseHitRatio;
    if (dist <= rBase) {
      mActiveDragZone = DragZone::Base;
      mDragStartY = y;
      mDragStartValue = mValue;
    }
  }

  void OnMouseDrag(float x, float y, float dX, float dY,
                   const IMouseMod &mod) override {
    if (mActiveDragZone == DragZone::None)
      return;

    const float sensitivity = 200.f;
    const float fineScale = (mod.S != 0.f) ? 0.1f : 1.0f;
    const double delta =
        static_cast<double>(-(y - mDragStartY) / sensitivity * fineScale);

    switch (mActiveDragZone) {
    case DragZone::Base: {
      mValue = std::clamp(mDragStartValue + delta, 0.0, 1.0);
      break;
    }
    case DragZone::HandleStart:
      mModStart = std::clamp(mDragStartModStart + delta, 0.0, mModEnd);
      break;
    case DragZone::HandleEnd:
      mModEnd = std::clamp(mDragStartModEnd + delta, mModStart, 1.0);
      break;
    default:
      break;
    }

    SetDirty(true);
  }

  void OnMouseDblClick(float x, float y, const IMouseMod &mod) override {
    mValue = GetParam()->GetDefault(true);
    SetDirty(true);
  }

  void OnMouseOver(float x, float y, const IMouseMod &mod) override {
    mIsHovered = true;
    mMouseIsOver = true;
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod &mod) override {
    mActiveDragZone = DragZone::None;
  }

  void OnMouseOut() override {
    mIsHovered = false;
    mMouseIsOver = false;
    SetDirty(false);
  }

  bool IsHit(float x, float y) const override {
    const float cx = mDrawRECT.MW();
    const float cy = mDrawRECT.MH();
    const float halfW = mDrawRECT.W() * 0.5f;
    const float rOuter = halfW * kOuterTrackRatio;
    const float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
    return dist <= rOuter + 5.f; // slight padding beyond outer ring
  }

  void OnResize() override {
    UpdateLayout();
    mCachedLayer = nullptr;
  }

  // ── Accessors for testing ──────────────────────────────────────

  const IRECT &GetDrawRECT() const { return mDrawRECT; }

private:
  // ── Layout constants (ratios of half-width) ────────────────────
  static constexpr float kBaseHitRatio = 0.35f;
  static constexpr float kInnerTrackRatio = 0.32f;
  static constexpr float kDialRatio = 0.25f;
  static constexpr float kOuterTrackRatio = 0.40f;

  // ── Angle constants ────────────────────────────────────────────
  static constexpr float kStartAngle = -135.f;
  static constexpr float kEndAngle = 135.f;
  static constexpr float kSweep = 270.f;

  // ── Drag zones ─────────────────────────────────────────────────
  enum class DragZone { None, Base, HandleStart, HandleEnd };

  void UpdateLayout() {
    mLabelRect = mRECT.GetFromTop(PolyTheme::LabelH);
    mValueRect = mRECT.GetFromBottom(PolyTheme::LabelH);

    // Square draw area centered in remaining space, scaled by mScale
    const IRECT inner = IRECT(mRECT.L, mLabelRect.B, mRECT.R, mValueRect.T);
    const float side = std::min(inner.W(), inner.H()) * mScale;
    const float cx = inner.MW();
    const float cy = inner.MH();
    mDrawRECT =
        IRECT(cx - side * 0.5f, cy - side * 0.5f, cx + side * 0.5f,
              cy + side * 0.5f);
  }

  void AnimateHoverPhase() {
    const float target = mIsHovered ? 1.f : 0.f;
    const float speed = 0.15f;
    if (std::abs(mHoverAnimPhase - target) > 0.01f) {
      mHoverAnimPhase += (target - mHoverAnimPhase) * speed;
      SetDirty(false);
    } else {
      mHoverAnimPhase = target;
    }
  }

  // ── State ──────────────────────────────────────────────────────
  double mValue = 0.5;
  double mModStart = 0.4;
  double mModEnd = 0.6;
  double mModCurrent = 0.5;
  bool mShowModulation = false;

  DragZone mActiveDragZone = DragZone::None;
  float mDragStartY = 0.f;
  double mDragStartValue = 0.0;
  double mDragStartModStart = 0.0;
  double mDragStartModEnd = 0.0;

  bool mIsHovered = false;
  float mHoverAnimPhase = 0.f;

  IColor mAccentColor = PolyTheme::KnobRingOn;
  WDL_String mLabelStr;
  float mScale = 1.0f;

  IRECT mLabelRect;
  IRECT mValueRect;
  IRECT mDrawRECT;
  ILayerPtr mCachedLayer = nullptr;
};
