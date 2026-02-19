#!/usr/bin/env bash
# Shared CLI logging helpers for PolySynth command runners.

set -euo pipefail

ps_repo_root() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  cd "${script_dir}/../.." && pwd
}

ps_slug() {
  local input="$1"
  local out
  out="$(echo "${input}" | tr '[:upper:]' '[:lower:]' | sed -E 's/[^a-z0-9._-]+/_/g; s/^_+//; s/_+$//')"
  if [[ -z "${out}" ]]; then
    out="step"
  fi
  printf "%s" "${out}"
}

ps_init_log_context() {
  local root
  root="$(ps_repo_root)"

  export POLYSYNTH_LOG_ROOT="${POLYSYNTH_LOG_ROOT:-${root}/.artifacts/logs}"
  export POLYSYNTH_RUN_ID="${POLYSYNTH_RUN_ID:-$(date +"%Y%m%d-%H%M%S")-$$}"
  export POLYSYNTH_RUN_DIR="${POLYSYNTH_LOG_ROOT}/${POLYSYNTH_RUN_ID}"

  mkdir -p "${POLYSYNTH_RUN_DIR}"
}

ps_append_summary() {
  local step="$1"
  local status="$2"
  local duration_s="$3"
  local logfile="$4"
  local summary_file="${POLYSYNTH_RUN_DIR}/summary.txt"

  if [[ ! -f "${summary_file}" ]]; then
    {
      echo "run_id=${POLYSYNTH_RUN_ID}"
      echo "run_dir=${POLYSYNTH_RUN_DIR}"
      echo "timestamp_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
      echo
      printf "%-24s %-8s %-10s %s\n" "STEP" "STATUS" "DURATION" "LOG"
    } > "${summary_file}"
  fi

  printf "%-24s %-8s %-10s %s\n" "${step}" "${status}" "${duration_s}s" "${logfile}" >> "${summary_file}"
}

ps_print_issue_excerpt() {
  local logfile="$1"
  local pattern
  local matches=""

  pattern='error|failed|failure|fatal|exception|traceback|assert|undefined reference|sanitizer|\bfail\b'

  if command -v rg >/dev/null 2>&1; then
    matches="$(rg -n -i "${pattern}" "${logfile}" 2>/dev/null | tail -n 40 || true)"
  else
    matches="$(grep -Ein "${pattern}" "${logfile}" 2>/dev/null | tail -n 40 || true)"
  fi

  if [[ -n "${matches}" ]]; then
    echo "Issue summary:"
    echo "${matches}"
  else
    echo "Issue summary (last 40 lines):"
    tail -n 40 "${logfile}" || true
  fi
}

ps_run_step() {
  if [[ $# -lt 2 ]]; then
    echo "ps_run_step requires: <step-name> <command...>" >&2
    return 2
  fi

  local step="$1"
  shift
  local cmd=("$@")

  ps_init_log_context

  local slug
  slug="$(ps_slug "${step}")"
  local logfile="${POLYSYNTH_RUN_DIR}/$(date +"%H%M%S")_${slug}.log"
  local started
  started="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  local start_s="${SECONDS}"
  local rc
  local duration_s

  {
    echo "# step: ${step}"
    echo "# started_utc: ${started}"
    printf "# command:"
    printf " %q" "${cmd[@]}"
    echo
    echo
  } > "${logfile}"

  set +e
  if [[ "${POLYSYNTH_VERBOSE:-0}" == "1" ]]; then
    "${cmd[@]}" > >(tee -a "${logfile}") 2> >(tee -a "${logfile}" >&2)
    rc=$?
  else
    "${cmd[@]}" >> "${logfile}" 2>&1
    rc=$?
  fi
  set -e

  duration_s="$((SECONDS - start_s))"

  if [[ ${rc} -eq 0 ]]; then
    echo "PASS ${step} (${duration_s}s)"
    ps_append_summary "${step}" "PASS" "${duration_s}" "${logfile}"
    return 0
  fi

  echo "FAIL ${step} (${duration_s}s) -> ${logfile}"
  ps_append_summary "${step}" "FAIL" "${duration_s}" "${logfile}"
  ps_print_issue_excerpt "${logfile}"
  echo "Full log: ${logfile}"
  echo "Run summary: ${POLYSYNTH_RUN_DIR}/summary.txt"
  return "${rc}"
}

ps_latest_run_dir() {
  local root
  root="$(ps_repo_root)"
  local log_root="${POLYSYNTH_LOG_ROOT:-${root}/.artifacts/logs}"

  if [[ ! -d "${log_root}" ]]; then
    return 1
  fi

  local latest
  latest="$(find "${log_root}" -mindepth 1 -maxdepth 1 -type d -print 2>/dev/null | sort | tail -n 1)"
  if [[ -z "${latest}" ]]; then
    return 1
  fi
  printf "%s\n" "${latest}"
}
