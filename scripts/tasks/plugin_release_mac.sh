#!/usr/bin/env bash
# Build unsigned release AU + VST3 bundles for macOS distribution.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DESKTOP_DIR="${ROOT_DIR}/src/platform/desktop"
XCODE_PROJECT="${DESKTOP_DIR}/projects/PolySynth-macOS.xcodeproj"
BUILD_ROOT="${DESKTOP_DIR}/build-mac"
OUT_DIR="${ROOT_DIR}/.artifacts/releases/macos"

if [[ ! -d "${ROOT_DIR}/external/iPlug2" ]]; then
  echo "Missing external/iPlug2 dependency. Run: just deps" >&2
  exit 2
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
  echo "xcodebuild not found. This task must run on macOS with Xcode installed." >&2
  exit 2
fi

echo "Building macOS VST3 (Release, unsigned)..."
xcodebuild \
  -project "${XCODE_PROJECT}" \
  -scheme "macOS-VST3" \
  -configuration Release \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGN_IDENTITY="" \
  DEVELOPMENT_TEAM="" \
  build

echo "Building macOS AUv2 (Release, unsigned)..."
xcodebuild \
  -project "${XCODE_PROJECT}" \
  -scheme "macOS-AUv2" \
  -configuration Release \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGN_IDENTITY="" \
  DEVELOPMENT_TEAM="" \
  build

find_release_bundle() {
  local name="$1"
  local path
  path="$(find "${BUILD_ROOT}" -type d -name "${name}" 2>/dev/null | grep '/Release/' | head -n 1 || true)"
  if [[ -n "${path}" ]]; then
    printf '%s\n' "${path}"
    return 0
  fi

  path="$(find "${BUILD_ROOT}" -type d -name "${name}" 2>/dev/null | head -n 1 || true)"
  if [[ -z "${path}" ]]; then
    return 1
  fi
  printf '%s\n' "${path}"
}

VST3_SRC="$(find_release_bundle "PolySynth.vst3")"
AU_SRC="$(find_release_bundle "PolySynth.component")"

if [[ -z "${VST3_SRC}" || ! -d "${VST3_SRC}" ]]; then
  echo "Failed to locate built VST3 bundle under ${BUILD_ROOT}" >&2
  exit 1
fi

if [[ -z "${AU_SRC}" || ! -d "${AU_SRC}" ]]; then
  echo "Failed to locate built AU bundle under ${BUILD_ROOT}" >&2
  exit 1
fi

if command -v xattr >/dev/null 2>&1; then
  xattr -cr "${VST3_SRC}" "${AU_SRC}"
fi

mkdir -p "${OUT_DIR}"
rm -rf "${OUT_DIR}/PolySynth.vst3" "${OUT_DIR}/PolySynth.component"
cp -R "${VST3_SRC}" "${OUT_DIR}/"
cp -R "${AU_SRC}" "${OUT_DIR}/"

echo "Staged macOS release bundles:"
echo "  ${OUT_DIR}/PolySynth.vst3"
echo "  ${OUT_DIR}/PolySynth.component"
