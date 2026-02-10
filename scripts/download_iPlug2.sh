#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
EXTERNAL_DIR="${REPO_ROOT}/external"
IPLUG2_DIR="${EXTERNAL_DIR}/iPlug2"

mkdir -p "${EXTERNAL_DIR}"

if [ ! -d "${IPLUG2_DIR}/.git" ]; then
  echo "Cloning iPlug2 into ${IPLUG2_DIR}..."
  git clone https://github.com/iPlug2/iPlug2.git "${IPLUG2_DIR}"
else
  echo "iPlug2 already present at ${IPLUG2_DIR}"
fi

echo "Ensuring iPlug2 SDK dependencies are present..."
cd "${IPLUG2_DIR}/Dependencies/IPlug"
./download-iplug-sdks.sh || true

ensure_repo_with_file() {
  local dir_name="$1"
  local repo_url="$2"
  local required_file="$3"

  if [ -f "${dir_name}/${required_file}" ]; then
    return
  fi

  echo "Repairing ${dir_name} (missing ${required_file})..."

  if [ -d "${dir_name}" ]; then
    mv "${dir_name}" "${dir_name}.bak.$(date +%s)"
  fi

  git clone "${repo_url}" "${dir_name}"
  rm -rf "${dir_name}/.git"
}

# Some iPlug2 dependency scripts require sudo and may leave placeholders.
# Ensure WAM dependencies are actually present.
ensure_repo_with_file "WAM_AWP" "https://github.com/iplug2/audioworklet-polyfill" "audioworklet.js"
ensure_repo_with_file "WAM_SDK" "https://github.com/iplug2/api.git" "wamsdk/processor.cpp"
