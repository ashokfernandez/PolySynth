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

## Design Philosophy

### When to Use Built-in Controls
For standard UI elements (knobs, sliders, buttons, etc.), we prefer iPlug2's built-in controls because:
1. **Battle-tested:** Handle edge cases, accessibility, and platform quirks
2. **Styling system:** Can be customized through iPlug2's `IVStyle` without code changes
3. **Maintenance:** No custom drawing code to maintain
4. **Consistency:** Match iPlug2 conventions and best practices

### When to Write Custom Controls
Create custom controls only when:
1. **No built-in equivalent exists** (like ADSR envelope visualizer)
2. **Highly specialized behavior** not covered by built-in controls
3. **Custom drawing is essential** to the component's purpose

### Control Design Guidelines
1. **Header-Only:** Controls are header-only for easy inclusion
2. **Minimal Dependencies:** Only depend on IGraphics, no DSP code
3. **Customizable:** Expose color and behavior customization where appropriate
4. **Reusable:** Shared between testing (ComponentGallery) and production code

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
