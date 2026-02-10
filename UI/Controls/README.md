# UI Controls Library

This directory contains reusable iPlug2/IGraphics UI controls for the PolySynth project.

## Controls

### Envelope.h
ADSR envelope visualizer (non-interactive display component).

**Usage:**
```cpp
#include "Envelope.h"

// In OnLayout()
auto* envelope = new Envelope(IRECT(x, y, x+400, y+200));
envelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f); // Attack, Decay, Sustain, Release
pGraphics->AttachControl(envelope);

// Optional: customize colors
envelope->SetColors(COLOR_BLUE, COLOR_BLUE.WithOpacity(0.2f));
```

**API:**
- `SetADSR(attack, decay, sustain, release)` - Set envelope values (0.0-1.0)
- `SetColors(stroke, fill)` - Customize stroke and fill colors

## Design Philosophy

1. **Header-Only:** Controls are header-only for easy inclusion
2. **Minimal Dependencies:** Only depend on IGraphics, no DSP code
3. **Customizable:** Colors and behavior can be customized
4. **Reusable:** Shared between testing (ComponentGallery) and production code

## Note on Built-in Controls

For standard controls like knobs and faders, we use iPlug2's built-in controls (`IVKnobControl`, `IVSliderControl`) which provide a good default appearance and can be styled later. Custom controls in this directory are for components that don't have built-in equivalents (like the ADSR envelope visualizer).

## Testing

All controls are tested in the ComponentGallery visual test harness:
```bash
./build_gallery.sh
./view_gallery.sh
```

Visual regression tests can be run with:
```bash
cd tests/Visual
npm run test
```
