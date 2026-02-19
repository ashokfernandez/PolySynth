#!/usr/bin/env bash
# PolySynth Desktop Launch Script
# This script launches the latest standalone desktop build.

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

# Search for the app in the build directory
APP_PATH=$(find build_desktop/out -name "PolySynth.app" -type d | head -n 1)

if [ -z "$APP_PATH" ] || [ ! -d "$APP_PATH" ]; then
    echo "❌ Error: PolySynth.app not found."
    echo "Please run 'just desktop-build' first."
    exit 1
fi

echo "🚀 Launching PolySynth from $APP_PATH..."
open "$APP_PATH"
