#!/bin/bash
# PolySynth Local macOS Installation Script
# This script copies the built AU and VST3 plugins to the local macOS plug-in folders.

set -e

# Ensure we are in the project root
cd "$(dirname "$0")"

CONFIG=${1:-Debug}
BUILD_DIR="build_desktop/out/$CONFIG"

echo "🎯 Installing PolySynth ($CONFIG) to local macOS plug-in folders..."

# 1. Check if the builds exist, if not, try to build them
if [ ! -d "$BUILD_DIR/PolySynth.component" ] || [ ! -d "$BUILD_DIR/PolySynth.vst3" ]; then
    echo "📦 Plugins not found in $BUILD_DIR. Building them now..."
    
    # Ensure build_desktop is configured
    if [ ! -d "build_desktop" ]; then
        echo "⚙️ Configuring build_desktop..."
        mkdir -p build_desktop
        cmake -S src/platform/desktop -B build_desktop -G Xcode
    fi
    
    echo "🔨 Building AU..."
    cmake --build build_desktop --config "$CONFIG" --target PolySynth-au
    
    echo "🔨 Building VST3..."
    cmake --build build_desktop --config "$CONFIG" --target PolySynth-vst3
fi

AU_SRC="$BUILD_DIR/PolySynth.component"
VST3_SRC="$BUILD_DIR/PolySynth.vst3"

AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"

# 2. Create destination directories if they don't exist
mkdir -p "$AU_DEST"
mkdir -p "$VST3_DEST"

# 3. Copy AU
if [ -d "$AU_SRC" ]; then
    echo "🚚 Installing AU to $AU_DEST..."
    rm -rf "$AU_DEST/PolySynth.component"
    cp -R "$AU_SRC" "$AU_DEST/"
    echo "✅ AU Installed"
else
    echo "❌ AU build not found at $AU_SRC"
fi

# 4. Copy VST3
if [ -d "$VST3_SRC" ]; then
    echo "🚚 Installing VST3 to $VST3_DEST..."
    rm -rf "$VST3_DEST/PolySynth.vst3"
    cp -R "$VST3_SRC" "$VST3_DEST/"
    echo "✅ VST3 Installed"
else
    echo "❌ VST3 build not found at $VST3_SRC"
fi

echo ""
echo "🎉 Installation complete!"
echo "📍 AU:   $AU_DEST/PolySynth.component"
echo "📍 VST3: $VST3_DEST/PolySynth.vst3"
echo ""
echo "ℹ️  Note: You may need to restart Ableton or rescan plugins to see the changes."
