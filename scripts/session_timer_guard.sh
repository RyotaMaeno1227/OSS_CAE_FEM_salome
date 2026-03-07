#!/usr/bin/env bash
set -euo pipefail

STATE_ROOT="${SESSION_TIMER_STATE_ROOT:-/tmp/codex_team_control}"
ACTIVE_DIR="${STATE_ROOT}/active"

write_state_file() {
  local path="$1"
  shift
  mkdir -p "$(dirname "${path}")"
  {
    for line in "$@"; do
      printf '%s\n' "${line}"
    done
  } > "${path}"
}

upsert_state_file() {
  local path="$1"
  shift
  local tmp
  tmp="$(mktemp)"
  if [[ -f "${path}" ]]; then
    cat "${path}" > "${tmp}"
  fi
  for kv in "$@"; do
    local key="${kv%%=*}"
    grep -v "^${key}=" "${tmp}" > "${tmp}.next" || true
    mv "${tmp}.next" "${tmp}"
    printf '%s\n' "${kv}" >> "${tmp}"
  done
  mkdir -p "$(dirname "${path}")"
  mv "${tmp}" "${path}"
}

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

guard_result="pass"
if (( elapsed_min < min_elapsed )); then
  guard_result="block"
fi

upsert_state_file "${ACTIVE_DIR}/${team_tag}.env" \
  "status=active" \
  "session_token=${token}" \
  "team_tag=${team_tag}" \
  "start_utc=${start_utc}" \
  "start_epoch=${start_epoch}" \
  "last_seen_utc=${now_utc}" \
  "last_seen_epoch=${now_epoch}" \
  "last_seen_source=guard" \
  "last_guard_utc=${now_utc}" \
  "last_guard_epoch=${now_epoch}" \
  "last_guard_min_required=${min_elapsed}" \
  "last_guard_result=${guard_result}"

echo "guard_result=${guard_result}"
if [[ "${guard_result}" == "block" ]]; then
  exit 1
fi
exit 0
