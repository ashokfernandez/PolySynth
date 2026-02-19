#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/lib/cli.sh
source "${SCRIPT_DIR}/lib/cli.sh"

usage() {
  cat <<'EOF'
PolySynth CLI helper

Usage:
  scripts/cli.sh run <step-name> -- <command...>
  scripts/cli.sh latest
  scripts/cli.sh help

Environment:
  POLYSYNTH_VERBOSE=1   Stream full command output live (still logs to file)
  POLYSYNTH_LOG_ROOT    Override log root (default: .artifacts/logs)
EOF
}

cmd="${1:-help}"
shift || true

case "${cmd}" in
  run)
    if [[ $# -lt 1 ]]; then
      echo "Missing <step-name>." >&2
      usage
      exit 2
    fi
    step="$1"
    shift
    if [[ "${1:-}" == "--" ]]; then
      shift
    fi
    if [[ $# -lt 1 ]]; then
      echo "Missing command to execute." >&2
      usage
      exit 2
    fi
    ps_run_step "${step}" "$@"
    ;;
  latest)
    if latest_dir="$(ps_latest_run_dir)"; then
      echo "Latest run: ${latest_dir}"
      if [[ -f "${latest_dir}/summary.txt" ]]; then
        echo
        cat "${latest_dir}/summary.txt"
      fi
    else
      echo "No run logs found yet."
    fi
    ;;
  help|-h|--help)
    usage
    ;;
  *)
    echo "Unknown command: ${cmd}" >&2
    usage
    exit 2
    ;;
esac
