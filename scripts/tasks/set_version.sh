#!/usr/bin/env bash
# Sync semantic version across release metadata files.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <major.minor.patch>" >&2
  exit 2
fi

VERSION="$1"
if [[ ! "$VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
  echo "Invalid version '$VERSION'. Expected semantic version: <major.minor.patch>" >&2
  exit 2
fi

MAJOR="${BASH_REMATCH[1]}"
MINOR="${BASH_REMATCH[2]}"
PATCH="${BASH_REMATCH[3]}"
HEX_VERSION="$(printf '0x%08X' "$(( (MAJOR << 16) | (MINOR << 8) | PATCH ))")"
RC_VERSION="${MAJOR},${MINOR},${PATCH},0"

VERSION_FILE="${ROOT_DIR}/VERSION"
CONFIG_H="${ROOT_DIR}/src/platform/desktop/config.h"
CMAKE_FILE="${ROOT_DIR}/src/platform/desktop/CMakeLists.txt"
MAIN_RC="${ROOT_DIR}/src/platform/desktop/resources/main.rc"

printf "%s\n" "${VERSION}" > "${VERSION_FILE}"

VERSION="${VERSION}" \
HEX_VERSION="${HEX_VERSION}" \
RC_VERSION="${RC_VERSION}" \
CONFIG_H="${CONFIG_H}" \
CMAKE_FILE="${CMAKE_FILE}" \
MAIN_RC="${MAIN_RC}" \
python3 - <<'PY'
import os
import re
from pathlib import Path

version = os.environ["VERSION"]
hex_version = os.environ["HEX_VERSION"]
rc_version = os.environ["RC_VERSION"]
config_h = Path(os.environ["CONFIG_H"])
cmake_file = Path(os.environ["CMAKE_FILE"])
main_rc = Path(os.environ["MAIN_RC"])


def replace_or_fail(path: Path, pattern: str, replacement: str) -> None:
    text = path.read_text(encoding="utf-8")
    new_text, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise SystemExit(f"Failed to update pattern in {path}: {pattern}")
    path.write_text(new_text, encoding="utf-8")


replace_or_fail(
    config_h,
    r'^#define PLUG_VERSION_HEX\s+0x[0-9A-Fa-f]+$',
    f"#define PLUG_VERSION_HEX {hex_version}",
)
replace_or_fail(
    config_h,
    r'^#define PLUG_VERSION_STR\s+"[^"]+"$',
    f'#define PLUG_VERSION_STR "{version}"',
)

replace_or_fail(
    cmake_file,
    r'^(project\(PolySynth VERSION )([0-9]+\.[0-9]+\.[0-9]+)(\))$',
    rf"\g<1>{version}\g<3>",
)

replace_or_fail(
    main_rc,
    r'^(\s*FILEVERSION\s+)\d+,\d+,\d+,\d+$',
    rf"\g<1>{rc_version}",
)
replace_or_fail(
    main_rc,
    r'^(\s*PRODUCTVERSION\s+)\d+,\d+,\d+,\d+$',
    rf"\g<1>{rc_version}",
)
replace_or_fail(
    main_rc,
    r'^(\s*VALUE "FileVersion",\s*")[^"]+(")$',
    rf'\g<1>{version}\g<2>',
)
replace_or_fail(
    main_rc,
    r'^(\s*VALUE "ProductVersion",\s*")[^"]+(")$',
    rf'\g<1>{version}\g<2>',
)
PY

echo "Updated version to ${VERSION}"
echo "  VERSION: ${VERSION_FILE}"
echo "  Config:  ${CONFIG_H}"
echo "  CMake:   ${CMAKE_FILE}"
echo "  RC:      ${MAIN_RC}"
