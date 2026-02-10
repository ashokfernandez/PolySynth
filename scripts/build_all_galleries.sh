#!/usr/bin/env bash
# Build all three component galleries with proper isolation
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "Building isolated component galleries..."

# Build knob gallery
echo "1/3 Building Knob gallery..."
GALLERY_COMPONENT=KNOB "${REPO_ROOT}/scripts/build_single_gallery.sh" knob

# Build fader gallery
echo "2/3 Building Fader gallery..."
GALLERY_COMPONENT=FADER "${REPO_ROOT}/scripts/build_single_gallery.sh" fader

# Build envelope gallery
echo "3/3 Building Envelope gallery..."
GALLERY_COMPONENT=ENVELOPE "${REPO_ROOT}/scripts/build_single_gallery.sh" envelope

echo ""
echo "All component galleries built successfully!"
echo "Output locations:"
echo "  - ComponentGallery/build-web/knob/"
echo "  - ComponentGallery/build-web/fader/"
echo "  - ComponentGallery/build-web/envelope/"
