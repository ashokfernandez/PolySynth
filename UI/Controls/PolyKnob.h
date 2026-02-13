#pragma once

#include "IControl.h"
#include <cmath>
#include "PolyTheme.h"

class PolyKnob : public IKnobControlBase {
public:
  PolyKnob(const IRECT& bounds, int paramIdx, const char* label)
      : IKnobControlBase(bounds, paramIdx), mLabelStr(label) {
    mTooltip.Set("");
    UpdateLayout();
  }

  void SetAccent(const IColor& color) {
    mAccentColor = color;
    SetDirty(false);
  }

  void Draw(IGraphics& g) override {
    g.DrawText(IText(12.f, PolyTheme::TextDark, "Roboto-Bold", EAlign::Center),
               mLabelStr.Get(), mLabelRect);

    const float angle = -135.f + (GetValue() * 270.f);
    const float radius = mKnobRect.W() * 0.5f;
    const float cx = mKnobRect.MW();
    const float cy = mKnobRect.MH();

    // Soft depth/shadow under the knob body
    g.FillCircle(IColor(45, 0, 0, 0), cx + 1.f, cy + 2.f, radius - 1.f);

    // Knob body with subtle bezel + highlight for Skia crispness
    g.FillCircle(IColor(255, 247, 245, 240), cx, cy, radius - 1.5f);
    g.DrawCircle(IColor(60, 255, 255, 255), cx, cy - 0.5f, radius - 2.0f, nullptr,
                 1.f);
    g.DrawCircle(IColor(35, 0, 0, 0), cx, cy + 1.f, radius - 2.0f, nullptr, 1.f);

    // Track and active arc + soft glow pass
    g.DrawArc(PolyTheme::KnobRingOff, cx, cy, radius, -135.f, 135.f, 0, 3.f);
    g.DrawArc(IColor(70, mAccentColor.R, mAccentColor.G, mAccentColor.B), cx, cy,
              radius, -135.f, angle, 0, 6.f);
    g.DrawArc(mAccentColor, cx, cy, radius, -135.f, angle, 0, 3.f);

    const float rad = DegToRad(angle - 90.f);
    const float x2 = cx + std::cos(rad) * radius;
    const float y2 = cy + std::sin(rad) * radius;
    g.DrawLine(mAccentColor, cx, cy, x2, y2, nullptr, 2.f);
    g.FillCircle(PolyTheme::Highlight, x2, y2, 1.8f);

    WDL_String valStr;
    if (GetParam())
      GetParam()->GetDisplay(valStr);

    g.DrawText(IText(12.f, PolyTheme::TextDark, "Roboto-Regular", EAlign::Center),
               valStr.Get(), mValueRect);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnResize() override { UpdateLayout(); }

private:
  void UpdateLayout() {
    mLabelRect = mRECT.GetFromTop(18.f);
    mValueRect = mRECT.GetFromBottom(18.f);
    mKnobRect = mRECT.GetPadded(-5.f).GetMidVPadded(mRECT.W() * 0.4f);
  }

  WDL_String mLabelStr;
  IColor mAccentColor = PolyTheme::KnobRingOn;
  IRECT mLabelRect;
  IRECT mKnobRect;
  IRECT mValueRect;
};
