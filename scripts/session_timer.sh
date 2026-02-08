#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/session_timer.sh start <team_tag>
  scripts/session_timer.sh end <session_token_path>

Examples:
  scripts/session_timer.sh start c_team
  scripts/session_timer.sh end /tmp/c_team_session_20260207T010203Z_12345.token
EOF
}

cmd="${1:-}"

if [[ "${cmd}" == "start" ]]; then
  team_tag="${2:-}"
  if [[ -z "${team_tag}" ]]; then
    echo "ERROR: team_tag is required for start" >&2
    usage
    exit 2
  fi

  now_utc="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  now_epoch="$(date +%s)"
  stamp="$(date -u +"%Y%m%dT%H%M%SZ")"
  token="/tmp/${team_tag}_session_${stamp}_$$.token"

  {
    printf 'team_tag=%s\n' "${team_tag}"
    printf 'start_utc=%s\n' "${now_utc}"
    printf 'start_epoch=%s\n' "${now_epoch}"
  } > "${token}"

  echo "SESSION_TIMER_START"
  echo "session_token=${token}"
  cat "${token}"
  exit 0
fi

if [[ "${cmd}" == "end" ]]; then
  token="${2:-}"
  if [[ -z "${token}" ]]; then
    echo "ERROR: session_token_path is required for end" >&2
    usage
    exit 2
  fi
  if [[ ! -f "${token}" ]]; then
    echo "ERROR: token file not found: ${token}" >&2
    exit 2
  fi

  team_tag="$(sed -n 's/^team_tag=//p' "${token}" | head -n1)"
  start_utc="$(sed -n 's/^start_utc=//p' "${token}" | head -n1)"
  start_epoch="$(sed -n 's/^start_epoch=//p' "${token}" | head -n1)"

  if [[ -z "${team_tag}" || -z "${start_utc}" || -z "${start_epoch}" ]]; then
    echo "ERROR: token file is malformed: ${token}" >&2
    exit 2
  fi
  if ! [[ "${start_epoch}" =~ ^[0-9]+$ ]]; then
    echo "ERROR: start_epoch is not numeric in token: ${token}" >&2
    exit 2
  fi

  end_utc="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  end_epoch="$(date +%s)"
  elapsed_sec=$((end_epoch - start_epoch))
  if (( elapsed_sec < 0 )); then
    elapsed_sec=0
  fi
  elapsed_min=$((elapsed_sec / 60))

  echo "SESSION_TIMER_END"
  echo "session_token=${token}"
  echo "team_tag=${team_tag}"
  echo "start_utc=${start_utc}"
  echo "end_utc=${end_utc}"
  echo "start_epoch=${start_epoch}"
  echo "end_epoch=${end_epoch}"
  echo "elapsed_sec=${elapsed_sec}"
  echo "elapsed_min=${elapsed_min}"
  exit 0
fi

echo "ERROR: unknown command: ${cmd}" >&2
usage
exit 2
