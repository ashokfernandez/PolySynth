#pragma once
#include "IControl.h"
#include "PolyTheme.h"

class LCDPanel final : public IControl {
public:
  LCDPanel(const IRECT &bounds, const IColor &bgColor)
      : IControl(bounds), mBgColor(bgColor) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    // Fill background with theme color
    g.FillRoundRect(mBgColor, mRECT, PolyTheme::RoundingButton);

    // Bezel effect: Inner shadow for depth
    g.DrawRoundRect(IColor(80, 0, 0, 0), mRECT, PolyTheme::RoundingButton,
                    nullptr, 2.f);

    // Soft outer highlights for a glassy feel
    g.DrawRoundRect(PolyTheme::InnerBorder, mRECT.GetPadded(-1.f),
                    PolyTheme::RoundingButton - 1.f, nullptr, 1.f);
  }

private:
  IColor mBgColor;
};
