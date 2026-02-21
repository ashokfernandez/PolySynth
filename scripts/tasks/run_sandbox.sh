#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="ComponentGallery/build"
# Prefer the .app bundle binary so macOS resolves Resources correctly
BINARY=$(find "$BUILD_DIR" -path "*.app/Contents/MacOS/PolySynthGallery" -type f 2>/dev/null | grep -v "\.appex" | head -1)

# Fallback: Linux/Windows standalone (no .app bundle)
if [ -z "$BINARY" ]; then
    BINARY=$(find "$BUILD_DIR" -name "PolySynthGallery" -type f 2>/dev/null | grep -v "\.vst3" | grep -v "\.clap" | grep -v "\.component" | grep -v "\.appex" | head -1)
fi

if [ -z "$BINARY" ]; then
    echo "Error: PolySynthGallery binary not found. Run 'just sandbox-build' first."
    exit 1
fi

BINARY="$ROOT_DIR/$BINARY"

# cd into the app bundle's Resources so iPlug2 can find fonts relative to CWD
if [[ "$BINARY" == *.app/Contents/MacOS/* ]]; then
    RESOURCE_DIR="$(dirname "$BINARY")/../Resources"
else
    RESOURCE_DIR="$(dirname "$BINARY")"
fi

echo "Launching PolySynthGallery..."
cd "$RESOURCE_DIR"
exec "$BINARY" "$@"
