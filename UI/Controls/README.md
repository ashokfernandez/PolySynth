# UI Controls Library

This directory contains reusable iPlug2/IGraphics UI controls for the PolySynth project.

## Controls

### Knob.h
Rotary knob control with vector graphics.

**Usage:**
```cpp
#include "UI/Controls/Knob.h"

// In OnLayout()
auto* knob = new Knob(IRECT(x, y, x+100, y+100), kParamId, "Label");
pGraphics->AttachControl(knob);

// Optional: customize colors
knob->SetColors(COLOR_BLACK, COLOR_BLUE, COLOR_WHITE);
```

**API:**
- `SetColors(track, value, pointer)` - Customize colors

### Fader.h
Vertical slider control.

**Usage:**
```cpp
#include "UI/Controls/Fader.h"

// In OnLayout()
auto* fader = new Fader(IRECT(x, y, x+60, y+200), kParamId, "Label");
pGraphics->AttachControl(fader);

// Optional: customize colors
fader->SetColors(COLOR_GRAY, COLOR_BLUE);
```

**API:**
- `SetColors(track, handle)` - Customize colors

### Envelope.h
ADSR envelope visualizer (non-interactive).

**Usage:**
```cpp
#include "UI/Controls/Envelope.h"

// In OnLayout()
auto* envelope = new Envelope(IRECT(x, y, x+400, y+200));
envelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f); // A, D, S, R
pGraphics->AttachControl(envelope);

// Optional: customize colors
envelope->SetColors(COLOR_BLUE, COLOR_BLUE.WithOpacity(0.2f));
```

**API:**
- `SetADSR(attack, decay, sustain, release)` - Set envelope values (0.0-1.0)
- `SetColors(stroke, fill)` - Customize colors

## Design Philosophy

1. **Header-Only:** Simple controls are header-only for easy inclusion
2. **Minimal Dependencies:** Only depend on IGraphics, no DSP code
3. **Customizable:** Colors and behavior can be customized
4. **Reusable:** Shared between testing (ComponentGallery) and production

## Testing

All controls are tested in the ComponentGallery visual test harness:
```bash
./build_gallery.sh
cd tests/Visual
npm run test
```
