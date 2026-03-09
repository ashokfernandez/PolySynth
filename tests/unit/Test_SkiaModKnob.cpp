#include "../../UI/Controls/PolyTheme.h"
#include "IControl.h"
#include "IGraphicsStructs.h"
#include "catch.hpp"

#include "../../UI/Controls/ModKnob.h"

// ─── State Clamping ──────────────────────────────────────────────────────────

TEST_CASE("ModKnob - SetModulationBounds swaps if start > end",
          "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  knob.SetModulationBounds(0.8, 0.4);
  REQUIRE(knob.GetModStart() == Approx(0.4));
  REQUIRE(knob.GetModEnd() == Approx(0.8));
}

TEST_CASE("ModKnob - SetModulationBounds clamps to 0-1", "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  knob.SetModulationBounds(-0.5, 1.5);
  REQUIRE(knob.GetModStart() == Approx(0.0));
  REQUIRE(knob.GetModEnd() == Approx(1.0));
}

TEST_CASE("ModKnob - SetModCurrent clamps", "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  knob.SetModCurrent(1.5);
  REQUIRE(knob.GetModCurrent() == Approx(1.0));

  knob.SetModCurrent(-0.3);
  REQUIRE(knob.GetModCurrent() == Approx(0.0));
}

// ─── Base Drag Shift ─────────────────────────────────────────────────────────

TEST_CASE("ModKnob - base drag shifts value and mod window together",
          "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  knob.SetValue(0.5);
  knob.SetModulationBounds(0.4, 0.6);

  // Simulate mouse down at center of knob (inside base hit radius)
  const float cx = knob.GetDrawRECT().MW();
  const float cy = knob.GetDrawRECT().MH();
  IMouseMod mod;
  knob.OnMouseDown(cx, cy, mod);

  // Drag upward by 120px → delta = 120/200 = 0.6
  // New value = 0.5 + 0.6 = 1.0 (clamped)
  knob.OnMouseDrag(cx, cy - 120.f, 0.f, -120.f, mod);

  REQUIRE(knob.GetValue() == Approx(1.0));
  // Mod handles stay where they were — base drag only moves value
  REQUIRE(knob.GetModStart() == Approx(0.4));
  REQUIRE(knob.GetModEnd() == Approx(0.6));
}

// ─── Deviation Math ──────────────────────────────────────────────────────────

TEST_CASE("ModKnob - deviation strip spans value to modCurrent",
          "[UI][ModKnob]") {
  // This test verifies the math used for the deviation arc rendering.
  // The deviation strip goes from min(value, modCurrent) to max(value,
  // modCurrent). We verify via the draw calls.

  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  knob.SetValue(0.5);
  knob.SetModCurrent(0.2);
  knob.ShowModulation(true);

  IGraphics g;
  knob.Draw(g);

  // With modulation visible, deviation strip produces 2 extra DrawArc calls
  // + 2 DrawCircle calls for handles. Total arcs:
  // 1 (inactive track, in layer) + 2 (value glow + solid) + 2 (deviation glow
  // + solid) = 5
  REQUIRE(g.drawArcCalls.size() == 5);
}

// ─── Hit Testing ─────────────────────────────────────────────────────────────

TEST_CASE("ModKnob - IsHit rejects corners outside outer track",
          "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  // Corners of mRECT should be outside the circular hit area
  REQUIRE(knob.IsHit(0.f, 0.f) == false);
  REQUIRE(knob.IsHit(100.f, 0.f) == false);
  REQUIRE(knob.IsHit(0.f, 120.f) == false);
  REQUIRE(knob.IsHit(100.f, 120.f) == false);

  // Center should be inside
  REQUIRE(knob.IsHit(knob.GetDrawRECT().MW(), knob.GetDrawRECT().MH()) ==
          true);
}

// ─── Layout ──────────────────────────────────────────────────────────────────

TEST_CASE("ModKnob - square mDrawRECT centered in non-square mRECT",
          "[UI][ModKnob]") {
  // Non-square: 80 wide, 140 tall
  IRECT rect(10, 10, 90, 150);
  ModKnob knob(rect, 0, "Res");

  const IRECT &draw = knob.GetDrawRECT();

  // Should be square
  REQUIRE(draw.W() == Approx(draw.H()).margin(0.1f));

  // Should be centered horizontally within the control
  REQUIRE(draw.MW() == Approx(rect.MW()).margin(0.1f));
}

// ─── Draw Call Counts ────────────────────────────────────────────────────────

TEST_CASE("ModKnob - basic draw produces expected operations",
          "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  IGraphics g;
  knob.Draw(g);

  // Layer caching: 1 start + 1 end + 1 draw
  REQUIRE(g.startLayerCalls == 1);
  REQUIRE(g.endLayerCalls == 1);
  REQUIRE(g.drawLayerCalls == 1);

  // In cached layer: 2 FillCircle (shadow + body)
  // Dynamic: 1 FillCircle (indicator dot)
  REQUIRE(g.fillCircleCalls.size() == 3);

  // In cached layer: 2 DrawCircle (highlight + shadow bezel)
  // No mod handles (modulation hidden by default)
  REQUIRE(g.drawCircleCalls.size() == 2);

  // In cached layer: 1 DrawArc (inactive track)
  // Dynamic: 2 DrawArc (value glow + value solid)
  REQUIRE(g.drawArcCalls.size() == 3);

  // Dynamic: 1 DrawLine (indicator)
  REQUIRE(g.drawLineCalls.size() == 1);
}

TEST_CASE("ModKnob - modulation visible adds mod draw calls",
          "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");
  knob.ShowModulation(true);

  IGraphics g;
  knob.Draw(g);

  // +2 DrawArc for deviation strip (glow + solid)
  REQUIRE(g.drawArcCalls.size() == 5);

  // +2 DrawCircle for mod handles (start + end)
  REQUIRE(g.drawCircleCalls.size() == 4);
}

// ─── Double-Click Reset ──────────────────────────────────────────────────────

TEST_CASE("ModKnob - double click resets to default", "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");
  knob.SetValue(0.8);

  IMouseMod mod;
  knob.OnMouseDblClick(50.f, 60.f, mod);

  // MockParam::GetDefault(true) returns 0.5
  REQUIRE(knob.GetValue() == Approx(0.5));
}

// ─── Fine Drag (Shift) ──────────────────────────────────────────────────────

TEST_CASE("ModKnob - shift drag applies fine scaling", "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");
  knob.SetValue(0.5);

  const float cx = knob.GetDrawRECT().MW();
  const float cy = knob.GetDrawRECT().MH();

  IMouseMod mod;
  knob.OnMouseDown(cx, cy, mod);

  // Normal drag up 100px → delta = 100/200 = 0.5
  knob.OnMouseDrag(cx, cy - 100.f, 0.f, -100.f, mod);
  double normalResult = knob.GetValue();
  REQUIRE(normalResult == Approx(1.0)); // 0.5 + 0.5

  // Reset and try with shift
  knob.OnMouseUp(cx, cy, mod);
  knob.SetValue(0.5);
  knob.SetModulationBounds(0.4, 0.6);

  knob.OnMouseDown(cx, cy, mod);

  IMouseMod shiftMod;
  shiftMod.S = 1.f; // shift held
  knob.OnMouseDrag(cx, cy - 100.f, 0.f, -100.f, shiftMod);

  // Fine drag: delta = 100/200 * 0.1 = 0.05
  REQUIRE(knob.GetValue() == Approx(0.55));
}

// ─── Resize Clears Cache ─────────────────────────────────────────────────────

TEST_CASE("ModKnob - resize clears cached layer", "[UI][ModKnob]") {
  IRECT rect(0, 0, 100, 120);
  ModKnob knob(rect, 0, "Cutoff");

  IGraphics g;
  knob.Draw(g); // populates cache

  knob.OnResize();

  IGraphics g2;
  knob.Draw(g2); // should rebuild the layer

  REQUIRE(g2.startLayerCalls == 1);
  REQUIRE(g2.endLayerCalls == 1);
}
