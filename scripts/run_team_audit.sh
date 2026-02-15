#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"
C_DRYRUN_POLICY="${3:-pass}"
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE:-scripts/c_coupled_freeze_forbidden_paths.txt}"
TEAM_AUDIT_REQUIRE_IMPL_CHANGES="${TEAM_AUDIT_REQUIRE_IMPL_CHANGES:-0}"

declare -a C_AUDIT_FLAGS=()
declare -a TEAM_AUDIT_FLAGS=()

case "${TEAM_AUDIT_REQUIRE_IMPL_CHANGES}" in
  0)
    ;;
  1)
    TEAM_AUDIT_FLAGS+=(--require-impl-changes)
    ;;
  *)
    echo "ERROR: invalid TEAM_AUDIT_REQUIRE_IMPL_CHANGES: ${TEAM_AUDIT_REQUIRE_IMPL_CHANGES} (use: 0|1)" >&2
    exit 2
    ;;
esac

case "${C_DRYRUN_POLICY}" in
  pass)
    C_AUDIT_FLAGS=(--require-pass)
    ;;
  pass_section)
    C_AUDIT_FLAGS=(--require-pass --require-c-section)
    ;;
  pass_section_freeze)
    C_AUDIT_FLAGS=(
      --require-pass
      --require-c-section
      --require-coupled-freeze
      --coupled-freeze-file
      "${COUPLED_FREEZE_FILE}"
    )
    ;;
  pass_section_freeze_timer)
    C_AUDIT_FLAGS=(
      --require-pass
      --require-c-section
      --require-coupled-freeze
      --require-complete-timer
      --coupled-freeze-file
      "${COUPLED_FREEZE_FILE}"
    )
    ;;
  pass_section_freeze_timer_safe)
    C_AUDIT_FLAGS=(
      --require-pass
      --require-c-section
      --require-coupled-freeze
      --require-complete-timer
      --require-safe-stage-command
      --coupled-freeze-file
      "${COUPLED_FREEZE_FILE}"
    )
    ;;
  both)
    C_AUDIT_FLAGS=(--require-both)
    ;;
  both_section)
    C_AUDIT_FLAGS=(--require-both --require-c-section)
    ;;
  both_section_freeze)
    C_AUDIT_FLAGS=(
      --require-both
      --require-c-section
      --require-coupled-freeze
      --coupled-freeze-file
      "${COUPLED_FREEZE_FILE}"
    )
    ;;
  both_section_freeze_timer)
    C_AUDIT_FLAGS=(
      --require-both
      --require-c-section
      --require-coupled-freeze
      --require-complete-timer
      --coupled-freeze-file
      "${COUPLED_FREEZE_FILE}"
    )
    ;;
  none)
    C_AUDIT_FLAGS=()
    ;;
  *)
    echo "ERROR: invalid C dry-run policy: ${C_DRYRUN_POLICY} (use: pass|pass_section|pass_section_freeze|pass_section_freeze_timer|pass_section_freeze_timer_safe|both|both_section|both_section_freeze|both_section_freeze_timer|none)" >&2
    exit 2
    ;;
esac

json_out="$(mktemp /tmp/team_audit_XXXXXX.json)"
c_json_out="$(mktemp /tmp/c_staging_audit_XXXXXX.json)"

set +e
python scripts/audit_team_sessions.py \
  --team-status "${TEAM_STATUS_PATH}" \
  --min-elapsed "${MIN_ELAPSED}" \
  "${TEAM_AUDIT_FLAGS[@]}" \
  --json > "${json_out}"
audit_rc=$?
set -e

echo "AUDIT_JSON=${json_out}"
python scripts/render_audit_feedback.py "${json_out}"

set +e
python scripts/audit_c_team_staging.py \
  --team-status "${TEAM_STATUS_PATH}" \
  "${C_AUDIT_FLAGS[@]}" \
  --json > "${c_json_out}"
c_rc=$?
set -e

echo "C_STAGING_AUDIT_JSON=${c_json_out}"
cat "${c_json_out}"

if [[ "${audit_rc}" -ne 0 || "${c_rc}" -ne 0 ]]; then
  exit 1
fi
exit 0
