# UI Controls Library

This directory contains UI control definitions for the PolySynth component gallery. Each control documented here is visually tested in the ComponentGallery and available in Storybook.

## Gallery Components

### Knob
Rotary knob control for continuous parameter adjustment.

**Implementation:** Uses iPlug2's built-in `IVKnobControl`
- Size: 100×100px
- Position: Top-left corner (20px padding)
- Interactive: Click and drag to adjust value

**Usage:**
```cpp
pGraphics->AttachControl(new IVKnobControl(
  IRECT(x, y, x+100, y+100),
  paramIdx));
```

**Why built-in?** `IVKnobControl` provides excellent default styling with NanoVG vector graphics. Using the built-in control gives us a solid foundation that can be customized later through iPlug2's styling system without maintaining custom drawing code.

---

### Fader
Vertical slider control for linear parameter adjustment.

**Implementation:** Uses iPlug2's built-in `IVSliderControl`
- Size: 60×200px
- Position: Top-left corner (20px padding)
- Interactive: Click and drag to adjust value

**Usage:**
```cpp
pGraphics->AttachControl(new IVSliderControl(
  IRECT(x, y, x+60, y+200),
  paramIdx));
```

**Why built-in?** `IVSliderControl` provides smooth interaction and professional appearance out of the box. The built-in control handles all edge cases (snapping, fine-tuning with modifiers, etc.) that we'd otherwise need to reimplement.

---

### Envelope
ADSR envelope visualizer for displaying envelope shapes.

**Implementation:** Custom control defined in `Envelope.h`
- Size: 400×150px
- Position: Top-left corner (20px padding)
- Display-only: Non-interactive visualization

**Usage:**
```cpp
#include "Envelope.h"

auto* envelope = new Envelope(IRECT(x, y, x+400, y+150));
envelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f); // Attack, Decay, Sustain, Release
pGraphics->AttachControl(envelope);

// Optional: customize colors
envelope->SetColors(COLOR_BLUE, COLOR_BLUE.WithOpacity(0.2f));
```

**API:**
- `SetADSR(attack, decay, sustain, release)` - Set envelope values (0.0-1.0)
- `SetColors(stroke, fill)` - Customize stroke and fill colors

**Why custom?** iPlug2 doesn't provide a built-in ADSR envelope visualizer. This custom control fills that gap by rendering a visual representation of envelope parameters.

---

### PolyKnob
Themed rotary knob with arc indicator, label, and value readout.

**Implementation:** Custom control defined in `PolyKnob.h`
- Base class: `IKnobControlBase`
- Default size: 80x116px (80px knob + 18px label top + 18px value bottom)
- Features: accent color arc, soft shadow, bezel highlight, glow pass
- Configurable: show/hide label, show/hide value, custom accent color

**Usage:**
```cpp
#include "PolyKnob.h"

auto* knob = new PolyKnob(IRECT(x, y, x+80, y+116), paramIdx, "Cutoff");
knob->SetAccent(PolyTheme::AccentCyan);  // Optional: change accent color
pGraphics->AttachControl(knob);

// For stacked controls (no label/value overlap):
auto* bare = new PolyKnob(bounds, paramIdx, "");
bare->WithShowLabel(false).WithShowValue(false);
```

**API:**
- `SetAccent(IColor)` - Set the arc/indicator accent color
- `WithShowLabel(bool)` - Show or hide the top label text (fluent)
- `WithShowValue(bool)` - Show or hide the bottom value readout (fluent)

**Why custom?** Provides a consistent themed appearance across the synth UI with the PolyTheme color system, cached arc rendering, and configurable label/value visibility for use in stacked control layouts.

---

### PolySection
Cached panel background with title, premium surface treatment, and depth borders.

**Implementation:** Custom control defined in `PolySection.h`
- Base class: `IControl` (display-only, `mIgnoreMouse = true`)
- Features: drop shadow, panel fill, top sheen, scanline texture, double border, title text
- Performance: Uses `ILayerPtr` caching - panel is rendered once and redrawn from cache
- Invalidates cache automatically on resize via `OnResize()` override

**Usage:**
```cpp
#include "PolySection.h"

pGraphics->AttachControl(new PolySection(
    IRECT(x, y, x+300, y+200), "FILTER"));
```

**Why custom?** iPlug2 doesn't provide a themed section panel with caching. This control creates the characteristic PolySynth panel appearance with a single-draw cache for performance.

---

### PolyToggleButton
Latching toggle button with themed active/inactive states.

**Implementation:** Custom control defined in `PolyToggleButton.h`
- Base class: `ISwitchControlBase` (2 states)
- Active state: colored background (AccentCyan), white text
- Inactive state: transparent background, dark text, subtle border
- Hover: semi-transparent white overlay
- **Key design decision:** Uses `GetValue() > 0.5` for active state (not `mMouseDown`), which fixes the latching bug present in `IVSwitchControl`

**Usage:**
```cpp
#include "PolyToggleButton.h"

pGraphics->AttachControl(new PolyToggleButton(
    IRECT(x, y, x+80, y+28), paramIdx, "MONO"));
```

**Why custom?** The built-in `IVSwitchControl` passes `mMouseDown` (momentary) to `DrawPressableShape`, causing buttons to only appear active while the mouse is held down. `PolyToggleButton` correctly reads the persistent parameter value for visual state, providing proper toggle behavior.

---

### PolyTheme
Centralized color constants for the PolySynth UI theme.

**Implementation:** Namespace defined in `PolyTheme.h`
- Panel colors: `PanelBG`, `SectionBorder`
- Text colors: `TextDark`
- Accent colors: `AccentRed`, `AccentCyan`
- Knob colors: `KnobRingOff`, `KnobRingOn`
- Toggle colors: `ToggleActiveBG`, `ToggleActiveFG`, `ToggleInactiveBG`
- Effects: `Highlight` (semi-transparent white overlay)

**Usage:**
```cpp
#include "PolyTheme.h"

g.FillRoundRect(PolyTheme::PanelBG, bounds, 5.f);
g.DrawText(IText(14.f, PolyTheme::TextDark, "Roboto-Bold"), text, rect);
```

**Why centralized?** All custom controls reference PolyTheme instead of hardcoding colors, ensuring visual consistency and enabling future theme switching.

---

## Design Philosophy

### When to Use Custom Poly* Controls
For PolySynth-specific UI elements, prefer the custom `Poly*` controls because:
1. **Theme consistency:** All controls use `PolyTheme` colors for a unified look
2. **Bug-free behavior:** Fixes known iPlug2 issues (e.g., `IVSwitchControl` latching bug)
3. **Performance:** `PolySection` uses `ILayerPtr` caching for efficient redraws
4. **Flexibility:** Fluent API for configuring label/value visibility, accent colors, etc.

### When to Use Built-in iPlug2 Controls
Use iPlug2's built-in controls when:
1. **No custom equivalent exists yet** (faders, radio buttons, tab switches)
2. **Prototyping:** Quick iteration before building a custom themed version
3. **Standard behavior is sufficient** and theme consistency is not critical

### When to Write New Custom Controls
Create a new custom control when:
1. **No built-in equivalent exists** (like ADSR envelope visualizer)
2. **Built-in behavior has bugs** that affect the user experience
3. **Custom drawing is essential** to maintain PolySynth's visual identity

### Control Design Guidelines
1. **Header-Only:** Controls are header-only for easy inclusion
2. **Minimal Dependencies:** Only depend on IGraphics + PolyTheme, no DSP code
3. **Theme-First:** Use `PolyTheme` colors instead of hardcoded `IColor()` values
4. **Customizable:** Expose color and behavior customization via fluent API where appropriate
5. **Reusable:** Shared between testing (ComponentGallery) and production code (PolySynth)

---

## Testing

All controls are tested in the ComponentGallery visual test harness with Storybook:

```bash
# Build all component galleries
./build_gallery.sh

# View in Storybook (interactive testing)
./view_gallery.sh
# Opens http://localhost:6006
```

Visual regression tests with Playwright:
```bash
cd tests/Visual
npm run test
```

Each component gets:
- **Isolated page** in Storybook for manual testing
- **Baseline screenshot** for visual regression detection
- **Interactive testing** to verify click/drag behavior
- **Build-time isolation** via preprocessor flags for true component separation

---

## Adding New Components

To add a new component to the gallery:

1. **Evaluate**: Check if iPlug2 has a built-in control first
2. **Implement**:
   - If using built-in: Add to `ComponentGallery.cpp` with preprocessor flag
   - If custom: Create header in `UI/Controls/` and add to `ComponentGallery.cpp`
3. **Build**: Add build variant to `scripts/build_all_galleries.sh`
4. **Document**: Add section to this README with implementation details
5. **Story**: Add Storybook story in `tests/Visual/stories/component-gallery.stories.js`
6. **Test**: Add visual regression test in `tests/Visual/specs/components.spec.ts`

Example preprocessor section for new component:
```cpp
#elif defined(GALLERY_COMPONENT_MYNEWCONTROL)
  pGraphics->AttachControl(new MyNewControl(
    IRECT(padding, padding, padding + width, padding + height),
    params...));
#endif
```
