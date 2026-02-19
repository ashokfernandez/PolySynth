#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

# 1. Build the web distribution using the patched script
echo "Building PolySynth Web Distribution..."
cd src/platform/desktop/scripts
./makedist-web.sh off ./

# 2. Serve the built artifacts
echo "Starting local server..."
cd ../build-web
echo "Serving on http://localhost:8000"

# Open the browser if possible
if command -v open >/dev/null 2>&1; then
    open http://localhost:8000
elif command -v xdg-open >/dev/null 2>&1; then
    xdg-open http://localhost:8000
fi

python3 -m http.server 8000
