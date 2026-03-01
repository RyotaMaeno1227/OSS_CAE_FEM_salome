#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/c_team_review_reason_utils.sh
source "${SCRIPT_DIR}/c_team_review_reason_utils.sh"

TEAM_STATUS_PATH="${1:-docs/team_status.md}"
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE:-scripts/c_coupled_freeze_forbidden_paths.txt}"
C_DRYRUN_POLICY="${C_DRYRUN_POLICY:-pass_section_freeze_timer_safe}"
C_STAGE_DRYRUN_LOG="${C_STAGE_DRYRUN_LOG:-}"
C_TEAM_STATUS_BLOCK_OUT="${C_TEAM_STATUS_BLOCK_OUT:-}"
C_APPLY_BLOCK_TO_TEAM_STATUS="${C_APPLY_BLOCK_TO_TEAM_STATUS:-0}"
if [[ -v C_COLLECT_PREFLIGHT_LOG ]]; then
  # Allow explicit empty value to disable preflight resolution.
  C_COLLECT_PREFLIGHT_LOG="${C_COLLECT_PREFLIGHT_LOG}"
else
  # Default to latest log extraction to reduce manual parameters.
  C_COLLECT_PREFLIGHT_LOG="latest"
fi
C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED:-1}"
C_COLLECT_EXPECT_TEAM_STATUS="${C_COLLECT_EXPECT_TEAM_STATUS:-${TEAM_STATUS_PATH}}"
C_COLLECT_LATEST_REQUIRE_FOUND="${C_COLLECT_LATEST_REQUIRE_FOUND:-0}"
C_SKIP_NESTED_SELFTESTS="${C_SKIP_NESTED_SELFTESTS:-0}"
C_REQUIRE_REVIEW_COMMANDS="${C_REQUIRE_REVIEW_COMMANDS:-0}"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY" "1")"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY" "0")"
C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV" "0")"
C_NESTED_SELFTEST_LOCK="${C_NESTED_SELFTEST_LOCK:-/tmp/c_team_nested_selftests.lock}"
collect_preflight_log_for_step16="${C_COLLECT_PREFLIGHT_LOG}"
collect_preflight_require_enabled_for_step16="${C_REQUIRE_COLLECT_PREFLIGHT_ENABLED}"
collect_preflight_expect_team_status_for_step16="${C_COLLECT_EXPECT_TEAM_STATUS}"
collect_preflight_latest_require_found_for_step16="${C_COLLECT_LATEST_REQUIRE_FOUND}"

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

build_staging_retry_command() {
  local command=""
  command+="C_COLLECT_PREFLIGHT_LOG=$(printf '%q' "${collect_preflight_log_for_step16}") "
  command+="C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=$(printf '%q' "${collect_preflight_require_enabled_for_step16}") "
  command+="C_COLLECT_EXPECT_TEAM_STATUS=$(printf '%q' "${collect_preflight_expect_team_status_for_step16}") "
  command+="C_COLLECT_LATEST_REQUIRE_FOUND=$(printf '%q' "${collect_preflight_latest_require_found_for_step16}") "
  command+="C_REQUIRE_REVIEW_COMMANDS=$(printf '%q' "${C_REQUIRE_REVIEW_COMMANDS}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY}") "
  command+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=$(printf '%q' "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV}") "
  command+="C_SKIP_NESTED_SELFTESTS=$(printf '%q' "${C_SKIP_NESTED_SELFTESTS}") "
  command+="C_DRYRUN_POLICY=$(printf '%q' "${C_DRYRUN_POLICY}") "
  command+="bash scripts/run_c_team_staging_checks.sh $(printf '%q' "${TEAM_STATUS_PATH}")"
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
  echo "submission_readiness_retry_command=$(build_staging_retry_command)" >&2
  echo "submission_readiness_fail_step=review_command" >&2
}

cleanup_paths=()
cleanup() {
  for path in "${cleanup_paths[@]}"; do
    rm -f "${path}"
  done
}
trap cleanup EXIT

if [[ -z "${C_STAGE_DRYRUN_LOG}" ]]; then
  C_STAGE_DRYRUN_LOG="$(mktemp /tmp/c_stage_dryrun_auto.XXXXXX.log)"
  cleanup_paths+=("${C_STAGE_DRYRUN_LOG}")
fi

if [[ -z "${C_TEAM_STATUS_BLOCK_OUT}" ]]; then
  C_TEAM_STATUS_BLOCK_OUT="$(mktemp /tmp/c_stage_team_status_block.XXXXXX.md)"
  cleanup_paths+=("${C_TEAM_STATUS_BLOCK_OUT}")
fi

if [[ "${TEAM_STATUS_PATH}" == "--help" || "${TEAM_STATUS_PATH}" == "-h" ]]; then
cat <<'EOF'
Usage: scripts/run_c_team_staging_checks.sh [team_status_path]

Run C-team staging compliance checks:
  0) coupled freeze file precheck
  1) dry-run compliance (pass)
  2) dry-run compliance (C_DRYRUN_POLICY, default: pass_section_freeze_timer_safe)
  3) optional review-command audit
  4) optional fail-trace retry consistency audit
  5) c_stage_dryrun output contract check
  6) render team_status dry-run markdown snippet
  7) optionally apply rendered block to team_status
  8) Python regression tests for audit/check wrappers

env:
  C_DRYRUN_POLICY=pass_section_freeze|pass_section_freeze_timer|pass_section_freeze_timer_safe
  C_STAGE_DRYRUN_LOG=/tmp/c_stage_dryrun_auto.log
  C_TEAM_STATUS_BLOCK_OUT=/tmp/c_stage_team_status_block.md
  C_APPLY_BLOCK_TO_TEAM_STATUS=0|1
  C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log (default: latest)
  C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=0|1
  C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md
  C_COLLECT_LATEST_REQUIRE_FOUND=0|1
  C_SKIP_NESTED_SELFTESTS=0|1
  C_REQUIRE_REVIEW_COMMANDS=0|1
  C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=0|1
  C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=0|1
  C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=0|1
  C_NESTED_SELFTEST_LOCK=/tmp/c_team_nested_selftests.lock
EOF
  exit 0
fi

echo "[0/22] coupled freeze file precheck"
python scripts/check_c_coupled_freeze_file.py "${COUPLED_FREEZE_FILE}"

echo "[1/22] dry-run compliance (pass)"
bash scripts/check_c_team_dryrun_compliance.sh "${TEAM_STATUS_PATH}" pass

echo "[2/22] dry-run compliance (${C_DRYRUN_POLICY})"
bash scripts/check_c_team_dryrun_compliance.sh "${TEAM_STATUS_PATH}" "${C_DRYRUN_POLICY}"

echo "[3/22] optional review-command audit"
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

echo "[4/22] optional fail-trace retry consistency audit"
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

echo "[5/22] c_stage_dryrun output contract check"
scripts/c_stage_dryrun.sh --log "${C_STAGE_DRYRUN_LOG}"
python scripts/check_c_stage_dryrun_report.py "${C_STAGE_DRYRUN_LOG}" --policy pass

echo "[6/22] render_c_stage_team_status_block.py"
python scripts/render_c_stage_team_status_block.py "${C_STAGE_DRYRUN_LOG}" \
  --output "${C_TEAM_STATUS_BLOCK_OUT}" >/dev/null
echo "team_status_block_output=${C_TEAM_STATUS_BLOCK_OUT}"

echo "[7/22] optional apply_c_stage_block_to_team_status.py"
if [[ "${C_APPLY_BLOCK_TO_TEAM_STATUS}" == "1" ]]; then
  python scripts/apply_c_stage_block_to_team_status.py \
    --team-status "${TEAM_STATUS_PATH}" \
    --block-file "${C_TEAM_STATUS_BLOCK_OUT}" \
    --in-place
  echo "team_status_block_apply=updated"
else
  echo "team_status_block_apply=skipped"
fi

# Avoid leaking optional preflight env settings into nested script tests.
unset C_COLLECT_PREFLIGHT_LOG
unset C_REQUIRE_COLLECT_PREFLIGHT_ENABLED
unset C_COLLECT_EXPECT_TEAM_STATUS
unset C_COLLECT_LATEST_REQUIRE_FOUND
unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY
unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY
unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV

run_nested_selftests() {
if [[ "${C_SKIP_NESTED_SELFTESTS}" == "1" ]]; then
  for step in 8 9 10 11 12 13 14 15 16 17 18 19 20 21; do
    echo "[${step}/22] nested self-test skipped (C_SKIP_NESTED_SELFTESTS=1)"
  done
else
  echo "[8/22] test_audit_c_team_staging.py"
  python scripts/test_audit_c_team_staging.py

  echo "[9/22] test_check_c_team_dryrun_compliance.py"
  python scripts/test_check_c_team_dryrun_compliance.py

  echo "[10/22] test_check_c_coupled_freeze_file.py"
  python scripts/test_check_c_coupled_freeze_file.py

  echo "[11/22] test_c_stage_dryrun.py"
  python scripts/test_c_stage_dryrun.py

  echo "[12/22] test_run_team_audit.py"
  python scripts/test_run_team_audit.py

  echo "[13/22] test_render_c_team_session_entry.py"
  python scripts/test_render_c_team_session_entry.py

  echo "[14/22] test_collect_c_team_session_evidence.py"
  python scripts/test_collect_c_team_session_evidence.py

  echo "[15/22] test_append_c_team_entry.py"
  python scripts/test_append_c_team_entry.py

  echo "[16/22] test_mark_c_team_entry_token_missing.py"
  python scripts/test_mark_c_team_entry_token_missing.py

  echo "[17/22] test_recover_c_team_token_missing_session.py"
  python scripts/test_recover_c_team_token_missing_session.py

  echo "[18/22] test_run_c_team_collect_preflight_check.py"
  python scripts/test_run_c_team_collect_preflight_check.py

  echo "[19/22] test_extract_c_team_latest_collect_log.py"
  python scripts/test_extract_c_team_latest_collect_log.py

  echo "[20/22] test_check_c_team_review_commands.py"
  python scripts/test_check_c_team_review_commands.py

  echo "[21/22] test_c_team_review_reason_utils.py"
  python scripts/test_c_team_review_reason_utils.py
fi
}

if command -v flock >/dev/null 2>&1; then
  exec {nested_lock_fd}> "${C_NESTED_SELFTEST_LOCK}"
  flock "${nested_lock_fd}"
  run_nested_selftests
  flock -u "${nested_lock_fd}"
else
  run_nested_selftests
fi

echo "[22/22] optional collect preflight report check"
collect_preflight_output=""
collect_preflight_rc=0
set +e
collect_preflight_output="$(
  C_COLLECT_PREFLIGHT_LOG="${collect_preflight_log_for_step16}" \
    C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="${collect_preflight_require_enabled_for_step16}" \
    C_COLLECT_EXPECT_TEAM_STATUS="${collect_preflight_expect_team_status_for_step16}" \
    C_COLLECT_LATEST_REQUIRE_FOUND="${collect_preflight_latest_require_found_for_step16}" \
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
  echo "submission_readiness_retry_command=$(build_staging_retry_command)" >&2
  echo "submission_readiness_fail_step=collect_preflight" >&2
  exit "${collect_preflight_rc}"
fi
if [[ -n "${collect_preflight_check}" ]]; then
  echo "submission_readiness_collect_preflight_check=${collect_preflight_check}"
fi
if [[ -n "${collect_preflight_reason}" ]]; then
  echo "submission_readiness_collect_preflight_reason=${collect_preflight_reason}"
fi

echo "PASS: C-team staging checks"
