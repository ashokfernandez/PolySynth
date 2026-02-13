#pragma once

#include "IControl.h"
#include "PolyTheme.h"

class PolySection : public IControl {
public:
  PolySection(const IRECT& bounds, const char* title)
      : IControl(bounds), mTitle(title) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override {
    if (!mCachedLayer) {
      g.StartLayer(this, mRECT);

      // Soft drop shadow + main panel shape
      g.FillRoundRect(IColor(35, 0, 0, 0), mRECT.GetTranslated(0.f, 2.f), 6.f);
      g.FillRoundRect(PolyTheme::PanelBG, mRECT, 5.f);

      // Subtle top sheen and scanline texture for a more premium surface
      const IRECT sheenRect = mRECT.GetFromTop(mRECT.H() * 0.22f);
      g.FillRoundRect(PolyTheme::Highlight, sheenRect, 5.f);

      for (float y = mRECT.T + 4.f; y < mRECT.B - 4.f; y += 4.f) {
        g.DrawLine(IColor(7, 255, 255, 255), mRECT.L + 5.f, y, mRECT.R - 5.f, y,
                   nullptr, 0.5f);
      }

      // Border stack: outer + inner edge for depth
      g.DrawRoundRect(PolyTheme::SectionBorder, mRECT, 5.f, nullptr, 1.5f);
      g.DrawRoundRect(IColor(40, 255, 255, 255), mRECT.GetPadded(-1.f), 4.f,
                      nullptr, 1.f);

      const IRECT titleRect(mRECT.L + 12.f, mRECT.T + 10.f, mRECT.R - 12.f,
                            mRECT.T + 32.f);
      g.DrawText(IText(14.f, PolyTheme::TextDark, "Roboto-Bold", EAlign::Near),
                 mTitle.Get(), titleRect);

      mCachedLayer = g.EndLayer();
    }

    g.DrawLayer(mCachedLayer);
  }

private:
  WDL_String mTitle;
  ILayerPtr mCachedLayer;
};
