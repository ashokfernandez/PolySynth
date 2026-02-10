#pragma once

#include "IControl.h"
#include "IGraphics.h"
#include <cmath>

using namespace iplug;
using namespace igraphics;

/**
 * @brief A high-quality vector-based knob control
 *
 * Features:
 * - Rotary control from -135° to +135°
 * - Smooth anti-aliased arcs
 * - Professional appearance with proper sizing
 * - Vertical drag interaction
 */
class Knob : public IKnobControlBase
{
public:
  Knob(const IRECT& bounds, int paramIdx, const char* label = "")
    : IKnobControlBase(bounds, paramIdx, EDirection::Vertical, DEFAULT_GEARING * 0.5) // Reduce sensitivity
    , mTrackColor(COLOR_MID_GRAY.WithOpacity(0.3f))
    , mValueColor(IColor(255, 0, 150, 255)) // Blue-purple accent
    , mPointerColor(COLOR_WHITE)
  {
    if (label && label[0])
      SetTooltip(label);
  }

  void Draw(IGraphics& g) override
  {
    double value = GetValue();

    // Calculate center and radius - use most of the available space
    float cx = mRECT.MW();
    float cy = mRECT.MH();
    float size = std::min(mRECT.W(), mRECT.H());
    float radius = size * 0.42f;
    float trackWidth = radius * 0.12f;

    // Angle range: -135° to +135° (270° total sweep)
    const float minAngleDeg = -135.0f;
    const float maxAngleDeg = 135.0f;
    const float minAngle = minAngleDeg * static_cast<float>(M_PI) / 180.0f;
    const float maxAngle = maxAngleDeg * static_cast<float>(M_PI) / 180.0f;
    const float currentAngle = minAngle + (static_cast<float>(value) * (maxAngle - minAngle));

    // Draw outer ring/shadow for depth
    g.FillCircle(COLOR_BLACK.WithOpacity(0.15f), cx + 1, cy + 2, radius + 4);

    // Draw background circle
    g.FillCircle(COLOR_DARK_GRAY, cx, cy, radius);

    // Draw background track (full arc) - slightly inset
    float arcRadius = radius - (trackWidth * 0.5f);
    g.PathClear();
    g.PathArc(cx, cy, arcRadius, minAngle, maxAngle);
    IStrokeOptions trackOpts;
    trackOpts.mCapOption = ELineCap::Round;
    g.PathStroke(mTrackColor, trackWidth, trackOpts);

    // Draw active value arc (from min to current position)
    if (value > 0.01)
    {
      g.PathClear();
      g.PathArc(cx, cy, arcRadius, minAngle, currentAngle);
      IStrokeOptions valueOpts;
      valueOpts.mCapOption = ELineCap::Round;
      g.PathStroke(mValueColor, trackWidth, valueOpts);
    }

    // Draw pointer indicator - a bold line from center outward
    float pointerLength = radius * 0.65f;
    float pointerStartRadius = radius * 0.15f;
    float pointerX1 = cx + std::cos(currentAngle) * pointerStartRadius;
    float pointerY1 = cy + std::sin(currentAngle) * pointerStartRadius;
    float pointerX2 = cx + std::cos(currentAngle) * pointerLength;
    float pointerY2 = cy + std::sin(currentAngle) * pointerLength;

    g.PathClear();
    g.PathMoveTo(pointerX1, pointerY1);
    g.PathLineTo(pointerX2, pointerY2);
    IStrokeOptions pointerOpts;
    pointerOpts.mCapOption = ELineCap::Round;
    g.PathStroke(mPointerColor, trackWidth * 1.2f, pointerOpts);

    // Draw center cap
    g.FillCircle(COLOR_DARK_GRAY, cx, cy, radius * 0.18f);
    g.FillCircle(mPointerColor.WithOpacity(0.8f), cx, cy, radius * 0.12f);
  }

  void SetColors(const IColor& track, const IColor& value, const IColor& pointer)
  {
    mTrackColor = track;
    mValueColor = value;
    mPointerColor = pointer;
  }

protected:
  IColor mTrackColor;
  IColor mValueColor;
  IColor mPointerColor;
};
