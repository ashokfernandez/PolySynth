---
description: Complete workflow for implementing, testing, and demonstrating new features
---

# Feature Complete Workflow

This workflow ensures every new feature is properly built, tested, demonstrated to the user, and documented with audio examples before being considered complete.

## Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    FEATURE COMPLETE WORKFLOW                    │
├─────────────────────────────────────────────────────────────────┤
│  1. BUILD    →  2. TEST    →  3. DEMO    →  4. CONFIRM  →  5. DOCUMENT  │
│  (Code)         (Verify)      (Show)        (Approve)       (Publish)   │
└─────────────────────────────────────────────────────────────────┘
```

A feature is NOT complete until all 5 steps are finished and the user has confirmed the demo works.

---

## Step 1: BUILD

Implement the feature in the appropriate location:

```bash
# Core DSP changes
src/core/

# Platform integration
src/platform/desktop/

# UI changes
src/platform/desktop/resources/web/src/
```

### Build Commands

```bash
# Build desktop app
cd src/platform/desktop
cmake -B build
cmake --build build --target PolySynth-app

# Build React UI
cd src/platform/desktop/resources/web
npm run build
```

---

## Step 2: TEST

Write and run tests to verify correctness:

### C++ Unit Tests

```bash
# Add tests to tests/unit/Test_*.cpp
# Build and run
cd tests && cmake -B build && cmake --build build
./build/run_tests
```

### JavaScript Tests

```bash
cd src/platform/desktop/resources/web
npm test
```

### What to Test
- [ ] New parameters work correctly
- [ ] State persistence (save/load)
- [ ] UI updates when state changes
- [ ] No audio glitches or artifacts
- [ ] Edge cases (min/max values)

---

## Step 3: DEMO

Launch the app and demonstrate the feature to the user:

```bash
# Open the built app
open ~/Applications/PolySynth.app
```

### Demo Checklist
- [ ] Show the feature working visually
- [ ] Play audio to demonstrate sound changes
- [ ] Test all controls related to the feature
- [ ] Verify UI updates correctly
- [ ] Test save/load if applicable

---

## Step 4: CONFIRM

**Wait for explicit user confirmation that the demo works correctly.**

The user may report issues like:
- "I can hear the change but the knobs don't move"
- "The filter doesn't seem to do anything"
- "There's a clicking sound when I play notes"

If issues are reported:
1. Debug and fix the issue
2. Rebuild
3. Re-demo
4. Wait for confirmation again

**DO NOT proceed until the user says it's working.**

---

## Step 5: DOCUMENT

Update the demo documentation page with audio examples:

### 1. Add a new demo executable (if needed)

Create `tests/demos/demo_<feature>.cpp`:

```cpp
#include "../utils/WavWriter.h"
#include "Engine.h"

int main() {
    // Render audio showcasing the feature
    // Save to demo_<feature>.wav
}
```

Add to `tests/CMakeLists.txt`:

```cmake
add_executable(demo_<feature> demos/demo_<feature>.cpp)
```

### 2. Add description in the report generator

Edit `scripts/generate_test_report.py`, add to `DEMO_DESCRIPTIONS`:

```python
"demo_<feature>": {
    "title": "🎵 Feature Name",
    "description": "What this demo shows...",
    "technical": "Technical details about DSP...",
    "listen_for": "What the user should notice..."
}
```

### 3. Regenerate docs

```bash
# Build demos
cd tests/build && make

# Generate report
python3 scripts/generate_test_report.py
```

### 4. Commit and push

```bash
git add -A
git commit -m "Add demo for <feature>"
git push origin main
```

The demo page will update at: https://ashokfernandez.github.io/PolySynth/

---

## Example: Implementing a New Filter Type

```
1. BUILD
   - Add filter to src/core/dsp/va/NewFilter.h
   - Add parameter to SynthState.h
   - Wire up UI control in App.jsx

2. TEST
   - Add Test_NewFilter.cpp
   - Run: ./tests/build/run_tests "[filter]"
   - Verify: all tests pass

3. DEMO
   - Open ~/Applications/PolySynth.app
   - Show filter controls
   - Play notes while adjusting cutoff/resonance
   - User sees knobs move, hears filter change

4. CONFIRM
   - User: "Sounds great, filter is working!"
   - ✅ Proceed to documentation

5. DOCUMENT
   - Create demo_new_filter.cpp
   - Add description to generate_test_report.py
   - Push changes
   - Demo page shows new audio sample
```

---

## Why This Matters

This workflow ensures:
- **Quality**: Features are tested before release
- **User Trust**: The user sees working demos, not just code
- **Documentation**: Future users can hear what each feature does
- **Regression Safety**: Audio artifacts can be compared over time
- **Completeness**: A feature isn't "done" until it's proven to work
