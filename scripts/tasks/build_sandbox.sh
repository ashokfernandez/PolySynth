#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

# 1. Check for dependencies
if [ ! -d "external/iPlug2" ]; then
    echo "Dependencies not found. Downloading..."
    ./scripts/download_dependencies.sh
fi

# 2. Detect generator
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
else
    GENERATOR="Unix Makefiles"
fi

CONFIG=${1:-Debug}
BUILD_DIR="ComponentGallery/build"

# 3. Configure
echo "Configuring PolySynthGallery ($CONFIG, $GENERATOR)..."
cmake_args=(
    -S ComponentGallery
    -B "$BUILD_DIR"
    -G "$GENERATOR"
    -DCMAKE_BUILD_TYPE="$CONFIG"
)

# Use ccache if available
if command -v ccache &>/dev/null; then
    cmake_args+=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cmake "${cmake_args[@]}"

# 4. Build
echo "Building PolySynthGallery..."
cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "Build Complete!"
echo "Run 'just sandbox-run' to launch."
