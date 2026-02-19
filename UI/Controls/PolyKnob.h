#pragma once

#include "IControl.h"
#include "PolyTheme.h"
#include <cmath>

class PolyKnob : public IKnobControlBase {
public:
  PolyKnob(const IRECT &bounds, int paramIdx, const char *label)
      : IKnobControlBase(bounds, paramIdx), mLabelStr(label) {
    mTooltip.Set("");
    UpdateLayout();
  }

  void SetAccent(const IColor &color) {
    mAccentColor = color;
    SetDirty(false);
  }

  PolyKnob &WithShowLabel(bool show) {
    mShowLabel = show;
    return *this;
  }
  PolyKnob &WithShowValue(bool show) {
    mShowValue = show;
    return *this;
  }

  void Draw(IGraphics &g) override {
    if (mShowLabel)
      g.DrawText(IText(PolyTheme::FontControl, PolyTheme::TextDark, "Bold",
                       EAlign::Center),
                 mLabelStr.Get(), mLabelRect);

    const float angle = -135.f + (GetValue() * 270.f);
    const float radius = mKnobRect.W() * 0.5f;
    const float cx = mKnobRect.MW();
    const float cy = mKnobRect.MH();

    // Soft depth/shadow under the knob body
    g.FillCircle(PolyTheme::ShadowDark, cx + 1.f, cy + 2.f, radius - 1.f);

    // Knob body with subtle bezel + highlight for Skia crispness
    g.FillCircle(PolyTheme::KnobBody, cx, cy, radius - 1.5f);
    g.DrawCircle(PolyTheme::InnerGlow, cx, cy - 0.5f, radius - 2.0f, nullptr,
                 1.f);
    g.DrawCircle(PolyTheme::ShadowLight, cx, cy + 1.f, radius - 2.0f, nullptr,
                 1.f);

    // Track and active arc + soft glow pass
    g.DrawArc(PolyTheme::KnobRingOff, cx, cy, radius, -135.f, 135.f, 0, 3.f);
    g.DrawArc(IColor(70, mAccentColor.R, mAccentColor.G, mAccentColor.B), cx,
              cy, radius, -135.f, angle, 0, 6.f);
    g.DrawArc(mAccentColor, cx, cy, radius, -135.f, angle, 0, 3.f);

    const float rad = DegToRad(angle - 90.f);
    const float x2 = cx + std::cos(rad) * radius;
    const float y2 = cy + std::sin(rad) * radius;
    g.DrawLine(mAccentColor, cx, cy, x2, y2, nullptr, 2.f);
    g.FillCircle(PolyTheme::Highlight, x2, y2, 1.8f);

    if (mShowValue) {
      WDL_String valStr;
      if (GetParam())
        GetParam()->GetDisplay(valStr);
      g.DrawText(IText(PolyTheme::FontControl, PolyTheme::TextDark,
                       "Roboto-Regular", EAlign::Center),
                 valStr.Get(), mValueRect);
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod &mod) override {
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnResize() override { UpdateLayout(); }

private:
  void UpdateLayout() {
    mLabelRect = mRECT.GetFromTop(PolyTheme::LabelH);
    mValueRect = mRECT.GetFromBottom(PolyTheme::LabelH);
    mKnobRect = mRECT.GetPadded(-5.f).GetMidVPadded(mRECT.W() * 0.4f);
  }

  WDL_String mLabelStr;
  IColor mAccentColor = PolyTheme::KnobRingOn;
  bool mShowLabel = true;
  bool mShowValue = true;
  IRECT mLabelRect;
  IRECT mKnobRect;
  IRECT mValueRect;
};
