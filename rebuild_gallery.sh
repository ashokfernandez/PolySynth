#!/bin/bash
# ComponentGallery Rebuild Script
# Alias for a clean gallery build.

set -e
cd "$(dirname "$0")"

./build_gallery.sh
