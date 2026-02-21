#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="ComponentGallery/build"
# Find the binary (name depends on iPlug2 target output)
BINARY=$(find "$BUILD_DIR" -name "PolySynthGallery" -type f -executable 2>/dev/null | head -1)

if [ -z "$BINARY" ]; then
    echo "Error: PolySynthGallery binary not found. Run 'just sandbox-build' first."
    exit 1
fi

echo "Launching PolySynthGallery..."
exec "$BINARY" "$@"
