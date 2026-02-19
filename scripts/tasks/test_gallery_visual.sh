#!/usr/bin/env bash
# ComponentGallery Visual Test Script
# Builds gallery assets, runs Storybook+Vitest checks, then Playwright snapshots.

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

./scripts/tasks/build_gallery.sh

if [ -s "$HOME/.nvm/nvm.sh" ]; then
  # shellcheck disable=SC1090
  source "$HOME/.nvm/nvm.sh"
  nvm use 22 >/dev/null || true
fi

if [ ! -d "tests/Visual/node_modules" ]; then
  (cd tests/Visual && npm ci)
fi

(cd tests/Visual && npm run test:stories)
(cd tests/Visual && npm test)
