#!/bin/bash
set -e

# Base directory
BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
EXTERNAL_DIR="$BASE_DIR/external"

echo "Downloading dependencies to $EXTERNAL_DIR..."
mkdir -p "$EXTERNAL_DIR"

# iPlug2
if [ ! -d "$EXTERNAL_DIR/iPlug2" ]; then
    echo "Cloning iPlug2..."
    git clone https://github.com/iPlug2/iPlug2.git "$EXTERNAL_DIR/iPlug2"
fi

echo "Updating iPlug2 dependencies..."
cd "$EXTERNAL_DIR/iPlug2/Dependencies/IPlug"
chmod +x download-vst3-sdk.sh
./download-vst3-sdk.sh
cd "$BASE_DIR"

# Catch2 (for consistency, though CMake handles it too, let's have it explicitly)
mkdir -p "$EXTERNAL_DIR/catch2"
if [ ! -f "$EXTERNAL_DIR/catch2/catch.hpp" ]; then
    echo "Downloading Catch2..."
    curl -L https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp -o "$EXTERNAL_DIR/catch2/catch.hpp"
fi

echo "Dependencies ready."
