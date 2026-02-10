#pragma once

#include "IControl.h"
#include "IGraphics.h"
#include <cmath>
#include <algorithm>

using namespace iplug;
using namespace igraphics;

/**
 * @brief A vertical fader/slider control matching the React Fader.jsx implementation
 *
 * Features:
 * - Vertical track
 * - Movable handle/thumb
 * - Value display from 0.0 to 1.0
 * - Vertical drag interaction
 */
class Fader : public ISliderControlBase
{
public:
  Fader(const IRECT& bounds, int paramIdx, const char* label = "")
    : ISliderControlBase(bounds, paramIdx, EDirection::Vertical, DEFAULT_GEARING)
    , mTrackColor(COLOR_BLACK.WithOpacity(0.2f))
    , mHandleColor(COLOR_BLUE)
    , mHandleHoverColor(COLOR_BLUE.WithContrast(0.1f))
  {
    if (label && label[0])
      SetTooltip(label);
  }

  void Draw(IGraphics& g) override
  {
    // Get the value (0.0 to 1.0)
    double value = GetValue();

    // Calculate dimensions
    float trackWidth = mRECT.W() * 0.3f;
    float trackX = mRECT.L + (mRECT.W() - trackWidth) * 0.5f;
    float trackY = mRECT.T + 10.0f;
    float trackHeight = mRECT.H() - 20.0f;

    // Draw track (background)
    IRECT trackRect(trackX, trackY, trackX + trackWidth, trackY + trackHeight);
    g.FillRoundRect(mTrackColor, trackRect, 3.0f);

    // Calculate handle position (inverted: 0 = bottom, 1 = top)
    float handleHeight = 15.0f;
    float handleWidth = mRECT.W() * 0.6f;
    float handleX = mRECT.L + (mRECT.W() - handleWidth) * 0.5f;
    float handleY = trackY + trackHeight - (static_cast<float>(value) * trackHeight) - (handleHeight * 0.5f);

    // Clamp handle position
    handleY = std::max(trackY - handleHeight * 0.5f, std::min(handleY, trackY + trackHeight - handleHeight * 0.5f));

    // Draw handle
    IRECT handleRect(handleX, handleY, handleX + handleWidth, handleY + handleHeight);
    IColor handleColor = mMouseIsOver ? mHandleHoverColor : mHandleColor;
    g.FillRoundRect(handleColor, handleRect, 3.0f);

    // Draw handle border
    g.DrawRoundRect(COLOR_WHITE.WithOpacity(0.5f), handleRect, 3.0f, nullptr, 1.5f);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    mMouseIsOver = true;
    ISliderControlBase::OnMouseOver(x, y, mod);
    SetDirty(false);
  }

  void OnMouseOut() override
  {
    mMouseIsOver = false;
    ISliderControlBase::OnMouseOut();
    SetDirty(false);
  }

  void SetColors(const IColor& track, const IColor& handle)
  {
    mTrackColor = track;
    mHandleColor = handle;
    mHandleHoverColor = handle.WithContrast(0.1f);
  }

protected:
  IColor mTrackColor;
  IColor mHandleColor;
  IColor mHandleHoverColor;
  bool mMouseIsOver = false;
};
