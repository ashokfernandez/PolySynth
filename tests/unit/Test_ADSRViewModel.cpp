#include "../../UI/Controls/ADSRViewModel.h"
#include "catch.hpp"

TEST_CASE("ADSRViewModel generates correct geometric nodes and curves",
          "[adsr_math]") {
  ADSRViewModel model;
  model.SetBounds(0, 0, 100, 100);

  // SetParams(Attack, Decay, SustainLvl, RelTime, AtkTension, DecTension,
  // RelTension)
  model.SetParams(0.25f, 0.25f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f);

  auto pathData = model.CalculatePathData();

  REQUIRE(pathData.attackNode.x == 25.0f);
  REQUIRE(pathData.attackNode.y == 0.0f);
  REQUIRE(pathData.decaySustainNode.x == 50.0f);
  REQUIRE(pathData.decaySustainNode.y == 50.0f);
  REQUIRE(pathData.releaseNode.x == 100.0f);
  REQUIRE(pathData.releaseNode.y == 100.0f);

  // Tests from phase 1
  // Zero-State Boundary:
  model.SetParams(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.attackNode.x == 1.0f); // edge case: zero attack
  REQUIRE(pathData.attackNode.y == 0.0f);

  // Sustain Y-Axis Inversion:
  model.SetParams(0.25f, 0.25f, 0.75f, 0.5f, 0.0f, 0.0f, 0.0f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.decaySustainNode.y == 25.0f);

  // Bezier Control Point Generation (Curve Tension):
  model.SetParams(0.25f, 0.2f, 0.5f, 0.2f, 0.5f, 0.8f, -0.5f);
  pathData = model.CalculatePathData();
  REQUIRE(pathData.attackNode.x == 25.0f);
  REQUIRE(pathData.attackControlPoint.y >
          pathData.attackNode.y); // Verifying convex curve
}

TEST_CASE("ADSRViewModel Dynamic Color System Tests", "[adsr_color]") {
  ADSRViewModel model;
  // base Hue 182, Saturation 1.0, Lightness 0.42
  model.SetBaseColorHSL(182.0f, 1.0f, 0.42f);

  REQUIRE(model.GetColor(ADSRViewModel::Theme::Base).A == 255);
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Dark).A == 255);
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Faint).A ==
          static_cast<int>(255 * 0.2f));
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Fill).A ==
          static_cast<int>(255 * 0.15f));
  REQUIRE(model.GetColor(ADSRViewModel::Theme::Glow).A ==
          static_cast<int>(255 * 0.4f));
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
