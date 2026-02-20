#!/usr/bin/env bash
# ComponentGallery Build Script
# Builds all per-component gallery pages used by Storybook iframe stories.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

if [ ! -d "external/iPlug2" ]; then
  echo "Dependencies not found. Downloading iPlug2..."
  ./scripts/download_iPlug2.sh
fi

# Ensure WAM SDK bits exist (some iPlug2 clones only contain placeholders).
if [ ! -f "external/iPlug2/Dependencies/IPlug/WAM_SDK/wamsdk/processor.cpp" ] || [ ! -f "external/iPlug2/Dependencies/IPlug/WAM_AWP/audioworklet.js" ]; then
  echo "WAM SDK files missing. Refreshing iPlug2 dependencies..."
  ./scripts/download_iPlug2.sh
fi

# Auto-activate emsdk if installed but not sourced.
if ! command -v emmake >/dev/null 2>&1; then
  if [ -f "$HOME/.emsdk/emsdk_env.sh" ]; then
    export EMSDK_QUIET=1
    # shellcheck disable=SC1091
    source "$HOME/.emsdk/emsdk_env.sh" >/dev/null
  fi
fi

if ! command -v emmake >/dev/null 2>&1; then
  echo "Error: emmake not found. Install Emscripten and re-run this script."
  echo "Quick setup:"
  echo "  git clone https://github.com/emscripten-core/emsdk.git ~/.emsdk"
  echo "  ~/.emsdk/emsdk install latest"
  echo "  ~/.emsdk/emsdk activate latest"
  echo "  source ~/.emsdk/emsdk_env.sh"
  exit 1
fi

./scripts/build_all_galleries.sh
python3 ./scripts/tasks/check_gallery_story_assets.py

echo ""
echo "Build complete."
echo "View it with: just gallery-view"
