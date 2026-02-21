# Sprint 4: Legacy Cleanup + Validation

**Depends on:** Sprint 3 (CI no longer references old files)
**Blocks:** Nothing — this is the final sprint
**PR Strategy:** Purely subtractive — removes dead code and files. CI already runs the new pipeline.

---

## Task 4.1: Remove Deprecated Justfile Commands

### What
Remove the old gallery-related commands from the Justfile that were superseded in Sprint 2.

### Why
After Sprint 3, CI no longer uses these commands. They reference scripts that will be deleted in this sprint.

### Files to Modify

**`Justfile`** — Remove these command blocks entirely:

1. `gallery-build` (line ~148-149)
2. `gallery-rebuild` (line ~152-153)
3. `gallery-view port="6006"` (line ~156-157)
4. `gallery-rebuild-view port="6006"` (line ~160-162)
5. `gallery-pages-build` (line ~165-166)
6. `gallery-pages-view port="8092"` (line ~169-170)

Also remove the section comment `# Component gallery workflow` that preceded these commands.

**Verify** that `quick-ui` and `gallery-test` were already updated in Sprint 2 to use the new sandbox commands. If not, update them now.

### Acceptance Criteria
- [ ] No `gallery-build`, `gallery-rebuild`, `gallery-view`, `gallery-rebuild-view`, `gallery-pages-build`, `gallery-pages-view` commands exist in Justfile
- [ ] `just --list` shows sandbox/VRT commands but no old gallery commands
- [ ] `quick-ui` points to sandbox, not Storybook
- [ ] `gallery-test` points to VRT, not Playwright

---

## Task 4.2: Remove Storybook/Playwright Infrastructure from tests/Visual/

### What
Delete all Node.js, Storybook, and Playwright files from `tests/Visual/`.

### Why
These are completely unused after the CI migration. Keeping them creates confusion and dependency maintenance burden.

### Files to Delete

```
tests/Visual/package.json
tests/Visual/package-lock.json
tests/Visual/.storybook/main.js
tests/Visual/.storybook/preview.js
tests/Visual/.storybook/vitest.setup.ts
tests/Visual/stories/component-gallery.stories.js
tests/Visual/specs/smoke-test.spec.ts
tests/Visual/specs/smoke-test.spec.ts-snapshots/smoke-test.png
tests/Visual/specs/components.spec.ts
tests/Visual/playwright.config.ts
tests/Visual/vitest.config.ts
tests/Visual/vitest.shims.d.ts
```

### Directories to Delete
```
tests/Visual/.storybook/        (entire directory)
tests/Visual/stories/           (entire directory)
tests/Visual/specs/             (entire directory, including snapshots)
```

### What Remains in tests/Visual/
After cleanup, the directory should contain only:
```
tests/Visual/
  baselines/          # Committed baseline PNGs (from Sprint 2)
    Envelope.png
    PolyKnob.png
    PolySection.png
    PolyToggle.png
    SectionFrame.png
    LCDPanel.png
    PresetSaveButton.png
  current/            # Gitignored, ephemeral
  output/             # Gitignored, ephemeral
```

### Acceptance Criteria
- [ ] No `package.json` or `package-lock.json` in `tests/Visual/`
- [ ] No `.storybook/` directory
- [ ] No `stories/` directory
- [ ] No `specs/` directory
- [ ] No `playwright.config.ts`, `vitest.config.ts`, or `vitest.shims.d.ts`
- [ ] `baselines/` directory with PNG files still exists
- [ ] `current/` and `output/` directories exist but are gitignored

---

## Task 4.3: Remove Gallery WAM Build Scripts

### What
Delete all shell scripts related to building ComponentGallery WAM artifacts.

### Why
The gallery no longer builds as WAM. These scripts referenced Emscripten and Makefiles that are no longer needed.

### Files to Delete

```
scripts/tasks/build_gallery.sh
scripts/tasks/view_gallery.sh
scripts/tasks/test_gallery_visual.sh
scripts/tasks/build_gallery_pages.sh
scripts/tasks/view_gallery_pages.sh
scripts/build_all_galleries.sh
scripts/build_single_gallery.sh
scripts/build_gallery_wam.sh
scripts/tasks/check_gallery_story_assets.py   (if it exists — verify)
```

### Acceptance Criteria
- [ ] None of the above files exist in the repository
- [ ] No remaining script references `build_gallery` or `ComponentGallery-wam`
- [ ] `grep -r "build_gallery" scripts/` returns no matches
- [ ] `grep -r "storybook" scripts/` returns no matches

---

## Task 4.4: Remove ComponentGallery WAM Build Files

### What
Delete the Emscripten/WAM build configuration files from ComponentGallery.

### Why
The ComponentGallery now builds natively via CMake. The Makefile-based WAM build is dead code.

### Files to Delete

```
ComponentGallery/projects/ComponentGallery-wam-controller.mk
ComponentGallery/projects/ComponentGallery-wam-processor.mk
ComponentGallery/config/ComponentGallery-web.mk
ComponentGallery/scripts/compile_wam.sh
```

### Directories to Delete
```
ComponentGallery/projects/      (entire directory)
ComponentGallery/config/        (entire directory)
ComponentGallery/scripts/       (entire directory — only contained compile_wam.sh)
```

### What Remains in ComponentGallery/
After cleanup:
```
ComponentGallery/
  CMakeLists.txt          # New (Sprint 1)
  ComponentGallery.cpp    # Rewritten (Sprint 1)
  ComponentGallery.h      # Rewritten (Sprint 1)
  config.h                # Updated (Sprint 1)
  resources/
    fonts/
      Roboto-Regular.ttf
      Roboto-Bold.ttf
    .gitkeep
  build/                  # Gitignored build output
```

### Acceptance Criteria
- [ ] No Makefile (.mk) files in ComponentGallery
- [ ] No `scripts/` subdirectory in ComponentGallery
- [ ] No `config/` subdirectory in ComponentGallery
- [ ] No `projects/` subdirectory in ComponentGallery
- [ ] CMakeLists.txt and source files still present

---

## Task 4.5: Update .gitignore

### What
Clean up .gitignore to remove Storybook/Playwright-specific entries and ensure new paths are covered.

### Why
Dead entries create confusion. New paths need proper coverage.

### Files to Modify

**`.gitignore`**

#### Remove these entries (no longer relevant):
```
tests/Visual/node_modules/
tests/Visual/node_modules_backup_*/
tests/Visual/playwright-report/
tests/Visual/test-results/
tests/Visual/storybook-static/
tests/Visual/debug-storybook.log
docs/component-gallery/
```

#### Ensure these entries exist (some may already be covered):
```
# VRT ephemeral output (baselines are tracked)
tests/Visual/current/
tests/Visual/output/

# Native sandbox build
ComponentGallery/build/
```

#### Keep these entries (still relevant):
```
node_modules/           # Root-level, in case any other Node tooling exists
ComponentGallery/build-web/  # WAM build artifacts (can remove if no longer generated)
```

Actually, `ComponentGallery/build-web/` can also be removed from .gitignore since WAM builds are no longer happening. Remove it.

### Acceptance Criteria
- [ ] No references to `storybook-static`, `playwright-report`, or `test-results`
- [ ] VRT ephemeral directories are ignored
- [ ] `ComponentGallery/build/` is ignored
- [ ] No stale entries for files/directories that no longer exist

---

## Task 4.6: Remove React/UI Strategy Docs (If Requested)

### What
The `plans/active/ui_strategy.md` file references a "Web First" IWebView approach that is being deprecated. Consider archiving it.

### Why
The strategy has changed from web-first to native-first. Keeping the old strategy doc as "active" is misleading.

### Files to Modify

Move `plans/active/ui_strategy.md` → `plans/archive/ui_strategy.md`

### Acceptance Criteria
- [ ] `plans/active/ui_strategy.md` no longer exists
- [ ] `plans/archive/ui_strategy.md` exists with original content
- [ ] No broken references to the file

---

## Sprint 4 Definition of Done

Before creating PR-4, verify ALL of the following:

### Automated Structural Validation

Create **`scripts/tasks/check_migration_complete.sh`** and run it both locally and in CI. The script must assert ALL of the following and exit non-zero on any failure:

```bash
#!/usr/bin/env bash
set -euo pipefail

PASS=0; FAIL=0

assert_absent() {
  if [ -e "$1" ]; then echo "FAIL: $1 still exists"; FAIL=$((FAIL+1))
  else echo "PASS: $1 absent"; PASS=$((PASS+1)); fi
}

assert_no_match() {
  if grep -q "$2" "$1" 2>/dev/null; then echo "FAIL: '$2' found in $1"; FAIL=$((FAIL+1))
  else echo "PASS: '$2' absent from $1"; PASS=$((PASS+1)); fi
}

assert_match() {
  if grep -q "$2" "$1" 2>/dev/null; then echo "PASS: '$2' found in $1"; PASS=$((PASS+1))
  else echo "FAIL: '$2' missing from $1"; FAIL=$((FAIL+1)); fi
}

# Files/dirs that must NOT exist
assert_absent tests/Visual/package.json
assert_absent tests/Visual/package-lock.json
assert_absent tests/Visual/.storybook
assert_absent tests/Visual/stories
assert_absent tests/Visual/specs
assert_absent tests/Visual/playwright.config.ts
assert_absent ComponentGallery/projects
assert_absent ComponentGallery/config
assert_absent ComponentGallery/scripts
assert_absent scripts/tasks/build_gallery.sh
assert_absent scripts/build_all_galleries.sh
assert_absent scripts/build_gallery_wam.sh

# Justfile commands that must NOT exist
assert_no_match Justfile "^gallery-build"
assert_no_match Justfile "^gallery-view"
assert_no_match Justfile "^gallery-rebuild"
assert_no_match Justfile "^gallery-pages-build"
assert_no_match Justfile "^gallery-pages-view"

# CI workflow assertions
assert_no_match .github/workflows/ci.yml "build-storybook-site"
assert_match    .github/workflows/ci.yml "build-wam-demo"

# Baselines must exist (exactly 7 PNGs)
COUNT=$(find tests/Visual/baselines -name "*.png" 2>/dev/null | wc -l)
if [ "$COUNT" -eq 7 ]; then echo "PASS: 7 baseline PNGs"; PASS=$((PASS+1))
else echo "FAIL: expected 7 baseline PNGs, found $COUNT"; FAIL=$((FAIL+1)); fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] || exit 1
```

Run locally:
```bash
bash scripts/tasks/check_migration_complete.sh
# Expected: all assertions pass, exit 0
```

Add as a CI step in the `native-ui-tests` job so the migration contract is enforced going forward.

### Pipeline Still Works
```bash
# Full pipeline test:
just sandbox-build
just vrt-run
# Expected: PASS

# Unit tests unaffected:
just build && just test
# Expected: PASS
```

### Spec Validation (Automated)

All structural assertions from the migration spec are now covered by `check_migration_complete.sh` (above). Run it:

```bash
bash scripts/tasks/check_migration_complete.sh && echo "ALL ASSERTIONS PASS"
```

Additionally verify VRT capture integrity manually once:
```bash
just sandbox-run -- --vrt-capture-mode --output-dir /tmp/vrt-assertion
test $(find /tmp/vrt-assertion -name "*.png" | wc -l) -eq 7 && echo "PASS: 7 PNGs" || echo "FAIL"
for f in /tmp/vrt-assertion/*.png; do
    size=$(stat -f%z "$f" 2>/dev/null || stat -c%s "$f" 2>/dev/null)
    test "$size" -gt 0 && echo "PASS: $f ($size bytes)" || echo "FAIL: $f is empty"
done
```

### Build Parity Gate
- [ ] Native CMake path verified: `just sandbox-build` exits 0
- [ ] Existing native test/build path verified: `just build` + `just test` pass
- [ ] WAM demo path verified still intact: CI `build-wam-demo` unchanged
- [ ] Parallel project files (CMake/Xcode/WAM makefiles/workflows) updated or explicitly declared unaffected

### PR Checklist
- [ ] Only file deletions and .gitignore updates in this PR
- [ ] No new code added
- [ ] Baseline PNGs preserved
- [ ] PR title: "chore: remove deprecated Storybook/Playwright visual testing infrastructure"
- [ ] PR description lists all removed files and references the migration spec
- [ ] `scripts/tasks/check_migration_complete.sh` committed and executable
- [ ] `check_migration_complete.sh` passes locally
- [ ] `check_migration_complete.sh` added as a CI step
- [ ] No LLM reasoning-trace comments in committed source
