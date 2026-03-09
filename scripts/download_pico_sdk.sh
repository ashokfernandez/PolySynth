#!/usr/bin/env bash
set -euo pipefail

# Downloads Raspberry Pi Pico SDK v2.1.0 into external/pico-sdk/.
# Idempotent — skips if the SDK is already present and valid.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PICO_SDK_DIR="${REPO_ROOT}/external/pico-sdk"
PICO_SDK_VERSION="2.1.0"

if [ -d "${PICO_SDK_DIR}/src/rp2350" ]; then
    echo "Pico SDK v${PICO_SDK_VERSION} already present at ${PICO_SDK_DIR}"
    exit 0
fi

echo "Downloading Pico SDK v${PICO_SDK_VERSION} to ${PICO_SDK_DIR}..."
mkdir -p "$(dirname "${PICO_SDK_DIR}")"

# Remove partial downloads
if [ -d "${PICO_SDK_DIR}" ]; then
    echo "Removing incomplete SDK at ${PICO_SDK_DIR}..."
    rm -rf "${PICO_SDK_DIR}"
fi

git clone --depth 1 --branch "${PICO_SDK_VERSION}" \
    https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_DIR}"

echo "Initializing SDK submodules (TinyUSB, cyw43-driver, etc.)..."
cd "${PICO_SDK_DIR}"
git submodule update --init --depth 1

echo "Pico SDK v${PICO_SDK_VERSION} ready at ${PICO_SDK_DIR}"
