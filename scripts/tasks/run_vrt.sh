#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

MODE="${1:---verify}"  # --generate or --verify
ROOT_DIR="$(pwd)"
BASELINE_DIR="$ROOT_DIR/tests/Visual/baselines"
CURRENT_DIR="$ROOT_DIR/tests/Visual/current"
OUTPUT_DIR="$ROOT_DIR/tests/Visual/output"

# Find the sandbox binary
BINARY=$(find ComponentGallery/build -name "PolySynthGallery" -type f 2>/dev/null | grep -E "(\.app/Contents/MacOS/PolySynthGallery$|[^/]$)" | grep -v "\.vst3" | grep -v "\.clap" | grep -v "\.component" | grep -v "\.appex" | head -1)
if [ -z "$BINARY" ]; then
    echo "Error: PolySynthGallery binary not found. Run 'just sandbox-build' first."
    exit 1
fi
BINARY="$ROOT_DIR/$BINARY"

# Change directory to the app bundle's Resources folder so that iPlug2 can find the fonts relative to CWD
if [[ "$BINARY" == *.app/Contents/MacOS/* ]]; then
    RESOURCE_DIR="$(dirname "$BINARY")/../Resources"
else
    # Linux/Windows standalone
    RESOURCE_DIR="$(dirname "$BINARY")"
fi


case "$MODE" in
  --generate)
    echo "=== Generating VRT Baselines ==="
    mkdir -p "$BASELINE_DIR"
    rm -f "$BASELINE_DIR"/*.png

    pushd "$RESOURCE_DIR" > /dev/null
    POLYSYNTH_VRT_MODE=1 POLYSYNTH_VRT_OUTPUT_DIR="$BASELINE_DIR" "$BINARY" || exit 1
    popd > /dev/null

    count=$(find "$BASELINE_DIR" -name "*.png" | wc -l)
    echo "Generated $count baseline images in $BASELINE_DIR/"
    ;;

  --verify)
    echo "=== Running VRT Verification ==="
    
    if [ ! -d "$BASELINE_DIR" ] || [ -z "$(ls -A "$BASELINE_DIR" 2>/dev/null)" ]; then
        echo "Error: Baselines do not exist. Run with --generate first."
        exit 1
    fi
    
    mkdir -p "$CURRENT_DIR"
    mkdir -p "$OUTPUT_DIR"
    rm -f "$CURRENT_DIR"/*.png "$OUTPUT_DIR"/*.png
    
    echo "Capturing current components..."
    pushd "$RESOURCE_DIR" > /dev/null
    POLYSYNTH_VRT_MODE=1 POLYSYNTH_VRT_OUTPUT_DIR="$CURRENT_DIR" "$BINARY" || exit 1
    popd > /dev/null
    
    echo "Running diff..."
    python3 scripts/vrt_diff.py \
      --baseline-dir "$BASELINE_DIR" \
      --current-dir "$CURRENT_DIR" \
      --output-dir "$OUTPUT_DIR"

    echo "=== VRT Verification Passed ==="
    ;;

  *)
    echo "Usage: run_vrt.sh [--generate|--verify]"
    exit 1
    ;;
esac
