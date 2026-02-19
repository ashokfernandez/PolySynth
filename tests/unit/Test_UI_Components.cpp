#include "IControl.h"         // Mock
#include "IGraphicsStructs.h" // Mock structs
#include "catch.hpp"

// Since LCDPanel and SectionFrame include "IControl.h" etc, we need to ensure
// our mocks are found. CMake configuration will handle include paths.

#include "../../UI/Controls/LCDPanel.h"
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

  // LCDPanel::Draw calls FillRoundRect (background) + DrawRoundRect x2 (bezel + highlight)
  REQUIRE(g.fillRectCalls.size() == 1);
  REQUIRE(g.drawRectCalls.size() == 2);
  REQUIRE(g.fillRectCalls[0].c.R == 20); // PolyTheme::LCDBackground.R
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
