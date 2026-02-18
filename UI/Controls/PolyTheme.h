#pragma once

#include "IControl.h"

namespace PolyTheme {
// ── Surface colors ──────────────────────────────────────────────
static const IColor PanelBG = IColor(255, 239, 237, 230);
static const IColor HeaderBG = IColor(255, 246, 244, 238);
static const IColor ControlBG = IColor(255, 240, 238, 232);
static const IColor SectionBorder = IColor(255, 180, 180, 180);

// ── Text colors ─────────────────────────────────────────────────
static const IColor TextDark = IColor(255, 30, 30, 30);

// ── Accent colors ───────────────────────────────────────────────
static const IColor AccentRed = IColor(255, 235, 74, 57);
static const IColor AccentCyan = IColor(255, 60, 218, 230);

// ── Knob colors ─────────────────────────────────────────────────
static const IColor KnobBody = IColor(255, 247, 245, 240);
static const IColor KnobRingOff = IColor(255, 200, 200, 200);
static const IColor KnobRingOn = AccentRed;

// ── Effect overlays ─────────────────────────────────────────────
static const IColor Highlight = IColor(100, 255, 255, 255);
static const IColor ShadowDark = IColor(45, 0, 0, 0);
static const IColor ShadowLight = IColor(35, 0, 0, 0);
static const IColor InnerGlow = IColor(60, 255, 255, 255);
static const IColor Scanline = IColor(7, 255, 255, 255);
static const IColor InnerBorder = IColor(40, 255, 255, 255);

// ── Control face colors ───────────────────────────────────────
static const IColor ControlFace =
    IColor(255, 230, 228, 222); // IVStyle kFG – button/tab faces
static const IColor TabInactiveBG =
    IColor(255, 220, 218, 212); // Slightly darker for unselected tabs

// ── Toggle button colors ────────────────────────────────────────
static const IColor ToggleActiveBG = AccentCyan;
static const IColor ToggleActiveFG = IColor(255, 255, 255, 255);
static const IColor ToggleInactiveBG = IColor(0, 0, 0, 0);

// ── LCD colors ──────────────────────────────────────────────────
static const IColor LCDBackground =
    IColor(255, 20, 25, 20); // Very dark greenish-black
static const IColor LCDText = IColor(255, 50, 255, 60); // Bright retro green

// ── Typography constants ────────────────────────────────────────
// Font family: "Roboto-Bold" for labels/titles, "Roboto-Regular" for values
static constexpr float FontTitle = 40.f;       // Plugin title
static constexpr float FontSectionHead = 14.f; // Section panel headers
static constexpr float FontLabel = 15.f;       // Stacked control labels
static constexpr float FontValue = 14.f;       // Value readouts / captions
static constexpr float FontControl = 12.f;     // Knob labels, toggle text
static constexpr float FontStyleLabel = 13.f;  // IVStyle label text
static constexpr float FontStyleValue = 11.f;  // IVStyle value text
static constexpr float FontTabSwitch = 12.f;   // Tab switch / model selector

// ── Layout constants ────────────────────────────────────────────
static constexpr float Padding = 12.f;       // Outer panel padding
static constexpr float SectionPadding = 8.f; // Inner section padding
static constexpr float SectionTitleH =
    36.f;                             // Height reserved for section title
static constexpr float LabelH = 18.f; // Stacked control label height
static constexpr float ValueH = 16.f; // Stacked control value height
} // namespace PolyTheme
