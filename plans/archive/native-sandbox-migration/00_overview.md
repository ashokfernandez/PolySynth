# Native iPlug2 UI Sandbox & Python-Driven VRT — Migration Overview

## Context

The current visual regression testing pipeline uses a Storybook/Playwright/Emscripten web stack. Each UI component is compiled to WebAssembly, loaded in a Storybook iframe, and screenshotted by Playwright in a headless Chromium browser. This approach suffers from:

- **Slow builds**: Each component must be compiled to WAM via Emscripten (minutes per component)
- **Flaky tests**: Browser-based screenshot comparisons are non-deterministic across environments
- **Heavy CI dependencies**: Node.js, Playwright, Chromium, Emscripten all required for visual tests
- **Poor developer experience**: Long iteration cycles for UI tweaks

## Target Architecture

Replace the entire web-based gallery with:

1. **Native C++ Standalone App** (`PolySynthGallery`) — an iPlug2 `APP` target that renders all UI components natively using Skia, with sidebar navigation for developer iteration
2. **Python VRT Engine** (`scripts/vrt_diff.py`) — a Pillow-based pixel diffing script that compares PNG screenshots against committed baselines
3. **Headless CI via xvfb** — the native app runs on Linux CI under X Virtual Framebuffer, producing deterministic screenshots without a display

## What Survives

- The **full WAM demo** (the playable PolySynth web plugin) continues to build via Emscripten for GitHub Pages
- All existing **DSP tests, linters, sanitizers** are completely unchanged
- The **macOS/Windows release builds** are unchanged

## What Gets Removed

- Storybook (package.json, stories, .storybook config)
- Playwright (tests, config, browser cache)
- Component WAM builds (per-component Emscripten compilation)
- Gallery-related shell scripts and justfile commands
- The `build-storybook-site` CI job

## Sprint Roadmap

| Sprint | PR | Title | Theme |
|--------|-----|-------|-------|
| 1 | PR-1 | C++ Sandbox App Foundation | Build the new native gallery app (additive — nothing removed) |
| 2 | PR-2 | Python VRT Engine + Integration | Add VRT tooling and wire up new justfile commands (additive) |
| 3 | PR-3 | CI/CD Pipeline Swap | Rewrite workflows to use native sandbox, stop running Storybook |
| 4 | PR-4 | Legacy Cleanup + Validation | Remove all deprecated files, scripts, and dependencies |

### Key Principle: Each PR Passes CI

- **Sprint 1 & 2** are purely additive — they add new files and commands without touching anything the current CI relies on. The existing Storybook pipeline continues to run.
- **Sprint 3** atomically swaps the CI pipeline — the new native VRT replaces Storybook/Playwright in one PR.
- **Sprint 4** removes dead code that is no longer referenced by any workflow.

### Cross-Sprint Guardrails (from Retrospective)

Every sprint PR must satisfy these before requesting review:

1. **Build parity** — verify all active build paths locally:
   - `just sandbox-build` (or equivalent native CMake path)
   - `just build` + `just test` (existing native test suite)
   - WAM demo path intact (where the sprint touches CI or docs packaging)
   - Parallel project files (CMake / Xcode / WAM makefiles / workflows) updated or explicitly declared unaffected
2. **`#if IPLUG_EDITOR` guards** — any new code that calls `GetUI()` or references IGraphics types must be wrapped
3. **Include what you use** — never rely on transitive includes; each header includes its own dependencies
4. **Centralized typography** — use `PolyTheme` font names; no hardcoded font strings
5. **No LLM artifacts** — no reasoning-trace comments, no "thinking out loud" in committed source
6. **Fix, do not suppress** — cppcheck findings on new code must be fixed, not baselined with suppressions

## Critical Files Reference

### Existing (to be modified)
- `ComponentGallery/ComponentGallery.cpp` — rewrite for sidebar nav + all components
- `ComponentGallery/ComponentGallery.h` — new enum/members for sidebar
- `ComponentGallery/config.h` — already has APP defines, may need adjustment
- `Justfile` — add new commands, later remove deprecated ones
- `.github/workflows/ci.yml` — remove storybook job, add native-ui-tests, update package-demo-site
- `.github/workflows/visual-tests.yml` — complete rewrite for xvfb
- `.github/workflows/build_headless.yml` — remove Storybook/Playwright steps

### New Files
- `ComponentGallery/CMakeLists.txt` — CMake target for native APP build
- `scripts/vrt_diff.py` — Python pixel-diffing engine
- `scripts/tasks/run_vrt.sh` — VRT orchestration script
- `scripts/tasks/build_sandbox.sh` — native sandbox build script
- `scripts/tasks/run_sandbox.sh` — sandbox launcher
- `tests/Visual/baselines/*.png` — committed baseline images
- `tests/Visual/vrt_config.json` — single source of truth for VRT tolerance constants (Sprint 2)

### To Be Deleted (Sprint 4)
- `tests/Visual/package.json`, `tests/Visual/package-lock.json`
- `tests/Visual/.storybook/` (entire directory)
- `tests/Visual/stories/` (entire directory)
- `tests/Visual/specs/` (entire directory)
- `tests/Visual/playwright.config.ts`, `tests/Visual/vitest.config.ts`, `tests/Visual/vitest.shims.d.ts`
- `scripts/tasks/build_gallery.sh`, `scripts/tasks/view_gallery.sh`
- `scripts/tasks/test_gallery_visual.sh`, `scripts/tasks/build_gallery_pages.sh`
- `scripts/tasks/view_gallery_pages.sh`
- `scripts/build_all_galleries.sh`, `scripts/build_single_gallery.sh`
- `scripts/build_gallery_wam.sh`
- `ComponentGallery/projects/` (WAM makefiles)
- `ComponentGallery/config/` (WAM web config)
- `ComponentGallery/scripts/compile_wam.sh`

## Reusable Assets

- **Roboto fonts** already bundled in `src/platform/desktop/resources/fonts/{Roboto-Regular.ttf, Roboto-Bold.ttf}` — copy or symlink into ComponentGallery resources
- **PolyTheme.h** already defines consistent colors, spacing, font sizes
- **iPlug2 CMake pattern** established in `src/platform/desktop/CMakeLists.txt` — use `include(${IPLUG2_DIR}/iPlug2.cmake)` + `iplug_add_plugin()`
- **cli.sh run pattern** in `scripts/cli.sh` — all new justfile commands should delegate through this
- **ccache pattern** from `scripts/dev.sh` — the sandbox build should support ccache for fast incremental builds
