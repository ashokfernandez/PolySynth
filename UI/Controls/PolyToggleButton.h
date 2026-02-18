#pragma once

#include "IControl.h"
#include "PolyTheme.h"

class PolyToggleButton : public ISwitchControlBase {
public:
  PolyToggleButton(const IRECT &bounds, int paramIdx, const char *label)
      : ISwitchControlBase(bounds, paramIdx, nullptr, 2), mLabel(label) {}

  void Draw(IGraphics &g) override {
    const bool active = GetValue() > 0.5;
    const float rounding = mRECT.H() * 0.3f;

    const IColor bg =
        active ? PolyTheme::ToggleActiveBG : PolyTheme::ToggleInactiveBG;
    const IColor fg = active ? PolyTheme::ToggleActiveFG : PolyTheme::TextDark;

    g.FillRoundRect(bg, mRECT, rounding);

    if (!active)
      g.DrawRoundRect(PolyTheme::SectionBorder, mRECT, rounding, nullptr, 1.f);

    if (mMouseIsOver)
      g.FillRoundRect(PolyTheme::Highlight, mRECT, rounding);

    g.DrawText(IText(PolyTheme::FontControl, fg, "Bold", EAlign::Center),
               mLabel.Get(), mRECT);
  }

private:
  WDL_String mLabel;
};
