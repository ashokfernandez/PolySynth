#!/bin/bash
# PolySynth Desktop Launch Script
# This script launches the latest standalone desktop build.

# Ensure we are in the project root
cd "$(dirname "$0")"

# Search for the app in the build directory
APP_PATH=$(find build_desktop/out -name "PolySynth.app" -type d | head -n 1)

if [ -z "$APP_PATH" ] || [ ! -d "$APP_PATH" ]; then
    echo "❌ Error: PolySynth.app not found."
    echo "Please run './build_desktop.sh' first."
    exit 1
fi

echo "🚀 Launching PolySynth from $APP_PATH..."
open "$APP_PATH"
