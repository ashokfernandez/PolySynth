#include "../../UI/Controls/ADSRViewModel.h"
#include "catch.hpp"

TEST_CASE("ADSRViewModel generates correct geometric nodes and curves",
          "[adsr_math]") {
  ADSRViewModel model;
  model.SetBounds(0, 0, 100, 100);

  // Proportional scaling formula:
  //   T_total   = max(0.001, A + D + R)
  //   T_display = T_total * 1.25
  //   scale     = width / T_display
  //   attackX       = L + A * scale
  //   decayX        = attackX + D * scale
  //   releaseStartX = decayX + T_total * 0.25 * scale  (always 20% of width)
  //   releaseX      = R (right edge)

  // A=0.25, D=0.25, R=0.5 → T_total=1.0, T_display=1.25, scale=80
  model.SetParams(0.25f, 0.25f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f);
  auto pathData = model.CalculatePathData();

  REQUIRE(pathData.attackNode.x == Approx(20.0f));
  REQUIRE(pathData.attackNode.y == 0.0f);
  REQUIRE(pathData.decaySustainNode.x == Approx(40.0f));
  REQUIRE(pathData.decaySustainNode.y == 50.0f);
  REQUIRE(pathData.releaseStartNode.x == Approx(60.0f));
  REQUIRE(pathData.releaseNode.x == 100.0f);
  REQUIRE(pathData.releaseNode.y == 100.0f);

  // Zero-State Boundary:
  // All params = 0 → T_total clamped to 0.001, attackX = 0 * scale = 0
  model.SetParams(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.attackNode.x == Approx(0.0f));
  REQUIRE(pathData.attackNode.y == 0.0f);

  // Sustain Y-Axis Inversion:
  // sustainY = bottom - (0.75 * 100) = 100 - 75 = 25
  model.SetParams(0.25f, 0.25f, 0.75f, 0.5f, 0.0f, 0.0f, 0.0f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.decaySustainNode.y == 25.0f);

  // Bezier Control Point Generation (Curve Tension):
  // A=0.25, D=0.2, R=0.2 → T_total=0.65, scale≈123.077
  // attackNode.x ≈ 0.25 * 123.077 ≈ 30.77
  model.SetParams(0.25f, 0.2f, 0.5f, 0.2f, 0.5f, 0.8f, -0.5f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.attackNode.x == Approx(30.769f).epsilon(0.01f));
  REQUIRE(pathData.attackControlPoint.y >
          pathData.attackNode.y); // Verifying convex curve

  // Zero tension produces straight line (control point at segment midpoint):
  // A=0.25, D=0.25, R=0.5 → attackX=20, midpoint of (0,100) to (20,0) = (10, 50)
  model.SetParams(0.25f, 0.25f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.attackControlPoint.x == Approx(10.0f));
  REQUIRE(pathData.attackControlPoint.y == Approx(50.0f));

  // releaseStartNode: always at 20% of width from decayX
  REQUIRE(pathData.releaseStartNode.x == Approx(60.0f));
  REQUIRE(pathData.releaseStartNode.y == Approx(50.0f)); // sustainY = 100 - 0.5*100
}

TEST_CASE("ADSRViewModel Dynamic Color System Tests", "[adsr_color]") {
  ADSRViewModel model;
  // base Hue 182, Saturation 1.0, Lightness 0.42
  model.SetBaseColorHSL(182.0f, 1.0f, 0.42f);

  REQUIRE(model.GetColor(ADSRViewModel::Theme::Base).A == 255);
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Dark).A == 255);
  // Current code: Faint=0.12, Fill=0.08, Glow=0.18
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Faint).A ==
          static_cast<int>(255 * 0.12f));
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Fill).A ==
          static_cast<int>(255 * 0.08f));
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Glow).A ==
          static_cast<int>(255 * 0.18f));
}

TEST_CASE("ADSRViewModel Playhead Interpolation Tests", "[adsr_playhead]") {
  ADSRViewModel model;
  model.SetBounds(0, 0, 100, 100);

  REQUIRE(model.GetPlayheadX(0.0f) == 0.0f);
  REQUIRE(model.GetPlayheadX(0.5f) == 50.0f);
  REQUIRE(model.GetPlayheadX(1.0f) == 100.0f);
  REQUIRE(model.GetPlayheadX(1.5f) == 100.0f); // clamp
  REQUIRE(model.GetPlayheadX(-0.5f) == 0.0f);  // clamp
}
