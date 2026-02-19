#pragma once

#include "IControl.h"
#include "PolyTheme.h"

class PolySection : public IControl {
public:
  PolySection(const IRECT &bounds, const char *title)
      : IControl(bounds), mTitle(title) {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics &g) override {
    if (!mCachedLayer) {
      g.StartLayer(this, mRECT);

      // Soft drop shadow + main panel shape
      g.FillRoundRect(PolyTheme::ShadowLight, mRECT.GetTranslated(0.f, 2.f),
                      PolyTheme::RoundingSection + 1.f);
      g.FillRoundRect(PolyTheme::PanelBG, mRECT, PolyTheme::RoundingSection);

      // Subtle top sheen
      const IRECT sheenRect = mRECT.GetFromTop(mRECT.H() * 0.22f);
      g.FillRoundRect(PolyTheme::Highlight, sheenRect,
                      PolyTheme::RoundingSection);

      // Subtler scanline texture (increased spacing and reduced opacity/width)
      for (float y = mRECT.T + 8.f; y < mRECT.B - 8.f; y += 8.f) {
        g.DrawLine(PolyTheme::Scanline.WithOpacity(0.05f), mRECT.L + 8.f, y,
                   mRECT.R - 8.f, y, nullptr, 0.25f);
      }

      // Border stack: outer + inner edge for depth
      g.DrawRoundRect(PolyTheme::SectionBorder, mRECT,
                      PolyTheme::RoundingSection, nullptr, 1.25f);
      g.DrawRoundRect(PolyTheme::InnerBorder, mRECT.GetPadded(-1.f),
                      PolyTheme::RoundingSection - 1.f, nullptr, 1.f);

      const IRECT titleRect(mRECT.L + 12.f, mRECT.T + 10.f, mRECT.R - 12.f,
                            mRECT.T + 32.f);
      g.DrawText(IText(PolyTheme::FontSectionHead, PolyTheme::TextDark, "Bold",
                       EAlign::Near),
                 mTitle.Get(), titleRect);

      mCachedLayer = g.EndLayer();
    }

    g.DrawLayer(mCachedLayer);
  }

  void OnResize() override { mCachedLayer = nullptr; }

private:
  WDL_String mTitle;
  ILayerPtr mCachedLayer;
};
