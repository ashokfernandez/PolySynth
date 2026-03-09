# Sprint 3: CI/CD Pipeline Swap

**Depends on:** Sprints 1 and 2 (sandbox builds, VRT works locally)
**Blocks:** Sprint 4 (cleanup can only happen after CI stops referencing old files)
**PR Strategy:** Atomic swap — CI workflows are updated in a single PR. After this merges, the old Storybook pipeline is no longer executed.

---

## Task 3.1: Rewrite `visual-tests.yml` (PR Gate)

### What
Completely replace the Storybook/Playwright visual regression workflow with a native C++ sandbox + Python VRT workflow running under xvfb on Ubuntu.

### Why
This is the main PR gate for visual regressions. It must run on every pull request and block merge if visual regressions are detected.

### Files to Modify

**`.github/workflows/visual-tests.yml`** — Complete rewrite

```yaml
name: PR Visual Regression

on:
  pull_request:

env:
  CCACHE_DIR: ~/.cache/ccache

jobs:
  native-visual-regression:
    name: Native Visual Regression
    runs-on: ubuntu-latest

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install C++ build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build ccache python3-pip

      - name: Install xvfb and X11/GL dependencies
        run: |
          sudo apt-get install -y \
            xvfb \
            libxcursor-dev \
            libxrandr-dev \
            libxinerama-dev \
            libxi-dev \
            libgl1-mesa-dev

      - name: Restore ccache
        uses: actions/cache@v4
        with:
          path: ~/.cache/ccache
          key: ${{ runner.os }}-ccache-sandbox-${{ hashFiles('ComponentGallery/CMakeLists.txt') }}
          restore-keys: |
            ${{ runner.os }}-ccache-sandbox-

      - name: Download project external dependencies
        run: |
          chmod +x scripts/download_dependencies.sh
          ./scripts/download_dependencies.sh

      - name: Install Python VRT dependencies
        run: pip install -r scripts/requirements-vrt.txt

      - name: Build PolySynthGallery sandbox
        run: |
          cmake -S ComponentGallery -B ComponentGallery/build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build ComponentGallery/build --parallel $(nproc)

      - name: Run VRT under xvfb
        run: |
          xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
            bash scripts/tasks/run_vrt.sh --verify

      - name: Upload diff artifacts on failure
        if: failure()
        uses: actions/upload-artifact@v4
        with:
          name: vrt-diff-reports
          path: tests/Visual/output/
          if-no-files-found: ignore

      - name: Show ccache stats
        if: always()
        run: ccache -s
```

### Key Differences from Old Workflow
| Old (Storybook) | New (Native) |
|---|---|
| 2 jobs (build WAM → run Playwright) | 1 job (build sandbox → run VRT) |
| Emscripten + Node.js + Playwright + Chromium | CMake + Ninja + Python + Pillow + xvfb |
| WAM artifact passed between jobs | Single job, no artifact transfer |
| Playwright screenshot → visual diff | Skia PNG capture → Python pixel diff |
| Minutes to build WAM per component | Seconds for native C++ compile |

### Acceptance Criteria
- [ ] Workflow triggers on `pull_request`
- [ ] xvfb is installed and used to run the sandbox headlessly
- [ ] ccache is restored for fast rebuilds
- [ ] VRT pass → job passes (green check)
- [ ] VRT fail → job fails, diff artifacts uploaded as `vrt-diff-reports`
- [ ] No references to Emscripten, Node.js, npm, Playwright, or Storybook
- [ ] Workflow name stays `PR Visual Regression` (or similar) for GitHub status check continuity

---

## Task 3.2: Update `ci.yml` — Remove Storybook, Add Native UI Tests

### What
Modify the main CI workflow to:
1. Delete the `build-storybook-site` job entirely
2. Add a `native-ui-tests` job before `build-macos` / `build-windows`
3. Update `package-demo-site` to remove Storybook artifact dependency

### Why
The Storybook job is no longer needed. The new native-ui-tests job provides the visual regression gate on the main branch. The demo site no longer includes a component gallery Storybook.

### Files to Modify

**`.github/workflows/ci.yml`**

#### Change 1: Delete Job 9 (`build-storybook-site`)

Remove the entire `build-storybook-site` job (lines ~470-523 of current file).

#### Change 2: Add `native-ui-tests` job

Insert a new job in the dependency graph. It should depend on `native-dsp-tests` (same as the old storybook job) and gate `build-macos` and `build-windows`:

```yaml
  # ---------------------------------------------------------------------------
  # Job 9 (NEW): Native UI Visual Regression Tests
  # Ensures no visual regressions before platform builds.
  # ---------------------------------------------------------------------------
  native-ui-tests:
    name: Native UI Tests
    runs-on: ubuntu-latest
    needs: native-dsp-tests

    steps:
      - uses: actions/checkout@v4

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build ccache python3-pip \
            xvfb libxcursor-dev libxrandr-dev libxinerama-dev \
            libxi-dev libgl1-mesa-dev

      - name: Restore ccache
        uses: actions/cache@v4
        with:
          path: ~/.cache/ccache
          key: ${{ runner.os }}-ccache-sandbox-${{ hashFiles('ComponentGallery/CMakeLists.txt') }}
          restore-keys: |
            ${{ runner.os }}-ccache-sandbox-

      - name: Download project external dependencies
        run: |
          chmod +x scripts/download_dependencies.sh
          ./scripts/download_dependencies.sh

      - name: Install Python VRT dependencies
        run: pip install -r scripts/requirements-vrt.txt

      - name: Build PolySynthGallery sandbox
        run: |
          cmake -S ComponentGallery -B ComponentGallery/build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build ComponentGallery/build --parallel $(nproc)

      - name: Run VRT under xvfb
        run: |
          xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
            bash scripts/tasks/run_vrt.sh --verify

      - name: Upload diff artifacts on failure
        if: failure()
        uses: actions/upload-artifact@v4
        with:
          name: vrt-diff-reports
          path: tests/Visual/output/
          if-no-files-found: ignore

      - name: Show ccache stats
        if: always()
        run: ccache -s
```

#### Change 3: Update dependency graph

Update `build-macos` and `build-windows` to also depend on `native-ui-tests`:

```yaml
  build-macos:
    needs: [native-dsp-tests, native-ui-tests]
    # ...

  build-windows:
    needs: [native-dsp-tests, native-ui-tests]
    # ...
```

#### Change 4: Update `package-demo-site`

Remove the dependency on `build-storybook-site` and all references to the Storybook artifact:

```yaml
  package-demo-site:
    name: Package Pages Site
    runs-on: ubuntu-latest
    needs: [build-wam-demo]  # CHANGED: was [build-wam-demo, build-storybook-site]
    # ...
```

Remove these steps from `package-demo-site`:
- "Download Storybook artifact"
- Any reference to `docs/component-gallery`

The remaining steps:
1. Download `polysynth-web-demo` artifact into `docs/web-demo`
2. Generate download metadata
3. Create `.nojekyll`
4. Upload Pages artifact

#### Change 5: Update dependency graph comment

Update the ASCII art comment at the top of the file to reflect the new graph:

```
#   lint-static-analysis ──┐
#                          ├──> native-dsp-tests ──> native-ui-tests ──> build-macos ──┐
#   build-asan-tests ──────┤                     |                   ├──> build-windows ├──> publish-release
#   build-tsan-tests ──────┘                     └──> build-wam-demo
#                                                          └──> package-demo-site
#                                                                    └──> publish-pages
```

### Acceptance Criteria
- [ ] `build-storybook-site` job completely removed
- [ ] `native-ui-tests` job added with xvfb + VRT pipeline
- [ ] `build-macos` and `build-windows` depend on `native-ui-tests`
- [ ] `package-demo-site` depends only on `build-wam-demo` (not Storybook)
- [ ] `package-demo-site` no longer downloads or stages Storybook artifact
- [ ] `build-wam-demo` (Job 8) completely unchanged
- [ ] `publish-pages` unchanged
- [ ] Dependency graph comment updated

---

## Task 3.3: Update `build_headless.yml` (Manual Workflow)

### What
Remove Storybook/Playwright steps from the manual headless validation workflow and add native VRT steps.

### Why
This workflow is used for manual full-pipeline validation. It must reflect the new architecture.

### Files to Modify

**`.github/workflows/build_headless.yml`**

#### Remove these steps:
- "Install visual test dependencies (npm)" (`npm ci` in tests/Visual)
- "Restore Playwright browser cache"
- "Install Playwright system dependencies"
- "Ensure Playwright Chromium is available"
- "Run component gallery visual test pipeline" (`test_gallery_visual.sh`)
- "Build Storybook static site" (`npm run build-storybook`)
- "Upload Storybook static artifact"
- Any staging of Storybook into `docs/component-gallery`

#### Remove these environment variables:
- `PLAYWRIGHT_BROWSERS_PATH`

#### Remove Node.js setup step:
- The "Setup Node" step (only needed for Storybook/Playwright)

#### Add these steps (after native test steps):

```yaml
      - name: Install xvfb and X11/GL dependencies
        run: |
          sudo apt-get install -y \
            xvfb libxcursor-dev libxrandr-dev libxinerama-dev \
            libxi-dev libgl1-mesa-dev python3-pip

      - name: Install Python VRT dependencies
        run: pip install -r scripts/requirements-vrt.txt

      - name: Build PolySynthGallery sandbox
        run: |
          cmake -S ComponentGallery -B ComponentGallery/build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build ComponentGallery/build --parallel $(nproc)

      - name: Run VRT under xvfb
        run: |
          xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
            bash scripts/tasks/run_vrt.sh --verify
```

#### Update docs staging step:
Remove the line that copies Storybook into `docs/component-gallery/`:
```yaml
      - name: Stage web demo into docs/
        run: |
          rm -rf docs/web-demo
          mkdir -p docs/web-demo
          cp -R src/platform/desktop/build-web/. docs/web-demo/
          touch docs/.nojekyll
```

### Acceptance Criteria
- [ ] No references to npm, Node.js, Playwright, or Storybook
- [ ] No `PLAYWRIGHT_BROWSERS_PATH` env variable
- [ ] Native VRT runs under xvfb
- [ ] WAM demo build still works
- [ ] docs/ staging only includes web-demo (no component-gallery)

---

## Sprint 3 Definition of Done

Before creating PR-3, verify the following:

### Local CI Simulation
```bash
# Verify sandbox still builds:
just sandbox-build
just vrt-run
# Expected: PASS

# Verify existing tests unaffected:
just build && just test
# Expected: PASS
```

### Build Parity Gate
- [ ] Native CMake path verified: `just sandbox-build` exits 0
- [ ] Existing native test/build path verified: `just build` + `just test` pass
- [ ] WAM demo path verified still intact: `build-wam-demo` job unchanged in ci.yml
- [ ] Parallel project files (CMake/Xcode/WAM makefiles/workflows) updated or explicitly declared unaffected

### Workflow YAML Validation
```bash
# Validate YAML syntax of all workflow files:
python3 -c "
import yaml
for f in ['.github/workflows/ci.yml', '.github/workflows/visual-tests.yml', '.github/workflows/build_headless.yml']:
    with open(f) as fh:
        yaml.safe_load(fh)
    print(f'{f}: valid YAML')
"
```

### Manual Review Checklist
- [ ] `visual-tests.yml` has no Storybook/Playwright references
- [ ] `ci.yml` has no `build-storybook-site` job
- [ ] `ci.yml` has `native-ui-tests` job with correct dependencies
- [ ] `ci.yml` `package-demo-site` no longer references Storybook artifact
- [ ] `ci.yml` `build-wam-demo` (Job 8) is completely unchanged
- [ ] `build_headless.yml` has no npm/Node/Playwright steps
- [ ] Dependency graph comment in `ci.yml` is updated

### CI Transition Proof Criteria
- [ ] Native visual workflow passes on at least one intentional UI-touching validation change (e.g., trivially adjust a component color, push, confirm VRT detects it)
- [ ] Failure-path validation: intentionally break a baseline, push, confirm VRT job fails AND diff artifacts are uploaded and downloadable from `vrt-diff-reports`
- [ ] Main CI dependency graph verified: `native-ui-tests` gates `build-macos` and `build-windows`; `build-storybook-site` is absent

### PR Checklist
- [ ] Only workflow YAML files are modified in this PR
- [ ] No source code changes
- [ ] PR title: "ci: migrate visual regression to native C++ sandbox + Python VRT"
- [ ] PR description lists the workflow changes and links to the spec
- [ ] No LLM reasoning-trace comments in committed source
- [ ] WAM demo build verified locally or declared untouched with justification

### CI Behavior After Merge
- PRs trigger `native-visual-regression` job (not Playwright)
- Main branch CI runs `native-ui-tests` before platform builds
- GitHub Pages only deploys web-demo (no component gallery Storybook)
- Build failures produce downloadable diff artifacts
