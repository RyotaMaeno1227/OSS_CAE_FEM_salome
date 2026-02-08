#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"

stamp="$(date -u +%Y%m%dT%H%M%SZ)"
json_out="/tmp/team_audit_${stamp}.json"
c_json_out="/tmp/c_staging_audit_${stamp}.json"

set +e
python scripts/audit_team_sessions.py \
  --team-status "${TEAM_STATUS_PATH}" \
  --min-elapsed "${MIN_ELAPSED}" \
  --json > "${json_out}"
audit_rc=$?
set -e

echo "AUDIT_JSON=${json_out}"
python scripts/render_audit_feedback.py "${json_out}"

set +e
python scripts/audit_c_team_staging.py \
  --team-status "${TEAM_STATUS_PATH}" \
  --json > "${c_json_out}"
c_rc=$?
set -e

echo "C_STAGING_AUDIT_JSON=${c_json_out}"
cat "${c_json_out}"

if [[ "${audit_rc}" -ne 0 || "${c_rc}" -ne 0 ]]; then
  exit 1
fi
exit 0
