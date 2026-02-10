#pragma once

#include "IControl.h"
#include "IGraphics.h"

using namespace iplug;
using namespace igraphics;

/**
 * @brief ADSR Envelope visualizer matching the React Envelope.jsx implementation
 *
 * Features:
 * - Visual representation of Attack, Decay, Sustain, Release
 * - Filled area with semi-transparent color
 * - Stroked outline
 * - Non-interactive (display only)
 */
class Envelope : public IControl
{
public:
  Envelope(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
    : IControl(bounds)
    , mAttack(0.2f)
    , mDecay(0.3f)
    , mSustain(0.7f)
    , mRelease(0.4f)
    , mStrokeColor(COLOR_BLUE)
    , mFillColor(COLOR_BLUE.WithOpacity(0.2f))
  {
    mIgnoreMouse = true; // This is a display-only control
  }

  void Draw(IGraphics& g) override
  {
    // Get drawing area
    float w = mRECT.W();
    float h = mRECT.H();
    float left = mRECT.L;
    float top = mRECT.T;

    // Ensure minimum values for visibility (matching React code)
    float a = std::max(0.01f, mAttack);
    float d = std::max(0.01f, mDecay);
    float s = mSustain;
    float r = std::max(0.01f, mRelease);

    // Calculate X positions (matching React logic)
    // Each parameter gets ~25% of width
    float attackX = w * (a * 0.25f);
    float decayX = attackX + (w * (d * 0.25f));
    float releaseX = w - (w * (r * 0.25f));

    // Ensure release starts after decay with some sustain section
    float sustainEndX = std::max(decayX, releaseX - (w * 0.1f));

    // Calculate Y position for sustain level (inverted: 0 = top, 1 = bottom)
    float sustainLevel = h - (s * h);

    // Build the envelope path
    g.PathClear();
    g.PathMoveTo(left, top + h);              // Start at bottom-left
    g.PathLineTo(left + attackX, top);        // Attack: rise to peak
    g.PathLineTo(left + decayX, top + sustainLevel);  // Decay: fall to sustain
    g.PathLineTo(left + releaseX, top + sustainLevel); // Sustain: hold level
    g.PathLineTo(left + w, top + h);          // Release: fall to zero

    // Stroke the outline
    g.PathStroke(mStrokeColor, 2.0f);

    // Fill the area
    g.PathClear();
    g.PathMoveTo(left, top + h);
    g.PathLineTo(left + attackX, top);
    g.PathLineTo(left + decayX, top + sustainLevel);
    g.PathLineTo(left + releaseX, top + sustainLevel);
    g.PathLineTo(left + w, top + h);
    g.PathLineTo(left, top + h); // Close the path
    g.PathFill(mFillColor);
  }

  /**
   * @brief Set the ADSR values
   * @param attack Attack time (0.0 to 1.0)
   * @param decay Decay time (0.0 to 1.0)
   * @param sustain Sustain level (0.0 to 1.0)
   * @param release Release time (0.0 to 1.0)
   */
  void SetADSR(float attack, float decay, float sustain, float release)
  {
    mAttack = std::max(0.0f, std::min(1.0f, attack));
    mDecay = std::max(0.0f, std::min(1.0f, decay));
    mSustain = std::max(0.0f, std::min(1.0f, sustain));
    mRelease = std::max(0.0f, std::min(1.0f, release));
    SetDirty(false);
  }

  void SetColors(const IColor& stroke, const IColor& fill)
  {
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
