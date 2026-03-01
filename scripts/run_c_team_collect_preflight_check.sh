#!/usr/bin/env bash
set -euo pipefail

TEAM_STATUS_PATH="${1:-docs/team_status.md}"

C_COLLECT_PREFLIGHT_LOG="${C_COLLECT_PREFLIGHT_LOG:-}"
C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED:-1}"
C_COLLECT_EXPECT_TEAM_STATUS="${C_COLLECT_EXPECT_TEAM_STATUS:-${TEAM_STATUS_PATH}}"
C_COLLECT_LATEST_REQUIRE_FOUND="${C_COLLECT_LATEST_REQUIRE_FOUND:-0}"
latest_mode=0

if [[ "${TEAM_STATUS_PATH}" == "--help" || "${TEAM_STATUS_PATH}" == "-h" ]]; then
cat <<'EOF'
Usage: scripts/run_c_team_collect_preflight_check.sh [team_status_path]

Run optional collect preflight report check with consistent output.

env:
  C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log
  C_COLLECT_PREFLIGHT_LOG=latest
  C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=0|1
  C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md
  C_COLLECT_LATEST_REQUIRE_FOUND=0|1
EOF
  exit 0
fi

if [[ -z "${C_COLLECT_PREFLIGHT_LOG}" ]]; then
  echo "collect_preflight_check=skipped"
  exit 0
fi

if [[ "${C_COLLECT_PREFLIGHT_LOG}" == "latest" ]]; then
  latest_mode=1
  if ! resolved_log="$(python scripts/extract_c_team_latest_collect_log.py --team-status "${TEAM_STATUS_PATH}" --print-path-only 2>&1)"; then
    if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND}" == "1" ]]; then
      echo "${resolved_log}" >&2
      echo "collect_preflight_check_reason=latest_not_found_strict" >&2
      echo "collect_preflight_check=fail" >&2
      exit 1
    fi
    echo "${resolved_log}"
    echo "collect_preflight_check_reason=latest_not_found_default_skip"
    echo "collect_preflight_check=skipped"
    exit 0
  fi
  echo "collect_preflight_log_resolved=${resolved_log}"
  C_COLLECT_PREFLIGHT_LOG="${resolved_log}"
  if [[ ! -f "${C_COLLECT_PREFLIGHT_LOG}" ]]; then
    echo "collect_preflight_log_missing=${C_COLLECT_PREFLIGHT_LOG}"
    if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND}" == "1" ]]; then
      echo "collect_preflight_check_reason=latest_resolved_log_missing_strict" >&2
      echo "collect_preflight_check=fail" >&2
      exit 1
    fi
    echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
    echo "collect_preflight_check=skipped"
    exit 0
  fi
fi

if [[ ! -f "${C_COLLECT_PREFLIGHT_LOG}" ]]; then
  echo "collect_preflight_log_missing=${C_COLLECT_PREFLIGHT_LOG}"
  echo "collect_preflight_check_reason=explicit_log_missing" >&2
  echo "collect_preflight_check=fail" >&2
  exit 1
fi

checker_args=("${C_COLLECT_PREFLIGHT_LOG}")
if [[ "${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED}" == "1" ]]; then
  checker_args+=(--require-enabled)
fi
if [[ -n "${C_COLLECT_EXPECT_TEAM_STATUS}" ]]; then
  checker_args+=(--expect-team-status "${C_COLLECT_EXPECT_TEAM_STATUS}")
fi

if ! python scripts/check_c_team_collect_preflight_report.py "${checker_args[@]}"; then
  # In latest auto-mode, treat invalid resolved reports as optional unless strict mode is requested.
  if [[ "${latest_mode}" == "1" && "${C_COLLECT_LATEST_REQUIRE_FOUND}" != "1" ]]; then
    echo "collect_preflight_check_reason=latest_invalid_report_default_skip"
    echo "collect_preflight_check=skipped"
    exit 0
  fi
  if [[ "${latest_mode}" == "1" ]]; then
    echo "collect_preflight_check_reason=latest_invalid_report_strict" >&2
  fi
  echo "collect_preflight_check=fail" >&2
  exit 1
fi
echo "collect_preflight_check=pass"
