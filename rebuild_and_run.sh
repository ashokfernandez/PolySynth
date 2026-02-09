#!/bin/bash
# PolySynth Rebuild and Run Script
# This script does a clean build and then launches the application.

set -e

# Ensure we are in the project root
cd "$(dirname "$0")"

./build_desktop.sh
./run_desktop.sh
