#!/usr/bin/env bash
set -euo pipefail

# Downloads Raspberry Pi FreeRTOS-Kernel fork into external/FreeRTOS-Kernel/.
# Idempotent — skips if already present and valid.
#
# Uses the RPi fork (not mainline) because mainline's ARM_CM33_NTZ/non_secure
# port crashes in crt0.S on RP2350. The RPi fork includes RP2350_ARM_NTZ
# portable layer that handles RP2350's secure-mode-only execution.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
FREERTOS_DIR="${REPO_ROOT}/external/FreeRTOS-Kernel"
# RPi fork has no tags — pin to a known-good commit on main
FREERTOS_COMMIT="4f7299d6ea746b27a9dd19e87af568e34bd65b15"

if [ -d "${FREERTOS_DIR}/portable/ThirdParty/GCC/RP2350_ARM_NTZ" ]; then
    echo "FreeRTOS-Kernel already present at ${FREERTOS_DIR}"
    exit 0
fi

echo "Downloading FreeRTOS-Kernel (RPi fork, ${FREERTOS_COMMIT:0:8}) to ${FREERTOS_DIR}..."
mkdir -p "$(dirname "${FREERTOS_DIR}")"

# Remove partial downloads
if [ -d "${FREERTOS_DIR}" ]; then
    echo "Removing incomplete FreeRTOS-Kernel at ${FREERTOS_DIR}..."
    rm -rf "${FREERTOS_DIR}"
fi

git clone https://github.com/raspberrypi/FreeRTOS-Kernel.git "${FREERTOS_DIR}"
cd "${FREERTOS_DIR}"
git checkout "${FREERTOS_COMMIT}"

echo "FreeRTOS-Kernel (${FREERTOS_COMMIT:0:8}) ready at ${FREERTOS_DIR}"
