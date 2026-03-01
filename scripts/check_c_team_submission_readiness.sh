#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/c_team_review_reason_utils.sh
source "${SCRIPT_DIR}/c_team_review_reason_utils.sh"

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
MIN_ELAPSED="${2:-30}"
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE:-scripts/c_coupled_freeze_forbidden_paths.txt}"
SKIP_STAGING_BUNDLE="${C_TEAM_SKIP_STAGING_BUNDLE:-0}"
COLLECT_PREFLIGHT_LOG_SOURCE="default_latest"
if [[ -v C_COLLECT_PREFLIGHT_LOG ]]; then
  # Allow explicit empty value to disable preflight resolution.
  C_COLLECT_PREFLIGHT_LOG="${C_COLLECT_PREFLIGHT_LOG}"
  COLLECT_PREFLIGHT_LOG_SOURCE="explicit_env"
else
  # Default to latest log extraction to reduce manual parameters.
  C_COLLECT_PREFLIGHT_LOG="latest"
fi
C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED:-1}"
C_COLLECT_EXPECT_TEAM_STATUS="${C_COLLECT_EXPECT_TEAM_STATUS:-${TEAM_STATUS_PATH}}"
C_COLLECT_LATEST_REQUIRE_FOUND="${C_COLLECT_LATEST_REQUIRE_FOUND:-0}"
C_REQUIRE_REVIEW_COMMANDS="${C_REQUIRE_REVIEW_COMMANDS:-0}"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY" "1")"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY" "0")"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV" "0")"

build_fail_trace_retry_consistency_retry_command() {
  local cmd="python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ${TEAM_STATUS_PATH}"
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}" == "1" ]]; then
    cmd+=" --require-retry-consistency-check-key"
  fi
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}" == "1" ]]; then
    cmd+=" --require-strict-env-prefix-match"
  fi
  printf '%s' "${cmd}"
}

emit_collect_preflight_retry_consistency_context() {
  local preflight_reason="${1:-unknown_collect_preflight_reason}"
  local normalized_reason
  normalized_reason="$(c_team_normalize_reason_code "${preflight_reason}")"
  if [[ -z "${normalized_reason}" ]]; then
    normalized_reason="unknown_collect_preflight_reason"
  fi
  echo "fail_trace_retry_consistency_reasons=collect_preflight_check_failed_before_retry_consistency (${preflight_reason})" >&2
  echo "fail_trace_retry_consistency_reason_codes=collect_preflight_${normalized_reason}_before_retry_consistency" >&2
  echo "fail_trace_retry_consistency_retry_command=$(build_fail_trace_retry_consistency_retry_command)" >&2
  echo "fail_trace_retry_consistency_check=unknown" >&2
}

build_submission_readiness_retry_command() {
  local command=""
  command+="C_COLLECT_PREFLIGHT_LOG=$(printf '%q' "${C_COLLECT_PREFLIGHT_LOG}") "
  command+="C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=$(printf '%q' "${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED}") "
  command+="C_COLLECT_EXPECT_TEAM_STATUS=$(printf '%q' "${C_COLLECT_EXPECT_TEAM_STATUS}") "
  command+="C_COLLECT_LATEST_REQUIRE_FOUND=$(printf '%q' "${C_COLLECT_LATEST_REQUIRE_FOUND}") "
  command+="C_REQUIRE_REVIEW_COMMANDS=$(printf '%q' "${C_REQUIRE_REVIEW_COMMANDS}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}") "
  command+="bash scripts/check_c_team_submission_readiness.sh "
  command+="$(printf '%q' "${TEAM_STATUS_PATH}") $(printf '%q' "${MIN_ELAPSED}")"
  printf '%s' "${command}"
}

build_review_command_retry_command() {
  printf 'python scripts/check_c_team_review_commands.py --team-status %q' "${TEAM_STATUS_PATH}"
}

emit_review_command_failure_context() {
  local review_output="${1:-}"
  local review_reasons
  local review_reason_codes
  local prefixed_reason_codes
  local reason_codes_source="fallback"
  review_reasons="$(
    printf '%s\n' "${review_output}" \
      | awk -F= '/^reasons=/{value=substr($0, index($0, "=") + 1)} END{print value}'
  )"
  review_reason_codes="$(
    printf '%s\n' "${review_output}" \
      | awk -F= '/^reason_codes=/{value=substr($0, index($0, "=") + 1)} END{print value}'
  )"
  if [[ -z "${review_reasons}" ]]; then
    review_reasons="unknown_review_command_reason"
  fi
  prefixed_reason_codes="$(
    c_team_build_prefixed_reason_codes "review_command_" "${review_reason_codes}" "${review_reasons}"
  )"
  if [[ -n "${review_reason_codes}" && "${review_reason_codes}" != "-" ]]; then
    reason_codes_source="checker"
  fi
  echo "review_command_fail_reason=${review_reasons}" >&2
  echo "review_command_fail_reason_codes=${prefixed_reason_codes}" >&2
  echo "review_command_fail_reason_codes_source=${reason_codes_source}" >&2
  echo "review_command_retry_command=$(build_review_command_retry_command)" >&2
  echo "submission_readiness_retry_command=$(build_submission_readiness_retry_command)" >&2
  echo "submission_readiness_fail_step=review_command" >&2
}

echo "submission_readiness_collect_preflight_log_source=${COLLECT_PREFLIGHT_LOG_SOURCE}"
echo "submission_readiness_collect_preflight_log_effective=${C_COLLECT_PREFLIGHT_LOG:-<empty>}"
echo "submission_readiness_require_review_commands=${C_REQUIRE_REVIEW_COMMANDS}"
echo "submission_readiness_require_fail_trace_retry_consistency=${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY}"
echo "submission_readiness_require_fail_trace_retry_consistency_key=${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}"
echo "submission_readiness_require_fail_trace_retry_consistency_strict_env=${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}"
echo "submission_readiness_collect_latest_require_found=${C_COLLECT_LATEST_REQUIRE_FOUND}"
echo "[0/6] optional collect preflight report check"
collect_preflight_output=""
collect_preflight_rc=0
set +e
collect_preflight_output="$(
  C_COLLECT_PREFLIGHT_LOG="${C_COLLECT_PREFLIGHT_LOG}" \
    C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED}" \
    C_COLLECT_EXPECT_TEAM_STATUS="${C_COLLECT_EXPECT_TEAM_STATUS}" \
    C_COLLECT_LATEST_REQUIRE_FOUND="${C_COLLECT_LATEST_REQUIRE_FOUND}" \
    bash scripts/run_c_team_collect_preflight_check.sh "${TEAM_STATUS_PATH}" 2>&1
)"
collect_preflight_rc=$?
set -e

collect_preflight_check="$(
  printf '%s\n' "${collect_preflight_output}" | awk -F= '/^collect_preflight_check=/{value=$2} END{print value}'
)"
collect_preflight_reason="$(
  printf '%s\n' "${collect_preflight_output}" | awk -F= '/^collect_preflight_check_reason=/{value=$2} END{print value}'
)"

if [[ -n "${collect_preflight_output}" ]]; then
  if [[ "${collect_preflight_rc}" == "0" ]]; then
    echo "${collect_preflight_output}"
  else
    echo "${collect_preflight_output}" >&2
  fi
fi
if [[ "${collect_preflight_rc}" != "0" ]]; then
  if [[ -n "${collect_preflight_check}" ]]; then
    echo "submission_readiness_collect_preflight_check=${collect_preflight_check}" >&2
  fi
  if [[ -n "${collect_preflight_reason}" ]]; then
    echo "submission_readiness_collect_preflight_reason=${collect_preflight_reason}" >&2
  fi
  emit_collect_preflight_retry_consistency_context "${collect_preflight_reason:-unknown_collect_preflight_reason}"
  echo "submission_readiness_retry_command=$(build_submission_readiness_retry_command)" >&2
  echo "submission_readiness_fail_step=collect_preflight" >&2
  exit "${collect_preflight_rc}"
fi
if [[ -n "${collect_preflight_check}" ]]; then
  echo "submission_readiness_collect_preflight_check=${collect_preflight_check}"
fi
if [[ -n "${collect_preflight_reason}" ]]; then
  echo "submission_readiness_collect_preflight_reason=${collect_preflight_reason}"
fi

echo "[1/6] optional review-command audit"
if [[ "${C_REQUIRE_REVIEW_COMMANDS}" == "1" ]]; then
  if review_command_output="$(python scripts/check_c_team_review_commands.py --team-status "${TEAM_STATUS_PATH}" 2>&1)"; then
    echo "${review_command_output}"
    echo "review_command_check=pass"
    echo "review_command_fail_reason=-"
    echo "review_command_fail_reason_codes=-"
    echo "review_command_fail_reason_codes_source=-"
    echo "review_command_retry_command=$(build_review_command_retry_command)"
  else
    echo "${review_command_output}" >&2
    echo "review_command_check=fail" >&2
    emit_review_command_failure_context "${review_command_output}"
    exit 1
  fi
else
  echo "skip_reason=C_REQUIRE_REVIEW_COMMANDS=0"
  echo "review_command_check=skipped"
  echo "review_command_fail_reason=-"
  echo "review_command_fail_reason_codes=-"
  echo "review_command_fail_reason_codes_source=-"
  echo "review_command_retry_command=$(build_review_command_retry_command)"
fi

echo "[2/6] optional fail-trace retry consistency audit"
if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY}" == "1" ]]; then
  fail_trace_retry_cmd=(
    python scripts/check_c_team_fail_trace_retry_consistency.py
    --team-status "${TEAM_STATUS_PATH}"
  )
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}" == "1" ]]; then
    fail_trace_retry_cmd+=(--require-retry-consistency-check-key)
  fi
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}" == "1" ]]; then
    fail_trace_retry_cmd+=(--require-strict-env-prefix-match)
  fi
  fail_trace_retry_replay_command="$(build_fail_trace_retry_consistency_retry_command)"
  if fail_trace_retry_output="$("${fail_trace_retry_cmd[@]}" 2>&1)"; then
    fail_trace_retry_reasons="$(
      printf '%s\n' "${fail_trace_retry_output}" | awk -F= '/^reasons=/{value=substr($0, 9)} END{print value}'
    )"
    fail_trace_retry_reason_codes="$(
      printf '%s\n' "${fail_trace_retry_output}" | awk -F= '/^reason_codes=/{value=substr($0, 14)} END{print value}'
    )"
    echo "${fail_trace_retry_output}"
    echo "fail_trace_retry_consistency_reasons=${fail_trace_retry_reasons:--}"
    echo "fail_trace_retry_consistency_reason_codes=${fail_trace_retry_reason_codes:--}"
    echo "fail_trace_retry_consistency_retry_command=${fail_trace_retry_replay_command}"
    echo "fail_trace_retry_consistency_check=pass"
  else
    fail_trace_retry_reasons="$(
      printf '%s\n' "${fail_trace_retry_output}" | awk -F= '/^reasons=/{value=substr($0, 9)} END{print value}'
    )"
    fail_trace_retry_reason_codes="$(
      printf '%s\n' "${fail_trace_retry_output}" | awk -F= '/^reason_codes=/{value=substr($0, 14)} END{print value}'
    )"
    echo "${fail_trace_retry_output}" >&2
    echo "fail_trace_retry_consistency_reasons=${fail_trace_retry_reasons:-unknown}" >&2
    echo "fail_trace_retry_consistency_reason_codes=${fail_trace_retry_reason_codes:-unknown}" >&2
    echo "fail_trace_retry_consistency_retry_command=${fail_trace_retry_replay_command}" >&2
    echo "fail_trace_retry_consistency_check=fail" >&2
    exit 1
  fi
else
  echo "skip_reason=C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=0"
  echo "fail_trace_retry_consistency_reasons=-"
  echo "fail_trace_retry_consistency_reason_codes=-"
  echo "fail_trace_retry_consistency_retry_command=$(build_fail_trace_retry_consistency_retry_command)"
  echo "fail_trace_retry_consistency_check=skipped"
fi

echo "[3/6] strict-safe dry-run compliance"
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE}" \
  bash scripts/check_c_team_dryrun_compliance.sh "${TEAM_STATUS_PATH}" pass_section_freeze_timer_safe

echo "[4/6] strict-safe staging bundle checks"
if [[ "${SKIP_STAGING_BUNDLE}" == "1" ]]; then
  echo "skip_reason=C_TEAM_SKIP_STAGING_BUNDLE=1"
else
  COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE}" \
    C_DRYRUN_POLICY=pass_section_freeze_timer_safe \
    C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY}" \
    C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}" \
    C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}" \
    bash scripts/run_c_team_staging_checks.sh "${TEAM_STATUS_PATH}"
fi

echo "[5/6] C-team elapsed/session evidence gate"
python scripts/audit_team_sessions.py --team-status "${TEAM_STATUS_PATH}" --min-elapsed "${MIN_ELAPSED}" --teams C

echo "PASS: C-team submission readiness"
