#pragma once
#include "IGraphicsStructs.h"
#include <vector>

// Minimal PolyTheme stub so UI controls compile in the test environment.
namespace PolyTheme {
  static const IColor LCDBackground{255, 20, 20, 20};
  static const IColor InnerBorder{100, 80, 80, 80};
  static constexpr float RoundingButton = 4.f;
}

class IGraphics {
public:
  struct DrawOp {
    IColor c;
    IRECT r;
  };
  std::vector<DrawOp> fillRectCalls;
  std::vector<DrawOp> drawRectCalls;

  void FillRect(const IColor &color, const IRECT &bounds) {
    fillRectCalls.push_back({color, bounds});
  }
  void DrawRect(const IColor &color, const IRECT &bounds, const void *blend,
                float thick) {
    drawRectCalls.push_back({color, bounds});
  }
  // Round-rect variants used by LCDPanel / SectionFrame
  void FillRoundRect(const IColor &color, const IRECT &bounds, float radius = 0.f) {
    fillRectCalls.push_back({color, bounds});
  }
  void DrawRoundRect(const IColor &color, const IRECT &bounds, float radius = 0.f,
                     const void *blend = nullptr, float thick = 1.f) {
    drawRectCalls.push_back({color, bounds});
  }
  void DrawText(const IText &text, const char *str, const IRECT &bounds) {
    // mock logic
  }
};

class IControl {
public:
  IControl(const IRECT &bounds) : mRECT(bounds) {}
  virtual ~IControl() {}
  virtual void Draw(IGraphics &g) = 0;

  IRECT mRECT;
  bool mIgnoreMouse = false;
};
