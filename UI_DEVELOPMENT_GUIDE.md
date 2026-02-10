# PolySynth UI Development Guide

**For Agents Building on the Component Gallery System**

This guide helps you understand the UI component architecture established in Phase 2 and how to extend it for the main PolySynth plugin UI (Phase 3 and beyond).

---

## 📋 Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Component Library (`/UI/Controls`)](#component-library)
3. [Build System](#build-system)
4. [Testing & Verification](#testing--verification)
5. [Adding New Components](#adding-new-components)
6. [Main Plugin Integration (Phase 3)](#main-plugin-integration-phase-3)
7. [Graphics Backend (NanoVG)](#graphics-backend)
8. [Common Pitfalls](#common-pitfalls)
9. [Quick Reference](#quick-reference)

---

## Architecture Overview

### The Two-Part System

**Part 1: Component Gallery (`/ComponentGallery`)**
- Visual testing harness for UI components
- Each component built in isolation using preprocessor flags
- Rendered via WebAssembly for Storybook integration
- **Purpose:** Verify components work before using in main plugin

**Part 2: Main Plugin (`/src`)**
- Production audio plugin (PolySynth)
- Uses components from `/UI/Controls` library
- Multi-platform: VST3, AU, AAX, Standalone, Web
- **Purpose:** Actual synthesizer product

### Key Principle: **Verify in Gallery First, Then Use in Plugin**

```
┌─────────────────┐         ┌──────────────────┐
│  UI/Controls/   │────────▶│ ComponentGallery │ (Test Here)
│  Button.h       │         │                  │
│  Knob.h (ref)   │         └──────────────────┘
│  Fader.h (ref)  │                  │
│  Envelope.h     │                  ▼
└─────────────────┘         Storybook + Playwright
         │                   Visual Regression
         │
         ▼
┌──────────────────┐
│  src/PolySynth   │ (Use Here)
│  Main Plugin UI  │
└──────────────────┘
```

---

## Component Library (`/UI/Controls`)

### Current Components (Phase 2)

| Component | Type | Implementation | File |
|-----------|------|----------------|------|
| **Knob** | Built-in | `IVKnobControl` | _(Reference Only)_ |
| **Fader** | Built-in | `IVSliderControl` | _(Reference Only)_ |
| **Envelope** | Custom | `Envelope` class | `Envelope.h` |

### Design Philosophy

**Use Built-in iPlug2 Controls When Possible:**
- `IVKnobControl` - Rotary knobs
- `IVSliderControl` - Linear sliders (vertical/horizontal)
- `IVButtonControl` - Push buttons
- `IVSwitchControl` - Toggle switches
- `IVMeterControl` - Level meters

**Why?**
1. Battle-tested edge case handling
2. Accessible through iPlug2's `IVStyle` system for global styling
3. Platform-specific behaviors handled automatically
4. No custom drawing code to maintain

**Write Custom Controls Only When:**
1. No built-in equivalent exists (e.g., ADSR envelope visualizer)
2. Highly specialized behavior needed
3. Custom drawing is essential to the component's purpose

### Custom Component Template

```cpp
// UI/Controls/MyComponent.h
#pragma once
#include "IControl.h"

using namespace iplug;
using namespace igraphics;

class MyComponent : public IControl
{
public:
  MyComponent(const IRECT& bounds, int paramIdx = kNoParameter)
    : IControl(bounds, paramIdx)
  {
    mIgnoreMouse = paramIdx == kNoParameter; // Display-only if no param
  }

  void Draw(IGraphics& g) override
  {
    // Your drawing code here using NanoVG
    g.FillRect(COLOR_RED, mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    // Handle interaction if needed
  }
};
```

---

## Build System

### Gallery Build Process

**Three Isolated WASM Binaries:**
```bash
./build_gallery.sh
# Creates:
#   ComponentGallery/build-web/knob/index.html     (100KB knob only)
#   ComponentGallery/build-web/fader/index.html    (100KB fader only)
#   ComponentGallery/build-web/envelope/index.html (100KB envelope only)
```

**How It Works:**
1. `build_gallery.sh` → calls `scripts/build_all_galleries.sh`
2. `build_all_galleries.sh` → calls `scripts/build_single_gallery.sh` 3 times with flags:
   - `-DGALLERY_COMPONENT_KNOB=1`
   - `-DGALLERY_COMPONENT_FADER=1`
   - `-DGALLERY_COMPONENT_ENVELOPE=1`
3. `build_single_gallery.sh` → invokes Emscripten with preprocessor flag
4. `ComponentGallery.cpp` → Uses `#if defined(GALLERY_COMPONENT_KNOB)` to select component

### Key Build Flags

**In `ComponentGallery/config/ComponentGallery-web.mk`:**
```makefile
# Include UI controls
WEB_CFLAGS += -I../../UI/Controls -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

# Embed WASM in JS (no separate .wasm file)
WEB_LDFLAGS += -s SINGLE_FILE=1
```

**Critical:** `-s SINGLE_FILE=1` embeds WASM as base64 in the JavaScript file. This:
- ✅ Avoids MIME type issues with `.wasm` files
- ✅ Simplifies deployment (one file instead of two)
- ❌ Creates larger initial download (~700KB per component)

---

## Testing & Verification

### Two-Tier Testing Strategy

#### Tier 1: Interactive Testing (Storybook)

**Start the dev server:**
```bash
./view_gallery.sh
# Opens http://localhost:6006
```

**Manual checks:**
- [ ] Component renders in top-left corner (20px padding)
- [ ] Click and drag works for interactive controls
- [ ] No console errors
- [ ] Visual appearance matches expectations

**Location:** `ComponentGallery → Components → [Knob, Fader, Envelope]`

#### Tier 2: Visual Regression (Playwright)

**Run tests:**
```bash
cd tests/Visual
npm run test
```

**What it does:**
1. Navigates to Storybook pages programmatically
2. Waits for WASM to load and canvas to render
3. Captures screenshots
4. Compares against baseline images

**Test file:** `tests/Visual/specs/components.spec.ts`

**Baseline images:** `tests/Visual/specs/components.spec.ts-snapshots/`

### The Testing Workflow

```
┌────────────────────────────────────────────────────┐
│ 1. Write Component (UI/Controls/NewControl.h)     │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 2. Add to Gallery (ComponentGallery.cpp)          │
│    #elif defined(GALLERY_COMPONENT_NEWCONTROL)    │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 3. Update Build (scripts/build_all_galleries.sh)  │
│    Add: build_single_gallery "newcontrol"         │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 4. Build & Test Manually                          │
│    ./build_gallery.sh && ./view_gallery.sh        │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 5. Add Storybook Story                            │
│    tests/Visual/stories/component-gallery.stories │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 6. Add Playwright Test                            │
│    tests/Visual/specs/components.spec.ts          │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 7. Generate Baseline (if looks good)              │
│    cd tests/Visual && npm run test -- --update    │
└─────────────────┬──────────────────────────────────┘
                  ▼
┌────────────────────────────────────────────────────┐
│ 8. Use in Main Plugin (src/PolySynth.cpp)         │
└────────────────────────────────────────────────────┘
```

---

## Adding New Components

### Example: Adding a Button Component (Phase 3)

**Step 1: Decide Implementation**
```
Question: Does iPlug2 have a built-in button?
Answer: Yes! IVButtonControl and IVSwitchControl

Decision: Use IVButtonControl for momentary, IVSwitchControl for toggle
```

**Step 2: Add to Gallery** (`ComponentGallery/ComponentGallery.cpp`)
```cpp
#elif defined(GALLERY_COMPONENT_BUTTON)
  // Small button in top-left corner
  const float buttonWidth = 120.0f;
  const float buttonHeight = 40.0f;
  pGraphics->AttachControl(new IVButtonControl(
    IRECT(padding, padding, padding + buttonWidth, padding + buttonHeight),
    kParamDemoMode,
    "Demo"));
#endif
```

**Step 3: Update Build Script** (`scripts/build_all_galleries.sh`)
```bash
echo "4/4 Building Button gallery..."
build_single_gallery "button"
```

**Step 4: Add Parameter** (`ComponentGallery/ComponentGallery.h`)
```cpp
enum EParams {
  kParamKnob1 = 0,
  kParamFader1,
  kParamDemoMode,  // NEW!
  kNumParams
};
```

Initialize in `ComponentGallery.cpp`:
```cpp
GetParam(kParamDemoMode)->InitBool("Demo Mode", false);
```

**Step 5: Build and Verify**
```bash
./build_gallery.sh
./view_gallery.sh
# Check: ComponentGallery → Components → Button
```

**Step 6: Add Storybook Story** (`tests/Visual/stories/component-gallery.stories.js`)
```javascript
export const Button = {
  render: () => createGalleryFrame('button')
};
```

**Step 7: Add Playwright Test** (`tests/Visual/specs/components.spec.ts`)
```typescript
test('button component renders isolated', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--button&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');

  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });
  await page.waitForTimeout(500);

  await expect(page).toHaveScreenshot('button-component.png', {
    fullPage: true,
    maxDiffPixelRatio: 0.012
  });
});
```

**Step 8: Update Documentation** (`UI/Controls/README.md`)
```markdown
### Button
Toggle button for binary parameters (on/off states).

**Implementation:** Uses iPlug2's built-in `IVSwitchControl`
- Size: 120×40px
- Position: Top-left corner (20px padding)
- Interactive: Click to toggle state

**Usage:**
```cpp
pGraphics->AttachControl(new IVSwitchControl(
  IRECT(x, y, x+120, y+40),
  paramIdx,
  "Label"));
```

**Why built-in?** IVSwitchControl provides proper toggle behavior,
visual feedback, and accessibility out of the box.
```

---

## Main Plugin Integration (Phase 3)

### Critical Differences: Gallery vs. Main Plugin

| Aspect | ComponentGallery | PolySynth Plugin |
|--------|------------------|------------------|
| **Purpose** | Visual testing | Production audio plugin |
| **Build** | WebAssembly only | VST3, AU, AAX, Standalone, Web |
| **Components** | One at a time (isolated) | Many together (complex layout) |
| **Parameters** | Dummy test params | Real DSP parameters |
| **Include Path** | `-I../../UI/Controls` | `-I../../UI/Controls` (same!) |

### Phase 3 Layout Strategy

**Reference:** `App.jsx` (React UI) shows grid layout:
```
┌─────────────┬─────────────┬─────────────────────┐
│ Oscillators │   Filter    │      Envelope       │
│  [Knob]     │ [Cutoff]    │   [Visual Graph]    │
│             │ [Resonance] │  [A] [D] [S] [R]    │
└─────────────┴─────────────┴─────────────────────┘
                    [Demo Button]
```

**Implementation:** `src/platform/desktop/PolySynth.cpp`

```cpp
void PolySynth::OnLayout(IGraphics* pGraphics)
{
  pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);

  const float margin = 20.0f;
  const float moduleWidth = 300.0f;

  // Column 1: Oscillators
  float x = margin;
  pGraphics->AttachControl(new IVKnobControl(
    IRECT(x, 50, x+100, 150),
    kParamOscWave), kCtrlTagOscWave, "Oscillators");

  // Column 2: Filter
  x += moduleWidth;
  pGraphics->AttachControl(new IVKnobControl(
    IRECT(x, 50, x+80, 130),
    kParamCutoff), kCtrlTagCutoff, "Filter");
  pGraphics->AttachControl(new IVKnobControl(
    IRECT(x+100, 50, x+180, 130),
    kParamResonance), kCtrlTagResonance);

  // Column 3: Envelope
  x += moduleWidth;

  // Envelope visualizer (top)
  auto* pEnv = new Envelope(IRECT(x, 50, x+400, 200));
  pEnv->SetADSR(
    GetParam(kParamAttack)->Value(),
    GetParam(kParamDecay)->Value(),
    GetParam(kParamSustain)->Value(),
    GetParam(kParamRelease)->Value()
  );
  pGraphics->AttachControl(pEnv, kCtrlTagEnvelope);

  // ADSR faders (bottom)
  const float faderY = 220;
  const float faderWidth = 60;
  const float faderHeight = 150;
  const float faderSpacing = 80;

  pGraphics->AttachControl(new IVSliderControl(
    IRECT(x, faderY, x+faderWidth, faderY+faderHeight),
    kParamAttack), kCtrlTagAttackFader, "A");

  pGraphics->AttachControl(new IVSliderControl(
    IRECT(x+faderSpacing, faderY, x+faderSpacing+faderWidth, faderY+faderHeight),
    kParamDecay), kCtrlTagDecayFader, "D");

  // ... S and R faders

  // Demo button (footer)
  pGraphics->AttachControl(new IVSwitchControl(
    IRECT(PLUG_WIDTH-150, PLUG_HEIGHT-60, PLUG_WIDTH-30, PLUG_HEIGHT-20),
    kParamDemoMode), kCtrlTagDemoButton, "Start Demo");
}
```

### Critical: Wire ADSR Faders to Envelope Visual

**Problem:** In React, `useEffect` updates the envelope when params change. In C++, we use `OnParamChange()`.

**Solution:** `src/platform/desktop/PolySynth.cpp`

```cpp
void PolySynth::OnParamChange(int paramIdx)
{
  // Always call base class first
  IPlugAPIBase::OnParamChange(paramIdx);

  // Check if it's an ADSR parameter
  if (paramIdx >= kParamAttack && paramIdx <= kParamRelease)
  {
    // Update the envelope visualizer
    if (auto* pGraphics = GetUI())
    {
      if (auto* pEnv = pGraphics->GetControlWithTag(kCtrlTagEnvelope))
      {
        if (auto* pEnvelope = dynamic_cast<Envelope*>(pEnv))
        {
          pEnvelope->SetADSR(
            GetParam(kParamAttack)->Value(),
            GetParam(kParamDecay)->Value(),
            GetParam(kParamSustain)->Value(),
            GetParam(kParamRelease)->Value()
          );
        }
      }
    }
  }
}
```

**Key Points:**
1. Use **control tags** (e.g., `kCtrlTagEnvelope`) to find controls by ID
2. Call `GetControlWithTag()` to retrieve the control
3. `dynamic_cast` to the specific type to access custom methods
4. Call `SetADSR()` which internally calls `SetDirty(false)` to trigger redraw

### Parameter Definition

**File:** `src/platform/desktop/config.h` (or wherever your plugin defines params)

```cpp
enum EParams
{
  // Oscillator
  kParamOscWave = 0,     // 0=Sine, 1=Saw, 2=Square, 3=Triangle

  // Filter
  kParamCutoff,          // 20Hz - 20kHz (logarithmic)
  kParamResonance,       // 0.0 - 1.0

  // ADSR Envelope
  kParamAttack,          // 0.001s - 2.0s
  kParamDecay,           // 0.001s - 2.0s
  kParamSustain,         // 0.0 - 1.0
  kParamRelease,         // 0.001s - 5.0s

  // Demo
  kParamDemoMode,        // 0=Off, 1=On

  kNumParams
};

enum EControlTags
{
  kCtrlTagOscWave = 0,
  kCtrlTagCutoff,
  kCtrlTagResonance,
  kCtrlTagEnvelope,
  kCtrlTagAttackFader,
  kCtrlTagDecayFader,
  kCtrlTagSustainFader,
  kCtrlTagReleaseFader,
  kCtrlTagDemoButton,
  kNumCtrlTags
};
```

**Initialization:** `src/platform/desktop/PolySynth.cpp`

```cpp
PolySynth::PolySynth(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Oscillator
  GetParam(kParamOscWave)->InitEnum("Waveform", 0, 4, "", 0, "", "Sine", "Saw", "Square", "Triangle");

  // Filter
  GetParam(kParamCutoff)->InitFrequency("Cutoff", 1000.0, 20.0, 20000.0);
  GetParam(kParamResonance)->InitPercentage("Resonance", 0.0);

  // ADSR - Match React defaults
  GetParam(kParamAttack)->InitSeconds("Attack", 0.01, 0.001, 2.0);
  GetParam(kParamDecay)->InitSeconds("Decay", 0.1, 0.001, 2.0);
  GetParam(kParamSustain)->InitPercentage("Sustain", 0.7);
  GetParam(kParamRelease)->InitSeconds("Release", 0.3, 0.001, 5.0);

  // Demo
  GetParam(kParamDemoMode)->InitBool("Demo Mode", false);

  #if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    OnLayout(pGraphics);
  };
  #endif
}
```

---

## Graphics Backend (NanoVG)

### What You're Working With

**Graphics API:** NanoVG (vector graphics library)
- Lightweight, fast 2D vector graphics
- Renders to OpenGL ES 2.0 (web) / OpenGL (desktop)
- Similar API to HTML5 Canvas

**Key Compile Flags:**
```makefile
-DIGRAPHICS_NANOVG    # Use NanoVG backend
-DIGRAPHICS_GLES2     # Target OpenGL ES 2.0 (web-compatible)
```

### Drawing Primitives (Common Operations)

```cpp
void Draw(IGraphics& g) override
{
  // Filled rectangle
  g.FillRect(COLOR_BLUE, mRECT);

  // Stroked rectangle
  g.DrawRect(COLOR_WHITE, mRECT, nullptr, 2.0f);

  // Circle
  g.FillCircle(COLOR_RED, centerX, centerY, radius);

  // Line
  g.DrawLine(COLOR_BLACK, x1, y1, x2, y2, nullptr, thickness);

  // Text
  g.DrawText(mText, "Label", mRECT, nullptr, true); // true = vertical center

  // Custom path (for envelope shape)
  IBlend blend;
  g.PathClear();
  g.PathMoveTo(x1, y1);
  g.PathLineTo(x2, y2);
  g.PathLineTo(x3, y3);
  g.PathStroke(COLOR_BLUE, 2.0f, IStrokeOptions(), &blend);
  g.PathFill(COLOR_BLUE.WithOpacity(0.2), IFillOptions(), &blend);
}
```

### Performance Considerations

1. **Minimize `SetDirty(true)` calls:** Only redraw when state actually changes
2. **Use `mDirty` flag:** Check before expensive operations
3. **Cache calculations:** Don't recalculate envelope points on every frame
4. **Batch draws:** Draw all primitives in one `Draw()` call if possible

---

## Common Pitfalls

### ❌ Pitfall 1: Include Path Issues

**Problem:**
```cpp
#include "UI/Controls/Envelope.h"  // ❌ WRONG
```

**Solution:**
```cpp
#include "Envelope.h"  // ✅ CORRECT (because -I../../UI/Controls is set)
```

### ❌ Pitfall 2: Forgetting `#if IPLUG_EDITOR` Guards

**Problem:**
```cpp
void PolySynth::OnLayout(IGraphics* pGraphics) { ... }  // ❌ Compiles UI code in DSP-only builds
```

**Solution:**
```cpp
#if IPLUG_EDITOR
void PolySynth::OnLayout(IGraphics* pGraphics) { ... }  // ✅ Only compiled when UI is enabled
#endif
```

### ❌ Pitfall 3: Not Adding Component to All Required Files

**Checklist when adding a component:**
- [ ] Create/reference component in `UI/Controls/`
- [ ] Add to `ComponentGallery.cpp` with `#elif defined(GALLERY_COMPONENT_X)`
- [ ] Add parameter to `ComponentGallery.h` enum
- [ ] Initialize parameter in `ComponentGallery.cpp` constructor
- [ ] Add build step to `scripts/build_all_galleries.sh`
- [ ] Add Storybook story to `component-gallery.stories.js`
- [ ] Add Playwright test to `components.spec.ts`
- [ ] Update `UI/Controls/README.md` with documentation

### ❌ Pitfall 4: Wrong Storybook IDs

**Problem:**
```typescript
await page.goto('/iframe.html?id=componentgallery-ui-components--button');
// ❌ Old ID format (before we renamed to "Components")
```

**Solution:**
```typescript
await page.goto('/iframe.html?id=componentgallery-components--button');
// ✅ Matches export default { title: 'ComponentGallery/Components' }
```

### ❌ Pitfall 5: Assuming Built-in Controls Have Custom Methods

**Problem:**
```cpp
auto* pKnob = new IVKnobControl(...);
pKnob->SetColors(...);  // ❌ IVKnobControl doesn't have this method
```

**Solution:**
```cpp
// Use IVStyle for built-in controls
IVStyle style = DEFAULT_STYLE.WithColors(COLOR_BLUE, COLOR_WHITE);
auto* pKnob = new IVKnobControl(..., "", style);
```

### ❌ Pitfall 6: Not Calling `SetDirty()` After State Changes

**Problem:**
```cpp
void Envelope::SetADSR(float a, float d, float s, float r)
{
  mAttack = a;
  // ... ❌ Component doesn't redraw
}
```

**Solution:**
```cpp
void Envelope::SetADSR(float a, float d, float s, float r)
{
  mAttack = a;
  // ...
  SetDirty(false);  // ✅ Triggers redraw
}
```

---

## Quick Reference

### File Locations

```
PolySynth/
├── UI/Controls/              # Shared component library
│   ├── Envelope.h           # Custom ADSR visualizer
│   └── README.md            # Component documentation
│
├── ComponentGallery/         # Visual testing harness
│   ├── ComponentGallery.cpp # Layout with preprocessor selection
│   ├── ComponentGallery.h   # Parameter definitions
│   ├── config.h             # Plugin config (size, name, etc.)
│   ├── config/
│   │   └── ComponentGallery-web.mk  # Web build settings
│   └── build-web/           # Build output (generated)
│       ├── knob/
│       ├── fader/
│       └── envelope/
│
├── src/platform/desktop/     # Main PolySynth plugin
│   ├── PolySynth.cpp        # Main plugin implementation
│   ├── PolySynth.h          # Plugin header
│   └── config/
│       ├── PolySynth-web.mk # Web build settings
│       └── config.h         # Plugin config
│
├── tests/Visual/             # Storybook + Playwright tests
│   ├── stories/
│   │   └── component-gallery.stories.js  # Storybook stories
│   ├── specs/
│   │   └── components.spec.ts           # Playwright tests
│   └── .storybook/
│       └── main.js          # Storybook config (staticDirs)
│
└── scripts/
    ├── build_all_galleries.sh      # Build all component variants
    └── build_single_gallery.sh     # Build one variant with flag
```

### Command Cheat Sheet

```bash
# Build component gallery
./build_gallery.sh

# View gallery in Storybook (dev mode)
./view_gallery.sh
# Opens http://localhost:6006

# Run visual regression tests
cd tests/Visual
npm run test

# Update baseline screenshots (after visual changes)
cd tests/Visual
npm run test -- --update-snapshots

# Build single component variant (debugging)
./scripts/build_single_gallery.sh knob
```

### Parameter Types Quick Reference

```cpp
// Continuous (0.0 - 1.0)
GetParam(idx)->InitDouble("Name", 0.5, 0.0, 1.0, 0.01);

// Percentage (0% - 100%)
GetParam(idx)->InitPercentage("Name", 50.0);

// Frequency (with log scale)
GetParam(idx)->InitFrequency("Cutoff", 1000.0, 20.0, 20000.0);

// Time (seconds)
GetParam(idx)->InitSeconds("Attack", 0.1, 0.001, 5.0);

// Enum/Choice
GetParam(idx)->InitEnum("Wave", 0, 4, "", 0, "", "Sine", "Saw", "Square", "Tri");

// Boolean
GetParam(idx)->InitBool("Demo", false);
```

---

## Success Criteria for Phase 3

Before considering Phase 3 complete, verify:

### ✅ Component Gallery
- [ ] Button component added to gallery
- [ ] Button builds successfully (`./build_gallery.sh`)
- [ ] Button appears in Storybook under "ComponentGallery → Components → Button"
- [ ] Clicking button toggles its visual state
- [ ] Playwright test passes and baseline screenshot captured

### ✅ Main Plugin UI
- [ ] PolySynth plugin compiles without errors (all platforms: VST3, AU, Web)
- [ ] UI roughly matches React reference layout (3 columns + footer)
- [ ] All components render in correct positions
- [ ] **Critical:** Moving ADSR faders immediately updates envelope visualizer

### ✅ Interaction Tests
- [ ] Oscillator knob rotates and updates waveform
- [ ] Filter knobs adjust cutoff and resonance
- [ ] ADSR faders update envelope graph in real-time (no lag)
- [ ] Demo button toggles color/state when clicked
- [ ] All parameters save and recall correctly in DAW

### ✅ Documentation
- [ ] `UI/Controls/README.md` updated with Button documentation
- [ ] Any custom controls have usage examples
- [ ] Commit messages clearly describe changes

---

## Getting Help

### Debugging Component Rendering Issues

**Step 1:** Check browser console when viewing in Storybook
```javascript
// Look for errors like:
// - Failed to load WASM
// - Canvas not found
// - Parameter index out of range
```

**Step 2:** Verify build output exists
```bash
ls -la ComponentGallery/build-web/button/scripts/ComponentGallery-web.js
# Should be ~700KB (WASM embedded)
```

**Step 3:** Check preprocessor flag is set
```bash
# In build output, look for:
-DGALLERY_COMPONENT_BUTTON=1
```

**Step 4:** Verify component is in `ComponentGallery.cpp`
```cpp
#elif defined(GALLERY_COMPONENT_BUTTON)
  // Your component code
#endif
```

### Resources

- **iPlug2 Documentation:** https://iplug2.github.io/
- **iPlug2 Source (for reference):** `external/iPlug2/IGraphics/Controls/`
- **NanoVG Reference:** Check `external/iPlug2/Dependencies/IGraphics/NanoVG/`
- **Storybook Docs:** https://storybook.js.org/
- **Playwright Docs:** https://playwright.dev/

---

**Good luck building Phase 3! Remember: Verify in the gallery first, then integrate into the main plugin. 🚀**
