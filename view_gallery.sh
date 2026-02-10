#!/bin/bash
# ComponentGallery View Script
# Launches Storybook and opens the ComponentGallery story.

set -e

# Ensure we are in the project root
cd "$(dirname "$0")"

PORT=${1:-6006}
BUILD_INDEX="ComponentGallery/build-web/index.html"
VISUAL_DIR="tests/Visual"
STORY_PATH="/?path=/story/componentgallery-ui-components--knob"
URL="http://localhost:${PORT}${STORY_PATH}"

if [ ! -f "$BUILD_INDEX" ]; then
  echo "Gallery build output not found. Running build first..."
  ./build_gallery.sh
fi

if [ ! -d "${VISUAL_DIR}/node_modules" ]; then
  echo "Installing visual dependencies..."
  if [ -s "$HOME/.nvm/nvm.sh" ]; then
    # shellcheck disable=SC1090
    source "$HOME/.nvm/nvm.sh"
    nvm use 22 >/dev/null || true
  fi
  (cd "${VISUAL_DIR}" && npm ci)
fi

if [ -s "$HOME/.nvm/nvm.sh" ]; then
  # shellcheck disable=SC1090
  source "$HOME/.nvm/nvm.sh"
  nvm use 22 >/dev/null || true
fi

echo "Starting Storybook at ${URL}"
echo "Press Ctrl+C to stop."

if [ -z "${CI:-}" ] && [ -z "${NO_BROWSER:-}" ]; then
  if command -v open >/dev/null 2>&1; then
    (sleep 1; open "$URL" >/dev/null 2>&1 || true) &
  elif command -v xdg-open >/dev/null 2>&1; then
    (sleep 1; xdg-open "$URL" >/dev/null 2>&1 || true) &
  fi
fi

cd "${VISUAL_DIR}"
npx storybook dev -p "$PORT" --ci
