#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_NAME="ComponentGallery"
IPLUG2_ROOT="${PROJECT_ROOT}/../external/iPlug2"
BUILD_DIR="${PROJECT_ROOT}/build-web"
SCRIPTS_DIR="${BUILD_DIR}/scripts"

if [ ! -d "${IPLUG2_ROOT}" ]; then
  echo "iPlug2 not found at ${IPLUG2_ROOT}. Run ./scripts/download_iPlug2.sh first."
  exit 1
fi

if ! command -v emmake >/dev/null 2>&1; then
  echo "emmake not found. Install and activate Emscripten first."
  exit 1
fi

rm -rf "${BUILD_DIR}"
mkdir -p "${SCRIPTS_DIR}" "${BUILD_DIR}/styles"

echo "Building ${PROJECT_NAME} WAM processor module..."
cd "${PROJECT_ROOT}/projects"
emmake make --makefile "${PROJECT_NAME}-wam-processor.mk"

echo "Preparing WAM runtime scripts..."
cd "${SCRIPTS_DIR}"
{
  echo "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {};"
  echo "AudioWorkletGlobalScope.WAM.${PROJECT_NAME} = { ENVIRONMENT: 'WEB' };"
  echo "const ModuleFactory = AudioWorkletGlobalScope.WAM.${PROJECT_NAME};"
  cat "${PROJECT_NAME}-wam.js"
} > "${PROJECT_NAME}-wam.tmp.js"
mv "${PROJECT_NAME}-wam.tmp.js" "${PROJECT_NAME}-wam.js"

cp "${IPLUG2_ROOT}/Dependencies/IPlug/WAM_SDK/wamsdk/"*.js "${SCRIPTS_DIR}/"
cp "${IPLUG2_ROOT}/Dependencies/IPlug/WAM_AWP/"*.js "${SCRIPTS_DIR}/"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/scripts/IPlugWAM-awn.js" "${SCRIPTS_DIR}/${PROJECT_NAME}-awn.js"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/scripts/IPlugWAM-awp.js" "${SCRIPTS_DIR}/${PROJECT_NAME}-awp.js"

sed -i.bak "s/NAME_PLACEHOLDER/${PROJECT_NAME}/g" "${SCRIPTS_DIR}/${PROJECT_NAME}-awn.js"
sed -i.bak "s/NAME_PLACEHOLDER/${PROJECT_NAME}/g" "${SCRIPTS_DIR}/${PROJECT_NAME}-awp.js"
# Use a relative script root so the build works both at /index.html and when
# mounted under a subpath (e.g. Storybook staticDirs at /gallery).
sed -i.bak "s,ORIGIN_PLACEHOLDER,./,g" "${SCRIPTS_DIR}/${PROJECT_NAME}-awn.js"

echo "Generating index.html and static assets..."
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/index.html" "${BUILD_DIR}/index.html"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/styles/style.css" "${BUILD_DIR}/styles/style.css"
cp "${IPLUG2_ROOT}/IPlug/WEB/Template/favicon.ico" "${BUILD_DIR}/favicon.ico"

sed -i.bak "s/NAME_PLACEHOLDER/${PROJECT_NAME}/g" "${BUILD_DIR}/index.html"
sed -i.bak "s#<script async src=\"scripts/${PROJECT_NAME}-web.js\"></script>#<script defer src=\"scripts/${PROJECT_NAME}-web.js\"></script>#g" "${BUILD_DIR}/index.html"

# No resource packs yet; disable optional loaders for deterministic output.
sed -i.bak 's#<script async src="fonts.js"></script>#<!--<script async src="fonts.js"></script>-->#g' "${BUILD_DIR}/index.html"
sed -i.bak 's#<script async src="svgs.js"></script>#<!--<script async src="svgs.js"></script>-->#g' "${BUILD_DIR}/index.html"
sed -i.bak 's#<script async src="imgs.js"></script>#<!--<script async src="imgs.js"></script>-->#g' "${BUILD_DIR}/index.html"
sed -i.bak 's#<script async src="imgs@2x.js"></script>#<!--<script async src="imgs@2x.js"></script>-->#g' "${BUILD_DIR}/index.html"

# Keep WAM mode enabled.
sed -i.bak 's#<script src="scripts/websocket.js"></script>#<!--<script src="scripts/websocket.js"></script>-->#g' "${BUILD_DIR}/index.html"

MAXNINPUTS=$(python3 "${IPLUG2_ROOT}/Scripts/parse_iostr.py" "${PROJECT_ROOT}" inputs)
MAXNOUTPUTS=$(python3 "${IPLUG2_ROOT}/Scripts/parse_iostr.py" "${PROJECT_ROOT}" outputs)
sed -i.bak "s/MAXNINPUTS_PLACEHOLDER/${MAXNINPUTS}/g" "${BUILD_DIR}/index.html"
sed -i.bak "s/MAXNOUTPUTS_PLACEHOLDER/${MAXNOUTPUTS}/g" "${BUILD_DIR}/index.html"

# The runtime may rename the canvas id, so center/visibility logic should query
# by class instead of hardcoding "#canvas".
python3 - "${BUILD_DIR}/index.html" <<'PY'
import re
import sys

index_path = sys.argv[1]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

pattern = r"function connectionsDone\(\)\s*\{[\s\S]*?\n\s*\}"
replacement = """function connectionsDone() {
        const canvas = document.querySelector("#wam canvas.pluginArea");
        if (!canvas) {
          console.log("Plugin canvas not ready");
          return;
        }
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

new_html, count = re.subn(pattern, replacement, html, count=1)
if count == 1:
  with open(index_path, "w", encoding="utf-8") as f:
    f.write(new_html)
PY

# Avoid blocking alert() popups for stderr and surface errors in-page/console.
python3 - "${BUILD_DIR}/index.html" <<'PY'
import re
import sys

index_path = sys.argv[1]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

pattern = r"printErr:\s*function\(text\)\s*\{\s*alert\('stderr: '\s*\+\s*text\)\s*\},"
replacement = """printErr: function(text) {
          console.error('stderr: ' + text);
          if (statusElement) {
            statusElement.innerHTML = 'Runtime error: ' + text;
          }
        },"""

new_html, count = re.subn(pattern, replacement, html, count=1)
if count == 1:
  with open(index_path, "w", encoding="utf-8") as f:
    f.write(new_html)
PY

# Prevent duplicate startup (manual clicks, reload race, or wrapper retries)
# from injecting the same scripts multiple times.
python3 - "${BUILD_DIR}/index.html" <<'PY'
import re
import sys

index_path = sys.argv[1]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

pattern = r"function startWebAudio\(\)\s*\{\s*var actx = new AudioContext\(\);"
replacement = """function startWebAudio() {
        if (window.__componentGalleryAudioStarting || window.__componentGalleryAudioStarted) {
          return;
        }
        window.__componentGalleryAudioStarting = true;
        var actx = new AudioContext();"""

new_html, count = re.subn(pattern, replacement, html, count=1)
if count == 1:
  with open(index_path, "w", encoding="utf-8") as f:
    f.write(new_html)
PY

# Auto-start the WAM graph when the runtime is ready so the gallery is
# interactive without a separate "Start web audio" click.
python3 - "${BUILD_DIR}/index.html" <<'PY'
import re
import sys

index_path = sys.argv[1]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

pattern = r"postRun:\s*function\(\)\s*\{\s*document\.getElementById\('startWebAudioButton'\)\.removeAttribute\(\"disabled\"\);\s*\},"
replacement = """postRun: function() {
          const startButton = document.getElementById('startWebAudioButton');
          startButton.removeAttribute("disabled");

          if (!window.__componentGalleryAutoStarted) {
            window.__componentGalleryAutoStarted = true;
            setTimeout(() => {
              try {
                startWebAudio();
              } catch (err) {
                console.log("Auto-start failed: " + err);
                const wam = document.getElementById('wam');
                if (wam) {
                  wam.hidden = false;
                }
              }
            }, 0);
          }
        },"""

new_html, count = re.subn(pattern, replacement, html, count=1)
if count == 1:
  with open(index_path, "w", encoding="utf-8") as f:
    f.write(new_html)
PY

# For no-input plugins (e.g. instruments), bypass getUserMedia() so the UI can
# initialize reliably and the canvas becomes interactive after pressing Start.
if [ "${MAXNINPUTS}" -eq 0 ]; then
  python3 - "${BUILD_DIR}/index.html" "${PROJECT_NAME}" <<'PY'
import re
import sys

index_path = sys.argv[1]
project_name = sys.argv[2]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

pattern = r"if\(NInputs > 0\)\s*\{[\s\S]*?\}\s*else\s*\{[\s\S]*?\}"
replacement = (
  f"{project_name}_WAM.connect(actx.destination); // connect WAM output to speakers\n"
  f"                    connectionsDone();"
)

new_html, count = re.subn(pattern, replacement, html, count=1)
if count == 1:
  with open(index_path, "w", encoding="utf-8") as f:
    f.write(new_html)
PY

  # Avoid an accidental one-input bus declaration like [0] for no-input plugins.
  python3 - "${BUILD_DIR}/index.html" <<'PY'
import re
import sys

index_path = sys.argv[1]

with open(index_path, "r", encoding="utf-8") as f:
  html = f.read()

new_html = re.sub(r"let inputBuses = \[\s*0\s*\];", "let inputBuses = [];", html, count=1)

with open(index_path, "w", encoding="utf-8") as f:
  f.write(new_html)
PY
fi

# Keep a stable wrapper id for visual tests.
sed -i.bak 's#<canvas class="pluginArea" id="canvas" oncontextmenu="event.preventDefault()"></canvas>#<div id="IPlugCanvas"><canvas class="pluginArea" id="canvas" oncontextmenu="event.preventDefault()"></canvas></div>#g' "${BUILD_DIR}/index.html"

cat <<'STYLE_EOF' >> "${BUILD_DIR}/styles/style.css"

body {
  background: rgb(70, 70, 70);
}
STYLE_EOF

echo "Building ${PROJECT_NAME} web UI module..."
cd "${PROJECT_ROOT}/projects"
emmake make --makefile "${PROJECT_NAME}-wam-controller.mk" EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=0

rm -f "${SCRIPTS_DIR}"/*.bak "${BUILD_DIR}"/*.bak "${BUILD_DIR}/styles"/*.bak

echo "Build complete: ${BUILD_DIR}/index.html"
