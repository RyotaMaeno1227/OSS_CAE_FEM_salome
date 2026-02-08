#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"

stamp="$(date -u +%Y%m%dT%H%M%SZ)"
json_out="/tmp/team_audit_${stamp}.json"

set +e
python scripts/audit_team_sessions.py \
  --team-status "${TEAM_STATUS_PATH}" \
  --min-elapsed "${MIN_ELAPSED}" \
  --json > "${json_out}"
audit_rc=$?
set -e

echo "AUDIT_JSON=${json_out}"
python scripts/render_audit_feedback.py "${json_out}"

exit "${audit_rc}"
