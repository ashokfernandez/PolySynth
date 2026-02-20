#!/usr/bin/env bash
# Build unsigned macOS installer package from staged release bundles.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSION_FILE="${ROOT_DIR}/VERSION"
STAGED_DIR="${ROOT_DIR}/.artifacts/releases/macos"
VST3_SRC="${STAGED_DIR}/PolySynth.vst3"
AU_SRC="${STAGED_DIR}/PolySynth.component"

if ! command -v pkgbuild >/dev/null 2>&1; then
  echo "pkgbuild not found. This task must run on macOS." >&2
  exit 2
fi

if [[ ! -f "${VERSION_FILE}" ]]; then
  echo "Missing ${VERSION_FILE}. Run: just set-version <version>" >&2
  exit 2
fi

VERSION="$(tr -d '[:space:]' < "${VERSION_FILE}")"
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Invalid VERSION value '${VERSION}'. Expected semantic version x.y.z" >&2
  exit 2
fi

if [[ ! -d "${VST3_SRC}" || ! -d "${AU_SRC}" ]]; then
  echo "Missing staged release bundles in ${STAGED_DIR}" >&2
  echo "Run: just plugin-release-mac" >&2
  exit 2
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/polysynth-pkg.XXXXXX")"
trap 'rm -rf "${WORK_DIR}"' EXIT

PKG_ROOT="${WORK_DIR}/pkgroot"
SCRIPTS_DIR="${WORK_DIR}/scripts"
OUT_PKG="${STAGED_DIR}/PolySynth-${VERSION}-macOS.pkg"

mkdir -p "${PKG_ROOT}/Library/Audio/Plug-Ins/VST3"
mkdir -p "${PKG_ROOT}/Library/Audio/Plug-Ins/Components"
mkdir -p "${SCRIPTS_DIR}"

cp -R "${VST3_SRC}" "${PKG_ROOT}/Library/Audio/Plug-Ins/VST3/"
cp -R "${AU_SRC}" "${PKG_ROOT}/Library/Audio/Plug-Ins/Components/"

cat > "${SCRIPTS_DIR}/postinstall" <<'EOS'
#!/usr/bin/env bash
set -euo pipefail

xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/PolySynth.vst3 2>/dev/null || true
xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/Components/PolySynth.component 2>/dev/null || true
exit 0
EOS

chmod +x "${SCRIPTS_DIR}/postinstall"
rm -f "${OUT_PKG}"

pkgbuild \
  --identifier "com.polysynth.installer" \
  --version "${VERSION}" \
  --root "${PKG_ROOT}" \
  --scripts "${SCRIPTS_DIR}" \
  --install-location "/" \
  "${OUT_PKG}"

echo "Created macOS installer package:"
echo "  ${OUT_PKG}"
