#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/session_timer_guard.sh <session_token_path> [min_elapsed_minutes]

Examples:
  scripts/session_timer_guard.sh /tmp/a_team_session_20260208T150220Z_19629.token
  scripts/session_timer_guard.sh /tmp/a_team_session_20260208T150220Z_19629.token 30
EOF
}

token="${1:-}"
min_elapsed="${2:-30}"

if [[ -z "${token}" ]]; then
  echo "ERROR: session_token_path is required" >&2
  usage
  exit 2
fi

if [[ ! -f "${token}" ]]; then
  echo "ERROR: token file not found: ${token}" >&2
  exit 2
fi

if ! [[ "${min_elapsed}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: min_elapsed_minutes must be integer: ${min_elapsed}" >&2
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

now_utc="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
now_epoch="$(date +%s)"
elapsed_sec=$((now_epoch - start_epoch))
if (( elapsed_sec < 0 )); then
  elapsed_sec=0
fi
elapsed_min=$((elapsed_sec / 60))

echo "SESSION_TIMER_GUARD"
echo "session_token=${token}"
echo "team_tag=${team_tag}"
echo "start_utc=${start_utc}"
echo "now_utc=${now_utc}"
echo "start_epoch=${start_epoch}"
echo "now_epoch=${now_epoch}"
echo "elapsed_sec=${elapsed_sec}"
echo "elapsed_min=${elapsed_min}"
echo "min_required=${min_elapsed}"

if (( elapsed_min < min_elapsed )); then
  echo "guard_result=block"
  exit 1
fi

echo "guard_result=pass"
exit 0
