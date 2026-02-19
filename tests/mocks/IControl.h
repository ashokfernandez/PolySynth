#pragma once
#include "IGraphicsStructs.h"
#include <functional>
#include <memory>
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
  // Round-rect variants used by LCDPanel / SectionFrame
  void FillRoundRect(const IColor &color, const IRECT &bounds,
                     float radius = 0.f) {
    fillRectCalls.push_back({color, bounds});
  }
  void DrawRoundRect(const IColor &color, const IRECT &bounds,
                     float radius = 0.f, const void *blend = nullptr,
                     float thick = 1.f) {
    drawRectCalls.push_back({color, bounds});
  }
  void DrawText(const IText &text, const char *str, const IRECT &bounds) {}

  std::vector<DrawOp> fillCircleCalls;
  std::vector<DrawOp> drawCircleCalls;
  std::vector<DrawOp> drawArcCalls;
  std::vector<DrawOp> drawLineCalls;

  void FillCircle(const IColor &color, float cx, float cy, float r,
                  const void *blend = nullptr) {
    fillCircleCalls.push_back({color, IRECT(cx - r, cy - r, cx + r, cy + r)});
  }
  void DrawCircle(const IColor &color, float cx, float cy, float r,
                  const void *blend = nullptr, float thick = 1.f) {
    drawCircleCalls.push_back({color, IRECT(cx - r, cy - r, cx + r, cy + r)});
  }
  void DrawArc(const IColor &color, float cx, float cy, float r, float aMin,
               float aMax, const void *blend = nullptr, float thick = 1.f) {
    drawArcCalls.push_back({color, IRECT(cx - r, cy - r, cx + r, cy + r)});
  }
  void DrawLine(const IColor &color, float x1, float y1, float x2, float y2,
                const void *blend = nullptr, float thick = 1.f) {
    drawLineCalls.push_back({color, IRECT(x1, y1, x2, y2)});
  }

  // Path rendering stubs for Envelope
  std::vector<std::pair<float, float>> currentPath;
  std::vector<std::vector<std::pair<float, float>>> pathFills;
  std::vector<std::vector<std::pair<float, float>>> pathStrokes;

  void PathClear() { currentPath.clear(); }
  void PathMoveTo(float x, float y) { currentPath.push_back({x, y}); }
  void PathLineTo(float x, float y) { currentPath.push_back({x, y}); }
  void PathFill(const IColor &color) { pathFills.push_back(currentPath); }
  void PathStroke(const IColor &color, float thick,
                  const void *blend = nullptr) {
    pathStrokes.push_back(currentPath);
  }

  // Layer caching stubs for PolySection
  int startLayerCalls = 0;
  int endLayerCalls = 0;
  int drawLayerCalls = 0;
  void StartLayer(void * /*ctrl*/, const IRECT & /*bounds*/) {
    startLayerCalls++;
  }
  void *EndLayer() {
    endLayerCalls++;
    return (void *)1;
  } // non-null sentinel
  void DrawLayer(void * /*layer*/, const void * /*blend*/ = nullptr) {
    drawLayerCalls++;
  }
};

// IMouseMod stub
struct IMouseMod {};

// IActionFunction type alias
using IActionFunction = std::function<void(void *)>;

// COLOR_WHITE stub
static const IColor COLOR_WHITE{255, 255, 255, 255};

// ILayerPtr stub - PolySection stores this to cache its layer
using ILayerPtr = void *;

class IControl {
public:
  IControl(const IRECT &bounds, int paramIdx = -1,
           IActionFunction actionFunc = nullptr)
      : mRECT(bounds), mParamIdx(paramIdx), mActionFunc(actionFunc) {}
  virtual ~IControl() {}
  virtual void Draw(IGraphics &g) = 0;
  virtual void OnResize() {}
  virtual void SetDirty(bool pushParamToPlug = true) {}
  virtual void OnMouseDown(float x, float y, const IMouseMod &mod) {}
  virtual void OnMouseUp(float x, float y, const IMouseMod &mod) {}
  virtual void OnMouseOut() {}
  void SetTargetRECT(const IRECT &bounds) { mRECT = bounds; }
  IActionFunction GetActionFunction() const { return mActionFunc; }

  IRECT mRECT;
  WDL_String mTooltip;
  int mParamIdx;
  bool mIgnoreMouse = false;
  bool mMouseIsOver = false;

protected:
  IActionFunction mActionFunc;
};

class IKnobControlBase : public IControl {
public:
  IKnobControlBase(const IRECT &bounds, int paramIdx = -1)
      : IControl(bounds, paramIdx) {}
  virtual ~IKnobControlBase() {}

  double GetValue() const { return mValue; }
  void SetValue(double value) { mValue = value; }

  // Minimal param stub for PolyKnob display text
  struct MockParam {
    void GetDisplay(WDL_String &str) const { str = WDL_String("0.5"); }
  };
  MockParam *GetParam() const { return (MockParam *)&mParam; }

protected:
  double mValue = 0.5;
  MockParam mParam;
};

// ISwitchControlBase stub for PolyToggleButton
class ISwitchControlBase : public IControl {
public:
  ISwitchControlBase(const IRECT &bounds, int paramIdx,
                     void * /*actionFunc*/ = nullptr, int /*numStates*/ = 2)
      : IControl(bounds, paramIdx) {}
  virtual ~ISwitchControlBase() {}
  double GetValue() const { return mValue; }
  void SetValue(double v) { mValue = v; }

protected:
  double mValue = 0.0;
};
