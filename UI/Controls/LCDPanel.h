#pragma once
#include "IControl.h"

class LCDPanel final : public IControl {
public:
  LCDPanel(const IRECT &bounds, const IColor &bgColor)
      : IControl(bounds), mBgColor(bgColor) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    // Fill background
    g.FillRect(mBgColor, mRECT);
    // Inner shadow style border
    g.DrawRect(IColor(50, 0, 0, 0), mRECT, nullptr, 2.f);
  }

private:
  IColor mBgColor;
};
