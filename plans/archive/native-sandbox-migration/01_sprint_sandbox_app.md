# Sprint 1: C++ Sandbox App Foundation

**Depends on:** Nothing
**Blocks:** Sprints 2, 3, 4
**PR Strategy:** Purely additive — no existing files are removed. Old Storybook CI continues to pass.

---

## Task 1.1: Create CMakeLists.txt for PolySynthGallery

### What
Create a CMake build definition for the `PolySynthGallery` standalone application target. This must support Linux (Ninja), macOS (Xcode or Ninja), and work headlessly under xvfb.

### Why
The current ComponentGallery has no CMake target — it only builds via Emscripten Makefiles. A native CMake target is the foundation for everything else.

### Files to Create

**`ComponentGallery/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.14)
project(PolySynthGallery VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# UI controls live at repo root UI/Controls/
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../UI/Controls
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    ComponentGallery.cpp
    ComponentGallery.h
    config.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    resources/fonts/Roboto-Bold.ttf
)
```

### Key Design Decisions
- The IPLUG2_DIR path goes up one level from ComponentGallery/ to find external/iPlug2
- UI/Controls are included from the repo root (same headers the main plugin uses)
- SEA_DSP is NOT linked — the gallery has no audio processing dependency
- Roboto fonts are bundled as resources for deterministic rendering

### Files to Create/Copy

**`ComponentGallery/resources/fonts/Roboto-Regular.ttf`** — Copy from `src/platform/desktop/resources/fonts/Roboto-Regular.ttf`
**`ComponentGallery/resources/fonts/Roboto-Bold.ttf`** — Copy from `src/platform/desktop/resources/fonts/Roboto-Bold.ttf`

### Acceptance Criteria
- [ ] `cmake -S ComponentGallery -B ComponentGallery/build -G Ninja` configures successfully on Linux
- [ ] `cmake --build ComponentGallery/build` compiles with zero errors
- [ ] The build does NOT require or link SEA_DSP, SEA_Util, or any audio engine headers
- [ ] Roboto font files are copied into the resources directory

---

## Task 1.2: Rewrite ComponentGallery.h for Runtime Component Selection

### What
Replace the preprocessor-based single-component selection with runtime data structures that support showing all components via sidebar navigation.

### Why
The current design compiles one component per build via `#ifdef GALLERY_COMPONENT_*`. The new app must show all components at runtime with sidebar selection.

### Files to Modify

**`ComponentGallery/ComponentGallery.h`**

Replace the entire file with a new class definition that:

1. Defines an enum of testable component IDs:
   ```cpp
   enum EGalleryComponent {
     kComponentEnvelope = 0,
     kComponentPolyKnob,
     kComponentPolySection,
     kComponentPolyToggle,
     kComponentSectionFrame,
     kComponentLCDPanel,
     kComponentPresetSaveButton,
     kNumComponents
   };
   ```

2. Adds a parameter for the state injection slider:
   ```cpp
   enum EParams {
     kParamKnobPrimary = 0,
     kParamKnobSecondary,
     kParamToggleMono,
     kParamTogglePoly,
     kParamToggleFX,
     kParamStateSlider,  // NEW: master 0-1 state injection
     kNumParams
   };
   ```

3. Adds class members for:
   - `int mCurrentComponent = kComponentPolyKnob;` — currently visible component
   - `std::vector<IControl*> mComponentControls[kNumComponents];` — controls grouped by component
   - `std::vector<IControl*> mSidebarButtons;` — sidebar navigation buttons
   - `bool mVRTCaptureMode = false;` — set via CLI flag
   - `std::string mVRTOutputDir;` — output directory for VRT captures
   - Method declarations: `void ShowComponent(int idx);`, `void RunVRTCapture();`, `void SaveComponentPNG(int idx, const std::string& outputDir);`

### Acceptance Criteria
- [ ] Header compiles without errors
- [ ] All 7 component IDs are defined
- [ ] State injection parameter is declared
- [ ] VRT capture mode flag and output dir are exposed

---

## Task 1.3: Rewrite ComponentGallery.cpp — Sidebar Layout + All Components

### What
Completely rewrite the `OnLayout` function to create a persistent sidebar with navigation buttons and a main viewing area where all components are instantiated but hidden by default.

### Why
The spec requires: "All UI components to be tested are instantiated and attached to the main viewing area. All components except the default one are immediately set to hidden."

### Files to Modify

**`ComponentGallery/ComponentGallery.cpp`**

The new `OnLayout(IGraphics* pGraphics)` must:

1. **Attach panel background** (dark gray, matching PolyTheme)

2. **Create sidebar** (left 180px):
   - For each component in `EGalleryComponent`, create a clickable text button
   - Button click handler calls `ShowComponent(idx)` which:
     - Iterates all component control groups
     - Calls `Hide(true)` on all except the selected
     - Calls `Hide(false)` on the selected
     - Sets `mCurrentComponent = idx`

3. **Create main viewing area** (right side, 180px to PLUG_WIDTH):
   - Instantiate ALL 7 component variants in the main area (same control code as current `#ifdef` blocks, but all at once)
   - Register each control into its `mComponentControls[idx]` group
   - Call `Hide(true)` on all except `kComponentPolyKnob` (default)

4. **Create state injection slider** (bottom bar):
   - An `IVSliderControl` or `IVKnobControl` mapped to `kParamStateSlider`
   - Range 0.0–1.0
   - In `OnParamChange` for this param, push the value to the currently visible component

5. **Component instantiation** — port each `#ifdef` block to a function like:
   ```cpp
   void ComponentGallery::CreateEnvelopeControls(IGraphics* pGraphics, IRECT bounds) {
     Envelope* pEnvelope = new Envelope(bounds);
     pEnvelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
     pGraphics->AttachControl(pEnvelope);
     mComponentControls[kComponentEnvelope].push_back(pEnvelope);
   }
   ```

6. **Font enforcement**: Load Roboto from bundled resources, not system fonts:
   ```cpp
   pGraphics->LoadFont(ROBOTO_FN, "Regular");
   ```

### Component Layout Reference

Port these from the existing `#ifdef` blocks (already working code):
- **Envelope**: 400x150 canvas
- **PolyKnob**: 3 variants (default red, cyan accent, bare no-label)
- **PolySection**: 2 panels ("FILTER", "LFO")
- **PolyToggle**: 3 buttons ("MONO", "POLY", "FX")
- **SectionFrame**: single 380x220 frame ("MOD MATRIX")
- **LCDPanel**: 320x72 panel with text overlay
- **PresetSaveButton**: 2 variants (dirty + clean states)

### Acceptance Criteria
- [ ] All 7 components instantiated on startup
- [ ] Only one component visible at a time (default: PolyKnob)
- [ ] Sidebar buttons switch visible component
- [ ] State slider visible at bottom
- [ ] No system font strings used (only bundled Roboto)

---

## Task 1.4: CLI Argument Parsing (`--vrt-capture-mode`)

### What
Add command-line argument parsing to the `PolySynthGallery` standalone app. When `--vrt-capture-mode` is passed, the app enters automated capture mode instead of interactive mode.

### Why
CI needs to run the app headlessly, capture all component PNGs, and exit without human interaction.

### Files to Modify

**`ComponentGallery/ComponentGallery.cpp`**

In the constructor or an early initialization hook:

1. Parse `argc/argv` for `--vrt-capture-mode` and optional `--output-dir <path>` flags
2. If VRT mode detected, set `mVRTCaptureMode = true` and `mVRTOutputDir`
3. After layout is complete, if `mVRTCaptureMode` is true, call `RunVRTCapture()`

**Note on iPlug2 APP argument access:** The standalone app's entry point passes args to the Plugin constructor via `InstanceInfo`. The implementation agent must determine the correct way to access CLI args in the iPlug2 APP context. Options:
- Parse `argc/argv` from the global scope before plugin instantiation
- Use platform-specific APIs (not preferred)
- Set via environment variable as a fallback (`POLYSYNTH_VRT_MODE=1`)

The `--output-dir` defaults to `tests/Visual/current/` relative to the repository root if not specified.

### Acceptance Criteria
- [ ] Running `./PolySynthGallery --vrt-capture-mode` does not open an interactive window (or opens and immediately closes)
- [ ] Running without the flag opens the normal interactive gallery window
- [ ] `--output-dir` flag customizes where PNGs are written

---

## Task 1.5: Skia PNG Export Implementation

### What
Implement the `SaveComponentPNG()` method that captures the rendered Skia frame of a specific component's bounding rectangle and writes it as a PNG file.

### Why
This is the core of VRT — deterministic pixel-perfect screenshots of each component for comparison.

### Files to Modify

**`ComponentGallery/ComponentGallery.cpp`**

Implement `SaveComponentPNG(int componentIdx, const std::string& outputDir)`:

1. Call `ShowComponent(componentIdx)` to make the target visible
2. Force a full redraw: `GetUI()->SetAllControlsDirty()` + render
3. Access the IGraphics Skia backing store

**Approach for pixel access** (implementation agent should validate with iPlug2 source):

Option A — Use `IGraphics::GetDrawBitmap()`:
```cpp
// Get the backing bitmap from IGraphics
LICE_IBitmap* bitmap = pGraphics->GetDrawBitmap();
// Extract pixel region for the component's IRECT
// Encode with stb_image_write or Skia encoder
```

Option B — Use Skia-specific APIs (if IGraphicsSkia is used):
```cpp
// Access the SkCanvas and SkSurface
// Create an SkImage from the surface
// Use SkPngEncoder to write the component region
```

Option C — Use iPlug2's built-in screenshot methods if available

4. Calculate the component's bounding IRECT from `mComponentControls[componentIdx]`
5. Crop to that region and save as `[ComponentName].png` in the output directory

**File naming convention:**
```
Envelope.png
PolyKnob.png
PolySection.png
PolyToggle.png
SectionFrame.png
LCDPanel.png
PresetSaveButton.png
```

### Implement `RunVRTCapture()`:

```cpp
void ComponentGallery::RunVRTCapture() {
  std::string outputDir = mVRTOutputDir.empty() ? "tests/Visual/current" : mVRTOutputDir;
  // Ensure output directory exists (use std::filesystem or platform mkdir)

  static const char* kComponentNames[] = {
    "Envelope", "PolyKnob", "PolySection", "PolyToggle",
    "SectionFrame", "LCDPanel", "PresetSaveButton"
  };

  for (int i = 0; i < kNumComponents; i++) {
    SaveComponentPNG(i, outputDir + "/" + kComponentNames[i] + ".png");
  }

  exit(0);  // Terminate after capture
}
```

### Deterministic Rendering Contract

To guarantee pixel-identical output across runs:

1. **Software raster backend** — when VRT capture mode is active and the Skia backend supports it, force the CPU (software) rasterizer. GPU-rendered frames vary across drivers.
2. **Warmup frames** — before capturing each component, render 2-3 throwaway frames (`SetAllControlsDirty()` + draw cycle) to flush animation/first-paint transients, then snapshot on the next frame.
3. **Fixed capture geometry** — capture region is the component's `IRECT` at 1x scale factor. Document this assumption at the capture call site. Do not rely on window DPI.

### Capture Abstraction

Isolate all capture logic in **one helper** (e.g., `VRTCaptureHelper`):

- **Preferred path:** direct Skia `SkSurface` readback via `SkPngEncoder`
- **Fallback path:** `IGraphics::GetDrawBitmap()` or framework-level screenshot export

The rest of the codebase calls only the helper; it never touches Skia surface internals directly. This keeps backend assumptions in one place and makes a future backend swap a single-file change.

### Acceptance Criteria
- [ ] Running with `--vrt-capture-mode` produces 7 PNG files
- [ ] Each PNG has non-zero file size
- [ ] PNGs are correctly cropped to the component bounding box (not full window)
- [ ] PNG dimensions match the expected component sizes
- [ ] PNGs are identical across multiple runs on the same machine (deterministic)
- [ ] App exits with code 0 after capture completes
- [ ] Software rasterizer selected in VRT mode (if backend supports it)
- [ ] 2-3 warmup frames rendered before each capture
- [ ] All capture logic lives in a single helper (no Skia surface access elsewhere)

---

## Task 1.6: Update config.h for Sandbox Needs

### What
Review and update `ComponentGallery/config.h` to ensure it properly supports the native APP build target.

### Why
The existing config.h is set up for WAM builds. While it has APP defines, we should verify all settings are correct for native rendering.

### Files to Modify

**`ComponentGallery/config.h`**

Review and ensure:
- `PLUG_NAME` → `"PolySynthGallery"` (or keep `"ComponentGallery"`)
- `PLUG_HAS_UI 1` — already set
- `PLUG_WIDTH 1024` and `PLUG_HEIGHT 768` — already set, keep these
- `PLUG_FPS 60` — already set
- `APP_NUM_CHANNELS 2` — already set
- `ROBOTO_FN "Roboto-Regular.ttf"` — already defined
- Consider adding `#define ROBOTO_BOLD_FN "Roboto-Bold.ttf"` if the bold variant is used

### Acceptance Criteria
- [ ] config.h compiles in APP context without errors
- [ ] Font defines match bundled resource files

---

## Task 1.7: Build Script + Justfile Commands

### What
Add a new build script and justfile commands for the native sandbox.

### Why
Developers need `just sandbox-build` and `just sandbox-run` to work.

### Files to Create

**`scripts/tasks/build_sandbox.sh`**
```bash
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

# 1. Check for dependencies
if [ ! -d "external/iPlug2" ]; then
    echo "Dependencies not found. Downloading..."
    ./scripts/download_dependencies.sh
fi

# 2. Detect generator
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
else
    GENERATOR="Unix Makefiles"
fi

CONFIG=${1:-Debug}
BUILD_DIR="ComponentGallery/build"

# 3. Configure
echo "Configuring PolySynthGallery ($CONFIG, $GENERATOR)..."
cmake_args=(
    -S ComponentGallery
    -B "$BUILD_DIR"
    -G "$GENERATOR"
    -DCMAKE_BUILD_TYPE="$CONFIG"
)

# Use ccache if available
if command -v ccache &>/dev/null; then
    cmake_args+=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cmake "${cmake_args[@]}"

# 4. Build
echo "Building PolySynthGallery..."
cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "Build Complete!"
echo "Run 'just sandbox-run' to launch."
```

**`scripts/tasks/run_sandbox.sh`**
```bash
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="ComponentGallery/build"
# Find the binary (name depends on iPlug2 target output)
BINARY=$(find "$BUILD_DIR" -name "PolySynthGallery" -type f -executable 2>/dev/null | head -1)

if [ -z "$BINARY" ]; then
    echo "Error: PolySynthGallery binary not found. Run 'just sandbox-build' first."
    exit 1
fi

echo "Launching PolySynthGallery..."
exec "$BINARY" "$@"
```

### Files to Modify

**`Justfile`** — Add these new commands (insert after the desktop app workflow section, before the gallery section):

```just
# UI Sandbox workflow
# Build the native PolySynthGallery standalone app.
sandbox-build config="Debug":
    bash ./scripts/cli.sh run sandbox-build -- ./scripts/tasks/build_sandbox.sh {{config}}

# Launch the native PolySynthGallery sandbox for interactive development.
sandbox-run *args:
    bash ./scripts/cli.sh run sandbox-run -- ./scripts/tasks/run_sandbox.sh {{args}}
```

### Files to Modify

**`.gitignore`** — Add `ComponentGallery/build/` to ensure native build artifacts are ignored. Check: already covered by `build*/` glob? The existing `.gitignore` has `build*/` which matches `build/` at any level, but `ComponentGallery/build-web/` is explicitly listed. Add `ComponentGallery/build/` for clarity.

### Acceptance Criteria
- [ ] `just sandbox-build` configures and compiles on Linux (Ninja) and macOS (Ninja or Makefiles)
- [ ] `just sandbox-run` launches the gallery window (on systems with a display)
- [ ] ccache is used when available
- [ ] Build output is in `ComponentGallery/build/`

---

## Sprint 1 Definition of Done

Before creating PR-1, verify ALL of the following locally:

### Build Verification
```bash
# On Linux (or macOS):
just sandbox-build
# Expected: exit code 0, binary exists in ComponentGallery/build/

# Interactive test (only on machine with display):
just sandbox-run
# Expected: Window opens showing sidebar + PolyKnob default view
# Click sidebar items to verify navigation works

# VRT capture test:
just sandbox-run -- --vrt-capture-mode --output-dir /tmp/vrt-test
# Expected: 7 PNG files in /tmp/vrt-test/, app exits 0
ls -la /tmp/vrt-test/
# Expected: Envelope.png, PolyKnob.png, etc., all non-empty
```

### CI Compatibility Check
```bash
# Existing tests must still pass:
just build && just test
# Expected: no regressions

# Existing gallery commands still work (not yet removed):
# just gallery-build  (if Emscripten available)
```

### Build Parity Gate
- [ ] Native CMake path verified: `just sandbox-build` exits 0
- [ ] Existing native test/build path verified: `just build` + `just test` pass
- [ ] WAM demo path verified still intact (no CI packaging changes in this sprint — declare unaffected)
- [ ] Parallel project files (CMake/Xcode/WAM makefiles/workflows) updated or explicitly declared unaffected

### Code Quality
- [ ] No system font strings (`"Arial"`, `"Helvetica"`, etc.) anywhere in gallery code
- [ ] No SEA_DSP or audio engine includes in gallery CMake or source
- [ ] All new shell scripts have `set -euo pipefail`
- [ ] New shell scripts are executable (`chmod +x`)
- [ ] All `GetUI()` calls guarded with `#if IPLUG_EDITOR` / `#endif`
- [ ] Every header includes what it uses (no reliance on transitive includes)
- [ ] No LLM reasoning-trace comments in committed source
- [ ] `cppcheck` passes on new code with no new suppressions
- [ ] Font names use `PolyTheme` constants (no hardcoded `"Roboto-Regular"` strings outside theme)

### PR Checklist
- [ ] All new files committed
- [ ] Font files copied to `ComponentGallery/resources/fonts/`
- [ ] `.gitignore` updated if needed
- [ ] PR title: "feat: add native PolySynthGallery sandbox app"
- [ ] PR description references this sprint plan
