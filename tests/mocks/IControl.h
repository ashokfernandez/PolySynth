#pragma once
#include "IGraphicsStructs.h"
#include <vector>

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
