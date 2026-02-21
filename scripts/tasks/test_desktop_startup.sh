set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

CONFIG="${1:-Debug}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "Skipping desktop startup smoke test on non-macOS host."
  exit 0
fi

APP_BIN="$(find "build_desktop/out/${CONFIG}" -path "*/PolySynth.app/Contents/MacOS/PolySynth" -type f | head -n 1)"
if [[ -z "${APP_BIN}" || ! -f "${APP_BIN}" ]]; then
  echo "Error: PolySynth app binary not found for config '${CONFIG}'."
  echo "Run 'just desktop-build ${CONFIG}' first."
  exit 1
fi
APP_BIN="${ROOT_DIR}/${APP_BIN}"

echo "Running desktop startup smoke test with ${APP_BIN}"

python3 - <<'PY' "${APP_BIN}" "${ROOT_DIR}"
import subprocess
import sys
import time

app_bin = sys.argv[1]
repo_root = sys.argv[2]

proc = subprocess.Popen(
    [app_bin],
    cwd=repo_root,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
)

alive_window_s = 4.0
deadline = time.time() + alive_window_s
while time.time() < deadline:
    if proc.poll() is not None:
        output = proc.stdout.read() if proc.stdout else ""
        if output:
            print(output, end="")
        print(
            f"FAIL: desktop app exited early with code {proc.returncode} "
            f"(before {alive_window_s:.1f}s)."
        )
        sys.exit(1)
    time.sleep(0.1)

print(f"PASS: desktop app stayed alive for {alive_window_s:.1f}s")

proc.terminate()
try:
    proc.wait(timeout=5)
except subprocess.TimeoutExpired:
    proc.kill()
    proc.wait(timeout=5)
PY
