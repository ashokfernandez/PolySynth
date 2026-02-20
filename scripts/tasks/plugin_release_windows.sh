#!/usr/bin/env bash
# Build release Windows VST3 bundle and stage zipped artifact.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DESKTOP_DIR="${ROOT_DIR}/src/platform/desktop"
SOLUTION_PATH="${DESKTOP_DIR}/PolySynth.sln"
BUILD_DIR="${DESKTOP_DIR}/build-win"
BUNDLE_PATH="${BUILD_DIR}/PolySynth.vst3"
OUT_DIR="${ROOT_DIR}/.artifacts/releases/windows"
ZIP_PATH="${OUT_DIR}/PolySynth-windows-vst3.zip"

if [[ ! -d "${ROOT_DIR}/external/iPlug2" ]]; then
  echo "Missing external/iPlug2 dependency. Run: just deps" >&2
  exit 2
fi

if ! command -v msbuild >/dev/null 2>&1; then
  echo "msbuild not found. This task must run on a Windows build host with MSBuild." >&2
  exit 2
fi

echo "Building Windows VST3 (Release|x64)..."
msbuild "${SOLUTION_PATH}" \
  -m \
  -t:PolySynth-vst3 \
  -p:Configuration=Release \
  -p:Platform=x64

if [[ ! -d "${BUNDLE_PATH}" ]]; then
  echo "Failed to locate built bundle at ${BUNDLE_PATH}" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"
rm -rf "${OUT_DIR}/PolySynth.vst3" "${ZIP_PATH}"
cp -R "${BUNDLE_PATH}" "${OUT_DIR}/"

python3 - <<'PY' "${OUT_DIR}/PolySynth.vst3" "${ZIP_PATH}"
import os
import sys
import zipfile

bundle = sys.argv[1]
zip_path = sys.argv[2]
root_name = os.path.basename(bundle.rstrip("/\\"))

with zipfile.ZipFile(zip_path, mode="w", compression=zipfile.ZIP_DEFLATED) as archive:
    for base, _, files in os.walk(bundle):
        for file_name in files:
            full_path = os.path.join(base, file_name)
            rel_path = os.path.relpath(full_path, bundle)
            archive.write(full_path, os.path.join(root_name, rel_path))
PY

echo "Staged Windows release artifacts:"
echo "  ${OUT_DIR}/PolySynth.vst3"
echo "  ${ZIP_PATH}"
