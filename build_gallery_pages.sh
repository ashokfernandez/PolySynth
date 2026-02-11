#!/bin/bash
# Build ComponentGallery and publish static Storybook into docs/component-gallery.

set -e
cd "$(dirname "$0")"

./scripts/build_all_galleries.sh

if [ -s "$HOME/.nvm/nvm.sh" ]; then
  # shellcheck disable=SC1090
  source "$HOME/.nvm/nvm.sh"
  nvm use 22 >/dev/null || true
fi

if [ ! -d "tests/Visual/node_modules" ]; then
  (cd tests/Visual && npm ci)
fi

(cd tests/Visual && npm run build-storybook)

mkdir -p docs/component-gallery
cp -R tests/Visual/storybook-static/. docs/component-gallery/

echo ""
echo "Component gallery published locally to:"
echo "  docs/component-gallery/index.html"
echo "View it over HTTP (required for Storybook assets):"
echo "  ./view_gallery_pages.sh"
