#pragma once

#include "IControl.h"
#include "IGraphics.h"
#include "PolyTheme.h"

using namespace iplug;
using namespace igraphics;

/**
 * @brief ADSR Envelope visualizer matching the React Envelope.jsx
 * implementation
 *
 * Features:
 * - Visual representation of Attack, Decay, Sustain, Release
 * - Filled area with semi-transparent color
 * - Stroked outline
 * - Non-interactive (display only)
 */
class Envelope : public IControl {
public:
  Envelope(const IRECT &bounds, const IVStyle &style = DEFAULT_STYLE)
      : IControl(bounds), mAttack(0.2f), mDecay(0.3f), mSustain(0.7f),
        mRelease(0.4f), mStrokeColor(COLOR_BLUE),
        mFillColor(COLOR_BLUE.WithOpacity(0.2f)) {
    mIgnoreMouse = true; // This is a display-only control
  }

  void Draw(IGraphics &g) override {
    // Draw background panel for the envelope area
    g.FillRoundRect(PolyTheme::ControlBG, mRECT, PolyTheme::RoundingSection);
    g.DrawRoundRect(PolyTheme::SectionBorder, mRECT, PolyTheme::RoundingSection,
                    nullptr, 1.f);

    // Get drawing area (padded slightly)
    IRECT drawRect = mRECT.GetPadded(-10.f);
    float w = drawRect.W();
    float h = drawRect.H();
    float left = drawRect.L;
    float top = drawRect.T;

    // Ensure minimum values for visibility
    float a = std::max(0.01f, mAttack);
    float d = std::max(0.01f, mDecay);
    float s = mSustain;
    float r = std::max(0.01f, mRelease);

    // Calculate X positions
    float attackX = w * (a * 0.25f);
    float decayX = attackX + (w * (d * 0.25f));
    float releaseX = w - (w * (r * 0.25f));
    float sustainEndX = std::max(decayX, releaseX - (w * 0.1f));

    // Calculate Y position for sustain level
    float sustainLevel = h - (s * h);

    // Path for the fill (matching PolyTheme accents)
    g.PathClear();
    g.PathMoveTo(left, top + h);
    g.PathLineTo(left + attackX, top);
    g.PathLineTo(left + decayX, top + sustainLevel);
    g.PathLineTo(left + sustainEndX, top + sustainLevel);
    g.PathLineTo(left + w, top + h);
    g.PathLineTo(left, top + h);
    g.PathFill(mFillColor);

    // Path for the stroke
    g.PathClear();
    g.PathMoveTo(left, top + h);
    g.PathLineTo(left + attackX, top);
    g.PathLineTo(left + decayX, top + sustainLevel);
    g.PathLineTo(left + sustainEndX, top + sustainLevel);
    g.PathLineTo(left + w, top + h);
    g.PathStroke(mStrokeColor, 2.0f);
  }

  /**
   * @brief Set the ADSR values
   * @param attack Attack time (0.0 to 1.0)
   * @param decay Decay time (0.0 to 1.0)
   * @param sustain Sustain level (0.0 to 1.0)
   * @param release Release time (0.0 to 1.0)
   */
  void SetADSR(float attack, float decay, float sustain, float release) {
    mAttack = std::max(0.0f, std::min(1.0f, attack));
    mDecay = std::max(0.0f, std::min(1.0f, decay));
    mSustain = std::max(0.0f, std::min(1.0f, sustain));
    mRelease = std::max(0.0f, std::min(1.0f, release));
    SetDirty(false);
  }

  void SetColors(const IColor &stroke, const IColor &fill) {
    mStrokeColor = stroke;
    mFillColor = fill;
    SetDirty(false);
  }

protected:
  float mAttack;
  float mDecay;
  float mSustain;
  float mRelease;
  IColor mStrokeColor;
  IColor mFillColor;
};
