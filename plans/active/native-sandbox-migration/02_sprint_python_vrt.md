# Sprint 2: Python VRT Engine + Integration

**Depends on:** Sprint 1 (native sandbox builds and produces PNGs)
**Blocks:** Sprint 3 (CI pipeline needs VRT commands)
**PR Strategy:** Purely additive — adds VRT tooling and justfile commands. Old Storybook CI untouched.

---

## Task 2.1: Create Python VRT Diff Script

### What
Create `scripts/vrt_diff.py` — the pixel-comparison engine that compares current screenshots against committed baselines.

### Why
This replaces Playwright screenshot regression. It runs purely on PNG files produced by the native sandbox, with no browser dependency.

### Files to Create

**`scripts/vrt_diff.py`**

The script must implement:

#### Arguments
```
python3 scripts/vrt_diff.py \
  --baseline-dir tests/Visual/baselines \
  --current-dir tests/Visual/current \
  --output-dir tests/Visual/output \
  --tolerance 3.0 \
  --max-failed-pixels 10
```

- `--baseline-dir` (required): path to committed baseline PNGs
- `--current-dir` (required): path to current test PNGs
- `--output-dir` (optional, default `tests/Visual/output`): path for diff artifact images
- `--tolerance` (optional, default `3.0`): percentage threshold for pixel color distance (0-100 scale where 100 = max possible distance)
- `--max-failed-pixels` (optional, default `10`): maximum allowed failing pixels before an image pair fails

#### Configuration File (Single Source of Truth)

Create **`tests/Visual/vrt_config.json`**:

```json
{
  "tolerance": 3.0,
  "max_failed_pixels": 10,
  "expected_components": [
    "Envelope", "PolyKnob", "PolySection", "PolyToggle",
    "SectionFrame", "LCDPanel", "PresetSaveButton"
  ]
}
```

`vrt_diff.py` must:
1. Load defaults from `vrt_config.json` (auto-detect repo root or accept `--config` path)
2. Allow CLI flags `--tolerance` and `--max-failed-pixels` to override config values
3. Log which source provided each value (config file vs. CLI override)

This keeps tolerance constants checked-in and reviewable, not buried in script defaults or CI YAML.

#### Algorithm (per image pair)

```python
from PIL import Image
import math, sys, os

def pixel_distance(p1, p2):
    """Euclidean distance in RGBA space, normalized to 0-100%"""
    r = (p1[0] - p2[0]) ** 2
    g = (p1[1] - p2[1]) ** 2
    b = (p1[2] - p2[2]) ** 2
    a = (p1[3] - p2[3]) ** 2 if len(p1) > 3 and len(p2) > 3 else 0
    max_dist = math.sqrt(255**2 * 4)  # Maximum possible distance
    return (math.sqrt(r + g + b + a) / max_dist) * 100.0
```

For each PNG in `--current-dir`:
1. Find matching filename in `--baseline-dir`. If missing → **FAIL** (log "Missing baseline: X.png")
2. Load both as RGBA images
3. **Dimensional check**: if width or height mismatch → **FAIL** (log "Dimension mismatch: X.png baseline=WxH current=WxH")
4. Compare pixel-by-pixel:
   - Calculate color distance per pixel
   - If distance > `tolerance` → mark pixel as failed
5. Count total failed pixels
6. If failed count > `max-failed-pixels` → **FAIL** this image pair

#### Diff Artifact Generation (on failure)

When an image pair fails:
1. Convert baseline to grayscale (muted backdrop)
2. Overlay failed pixel locations in solid red (`#FF0000`)
3. Save as `diff_[OriginalFilename].png` in `--output-dir`

#### Exit Codes and Output

- Print summary for each image:
  ```
  PASS  Envelope.png (0 differing pixels)
  PASS  PolyKnob.png (3 differing pixels, within tolerance)
  FAIL  PolySection.png (847 differing pixels, max allowed: 10)
  ```
- If ALL pass → `sys.exit(0)`
- If ANY fail → print failing component names, `sys.exit(1)`

#### Also check for orphan baselines
If a baseline exists but no matching current image, log a warning (not a failure):
```
WARN  Orphan baseline: OldComponent.png (no matching current image)
```

### Acceptance Criteria
- [ ] Script runs with Python 3 and only requires `Pillow`
- [ ] Two identical PNGs → exit 0
- [ ] Sub-threshold noise → exit 0
- [ ] Significant pixel change → exit 1
- [ ] Diff artifact generated on failure with red pixel overlay
- [ ] Missing baseline → exit 1
- [ ] Dimension mismatch → exit 1
- [ ] Clear stdout summary of pass/fail per image

---

## Task 2.2: Create VRT Orchestration Script

### What
Create `scripts/tasks/run_vrt.sh` — the shell script that orchestrates VRT capture + comparison.

### Why
This is the single entry point for both generating baselines and running verification. It wraps the sandbox binary execution and Python diff script invocation.

### Files to Create

**`scripts/tasks/run_vrt.sh`**
```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

MODE="${1:---verify}"  # --generate or --verify

BASELINE_DIR="tests/Visual/baselines"
CURRENT_DIR="tests/Visual/current"
OUTPUT_DIR="tests/Visual/output"

# Find the sandbox binary
BINARY=$(find ComponentGallery/build -name "PolySynthGallery" -type f -executable 2>/dev/null | head -1)
if [ -z "$BINARY" ]; then
    echo "Error: PolySynthGallery binary not found. Run 'just sandbox-build' first."
    exit 1
fi

case "$MODE" in
  --generate)
    echo "=== Generating VRT Baselines ==="
    mkdir -p "$BASELINE_DIR"
    rm -f "$BASELINE_DIR"/*.png

    "$BINARY" --vrt-capture-mode --output-dir "$BASELINE_DIR"

    count=$(find "$BASELINE_DIR" -name "*.png" | wc -l)
    echo "Generated $count baseline images in $BASELINE_DIR/"
    ;;

  --verify)
    echo "=== Running VRT Verification ==="
    mkdir -p "$CURRENT_DIR" "$OUTPUT_DIR"
    rm -f "$CURRENT_DIR"/*.png "$OUTPUT_DIR"/*.png

    # Step 1: Capture current screenshots
    "$BINARY" --vrt-capture-mode --output-dir "$CURRENT_DIR"

    # Step 2: Run Python diff engine
    python3 scripts/vrt_diff.py \
      --baseline-dir "$BASELINE_DIR" \
      --current-dir "$CURRENT_DIR" \
      --output-dir "$OUTPUT_DIR"

    echo "=== VRT Verification Passed ==="
    ;;

  *)
    echo "Usage: run_vrt.sh [--generate|--verify]"
    exit 1
    ;;
esac
```

### Acceptance Criteria
- [ ] `run_vrt.sh --generate` produces baseline PNGs
- [ ] `run_vrt.sh --verify` captures + compares + passes when baselines match
- [ ] Script fails with clear error if sandbox binary not found
- [ ] Script cleans previous output before each run

---

## Task 2.3: Add VRT Justfile Commands

### What
Wire the VRT scripts into the Justfile and update existing command aliases.

### Why
Developers need muscle-memory commands: `just vrt-baseline`, `just vrt-run`, and updated `just quick-ui` / `just gallery-test`.

### Files to Modify

**`Justfile`** — Add after the sandbox commands:

```just
# Visual regression testing workflow
# Generate fresh VRT baseline screenshots from the native sandbox.
vrt-baseline:
    bash ./scripts/cli.sh run vrt-baseline -- ./scripts/tasks/run_vrt.sh --generate

# Run VRT: capture current screenshots and diff against baselines.
vrt-run:
    bash ./scripts/cli.sh run vrt-run -- ./scripts/tasks/run_vrt.sh --verify
```

**Update existing commands:**

```just
# Quick path: build gallery and open Storybook.
# UPDATED: now builds and runs native sandbox.
quick-ui:
    just sandbox-build
    just sandbox-run

# Run gallery visual regression test pipeline.
# UPDATED: now builds sandbox and runs VRT.
gallery-test:
    just sandbox-build
    just vrt-run
```

### Acceptance Criteria
- [ ] `just vrt-baseline` generates baselines
- [ ] `just vrt-run` captures + diffs
- [ ] `just quick-ui` builds and opens native sandbox
- [ ] `just gallery-test` builds + runs VRT
- [ ] All commands delegate through `cli.sh run` for logging

---

## Task 2.4: Create tests/Visual Directory Structure

### What
Set up the directory structure for VRT baselines, current captures, and diff output.

### Why
The VRT pipeline needs designated directories. Baselines are committed to git. Current and output are ephemeral.

### Files to Create/Modify

**`tests/Visual/baselines/.gitkeep`** — Empty file to ensure directory is tracked

**`tests/Visual/current/.gitkeep`** — Placeholder (directory will be .gitignored)

**`tests/Visual/output/.gitkeep`** — Placeholder (directory will be .gitignored)

**`.gitignore`** — Add:
```
# VRT ephemeral directories
tests/Visual/current/
tests/Visual/output/
```

Note: `tests/Visual/baselines/` is intentionally NOT ignored — baseline PNGs must be committed.

### Acceptance Criteria
- [ ] `tests/Visual/baselines/` exists and is tracked by git
- [ ] `tests/Visual/current/` is gitignored
- [ ] `tests/Visual/output/` is gitignored

---

## Task 2.5: Generate and Commit Initial Baselines

### What
Run the VRT baseline generation and commit the resulting PNG files.

### Why
Baselines must be committed so that future VRT runs have something to compare against. These are the "golden" reference images.

### Steps

```bash
# Build the sandbox (should already work from Sprint 1)
just sandbox-build

# Generate baselines
just vrt-baseline

# Verify baselines were created
ls -la tests/Visual/baselines/
# Expected: 7 PNG files (Envelope.png, PolyKnob.png, etc.)

# Verify VRT passes against itself
just vrt-run
# Expected: all PASS, exit 0

# Commit baselines
git add tests/Visual/baselines/*.png
git commit -m "feat: add initial VRT baseline screenshots"
```

### Acceptance Criteria
- [ ] 7 baseline PNGs committed to `tests/Visual/baselines/`
- [ ] Each PNG is non-empty (> 1KB)
- [ ] `just vrt-run` passes (baselines match themselves)
- [ ] PNG file names match: Envelope.png, PolyKnob.png, PolySection.png, PolyToggle.png, SectionFrame.png, LCDPanel.png, PresetSaveButton.png

---

## Task 2.6: Add requirements.txt for Python Dependencies

### What
Create a requirements file for the Python VRT dependencies.

### Why
CI needs to install Pillow deterministically. A requirements file ensures consistent versions.

### Files to Create

**`scripts/requirements-vrt.txt`**
```
Pillow>=10.0.0,<12.0.0
```

### Acceptance Criteria
- [ ] `pip install -r scripts/requirements-vrt.txt` succeeds
- [ ] `python3 scripts/vrt_diff.py --help` (or with args) works after installation

---

## Task 2.7: Automated Tests for vrt_diff.py

### What
Create a pytest test suite for the VRT diff engine using deterministic synthetic fixtures.

### Why
The VRT script is a critical gate. A bug in the diff logic silently passes regressions or blocks clean PRs. It must have its own tests.

### Files to Create

**`tests/test_vrt_diff.py`**

Use **synthetic PNG fixtures generated in test setup via Pillow** (no dependency on the sandbox binary). Required test cases:

1. **Identical images pass** — two identical 64x64 solid-color PNGs, assert exit 0
2. **Within-threshold drift passes** — pair differing by a few pixels below `max_failed_pixels`, assert exit 0
3. **Beyond-threshold modification fails** — pair with a large changed region, assert exit 1
4. **Missing baseline fails** — current image with no matching baseline, assert exit 1
5. **Diff artifact red-mask validation** — on a failing pair, load `diff_*.png` and assert pixels at known changed coordinates are red (`#FF0000`)

### Test Infrastructure
- Use `tmp_path` (pytest fixture) for all I/O
- Generate fixtures with `PIL.Image.new()` + `putpixel()` — deterministic, no external files
- Import `vrt_diff.py` comparison function directly or invoke as subprocess

### Acceptance Criteria
- [ ] `pytest tests/test_vrt_diff.py` passes
- [ ] All 5 test cases above are implemented
- [ ] No test depends on the sandbox binary or real baseline PNGs
- [ ] Tests complete in < 2 seconds

---

## Sprint 2 Definition of Done

Before creating PR-2, verify ALL of the following locally:

### VRT Pipeline End-to-End
```bash
# Full pipeline test:
just sandbox-build
just vrt-baseline
just vrt-run
# Expected: All components PASS

# Verify failure detection:
# (Manually edit a baseline PNG or use the diff script's test mode)
python3 -c "
from PIL import Image
img = Image.open('tests/Visual/baselines/PolyKnob.png')
for x in range(50, 60):
    for y in range(50, 60):
        img.putpixel((x,y), (0, 0, 0, 255))
img.save('tests/Visual/current/PolyKnob.png')
"
# Copy remaining baselines to current for the other components
cp tests/Visual/baselines/Envelope.png tests/Visual/current/
cp tests/Visual/baselines/PolySection.png tests/Visual/current/
cp tests/Visual/baselines/PolyToggle.png tests/Visual/current/
cp tests/Visual/baselines/SectionFrame.png tests/Visual/current/
cp tests/Visual/baselines/LCDPanel.png tests/Visual/current/
cp tests/Visual/baselines/PresetSaveButton.png tests/Visual/current/

python3 scripts/vrt_diff.py \
  --baseline-dir tests/Visual/baselines \
  --current-dir tests/Visual/current \
  --output-dir tests/Visual/output
# Expected: PolyKnob FAIL, all others PASS, exit code 1
# Expected: diff_PolyKnob.png exists in output with red pixels

# Clean up test artifacts
rm -rf tests/Visual/current/*.png tests/Visual/output/*.png
```

### Code Quality
- [ ] `cppcheck` passes on any new C++ code with no new suppressions
- [ ] No LLM reasoning-trace comments in committed source
- [ ] Every new header includes what it uses directly

### Regression Check
```bash
# Existing tests still pass:
just build && just test

# Old gallery commands still work (if Emscripten available)
```

### Build Parity Gate
- [ ] Native CMake path verified: `just sandbox-build` exits 0
- [ ] Existing native test/build path verified: `just build` + `just test` pass
- [ ] WAM demo path verified still intact (no CI packaging changes in this sprint — declare unaffected)
- [ ] Parallel project files (CMake/Xcode/WAM makefiles/workflows) updated or explicitly declared unaffected

### PR Checklist
- [ ] `scripts/vrt_diff.py` is committed and executable
- [ ] `scripts/tasks/run_vrt.sh` is committed and executable
- [ ] `scripts/requirements-vrt.txt` committed
- [ ] Baseline PNGs committed in `tests/Visual/baselines/`
- [ ] `.gitignore` updated for VRT directories
- [ ] Justfile updated with new commands
- [ ] PR title: "feat: add Python VRT engine and baseline screenshots"
- [ ] `tests/Visual/vrt_config.json` committed with tolerance defaults
- [ ] `pytest tests/test_vrt_diff.py` passes (all 5 VRT diff test cases green)
- [ ] No LLM reasoning-trace comments in committed source
