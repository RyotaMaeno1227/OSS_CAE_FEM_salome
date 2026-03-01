#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
POLICY="${2:-pass}"
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE:-scripts/c_coupled_freeze_forbidden_paths.txt}"

if [[ "${TEAM_STATUS_PATH}" == "--help" || "${TEAM_STATUS_PATH}" == "-h" ]]; then
  cat <<'EOF'
Usage: scripts/check_c_team_dryrun_compliance.sh [team_status_path] [policy]

policy:
  pass          require dryrun_result=pass
  pass_section  require dryrun_result=pass + latest C entry in ## Cチーム section
  pass_section_freeze  require pass_section + coupled freeze path policy
  pass_section_freeze_timer  require pass_section_freeze + timer completion
  pass_section_freeze_timer_safe  require pass_section_freeze_timer + safe_stage_command evidence + no template placeholders
  both          require dryrun_result=pass and dryrun_result=fail
  both_section  require both + latest C entry in ## Cチーム section
  both_section_freeze  require both_section + coupled freeze path policy
  both_section_freeze_timer  require both_section_freeze + timer completion
  none          only require dryrun_result presence and command evidence

env:
  COUPLED_FREEZE_FILE  optional forbid path list (default: scripts/c_coupled_freeze_forbidden_paths.txt)
EOF
  exit 0
fi

case "${POLICY}" in
  pass)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-pass
    ;;
  pass_section)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-pass \
      --require-c-section
    ;;
  pass_section_freeze)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-pass \
      --require-c-section \
      --coupled-freeze-file "${COUPLED_FREEZE_FILE}" \
      --require-coupled-freeze
    ;;
  pass_section_freeze_timer)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-pass \
      --require-c-section \
      --coupled-freeze-file "${COUPLED_FREEZE_FILE}" \
      --require-coupled-freeze \
      --require-complete-timer
    ;;
  pass_section_freeze_timer_safe)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-pass \
      --require-c-section \
      --coupled-freeze-file "${COUPLED_FREEZE_FILE}" \
      --require-coupled-freeze \
      --require-complete-timer \
      --require-safe-stage-command \
      --require-no-template-placeholder
    ;;
  both)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-both
    ;;
  both_section)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-both \
      --require-c-section
    ;;
  both_section_freeze)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-both \
      --require-c-section \
      --coupled-freeze-file "${COUPLED_FREEZE_FILE}" \
      --require-coupled-freeze
    ;;
  both_section_freeze_timer)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}" \
      --require-both \
      --require-c-section \
      --coupled-freeze-file "${COUPLED_FREEZE_FILE}" \
      --require-coupled-freeze \
      --require-complete-timer
    ;;
  none)
    python scripts/audit_c_team_staging.py \
      --team-status "${TEAM_STATUS_PATH}"
    ;;
  *)
    echo "ERROR: invalid policy '${POLICY}' (use: pass|pass_section|pass_section_freeze|pass_section_freeze_timer|pass_section_freeze_timer_safe|both|both_section|both_section_freeze|both_section_freeze_timer|none)" >&2
    exit 2
    ;;
esac
