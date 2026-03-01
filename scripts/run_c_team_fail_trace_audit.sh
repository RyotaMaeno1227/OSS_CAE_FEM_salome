#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/c_team_review_reason_utils.sh
source "${SCRIPT_DIR}/c_team_review_reason_utils.sh"

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"
READINESS_SCRIPT="${C_FAIL_TRACE_READINESS_SCRIPT:-scripts/check_c_team_submission_readiness.sh}"
STAGING_SCRIPT="${C_FAIL_TRACE_STAGING_SCRIPT:-scripts/run_c_team_staging_checks.sh}"
ORDER_CHECKER_SCRIPT="${C_FAIL_TRACE_ORDER_CHECKER_SCRIPT:-scripts/check_c_team_fail_trace_order.py}"
RETRY_CONSISTENCY_SCRIPT="${C_FAIL_TRACE_RETRY_CONSISTENCY_SCRIPT:-scripts/check_c_team_fail_trace_retry_consistency.py}"
REQUIRE_REVIEW="${C_FAIL_TRACE_REQUIRE_REVIEW:-1}"
SKIP_NESTED_SELFTESTS="${C_FAIL_TRACE_SKIP_NESTED_SELFTESTS:-1}"
REQUIRE_RETRY_CONSISTENCY="$(c_team_resolve_binary_toggle "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY" "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY" "1")"
REQUIRE_RETRY_CONSISTENCY_KEY="$(c_team_resolve_binary_toggle "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY" "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY" "0")"
REQUIRE_RETRY_CONSISTENCY_STRICT_ENV="$(c_team_resolve_binary_toggle "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV" "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV" "0")"

if [[ "${TEAM_STATUS_PATH}" == "--help" || "${TEAM_STATUS_PATH}" == "-h" ]]; then
  cat <<'EOF'
Usage: scripts/run_c_team_fail_trace_audit.sh [team_status_path] [min_elapsed]

Capture readiness/staging strict+default logs and validate fail-trace ordering.

env:
  C_FAIL_TRACE_READINESS_SCRIPT=scripts/check_c_team_submission_readiness.sh
  C_FAIL_TRACE_STAGING_SCRIPT=scripts/run_c_team_staging_checks.sh
  C_FAIL_TRACE_ORDER_CHECKER_SCRIPT=scripts/check_c_team_fail_trace_order.py
  C_FAIL_TRACE_RETRY_CONSISTENCY_SCRIPT=scripts/check_c_team_fail_trace_retry_consistency.py
  C_FAIL_TRACE_REQUIRE_REVIEW=0|1
  C_FAIL_TRACE_SKIP_NESTED_SELFTESTS=0|1
  C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=0|1
  C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=0|1
  C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=0|1
EOF
  exit 0
fi

readiness_default_log="$(mktemp /tmp/c47_readiness_default.XXXXXX.log)"
readiness_strict_log="$(mktemp /tmp/c47_readiness_strict.XXXXXX.log)"
staging_default_log="$(mktemp /tmp/c47_staging_default.XXXXXX.log)"
staging_strict_log="$(mktemp /tmp/c47_staging_strict.XXXXXX.log)"

printf '[1/9] readiness default capture\n'
set +e
C_REQUIRE_REVIEW_COMMANDS="${REQUIRE_REVIEW}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="${REQUIRE_RETRY_CONSISTENCY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="${REQUIRE_RETRY_CONSISTENCY_KEY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" \
  bash "${READINESS_SCRIPT}" "${TEAM_STATUS_PATH}" "${MIN_ELAPSED}" >"${readiness_default_log}" 2>&1
readiness_default_rc=$?
set -e
if [[ "${readiness_default_rc}" != "0" ]]; then
  echo "readiness default capture failed: rc=${readiness_default_rc}" >&2
  cat "${readiness_default_log}" >&2
  echo "readiness_default_log=${readiness_default_log}" >&2
  echo "FAIL_TRACE_AUDIT_RESULT=FAIL" >&2
  exit "${readiness_default_rc}"
fi

printf '[2/9] readiness strict capture (expected fail)\n'
set +e
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS="${REQUIRE_REVIEW}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="${REQUIRE_RETRY_CONSISTENCY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="${REQUIRE_RETRY_CONSISTENCY_KEY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" \
  bash "${READINESS_SCRIPT}" "${TEAM_STATUS_PATH}" "${MIN_ELAPSED}" >"${readiness_strict_log}" 2>&1
readiness_strict_rc=$?
set -e
if [[ "${readiness_strict_rc}" == "0" ]]; then
  echo "strict readiness unexpectedly succeeded" >&2
  cat "${readiness_strict_log}" >&2
  echo "readiness_strict_log=${readiness_strict_log}" >&2
  exit 1
fi

printf '[3/9] staging default capture\n'
set +e
C_SKIP_NESTED_SELFTESTS="${SKIP_NESTED_SELFTESTS}" C_REQUIRE_REVIEW_COMMANDS="${REQUIRE_REVIEW}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="${REQUIRE_RETRY_CONSISTENCY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="${REQUIRE_RETRY_CONSISTENCY_KEY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" \
  bash "${STAGING_SCRIPT}" "${TEAM_STATUS_PATH}" >"${staging_default_log}" 2>&1
staging_default_rc=$?
set -e
if [[ "${staging_default_rc}" != "0" ]]; then
  echo "staging default capture failed: rc=${staging_default_rc}" >&2
  cat "${staging_default_log}" >&2
  echo "staging_default_log=${staging_default_log}" >&2
  echo "FAIL_TRACE_AUDIT_RESULT=FAIL" >&2
  exit "${staging_default_rc}"
fi

printf '[4/9] staging strict capture (expected fail)\n'
set +e
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_SKIP_NESTED_SELFTESTS="${SKIP_NESTED_SELFTESTS}" C_REQUIRE_REVIEW_COMMANDS="${REQUIRE_REVIEW}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="${REQUIRE_RETRY_CONSISTENCY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="${REQUIRE_RETRY_CONSISTENCY_KEY}" C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" \
  bash "${STAGING_SCRIPT}" "${TEAM_STATUS_PATH}" >"${staging_strict_log}" 2>&1
staging_strict_rc=$?
set -e
if [[ "${staging_strict_rc}" == "0" ]]; then
  echo "strict staging unexpectedly succeeded" >&2
  cat "${staging_strict_log}" >&2
  echo "staging_strict_log=${staging_strict_log}" >&2
  exit 1
fi

printf '[5/9] readiness default order check\n'
python "${ORDER_CHECKER_SCRIPT}" "${readiness_default_log}" --mode default

printf '[6/9] readiness strict order check\n'
python "${ORDER_CHECKER_SCRIPT}" "${readiness_strict_log}" --mode strict

printf '[7/9] staging default order check\n'
python "${ORDER_CHECKER_SCRIPT}" "${staging_default_log}" --mode default

printf '[8/9] staging strict order check\n'
python "${ORDER_CHECKER_SCRIPT}" "${staging_strict_log}" --mode strict

printf '[9/9] fail-trace retry consistency check\n'
echo "fail_trace_require_retry_consistency=${REQUIRE_RETRY_CONSISTENCY}"
echo "fail_trace_require_retry_consistency_key=${REQUIRE_RETRY_CONSISTENCY_KEY}"
echo "fail_trace_require_retry_consistency_strict_env=${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}"
retry_consistency_command_display="python ${RETRY_CONSISTENCY_SCRIPT} --team-status ${TEAM_STATUS_PATH}"
if [[ "${REQUIRE_RETRY_CONSISTENCY_KEY}" == "1" ]]; then
  retry_consistency_command_display+=" --require-retry-consistency-check-key"
fi
if [[ "${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" == "1" ]]; then
  retry_consistency_command_display+=" --require-strict-env-prefix-match"
fi
echo "fail_trace_retry_consistency_command=${retry_consistency_command_display}"
echo "fail_trace_retry_consistency_retry_command=${retry_consistency_command_display}"
if [[ "${REQUIRE_RETRY_CONSISTENCY}" == "1" ]]; then
  retry_consistency_cmd=(python "${RETRY_CONSISTENCY_SCRIPT}" --team-status "${TEAM_STATUS_PATH}")
  if [[ "${REQUIRE_RETRY_CONSISTENCY_KEY}" == "1" ]]; then
    retry_consistency_cmd+=(--require-retry-consistency-check-key)
  fi
  if [[ "${REQUIRE_RETRY_CONSISTENCY_STRICT_ENV}" == "1" ]]; then
    retry_consistency_cmd+=(--require-strict-env-prefix-match)
  fi
  set +e
  retry_consistency_output="$("${retry_consistency_cmd[@]}" 2>&1)"
  retry_consistency_rc=$?
  set -e
  retry_consistency_reasons="$(
    printf '%s\n' "${retry_consistency_output}" | awk -F= '/^reasons=/{value=substr($0, 9)} END{print value}'
  )"
  retry_consistency_reason_codes="$(
    printf '%s\n' "${retry_consistency_output}" | awk -F= '/^reason_codes=/{value=substr($0, 14)} END{print value}'
  )"
  if [[ "${retry_consistency_rc}" == "0" ]]; then
    echo "${retry_consistency_output}"
    echo "fail_trace_retry_consistency_reasons=${retry_consistency_reasons:--}"
    echo "fail_trace_retry_consistency_reason_codes=${retry_consistency_reason_codes:--}"
    echo "fail_trace_retry_consistency_check=pass"
  else
    echo "${retry_consistency_output}" >&2
    echo "fail_trace_retry_consistency_reasons=${retry_consistency_reasons:-unknown}" >&2
    echo "fail_trace_retry_consistency_reason_codes=${retry_consistency_reason_codes:-unknown}" >&2
    echo "fail_trace_retry_consistency_check=fail" >&2
    echo "FAIL_TRACE_AUDIT_RESULT=FAIL" >&2
    exit "${retry_consistency_rc}"
  fi
else
  echo "fail_trace_retry_consistency_reasons=-"
  echo "fail_trace_retry_consistency_reason_codes=-"
  echo "fail_trace_retry_consistency_check=skipped"
fi

echo "readiness_default_log=${readiness_default_log}"
echo "readiness_strict_log=${readiness_strict_log}"
echo "staging_default_log=${staging_default_log}"
echo "staging_strict_log=${staging_strict_log}"
echo "readiness_strict_rc=${readiness_strict_rc}"
echo "staging_strict_rc=${staging_strict_rc}"
echo "FAIL_TRACE_AUDIT_RESULT=PASS"
