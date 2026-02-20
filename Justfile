set shell := ["bash", "-eu", "-o", "pipefail", "-c"]
set positional-arguments := true

# Show command help by default.
default:
    @just --list

# Show all available commands and their descriptions.
help:
    @just --list

# Core developer workflow (delegates to scripts/dev.sh)
# Download/refresh external dependencies (iPlug2, DaisySP, SDK assets).
deps *args:
    bash ./scripts/cli.sh run deps -- ./scripts/dev.sh deps {{args}}

# Configure and build C++ test targets (no sanitizers).
build *args:
    bash ./scripts/cli.sh run build -- ./scripts/dev.sh build {{args}}

# Force Ninja generator for test/sanitizer tasks.
build-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run build-ninja -- ./scripts/dev.sh build {{args}}

# Run unit tests. Pass-through example: `just test -- --filter "[engine]"`.
test *args:
    bash ./scripts/cli.sh run test -- ./scripts/dev.sh test {{args}}

# Force Ninja generator for unit tests.
test-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run test-ninja -- ./scripts/dev.sh test {{args}}

# Run static analysis (Cppcheck/Clang-Tidy based on local availability).
lint *args:
    bash ./scripts/cli.sh run lint -- ./scripts/dev.sh lint {{args}}

# Force Ninja generator for static analysis configure/build graph.
lint-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run lint-ninja -- ./scripts/dev.sh lint {{args}}

# Build and run tests under AddressSanitizer + UBSan.
asan *args:
    bash ./scripts/cli.sh run asan -- ./scripts/dev.sh asan {{args}}

# Force Ninja generator for ASan pipeline.
asan-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run asan-ninja -- ./scripts/dev.sh asan {{args}}

# Build and run tests under ThreadSanitizer.
tsan *args:
    bash ./scripts/cli.sh run tsan -- ./scripts/dev.sh tsan {{args}}

# Force Ninja generator for TSan pipeline.
tsan-ninja *args:
    POLYSYNTH_CMAKE_GENERATOR=Ninja bash ./scripts/cli.sh run tsan-ninja -- ./scripts/dev.sh tsan {{args}}

# Remove test build directories managed by scripts/dev.sh.
clean *args:
    bash ./scripts/cli.sh run clean -- ./scripts/dev.sh clean {{args}}

# Force Unix Makefiles generator (compat/debug fallback).
build-make *args:
    POLYSYNTH_CMAKE_GENERATOR="Unix Makefiles" bash ./scripts/cli.sh run build-make -- ./scripts/dev.sh build {{args}}

# Quick path: build + unit tests (fast local confidence check).
quick:
    just build
    just test

# Quick path: run the full local PR gate.
quick-pr:
    just ci-pr

# Quick path: rebuild and launch desktop app.
quick-desktop:
    just desktop-rebuild

# Quick path: build gallery and open Storybook.
quick-ui:
    just gallery-rebuild-view

# Quick path: inspect latest failure logs.
quick-logs:
    just logs-latest

# Quick local gate: lint + unit tests.
check:
    just lint
    just test

# Run both sanitizer pipelines locally.
check-sanitizers:
    just asan
    just tsan

# Approximate PR gate: lint + ASan + TSan + unit tests.
ci-pr:
    just lint
    just asan
    just tsan
    just test

# Desktop app workflow
# Build the standalone desktop app (default config: Debug).
desktop-build config="Debug":
    bash ./scripts/cli.sh run desktop-build -- ./scripts/tasks/build_desktop.sh {{config}}

# Launch latest built desktop app bundle.
desktop-run:
    bash ./scripts/cli.sh run desktop-run -- ./scripts/tasks/run_desktop.sh

# Rebuild then launch desktop app.
desktop-rebuild config="Debug":
    bash ./scripts/cli.sh run desktop-build -- ./scripts/tasks/build_desktop.sh {{config}}
    bash ./scripts/cli.sh run desktop-run -- ./scripts/tasks/run_desktop.sh

# Install built AU/VST3 into local macOS plugin folders.
install-local config="Debug":
    bash ./scripts/cli.sh run install-local -- ./scripts/tasks/install_local_macos.sh {{config}}

# Alias for install-local (kept for explicitness).
install-local-macos config="Debug":
    bash ./scripts/cli.sh run install-local-macos -- ./scripts/tasks/install_local_macos.sh {{config}}

# PolySynth web demo workflow
# Build and serve web demo locally (opens browser when possible).
web-demo:
    bash ./scripts/cli.sh run web-demo -- ./scripts/tasks/run_web.sh

# Build WAM demo bundle only (no local server).
wam-demo-build:
    bash ./scripts/cli.sh run wam-demo-build -- bash -lc 'cd src/platform/desktop && chmod +x scripts/makedist-web.sh && ./scripts/makedist-web.sh off'

# Component gallery workflow
# Build ComponentGallery WAM/web assets.
gallery-build:
    bash ./scripts/cli.sh run gallery-build -- ./scripts/tasks/build_gallery.sh

# Alias for gallery-build (kept for familiarity).
gallery-rebuild:
    bash ./scripts/cli.sh run gallery-rebuild -- ./scripts/tasks/build_gallery.sh

# Run Storybook dev server for component gallery.
gallery-view port="6006":
    bash ./scripts/cli.sh run gallery-view -- ./scripts/tasks/view_gallery.sh {{port}}

# Build gallery then open Storybook.
gallery-rebuild-view port="6006":
    bash ./scripts/cli.sh run gallery-build -- ./scripts/tasks/build_gallery.sh
    bash ./scripts/cli.sh run gallery-view -- ./scripts/tasks/view_gallery.sh {{port}}

# Build static Storybook pages into docs/component-gallery.
gallery-pages-build:
    bash ./scripts/cli.sh run gallery-pages-build -- ./scripts/tasks/build_gallery_pages.sh

# Serve docs/component-gallery over local HTTP.
gallery-pages-view port="8092":
    bash ./scripts/cli.sh run gallery-pages-view -- ./scripts/tasks/view_gallery_pages.sh {{port}}

# Run gallery visual regression test pipeline.
gallery-test:
    bash ./scripts/cli.sh run gallery-test -- ./scripts/tasks/test_gallery_visual.sh

# Show latest run summary and log file locations.
logs-latest:
    bash ./scripts/cli.sh latest
