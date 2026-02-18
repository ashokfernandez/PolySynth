#pragma once

#include "IControl.h"
#include "PolyTheme.h"

class PresetSaveButton : public IControl {
public:
  PresetSaveButton(const IRECT &bounds, IActionFunction actionFunc)
      : IControl(bounds, -1, actionFunc) {}

  void Draw(IGraphics &g) override {
    // Determine state based on dirty flag (which we use to track "unsaved
    // changes" state here)
    const bool isDirty = mHasUnsavedChanges;
    const bool isPressed = mIsPressed;

    IColor bg, fg;
    if (isDirty) {
      bg = PolyTheme::AccentRed;
      fg = COLOR_WHITE;
    } else {
      // "Disabled" / Clean state
      bg = PolyTheme::ControlFace; // Greyish
      fg = PolyTheme::TextDark.WithOpacity(0.5f);
    }

    if (isPressed && isDirty) {
      bg = bg.WithOpacity(0.8f);
    }

    // Draw background
    g.FillRoundRect(bg, mRECT, PolyTheme::RoundingButton);

    // Draw border if clean (to define shape)
    if (!isDirty) {
      g.DrawRoundRect(PolyTheme::SectionBorder, mRECT,
                      PolyTheme::RoundingButton, nullptr, 1.f);
    }

    // Draw Text
    WDL_String label("SAVE");
    if (isDirty)
      label.Append(" *");

    g.DrawText(IText(13.f, fg, "Bold", EAlign::Center), label.Get(), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod &mod) override {
    if (mHasUnsavedChanges) {
      mIsPressed = true;
      SetDirty(false); // Redraw
    }
  }

  void OnMouseUp(float x, float y, const IMouseMod &mod) override {
    if (mIsPressed) {
      mIsPressed = false;
      if (mRECT.Contains(x, y) && mHasUnsavedChanges) {
        // Execute action
        if (GetActionFunction())
          GetActionFunction()(this);
      }
      SetDirty(false);
    }
  }

  void OnMouseOut() override {
    if (mIsPressed) {
      mIsPressed = false;
      SetDirty(false);
    }
  }

  void SetHasUnsavedChanges(bool hasChanges) {
    mHasUnsavedChanges = hasChanges;
    SetDirty(false); // Trigger redraw
  }

  bool GetHasUnsavedChanges() const { return mHasUnsavedChanges; }

private:
  bool mHasUnsavedChanges = false;
  bool mIsPressed = false;
};
