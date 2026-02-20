#!/usr/bin/env bash
# Build all three component galleries with proper isolation
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "Building isolated component galleries in parallel..."

# Start from a clean gallery artifact root to avoid stale components lingering.
rm -rf "${REPO_ROOT}/ComponentGallery/build-web"
mkdir -p "${REPO_ROOT}/ComponentGallery/build-web"

# 1. Build the WAM processor once (it's shared by all variants)
echo "Building base WAM processor..."
# We use 'knob' as a dummy name just to get the processor built once into build-web/scripts
GALLERY_COMPONENT=KNOB "${REPO_ROOT}/scripts/build_single_gallery.sh" knob

# 2. Build all variants in parallel (using the pre-built processor)
echo "Launching parallel UI builds for all variants..."

# List of all component variants to build
COMPONENTS=(
  "knob" "fader" "envelope" "button"
  "switch" "toggle" "slideswitch" "tabswitch" "radiobutton"
  "polyknob" "polysection" "polytoggle"
  "sectionframe" "lcdpanel" "presetsavebutton"
)

for comp in "${COMPONENTS[@]}"; do
  # Skip 'knob' processor build if we already did it, but actually build_single_gallery
  # handles copying it. Since we already built it for 'knob', we can skip for all including knob 
  # itself if we want to be perfectly clean, but let's just use --skip-processor for all others.
  
  # For the first one (knob), we already built it. For others, skip.
  if [ "$comp" == "knob" ]; then
    # Already done above, but we need to ensure the JS controller is built for knob too
    # Actually, the above call already built BOTH for knob. So we just skip knob in the loop.
    continue
  fi
  
  echo "-> Starting ${comp} build..."
  GALLERY_COMPONENT=$(echo "${comp}" | tr '[:lower:]' '[:upper:]') \
    "${REPO_ROOT}/scripts/build_single_gallery.sh" "${comp}" --skip-processor &
done

# Wait for all background jobs to complete
echo "Waiting for all builds to finish..."
wait

echo ""
echo "All component galleries built successfully!"
echo "Output locations:"
for comp in "${COMPONENTS[@]}"; do
  echo "  - ComponentGallery/build-web/${comp}/"
done
