#include "../../UI/Controls/PolyTheme.h"
#include "IControl.h"         // Mock
#include "IGraphicsStructs.h" // Mock structs
#include "catch.hpp"

// Since LCDPanel and SectionFrame include "IControl.h" etc, we need to ensure
// our mocks are found. CMake configuration will handle include paths.

#include "../../UI/Controls/LCDPanel.h"
#include "../../UI/Controls/PolyKnob.h"
#include "../../UI/Controls/SectionFrame.h"

TEST_CASE("LCDPanel Basic", "[UI][LCD]") {
  IRECT rect(0, 0, 100, 50);
  IColor color(255, 0, 255, 0); // Green

  LCDPanel panel(rect, color);
  REQUIRE(panel.mRECT.L == 0);
  REQUIRE(panel.mRECT.R == 100);
  REQUIRE(panel.mIgnoreMouse == true);

  // Mock Graphics
  IGraphics g;
  panel.Draw(g);

  // LCDPanel::Draw calls FillRoundRect (background) + DrawRoundRect x2 (bezel +
  // highlight)
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 2);
  REQUIRE(g.fillRectCalls[0].c.R ==
          0); // PolyTheme::LCDBackground default constructed or explicitly
              // passed green color
}

TEST_CASE("SectionFrame Basic", "[UI][Frame]") {
  IRECT rect(10, 10, 200, 100);
  SectionFrame frame(rect, "Test", IColor(255), IColor(255));

  REQUIRE(frame.mRECT.L == 10);

  IGraphics g;
  frame.Draw(g);

  REQUIRE(g.drawRectCalls.size() == 1);
  // DrawText logic mocked in IGraphics::DrawText (currently empty)
}

TEST_CASE("PolyKnob Layout & Bounds", "[UI][Knob]") {
  IRECT rect(0, 0, 100, 100);
  PolyKnob knob(rect, 0, "Cutoff");

  // Check initial state
  REQUIRE(knob.mRECT.W() == 100);

  // Test resize logic calculates internal rects correctly
  knob.SetTargetRECT(IRECT(0, 0, 50, 150));
  knob.OnResize();

  // PolyKnob uses a 40% midpad for the knob itself based on width
  // W=50, H=150. mKnobRect = mRECT.GetPadded(-5).GetMidVPadded(50 * 0.4)
  // W=40, height constrained by MidVPadded(20) = 40.

  IGraphics g;
  knob.Draw(g);

  // PolyKnob drawing logic:
  // 1 FillCircle (shadow) + 1 FillCircle (body) + 1 FillCircle (indicator dot)
  // = 3 2 DrawCircle (bezel, glow) 3 DrawArc (track, active, inner active) 1
  // DrawLine (indicator)
  REQUIRE(g.fillCircleCalls.size() == 3);
  REQUIRE(g.drawCircleCalls.size() == 2);
  REQUIRE(g.drawArcCalls.size() == 3);
  REQUIRE(g.drawLineCalls.size() == 1);
}

#include "../../UI/Controls/Envelope.h"

TEST_CASE("Envelope Bounds & Draw", "[UI][Envelope]") {
  IRECT rect(0, 0, 200, 100);
  Envelope env(rect);

  // Set predictable ADSR: 25% Attack, 25% Decay, 50% Sustain, 25% Release
  env.SetADSR(0.25f, 0.25f, 0.5f, 0.25f);

  IGraphics g;
  env.Draw(g);

  // Background and border
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 1);

  // Path building for fill
  REQUIRE(g.pathFills.size() == 1);
  // Path building for stroke
  REQUIRE(g.pathStrokes.size() == 1);

  // Paths are cleared and rebuilt twice
  REQUIRE(g.currentPath.size() == 5); // 1 MoveTo + 4 LineTo's for stroke

  const auto &stroke = g.pathStrokes[0];
  // Verify coordinates of the stroke path vertices
  // Padded bounds: 10px all sides -> w = 180, h = 80, left = 10, top = 10
  // Attack (0.25) -> ax = 180 * (0.25 * 0.25) = 11.25
  // Left + ax = 21.25. Y = top = 10.
  REQUIRE(stroke.size() == 5);
  REQUIRE(stroke[1].first == Approx(21.25f).margin(0.1f));
  REQUIRE(stroke[1].second == Approx(10.0f).margin(0.1f));
}

// ─── PolyToggleButton ────────────────────────────────────────────────────────
#include "../../UI/Controls/PolyToggleButton.h"

TEST_CASE("PolyToggleButton - inactive state", "[UI][Toggle]") {
  IRECT rect(0, 0, 80, 24);
  PolyToggleButton btn(rect, 0, "Filter");

  IGraphics g;
  btn.Draw(g);

  // Inactive: 1 FillRoundRect (background) + 1 DrawRoundRect (border)
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 1);
}

TEST_CASE("PolyToggleButton - active state", "[UI][Toggle]") {
  IRECT rect(0, 0, 80, 24);
  PolyToggleButton btn(rect, 0, "Filter");
  btn.SetValue(1.0); // active

  IGraphics g;
  btn.Draw(g);

  // Active: 1 FillRoundRect (background colour), no border DrawRoundRect
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 0);
}

// ─── PresetSaveButton ────────────────────────────────────────────────────────
#include "../../UI/Controls/PresetSaveButton.h"

TEST_CASE("PresetSaveButton - clean state", "[UI][Preset]") {
  IRECT rect(0, 0, 80, 28);
  PresetSaveButton btn(rect, nullptr);

  IGraphics g;
  btn.Draw(g);

  // Clean (no unsaved changes): 1 fill + 1 border
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 1);
}

TEST_CASE("PresetSaveButton - dirty state", "[UI][Preset]") {
  IRECT rect(0, 0, 80, 28);
  PresetSaveButton btn(rect, nullptr);
  btn.SetHasUnsavedChanges(true);

  IGraphics g;
  btn.Draw(g);

  // Dirty (unsaved changes): 1 fill, NO border (accent colour fills)
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 0);
  REQUIRE(btn.GetHasUnsavedChanges() == true);
}

// ─── PolySection ─────────────────────────────────────────────────────────────
#include "../../UI/Controls/PolySection.h"

TEST_CASE("PolySection - first draw triggers layer caching", "[UI][Section]") {
  IRECT rect(0, 0, 200, 150);
  PolySection section(rect, "OSC");

  IGraphics g;

  // First draw: StartLayer → draw content → EndLayer → DrawLayer
  section.Draw(g);
  REQUIRE(g.startLayerCalls == 1);
  REQUIRE(g.endLayerCalls == 1);
  REQUIRE(g.drawLayerCalls == 1);
  // Background fills: shadow + panel + sheen = at least 3
  REQUIRE(g.fillRectCalls.size() >= 3);
}

TEST_CASE("PolySection - resize clears cached layer", "[UI][Section]") {
  IRECT rect(0, 0, 200, 150);
  PolySection section(rect, "ENV");

  IGraphics g;
  section.Draw(g); // populates cache

  // Resize invalidates the cache
  section.OnResize();

  IGraphics g2;
  section.Draw(g2); // should rebuild the layer

  REQUIRE(g2.startLayerCalls == 1);
  REQUIRE(g2.endLayerCalls == 1);
}
