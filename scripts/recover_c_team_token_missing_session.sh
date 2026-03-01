#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/c_team_review_reason_utils.sh
source "${SCRIPT_DIR}/c_team_review_reason_utils.sh"

usage() {
  cat <<'EOF'
Usage:
  scripts/recover_c_team_token_missing_session.sh \
    --team-status docs/team_status.md \
    --target-start-epoch <start_epoch> \
    --token-path <missing_token_path> \
    [--new-team-tag c_team]

  scripts/recover_c_team_token_missing_session.sh \
    --team-status docs/team_status.md \
    --finalize-session-token <session_token> \
    --task-title "<title>" \
    [--guard-minutes 30] \
    [--guard-checkpoint-minutes <minutes>] \
    [--dryrun-log /tmp/c_stage_dryrun_auto.log] \
    [--dryrun-block-out /tmp/c_stage_team_status_block.md] \
    [--timer-guard-out /tmp/c_team_timer_guard.txt] \
    [--timer-end-out /tmp/c_team_timer_end.txt] \
    [--entry-out /tmp/c_team_session_entry.md] \
    [--check-compliance-policy pass_section_freeze_timer_safe] \
    [--check-submission-readiness-minutes 30] \
    [--collect-log-out /tmp/c_team_collect.log] \
    [--fail-trace-audit-log /tmp/c_team_fail_trace_audit.log] \
    [--collect-latest-require-found 0|1] \
    [--change-line "<path-or-summary>"] \
    [--command-line "<command -> PASS>"]

Run token-missing recovery in one command:
1) mark target C entry as token-missing
2) start a new session timer

Or finalize a recovered session in one command:
1) collect dry-run + timer guard/end evidence
2) append rendered C entry to team_status
EOF
}

team_status="docs/team_status.md"
target_start_epoch=""
token_path=""
new_team_tag="c_team"
finalize_session_token=""
task_title=""
guard_minutes=30
declare -a guard_checkpoint_minutes=()
dryrun_log="/tmp/c_stage_dryrun_auto.log"
dryrun_block_out="/tmp/c_stage_team_status_block.md"
timer_guard_out="/tmp/c_team_timer_guard.txt"
timer_end_out="/tmp/c_team_timer_end.txt"
entry_out="/tmp/c_team_session_entry.md"
check_compliance_policy=""
check_submission_readiness_minutes=""
declare -a done_lines=()
declare -a in_progress_lines=()
declare -a command_lines=()
declare -a change_lines=()
pass_fail_line=""
collect_log_out=""
fail_trace_audit_log=""
collect_latest_require_found="0"
missing_log_review_pattern="$(c_team_collect_missing_log_review_pattern)"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --team-status)
      team_status="${2:-}"
      shift 2
      ;;
    --target-start-epoch)
      target_start_epoch="${2:-}"
      shift 2
      ;;
    --token-path)
      token_path="${2:-}"
      shift 2
      ;;
    --new-team-tag)
      new_team_tag="${2:-}"
      shift 2
      ;;
    --finalize-session-token)
      finalize_session_token="${2:-}"
      shift 2
      ;;
    --task-title)
      task_title="${2:-}"
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
    --check-compliance-policy)
      check_compliance_policy="${2:-}"
      shift 2
      ;;
    --check-submission-readiness-minutes)
      check_submission_readiness_minutes="${2:-}"
      shift 2
      ;;
    --collect-log-out)
      collect_log_out="${2:-}"
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

if [[ -n "${finalize_session_token}" ]]; then
  if [[ -z "${task_title}" ]]; then
    echo "ERROR: --task-title is required in finalize mode" >&2
    exit 2
  fi
  if [[ ! -f "${finalize_session_token}" ]]; then
    echo "ERROR: token file not found: ${finalize_session_token}" >&2
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
    retry_minutes="${check_submission_readiness_minutes:-30}"
    audit_retry_prefix=""
    if [[ "${C_REQUIRE_REVIEW_COMMANDS:-}" == "1" ]]; then
      audit_retry_prefix+="C_REQUIRE_REVIEW_COMMANDS=1 "
    fi
    if [[ "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY:-}" == "0" || "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY:-}" == "1" ]]; then
      audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY} "
    fi
    if [[ "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY:-}" == "0" || "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY:-}" == "1" ]]; then
      audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY} "
    fi
    if [[ "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV:-}" == "0" || "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV:-}" == "1" ]]; then
      audit_retry_prefix+="C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV} "
    fi
    retry_consistency_retry_command="python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ${team_status}"
    if [[ "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY:-}" == "1" ]]; then
      retry_consistency_retry_command+=" --require-retry-consistency-check-key"
    fi
    if [[ "${C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV:-}" == "1" ]]; then
      retry_consistency_retry_command+=" --require-strict-env-prefix-match"
    fi
    retry_cmd=(
      bash scripts/recover_c_team_token_missing_session.sh
      --team-status "${team_status}"
      --finalize-session-token "${finalize_session_token}"
      --task-title "${task_title}"
      --guard-minutes "${guard_minutes}"
      --fail-trace-audit-log "${fail_trace_audit_log}"
    )
    for checkpoint in "${guard_checkpoint_minutes[@]}"; do
      retry_cmd+=(--guard-checkpoint-minutes "${checkpoint}")
    done
    if [[ -n "${check_compliance_policy}" ]]; then
      retry_cmd+=(--check-compliance-policy "${check_compliance_policy}")
    fi
    if [[ -n "${check_submission_readiness_minutes}" ]]; then
      retry_cmd+=(--check-submission-readiness-minutes "${check_submission_readiness_minutes}")
    fi
    if [[ -n "${collect_log_out}" ]]; then
      retry_cmd+=(--collect-log-out "${collect_log_out}")
    fi
    if [[ "${collect_latest_require_found}" == "1" ]]; then
      retry_cmd+=(--collect-latest-require-found 1)
    fi
    echo "ERROR: --fail-trace-audit-log not found: ${fail_trace_audit_log}" >&2
    echo "fail_trace_audit_retry_command=${audit_retry_prefix}scripts/run_c_team_fail_trace_audit.sh ${team_status} ${retry_minutes} | tee ${fail_trace_audit_log}" >&2
    echo "fail_trace_retry_consistency_retry_command=${retry_consistency_retry_command}" >&2
    printf 'fail_trace_finalize_retry_command=%s' "${audit_retry_prefix}" >&2
    printf '%q ' "${retry_cmd[@]}" >&2
    printf '\n' >&2
    exit 2
  fi

  echo "[1/1] collect evidence and append recovered C entry"
  collect_cmd=(
    bash scripts/collect_c_team_session_evidence.sh
    --task-title "${task_title}"
    --session-token "${finalize_session_token}"
    --guard-minutes "${guard_minutes}"
    --dryrun-log "${dryrun_log}"
    --dryrun-block-out "${dryrun_block_out}"
    --timer-guard-out "${timer_guard_out}"
    --timer-end-out "${timer_end_out}"
    --entry-out "${entry_out}"
    --team-status "${team_status}"
    --append-to-team-status
  )
  for checkpoint in "${guard_checkpoint_minutes[@]}"; do
    collect_cmd+=(--guard-checkpoint-minutes "${checkpoint}")
  done
  if [[ -n "${check_compliance_policy}" ]]; then
    collect_cmd+=(--check-compliance-policy "${check_compliance_policy}")
  fi
  if [[ -n "${check_submission_readiness_minutes}" ]]; then
    collect_cmd+=(--check-submission-readiness-minutes "${check_submission_readiness_minutes}")
  fi
  if [[ -n "${fail_trace_audit_log}" ]]; then
    collect_cmd+=(--fail-trace-audit-log "${fail_trace_audit_log}")
  fi
  collect_cmd+=(--collect-latest-require-found "${collect_latest_require_found}")
  for line in "${done_lines[@]}"; do
    collect_cmd+=(--done-line "${line}")
  done
  for line in "${in_progress_lines[@]}"; do
    collect_cmd+=(--in-progress-line "${line}")
  done
  for line in "${command_lines[@]}"; do
    collect_cmd+=(--command-line "${line}")
  done
  if [[ -n "${collect_log_out}" ]]; then
    collect_cmd+=(--command-line "python scripts/check_c_team_collect_preflight_report.py ${collect_log_out} --require-enabled -> PASS")
    collect_cmd+=(--command-line "collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py ${collect_log_out} --require-enabled --expect-team-status ${team_status}")
  fi
  for line in "${change_lines[@]}"; do
    collect_cmd+=(--change-line "${line}")
  done
  if [[ -n "${pass_fail_line}" ]]; then
    collect_cmd+=(--pass-fail-line "${pass_fail_line}")
  fi
  if [[ -n "${collect_log_out}" ]]; then
    if ! "${collect_cmd[@]}" 2>&1 | tee "${collect_log_out}"; then
      review_reason_present=0
      review_source_present=0
      for key in \
        review_command_fail_reason \
        review_command_fail_reason_codes \
        review_command_fail_reason_codes_source \
        review_command_retry_command \
        submission_readiness_fail_step \
        submission_readiness_retry_command; do
        value="$(
          awk -F= -v key="${key}" '
            $1 == key {value=substr($0, index($0, "=") + 1)}
            END {if (value != "") print value}
          ' "${collect_log_out}"
        )"
        if [[ -n "${value}" ]]; then
          echo "${key}=${value}" >&2
          if [[ "${key}" == "review_command_fail_reason" ]]; then
            review_reason_present=1
          fi
          if [[ "${key}" == "review_command_fail_reason_codes_source" ]]; then
            review_source_present=1
          fi
        fi
      done
      if [[ "${review_reason_present}" == "1" && "${review_source_present}" == "0" ]]; then
        echo "review_command_fail_reason_codes_source=fallback" >&2
      fi
      echo "collect_log_out=${collect_log_out}" >&2
      echo "entry_out=${entry_out}" >&2
      echo "missing_log_review_command=rg -n '${missing_log_review_pattern}' ${entry_out}" >&2
      echo "collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py ${collect_log_out} --require-enabled --expect-team-status ${team_status}" >&2
      exit 1
    fi
    python scripts/check_c_team_collect_preflight_report.py \
      "${collect_log_out}" \
      --require-enabled \
      --expect-team-status "${team_status}"
    echo "collect_log_out=${collect_log_out}"
  else
    if ! "${collect_cmd[@]}"; then
      echo "entry_out=${entry_out}" >&2
      echo "missing_log_review_command=rg -n '${missing_log_review_pattern}' ${entry_out}" >&2
      exit 1
    fi
  fi
  echo "recovery_finalize_result=PASS"
  exit 0
fi

if [[ -z "${target_start_epoch}" ]]; then
  echo "ERROR: --target-start-epoch is required in start mode" >&2
  exit 2
fi
if [[ -z "${token_path}" ]]; then
  echo "ERROR: --token-path is required in start mode" >&2
  exit 2
fi
if ! [[ "${target_start_epoch}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: --target-start-epoch must be numeric: ${target_start_epoch}" >&2
  exit 2
fi

echo "[1/2] mark token-missing entry"
python scripts/mark_c_team_entry_token_missing.py \
  --team-status "${team_status}" \
  --target-start-epoch "${target_start_epoch}" \
  --token-path "${token_path}" \
  --in-place

echo "[2/2] start new session timer"
start_output="$(scripts/session_timer.sh start "${new_team_tag}")"
echo "${start_output}"
new_session_token="$(awk -F= '/^session_token=/{print $2}' <<<"${start_output}" | tail -n1)"
if [[ -n "${new_session_token}" ]]; then
  finalize_prefix=""
  if [[ "${C_REQUIRE_REVIEW_COMMANDS:-}" == "1" ]]; then
    finalize_prefix+="C_REQUIRE_REVIEW_COMMANDS=1 "
  fi
  session_token_basename="$(basename "${new_session_token}")"
  fail_trace_audit_log_default="/tmp/c_team_fail_trace_audit_${session_token_basename%.token}.log"
  echo "next_finalize_command=${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30"
  echo "next_finalize_command_strict_latest=${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1"
  echo "next_finalize_fail_trace_audit_command=${finalize_prefix}scripts/run_c_team_fail_trace_audit.sh ${team_status} 30"
  echo "next_finalize_fail_trace_audit_log=${fail_trace_audit_log_default}"
  echo "next_finalize_command_with_fail_trace_log=${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_command_strict_latest_with_fail_trace_log=${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_embed_command=${finalize_prefix}scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_embed_command_strict_latest=${finalize_prefix}scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_audit_command_strict_key=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30"
  echo "next_finalize_fail_trace_embed_command_strict_key=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_embed_command_strict_latest_strict_key=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_audit_command_strict_key_strict_env=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30"
  echo "next_finalize_fail_trace_embed_command_strict_key_strict_env=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_fail_trace_embed_command_strict_latest_strict_key_strict_env=${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh ${team_status} 30 | tee ${fail_trace_audit_log_default} && ${finalize_prefix}C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh --team-status ${team_status} --finalize-session-token ${new_session_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log ${fail_trace_audit_log_default}"
  echo "next_finalize_review_keys=collect_preflight_log_resolved collect_preflight_log_missing collect_preflight_check_reason submission_readiness_retry_command"
  echo "next_finalize_review_command=rg -n '${missing_log_review_pattern}' <entry_out_path>"
fi
