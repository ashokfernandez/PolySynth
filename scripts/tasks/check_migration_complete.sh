#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

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
