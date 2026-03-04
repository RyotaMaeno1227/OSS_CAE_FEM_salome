#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"
MAX_ELAPSED="${TEAM_ACCEPTANCE_MAX_ELAPSED:-90}"
MAX_CONSECUTIVE_SAME_COMMAND="${TEAM_ACCEPTANCE_MAX_CONSECUTIVE_SAME_COMMAND:-1}"
REQUIRE_IMPL_CHANGES="${TEAM_ACCEPTANCE_REQUIRE_IMPL_CHANGES:-1}"

declare -a AUDIT_ARGS=(
  --team-status "${TEAM_STATUS_PATH}"
  --min-elapsed "${MIN_ELAPSED}"
  --max-elapsed "${MAX_ELAPSED}"
  --max-consecutive-same-command "${MAX_CONSECUTIVE_SAME_COMMAND}"
)

case "${REQUIRE_IMPL_CHANGES}" in
  0)
    ;;
  1)
    AUDIT_ARGS+=(--require-impl-changes)
    ;;
  *)
    echo "ERROR: invalid TEAM_ACCEPTANCE_REQUIRE_IMPL_CHANGES: ${REQUIRE_IMPL_CHANGES} (use: 0|1)" >&2
    exit 2
    ;;
esac

python scripts/audit_team_sessions.py "${AUDIT_ARGS[@]}"
