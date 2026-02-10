#!/bin/bash
# ComponentGallery Rebuild and View Script
# Clean builds the gallery and then serves it.

set -e

# Ensure we are in the project root
cd "$(dirname "$0")"

./build_gallery.sh
./view_gallery.sh "$@"
