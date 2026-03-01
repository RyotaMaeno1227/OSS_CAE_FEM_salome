#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/c_team_review_reason_utils.sh
source "${SCRIPT_DIR}/c_team_review_reason_utils.sh"

usage() {
  cat <<'EOF'
Usage:
  scripts/collect_c_team_session_evidence.sh \
    --task-title "<title>" \
    --session-token <token_path> \
    [--guard-minutes 30] \
    [--guard-checkpoint-minutes <minutes>] \
    [--dryrun-log /tmp/c_stage_dryrun_auto.log] \
    [--dryrun-block-out /tmp/c_stage_team_status_block.md] \
    [--timer-guard-out /tmp/c_team_timer_guard.txt] \
    [--timer-end-out /tmp/c_team_timer_end.txt] \
    [--entry-out /tmp/c_team_session_entry.md] \
    [--team-status docs/team_status.md] \
    [--append-to-team-status] \
    [--check-compliance-policy pass_section_freeze_timer_safe] \
    [--check-submission-readiness-minutes 30] \
    [--collect-preflight-log /tmp/c_team_collect.log] \
    [--fail-trace-audit-log /tmp/c_team_fail_trace_audit.log] \
    [--collect-latest-require-found 0|1] \
    [--change-line "<path-or-summary>"] \
    [--command-line "<command -> PASS>"]

Collect C-team reporting artifacts in one command:
1) c_stage_dryrun
2) dryrun report contract check
3) render dry-run block markdown
4) session_timer_guard
5) session_timer end
6) render_c_team_session_entry
EOF
}

task_title=""
session_token=""
guard_minutes=30
declare -a guard_checkpoint_minutes=()
dryrun_log=""
dryrun_block_out=""
timer_guard_out="/tmp/c_team_timer_guard.txt"
timer_end_out="/tmp/c_team_timer_end.txt"
entry_out="/tmp/c_team_session_entry.md"
team_status="docs/team_status.md"
append_to_team_status=0
check_compliance_policy=""
check_submission_readiness_minutes=""
collect_preflight_log=""
fail_trace_audit_log=""
collect_latest_require_found="0"
collect_fail_trace_require_retry_consistency="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY" "")"
collect_fail_trace_require_retry_consistency_key="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY" "")"
collect_fail_trace_require_retry_consistency_strict_env="$(c_team_resolve_binary_toggle "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV" "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV" "")"
declare -a done_lines=()
declare -a in_progress_lines=()
declare -a command_lines=()
declare -a change_lines=()
declare -a collect_preflight_reason_lines=()
declare -a collect_preflight_context_lines=()
pass_fail_line=""
validation_team_status=""
preflight_enabled=0
preflight_result="skipped"
submission_readiness_retry_command=""
missing_log_review_pattern="$(c_team_collect_missing_log_review_pattern)"

cleanup_tmp_files=()
cleanup() {
  for tmp in "${cleanup_tmp_files[@]}"; do
    rm -f "${tmp}"
  done
}
trap cleanup EXIT

append_reason_line_if_missing() {
  local candidate="$1"
  [[ -z "${candidate}" ]] && return 0
  local existing
  for existing in "${collect_preflight_reason_lines[@]}"; do
    if [[ "${existing}" == "${candidate}" ]]; then
      return 0
    fi
  done
  collect_preflight_reason_lines+=("${candidate}")
}

append_context_line_if_missing() {
  local candidate="$1"
  [[ -z "${candidate}" ]] && return 0
  local existing
  for existing in "${collect_preflight_context_lines[@]}"; do
    if [[ "${existing}" == "${candidate}" ]]; then
      return 0
    fi
  done
  collect_preflight_context_lines+=("${candidate}")
}

append_command_line_if_missing() {
  local candidate="$1"
  [[ -z "${candidate}" ]] && return 0
  local existing
  for existing in "${command_lines[@]}"; do
    if [[ "${existing}" == "${candidate}" ]]; then
      return 0
    fi
  done
  command_lines+=("${candidate}")
}

build_readiness_command_prefix() {
  local prefix=""
  if [[ "${collect_latest_require_found}" == "1" ]]; then
    prefix+="C_COLLECT_LATEST_REQUIRE_FOUND=1 "
  fi
  if [[ "${C_REQUIRE_REVIEW_COMMANDS:-0}" == "1" ]]; then
    prefix+="C_REQUIRE_REVIEW_COMMANDS=1 "
  fi
  if [[ "${collect_fail_trace_require_retry_consistency}" == "0" || "${collect_fail_trace_require_retry_consistency}" == "1" ]]; then
    prefix+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=${collect_fail_trace_require_retry_consistency} "
  fi
  if [[ "${collect_fail_trace_require_retry_consistency_key}" == "0" || "${collect_fail_trace_require_retry_consistency_key}" == "1" ]]; then
    prefix+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=${collect_fail_trace_require_retry_consistency_key} "
  fi
  if [[ "${collect_fail_trace_require_retry_consistency_strict_env}" == "0" || "${collect_fail_trace_require_retry_consistency_strict_env}" == "1" ]]; then
    prefix+="C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=${collect_fail_trace_require_retry_consistency_strict_env} "
  fi
  printf '%s' "${prefix}"
}

build_retry_consistency_command() {
  local cmd="python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ${team_status}"
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY:-0}" == "1" ]]; then
    cmd+=" --require-retry-consistency-check-key"
  fi
  if [[ "${C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV:-0}" == "1" ]]; then
    cmd+=" --require-strict-env-prefix-match"
  fi
  printf '%s' "${cmd}"
}

append_fail_trace_review_commands_from_log() {
  local log_path="$1"
  local audit_result=""
  local readiness_default_log=""
  local readiness_strict_log=""
  local staging_default_log=""
  local staging_strict_log=""
  local retry_consistency_check=""
  local retry_consistency_reasons=""
  local retry_consistency_reason_codes=""
  local retry_consistency_required=""
  local retry_consistency_require_key=""
  local retry_consistency_require_strict_env=""
  local retry_consistency_command=""
  local audit_retry_prefix=""
  local retry_minutes=""
  local finalize_retry_cmd=""
  local -a missing_keys=()
  audit_result="$(awk -F= '/^FAIL_TRACE_AUDIT_RESULT=/{value=$2} END{print value}' "${log_path}")"
  readiness_default_log="$(awk -F= '/^readiness_default_log=/{value=$2} END{print value}' "${log_path}")"
  readiness_strict_log="$(awk -F= '/^readiness_strict_log=/{value=$2} END{print value}' "${log_path}")"
  staging_default_log="$(awk -F= '/^staging_default_log=/{value=$2} END{print value}' "${log_path}")"
  staging_strict_log="$(awk -F= '/^staging_strict_log=/{value=$2} END{print value}' "${log_path}")"
  retry_consistency_check="$(awk -F= '/^fail_trace_retry_consistency_check=/{value=$2} END{print value}' "${log_path}")"
  retry_consistency_required="$(awk -F= '/^fail_trace_require_retry_consistency=/{value=$2} END{print value}' "${log_path}")"
  retry_consistency_require_key="$(awk -F= '/^fail_trace_require_retry_consistency_key=/{value=$2} END{print value}' "${log_path}")"
  retry_consistency_require_strict_env="$(awk -F= '/^fail_trace_require_retry_consistency_strict_env=/{value=$2} END{print value}' "${log_path}")"
  retry_consistency_reasons="$(
    awk '
      /^C_TEAM_FAIL_TRACE_RETRY_CONSISTENCY$/ {in_block=1; next}
      in_block && /^reasons=/{value=substr($0, index($0, "=") + 1)}
      in_block && /^FAIL_TRACE_AUDIT_RESULT=/{in_block=0}
      END {if (value != "") print value}
    ' "${log_path}"
  )"
  retry_consistency_reason_codes="$(
    awk '
      /^C_TEAM_FAIL_TRACE_RETRY_CONSISTENCY$/ {in_block=1; next}
      in_block && /^reason_codes=/{value=substr($0, index($0, "=") + 1)}
      in_block && /^FAIL_TRACE_AUDIT_RESULT=/{in_block=0}
      END {if (value != "") print value}
    ' "${log_path}"
  )"
  if [[ -z "${retry_consistency_reasons}" ]]; then
    retry_consistency_reasons="$(
      awk '/^fail_trace_retry_consistency_reasons=/{value=substr($0, index($0, "=") + 1)} END{print value}' "${log_path}"
    )"
  fi
  if [[ -z "${retry_consistency_reason_codes}" ]]; then
    retry_consistency_reason_codes="$(
      awk '/^fail_trace_retry_consistency_reason_codes=/{value=substr($0, index($0, "=") + 1)} END{print value}' "${log_path}"
    )"
  fi
  retry_minutes="${check_submission_readiness_minutes:-30}"

  append_command_line_if_missing "fail_trace_audit_log=${log_path}"
  if [[ -z "${audit_result}" ]]; then
    missing_keys+=("FAIL_TRACE_AUDIT_RESULT")
    append_command_line_if_missing "fail_trace_audit_result=missing"
  else
    append_command_line_if_missing "fail_trace_audit_result=${audit_result}"
  fi
  if [[ -n "${readiness_default_log}" ]]; then
    append_command_line_if_missing "fail_trace_readiness_default_log=${readiness_default_log}"
    append_command_line_if_missing "fail_trace_readiness_default_review_command=python scripts/check_c_team_fail_trace_order.py ${readiness_default_log} --mode default"
  else
    missing_keys+=("readiness_default_log")
  fi
  if [[ -n "${readiness_strict_log}" ]]; then
    append_command_line_if_missing "fail_trace_readiness_strict_log=${readiness_strict_log}"
    append_command_line_if_missing "fail_trace_readiness_strict_review_command=python scripts/check_c_team_fail_trace_order.py ${readiness_strict_log} --mode strict"
  else
    missing_keys+=("readiness_strict_log")
  fi
  if [[ -n "${staging_default_log}" ]]; then
    append_command_line_if_missing "fail_trace_staging_default_log=${staging_default_log}"
    append_command_line_if_missing "fail_trace_staging_default_review_command=python scripts/check_c_team_fail_trace_order.py ${staging_default_log} --mode default"
  else
    missing_keys+=("staging_default_log")
  fi
  if [[ -n "${staging_strict_log}" ]]; then
    append_command_line_if_missing "fail_trace_staging_strict_log=${staging_strict_log}"
    append_command_line_if_missing "fail_trace_staging_strict_review_command=python scripts/check_c_team_fail_trace_order.py ${staging_strict_log} --mode strict"
  else
    missing_keys+=("staging_strict_log")
  fi
  retry_consistency_command="python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ${team_status}"
  if [[ "${retry_consistency_require_key}" == "1" ]]; then
    retry_consistency_command+=" --require-retry-consistency-check-key"
  fi
  if [[ "${retry_consistency_require_strict_env}" == "1" ]]; then
    retry_consistency_command+=" --require-strict-env-prefix-match"
  fi
  append_command_line_if_missing "fail_trace_retry_consistency_command=${retry_consistency_command}"
  if [[ -n "${retry_consistency_required}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_required=${retry_consistency_required}"
  fi
  if [[ -n "${retry_consistency_require_key}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_require_key=${retry_consistency_require_key}"
  fi
  if [[ -n "${retry_consistency_require_strict_env}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_require_strict_env=${retry_consistency_require_strict_env}"
  fi
  if [[ -n "${retry_consistency_reasons}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_reasons=${retry_consistency_reasons}"
  fi
  if [[ -n "${retry_consistency_reason_codes}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_reason_codes=${retry_consistency_reason_codes}"
  fi
  if [[ -n "${retry_consistency_check}" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_check=${retry_consistency_check}"
  else
    append_command_line_if_missing "fail_trace_retry_consistency_check=unknown"
  fi
  if [[ "${retry_consistency_required}" == "1" || "${retry_consistency_required}" == "0" ]]; then
    audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=${retry_consistency_required} "
  fi
  if [[ "${retry_consistency_require_key}" == "1" || "${retry_consistency_require_key}" == "0" ]]; then
    audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=${retry_consistency_require_key} "
  fi
  if [[ "${retry_consistency_require_strict_env}" == "1" || "${retry_consistency_require_strict_env}" == "0" ]]; then
    audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=${retry_consistency_require_strict_env} "
  fi
  append_command_line_if_missing "fail_trace_audit_retry_command=${audit_retry_prefix}scripts/run_c_team_fail_trace_audit.sh ${team_status} ${retry_minutes} | tee ${log_path}"
  if [[ -z "${retry_consistency_check}" || "${retry_consistency_check}" != "pass" ]]; then
    append_command_line_if_missing "fail_trace_retry_consistency_retry_command=${retry_consistency_command}"
  fi
  if [[ "${audit_result}" != "PASS" ]]; then
    append_command_line_if_missing "fail_trace_audit_retry_reason=audit_result_${audit_result:-missing}"
  fi
  if [[ "${#missing_keys[@]}" -gt 0 ]]; then
    append_command_line_if_missing "fail_trace_audit_missing_keys=${missing_keys[*]}"
  fi
  if [[ "${audit_result}" != "PASS" || "${#missing_keys[@]}" -gt 0 ]]; then
    finalize_retry_cmd="${audit_retry_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${session_token} --task-title \"${task_title}\" --guard-minutes ${guard_minutes}"
    if [[ -n "${check_compliance_policy}" ]]; then
      finalize_retry_cmd+=" --check-compliance-policy ${check_compliance_policy}"
    fi
    if [[ -n "${check_submission_readiness_minutes}" ]]; then
      finalize_retry_cmd+=" --check-submission-readiness-minutes ${check_submission_readiness_minutes}"
    fi
    if [[ "${collect_latest_require_found}" == "1" ]]; then
      finalize_retry_cmd+=" --collect-latest-require-found 1"
    fi
    finalize_retry_cmd+=" --fail-trace-audit-log ${log_path}"
    append_command_line_if_missing "fail_trace_finalize_retry_command=${finalize_retry_cmd}"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --task-title)
      task_title="${2:-}"
      shift 2
      ;;
    --session-token)
      session_token="${2:-}"
      shift 2
      ;;
    --guard-minutes)
      guard_minutes="${2:-}"
      shift 2
      ;;
    --guard-checkpoint-minutes)
      guard_checkpoint_minutes+=("${2:-}")
      shift 2
      ;;
    --dryrun-log)
      dryrun_log="${2:-}"
      shift 2
      ;;
    --dryrun-block-out)
      dryrun_block_out="${2:-}"
      shift 2
      ;;
    --timer-guard-out)
      timer_guard_out="${2:-}"
      shift 2
      ;;
    --timer-end-out)
      timer_end_out="${2:-}"
      shift 2
      ;;
    --entry-out)
      entry_out="${2:-}"
      shift 2
      ;;
    --team-status)
      team_status="${2:-}"
      shift 2
      ;;
    --append-to-team-status)
      append_to_team_status=1
      shift 1
      ;;
    --check-compliance-policy)
      check_compliance_policy="${2:-}"
      shift 2
      ;;
    --check-submission-readiness-minutes)
      check_submission_readiness_minutes="${2:-}"
      shift 2
      ;;
    --collect-preflight-log)
      collect_preflight_log="${2:-}"
      shift 2
      ;;
    --fail-trace-audit-log)
      fail_trace_audit_log="${2:-}"
      shift 2
      ;;
    --collect-latest-require-found)
      collect_latest_require_found="${2:-}"
      shift 2
      ;;
    --done-line)
      done_lines+=("${2:-}")
      shift 2
      ;;
    --in-progress-line)
      in_progress_lines+=("${2:-}")
      shift 2
      ;;
    --command-line)
      command_lines+=("${2:-}")
      shift 2
      ;;
    --change-line)
      change_lines+=("${2:-}")
      shift 2
      ;;
    --pass-fail-line)
      pass_fail_line="${2:-}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ -z "${task_title}" ]]; then
  echo "ERROR: --task-title is required" >&2
  exit 2
fi
if [[ -z "${session_token}" ]]; then
  echo "ERROR: --session-token is required" >&2
  exit 2
fi
if [[ ! -f "${session_token}" ]]; then
  echo "ERROR: token file not found: ${session_token}" >&2
  exit 2
fi
if ! [[ "${guard_minutes}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: --guard-minutes must be an integer: ${guard_minutes}" >&2
  exit 2
fi
for checkpoint in "${guard_checkpoint_minutes[@]}"; do
  if ! [[ "${checkpoint}" =~ ^[0-9]+$ ]]; then
    echo "ERROR: --guard-checkpoint-minutes must be an integer: ${checkpoint}" >&2
    exit 2
  fi
done
if [[ -n "${check_submission_readiness_minutes}" ]] && ! [[ "${check_submission_readiness_minutes}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: --check-submission-readiness-minutes must be an integer: ${check_submission_readiness_minutes}" >&2
  exit 2
fi
if [[ "${collect_latest_require_found}" != "0" && "${collect_latest_require_found}" != "1" ]]; then
  echo "ERROR: --collect-latest-require-found must be 0 or 1: ${collect_latest_require_found}" >&2
  exit 2
fi
if [[ -n "${fail_trace_audit_log}" && ! -f "${fail_trace_audit_log}" ]]; then
  echo "ERROR: --fail-trace-audit-log not found: ${fail_trace_audit_log}" >&2
  exit 2
fi
if [[ -n "${check_submission_readiness_minutes}" ]] && [[ "${append_to_team_status}" != "1" ]]; then
  echo "ERROR: --check-submission-readiness-minutes requires --append-to-team-status" >&2
  exit 2
fi
if [[ -n "${check_compliance_policy}" || -n "${check_submission_readiness_minutes}" ]]; then
  preflight_enabled=1
fi
if [[ -z "${dryrun_log}" ]]; then
  dryrun_log="$(mktemp /tmp/c_stage_dryrun_auto.XXXXXX.log)"
  cleanup_tmp_files+=("${dryrun_log}")
fi
if [[ -z "${dryrun_block_out}" ]]; then
  dryrun_block_out="$(mktemp /tmp/c_stage_team_status_block.XXXXXX.md)"
  cleanup_tmp_files+=("${dryrun_block_out}")
fi
needs_template_fill=0
if [[ -n "${check_submission_readiness_minutes}" || "${check_compliance_policy}" == "pass_section_freeze_timer_safe" ]]; then
  needs_template_fill=1
fi
if [[ "${needs_template_fill}" == "1" ]]; then
  if [[ "${#done_lines[@]}" -eq 0 ]]; then
    if [[ -n "${check_submission_readiness_minutes}" ]]; then
      done_lines+=("提出前ゲート実行（strict-safe + elapsed監査）")
    else
      done_lines+=("strict-safe 監査を実行")
    fi
  fi
  if [[ "${#in_progress_lines[@]}" -eq 0 ]]; then
    in_progress_lines+=("次タスクを In Progress で継続")
  fi
  if [[ -z "${pass_fail_line}" ]]; then
    if [[ -n "${check_submission_readiness_minutes}" ]]; then
      pass_fail_line="PASS（strict-safe + submission readiness）"
    else
      pass_fail_line="PASS（strict-safe compliance）"
    fi
  fi
fi
if [[ -n "${check_compliance_policy}" ]]; then
  append_command_line_if_missing "bash scripts/check_c_team_dryrun_compliance.sh ${team_status} ${check_compliance_policy} -> PASS（preflight）"
fi
if [[ "${#guard_checkpoint_minutes[@]}" -gt 0 ]]; then
  checkpoint_csv="$(IFS=,; printf '%s' "${guard_checkpoint_minutes[*]}")"
  append_command_line_if_missing "guard_checkpoints=${checkpoint_csv}"
fi
if [[ -n "${check_submission_readiness_minutes}" ]]; then
  readiness_command_prefix="$(build_readiness_command_prefix)"
  retry_consistency_command="$(build_retry_consistency_command)"
  append_command_line_if_missing "${readiness_command_prefix}bash scripts/check_c_team_submission_readiness.sh ${team_status} ${check_submission_readiness_minutes} -> RUN（preflight gate）"
  append_command_line_if_missing "submission_readiness_retry_command=${readiness_command_prefix}bash scripts/check_c_team_submission_readiness.sh ${team_status} ${check_submission_readiness_minutes}"
  append_command_line_if_missing "fail_trace_audit_command=scripts/run_c_team_fail_trace_audit.sh ${team_status} ${check_submission_readiness_minutes}"
  append_command_line_if_missing "fail_trace_retry_consistency_command=${retry_consistency_command}"
  append_command_line_if_missing "fail_trace_retry_consistency_check=unknown"
  append_command_line_if_missing "fail_trace_retry_consistency_retry_command=${retry_consistency_command}"
fi
if [[ -n "${fail_trace_audit_log}" ]]; then
  append_fail_trace_review_commands_from_log "${fail_trace_audit_log}"
fi
if [[ "${preflight_enabled}" == "1" ]]; then
  append_command_line_if_missing "review_command_audit_command=python scripts/check_c_team_review_commands.py --team-status ${team_status}"
  append_command_line_if_missing "review_command_required=${C_REQUIRE_REVIEW_COMMANDS:-0}"
  append_command_line_if_missing "review_command_fail_reason_codes_source=-"
  append_command_line_if_missing "missing_log_review_command=rg -n '${missing_log_review_pattern}' ${entry_out}"
  if [[ -n "${collect_preflight_log}" ]]; then
    append_command_line_if_missing "collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py ${collect_preflight_log} --require-enabled --expect-team-status ${team_status}"
  fi
fi

echo "[1/9] c_stage_dryrun"
scripts/c_stage_dryrun.sh --log "${dryrun_log}" >/dev/null

echo "[2/9] check_c_stage_dryrun_report"
python scripts/check_c_stage_dryrun_report.py "${dryrun_log}" --policy pass >/dev/null

echo "[3/9] render_c_stage_team_status_block"
python scripts/render_c_stage_team_status_block.py "${dryrun_log}" --output "${dryrun_block_out}" >/dev/null

echo "[4/9] session_timer_guard"
: > "${timer_guard_out}"
for checkpoint in "${guard_checkpoint_minutes[@]}"; do
  echo "[4/9] session_timer_guard checkpoint(${checkpoint})"
  set +e
  checkpoint_output="$(bash scripts/session_timer_guard.sh "${session_token}" "${checkpoint}" 2>&1)"
  checkpoint_rc=$?
  set -e
  printf '%s\n' "${checkpoint_output}" | tee -a "${timer_guard_out}" >/dev/null
  if [[ "${checkpoint_rc}" -ne 0 && "${checkpoint_rc}" -ne 1 ]]; then
    echo "${checkpoint_output}" >&2
    exit "${checkpoint_rc}"
  fi
done
bash scripts/session_timer_guard.sh "${session_token}" "${guard_minutes}" | tee -a "${timer_guard_out}" >/dev/null

echo "[5/9] session_timer_end"
scripts/session_timer.sh end "${session_token}" | tee "${timer_end_out}" >/dev/null

if [[ "${preflight_enabled}" == "1" ]]; then
  probe_collect_preflight_log="${collect_preflight_log}"
  if [[ -z "${probe_collect_preflight_log}" ]]; then
    probe_collect_preflight_log="latest"
  fi
  probe_output="$(
    C_COLLECT_PREFLIGHT_LOG="${probe_collect_preflight_log}" \
      C_REQUIRE_COLLECT_PREFLIGHT_ENABLED="1" \
      C_COLLECT_EXPECT_TEAM_STATUS="${team_status}" \
      C_COLLECT_LATEST_REQUIRE_FOUND="${collect_latest_require_found}" \
      bash scripts/run_c_team_collect_preflight_check.sh "${team_status}" 2>&1 || true
  )"
  while IFS= read -r probe_line; do
    case "${probe_line}" in
      collect_preflight_log_resolved=*|collect_preflight_log_missing=*)
        append_context_line_if_missing "${probe_line}"
        ;;
      collect_preflight_check_reason=*)
        append_reason_line_if_missing "${probe_line}"
        ;;
    esac
  done <<< "${probe_output}"
fi
for context_line in "${collect_preflight_context_lines[@]}"; do
  command_lines+=("${context_line}")
done
for reason_line in "${collect_preflight_reason_lines[@]}"; do
  command_lines+=("${reason_line}")
done
if [[ "${preflight_enabled}" == "1" ]]; then
  if [[ "${#collect_preflight_reason_lines[@]}" -gt 0 ]]; then
    command_lines+=("collect_preflight_reasons=${collect_preflight_reason_lines[*]}")
  else
    command_lines+=("collect_preflight_reasons=-")
  fi
fi

echo "[6/9] render_c_team_session_entry"
render_cmd=(
  python scripts/render_c_team_session_entry.py
  --task-title "${task_title}"
  --session-token "${session_token}"
  --timer-end-file "${timer_end_out}"
  --timer-guard-file "${timer_guard_out}"
  --dryrun-block-file "${dryrun_block_out}"
  --c-stage-dryrun-log "${dryrun_log}"
  --collect-latest-require-found "${collect_latest_require_found}"
  --output "${entry_out}"
)
if [[ -n "${collect_preflight_log}" ]]; then
  render_cmd+=(--collect-preflight-log "${collect_preflight_log}")
fi
for line in "${done_lines[@]}"; do
  render_cmd+=(--done-line "${line}")
done
for line in "${in_progress_lines[@]}"; do
  render_cmd+=(--in-progress-line "${line}")
done
for line in "${command_lines[@]}"; do
  render_cmd+=(--command-line "${line}")
done
for line in "${change_lines[@]}"; do
  render_cmd+=(--change-line "${line}")
done
if [[ -n "${pass_fail_line}" ]]; then
  render_cmd+=(--pass-fail-line "${pass_fail_line}")
fi
"${render_cmd[@]}" >/dev/null

echo "[7/9] optional append_c_team_entry"
if [[ "${append_to_team_status}" == "1" ]]; then
  if [[ -n "${check_compliance_policy}" || -n "${check_submission_readiness_minutes}" ]]; then
    validation_team_status="$(mktemp)"
    cleanup_tmp_files+=("${validation_team_status}")
    cp "${team_status}" "${validation_team_status}"
    python scripts/append_c_team_entry.py \
      --team-status "${validation_team_status}" \
      --entry-file "${entry_out}" \
      --in-place >/dev/null
    echo "team_status_append=pending_validation"
  else
    python scripts/append_c_team_entry.py \
      --team-status "${team_status}" \
      --entry-file "${entry_out}" \
      --in-place >/dev/null
    echo "team_status_append=updated"
  fi
else
  echo "team_status_append=skipped"
fi

echo "[8/9] optional strict-safe compliance"
audit_target_team_status="${team_status}"
if [[ -n "${validation_team_status}" ]]; then
  audit_target_team_status="${validation_team_status}"
elif [[ "${append_to_team_status}" != "1" && ( -n "${check_compliance_policy}" || -n "${check_submission_readiness_minutes}" ) ]]; then
  validation_team_status="$(mktemp)"
  cleanup_tmp_files+=("${validation_team_status}")
  cp "${team_status}" "${validation_team_status}"
  python scripts/append_c_team_entry.py \
    --team-status "${validation_team_status}" \
    --entry-file "${entry_out}" \
    --in-place >/dev/null
  audit_target_team_status="${validation_team_status}"
fi
preflight_report_team_status="${audit_target_team_status}"
if [[ "${append_to_team_status}" == "1" && -n "${validation_team_status}" ]]; then
  # Report the canonical destination path once append target is fixed.
  preflight_report_team_status="${team_status}"
fi
if [[ "${preflight_enabled}" == "1" ]]; then
  echo "preflight_mode=enabled"
  echo "preflight_team_status=${preflight_report_team_status}"
  echo "preflight_latest_require_found=${collect_latest_require_found}"
else
  echo "preflight_mode=disabled"
fi

if [[ -n "${check_compliance_policy}" ]]; then
  if ! compliance_output="$(bash scripts/check_c_team_dryrun_compliance.sh "${audit_target_team_status}" "${check_compliance_policy}" 2>&1)"; then
    echo "${compliance_output}" >&2
    exit 1
  fi
  echo "compliance_check=pass"
  echo "compliance_policy=${check_compliance_policy}"
else
  echo "compliance_check=skipped"
fi

echo "[9/9] optional submission readiness"
if [[ -n "${check_submission_readiness_minutes}" ]]; then
  readiness_command_prefix="$(build_readiness_command_prefix)"
  readiness_expect_team_status="${audit_target_team_status}"
  readiness_env=()
  readiness_env+=("C_COLLECT_EXPECT_TEAM_STATUS=${readiness_expect_team_status}")
  readiness_env+=("C_COLLECT_LATEST_REQUIRE_FOUND=${collect_latest_require_found}")
  if [[ "${collect_fail_trace_require_retry_consistency}" == "0" || "${collect_fail_trace_require_retry_consistency}" == "1" ]]; then
    readiness_env+=("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=${collect_fail_trace_require_retry_consistency}")
  fi
  if [[ "${collect_fail_trace_require_retry_consistency_key}" == "0" || "${collect_fail_trace_require_retry_consistency_key}" == "1" ]]; then
    readiness_env+=("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=${collect_fail_trace_require_retry_consistency_key}")
  fi
  if [[ "${collect_fail_trace_require_retry_consistency_strict_env}" == "0" || "${collect_fail_trace_require_retry_consistency_strict_env}" == "1" ]]; then
    readiness_env+=("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=${collect_fail_trace_require_retry_consistency_strict_env}")
  fi
  # When explicit collect log is provided, keep expected team_status anchored to the
  # canonical destination to avoid validation-temp path mismatch.
  if [[ -n "${collect_preflight_log}" ]]; then
    readiness_expect_team_status="${team_status}"
    readiness_env[0]="C_COLLECT_EXPECT_TEAM_STATUS=${readiness_expect_team_status}"
  fi
  if [[ -n "${collect_preflight_log}" ]]; then
    readiness_env+=("C_COLLECT_PREFLIGHT_LOG=${collect_preflight_log}")
    if ! readiness_output="$(
      env "${readiness_env[@]}" \
        bash scripts/check_c_team_submission_readiness.sh "${audit_target_team_status}" "${check_submission_readiness_minutes}" 2>&1
    )"; then
      submission_readiness_retry_command="${readiness_command_prefix}bash scripts/check_c_team_submission_readiness.sh ${team_status} ${check_submission_readiness_minutes}"
      echo "${readiness_output}" >&2
      if [[ "${#collect_preflight_reason_lines[@]}" -gt 0 ]]; then
        for reason_line in "${collect_preflight_reason_lines[@]}"; do
          echo "${reason_line}" >&2
        done
      fi
      echo "submission_readiness_retry_command=${submission_readiness_retry_command}" >&2
      exit 1
    fi
  elif ! readiness_output="$(
    env "${readiness_env[@]}" \
      bash scripts/check_c_team_submission_readiness.sh "${audit_target_team_status}" "${check_submission_readiness_minutes}" 2>&1
  )"; then
    submission_readiness_retry_command="${readiness_command_prefix}bash scripts/check_c_team_submission_readiness.sh ${team_status} ${check_submission_readiness_minutes}"
    echo "${readiness_output}" >&2
    if [[ "${#collect_preflight_reason_lines[@]}" -gt 0 ]]; then
      for reason_line in "${collect_preflight_reason_lines[@]}"; do
        echo "${reason_line}" >&2
      done
    fi
    echo "submission_readiness_retry_command=${submission_readiness_retry_command}" >&2
    exit 1
  fi
  echo "submission_readiness=pass"
  echo "submission_readiness_min_elapsed=${check_submission_readiness_minutes}"
else
  echo "submission_readiness=skipped"
fi
if [[ "${preflight_enabled}" == "1" ]]; then
  preflight_result="pass"
fi

if [[ "${append_to_team_status}" == "1" && -n "${validation_team_status}" ]]; then
  cp "${validation_team_status}" "${team_status}"
  echo "team_status_append=updated"
fi

echo "collect_result=PASS"
echo "session_token=${session_token}"
echo "dryrun_log=${dryrun_log}"
echo "dryrun_block_out=${dryrun_block_out}"
echo "timer_guard_out=${timer_guard_out}"
echo "timer_end_out=${timer_end_out}"
echo "entry_out=${entry_out}"
echo "preflight_result=${preflight_result}"
if [[ "${#collect_preflight_reason_lines[@]}" -gt 0 ]]; then
  echo "collect_preflight_reasons=${collect_preflight_reason_lines[*]}"
else
  echo "collect_preflight_reasons=-"
fi
