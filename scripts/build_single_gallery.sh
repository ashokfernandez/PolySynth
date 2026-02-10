#!/usr/bin/env bash
# Build a single component gallery variant
set -euo pipefail

COMPONENT_NAME="$1"  # knob, fader, or envelope
COMPONENT_UPPER=$(echo "${COMPONENT_NAME}" | tr '[:lower:]' '[:upper:]')

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_ROOT="${REPO_ROOT}/ComponentGallery"
BUILD_ROOT="${PROJECT_ROOT}/build-web"
OUTPUT_DIR="${BUILD_ROOT}/${COMPONENT_NAME}"

# Define the preprocessor flag
COMPONENT_FLAG="-DGALLERY_COMPONENT_${COMPONENT_UPPER}=1"

# Ensure dependencies
if [ ! -d "${REPO_ROOT}/external/iPlug2" ]; then
  echo "iPlug2 not found. Run ./scripts/download_iPlug2.sh first."
  exit 1
fi

if ! command -v emmake >/dev/null 2>&1; then
  if [ -f "$HOME/.emsdk/emsdk_env.sh" ]; then
    export EMSDK_QUIET=1
    source "$HOME/.emsdk/emsdk_env.sh" >/dev/null
  else
    echo "Error: emmake not found. Install Emscripten."
    exit 1
  fi
fi

# Clean and create output directory
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/scripts"
mkdir -p "${OUTPUT_DIR}/styles"

# Also create temp build directory for make
mkdir -p "${BUILD_ROOT}/scripts"

# Build WAM processor (DSP - no component flag needed)
echo "Building ${COMPONENT_NAME} WAM processor..."
cd "${PROJECT_ROOT}/projects"
emmake make --makefile ComponentGallery-wam-processor.mk
mv ../build-web/scripts/ComponentGallery-wam.js "${OUTPUT_DIR}/scripts/"

# Build WAM controller (UI - with component flag)
echo "Building ${COMPONENT_NAME} WAM controller..."
emmake make --makefile ComponentGallery-wam-controller.mk EXTRA_CFLAGS="${COMPONENT_FLAG} -DWEBSOCKET_CLIENT=0"
mv ../build-web/scripts/ComponentGallery-web.js "${OUTPUT_DIR}/scripts/"

# Copy WAM runtime scripts
echo "Setting up WAM runtime..."
IPLUG2_ROOT="${REPO_ROOT}/external/iPlug2"
SCRIPTS_DIR="${OUTPUT_DIR}/scripts"

cd "${SCRIPTS_DIR}"
{
  echo "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {};"
  echo "AudioWorkletGlobalScope.WAM.ComponentGallery = { ENVIRONMENT: 'WEB' };"
  echo "const ModuleFactory = AudioWorkletGlobalScope.WAM.ComponentGallery;"
  cat "ComponentGallery-wam.js"
} > "ComponentGallery-wam.tmp.js"
mv "ComponentGallery-wam.tmp.js" "ComponentGallery-wam.js"

cp "${IPLUG2_ROOT}/Dependencies/IPlug/WAM_SDK/wamsdk/"*.js "${SCRIPTS_DIR}/"
cp "${IPLUG2_ROOT}/Dependencies/IPlug/WAM_AWP/"*.js "${SCRIPTS_DIR}/"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/scripts/IPlugWAM-awn.js" "${SCRIPTS_DIR}/ComponentGallery-awn.js"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/scripts/IPlugWAM-awp.js" "${SCRIPTS_DIR}/ComponentGallery-awp.js"

sed -i.bak "s/NAME_PLACEHOLDER/ComponentGallery/g" "${SCRIPTS_DIR}/ComponentGallery-awn.js"
sed -i.bak "s/NAME_PLACEHOLDER/ComponentGallery/g" "${SCRIPTS_DIR}/ComponentGallery-awp.js"
sed -i.bak "s,ORIGIN_PLACEHOLDER,./,g" "${SCRIPTS_DIR}/ComponentGallery-awn.js"

# Generate index.html
echo "Generating index.html..."
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/index.html" "${OUTPUT_DIR}/index.html"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/styles/style.css" "${OUTPUT_DIR}/styles/style.css"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/favicon.ico" "${OUTPUT_DIR}/favicon.ico"

sed -i.bak "s/NAME_PLACEHOLDER/ComponentGallery/g" "${OUTPUT_DIR}/index.html"
sed -i.bak "s#<script async src=\"scripts/ComponentGallery-web.js\"></script>#<script defer src=\"scripts/ComponentGallery-web.js\"></script>#g" "${OUTPUT_DIR}/index.html"
sed -i.bak 's#<script async src="fonts.js"></script>#<!--<script async src="fonts.js"></script>-->#g' "${OUTPUT_DIR}/index.html"
sed -i.bak 's#<script async src="svgs.js"></script>#<!--<script async src="svgs.js"></script>-->#g' "${OUTPUT_DIR}/index.html"
sed -i.bak 's#<script async src="imgs.js"></script>#<!--<script async src="imgs.js"></script>-->#g' "${OUTPUT_DIR}/index.html"
sed -i.bak 's#<script async src="imgs@2x.js"></script>#<!--<script async src="imgs@2x.js"></script>-->#g' "${OUTPUT_DIR}/index.html"
sed -i.bak 's#<script src="scripts/websocket.js"></script>#<!--<script src="scripts/websocket.js"></script>-->#g' "${OUTPUT_DIR}/index.html"

MAXNINPUTS=$(python3 "${IPLUG2_ROOT}/Scripts/parse_iostr.py" "${PROJECT_ROOT}" inputs)
MAXNOUTPUTS=$(python3 "${IPLUG2_ROOT}/Scripts/parse_iostr.py" "${PROJECT_ROOT}" outputs)
sed -i.bak "s/MAXNINPUTS_PLACEHOLDER/${MAXNINPUTS}/g" "${OUTPUT_DIR}/index.html"
sed -i.bak "s/MAXNOUTPUTS_PLACEHOLDER/${MAXNOUTPUTS}/g" "${OUTPUT_DIR}/index.html"

# Auto-start and styling patches
python3 - "${OUTPUT_DIR}/index.html" <<'PY'
import re, sys
index_path = sys.argv[1]
with open(index_path, "r") as f:
    html = f.read()

# Hide buttons
pattern = r"function connectionsDone\(\)\s*\{[\s\S]*?\n\s*\}"
replacement = """function connectionsDone() {
        const canvas = document.querySelector("#wam canvas.pluginArea");
        if (!canvas) { console.log("Plugin canvas not ready"); return; }
        const canvasWidth = parseInt(canvas.style.width || canvas.getAttribute("width") || canvas.width || 0, 10);
        const canvasHeight = parseInt(canvas.style.height || canvas.getAttribute("height") || canvas.height || 0, 10);
        document.getElementById("wam").style.left = ((window.innerWidth / 2) - (canvasWidth / 2)) + "px";
        document.getElementById("wam").style.top = ((window.innerHeight / 2) - (canvasHeight / 2)) + "px";
        document.getElementById('wam').hidden = false;
        document.getElementById('startWebAudioButton').setAttribute("disabled", "true");
        window.__componentGalleryAudioStarting = false;
        window.__componentGalleryAudioStarted = true;
        initMidiComboBox(false, "#midiInSelect");
        initMidiComboBox(true, "#midiOutSelect");
      }"""
html = re.sub(pattern, replacement, html, count=1)

# Auto-start
pattern = r"postRun:\s*function\(\)\s*\{\s*document\.getElementById\('startWebAudioButton'\)\.removeAttribute\(\"disabled\"\);\s*\},"
replacement = """postRun: function() {
          const startButton = document.getElementById('startWebAudioButton');
          startButton.removeAttribute("disabled");
          if (!window.__componentGalleryAutoStarted) {
            window.__componentGalleryAutoStarted = true;
            setTimeout(() => { try { startWebAudio(); } catch (err) { console.log("Auto-start failed: " + err); window.__componentGalleryAudioStarting = false; window.__componentGalleryAudioStarted = false; window.__componentGalleryAutoStarted = false; const wam = document.getElementById('wam'); if (wam) { wam.hidden = false; } } }, 0);
          }
        },"""
html = re.sub(pattern, replacement, html, count=1)

with open(index_path, "w") as f:
    f.write(html)
PY

# Clean up
rm -f "${SCRIPTS_DIR}"/*.bak "${OUTPUT_DIR}"/*.bak "${OUTPUT_DIR}/styles"/*.bak

echo "Build complete: ${OUTPUT_DIR}/index.html"
