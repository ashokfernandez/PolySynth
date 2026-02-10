#!/bin/bash
# ComponentGallery Visual Test Script
# Builds gallery assets, runs Storybook+Vitest checks, then Playwright snapshots.

set -e
cd "$(dirname "$0")"

./build_gallery.sh

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
