#pragma once
#include "IControl.h"

class SectionFrame final : public IControl {
public:
  SectionFrame(const IRECT &bounds, const char *title,
               const IColor &borderColor, const IColor &textColor,
               const IColor &bgColor = COLOR_TRANSPARENT)
      : IControl(bounds), mTitle(title), mBorderColor(borderColor),
        mTextColor(textColor), mBgColor(bgColor) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    if (mBgColor.A > 0)
      g.FillRect(mBgColor, mRECT);
    g.DrawRect(mBorderColor, mRECT, nullptr, 1.25f);
    if (mTitle.GetLength()) {
      // Larger, bolder section headers, well-offset from corners
      g.DrawText(
          IText(18.f, mTextColor, "Bold", EAlign::Near), mTitle.Get(),
          mRECT.GetPadded(-12.f).GetFromTop(24.f).GetTranslated(4.f, 4.f));
    }
  }

private:
  WDL_String mTitle;
  IColor mBorderColor;
  IColor mTextColor;
  IColor mBgColor;
};
