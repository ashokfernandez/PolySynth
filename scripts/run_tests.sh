#!/bin/bash
set -e

# Directory setup
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
TEST_DIR="$PROJECT_ROOT/tests"
BUILD_DIR="$TEST_DIR/build"

echo "=== PolySynth Test Runner ==="
echo "Project Root: $PROJECT_ROOT"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "--> Configuring CMake..."
cmake .. -DIPLUG_UNIT_TESTS=ON

# Build
echo "--> Building..."
make -j4

# Run Tests
echo "--> Running Tests..."
./run_tests

echo "=== All Tests Passed ==="
