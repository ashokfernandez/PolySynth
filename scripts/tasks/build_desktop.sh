#!/usr/bin/env bash
# PolySynth Desktop Build Script
# This script performs a clean build of the standalone desktop application.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

# 1. Check for dependencies
if [ ! -d "external/iPlug2" ]; then
    echo "📦 Dependencies not found. Downloading..."
    ./scripts/download_dependencies.sh
fi

# 2. Clean build directory
echo "🧹 Cleaning build_desktop/ directory..."
rm -rf build_desktop
mkdir -p build_desktop

# 3. Configure with CMake
echo "⚙️ Configuring with CMake (Xcode generator)..."
cmake -S src/platform/desktop -B build_desktop -G Xcode

# 4. Build the APP target
CONFIG=${1:-Debug}
echo "🔨 Building PolySynth-app ($CONFIG)..."
cmake --build build_desktop --config "$CONFIG" --target PolySynth-app

echo ""
echo "✅ Build Complete!"
echo "📍 Location: $(find build_desktop/out -name "PolySynth.app" -type d | head -n 1)"
echo "🚀 Run 'just desktop-run' to launch."
