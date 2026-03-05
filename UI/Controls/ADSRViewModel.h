#pragma once

#include "IControl.h"
#include <algorithm>
#include <cmath>

using namespace iplug;
using namespace igraphics;

struct ADSRPathData {
  struct Point {
    float x = 0.f;
    float y = 0.f;
  };
  Point attackNode;
  Point attackControlPoint;
  Point decaySustainNode;
  Point decayControlPoint;
  Point releaseStartNode;
  Point releaseNode;
  Point releaseControlPoint;
};

class ADSRViewModel {
public:
  enum class Theme { Base, Dark, Faint, Fill, Glow };

  struct ColorHSL {
    float h;
    float s;
    float l;
  };

  ADSRViewModel()
      : mBounds{0, 0, 0, 0}, mHue(182.0f), mSat(1.0f), mLight(0.42f) {
    SetParams(0.2f, 0.2f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f);
  }

  void SetBounds(float x, float y, float w, float h) {
    mBounds.L = x;
    mBounds.T = y;
    mBounds.R = x + w;
    mBounds.B = y + h;
  }

  void SetParams(float a, float d, float s, float r, float at, float dt,
                 float rt) {
    mAttack = std::max(0.0f, std::min(1.0f, a));
    mDecay = std::max(0.0f, std::min(1.0f, d));
    mSustain = std::max(0.0f, std::min(1.0f, s));
    mRelease = std::max(0.0f, std::min(1.0f, r));
    mAtkTension = std::max(-1.0f, std::min(1.0f, at));
    mDecTension = std::max(-1.0f, std::min(1.0f, dt));
    mRelTension = std::max(-1.0f, std::min(1.0f, rt));
  }

  ADSRPathData CalculatePathData() const {
    ADSRPathData data;
    float w = mBounds.W();
    float h = mBounds.H();

    if (w == 0.0f || h == 0.0f) {
      data.attackNode = {0, mBounds.B};
      data.decaySustainNode = {0, mBounds.B};
      data.releaseNode = {0, mBounds.B};
      return data;
    }

    // Proportional time scaling: A + D + R fill the display proportionally,
    // with a fixed 20% slot reserved for the sustain plateau.
    float T_total   = std::max(0.001f, mAttack + mDecay + mRelease);
    float T_display = T_total * 1.25f;   // sustain slot = 25% of T_total = 20% of display
    float scale     = w / T_display;

    float attackX       = mBounds.L + mAttack              * scale;
    float decayX        = attackX   + mDecay               * scale;
    float releaseStartX = decayX    + T_total * 0.25f      * scale;  // always 20% of width
    float releaseX      = mBounds.R;                                  // always fills right edge

    float attackY = mBounds.T;
    float sustainY = mBounds.B - (mSustain * h);

    data.attackNode = {attackX, attackY};
    data.decaySustainNode = {decayX, sustainY};
    data.releaseStartNode = {releaseStartX, sustainY};
    data.releaseNode = {releaseX, mBounds.B};

    // Attack: segment from (L, B) to (attackX, T).
    // Control point at segment midpoint when tension=0 → straight line.
    // +tension pulls toward bottom-right (exponential rise), -tension toward top-left.
    float atkMidX = (mBounds.L + attackX) * 0.5f;
    float atkMidY = (mBounds.B + attackY) * 0.5f;
    data.attackControlPoint = {
        atkMidX + mAtkTension * (attackX - atkMidX),
        atkMidY + mAtkTension * (mBounds.B - atkMidY)};

    // Decay: segment from (attackX, T) to (decayX, sustainY).
    // Control point at midpoint when tension=0, +tension pulls toward top-right (convex arc).
    data.decayControlPoint = {attackX + (decayX - attackX) * 0.5f,
                              attackY + (sustainY - attackY) *
                                            (0.5f + mDecTension * 0.5f)};

    // Release: segment from (releaseStartX, sustainY) to (releaseX, B).
    // Control point at midpoint when tension=0, +tension pulls toward bottom-left (lingers).
    float relMidX = (releaseStartX + releaseX) * 0.5f;
    float relMidY = (sustainY + mBounds.B) * 0.5f;
    data.releaseControlPoint = {
        relMidX + mRelTension * (releaseStartX - relMidX),
        relMidY + mRelTension * (mBounds.B - relMidY)};

    return data;
  }

  void SetBaseColorHSL(float h, float s, float l) {
    mHue = h;
    mSat = s;
    mLight = l;
  }

  IColor GetColor(Theme theme) const {
    float l = mLight;
    float a = 1.0f;

    switch (theme) {
    case Theme::Base:
      break;
    case Theme::Dark:
      l -= 0.25f;
      break;
    case Theme::Faint:
      a = 0.12f;
      break;
    case Theme::Fill:
      a = 0.08f;
      break;
    case Theme::Glow:
      a = 0.18f;
      break;
    }

    l = std::max(0.0f, std::min(1.0f, l));

    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * mSat;
    float x = c * (1.0f - std::abs(std::fmod(mHue / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r = 0, g = 0, b = 0;
    if (0 <= mHue && mHue < 60) {
      r = c;
      g = x;
      b = 0;
    } else if (60 <= mHue && mHue < 120) {
      r = x;
      g = c;
      b = 0;
    } else if (120 <= mHue && mHue < 180) {
      r = 0;
      g = c;
      b = x;
    } else if (180 <= mHue && mHue < 240) {
      r = 0;
      g = x;
      b = c;
    } else if (240 <= mHue && mHue < 300) {
      r = x;
      g = 0;
      b = c;
    } else if (300 <= mHue && mHue < 360) {
      r = c;
      g = 0;
      b = x;
    }

    return IColor(static_cast<int>(a * 255), static_cast<int>((r + m) * 255),
                  static_cast<int>((g + m) * 255),
                  static_cast<int>((b + m) * 255));
  }

  float GetAttack()  const { return mAttack; }
  float GetDecay()   const { return mDecay; }
  float GetSustain() const { return mSustain; }
  float GetRelease() const { return mRelease; }

  float GetPlayheadX(float phase) const {
    float p = std::max(0.0f, std::min(1.0f, phase));
    return mBounds.L + p * mBounds.W();
  }

private:
  IRECT mBounds;
  float mAttack, mDecay, mSustain, mRelease;
  float mAtkTension, mDecTension, mRelTension;
  float mHue, mSat, mLight;
};
