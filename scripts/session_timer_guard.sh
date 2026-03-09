#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
export TEAM_TIMER_STATE_ROOT="${TEAM_TIMER_STATE_ROOT:-/tmp/highperformanceFEM_team_timer}"
exec python3 "${REPO_ROOT}/tools/team_timer/team_timer.py" guard "$@"
