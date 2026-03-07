#!/usr/bin/env bash
set -euo pipefail

STATE_ROOT="${SESSION_TIMER_STATE_ROOT:-/tmp/codex_team_control}"
ACTIVE_DIR="${STATE_ROOT}/active"
LAST_DIR="${STATE_ROOT}/last"

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
  scripts/session_timer.sh start <team_tag>
  scripts/session_timer.sh declare <session_token_path> <primary_task> <secondary_task> [plan_note]
  scripts/session_timer.sh end <session_token_path>

Examples:
  scripts/session_timer.sh start c_team
  scripts/session_timer.sh declare /tmp/c_team_session_20260207T010203Z_12345.token C-49 C-50 "primary完了後はC-50へ遷移"
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

  mkdir -p "${ACTIVE_DIR}" "${LAST_DIR}"
  write_state_file "${ACTIVE_DIR}/${team_tag}.env" \
    "status=active" \
    "session_token=${token}" \
    "team_tag=${team_tag}" \
    "start_utc=${now_utc}" \
    "start_epoch=${now_epoch}" \
    "last_seen_utc=${now_utc}" \
    "last_seen_epoch=${now_epoch}" \
    "last_seen_source=start"

  echo "SESSION_TIMER_START"
  echo "session_token=${token}"
  cat "${token}"
  exit 0
fi

if [[ "${cmd}" == "declare" ]]; then
  token="${2:-}"
  primary_task="${3:-}"
  secondary_task="${4:-}"
  shift_count=4
  if [[ -z "${token}" || -z "${primary_task}" || -z "${secondary_task}" ]]; then
    echo "ERROR: declare requires <session_token_path> <primary_task> <secondary_task>" >&2
    usage
    exit 2
  fi
  shift "${shift_count}" || true
  plan_note="${*:-}"

  if [[ ! -f "${token}" ]]; then
    echo "ERROR: token file not found: ${token}" >&2
    exit 2
  fi
  if [[ "${primary_task}" == "-" || "${secondary_task}" == "-" ]]; then
    echo "ERROR: primary_task / secondary_task must be explicit task ids (create Auto-Next first if needed)" >&2
    exit 2
  fi

  team_tag="$(sed -n 's/^team_tag=//p' "${token}" | head -n1)"
  start_utc="$(sed -n 's/^start_utc=//p' "${token}" | head -n1)"
  start_epoch="$(sed -n 's/^start_epoch=//p' "${token}" | head -n1)"
  if [[ -z "${team_tag}" || -z "${start_utc}" || -z "${start_epoch}" ]]; then
    echo "ERROR: token file is malformed: ${token}" >&2
    exit 2
  fi

  plan_utc="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  plan_epoch="$(date +%s)"
  mkdir -p "${ACTIVE_DIR}" "${LAST_DIR}"
  upsert_state_file "${token}" \
    "declared_primary_task=${primary_task}" \
    "declared_secondary_task=${secondary_task}" \
    "plan_utc=${plan_utc}" \
    "plan_epoch=${plan_epoch}" \
    "plan_note=${plan_note}"
  upsert_state_file "${ACTIVE_DIR}/${team_tag}.env" \
    "status=active" \
    "session_token=${token}" \
    "team_tag=${team_tag}" \
    "start_utc=${start_utc}" \
    "start_epoch=${start_epoch}" \
    "last_seen_utc=${plan_utc}" \
    "last_seen_epoch=${plan_epoch}" \
    "last_seen_source=declare" \
    "declared_primary_task=${primary_task}" \
    "declared_secondary_task=${secondary_task}" \
    "plan_utc=${plan_utc}" \
    "plan_epoch=${plan_epoch}" \
    "plan_note=${plan_note}"

  echo "SESSION_TIMER_DECLARE"
  echo "session_token=${token}"
  echo "team_tag=${team_tag}"
  echo "primary_task=${primary_task}"
  echo "secondary_task=${secondary_task}"
  echo "plan_utc=${plan_utc}"
  echo "plan_epoch=${plan_epoch}"
  echo "plan_note=${plan_note}"
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

  mkdir -p "${ACTIVE_DIR}" "${LAST_DIR}"
  declared_primary_task="$(sed -n 's/^declared_primary_task=//p' "${token}" | tail -n1)"
  declared_secondary_task="$(sed -n 's/^declared_secondary_task=//p' "${token}" | tail -n1)"
  plan_utc="$(sed -n 's/^plan_utc=//p' "${token}" | tail -n1)"
  plan_epoch="$(sed -n 's/^plan_epoch=//p' "${token}" | tail -n1)"
  plan_note="$(sed -n 's/^plan_note=//p' "${token}" | tail -n1)"
  last_lines=(
    "status=ended"
    "session_token=${token}"
    "team_tag=${team_tag}"
    "start_utc=${start_utc}"
    "end_utc=${end_utc}"
    "start_epoch=${start_epoch}"
    "end_epoch=${end_epoch}"
    "elapsed_sec=${elapsed_sec}"
    "elapsed_min=${elapsed_min}"
  )
  if [[ -n "${declared_primary_task}" ]]; then
    last_lines+=("declared_primary_task=${declared_primary_task}")
  fi
  if [[ -n "${declared_secondary_task}" ]]; then
    last_lines+=("declared_secondary_task=${declared_secondary_task}")
  fi
  if [[ -n "${plan_utc}" ]]; then
    last_lines+=("plan_utc=${plan_utc}")
  fi
  if [[ -n "${plan_epoch}" ]]; then
    last_lines+=("plan_epoch=${plan_epoch}")
  fi
  if [[ -n "${plan_note}" ]]; then
    last_lines+=("plan_note=${plan_note}")
  fi
  write_state_file "${LAST_DIR}/${team_tag}.env" "${last_lines[@]}"
  if [[ -f "${ACTIVE_DIR}/${team_tag}.env" ]]; then
    active_token="$(sed -n 's/^session_token=//p' "${ACTIVE_DIR}/${team_tag}.env" | head -n1 || true)"
    if [[ -z "${active_token}" || "${active_token}" == "${token}" ]]; then
      rm -f "${ACTIVE_DIR}/${team_tag}.env"
    fi
  fi

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
