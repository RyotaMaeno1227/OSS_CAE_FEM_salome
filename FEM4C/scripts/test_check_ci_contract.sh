#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
lock_scope_id="${FEM4C_CI_CONTRACT_TEST_LOCK_SCOPE_ID:-${PPID:-$$}}"
selftest_lock_dir="${FEM4C_CI_CONTRACT_TEST_SELFTEST_LOCK_DIR:-/tmp/fem4c_test_check_ci_contract.${lock_scope_id}.lock}"
selftest_lock_pid="${selftest_lock_dir}/pid"
lock_wait_sec="${FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC:-2}"
skip_lock_wait_runtime_smoke="${FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE:-0}"
cleanup() {
  # Terminate only background jobs launched by this shell.
  local bg_pids=""
  bg_pids="$(jobs -pr 2>/dev/null || true)"
  if [[ -n "${bg_pids}" ]]; then
    kill ${bg_pids} 2>/dev/null || true
  fi
  rm -f "${selftest_lock_pid}" 2>/dev/null || true
  rmdir "${selftest_lock_dir}" 2>/dev/null || true
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

build_lock_busy_message() {
  local lock_dir="$1"
  local owner_pid_value="$2"
  local wait_sec_value="$3"
  local pair_fragment=""
  pair_fragment="$(build_lock_pair_fragment "${owner_pid_value:-unknown}" "${wait_sec_value}")"
  printf 'FAIL: test_check_ci_contract already running (%s, %s)' \
    "${lock_dir}" "${pair_fragment}"
}

build_lock_pair_fragment() {
  local owner_pid_value="$1"
  local wait_sec_value="$2"
  printf 'owner_pid=%s, lock_wait_sec=%s' \
    "${owner_pid_value:-unknown}" "${wait_sec_value}"
}

acquire_selftest_lock() {
  local owner_pid
  local now_epoch
  local deadline_epoch
  if [[ ! "${lock_wait_sec}" =~ ^[0-9]+$ ]]; then
    echo "FAIL: FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC must be a non-negative integer" >&2
    return 1
  fi
  now_epoch="$(date +%s)"
  deadline_epoch=$((now_epoch + lock_wait_sec))
  while true; do
    if mkdir "${selftest_lock_dir}" 2>/dev/null; then
      echo "$$" >"${selftest_lock_pid}"
      return 0
    fi

    if [[ -f "${selftest_lock_pid}" ]]; then
      owner_pid="$(cat "${selftest_lock_pid}" 2>/dev/null || true)"
      if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then
        rm -rf "${selftest_lock_dir}" 2>/dev/null || true
        if mkdir "${selftest_lock_dir}" 2>/dev/null; then
          echo "$$" >"${selftest_lock_pid}"
          return 0
        fi
      fi
    else
      rm -rf "${selftest_lock_dir}" 2>/dev/null || true
      if mkdir "${selftest_lock_dir}" 2>/dev/null; then
        echo "$$" >"${selftest_lock_pid}"
        return 0
      fi
    fi

    now_epoch="$(date +%s)"
    if (( now_epoch >= deadline_epoch )); then
      echo "$(build_lock_busy_message "${selftest_lock_dir}" "${owner_pid:-unknown}" "${lock_wait_sec}")" >&2
      return 1
    fi
    sleep 1
  done
}

acquire_selftest_lock

run_lock_wait_runtime_smoke() {
  local runtime_lock_dir="${tmp_dir}/runtime_lock_wait_smoke.lock"
  local runtime_lock_pid="${runtime_lock_dir}/pid"
  local runtime_log="${tmp_dir}/runtime_lock_wait_smoke.log"
  mkdir -p "${runtime_lock_dir}"
  echo "$$" >"${runtime_lock_pid}"
  if FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE=1 \
     FEM4C_CI_CONTRACT_TEST_SELFTEST_LOCK_DIR="${runtime_lock_dir}" \
     FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC=0 \
     bash "FEM4C/scripts/test_check_ci_contract.sh" >"${runtime_log}" 2>&1; then
    echo "FAIL: lock wait runtime smoke should fail immediately when lock is held and wait_sec=0" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  if ! grep -q "test_check_ci_contract already running" "${runtime_log}"; then
    echo "FAIL: lock wait runtime smoke did not emit lock contention message" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  if ! grep -q "owner_pid=$$" "${runtime_log}"; then
    echo "FAIL: lock wait runtime smoke did not emit owner_pid trace" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  if ! grep -q "lock_wait_sec=0" "${runtime_log}"; then
    echo "FAIL: lock wait runtime smoke did not emit lock_wait_sec trace" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  local expected_runtime_pair=""
  expected_runtime_pair="$(build_lock_pair_fragment "$$" "0")"
  if ! grep -Fq "${expected_runtime_pair}" "${runtime_log}"; then
    echo "FAIL: lock wait runtime smoke did not emit owner/wait pair trace" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  local expected_runtime_busy_line=""
  expected_runtime_busy_line="$(build_lock_busy_message "${runtime_lock_dir}" "$$" "0")"
  if ! grep -Fq "${expected_runtime_busy_line}" "${runtime_log}"; then
    echo "FAIL: lock wait runtime smoke did not emit lock-dir anchored pair trace" >&2
    cat "${runtime_log}" >&2
    exit 1
  fi
  rm -f "${runtime_lock_pid}" 2>/dev/null || true
  rmdir "${runtime_lock_dir}" 2>/dev/null || true
}

if [[ "${skip_lock_wait_runtime_smoke}" != "0" && "${skip_lock_wait_runtime_smoke}" != "1" ]]; then
  echo "FAIL: FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE must be 0 or 1" >&2
  exit 1
fi
if [[ "${skip_lock_wait_runtime_smoke}" == "0" ]]; then
  run_lock_wait_runtime_smoke
fi

workflow_copy="${tmp_dir}/ci.yaml"
workflow_fail_gate_call="${tmp_dir}/ci.fail.gate_call.yaml"
marker_script_pass="${tmp_dir}/check_fem4c_test_log_markers.pass.sh"
marker_script_fail_mbd_suite="${tmp_dir}/check_fem4c_test_log_markers.fail_mbd_suite.sh"
mbd_integrator_script_pass="${tmp_dir}/check_mbd_integrators.pass.sh"
mbd_integrator_script_fail_dt_case="${tmp_dir}/check_mbd_integrators.fail_dt_case.sh"
mbd_integrator_script_fail_whitespace_case="${tmp_dir}/check_mbd_integrators.fail_whitespace_case.sh"
mbd_integrator_script_fail_compact_trace_case="${tmp_dir}/check_mbd_integrators.fail_compact_trace_case.sh"
mbd_integrator_script_fail_cli_compact_trace_case="${tmp_dir}/check_mbd_integrators.fail_cli_compact_trace_case.sh"
mbd_integrator_script_fail_source_marker="${tmp_dir}/check_mbd_integrators.fail_source_marker.sh"
mbd_integrator_script_fail_time_source_marker="${tmp_dir}/check_mbd_integrators.fail_time_source_marker.sh"
mbd_integrator_script_fail_step_trace_marker="${tmp_dir}/check_mbd_integrators.fail_step_trace_marker.sh"
mbd_integrator_script_fail_bin_default_marker="${tmp_dir}/check_mbd_integrators.fail_bin_default_marker.sh"
mbd_integrator_script_fail_bin_env_override_marker="${tmp_dir}/check_mbd_integrators.fail_bin_env_override_marker.sh"
mbd_integrator_script_fail_bin_preflight_marker="${tmp_dir}/check_mbd_integrators.fail_bin_preflight_marker.sh"
a24_regression_script_pass="${tmp_dir}/run_a24_regression.pass.sh"
a24_regression_script_fail_command="${tmp_dir}/run_a24_regression.fail_command.sh"
a24_regression_script_fail_contract_knob_marker="${tmp_dir}/run_a24_regression.fail_contract_knob_marker.sh"
a24_regression_script_fail_contract_knob_validation_marker="${tmp_dir}/run_a24_regression.fail_contract_knob_validation_marker.sh"
a24_regression_script_fail_contract_skip_marker="${tmp_dir}/run_a24_regression.fail_contract_skip_marker.sh"
a24_regression_script_fail_makeflags_isolation_marker="${tmp_dir}/run_a24_regression.fail_makeflags_isolation_marker.sh"
a24_regression_script_fail_summary_marker="${tmp_dir}/run_a24_regression.fail_summary_marker.sh"
a24_regression_script_fail_summary_out_marker="${tmp_dir}/run_a24_regression.fail_summary_out_marker.sh"
a24_regression_script_fail_summary_out_dir_validation_marker="${tmp_dir}/run_a24_regression.fail_summary_out_dir_validation_marker.sh"
a24_regression_script_fail_summary_out_type_validation_marker="${tmp_dir}/run_a24_regression.fail_summary_out_type_validation_marker.sh"
a24_regression_script_fail_summary_out_write_validation_marker="${tmp_dir}/run_a24_regression.fail_summary_out_write_validation_marker.sh"
a24_regression_script_fail_skip_lock_knob_marker="${tmp_dir}/run_a24_regression.fail_skip_lock_knob_marker.sh"
a24_regression_script_fail_skip_lock_validation_marker="${tmp_dir}/run_a24_regression.fail_skip_lock_validation_marker.sh"
a24_regression_script_fail_lock_dir_default_marker="${tmp_dir}/run_a24_regression.fail_lock_dir_default_marker.sh"
a24_regression_script_fail_lock_fail_marker="${tmp_dir}/run_a24_regression.fail_lock_fail_marker.sh"
a24_regression_full_script_pass="${tmp_dir}/run_a24_regression_full.pass.sh"
a24_regression_full_script_fail_command="${tmp_dir}/run_a24_regression_full.fail_command.sh"
a24_regression_full_script_fail_summary_out_dir_validation_marker="${tmp_dir}/run_a24_regression_full.fail_summary_out_dir_validation_marker.sh"
a24_regression_full_script_fail_summary_out_type_validation_marker="${tmp_dir}/run_a24_regression_full.fail_summary_out_type_validation_marker.sh"
a24_regression_full_script_fail_summary_out_write_validation_marker="${tmp_dir}/run_a24_regression_full.fail_summary_out_write_validation_marker.sh"
a24_regression_full_script_fail_retry_knob_marker="${tmp_dir}/run_a24_regression_full.fail_retry_knob_marker.sh"
a24_regression_full_script_fail_retry_validation_marker="${tmp_dir}/run_a24_regression_full.fail_retry_validation_marker.sh"
a24_regression_full_script_fail_retry_used_marker="${tmp_dir}/run_a24_regression_full.fail_retry_used_marker.sh"
a24_regression_full_script_fail_nested_failed_step_marker="${tmp_dir}/run_a24_regression_full.fail_nested_failed_step_marker.sh"
a24_regression_full_script_fail_nested_failed_cmd_marker="${tmp_dir}/run_a24_regression_full.fail_nested_failed_cmd_marker.sh"
a24_regression_full_script_fail_nested_integrator_hint_marker="${tmp_dir}/run_a24_regression_full.fail_nested_integrator_hint_marker.sh"
a24_regression_full_script_fail_nested_log_fallback_function_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_fallback_function_marker.sh"
a24_regression_full_script_fail_nested_log_summary_parser_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_parser_marker.sh"
a24_regression_full_script_fail_nested_log_summary_crlf_trim_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_crlf_trim_marker.sh"
a24_regression_full_script_fail_nested_log_summary_line_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_line_marker.sh"
a24_regression_full_script_fail_nested_log_summary_casefold_line_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_casefold_line_marker.sh"
a24_regression_full_script_fail_nested_log_summary_casefold_token_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_casefold_token_marker.sh"
a24_regression_full_script_fail_nested_log_summary_token_equals_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_token_equals_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_key_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_key_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_value_equals_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_value_equals_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_quote_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_quote_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_value_charset_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_value_charset_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_partial_guard_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_partial_guard_marker.sh"
a24_regression_full_script_fail_nested_log_summary_precedence_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_precedence_marker.sh"
a24_regression_full_script_fail_nested_log_summary_parse_call_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_summary_parse_call_marker.sh"
a24_regression_full_script_fail_nested_log_fallback_pattern_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_fallback_pattern_marker.sh"
a24_regression_full_script_fail_nested_log_fallback_call_marker="${tmp_dir}/run_a24_regression_full.fail_nested_log_fallback_call_marker.sh"
a24_batch_script_pass="${tmp_dir}/run_a24_batch.pass.sh"
a24_batch_script_fail_regression_command="${tmp_dir}/run_a24_batch.fail_regression_command.sh"
a24_batch_script_fail_command="${tmp_dir}/run_a24_batch.fail_command.sh"
a24_batch_script_fail_lock_pid_marker="${tmp_dir}/run_a24_batch.fail_lock_pid_marker.sh"
a24_batch_script_fail_lock_marker="${tmp_dir}/run_a24_batch.fail_lock_marker.sh"
a24_batch_script_fail_stale_recovery_marker="${tmp_dir}/run_a24_batch.fail_stale_recovery_marker.sh"
a24_batch_script_fail_summary_out_marker="${tmp_dir}/run_a24_batch.fail_summary_out_marker.sh"
a24_batch_script_fail_summary_out_dir_validation_marker="${tmp_dir}/run_a24_batch.fail_summary_out_dir_validation_marker.sh"
a24_batch_script_fail_summary_out_type_validation_marker="${tmp_dir}/run_a24_batch.fail_summary_out_type_validation_marker.sh"
a24_batch_script_fail_summary_out_write_validation_marker="${tmp_dir}/run_a24_batch.fail_summary_out_write_validation_marker.sh"
a24_batch_script_fail_makeflags_marker="${tmp_dir}/run_a24_batch.fail_makeflags_marker.sh"
a24_batch_script_fail_failed_cmd_marker="${tmp_dir}/run_a24_batch.fail_failed_cmd_marker.sh"
a24_batch_script_fail_retry_knob_marker="${tmp_dir}/run_a24_batch.fail_retry_knob_marker.sh"
a24_batch_script_fail_retry_validation_marker="${tmp_dir}/run_a24_batch.fail_retry_validation_marker.sh"
a24_batch_script_fail_retry_used_marker="${tmp_dir}/run_a24_batch.fail_retry_used_marker.sh"
a24_batch_script_fail_nested_failed_step_marker="${tmp_dir}/run_a24_batch.fail_nested_failed_step_marker.sh"
a24_batch_script_fail_nested_failed_cmd_marker="${tmp_dir}/run_a24_batch.fail_nested_failed_cmd_marker.sh"
a24_batch_script_fail_nested_integrator_hint_marker="${tmp_dir}/run_a24_batch.fail_nested_integrator_hint_marker.sh"
a24_batch_script_fail_nested_log_fallback_function_marker="${tmp_dir}/run_a24_batch.fail_nested_log_fallback_function_marker.sh"
a24_batch_script_fail_nested_log_summary_parser_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_parser_marker.sh"
a24_batch_script_fail_nested_log_summary_crlf_trim_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_crlf_trim_marker.sh"
a24_batch_script_fail_nested_log_summary_line_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_line_marker.sh"
a24_batch_script_fail_nested_log_summary_casefold_line_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_casefold_line_marker.sh"
a24_batch_script_fail_nested_log_summary_casefold_token_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_casefold_token_marker.sh"
a24_batch_script_fail_nested_log_summary_token_equals_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_token_equals_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_key_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_key_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_value_equals_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_value_equals_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_quote_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_quote_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_value_charset_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_value_charset_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_partial_guard_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_partial_guard_marker.sh"
a24_batch_script_fail_nested_log_summary_precedence_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_precedence_marker.sh"
a24_batch_script_fail_nested_log_summary_parse_call_marker="${tmp_dir}/run_a24_batch.fail_nested_log_summary_parse_call_marker.sh"
a24_batch_script_fail_nested_log_fallback_pattern_marker="${tmp_dir}/run_a24_batch.fail_nested_log_fallback_pattern_marker.sh"
a24_batch_script_fail_nested_log_fallback_call_marker="${tmp_dir}/run_a24_batch.fail_nested_log_fallback_call_marker.sh"
a24_regression_test_script_pass="${tmp_dir}/test_run_a24_regression.pass.sh"
a24_regression_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_regression.fail_build_preflight_marker.sh"
a24_regression_test_script_fail_summary_out_missing_dir_case_marker="${tmp_dir}/test_run_a24_regression.fail_summary_out_missing_dir_case_marker.sh"
a24_regression_test_script_fail_summary_out_dir_path_case_marker="${tmp_dir}/test_run_a24_regression.fail_summary_out_dir_path_case_marker.sh"
a24_regression_test_script_fail_summary_out_readonly_parent_case_marker="${tmp_dir}/test_run_a24_regression.fail_summary_out_readonly_parent_case_marker.sh"
a24_regression_test_script_fail_summary_out_write_case_marker="${tmp_dir}/test_run_a24_regression.fail_summary_out_write_case_marker.sh"
a24_batch_test_script_pass="${tmp_dir}/test_run_a24_batch.pass.sh"
a24_batch_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_batch.fail_build_preflight_marker.sh"
a24_batch_test_script_fail_selftest_lock_dir_marker="${tmp_dir}/test_run_a24_batch.fail_selftest_lock_dir_marker.sh"
a24_batch_test_script_fail_selftest_lock_function_marker="${tmp_dir}/test_run_a24_batch.fail_selftest_lock_function_marker.sh"
a24_batch_test_script_fail_selftest_lock_busy_marker="${tmp_dir}/test_run_a24_batch.fail_selftest_lock_busy_marker.sh"
a24_batch_test_script_fail_selftest_lock_stale_recovery_marker="${tmp_dir}/test_run_a24_batch.fail_selftest_lock_stale_recovery_marker.sh"
a24_batch_test_script_fail_full_chain_marker="${tmp_dir}/test_run_a24_batch.fail_full_chain_marker.sh"
a24_batch_test_script_fail_summary_out_missing_dir_case_marker="${tmp_dir}/test_run_a24_batch.fail_summary_out_missing_dir_case_marker.sh"
a24_batch_test_script_fail_summary_out_dir_path_case_marker="${tmp_dir}/test_run_a24_batch.fail_summary_out_dir_path_case_marker.sh"
a24_batch_test_script_fail_summary_out_readonly_parent_case_marker="${tmp_dir}/test_run_a24_batch.fail_summary_out_readonly_parent_case_marker.sh"
a24_batch_test_script_fail_summary_out_write_case_marker="${tmp_dir}/test_run_a24_batch.fail_summary_out_write_case_marker.sh"
a24_batch_test_script_fail_nonexec_bin_case_marker="${tmp_dir}/test_run_a24_batch.fail_nonexec_bin_case_marker.sh"
a24_batch_test_script_fail_regression_lock_dir_isolation_marker="${tmp_dir}/test_run_a24_batch.fail_regression_lock_dir_isolation_marker.sh"
a24_batch_test_script_fail_log_summary_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_precedence_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_precedence_case_marker.sh"
a24_batch_test_script_fail_log_summary_malformed_key_precedence_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_malformed_key_precedence_case_marker.sh"
a24_batch_test_script_fail_log_summary_duplicate_key_precedence_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_duplicate_key_precedence_case_marker.sh"
a24_batch_test_script_fail_log_summary_malformed_unknown_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_malformed_unknown_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_uppercase_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_uppercase_case_marker.sh"
a24_batch_test_script_fail_log_summary_uppercase_precedence_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_uppercase_precedence_case_marker.sh"
a24_batch_test_script_fail_log_summary_crlf_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_crlf_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_tab_whitespace_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_tab_whitespace_case_marker.sh"
a24_batch_test_script_fail_log_summary_equals_whitespace_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_equals_whitespace_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_quoted_value_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_quoted_value_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_single_quote_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_single_quote_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_quote_mixed_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_quote_mixed_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_backslash_value_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_backslash_value_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_quoted_key_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_quoted_key_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_single_quoted_key_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_single_quoted_key_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_quoted_cmd_key_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_partial_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_partial_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_missing_equals_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_missing_equals_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_empty_value_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_empty_value_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_empty_key_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_empty_key_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_extra_equals_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_extra_equals_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_leading_punct_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_leading_punct_fallback_case_marker.sh"
a24_batch_test_script_fail_log_summary_internal_symbol_fallback_case_marker="${tmp_dir}/test_run_a24_batch.fail_log_summary_internal_symbol_fallback_case_marker.sh"
a24_regression_full_test_script_pass="${tmp_dir}/test_run_a24_regression_full.pass.sh"
a24_regression_full_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_regression_full.fail_build_preflight_marker.sh"
a24_regression_full_test_script_fail_selftest_lock_dir_marker="${tmp_dir}/test_run_a24_regression_full.fail_selftest_lock_dir_marker.sh"
a24_regression_full_test_script_fail_selftest_lock_function_marker="${tmp_dir}/test_run_a24_regression_full.fail_selftest_lock_function_marker.sh"
a24_regression_full_test_script_fail_selftest_lock_busy_marker="${tmp_dir}/test_run_a24_regression_full.fail_selftest_lock_busy_marker.sh"
a24_regression_full_test_script_fail_selftest_lock_stale_recovery_marker="${tmp_dir}/test_run_a24_regression_full.fail_selftest_lock_stale_recovery_marker.sh"
a24_regression_full_test_script_fail_summary_out_missing_dir_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_summary_out_missing_dir_case_marker.sh"
a24_regression_full_test_script_fail_summary_out_dir_path_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_summary_out_dir_path_case_marker.sh"
a24_regression_full_test_script_fail_summary_out_readonly_parent_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_summary_out_readonly_parent_case_marker.sh"
a24_regression_full_test_script_fail_summary_out_write_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_summary_out_write_case_marker.sh"
a24_regression_full_test_script_fail_nonexec_bin_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_nonexec_bin_case_marker.sh"
a24_regression_full_test_script_fail_regression_lock_dir_isolation_marker="${tmp_dir}/test_run_a24_regression_full.fail_regression_lock_dir_isolation_marker.sh"
a24_regression_full_test_script_fail_log_summary_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_precedence_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_precedence_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_malformed_key_precedence_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_malformed_key_precedence_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_duplicate_key_precedence_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_duplicate_key_precedence_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_malformed_unknown_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_malformed_unknown_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_uppercase_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_uppercase_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_uppercase_precedence_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_uppercase_precedence_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_crlf_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_crlf_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_tab_whitespace_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_tab_whitespace_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_equals_whitespace_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_equals_whitespace_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_quoted_value_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_quoted_value_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_single_quote_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_single_quote_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_quote_mixed_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_quote_mixed_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_backslash_value_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_backslash_value_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_quoted_key_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_quoted_key_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_single_quoted_key_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_single_quoted_key_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_quoted_cmd_key_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_partial_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_partial_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_missing_equals_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_missing_equals_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_empty_value_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_empty_value_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_empty_key_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_empty_key_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_extra_equals_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_extra_equals_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_leading_punct_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_leading_punct_fallback_case_marker.sh"
a24_regression_full_test_script_fail_log_summary_internal_symbol_fallback_case_marker="${tmp_dir}/test_run_a24_regression_full.fail_log_summary_internal_symbol_fallback_case_marker.sh"
b8_regression_script_pass="${tmp_dir}/run_b8_regression.pass.sh"
b8_regression_script_fail_validation="${tmp_dir}/run_b8_regression.fail_validation.sh"
b8_regression_script_fail_make_validation="${tmp_dir}/run_b8_regression.fail_make_validation.sh"
b8_regression_script_fail_b14_target_default="${tmp_dir}/run_b8_regression.fail_b14_target_default.sh"
b8_regression_script_fail_b14_target_pass_through="${tmp_dir}/run_b8_regression.fail_b14_target_pass_through.sh"
b8_regression_script_fail_makeflags_isolation="${tmp_dir}/run_b8_regression.fail_makeflags_isolation.sh"
b8_regression_full_script_pass="${tmp_dir}/run_b8_regression_full.pass.sh"
b8_regression_full_script_fail_validation="${tmp_dir}/run_b8_regression_full.fail_validation.sh"
b8_regression_full_script_fail_make_validation="${tmp_dir}/run_b8_regression_full.fail_make_validation.sh"
b8_regression_full_script_fail_b14_target_default="${tmp_dir}/run_b8_regression_full.fail_b14_target_default.sh"
b8_regression_full_script_fail_b14_target_pass_through="${tmp_dir}/run_b8_regression_full.fail_b14_target_pass_through.sh"
b8_knob_matrix_script_pass="${tmp_dir}/test_b8_knob_matrix.pass.sh"
b8_knob_matrix_script_fail_regression_one="${tmp_dir}/test_b8_knob_matrix.fail_regression_one.sh"
b8_knob_matrix_script_fail_regression_invalid_make="${tmp_dir}/test_b8_knob_matrix.fail_regression_invalid_make.sh"
b8_knob_matrix_script_fail_full_one="${tmp_dir}/test_b8_knob_matrix.fail_full_one.sh"
b8_knob_matrix_script_fail_full_invalid_make="${tmp_dir}/test_b8_knob_matrix.fail_full_invalid_make.sh"
b8_knob_matrix_script_fail_skip_validation="${tmp_dir}/test_b8_knob_matrix.fail_skip_validation.sh"
b8_knob_matrix_script_fail_skip_info_marker="${tmp_dir}/test_b8_knob_matrix.fail_skip_info_marker.sh"
b8_knob_matrix_script_fail_local_target_env_marker="${tmp_dir}/test_b8_knob_matrix.fail_local_target_env_marker.sh"
b8_knob_matrix_script_fail_regression_repo_default_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_regression_repo_default_lock_source.sh"
b8_knob_matrix_script_fail_regression_global_default_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_regression_global_default_lock_source.sh"
b8_knob_matrix_script_fail_regression_env_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_regression_env_lock_source.sh"
b8_knob_matrix_script_fail_regression_env_lock_source_trace="${tmp_dir}/test_b8_knob_matrix.fail_regression_env_lock_source_trace.sh"
b8_knob_matrix_script_fail_full_repo_default_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_full_repo_default_lock_source.sh"
b8_knob_matrix_script_fail_full_global_default_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_full_global_default_lock_source.sh"
b8_knob_matrix_script_fail_full_env_lock_source="${tmp_dir}/test_b8_knob_matrix.fail_full_env_lock_source.sh"
b8_knob_matrix_script_fail_full_env_lock_source_trace="${tmp_dir}/test_b8_knob_matrix.fail_full_env_lock_source_trace.sh"
b8_knob_matrix_script_fail_full_parser_lock_cleanup="${tmp_dir}/test_b8_knob_matrix.fail_full_parser_lock_cleanup.sh"
b8_guard_script_pass="${tmp_dir}/run_b8_guard.pass.sh"
b8_guard_script_fail_makeflags_isolation="${tmp_dir}/run_b8_guard.fail_makeflags_isolation.sh"
b8_guard_script_fail_local_target_default="${tmp_dir}/run_b8_guard.fail_local_target_default.sh"
b8_guard_test_script_pass="${tmp_dir}/test_run_b8_guard.pass.sh"
b8_guard_test_script_fail_makeflags_case_marker="${tmp_dir}/test_run_b8_guard.fail_makeflags_case_marker.sh"
b8_guard_contract_test_script_pass="${tmp_dir}/test_run_b8_guard_contract.pass.sh"
b8_guard_contract_test_script_fail_b14_target_override="${tmp_dir}/test_run_b8_guard_contract.fail_b14_target_override.sh"
b8_guard_contract_test_script_fail_temp_copy_marker="${tmp_dir}/test_run_b8_guard_contract.fail_temp_copy_marker.sh"
b8_regression_test_script_pass="${tmp_dir}/test_run_b8_regression.pass.sh"
b8_regression_test_script_fail_temp_copy_marker="${tmp_dir}/test_run_b8_regression.fail_temp_copy_marker.sh"
b8_regression_full_test_script_pass="${tmp_dir}/test_run_b8_regression_full.pass.sh"
b8_regression_full_test_script_fail_temp_copy_marker="${tmp_dir}/test_run_b8_regression_full.fail_temp_copy_marker.sh"
a24_acceptance_serial_script_pass="${tmp_dir}/run_a24_acceptance_serial.pass.sh"
a24_acceptance_serial_script_fail_summary_marker="${tmp_dir}/run_a24_acceptance_serial.fail_summary_marker.sh"
a24_acceptance_serial_script_fail_retry_knob_marker="${tmp_dir}/run_a24_acceptance_serial.fail_retry_knob_marker.sh"
a24_acceptance_serial_script_fail_retry_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_retry_validation_marker.sh"
a24_acceptance_serial_script_fail_fake_step_knob_marker="${tmp_dir}/run_a24_acceptance_serial.fail_fake_step_knob_marker.sh"
a24_acceptance_serial_script_fail_fake_step_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_fake_step_validation_marker.sh"
a24_acceptance_serial_script_fail_step_log_dir_knob_marker="${tmp_dir}/run_a24_acceptance_serial.fail_step_log_dir_knob_marker.sh"
a24_acceptance_serial_script_fail_step_log_dir_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_step_log_dir_validation_marker.sh"
a24_acceptance_serial_script_fail_step_log_dir_type_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_step_log_dir_type_validation_marker.sh"
a24_acceptance_serial_script_fail_step_log_dir_writable_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_step_log_dir_writable_validation_marker.sh"
a24_acceptance_serial_script_fail_summary_out_dir_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_summary_out_dir_validation_marker.sh"
a24_acceptance_serial_script_fail_summary_out_type_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_summary_out_type_validation_marker.sh"
a24_acceptance_serial_script_fail_summary_out_write_validation_marker="${tmp_dir}/run_a24_acceptance_serial.fail_summary_out_write_validation_marker.sh"
a24_acceptance_serial_script_fail_failed_rc_marker="${tmp_dir}/run_a24_acceptance_serial.fail_failed_rc_marker.sh"
a24_acceptance_serial_script_fail_failed_log_marker="${tmp_dir}/run_a24_acceptance_serial.fail_failed_log_marker.sh"
a24_acceptance_serial_script_fail_cmd_ci_contract_marker="${tmp_dir}/run_a24_acceptance_serial.fail_cmd_ci_contract_marker.sh"
a24_acceptance_serial_test_script_pass="${tmp_dir}/test_run_a24_acceptance_serial.pass.sh"
a24_acceptance_serial_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_build_preflight_marker.sh"
a24_acceptance_serial_test_script_fail_retry_knob_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_retry_knob_case_marker.sh"
a24_acceptance_serial_test_script_fail_fake_step_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_fake_step_case_marker.sh"
a24_acceptance_serial_test_script_fail_step_log_dir_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_step_log_dir_case_marker.sh"
a24_acceptance_serial_test_script_fail_step_log_file_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_step_log_file_case_marker.sh"
a24_acceptance_serial_test_script_fail_step_log_readonly_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_step_log_readonly_case_marker.sh"
a24_acceptance_serial_test_script_fail_summary_out_missing_dir_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_summary_out_missing_dir_case_marker.sh"
a24_acceptance_serial_test_script_fail_summary_out_dir_path_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_summary_out_dir_path_case_marker.sh"
a24_acceptance_serial_test_script_fail_summary_out_readonly_case_marker="${tmp_dir}/test_run_a24_acceptance_serial.fail_summary_out_readonly_case_marker.sh"
ci_contract_test_script_pass="${tmp_dir}/test_check_ci_contract.pass.sh"
ci_contract_test_script_fail_selftest_lock_scope_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_scope_marker.sh"
ci_contract_test_script_fail_selftest_lock_dir_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_dir_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_knob_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_knob_marker.sh"
ci_contract_test_script_fail_selftest_lock_pair_fragment_function_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_pair_fragment_function_marker.sh"
ci_contract_test_script_fail_selftest_lock_pair_fragment_busy_call_order_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_pair_fragment_busy_call_order_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_knob_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_skip_knob_marker.sh"
ci_contract_test_script_fail_selftest_lock_function_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_function_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_function_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_function_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_child_call_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_child_call_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_validation_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_skip_validation_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_invocation_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_invocation_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_fail_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_fail_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_contention_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_contention_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_owner_trace_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_owner_trace_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_wait_trace_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_wait_trace_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_trace_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_pair_trace_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_grep_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_pair_grep_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_call_order_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_pair_call_order_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_template_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_template_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_pair_fragment_arg_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_pair_fragment_arg_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_owner_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_owner_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_wait_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_wait_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_owner_wait_pair_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_owner_wait_pair_marker.sh"
ci_contract_test_script_fail_selftest_lock_busy_owner_wait_order_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_busy_owner_wait_order_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_validation_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_validation_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_deadline_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_deadline_marker.sh"
ci_contract_test_script_fail_selftest_lock_missing_pid_recovery_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_missing_pid_recovery_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_deadline_guard_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_deadline_guard_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_loop_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_loop_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_sleep_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_sleep_marker.sh"
ci_contract_test_script_fail_selftest_lock_wait_now_epoch_refresh_count_marker="${tmp_dir}/test_check_ci_contract.fail_selftest_lock_wait_now_epoch_refresh_count_marker.sh"
ci_contract_test_script_fail_cleanup_jobs_marker="${tmp_dir}/test_check_ci_contract.fail_cleanup_jobs_marker.sh"
ci_contract_test_script_fail_cleanup_kill_marker="${tmp_dir}/test_check_ci_contract.fail_cleanup_kill_marker.sh"
ci_contract_test_script_fail_cleanup_call_order_marker="${tmp_dir}/test_check_ci_contract.fail_cleanup_call_order_marker.sh"
ci_contract_test_script_fail_cleanup_no_pkill_marker="${tmp_dir}/test_check_ci_contract.fail_cleanup_no_pkill_marker.sh"
makefile_pass="${tmp_dir}/Makefile.pass"
makefile_fail_mbd_chain="${tmp_dir}/Makefile.fail.mbd_chain"
makefile_fail_test_entry="${tmp_dir}/Makefile.fail.test_entry"
makefile_fail_a21_target="${tmp_dir}/Makefile.fail.a21_target"
makefile_fail_a21_test_target="${tmp_dir}/Makefile.fail.a21_test_target"
makefile_fail_a24_target="${tmp_dir}/Makefile.fail.a24_target"
makefile_fail_a24_test_target="${tmp_dir}/Makefile.fail.a24_test_target"
makefile_fail_a24_full_target="${tmp_dir}/Makefile.fail.a24_full_target"
makefile_fail_a24_full_test_target="${tmp_dir}/Makefile.fail.a24_full_test_target"
makefile_fail_a24_batch_target="${tmp_dir}/Makefile.fail.a24_batch_target"
makefile_fail_a24_batch_test_target="${tmp_dir}/Makefile.fail.a24_batch_test_target"
makefile_fail_a24_acceptance_serial_target="${tmp_dir}/Makefile.fail.a24_acceptance_serial_target"
makefile_fail_a24_acceptance_serial_test_target="${tmp_dir}/Makefile.fail.a24_acceptance_serial_test_target"
makefile_fail_a24_acceptance_serial_help="${tmp_dir}/Makefile.fail.a24_acceptance_serial_help"
makefile_fail_a24_acceptance_serial_test_help="${tmp_dir}/Makefile.fail.a24_acceptance_serial_test_help"
makefile_fail_b8_guard_test_target="${tmp_dir}/Makefile.fail.b8_guard_test_target"
makefile_fail_b8_guard_contract_target="${tmp_dir}/Makefile.fail.b8_guard_contract_target"
makefile_fail_b8_guard_contract_test_target="${tmp_dir}/Makefile.fail.b8_guard_contract_test_target"
makefile_fail_b8_syntax_target="${tmp_dir}/Makefile.fail.b8_syntax_target"
makefile_fail_b8_output_test_target="${tmp_dir}/Makefile.fail.b8_output_test_target"
makefile_fail_b8_target="${tmp_dir}/Makefile.fail.b8_target"
makefile_fail_b8_test_target="${tmp_dir}/Makefile.fail.b8_test_target"
makefile_fail_b8_full_target="${tmp_dir}/Makefile.fail.b8_full_target"
makefile_fail_b8_full_test_target="${tmp_dir}/Makefile.fail.b8_full_test_target"
makefile_fail_b8_knob_matrix_test_target="${tmp_dir}/Makefile.fail.b8_knob_matrix_test_target"
makefile_fail_b8_knob_matrix_smoke_test_target="${tmp_dir}/Makefile.fail.b8_knob_matrix_smoke_test_target"
makefile_fail_b8_knob_matrix_smoke_skip_flag="${tmp_dir}/Makefile.fail.b8_knob_matrix_smoke_skip_flag"
makefile_fail_b8_make_knob="${tmp_dir}/Makefile.fail.b8_make_knob"
makefile_fail_b8_b14_knob="${tmp_dir}/Makefile.fail.b8_b14_knob"
makefile_fail_b8_local_target_default="${tmp_dir}/Makefile.fail.b8_local_target_default"

cp ".github/workflows/ci.yaml" "${workflow_copy}"
cp ".github/workflows/ci.yaml" "${workflow_fail_gate_call}"
cp "FEM4C/scripts/check_fem4c_test_log_markers.sh" "${marker_script_pass}"
cp "FEM4C/scripts/check_fem4c_test_log_markers.sh" "${marker_script_fail_mbd_suite}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_pass}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_dt_case}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_whitespace_case}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_compact_trace_case}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_cli_compact_trace_case}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_source_marker}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_time_source_marker}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_step_trace_marker}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_bin_default_marker}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_bin_env_override_marker}"
cp "FEM4C/scripts/check_mbd_integrators.sh" "${mbd_integrator_script_fail_bin_preflight_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_pass}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_command}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_knob_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_knob_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_skip_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_makeflags_isolation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_out_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_out_dir_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_out_type_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_out_write_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_skip_lock_knob_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_skip_lock_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_lock_dir_default_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_lock_fail_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_pass}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_command}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_summary_out_dir_validation_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_summary_out_type_validation_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_summary_out_write_validation_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_knob_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_validation_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_used_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_failed_step_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_failed_cmd_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_integrator_hint_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_fallback_function_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_parser_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_crlf_trim_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_line_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_casefold_line_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_casefold_token_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_token_equals_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_key_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_value_equals_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_quote_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_value_charset_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_partial_guard_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_precedence_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_summary_parse_call_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_fallback_pattern_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_nested_log_fallback_call_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_pass}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_regression_command}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_command}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_lock_pid_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_lock_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_stale_recovery_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_summary_out_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_summary_out_dir_validation_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_summary_out_type_validation_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_summary_out_write_validation_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_makeflags_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_failed_cmd_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_knob_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_validation_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_used_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_failed_step_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_failed_cmd_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_integrator_hint_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_fallback_function_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_parser_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_crlf_trim_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_line_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_casefold_line_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_casefold_token_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_token_equals_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_key_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_value_equals_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_quote_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_value_charset_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_partial_guard_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_precedence_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_summary_parse_call_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_fallback_pattern_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_nested_log_fallback_call_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_summary_out_missing_dir_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_summary_out_dir_path_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_summary_out_readonly_parent_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_summary_out_write_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_selftest_lock_dir_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_selftest_lock_function_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_selftest_lock_busy_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_selftest_lock_stale_recovery_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_full_chain_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_summary_out_missing_dir_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_summary_out_dir_path_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_summary_out_readonly_parent_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_summary_out_write_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_nonexec_bin_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_regression_lock_dir_isolation_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_malformed_key_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_duplicate_key_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_uppercase_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_uppercase_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_crlf_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_tab_whitespace_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_quoted_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_single_quote_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_quote_mixed_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_backslash_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_quoted_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_partial_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_missing_equals_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_empty_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_empty_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_extra_equals_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_leading_punct_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_log_summary_internal_symbol_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_selftest_lock_dir_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_selftest_lock_function_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_selftest_lock_busy_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_selftest_lock_stale_recovery_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_summary_out_missing_dir_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_summary_out_dir_path_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_summary_out_readonly_parent_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_summary_out_write_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_nonexec_bin_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_regression_lock_dir_isolation_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_malformed_key_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_duplicate_key_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_uppercase_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_uppercase_precedence_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_crlf_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_tab_whitespace_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_quoted_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_single_quote_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_quote_mixed_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_backslash_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_quoted_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_partial_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_missing_equals_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_empty_value_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_empty_key_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_extra_equals_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_leading_punct_fallback_case_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_log_summary_internal_symbol_fallback_case_marker}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_pass}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_validation}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_make_validation}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_b14_target_default}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_b14_target_pass_through}"
cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_pass}"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_make_validation}"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_pass}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_one}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_invalid_make}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_one}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_invalid_make}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_skip_validation}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_skip_info_marker}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_local_target_env_marker}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_repo_default_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_global_default_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_env_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_env_lock_source_trace}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_repo_default_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_global_default_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_env_lock_source}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_env_lock_source_trace}"
cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
cp "FEM4C/scripts/run_b8_guard.sh" "${b8_guard_script_pass}"
cp "FEM4C/scripts/run_b8_guard.sh" "${b8_guard_script_fail_makeflags_isolation}"
cp "FEM4C/scripts/run_b8_guard.sh" "${b8_guard_script_fail_local_target_default}"
cp "FEM4C/scripts/test_run_b8_guard.sh" "${b8_guard_test_script_pass}"
cp "FEM4C/scripts/test_run_b8_guard.sh" "${b8_guard_test_script_fail_makeflags_case_marker}"
cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_pass}"
cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_b14_target_override}"
cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_temp_copy_marker}"
cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_pass}"
cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_pass}"
cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_pass}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_summary_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_retry_knob_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_retry_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_fake_step_knob_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_fake_step_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_step_log_dir_knob_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_step_log_dir_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_step_log_dir_type_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_step_log_dir_writable_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_summary_out_dir_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_summary_out_type_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_summary_out_write_validation_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_failed_rc_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_failed_log_marker}"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${a24_acceptance_serial_script_fail_cmd_ci_contract_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_retry_knob_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_fake_step_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_step_log_dir_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_step_log_file_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_step_log_readonly_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_summary_out_missing_dir_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_summary_out_dir_path_case_marker}"
cp "FEM4C/scripts/test_run_a24_acceptance_serial.sh" "${a24_acceptance_serial_test_script_fail_summary_out_readonly_case_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_pass}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_scope_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_dir_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_knob_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_pair_fragment_function_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_pair_fragment_busy_call_order_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_knob_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_function_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_function_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_child_call_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_validation_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_invocation_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_fail_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_contention_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_owner_trace_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_wait_trace_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_trace_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_grep_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_call_order_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_template_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_pair_fragment_arg_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_owner_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_wait_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_pair_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_order_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_validation_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_deadline_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_missing_pid_recovery_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_deadline_guard_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_loop_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_sleep_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_selftest_lock_wait_now_epoch_refresh_count_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_cleanup_jobs_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_cleanup_kill_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_cleanup_call_order_marker}"
cp "FEM4C/scripts/test_check_ci_contract.sh" "${ci_contract_test_script_fail_cleanup_no_pkill_marker}"
cp "FEM4C/Makefile" "${makefile_pass}"
cp "FEM4C/Makefile" "${makefile_fail_mbd_chain}"
cp "FEM4C/Makefile" "${makefile_fail_test_entry}"
cp "FEM4C/Makefile" "${makefile_fail_a21_target}"
cp "FEM4C/Makefile" "${makefile_fail_a21_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_full_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_full_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_batch_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_batch_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_acceptance_serial_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_acceptance_serial_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_a24_acceptance_serial_help}"
cp "FEM4C/Makefile" "${makefile_fail_a24_acceptance_serial_test_help}"
cp "FEM4C/Makefile" "${makefile_fail_b8_guard_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_guard_contract_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_guard_contract_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_syntax_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_output_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_full_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_full_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_knob_matrix_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_knob_matrix_smoke_test_target}"
cp "FEM4C/Makefile" "${makefile_fail_b8_knob_matrix_smoke_skip_flag}"
cp "FEM4C/Makefile" "${makefile_fail_b8_make_knob}"
cp "FEM4C/Makefile" "${makefile_fail_b8_b14_knob}"
cp "FEM4C/Makefile" "${makefile_fail_b8_local_target_default}"

if ! bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_pass.log" 2>&1; then
  echo "FAIL: check_ci_contract should pass with current workflow/makefile" >&2
  cat "${tmp_dir}/contract_pass.log" >&2
  exit 1
fi

sed -i 's/lock_scope_id="${FEM4C_CI_CONTRACT_TEST_LOCK_SCOPE_ID:-${PPID:-$$}}"/lock_scope_id_removed="${FEM4C_CI_CONTRACT_TEST_LOCK_SCOPE_ID:-${PPID:-$$}}"/' "${ci_contract_test_script_fail_selftest_lock_scope_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_scope_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_scope_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock scope marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_scope_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_scope_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_scope_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_scope_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_scope_marker.log" >&2
  exit 1
fi

sed -i 's#selftest_lock_dir="${FEM4C_CI_CONTRACT_TEST_SELFTEST_LOCK_DIR:-/tmp/fem4c_test_check_ci_contract.${lock_scope_id}.lock}"#selftest_lock_dir_removed="${FEM4C_CI_CONTRACT_TEST_SELFTEST_LOCK_DIR:-/tmp/fem4c_test_check_ci_contract.${lock_scope_id}.lock}"#' "${ci_contract_test_script_fail_selftest_lock_dir_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_dir_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock dir marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_dir_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

sed -i 's/lock_wait_sec="${FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC:-2}"/lock_wait_sec_removed="${FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC:-2}"/' "${ci_contract_test_script_fail_selftest_lock_wait_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_knob_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock wait knob marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_knob_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_knob_marker.log" >&2
  exit 1
fi

sed -i 's/build_lock_pair_fragment() {/build_lock_pair_fragment_removed() {/' "${ci_contract_test_script_fail_selftest_lock_pair_fragment_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_pair_fragment_function_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock pair fragment function marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_pair_fragment_builder_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_function_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_pair_fragment_builder_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_function_marker.log" >&2
  exit 1
fi

sed -i 's/pair_fragment="$(build_lock_pair_fragment "${owner_pid_value:-unknown}" "${wait_sec_value}")"/pair_fragment="$(build_lock_pair_fragment "${wait_sec_value}" "${owner_pid_value:-unknown}")"/' "${ci_contract_test_script_fail_selftest_lock_pair_fragment_busy_call_order_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_pair_fragment_busy_call_order_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_busy_call_order_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script swaps lock pair fragment busy call order" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_busy_call_order_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_pair_fragment_busy_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_busy_call_order_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_pair_fragment_busy_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_pair_fragment_busy_call_order_marker.log" >&2
  exit 1
fi

sed -i 's/skip_lock_wait_runtime_smoke="${FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE:-0}"/skip_lock_wait_runtime_smoke_removed="${FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE:-0}"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_knob_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime skip knob marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker.log" >&2
  exit 1
fi

sed -i 's/run_lock_wait_runtime_smoke() {/run_lock_wait_runtime_smoke_removed() {/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_function_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke function marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker.log" >&2
  exit 1
fi

sed -i 's/FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE=1/FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE_REMOVED=1/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_child_call_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_child_call_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke child call marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker.log" >&2
  exit 1
fi

sed -i 's/FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE must be 0 or 1/FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE marker removed/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_skip_validation_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime skip validation marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker.log" >&2
  exit 1
fi

sed -i 's/if \[\[ "${skip_lock_wait_runtime_smoke}" == "0" \]\]; then/if [[ "${skip_lock_wait_runtime_smoke}" == "9" ]]; then/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_invocation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_invocation_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke invocation marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker.log" >&2
  exit 1
fi

sed -i 's|lock wait runtime smoke did not emit owner/wait pair trace|lock wait runtime smoke pair marker removed|' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_trace_message_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_trace_message_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke pair trace message marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker.log" >&2
  exit 1
fi

sed -i 's/grep -Fq "${expected_runtime_pair}" "${runtime_log}"/grep -Fq "${expected_runtime_pair_removed}" "${runtime_log}"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_grep_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_grep_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke pair grep marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker.log" >&2
  exit 1
fi

sed -i 's/expected_runtime_pair="$(build_lock_pair_fragment "$$" "0")"/expected_runtime_pair="$(build_lock_pair_fragment "0" "$$")"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_call_order_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_call_order_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script swaps runtime pair call order" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker.log" >&2
  exit 1
fi

sed -i 's/expected_runtime_pair="$(build_lock_pair_fragment "$$" "0")"/expected_runtime_pair="$(printf '"'"'\'"'"''"'"'owner_pid=%s, lock_wait_sec=%s'"'"'\'"'"''"'"' "$$" "0")"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script uses runtime pair literal fallback" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_absence_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_absence_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_marker.log" >&2
  exit 1
fi

sed -i 's/lock wait runtime smoke did not emit lock-dir anchored pair trace/lock wait runtime smoke lock-dir pair marker removed/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke lock-dir pair trace message marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker.log" >&2
  exit 1
fi

sed -i 's/grep -Fq "${expected_runtime_busy_line}" "${runtime_log}"/grep -Fq "${expected_runtime_busy_line_removed}" "${runtime_log}"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait runtime smoke lock-dir pair grep marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker.log" >&2
  exit 1
fi

sed -i 's/expected_runtime_busy_line="$(build_lock_busy_message "${runtime_lock_dir}" "$$" "0")"/expected_runtime_busy_line="$(build_lock_busy_message "${runtime_lock_dir}" "0" "$$")"/' "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script swaps lock wait runtime smoke lock-dir pair grep order" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker.log" >&2
  exit 1
fi

sed -i 's/FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC must be a non-negative integer/FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC marker removed/' "${ci_contract_test_script_fail_selftest_lock_wait_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_validation_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait validation marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_validation_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_validation_marker.log" >&2
  exit 1
fi

sed -i 's/deadline_epoch=$((now_epoch + lock_wait_sec))/deadline_epoch_removed=$((now_epoch + lock_wait_sec))/' "${ci_contract_test_script_fail_selftest_lock_wait_deadline_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_deadline_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait deadline marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_deadline_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_deadline_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_marker.log" >&2
  exit 1
fi

sed -i 's/if (( now_epoch >= deadline_epoch )); then/if (( now_epoch > deadline_epoch )); then/' "${ci_contract_test_script_fail_selftest_lock_wait_deadline_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_deadline_guard_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait deadline guard marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_deadline_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_guard_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_deadline_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_deadline_guard_marker.log" >&2
  exit 1
fi

sed -i 's/while true; do/while false; do/' "${ci_contract_test_script_fail_selftest_lock_wait_loop_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_loop_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_loop_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait loop marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_loop_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_loop_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_loop_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_loop_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_loop_marker.log" >&2
  exit 1
fi

sed -i 's/sleep 1/sleep 2/' "${ci_contract_test_script_fail_selftest_lock_wait_sleep_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_sleep_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_sleep_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait sleep marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_sleep_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_sleep_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_sleep_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_sleep_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_sleep_marker.log" >&2
  exit 1
fi

sed -i '0,/now_epoch=\"$(date +%s)\"/s//now_epoch_removed=\"$(date +%s)\"/' "${ci_contract_test_script_fail_selftest_lock_wait_now_epoch_refresh_count_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_wait_now_epoch_refresh_count_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses lock wait now_epoch refresh count marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker.log" >&2
  exit 1
fi

sed -i 's/acquire_selftest_lock()/acquire_selftest_lock_removed()/g' "${ci_contract_test_script_fail_selftest_lock_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_function_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock function marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_function_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

sed -i 's/test_check_ci_contract already running/test_check_ci_contract lock marker removed/' "${ci_contract_test_script_fail_selftest_lock_busy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock busy marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_marker.log" \
   && ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_template_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_marker (or template-marker) failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

sed -i 's/FAIL: test_check_ci_contract already running (%s, %s)/FAIL: test_check_ci_contract already running %s; %s/' "${ci_contract_test_script_fail_selftest_lock_busy_template_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_template_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_template_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock busy template marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_template_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_template_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_template_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_template_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_template_marker.log" >&2
  exit 1
fi

sed -i 's/"${lock_dir}" "${pair_fragment}"/"${lock_dir}" "${owner_pid_value:-unknown}"/' "${ci_contract_test_script_fail_selftest_lock_busy_pair_fragment_arg_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_pair_fragment_arg_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script bypasses selftest lock busy pair fragment arg marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker.log" >&2
  exit 1
fi

sed -i 's/owner_pid=%s, lock_wait_sec=%s/owner_pid_removed=%s, lock_wait_sec=%s/' "${ci_contract_test_script_fail_selftest_lock_busy_owner_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_owner_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock busy owner marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_owner_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_marker.log" \
   && ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_owner_wait_pair_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_owner_marker (or pair-marker) failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_marker.log" >&2
  exit 1
fi

sed -i 's/owner_pid=%s, lock_wait_sec=%s/owner_pid=%s, lock_wait_sec_removed=%s/' "${ci_contract_test_script_fail_selftest_lock_busy_wait_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_wait_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_wait_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock busy wait marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_wait_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_wait_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_wait_marker.log" \
   && ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_owner_wait_pair_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_wait_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_wait_marker (or pair-marker) failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_wait_marker.log" >&2
  exit 1
fi

sed -i 's/owner_pid=%s, lock_wait_sec=%s/owner_pid_removed=%s, lock_wait_sec_removed=%s/' "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_pair_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_pair_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_pair_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock busy owner/wait pair marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_pair_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_owner_wait_pair_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_pair_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_owner_wait_pair_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_pair_marker.log" >&2
  exit 1
fi

sed -i 's/owner_pid=%s, lock_wait_sec=%s/lock_wait_sec=%s, owner_pid=%s/' "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_order_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_busy_owner_wait_order_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_order_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script swaps selftest lock busy owner/wait marker order" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_order_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_busy_owner_wait_pair_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_order_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_busy_owner_wait_pair_marker failure marker (order swap case) was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_busy_owner_wait_order_marker.log" >&2
  exit 1
fi

sed -i 's/if \[\[ -f "${selftest_lock_pid}" \]\]/if [[ -f "${selftest_lock_pid_removed}" ]]/' "${ci_contract_test_script_fail_selftest_lock_missing_pid_recovery_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_selftest_lock_missing_pid_recovery_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_missing_pid_recovery_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses selftest lock missing-pid recovery marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_missing_pid_recovery_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_selftest_lock_missing_pid_recovery_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_missing_pid_recovery_marker.log"; then
  echo "FAIL: expected ci_contract_test_selftest_lock_missing_pid_recovery_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_selftest_lock_missing_pid_recovery_marker.log" >&2
  exit 1
fi

sed -i 's/bg_pids="$(jobs -pr/bg_pids_removed="$(jobs -pr/' "${ci_contract_test_script_fail_cleanup_jobs_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_cleanup_jobs_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_cleanup_jobs_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses cleanup jobs marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_jobs_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_cleanup_jobs_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_cleanup_jobs_marker.log"; then
  echo "FAIL: expected ci_contract_test_cleanup_jobs_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_jobs_marker.log" >&2
  exit 1
fi

sed -i 's/kill ${bg_pids}/kill_removed ${bg_pids}/' "${ci_contract_test_script_fail_cleanup_kill_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_cleanup_kill_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_cleanup_kill_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script misses cleanup kill marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_kill_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_cleanup_kill_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_cleanup_kill_marker.log"; then
  echo "FAIL: expected ci_contract_test_cleanup_kill_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_kill_marker.log" >&2
  exit 1
fi

sed -i 's/bg_pids="$(jobs -pr 2>\/dev\/null || true)"/__CI_BG_PIDS_LINE__/' "${ci_contract_test_script_fail_cleanup_call_order_marker}"
sed -i 's/kill ${bg_pids} 2>\/dev\/null || true/kill ${bg_pids} 2>\/dev\/null || true\n  bg_pids="$(jobs -pr 2>\/dev\/null || true)"/' "${ci_contract_test_script_fail_cleanup_call_order_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_cleanup_call_order_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_cleanup_call_order_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract cleanup call order marker is reversed" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_call_order_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_cleanup_call_order_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_cleanup_call_order_marker.log"; then
  echo "FAIL: expected ci_contract_test_cleanup_call_order_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_call_order_marker.log" >&2
  exit 1
fi

sed -i '1a pkill -P __CI_PPID__ 2>/dev/null || true' "${ci_contract_test_script_fail_cleanup_no_pkill_marker}"
sed -i 's/__CI_PPID__/$$/' "${ci_contract_test_script_fail_cleanup_no_pkill_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_pass}" "${ci_contract_test_script_fail_cleanup_no_pkill_marker}" >"${tmp_dir}/contract_fail_ci_contract_test_cleanup_no_pkill_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when ci_contract test script reintroduces pkill cleanup marker" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_no_pkill_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[ci_contract_test_cleanup_no_pkill_marker\\]=FAIL" "${tmp_dir}/contract_fail_ci_contract_test_cleanup_no_pkill_marker.log"; then
  echo "FAIL: expected ci_contract_test_cleanup_no_pkill_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_ci_contract_test_cleanup_no_pkill_marker.log" >&2
  exit 1
fi

sed -i 's/ mbd_integrator_checks//' "${makefile_fail_mbd_chain}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_mbd_chain}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_mbd_chain.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_checks no longer includes mbd_integrator_checks" >&2
  cat "${tmp_dir}/contract_fail_mbd_chain.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_checks_dep_integrator\\]=FAIL" "${tmp_dir}/contract_fail_mbd_chain.log"; then
  echo "FAIL: expected mbd_checks_dep_integrator failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_chain.log" >&2
  exit 1
fi

sed -i '/\$(MAKE) mbd_checks/d' "${makefile_fail_test_entry}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_test_entry}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_test_entry.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when test entry no longer calls mbd_checks" >&2
  cat "${tmp_dir}/contract_fail_test_entry.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_checks_in_test\\]=FAIL" "${tmp_dir}/contract_fail_test_entry.log"; then
  echo "FAIL: expected mbd_checks_in_test failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_test_entry.log" >&2
  exit 1
fi

sed -i '/^mbd_a21_regression:/d' "${makefile_fail_a21_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a21_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a21_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a21_regression target is missing" >&2
  cat "${tmp_dir}/contract_fail_a21_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a21_target\\]=FAIL" "${tmp_dir}/contract_fail_a21_target.log"; then
  echo "FAIL: expected mbd_a21_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a21_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a21_regression_test:/d' "${makefile_fail_a21_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a21_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a21_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a21_regression_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_a21_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a21_test_target\\]=FAIL" "${tmp_dir}/contract_fail_a21_test_target.log"; then
  echo "FAIL: expected mbd_a21_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a21_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_regression:/d' "${makefile_fail_a24_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_regression target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_target.log"; then
  echo "FAIL: expected mbd_a24_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_regression_test:/d' "${makefile_fail_a24_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_regression_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_test_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_test_target.log"; then
  echo "FAIL: expected mbd_a24_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_regression_full:/d' "${makefile_fail_a24_full_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_full_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_full_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_regression_full target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_full_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_full_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_target.log"; then
  echo "FAIL: expected mbd_a24_full_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_regression_full_test:/d' "${makefile_fail_a24_full_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_full_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_full_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_regression_full_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_full_test_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_target.log"; then
  echo "FAIL: expected mbd_a24_full_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_batch:/d' "${makefile_fail_a24_batch_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_batch_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_batch target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_batch_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_target.log"; then
  echo "FAIL: expected mbd_a24_batch_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_batch_test:/d' "${makefile_fail_a24_batch_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_batch_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_batch_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_batch_test_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_target.log"; then
  echo "FAIL: expected mbd_a24_batch_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_acceptance_serial:/d' "${makefile_fail_a24_acceptance_serial_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_acceptance_serial_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_acceptance_serial target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_acceptance_serial_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_target.log"; then
  echo "FAIL: expected mbd_a24_acceptance_serial_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_target.log" >&2
  exit 1
fi

sed -i '/^mbd_a24_acceptance_serial_test:/d' "${makefile_fail_a24_acceptance_serial_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_acceptance_serial_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_acceptance_serial_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_acceptance_serial_test_target\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_target.log"; then
  echo "FAIL: expected mbd_a24_acceptance_serial_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_target.log" >&2
  exit 1
fi

sed -i '/mbd_a24_acceptance_serial - Run A-24 serial acceptance/d' "${makefile_fail_a24_acceptance_serial_help}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_acceptance_serial_help}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_help.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_acceptance_serial help marker is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_help.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_acceptance_serial_help\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_help.log"; then
  echo "FAIL: expected mbd_a24_acceptance_serial_help failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_help.log" >&2
  exit 1
fi

sed -i '/mbd_a24_acceptance_serial_test - Self-test for A-24 serial acceptance wrapper/d' "${makefile_fail_a24_acceptance_serial_test_help}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_a24_acceptance_serial_test_help}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_help.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_a24_acceptance_serial_test help marker is missing" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_help.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_a24_acceptance_serial_test_help\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_help.log"; then
  echo "FAIL: expected mbd_a24_acceptance_serial_test_help failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_help.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_guard_test:/d' "${makefile_fail_b8_guard_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_guard_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_guard_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_guard_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_test_target.log"; then
  echo "FAIL: expected mbd_b8_guard_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_guard_contract:/d' "${makefile_fail_b8_guard_contract_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_guard_contract_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_guard_contract target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_guard_contract_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_target.log"; then
  echo "FAIL: expected mbd_b8_guard_contract_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_guard_contract_test:/d' "${makefile_fail_b8_guard_contract_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_guard_contract_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_guard_contract_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_guard_contract_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_target.log"; then
  echo "FAIL: expected mbd_b8_guard_contract_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_syntax:/d' "${makefile_fail_b8_syntax_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_syntax_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_syntax_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_syntax target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_syntax_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_syntax_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_syntax_target.log"; then
  echo "FAIL: expected mbd_b8_syntax_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_syntax_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_guard_output_test:/d' "${makefile_fail_b8_output_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_output_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_output_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_guard_output_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_output_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_output_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_output_test_target.log"; then
  echo "FAIL: expected mbd_b8_output_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_output_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_regression:/d' "${makefile_fail_b8_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_regression target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_target.log"; then
  echo "FAIL: expected mbd_b8_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_regression_test:/d' "${makefile_fail_b8_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_regression_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_test_target.log"; then
  echo "FAIL: expected mbd_b8_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_regression_full:/d' "${makefile_fail_b8_full_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_full_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_full_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_regression_full target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_full_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_target.log"; then
  echo "FAIL: expected mbd_b8_full_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_regression_full_test:/d' "${makefile_fail_b8_full_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_full_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_full_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_regression_full_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_full_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_target.log"; then
  echo "FAIL: expected mbd_b8_full_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_knob_matrix_test:/d' "${makefile_fail_b8_knob_matrix_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_knob_matrix_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_knob_matrix_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_knob_matrix_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_knob_matrix_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_test_target.log"; then
  echo "FAIL: expected mbd_b8_knob_matrix_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_test_target.log" >&2
  exit 1
fi

sed -i '/^mbd_b8_knob_matrix_smoke_test:/d' "${makefile_fail_b8_knob_matrix_smoke_test_target}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_knob_matrix_smoke_test_target}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_knob_matrix_smoke_test_target.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd_b8_knob_matrix_smoke_test target is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_test_target.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_knob_matrix_smoke_test_target\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_test_target.log"; then
  echo "FAIL: expected mbd_b8_knob_matrix_smoke_test_target failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_test_target.log" >&2
  exit 1
fi

sed -i '/B8_KNOB_MATRIX_SKIP_FULL=1 bash \$(MBD_B8_KNOB_MATRIX_TEST_SCRIPT)/d' "${makefile_fail_b8_knob_matrix_smoke_skip_flag}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_knob_matrix_smoke_skip_flag}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_knob_matrix_smoke_skip_flag.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when smoke target skip flag is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_skip_flag.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_knob_matrix_smoke_skip_flag\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_skip_flag.log"; then
  echo "FAIL: expected mbd_b8_knob_matrix_smoke_skip_flag failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_smoke_skip_flag.log" >&2
  exit 1
fi

sed -i '/B8_MAKE_CMD=\$(if \$(B8_MAKE_CMD),\$(B8_MAKE_CMD),make)/d' "${makefile_fail_b8_make_knob}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_make_knob}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_make_knob.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when B8_MAKE_CMD pass-through is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_make_knob.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_make_knob_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_make_knob.log"; then
  echo "FAIL: expected mbd_b8_make_knob_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_make_knob.log" >&2
  exit 1
fi

sed -i '/B8_RUN_B14_REGRESSION=\$(if \$(B8_RUN_B14_REGRESSION),\$(B8_RUN_B14_REGRESSION),1)/d' "${makefile_fail_b8_b14_knob}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_b14_knob}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_b14_knob.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when B8_RUN_B14_REGRESSION pass-through is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_knob.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_b14_knob_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_b14_knob.log"; then
  echo "FAIL: expected mbd_b8_b14_knob_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_knob.log" >&2
  exit 1
fi

sed -i 's/B8_LOCAL_TARGET=\$(if \$(B8_LOCAL_TARGET),\$(B8_LOCAL_TARGET),mbd_checks)/B8_LOCAL_TARGET=\$(if \$(B8_LOCAL_TARGET),\$(B8_LOCAL_TARGET),removed_mbd_checks)/g' "${makefile_fail_b8_local_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_fail_b8_local_target_default}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_b8_local_target_default.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when B8_LOCAL_TARGET default is not mbd_checks" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_default.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_b8_local_target_default\\]=FAIL" "${tmp_dir}/contract_fail_b8_local_target_default.log"; then
  echo "FAIL: expected mbd_b8_local_target_default failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_default.log" >&2
  exit 1
fi

sed -i 's/env -u MAKEFLAGS -u MFLAGS/removed_makeflags_isolation/' "${b8_guard_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_fail_makeflags_isolation}" >"${tmp_dir}/contract_fail_b8_guard_makeflags_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard script misses MAKEFLAGS isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_makeflags_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_makeflags_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_makeflags_isolation.log"; then
  echo "FAIL: expected b8_guard_makeflags_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_makeflags_isolation.log" >&2
  exit 1
fi

sed -i 's/local_target="${B8_LOCAL_TARGET:-mbd_checks}"/local_target="${B8_LOCAL_TARGET:-removed_mbd_checks}"/' "${b8_guard_script_fail_local_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_fail_local_target_default}" >"${tmp_dir}/contract_fail_b8_guard_local_target_default.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard local target default is not mbd_checks" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_local_target_default.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_local_target_default\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_local_target_default.log"; then
  echo "FAIL: expected b8_guard_local_target_default failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_local_target_default.log" >&2
  exit 1
fi

sed -i 's/B8_B14_TARGET=mbd_ci_contract/B8_B14_TARGET=removed_ci_contract/' "${b8_guard_contract_test_script_fail_b14_target_override}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_b14_target_override}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_b14_target_override.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard-contract self-test misses b14 target override" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_b14_target_override.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_b14_target_override\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_b14_target_override.log"; then
  echo "FAIL: expected b8_guard_contract_test_b14_target_override failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_b14_target_override.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_b14_target_override}"
sed -i 's/FEM4C_REPO_ROOT="${root_dir}"/FEM4C_REPO_ROOT=removed_root/' "${b8_guard_contract_test_script_fail_b14_target_override}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_b14_target_override}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_repo_root_passthrough_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard-contract self-test misses repo-root pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_repo_root_passthrough_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_repo_root_passthrough_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_repo_root_passthrough_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_repo_root_passthrough_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_repo_root_passthrough_marker.log" >&2
  exit 1
fi

sed -i 's/makeflags_isolation/makeflags_case_removed/' "${b8_guard_test_script_fail_makeflags_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_fail_makeflags_case_marker}" >"${tmp_dir}/contract_fail_b8_guard_test_makeflags_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard self-test misses makeflags isolation case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_test_makeflags_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_test_makeflags_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_test_makeflags_case_marker.log"; then
  echo "FAIL: expected b8_guard_test_makeflags_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_test_makeflags_case_marker.log" >&2
  exit 1
fi

sed -i '/check_fem4c_test_log_markers.sh fem4c_test.log/d' "${workflow_fail_gate_call}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_fail_gate_call}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_workflow.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when workflow misses test log gate script call" >&2
  cat "${tmp_dir}/contract_fail_workflow.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[test_log_gate_script_call\\]=FAIL" "${tmp_dir}/contract_fail_workflow.log"; then
  echo "FAIL: expected test_log_gate_script_call failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_workflow.log" >&2
  exit 1
fi

sed -i '/PASS: all MBD checks completed/d' "${marker_script_fail_mbd_suite}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_fail_mbd_suite}" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_marker_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when marker script misses MBD suite marker" >&2
  cat "${tmp_dir}/contract_fail_marker_script.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[marker_mbd_suite\\]=FAIL" "${tmp_dir}/contract_fail_marker_script.log"; then
  echo "FAIL: expected marker_mbd_suite failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_marker_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${tmp_dir}/missing_marker_script.sh" "${mbd_integrator_script_pass}" >"${tmp_dir}/contract_fail_missing_marker_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when marker script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_marker_script.log" >&2
  exit 1
fi

if ! grep -q "marker script missing" "${tmp_dir}/contract_fail_missing_marker_script.log"; then
  echo "FAIL: expected missing marker script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_marker_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${tmp_dir}/missing_run_a24_regression.sh" >"${tmp_dir}/contract_fail_missing_a24_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_script.log" >&2
  exit 1
fi

if ! grep -q "a24 regression script missing" "${tmp_dir}/contract_fail_missing_a24_script.log"; then
  echo "FAIL: expected missing a24 regression script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${tmp_dir}/missing_run_b8_regression.sh" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_missing_b8_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_script.log" >&2
  exit 1
fi

if ! grep -q "b8 regression script missing" "${tmp_dir}/contract_fail_missing_b8_script.log"; then
  echo "FAIL: expected missing b8 regression script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${tmp_dir}/missing_run_b8_regression_full.sh" >"${tmp_dir}/contract_fail_missing_b8_full_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_full_script.log" >&2
  exit 1
fi

if ! grep -q "b8 full regression script missing" "${tmp_dir}/contract_fail_missing_b8_full_script.log"; then
  echo "FAIL: expected missing b8 full regression script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_full_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${tmp_dir}/missing_test_b8_knob_matrix.sh" >"${tmp_dir}/contract_fail_missing_b8_knob_matrix_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_knob_matrix_script.log" >&2
  exit 1
fi

if ! grep -q "b8 knob matrix script missing" "${tmp_dir}/contract_fail_missing_b8_knob_matrix_script.log"; then
  echo "FAIL: expected missing b8 knob matrix script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_knob_matrix_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${tmp_dir}/missing_test_run_b8_guard.sh" >"${tmp_dir}/contract_fail_missing_b8_guard_test_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard test script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_guard_test_script.log" >&2
  exit 1
fi

if ! grep -q "b8 guard test script missing" "${tmp_dir}/contract_fail_missing_b8_guard_test_script.log"; then
  echo "FAIL: expected missing b8 guard test script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_b8_guard_test_script.log" >&2
  exit 1
fi

sed -i 's/B8_RUN_B14_REGRESSION must be 0 or 1/B8_RUN_B14_REGRESSION validation removed/' "${b8_regression_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_validation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_validation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression validation marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_validation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_knob_validation\\]=FAIL" "${tmp_dir}/contract_fail_b8_validation.log"; then
  echo "FAIL: expected b8_regression_knob_validation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_validation.log" >&2
  exit 1
fi

sed -i 's/B8_MAKE_CMD is not executable/B8_MAKE_CMD validation removed/' "${b8_regression_script_fail_make_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_make_validation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_make_validation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression make-command validation marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_make_validation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_make_cmd_validation\\]=FAIL" "${tmp_dir}/contract_fail_b8_make_validation.log"; then
  echo "FAIL: expected b8_regression_make_cmd_validation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_make_validation.log" >&2
  exit 1
fi

sed -i 's/root_dir="${FEM4C_REPO_ROOT:-}"/root_dir="${FEM4C_REPO_ROOT_REMOVED:-}"/' "${b8_regression_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_b14_target_default}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_repo_root_override_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression repo-root override marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_repo_root_override_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_repo_root_override_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_repo_root_override_marker.log"; then
  echo "FAIL: expected b8_regression_repo_root_override_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_repo_root_override_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_b14_target_default}"
sed -i 's/b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"/b8_b14_target="${B8_B14_TARGET:-removed_target}"/' "${b8_regression_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_b14_target_default}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_b14_target_default.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression b14 target default marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_default.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_b14_target_default\\]=FAIL" "${tmp_dir}/contract_fail_b8_b14_target_default.log"; then
  echo "FAIL: expected b8_regression_b14_target_default failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_default.log" >&2
  exit 1
fi

sed -i 's/B8_B14_TARGET="$b8_b14_target"/B8_B14_TARGET=removed_target/' "${b8_regression_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_b14_target_pass_through}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_b14_target_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression b14 target pass-through marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_b14_target_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_b14_target_pass_through.log"; then
  echo "FAIL: expected b8_regression_b14_target_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_pass_through.log" >&2
  exit 1
fi

sed -i 's/env -u MAKEFLAGS -u MFLAGS/removed_makeflags_isolation/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_makeflags_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses MAKEFLAGS isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_makeflags_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_makeflags_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_makeflags_isolation.log"; then
  echo "FAIL: expected b8_regression_makeflags_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_makeflags_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i '/run_make_target mbd_ci_contract/a run_make_target mbd_ci_contract_test' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_no_direct_contract_test_call.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script directly invokes mbd_ci_contract_test" >&2
  cat "${tmp_dir}/contract_fail_b8_no_direct_contract_test_call.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_no_direct_contract_test_call\\]=FAIL" "${tmp_dir}/contract_fail_b8_no_direct_contract_test_call.log"; then
  echo "FAIL: expected b8_regression_no_direct_contract_test_call failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_no_direct_contract_test_call.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/-u B8_LOCAL_TARGET/removed_local_target_isolation/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_local_target_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses B8_LOCAL_TARGET isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_local_target_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_local_target_isolation.log"; then
  echo "FAIL: expected b8_regression_local_target_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/-u B8_B14_TARGET/removed_b14_target_isolation/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_b14_target_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses B8_B14_TARGET isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_b14_target_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_b14_target_isolation.log"; then
  echo "FAIL: expected b8_regression_b14_target_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_target_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/-u B8_RUN_B14_REGRESSION/removed_b14_knob_isolation/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_b14_knob_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses B8_RUN_B14_REGRESSION isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_knob_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_b14_knob_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_b14_knob_isolation.log"; then
  echo "FAIL: expected b8_regression_b14_knob_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_b14_knob_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_b14_target_pass_through}"
sed -i 's/B8_LOCAL_TARGET="\$b8_local_target"/B8_LOCAL_TARGET=removed_local_target/' "${b8_regression_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_b14_target_pass_through}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_local_target_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses B8_LOCAL_TARGET pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_local_target_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_local_target_pass_through.log"; then
  echo "FAIL: expected b8_regression_local_target_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_b14_target_pass_through}"
sed -i 's/local_target=\$b8_local_target/local_target=removed_local_target_trace/' "${b8_regression_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_b14_target_pass_through}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_local_target_summary_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses local_target summary trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_summary_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_local_target_summary_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_local_target_summary_trace_marker.log"; then
  echo "FAIL: expected b8_regression_local_target_summary_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_local_target_summary_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"/b8_skip_lock="${B8_REGRESSION_SKIP_LOCK_REMOVED:-0}"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_skip_lock_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses skip-lock knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_skip_lock_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_skip_lock_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_skip_lock_knob_marker.log"; then
  echo "FAIL: expected b8_regression_skip_lock_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_skip_lock_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/B8_REGRESSION_SKIP_LOCK must be 0 or 1/B8_REGRESSION_SKIP_LOCK validation removed/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_skip_lock_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses skip-lock validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_skip_lock_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_skip_lock_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_skip_lock_validation_marker.log"; then
  echo "FAIL: expected b8_regression_skip_lock_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_skip_lock_validation_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"/b8_lock_scope="${B8_REGRESSION_LOCK_SCOPE_REMOVED:-repo}"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_scope_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-scope knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_scope_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_scope_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_scope_knob_marker.log"; then
  echo "FAIL: expected b8_regression_lock_scope_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_scope_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE must be repo or global/B8_REGRESSION_LOCK_SCOPE validation removed/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_scope_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-scope validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_scope_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_scope_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_scope_validation_marker.log"; then
  echo "FAIL: expected b8_regression_lock_scope_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_scope_validation_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's#b8_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"#b8_lock_dir_default_global="/tmp/removed_fem4c_b8_regression.lock"#' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_default_global_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_default_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_default_global_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_default_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_lock_dir_source="env"/b8_lock_dir_source="removed_env"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_source_default_env_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir source default-env marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_default_env_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_source_default_env_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_source_default_env_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_source_default_env_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_default_env_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/| cksum | awk '\''{print $1}'\''/| removed_cksum/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_repo_hash_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses repo-hash lock marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_repo_hash_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_repo_hash_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_repo_hash_marker.log"; then
  echo "FAIL: expected b8_regression_lock_repo_hash_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_repo_hash_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's#b8_lock_dir="/tmp/fem4c_b8_regression.\${b8_lock_repo_hash}.lock"#b8_lock_dir="/tmp/removed_fem4c_b8_regression.\${b8_lock_repo_hash}.lock"#' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_default_repo_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses repo-scoped lock-dir default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_default_repo_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_default_repo_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_default_repo_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_default_repo_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_default_repo_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_lock_dir_source="scope_repo_default"/b8_lock_dir_source="removed_repo_default"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_source_repo_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir source repo-default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_repo_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_source_repo_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_source_repo_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_source_repo_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_repo_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_lock_dir_source="scope_global_default"/b8_lock_dir_source="removed_global_default"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_source_global_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir source global-default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_global_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_source_global_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_source_global_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_source_global_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_global_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/b8_lock_pid_file="${b8_lock_dir}\/pid"/b8_lock_pid_file="${b8_lock_dir}\/removed_pid"/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_pid_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock pid marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_pid_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_pid_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_pid_marker.log"; then
  echo "FAIL: expected b8_regression_lock_pid_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_pid_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/FAIL: b8 regression lock is already held/FAIL: b8 regression lock marker removed/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_fail_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock held fail marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_fail_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_fail_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_fail_marker.log"; then
  echo "FAIL: expected b8_regression_lock_fail_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_fail_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/INFO: recovered stale b8 regression lock/INFO: stale lock recovery marker removed/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_stale_recovery_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses stale-lock recovery marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_stale_recovery_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_stale_recovery_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_stale_recovery_marker.log"; then
  echo "FAIL: expected b8_regression_lock_stale_recovery_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_stale_recovery_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/lock_dir=\$b8_lock_dir/lock_dir=removed_lock_dir/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression.sh" "${b8_regression_script_fail_makeflags_isolation}"
sed -i 's/lock_dir_source=\$b8_lock_dir_source/lock_dir_source=removed_source/' "${b8_regression_script_fail_makeflags_isolation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_fail_makeflags_isolation}" "${b8_regression_full_script_pass}" >"${tmp_dir}/contract_fail_b8_lock_dir_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression script misses lock-dir source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_lock_dir_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_lock_dir_source_trace_marker.log"; then
  echo "FAIL: expected b8_regression_lock_dir_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

sed -i 's/B8_RUN_B14_REGRESSION must be 0 or 1/B8_RUN_B14_REGRESSION validation removed/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_validation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression validation marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_validation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_knob_validation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_validation.log"; then
  echo "FAIL: expected b8_full_regression_knob_validation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_validation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/b8_regression_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"/b8_regression_skip_lock="${B8_REGRESSION_SKIP_LOCK_REMOVED:-0}"/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_skip_lock_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses skip-lock knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_skip_lock_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_skip_lock_knob_marker.log"; then
  echo "FAIL: expected b8_full_regression_skip_lock_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/B8_REGRESSION_SKIP_LOCK must be 0 or 1/B8_REGRESSION_SKIP_LOCK validation removed/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_skip_lock_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses skip-lock validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_skip_lock_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_skip_lock_validation_marker.log"; then
  echo "FAIL: expected b8_full_regression_skip_lock_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_validation_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/b8_regression_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"/b8_regression_lock_scope="${B8_REGRESSION_LOCK_SCOPE_REMOVED:-repo}"/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_scope_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-scope knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_scope_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_scope_knob_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_scope_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE must be repo or global/B8_REGRESSION_LOCK_SCOPE validation removed/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_scope_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-scope validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_scope_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_scope_validation_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_scope_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_validation_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's#b8_regression_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"#b8_regression_lock_dir_default_global="/tmp/removed_fem4c_b8_regression.lock"#' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_default_global_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_default_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_default_global_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_default_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/b8_regression_lock_dir_source="env"/b8_regression_lock_dir_source="removed_env"/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_source_default_env_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir source default-env marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_default_env_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_source_default_env_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_source_default_env_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_source_default_env_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_default_env_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's#b8_lock_repo_hash="\$(printf '\''%s\\n'\'' "\$root_dir" | cksum | awk '\''{print \$1}'\'')"#b8_lock_repo_hash="removed_hash"#' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_repo_hash_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses repo-hash lock marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_repo_hash_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_repo_hash_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_repo_hash_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_repo_hash_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_repo_hash_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's#b8_regression_lock_dir="/tmp/fem4c_b8_regression.\${b8_lock_repo_hash}.lock"#b8_regression_lock_dir="/tmp/removed_fem4c_b8_regression.\${b8_lock_repo_hash}.lock"#' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_default_repo_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses repo-scoped lock-dir default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_default_repo_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_default_repo_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_default_repo_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_default_repo_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_default_repo_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/b8_regression_lock_dir_source="scope_repo_default"/b8_regression_lock_dir_source="removed_repo_default"/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_source_repo_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir source repo-default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_repo_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_source_repo_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_source_repo_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_source_repo_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_repo_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_validation}"
sed -i 's/b8_regression_lock_dir_source="scope_global_default"/b8_regression_lock_dir_source="removed_global_default"/' "${b8_regression_full_script_fail_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_validation}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_source_global_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir source global-default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_global_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_source_global_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_source_global_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_source_global_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_global_marker.log" >&2
  exit 1
fi

sed -i 's/B8_MAKE_CMD is not executable/B8_MAKE_CMD validation removed/' "${b8_regression_full_script_fail_make_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_make_validation}" >"${tmp_dir}/contract_fail_b8_full_make_validation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression make-command validation marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_validation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_cmd_validation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_validation.log"; then
  echo "FAIL: expected b8_full_regression_make_cmd_validation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_validation.log" >&2
  exit 1
fi

sed -i 's/root_dir="${FEM4C_REPO_ROOT:-}"/root_dir="${FEM4C_REPO_ROOT_REMOVED:-}"/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_repo_root_override_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression repo-root override marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_repo_root_override_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_repo_root_override_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_repo_root_override_marker.log"; then
  echo "FAIL: expected b8_full_regression_repo_root_override_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_repo_root_override_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
sed -i 's/b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"/b8_b14_target="${B8_B14_TARGET:-removed_target}"/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_b14_target_default.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression b14 target default marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_b14_target_default.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_b14_target_default\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_b14_target_default.log"; then
  echo "FAIL: expected b8_full_regression_b14_target_default failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_b14_target_default.log" >&2
  exit 1
fi

sed -i 's/B8_B14_TARGET="$b8_b14_target"/B8_B14_TARGET=removed_target/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_b14_target_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression b14 target pass-through marker is missing" >&2
  cat "${tmp_dir}/contract_fail_b8_full_b14_target_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_b14_target_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_b14_target_pass_through.log"; then
  echo "FAIL: expected b8_full_regression_b14_target_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_b14_target_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
sed -i 's/-u B8_LOCAL_TARGET/removed_local_target_isolation/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_local_target_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses B8_LOCAL_TARGET isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_local_target_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_local_target_isolation.log"; then
  echo "FAIL: expected b8_full_regression_local_target_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/B8_LOCAL_TARGET="\$b8_local_target"/B8_LOCAL_TARGET=removed_local_target/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_local_target_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses B8_LOCAL_TARGET pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_local_target_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_local_target_pass_through.log"; then
  echo "FAIL: expected b8_full_regression_local_target_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/local_target=\$b8_local_target/local_target=removed_local_target_trace/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_local_target_summary_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses local_target summary trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_summary_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_local_target_summary_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_local_target_summary_trace_marker.log"; then
  echo "FAIL: expected b8_full_regression_local_target_summary_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_local_target_summary_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
sed -i 's/-u B8_REGRESSION_SKIP_LOCK/removed_skip_lock_isolation/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_skip_lock_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses skip-lock isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_skip_lock_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_skip_lock_isolation.log"; then
  echo "FAIL: expected b8_full_regression_skip_lock_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
sed -i 's/-u B8_REGRESSION_LOCK_SCOPE/removed_lock_scope_isolation/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_lock_scope_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-scope isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_scope_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_scope_isolation.log"; then
  echo "FAIL: expected b8_full_regression_lock_scope_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_default}"
sed -i 's/-u B8_REGRESSION_LOCK_DIR/removed_lock_dir_isolation/' "${b8_regression_full_script_fail_b14_target_default}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_default}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_isolation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir isolation" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_isolation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_isolation\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_isolation.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_isolation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_isolation.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/B8_REGRESSION_SKIP_LOCK="\$b8_regression_skip_lock"/B8_REGRESSION_SKIP_LOCK=removed_skip_lock/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_skip_lock_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses skip-lock pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_skip_lock_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_skip_lock_pass_through.log"; then
  echo "FAIL: expected b8_full_regression_skip_lock_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_skip_lock_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE="\$b8_regression_lock_scope"/B8_REGRESSION_LOCK_SCOPE=removed_lock_scope/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_lock_scope_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-scope pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_scope_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_scope_pass_through.log"; then
  echo "FAIL: expected b8_full_regression_lock_scope_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_scope_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/B8_REGRESSION_LOCK_DIR="\$b8_regression_lock_dir"/B8_REGRESSION_LOCK_DIR=removed_lock_dir/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_pass_through.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir pass-through marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_pass_through.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_pass_through\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_pass_through.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_pass_through failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_pass_through.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/b8_lock_dir_source=\$b8_regression_lock_dir_source/b8_lock_dir_source=removed_source/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_lock_dir_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses lock-dir source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_lock_dir_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_lock_dir_source_trace_marker.log"; then
  echo "FAIL: expected b8_full_regression_lock_dir_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/b8_test_retry_used=0/b8_test_retry_used=2/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_used_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-used default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_used_default_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_used_default_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_used_default_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_default_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/b8_test_retry_reason="none"/b8_test_retry_reason="removed_none"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-reason default marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_reason_default_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_default_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_reason_default_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_default_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/run_test_with_parser_retry() {/run_test_with_parser_retry_removed() {/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_fn_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses test-retry function marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_fn_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_fn_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_fn_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_fn_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_fn_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/b8_test_retry_used=1/b8_test_retry_used_removed=1/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_used_set_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-used set marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_set_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_used_set_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_used_set_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_used_set_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_set_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/b8_test_retry_reason="parser_missing"/b8_test_retry_reason="removed_parser"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_set_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-reason set marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_set_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_reason_set_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_set_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_reason_set_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_set_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/parser executable missing after test failure; rebuilding via make all and retrying test once/parser retry marker removed/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_parser_missing_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses parser-missing retry marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_parser_missing_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_parser_missing_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_parser_missing_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_parser_missing_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_parser_missing_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/^[[:space:]]*run_test_with_parser_retry[[:space:]]*$/run_test_with_parser_retry_removed/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses test-retry call marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_call_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_call_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/"\$b8_test_retry_reason" != "none"/"$b8_test_retry_reason" != "removed_none"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_zero_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-reason consistency-zero marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_zero_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_reason_consistency_zero_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_zero_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_reason_consistency_zero_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_zero_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/"\$b8_test_retry_reason" != "parser_missing"/"$b8_test_retry_reason" != "removed_parser"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_one_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-reason consistency-one marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_one_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_reason_consistency_one_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_one_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_reason_consistency_one_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_consistency_one_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/test_retry_used=\$b8_test_retry_used/test_retry_used=removed_retry/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_used_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-used summary marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_used_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_used_summary_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_used_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_used_summary_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/test_retry_reason=\$b8_test_retry_reason/test_retry_reason=removed_reason/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses retry-reason summary marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_test_retry_reason_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_summary_marker.log"; then
  echo "FAIL: expected b8_full_regression_test_retry_reason_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_summary_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/"\$b8_make_cmd" -j1 -C FEM4C "\$target"/"\$b8_make_cmd" -C FEM4C "\$target"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_target_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses serial make target marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_target_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_target_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_target_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i '0,/"\$b8_make_cmd" -j1 -C FEM4C "\$target"/s//"$b8_make_cmd" -C FEM4C "$target"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_target_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script serial make target count is not exactly two" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_target_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_target_count_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_target_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_count_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i '0,/"\$b8_make_cmd" -j1 -C FEM4C "\$target"/s//&\
    "$b8_make_cmd" -j1 -C FEM4C "$target"/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_target_over_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script has more than two serial make target markers" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_over_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_target_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_target_over_count_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_target_count_marker over-count failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_target_over_count_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i 's/"\$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression/"\$b8_make_cmd" -C FEM4C mbd_b8_regression/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_b8_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script misses serial make b8 marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_b8_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_b8_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_b8_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i '0,/"\$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression/s//"$b8_make_cmd" -C FEM4C mbd_b8_regression/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_b8_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script serial make b8-marker count is not exactly two" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_b8_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_b8_count_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_b8_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_count_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/run_b8_regression_full.sh" "${b8_regression_full_script_fail_b14_target_pass_through}"
sed -i '0,/"\$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression/s//&\
      "$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression/' "${b8_regression_full_script_fail_b14_target_pass_through}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_fail_b14_target_pass_through}" >"${tmp_dir}/contract_fail_b8_full_make_serial_b8_over_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression script has more than two serial make b8 markers" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_over_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_regression_make_serial_b8_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_make_serial_b8_over_count_marker.log"; then
  echo "FAIL: expected b8_full_regression_make_serial_b8_count_marker over-count failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_make_serial_b8_over_count_marker.log" >&2
  exit 1
fi

sed -i '/script_copy_dir=/c\script_copy_dir="removed_copy_dir"' "${b8_guard_contract_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_temp_copy_marker}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard contract test misses temp-copy dir knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_temp_copy_dir_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_knob_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_temp_copy_dir_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR does not exist/B8_TEST_TMP_COPY_DIR validation removed/' "${b8_guard_contract_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_temp_copy_marker}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_validate_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard contract test misses temp-copy dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_temp_copy_dir_validate_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_validate_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_temp_copy_dir_validate_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR is not writable/B8_TEST_TMP_COPY_DIR writable validation removed/' "${b8_guard_contract_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_temp_copy_marker}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_writable_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard contract test misses temp-copy dir writable marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_temp_copy_dir_writable_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_writable_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_temp_copy_dir_writable_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_temp_copy_marker}"
sed -i 's/temp_copy_stamp="\$\$\.\${RANDOM}"/temp_copy_stamp="removed_stamp"/' "${b8_guard_contract_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_temp_copy_marker}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_stamp_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard contract test misses temp-copy stamp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_temp_copy_stamp_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_stamp_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_temp_copy_stamp_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_guard_contract.sh" "${b8_guard_contract_test_script_fail_temp_copy_marker}"
sed -i 's#tmp_run_b8_guard_contract_fail\.[^"]*\.sh#tmp_run_b8_guard_contract_fail.REMOVED.sh#' "${b8_guard_contract_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_fail_temp_copy_marker}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 guard contract test misses temp-copy mktemp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_guard_contract_test_temp_copy_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_marker.log"; then
  echo "FAIL: expected b8_guard_contract_test_temp_copy_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_guard_contract_test_temp_copy_marker.log" >&2
  exit 1
fi

sed -i '/script_copy_dir=/c\script_copy_dir="removed_copy_dir"' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses temp-copy dir knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_temp_copy_dir_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_knob_marker.log"; then
  echo "FAIL: expected b8_regression_test_temp_copy_dir_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR does not exist/B8_TEST_TMP_COPY_DIR validation removed/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_validate_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses temp-copy dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_temp_copy_dir_validate_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_validate_marker.log"; then
  echo "FAIL: expected b8_regression_test_temp_copy_dir_validate_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR is not writable/B8_TEST_TMP_COPY_DIR writable validation removed/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_writable_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses temp-copy dir writable marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_temp_copy_dir_writable_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_writable_marker.log"; then
  echo "FAIL: expected b8_regression_test_temp_copy_dir_writable_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_B14_TARGET=mbd_b8_syntax/B8_B14_TARGET=removed_b14_target/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_b14_target_override_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses b14 target override case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_b14_target_override_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_b14_target_override_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_b14_target_override_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_b14_target_override_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_b14_target_override_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_MAKE_CALL_LOG="\${mock_make_call_log}"/B8_MAKE_CALL_LOG=removed_call_log/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_make_call_log_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses make call log marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_make_call_log_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_make_call_log_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_make_call_log_marker.log"; then
  echo "FAIL: expected b8_regression_test_make_call_log_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_make_call_log_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/run_b8_regression should not invoke mbd_ci_contract_test directly/no-direct-contract-test marker removed/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_no_direct_contract_test_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses no-direct-contract-test marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_no_direct_contract_test_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_no_direct_contract_test_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_no_direct_contract_test_marker.log"; then
  echo "FAIL: expected b8_regression_test_no_direct_contract_test_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_no_direct_contract_test_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#default_lock_dir="${tmp_dir}/b8_regression_test.lock"#default_lock_dir="${tmp_dir}/removed_b8_regression_test.lock"#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_default_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses default lock-dir marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_default_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_default_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_default_lock_dir_marker.log"; then
  echo "FAIL: expected b8_regression_test_default_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_default_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/export B8_REGRESSION_LOCK_DIR="${default_lock_dir}"/export B8_REGRESSION_LOCK_DIR="${removed_default_lock_dir}"/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_export_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses lock-dir export marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_export_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_export_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_export_lock_dir_marker.log"; then
  echo "FAIL: expected b8_regression_test_export_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_export_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_scope=repo" "${tmp_dir}/pass.log#lock_scope=repo" "${tmp_dir}/pass_removed.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=env" "${tmp_dir}/pass.log#lock_dir_source=removed_env" "${tmp_dir}/pass.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_env_lock_dir_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses env lock-dir source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_env_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_env_lock_dir_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_env_lock_dir_source_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_env_lock_dir_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_env_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE=cluster/B8_REGRESSION_LOCK_SCOPE=removed_cluster/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_invalid_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses invalid lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_invalid_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_invalid_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_invalid_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_invalid_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_invalid_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE=global/B8_REGRESSION_LOCK_SCOPE=removed_global/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_scope=global" "${tmp_dir}/global_lock_scope.log#lock_scope=global" "${tmp_dir}/global_lock_scope_removed.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=env" "${tmp_dir}/global_lock_scope.log#lock_dir_source=removed_env" "${tmp_dir}/global_lock_scope.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_env_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global lock-scope env-source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_env_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_lock_scope_env_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_env_source_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_lock_scope_env_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_lock_scope_env_source_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#repo_default_lock_dir="/tmp/fem4c_b8_regression.\${repo_lock_hash}.lock"#repo_default_lock_dir="/tmp/removed_fem4c_b8_regression.\${repo_lock_hash}.lock"#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses repo-default lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_repo_default_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_repo_default_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log#lock_dir=removed_repo_default" "${tmp_dir}/repo_default_lock_scope.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses repo-default lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_repo_default_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_repo_default_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log#lock_dir_source=removed_repo_default" "${tmp_dir}/repo_default_lock_scope.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses repo-default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_repo_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_repo_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_repo_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#global_default_lock_dir="/tmp/fem4c_b8_regression.lock"#global_default_lock_dir="/tmp/removed_fem4c_b8_regression.lock"#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global-default lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_default_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_default_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log#lock_dir=removed_global_default" "${tmp_dir}/global_default_lock_scope.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global-default lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_default_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_default_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log#lock_dir_source=removed_global_default" "${tmp_dir}/global_default_lock_scope.log#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses global-default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_global_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_regression_test_global_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_global_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_SKIP_LOCK=2/B8_REGRESSION_SKIP_LOCK=removed_invalid_skip/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_invalid_skip_lock_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses invalid skip-lock case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_invalid_skip_lock_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_invalid_skip_lock_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_invalid_skip_lock_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_invalid_skip_lock_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_invalid_skip_lock_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/held_lock_dir="${tmp_dir}\/lock_held"/held_lock_dir="${tmp_dir}\/removed_lock_held"/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_lock_held_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses lock-held case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_lock_held_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_lock_held_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_lock_held_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_lock_held_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_lock_held_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_SKIP_LOCK=1/B8_REGRESSION_SKIP_LOCK=removed_skip/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_skip_lock_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses skip-lock case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_skip_lock_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_skip_lock_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_skip_lock_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_skip_lock_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_skip_lock_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/stale_lock_dir="${tmp_dir}\/stale_lock"/stale_lock_dir="${tmp_dir}\/removed_stale_lock"/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_stale_lock_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses stale-lock case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_stale_lock_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_stale_lock_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_stale_lock_case_marker.log"; then
  echo "FAIL: expected b8_regression_test_stale_lock_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_stale_lock_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's/temp_copy_stamp="\$\$\.\${RANDOM}"/temp_copy_stamp="removed_stamp"/' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_temp_copy_stamp_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses temp-copy stamp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_temp_copy_stamp_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_stamp_marker.log"; then
  echo "FAIL: expected b8_regression_test_temp_copy_stamp_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression.sh" "${b8_regression_test_script_fail_temp_copy_marker}"
sed -i 's#mktemp "${script_copy_dir}/\.tmp_run_b8_regression_fail\.#mktemp "${script_copy_dir}/.removed_run_b8_regression_fail.#' "${b8_regression_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_fail_temp_copy_marker}" "${b8_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_b8_regression_test_temp_copy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 regression test misses temp-copy mktemp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_regression_test_temp_copy_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_marker.log"; then
  echo "FAIL: expected b8_regression_test_temp_copy_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_regression_test_temp_copy_marker.log" >&2
  exit 1
fi

sed -i '/script_copy_dir=/c\script_copy_dir="removed_copy_dir"' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses temp-copy dir knob marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_temp_copy_dir_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_knob_marker.log"; then
  echo "FAIL: expected b8_full_test_temp_copy_dir_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_knob_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR does not exist/B8_TEST_TMP_COPY_DIR validation removed/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_validate_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses temp-copy dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_temp_copy_dir_validate_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_validate_marker.log"; then
  echo "FAIL: expected b8_full_test_temp_copy_dir_validate_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_validate_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_TEST_TMP_COPY_DIR is not writable/B8_TEST_TMP_COPY_DIR writable validation removed/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_writable_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses temp-copy dir writable marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_temp_copy_dir_writable_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_writable_marker.log"; then
  echo "FAIL: expected b8_full_test_temp_copy_dir_writable_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_dir_writable_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_RUN_B14_REGRESSION=0/B8_RUN_B14_REGRESSION=3/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_b14_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-b14 case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_b14_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_b14_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_b14_case_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_b14_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_b14_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#full_default_lock_dir="${tmp_dir}/b8_regression_full_test.lock"#full_default_lock_dir="${tmp_dir}/removed_b8_regression_full_test.lock"#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses default lock-dir marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_default_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_marker.log"; then
  echo "FAIL: expected b8_full_test_default_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/export B8_REGRESSION_LOCK_DIR="${full_default_lock_dir}"/export B8_REGRESSION_LOCK_DIR="${removed_full_default_lock_dir}"/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_export_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses lock-dir export marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_export_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_export_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_export_lock_dir_marker.log"; then
  echo "FAIL: expected b8_full_test_export_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_export_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_scope=repo" "${tmp_dir}/pass.log#b8_lock_scope=repo" "${tmp_dir}/pass_removed.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir_source=env" "${tmp_dir}/pass.log#b8_lock_dir_source=removed_env" "${tmp_dir}/pass.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_env_lock_dir_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses env lock-dir source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_env_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_env_lock_dir_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_env_lock_dir_source_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_env_lock_dir_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_env_lock_dir_source_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir=${full_default_lock_dir}" "${tmp_dir}/pass.log#b8_lock_dir=removed_full_default_lock_dir" "${tmp_dir}/pass.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses default lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_default_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_default_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE=cluster/B8_REGRESSION_LOCK_SCOPE=removed_cluster/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_invalid_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses invalid lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_invalid_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_invalid_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_invalid_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_full_test_invalid_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_invalid_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_SKIP_LOCK=2/B8_REGRESSION_SKIP_LOCK=removed_invalid_skip/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_invalid_skip_lock_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses invalid skip-lock case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_invalid_skip_lock_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_invalid_skip_lock_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_invalid_skip_lock_case_marker.log"; then
  echo "FAIL: expected b8_full_test_invalid_skip_lock_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_invalid_skip_lock_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_SKIP_LOCK=1/B8_REGRESSION_SKIP_LOCK=removed_skip_lock/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_lock_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-lock case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_lock_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_lock_case_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_lock_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_REGRESSION_LOCK_SCOPE=global/B8_REGRESSION_LOCK_SCOPE=removed_global/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_full_test_global_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir_source=env" "${tmp_dir}/skip_b14.log#b8_lock_dir_source=removed_env" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_env_source_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global lock-scope env-source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_env_source_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_lock_scope_env_source_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_env_source_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_global_lock_scope_env_source_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_env_source_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#repo_default_lock_dir="/tmp/fem4c_b8_regression.\${repo_lock_hash}.lock"#repo_default_lock_dir="/tmp/removed_fem4c_b8_regression.\${repo_lock_hash}.lock"#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses repo-default lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_repo_default_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_full_test_repo_default_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log#b8_lock_dir=removed_repo_default" "${tmp_dir}/repo_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses repo-default lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_repo_default_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_repo_default_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log#lock_dir_source=removed_repo_default" "${tmp_dir}/repo_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses repo-default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_repo_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_repo_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#test_retry_reason=" "${tmp_dir}/repo_default_lock_scope.log#test_retry_reason_removed=" "${tmp_dir}/repo_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_retry_reason_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses repo-default lock retry-reason trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_repo_default_lock_retry_reason_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_retry_reason_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_repo_default_lock_retry_reason_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_repo_default_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#global_default_lock_dir="/tmp/fem4c_b8_regression.lock"#global_default_lock_dir="/tmp/removed_fem4c_b8_regression.lock"#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global-default lock-scope case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_default_lock_scope_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_case_marker.log"; then
  echo "FAIL: expected b8_full_test_global_default_lock_scope_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log#b8_lock_dir=removed_global_default" "${tmp_dir}/global_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_default_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global-default lock-dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_default_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_global_default_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log#lock_dir_source=removed_global_default" "${tmp_dir}/global_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global-default lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_default_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_global_default_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#test_retry_reason=" "${tmp_dir}/global_default_lock_scope.log#test_retry_reason_removed=" "${tmp_dir}/global_default_lock_scope.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_default_lock_retry_reason_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global-default lock retry-reason trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_default_lock_retry_reason_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_retry_reason_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_global_default_lock_retry_reason_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_default_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/full_skip_lock_dir="${tmp_dir}\/full_skip_lock.lock"/full_skip_lock_dir="${tmp_dir}\/removed_skip_lock.lock"/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-lock dir case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_lock_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_case_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_lock_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/B8_B14_TARGET=mbd_b8_syntax/B8_B14_TARGET=removed_b14_target/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_b14_target_override_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses b14 target override case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_b14_target_override_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_b14_target_override_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_b14_target_override_case_marker.log"; then
  echo "FAIL: expected b8_full_test_b14_target_override_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_b14_target_override_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#run_b14_regression=1" "${tmp_dir}/pass.log#run_b14_regression=1" "${tmp_dir}/pass_removed.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_baseline_run_b14_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses baseline run_b14 trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_baseline_run_b14_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_baseline_run_b14_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_baseline_run_b14_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_baseline_run_b14_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_baseline_run_b14_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#run_b14_regression=1" "${tmp_dir}/override_b14_target.log#run_b14_regression=1" "${tmp_dir}/override_removed.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_override_run_b14_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses override run_b14 trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_override_run_b14_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_override_run_b14_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_override_run_b14_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_override_run_b14_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_override_run_b14_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b14_target=mbd_ci_contract" "${tmp_dir}/skip_b14.log#b14_target=removed_target" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_b14_target_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-b14 target trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_b14_target_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_b14_target_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_b14_target_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_b14_target_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_b14_target_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_skip_lock=1" "${tmp_dir}/skip_b14.log#b8_skip_lock=removed" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_lock_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-lock trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_lock_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_lock_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_lock_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_scope=global" "${tmp_dir}/skip_b14.log#b8_lock_scope=removed_global" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses global lock-scope trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_global_lock_scope_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_global_lock_scope_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_global_lock_scope_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#b8_lock_dir=${full_skip_lock_dir}" "${tmp_dir}/skip_b14.log#b8_lock_dir=removed_lock_dir" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-lock dir trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_lock_dir_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_lock_dir_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_dir_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#test_retry_reason=" "${tmp_dir}/skip_b14.log#test_retry_reason_removed=" "${tmp_dir}/skip_b14.log#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_skip_lock_retry_reason_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses skip-lock retry-reason trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_skip_lock_retry_reason_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_skip_lock_retry_reason_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_skip_lock_retry_reason_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_skip_lock_retry_reason_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/test_retry_used=0/test_retry_used=removed_zero/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_zero_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry-used=0 trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_zero_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_zero_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_zero_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_zero_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_zero_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#retry_make_dir="${tmp_dir}/retry_make_dir"#retry_make_dir="${tmp_dir}/removed_retry_make_dir"#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_make_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry-make case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_make_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_make_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_make_case_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_make_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_make_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#parser executable missing after test failure; rebuilding via make all and retrying test once#retry_trace_removed#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/test_retry_used=1/test_retry_used=removed_one/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_one_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry-used=1 trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_one_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_one_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_one_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_one_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_one_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/test_retry_reason=none/test_retry_reason=removed_none/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_none_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry-reason=none trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_none_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_reason_none_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_none_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_reason_none_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_none_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/test_retry_reason=parser_missing/test_retry_reason=removed_parser/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_reason_parser_missing_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry-reason=parser_missing trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_parser_missing_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_reason_parser_missing_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_reason_parser_missing_trace_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_reason_parser_missing_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_reason_parser_missing_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i "s/rg -n --fixed-strings -- ' test' \"\${retry_call_log}\"/rg -n --fixed-strings -- ' removed_test' \"\${retry_call_log}\"/" "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_retry_test_call_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses retry test-call count marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_test_call_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_retry_test_call_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_retry_test_call_count_marker.log"; then
  echo "FAIL: expected b8_full_test_retry_test_call_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_retry_test_call_count_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's/temp_copy_stamp="\$\$\.\${RANDOM}"/temp_copy_stamp="removed_stamp"/' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_temp_copy_stamp_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses temp-copy stamp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_temp_copy_stamp_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_temp_copy_stamp_marker.log"; then
  echo "FAIL: expected b8_full_test_temp_copy_stamp_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_stamp_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_run_b8_regression_full.sh" "${b8_regression_full_test_script_fail_temp_copy_marker}"
sed -i 's#tmp_run_b8_regression_full_fail\.[^"]*\.sh#tmp_run_b8_regression_full_fail.REMOVED.sh#' "${b8_regression_full_test_script_fail_temp_copy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_fail_temp_copy_marker}" >"${tmp_dir}/contract_fail_b8_full_test_temp_copy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 full regression test misses temp-copy mktemp marker" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_full_test_temp_copy_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_full_test_temp_copy_marker.log"; then
  echo "FAIL: expected b8_full_test_temp_copy_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_full_test_temp_copy_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_SUMMARY lock=/A24_ACCEPT_SERIAL_SUMMARY_REMOVED lock=/' "${a24_acceptance_serial_script_fail_summary_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_summary_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses summary marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_marker.log" >&2
  exit 1
fi

sed -i 's/retry_on_137="${A24_ACCEPT_SERIAL_RETRY_ON_137:-1}"/retry_on_137_removed=1/' "${a24_acceptance_serial_script_fail_retry_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_retry_knob_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_retry_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses retry knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_retry_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_knob_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_retry_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_RETRY_ON_137 must be 0 or 1/A24_ACCEPT_SERIAL_RETRY_ON_137 validation removed/' "${a24_acceptance_serial_script_fail_retry_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_retry_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_retry_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses retry validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_retry_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_retry_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_retry_validation_marker.log" >&2
  exit 1
fi

sed -i 's/fake_137_step="${A24_ACCEPT_SERIAL_FAKE_137_STEP:-none}"/fake_137_step_removed=none/' "${a24_acceptance_serial_script_fail_fake_step_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_fake_step_knob_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses fake-step knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_fake_137_step_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_knob_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_fake_137_step_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_FAKE_137_STEP must be one of/A24_ACCEPT_SERIAL_FAKE_137_STEP validation removed/' "${a24_acceptance_serial_script_fail_fake_step_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_fake_step_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses fake-step validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_fake_137_step_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_fake_137_step_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_fake_step_validation_marker.log" >&2
  exit 1
fi

sed -i 's/step_log_dir="${A24_ACCEPT_SERIAL_STEP_LOG_DIR:-}"/step_log_dir_removed=/' "${a24_acceptance_serial_script_fail_step_log_dir_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_step_log_dir_knob_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses step-log dir knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_step_log_dir_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_knob_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_step_log_dir_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_knob_marker.log" >&2
  exit 1
fi

sed -i 's/cannot create A24 acceptance serial step-log dir/step-log dir validation removed/' "${a24_acceptance_serial_script_fail_step_log_dir_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_step_log_dir_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses step-log dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_step_log_dir_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_step_log_dir_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 acceptance serial step-log dir must be a directory/step-log dir type validation removed/' "${a24_acceptance_serial_script_fail_step_log_dir_type_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_step_log_dir_type_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_type_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses step-log dir type validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_type_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_step_log_dir_type_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_type_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_step_log_dir_type_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_type_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 acceptance serial step-log dir is not writable/step-log dir writable validation removed/' "${a24_acceptance_serial_script_fail_step_log_dir_writable_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_step_log_dir_writable_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_writable_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses step-log dir writable validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_writable_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_step_log_dir_writable_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_writable_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_step_log_dir_writable_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_step_log_dir_writable_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 acceptance serial summary output directory does not exist/summary output directory validation removed/' "${a24_acceptance_serial_script_fail_summary_out_dir_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_summary_out_dir_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_dir_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses summary output directory validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_summary_out_dir_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_dir_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_summary_out_dir_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 acceptance serial summary output path must be a file/summary output path-type validation removed/' "${a24_acceptance_serial_script_fail_summary_out_type_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_summary_out_type_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_type_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses summary output path-type validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_type_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_summary_out_type_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_type_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_summary_out_type_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_type_validation_marker.log" >&2
  exit 1
fi

sed -i 's/cannot write A24 acceptance serial summary output/summary output write validation removed/' "${a24_acceptance_serial_script_fail_summary_out_write_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_summary_out_write_validation_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_write_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses summary output write validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_write_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_summary_out_write_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_write_validation_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_summary_out_write_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_summary_out_write_validation_marker.log" >&2
  exit 1
fi

sed -i 's/failed_rc=/failed_rc_removed=/' "${a24_acceptance_serial_script_fail_failed_rc_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_failed_rc_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_failed_rc_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses failed_rc marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_rc_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_failed_rc_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_rc_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_failed_rc_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_rc_marker.log" >&2
  exit 1
fi

sed -i 's/failed_log=/failed_log_removed=/' "${a24_acceptance_serial_script_fail_failed_log_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_failed_log_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_failed_log_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses failed_log marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_log_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_failed_log_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_log_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_failed_log_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_failed_log_marker.log" >&2
  exit 1
fi

sed -i 's/"mbd_ci_contract_test"/"mbd_ci_contract_test_removed"/' "${a24_acceptance_serial_script_fail_cmd_ci_contract_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_fail_cmd_ci_contract_marker}" "${a24_acceptance_serial_test_script_pass}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_cmd_ci_contract_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial script misses ci_contract_test command marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_cmd_ci_contract_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_cmd_ci_contract_test\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_cmd_ci_contract_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_cmd_ci_contract_test failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_cmd_ci_contract_marker.log" >&2
  exit 1
fi

sed -i 's/run_a24_acceptance_serial self-test requires successful FEM4C build preflight/run_a24_acceptance_serial self-test preflight removed/' "${a24_acceptance_serial_test_script_fail_build_preflight_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_build_preflight_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_build_preflight_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses build preflight marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_build_preflight_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_build_preflight_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_build_preflight_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_build_preflight_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_build_preflight_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_RETRY_ON_137=2/A24_ACCEPT_SERIAL_RETRY_ON_137_REMOVED=2/' "${a24_acceptance_serial_test_script_fail_retry_knob_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_retry_knob_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_retry_knob_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses retry knob case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_retry_knob_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_retry_knob_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_retry_knob_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_retry_knob_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_retry_knob_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test/A24_ACCEPT_SERIAL_FAKE_137_STEP_REMOVED=batch_test/' "${a24_acceptance_serial_test_script_fail_fake_step_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_fake_step_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_fake_step_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses fake-step case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_fake_step_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_fake_step_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_fake_step_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_fake_step_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_fake_step_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_STEP_LOG_DIR=/A24_ACCEPT_SERIAL_STEP_LOG_DIR_REMOVED=/' "${a24_acceptance_serial_test_script_fail_step_log_dir_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_step_log_dir_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses step-log dir case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_step_log_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_dir_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_step_log_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_dir_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_STEP_LOG_DIR=\"${tmp_dir}\/step_log_file\"/A24_ACCEPT_SERIAL_STEP_LOG_DIR_REMOVED=\"${tmp_dir}\/step_log_file\"/' "${a24_acceptance_serial_test_script_fail_step_log_file_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_step_log_file_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_file_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses step-log file-path case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_file_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_step_log_file_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_file_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_step_log_file_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_file_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_STEP_LOG_DIR=\"${tmp_dir}\/step_logs_readonly\"/A24_ACCEPT_SERIAL_STEP_LOG_DIR_REMOVED=\"${tmp_dir}\/step_logs_readonly\"/' "${a24_acceptance_serial_test_script_fail_step_log_readonly_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_step_log_readonly_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_readonly_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses step-log readonly case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_readonly_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_step_log_readonly_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_readonly_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_step_log_readonly_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_step_log_readonly_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_SUMMARY_OUT=\"${tmp_dir}\/missing_summary_dir\/summary.log\"/A24_ACCEPT_SERIAL_SUMMARY_OUT_REMOVED=\"${tmp_dir}\/missing_summary_dir\/summary.log\"/' "${a24_acceptance_serial_test_script_fail_summary_out_missing_dir_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_summary_out_missing_dir_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_missing_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses summary-out missing-dir case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_summary_out_missing_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_missing_dir_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_summary_out_missing_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_SUMMARY_OUT=\"${tmp_dir}\/summary_dir_path\"/A24_ACCEPT_SERIAL_SUMMARY_OUT_REMOVED=\"${tmp_dir}\/summary_dir_path\"/' "${a24_acceptance_serial_test_script_fail_summary_out_dir_path_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_summary_out_dir_path_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_dir_path_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses summary-out dir-path case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_summary_out_dir_path_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_dir_path_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_summary_out_dir_path_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_ACCEPT_SERIAL_SUMMARY_OUT=\"${tmp_dir}\/summary_readonly.log\"/A24_ACCEPT_SERIAL_SUMMARY_OUT_REMOVED=\"${tmp_dir}\/summary_readonly.log\"/' "${a24_acceptance_serial_test_script_fail_summary_out_readonly_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" "${b8_regression_test_script_pass}" "${b8_regression_full_test_script_pass}" "${a24_acceptance_serial_script_pass}" "${a24_acceptance_serial_test_script_fail_summary_out_readonly_case_marker}" >"${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_readonly_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 acceptance serial test script misses summary-out readonly case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_readonly_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_acceptance_serial_test_summary_out_readonly_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_readonly_case_marker.log"; then
  echo "FAIL: expected a24_acceptance_serial_test_summary_out_readonly_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_acceptance_serial_test_summary_out_readonly_case_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_one}"
sed -i 's/grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}\/regression_1.log"/grep -Fq "local_target_removed=${B8_LOCAL_TARGET}" "${tmp_dir}\/regression_1.log"/' "${b8_knob_matrix_script_fail_regression_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_local_target_summary_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression local-target summary trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_local_target_summary_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_local_target_summary_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_local_target_summary_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_local_target_summary_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_local_target_summary_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_regression_one}"
sed -i 's/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_regression_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_one.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression one-case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_one.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_one_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_one.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_one_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_one.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_regression_invalid_make}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_invalid_make}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_invalid_make.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression invalid-make marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_invalid_make.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_invalid_make_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_invalid_make.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_invalid_make_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_invalid_make.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_regression_repo_default_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_repo_default_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_repo_default_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression repo default lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_repo_default_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_repo_default_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_repo_default_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_repo_default_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_repo_default_lock_source.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_regression_global_default_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_global_default_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_global_default_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression global default lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_global_default_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_global_default_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_global_default_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_global_default_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_global_default_lock_source.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${regression_env_lock_dir}" B8_MAKE_CMD=make/mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${regression_env_lock_dir}" B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_regression_env_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_env_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression env lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_env_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_env_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source.log" >&2
  exit 1
fi

sed -i 's/grep -q "lock_dir_source=env" "${tmp_dir}\/regression_env_lock_scope.log"/grep -q "lock_dir_source_removed=env" "${tmp_dir}\/regression_env_lock_scope.log"/' "${b8_knob_matrix_script_fail_regression_env_lock_source_trace}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_regression_env_lock_source_trace}" >"${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses regression env lock-source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_regression_env_lock_source_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_regression_env_lock_source_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_regression_env_lock_source_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_one}"
sed -i 's/test_retry_reason=(none|parser_missing)/test_retry_reason_removed=(none|parser_missing)/' "${b8_knob_matrix_script_fail_full_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_zero_retry_reason_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full zero retry-reason trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_zero_retry_reason_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_zero_retry_reason_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_zero_retry_reason_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_full_zero_retry_reason_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_zero_retry_reason_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_one}"
sed -i 's/test_retry_reason=(none|parser_missing)/test_retry_reason_removed=(none|parser_missing)/' "${b8_knob_matrix_script_fail_full_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_one_retry_reason_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full one retry-reason trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_one_retry_reason_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_one_retry_reason_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_one_retry_reason_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_full_one_retry_reason_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_one_retry_reason_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_one}"
sed -i 's/grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}\/full_1.log"/grep -Fq "local_target_removed=${B8_LOCAL_TARGET}" "${tmp_dir}\/full_1.log"/' "${b8_knob_matrix_script_fail_full_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_local_target_summary_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full local-target summary trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_local_target_summary_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_local_target_summary_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_local_target_summary_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_full_local_target_summary_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_local_target_summary_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_one}"
sed -i 's/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_full_one}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_one}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_one.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full one-case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_one.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_one_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_one.log"; then
  echo "FAIL: expected b8_knob_matrix_full_one_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_one.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_full_invalid_make}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_invalid_make}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_invalid_make.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full invalid-make marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_invalid_make.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_invalid_make_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_invalid_make.log"; then
  echo "FAIL: expected b8_knob_matrix_full_invalid_make_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_invalid_make.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_full_repo_default_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_repo_default_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full repo default lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_repo_default_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_full_repo_default_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_source.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_full_global_default_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_global_default_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full global default lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_global_default_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_full_global_default_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_source.log" >&2
  exit 1
fi

sed -i 's/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${full_env_lock_dir}" B8_MAKE_CMD=make/mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${full_env_lock_dir}" B8_MAKE_CMD=marker_removed/' "${b8_knob_matrix_script_fail_full_env_lock_source}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_env_lock_source}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full env lock-source case marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_env_lock_source_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source.log"; then
  echo "FAIL: expected b8_knob_matrix_full_env_lock_source_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source.log" >&2
  exit 1
fi

sed -i 's/grep -q "b8_lock_dir_source=env" "${tmp_dir}\/full_env_lock_scope.log"/grep -q "b8_lock_dir_source_removed=env" "${tmp_dir}\/full_env_lock_scope.log"/' "${b8_knob_matrix_script_fail_full_env_lock_source_trace}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_env_lock_source_trace}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full env lock-source trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_env_lock_source_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source_trace.log"; then
  echo "FAIL: expected b8_knob_matrix_full_env_lock_source_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_env_lock_source_trace.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/repo_lock_hash=/repo_lock_hash_removed=/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_lock_hash_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full repo lock hash marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_lock_hash_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_repo_lock_hash_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_lock_hash_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_repo_lock_hash_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_lock_hash_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/repo_default_lock_dir=/repo_default_lock_dir_removed=/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full repo default lock dir marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_repo_default_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_dir_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_repo_default_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_repo_default_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/global_default_lock_dir=/global_default_lock_dir_removed=/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full global default lock dir marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_global_default_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_dir_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_global_default_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_global_default_lock_dir_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/full_cleanup_expected_calls=7/full_cleanup_expected_calls_removed=7/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_calls_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full cleanup expected-calls marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_calls_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_expected_calls_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_calls_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_expected_calls_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_calls_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/parser_cleanup_call_count=\$((parser_cleanup_call_count + 1))/parser_cleanup_counter_removed=\$((parser_cleanup_call_count + 1))/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_counter_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses parser cleanup counter marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_counter_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_cleanup_counter_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_counter_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_cleanup_counter_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_counter_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/b8_cleanup_call_count=\$((b8_cleanup_call_count + 1))/b8_cleanup_counter_removed=\$((b8_cleanup_call_count + 1))/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_counter_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 cleanup counter marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_counter_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_cleanup_counter_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_counter_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_cleanup_counter_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_counter_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/full_cleanup_expected_order_trace=\"\"/full_cleanup_expected_order_trace_removed=\"\"/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup expected-order trace marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_expected_order_trace_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_trace_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_expected_order_trace_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_trace_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/for ((cleanup_idx=0; cleanup_idx<full_cleanup_expected_calls; cleanup_idx++)); do/for ((cleanup_idx_removed=0; cleanup_idx_removed<full_cleanup_expected_calls; cleanup_idx_removed++)); do/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_build_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup expected-order build marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_build_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_expected_order_build_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_build_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_expected_order_build_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_expected_order_build_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/append_cleanup_call_order_trace() {/append_cleanup_call_order_trace_removed() {/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_append_fn_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup order-trace append function marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_append_fn_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_order_trace_append_fn_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_append_fn_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_order_trace_append_fn_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_append_fn_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/append_cleanup_call_order_trace \"parser\"/append_cleanup_call_order_trace_removed \"parser\"/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_append_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses parser cleanup order-trace append marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_append_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_cleanup_order_trace_append_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_append_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_cleanup_order_trace_append_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_append_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/append_cleanup_call_order_trace \"b8\"/append_cleanup_call_order_trace_removed \"b8\"/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_append_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 cleanup order-trace append marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_append_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_cleanup_order_trace_append_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_append_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_cleanup_order_trace_append_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_append_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i '0,/append_cleanup_call_order_trace \"parser\"/s//append_cleanup_call_order_trace_parser_order_removed \"parser\"/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script parser cleanup order-trace before-rm marker is broken" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i '0,/append_cleanup_call_order_trace \"b8\"/s//append_cleanup_call_order_trace_b8_order_removed \"b8\"/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script b8 cleanup order-trace before-rm marker is broken" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/cleanup_parser_compat_lock() {/cleanup_parser_compat_lock_removed() {/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_fn_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full parser lock cleanup function marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_fn_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_lock_cleanup_fn_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_fn_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_lock_cleanup_fn_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_fn_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/  cleanup_parser_compat_lock/  cleanup_parser_lock_cleanup_removed/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full parser lock cleanup call marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_lock_cleanup_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_lock_cleanup_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i '0,/  cleanup_parser_compat_lock/s//  parser_lock_cleanup_removed/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_call_order_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script cleanup call-order marker is broken" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_call_order_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_call_order_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_call_order_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_call_order_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_call_order_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i '0,/  cleanup_parser_compat_lock/s//  parser_lock_cleanup_count_removed/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script parser cleanup call count is reduced" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_lock_cleanup_call_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_count_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_lock_cleanup_call_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup_call_count_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i '0,/  cleanup_b8_regression_locks/s//  b8_lock_cleanup_count_removed/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_count_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script b8 cleanup call count is reduced" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_count_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_lock_cleanup_call_count_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_count_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_lock_cleanup_call_count_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_count_marker.log" >&2
  exit 1
fi

sed -i 's#rm -rf /tmp/fem4c_parser_compat.lock 2>/dev/null || true#parser_lock_cleanup_removed#' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses full parser lock cleanup marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_lock_cleanup_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_lock_cleanup_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_lock_cleanup.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/cleanup_b8_regression_locks() {/cleanup_b8_regression_locks_removed() {/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_fn_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 lock cleanup function marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_fn_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_lock_cleanup_fn_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_fn_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_lock_cleanup_fn_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_fn_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's#rm -rf "${repo_default_lock_dir}" "${global_default_lock_dir}" 2>/dev/null || true#b8_lock_cleanup_removed#' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 lock cleanup marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_lock_cleanup_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_lock_cleanup_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/  cleanup_b8_regression_locks/  cleanup_b8_lock_cleanup_removed/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 lock cleanup call marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_lock_cleanup_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_lock_cleanup_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_lock_cleanup_call_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/if \[\[ "${parser_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" \]\]; then/if [[ "${parser_cleanup_call_count}" -eq "${full_cleanup_expected_calls}" ]]; then/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_count_assert_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses parser cleanup count assert marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_count_assert_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_parser_cleanup_count_assert_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_count_assert_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_parser_cleanup_count_assert_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_parser_cleanup_count_assert_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/if \[\[ "${b8_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" \]\]; then/if [[ "${b8_cleanup_call_count}" -eq "${full_cleanup_expected_calls}" ]]; then/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_count_assert_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses b8 cleanup count assert marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_count_assert_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_b8_cleanup_count_assert_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_count_assert_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_b8_cleanup_count_assert_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_b8_cleanup_count_assert_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/if \[\[ "${cleanup_call_order_trace}" != "${full_cleanup_expected_order_trace}" \]\]; then/if [[ "${cleanup_call_order_trace}" == "${full_cleanup_expected_order_trace}" ]]; then/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_assert_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup order-trace assert marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_assert_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_order_trace_assert_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_assert_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_order_trace_assert_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_trace_assert_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/INFO: full cleanup call count parser=/INFO: full cleanup count parser=/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_count_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup count summary marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_count_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_count_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_count_summary_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_count_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_count_summary_marker.log" >&2
  exit 1
fi

cp "FEM4C/scripts/test_b8_knob_matrix.sh" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"
sed -i 's/INFO: full cleanup call order trace=/INFO: full cleanup order trace=/' "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_full_parser_lock_cleanup}" >"${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses cleanup order summary marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_full_cleanup_order_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_summary_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_full_cleanup_order_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_full_cleanup_order_summary_marker.log" >&2
  exit 1
fi

sed -i 's/B8_KNOB_MATRIX_SKIP_FULL must be 0 or 1/B8_KNOB_MATRIX_SKIP_FULL validation removed/' "${b8_knob_matrix_script_fail_skip_validation}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_skip_validation}" >"${tmp_dir}/contract_fail_b8_knob_matrix_skip_validation.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses skip-full validation marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_skip_validation.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_skip_full_validation\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_skip_validation.log"; then
  echo "FAIL: expected b8_knob_matrix_skip_full_validation failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_skip_validation.log" >&2
  exit 1
fi

sed -i 's/INFO: skip full regression matrix cases (B8_KNOB_MATRIX_SKIP_FULL=1)/INFO: skip marker removed/' "${b8_knob_matrix_script_fail_skip_info_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_skip_info_marker}" >"${tmp_dir}/contract_fail_b8_knob_matrix_skip_info_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses skip-full info marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_skip_info_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_skip_full_info_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_skip_info_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_skip_full_info_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_skip_info_marker.log" >&2
  exit 1
fi

sed -i 's/export B8_LOCAL_TARGET="${B8_LOCAL_TARGET:-mbd_b8_syntax}"/export B8_LOCAL_TARGET="${B8_LOCAL_TARGET:-removed_mbd_b8_syntax}"/' "${b8_knob_matrix_script_fail_local_target_env_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_fail_local_target_env_marker}" >"${tmp_dir}/contract_fail_b8_knob_matrix_local_target_env_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when b8 knob matrix script misses local-target env marker" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_local_target_env_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[b8_knob_matrix_local_target_env_marker\\]=FAIL" "${tmp_dir}/contract_fail_b8_knob_matrix_local_target_env_marker.log"; then
  echo "FAIL: expected b8_knob_matrix_local_target_env_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_b8_knob_matrix_local_target_env_marker.log" >&2
  exit 1
fi

sed -i 's/run_cli_invalid_dt_case/run_cli_dt_invalid_case/' "${mbd_integrator_script_fail_dt_case}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_dt_case}" >"${tmp_dir}/contract_fail_mbd_integrator_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses dt invalid case" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_script.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_cli_invalid_dt_case\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_script.log"; then
  echo "FAIL: expected mbd_integrator_cli_invalid_dt_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_script.log" >&2
  exit 1
fi

sed -i 's/run_env_time_whitespace_case/run_env_time_space_case/' "${mbd_integrator_script_fail_whitespace_case}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_whitespace_case}" >"${tmp_dir}/contract_fail_mbd_integrator_whitespace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses whitespace env case" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_whitespace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_env_time_whitespace_case\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_whitespace.log"; then
  echo "FAIL: expected mbd_integrator_env_time_whitespace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_whitespace.log" >&2
  exit 1
fi

sed -i 's/run_env_time_compact_trace_case/run_env_time_compact_case/' "${mbd_integrator_script_fail_compact_trace_case}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_compact_trace_case}" >"${tmp_dir}/contract_fail_mbd_integrator_compact_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses compact trace case" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_compact_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_env_time_compact_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_compact_trace.log"; then
  echo "FAIL: expected mbd_integrator_env_time_compact_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_compact_trace.log" >&2
  exit 1
fi

sed -i 's/run_cli_compact_trace_case/run_cli_compact_case/' "${mbd_integrator_script_fail_cli_compact_trace_case}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_cli_compact_trace_case}" >"${tmp_dir}/contract_fail_mbd_integrator_cli_compact_trace.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses cli compact-trace case" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_cli_compact_trace.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_cli_compact_trace_case\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_cli_compact_trace.log"; then
  echo "FAIL: expected mbd_integrator_cli_compact_trace_case failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_cli_compact_trace.log" >&2
  exit 1
fi

sed -i 's/newmark_beta_source_status,cli/newmark_beta_source_status,removed_cli/' "${mbd_integrator_script_fail_source_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_source_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_source_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses source-status marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_source_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_source_status_cli_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_source_marker.log"; then
  echo "FAIL: expected mbd_integrator_source_status_cli_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_source_marker.log" >&2
  exit 1
fi

sed -i 's/dt_source_status,env_invalid_fallback/dt_source_status,env_invalid_removed/' "${mbd_integrator_script_fail_time_source_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_time_source_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_time_source_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses time source-status marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_time_source_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_time_source_status_env_fallback_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_time_source_marker.log"; then
  echo "FAIL: expected mbd_time_source_status_env_fallback_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_time_source_marker.log" >&2
  exit 1
fi

sed -i 's/steps_requested,3/steps_requested,removed_3/' "${mbd_integrator_script_fail_step_trace_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_step_trace_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_step_trace_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses steps_requested marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_step_trace_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_step_trace_cli_output_requested_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_step_trace_marker.log"; then
  echo "FAIL: expected mbd_step_trace_cli_output_requested_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_step_trace_marker.log" >&2
  exit 1
fi

sed -i 's/FEM4C_BIN_DEFAULT="${FEM4C_DIR}\/bin\/fem4c"/FEM4C_BIN_DEFAULT_REMOVED="${FEM4C_DIR}\/bin\/fem4c"/' "${mbd_integrator_script_fail_bin_default_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_bin_default_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_bin_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses fem4c bin default marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_bin_default_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_bin_default_marker.log"; then
  echo "FAIL: expected mbd_integrator_bin_default_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_default_marker.log" >&2
  exit 1
fi

sed -i 's/FEM4C_BIN="${FEM4C_MBD_BIN:-${FEM4C_BIN_DEFAULT}}"/FEM4C_BIN_REMOVED="${FEM4C_MBD_BIN:-${FEM4C_BIN_DEFAULT}}"/' "${mbd_integrator_script_fail_bin_env_override_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_bin_env_override_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_bin_env_override_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses fem4c bin env override marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_env_override_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_bin_env_override_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_bin_env_override_marker.log"; then
  echo "FAIL: expected mbd_integrator_bin_env_override_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_env_override_marker.log" >&2
  exit 1
fi

sed -i 's/mbd integrator checker requires executable fem4c binary/mbd integrator bin preflight removed/' "${mbd_integrator_script_fail_bin_preflight_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_fail_bin_preflight_marker}" >"${tmp_dir}/contract_fail_mbd_integrator_bin_preflight_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when mbd integrator script misses fem4c bin preflight marker" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_preflight_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[mbd_integrator_bin_preflight_marker\\]=FAIL" "${tmp_dir}/contract_fail_mbd_integrator_bin_preflight_marker.log"; then
  echo "FAIL: expected mbd_integrator_bin_preflight_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_mbd_integrator_bin_preflight_marker.log" >&2
  exit 1
fi

sed -i 's/"mbd_ci_contract_test"/"mbd_ci_contract_test_missing"/' "${a24_regression_script_fail_command}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_command}" >"${tmp_dir}/contract_fail_a24_regression_command.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses mbd_ci_contract_test command" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_command.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_cmd_ci_contract_test\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_command.log"; then
  echo "FAIL: expected a24_regression_cmd_ci_contract_test failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_command.log" >&2
  exit 1
fi

sed -i 's/run_contract_test="${A24_RUN_CONTRACT_TEST:-1}"/run_contract_test_removed="${A24_RUN_CONTRACT_TEST:-1}"/' "${a24_regression_script_fail_contract_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_contract_knob_marker}" >"${tmp_dir}/contract_fail_a24_regression_contract_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses contract-test knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_contract_test_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_contract_knob_marker.log"; then
  echo "FAIL: expected a24_regression_contract_test_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_RUN_CONTRACT_TEST must be 0 or 1/A24_RUN_CONTRACT_TEST validation removed/' "${a24_regression_script_fail_contract_knob_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_contract_knob_validation_marker}" >"${tmp_dir}/contract_fail_a24_regression_contract_knob_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses contract-test knob validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_knob_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_contract_test_knob_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_contract_knob_validation_marker.log"; then
  echo "FAIL: expected a24_regression_contract_test_knob_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_knob_validation_marker.log" >&2
  exit 1
fi

sed -i 's/INFO: skip mbd_ci_contract_test (A24_RUN_CONTRACT_TEST=0)/INFO: skip marker removed/' "${a24_regression_script_fail_contract_skip_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_contract_skip_marker}" >"${tmp_dir}/contract_fail_a24_regression_contract_skip_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses contract-test skip marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_skip_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_contract_test_skip_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_contract_skip_marker.log"; then
  echo "FAIL: expected a24_regression_contract_test_skip_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_contract_skip_marker.log" >&2
  exit 1
fi

sed -i 's/env -u MAKEFLAGS -u MFLAGS make -C FEM4C/make -C FEM4C/' "${a24_regression_script_fail_makeflags_isolation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_makeflags_isolation_marker}" >"${tmp_dir}/contract_fail_a24_regression_makeflags_isolation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses makeflags isolation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_makeflags_isolation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_makeflags_isolation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_makeflags_isolation_marker.log"; then
  echo "FAIL: expected a24_regression_makeflags_isolation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_makeflags_isolation_marker.log" >&2
  exit 1
fi

sed -i 's/A24_REGRESSION_SUMMARY contract_test=/A24_REGRESSION_SUMMARY_REMOVED contract_test=/' "${a24_regression_script_fail_summary_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_summary_marker}" >"${tmp_dir}/contract_fail_a24_regression_summary_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses summary marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_summary_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_summary_marker.log"; then
  echo "FAIL: expected a24_regression_summary_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out="${A24_REGRESSION_SUMMARY_OUT:-}"/summary_out_removed="${A24_REGRESSION_SUMMARY_OUT:-}"/' "${a24_regression_script_fail_summary_out_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_summary_out_marker}" >"${tmp_dir}/contract_fail_a24_regression_summary_out_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses summary-out marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_summary_out_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_summary_out_marker.log"; then
  echo "FAIL: expected a24_regression_summary_out_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_marker.log" >&2
  exit 1
fi

sed -i 's/A24 regression summary output directory does not exist/summary output directory validation removed/' "${a24_regression_script_fail_summary_out_dir_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_summary_out_dir_validation_marker}" >"${tmp_dir}/contract_fail_a24_regression_summary_out_dir_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses summary-out missing-dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_summary_out_dir_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_summary_out_dir_validation_marker.log"; then
  echo "FAIL: expected a24_regression_summary_out_dir_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 regression summary output path must be a file/summary output path-type validation removed/' "${a24_regression_script_fail_summary_out_type_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_summary_out_type_validation_marker}" >"${tmp_dir}/contract_fail_a24_regression_summary_out_type_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses summary-out path-type validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_type_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_summary_out_type_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_summary_out_type_validation_marker.log"; then
  echo "FAIL: expected a24_regression_summary_out_type_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_type_validation_marker.log" >&2
  exit 1
fi

sed -i 's/cannot write A24 regression summary output/summary output write validation removed/' "${a24_regression_script_fail_summary_out_write_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_summary_out_write_validation_marker}" >"${tmp_dir}/contract_fail_a24_regression_summary_out_write_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses summary-out write validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_write_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_summary_out_write_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_summary_out_write_validation_marker.log"; then
  echo "FAIL: expected a24_regression_summary_out_write_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_summary_out_write_validation_marker.log" >&2
  exit 1
fi

sed -i 's/skip_lock="${A24_REGRESSION_SKIP_LOCK:-0}"/skip_lock_removed="${A24_REGRESSION_SKIP_LOCK:-0}"/' "${a24_regression_script_fail_skip_lock_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_skip_lock_knob_marker}" >"${tmp_dir}/contract_fail_a24_regression_skip_lock_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses skip-lock knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_skip_lock_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_skip_lock_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_skip_lock_knob_marker.log"; then
  echo "FAIL: expected a24_regression_skip_lock_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_skip_lock_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_REGRESSION_SKIP_LOCK must be 0 or 1/A24_REGRESSION_SKIP_LOCK validation removed/' "${a24_regression_script_fail_skip_lock_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_skip_lock_validation_marker}" >"${tmp_dir}/contract_fail_a24_regression_skip_lock_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses skip-lock validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_skip_lock_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_skip_lock_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_skip_lock_validation_marker.log"; then
  echo "FAIL: expected a24_regression_skip_lock_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_skip_lock_validation_marker.log" >&2
  exit 1
fi

sed -i 's/lock_dir="${A24_REGRESSION_LOCK_DIR:-\/tmp\/fem4c_a24_regression.lock}"/lock_dir_removed="${A24_REGRESSION_LOCK_DIR:-\/tmp\/fem4c_a24_regression.lock}"/' "${a24_regression_script_fail_lock_dir_default_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_lock_dir_default_marker}" >"${tmp_dir}/contract_fail_a24_regression_lock_dir_default_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses lock-dir default marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_lock_dir_default_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_lock_dir_default_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_lock_dir_default_marker.log"; then
  echo "FAIL: expected a24_regression_lock_dir_default_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_lock_dir_default_marker.log" >&2
  exit 1
fi

sed -i 's/FAIL: a24 regression lock is already held/FAIL: lock marker removed/' "${a24_regression_script_fail_lock_fail_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_fail_lock_fail_marker}" >"${tmp_dir}/contract_fail_a24_regression_lock_fail_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression script misses lock-fail marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_lock_fail_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_lock_fail_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_lock_fail_marker.log"; then
  echo "FAIL: expected a24_regression_lock_fail_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_lock_fail_marker.log" >&2
  exit 1
fi

sed -i 's/run_a24_regression self-test requires successful FEM4C build preflight/run_a24_regression self-test preflight removed/' "${a24_regression_test_script_fail_build_preflight_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_fail_build_preflight_marker}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_regression_test_build_preflight_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression-test script misses build preflight marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_build_preflight_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_test_build_preflight_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_test_build_preflight_marker.log"; then
  echo "FAIL: expected a24_regression_test_build_preflight_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_build_preflight_marker.log" >&2
  exit 1
fi

sed -i 's/A24_REGRESSION_SUMMARY_OUT="${tmp_dir}\/missing_summary_dir\/summary.log"/A24_REGRESSION_SUMMARY_OUT_REMOVED="${tmp_dir}\/missing_summary_dir\/summary.log"/' "${a24_regression_test_script_fail_summary_out_missing_dir_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_fail_summary_out_missing_dir_case_marker}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_regression_test_summary_out_missing_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression-test script misses summary-out missing-dir case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_test_summary_out_missing_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_test_summary_out_missing_dir_case_marker.log"; then
  echo "FAIL: expected a24_regression_test_summary_out_missing_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_REGRESSION_SUMMARY_OUT="${tmp_dir}\/summary_dir_path"/A24_REGRESSION_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_dir_path"/' "${a24_regression_test_script_fail_summary_out_dir_path_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_fail_summary_out_dir_path_case_marker}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_regression_test_summary_out_dir_path_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression-test script misses summary-out dir-path case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_test_summary_out_dir_path_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_test_summary_out_dir_path_case_marker.log"; then
  echo "FAIL: expected a24_regression_test_summary_out_dir_path_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_REGRESSION_SUMMARY_OUT="${tmp_dir}\/summary_parent_readonly\/summary.log"/A24_REGRESSION_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_parent_readonly\/summary.log"/' "${a24_regression_test_script_fail_summary_out_readonly_parent_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_fail_summary_out_readonly_parent_case_marker}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_regression_test_summary_out_readonly_parent_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression-test script misses summary-out readonly-parent case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_test_summary_out_readonly_parent_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_test_summary_out_readonly_parent_case_marker.log"; then
  echo "FAIL: expected a24_regression_test_summary_out_readonly_parent_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_write_probe="\/proc\/1\/cmdline"/summary_out_write_probe_removed="\/proc\/1\/cmdline"/' "${a24_regression_test_script_fail_summary_out_write_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_fail_summary_out_write_case_marker}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_regression_test_summary_out_write_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 regression-test script misses summary-out write case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_regression_test_summary_out_write_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_regression_test_summary_out_write_case_marker.log"; then
  echo "FAIL: expected a24_regression_test_summary_out_write_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_regression_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

sed -i 's/run_a24_batch self-test requires successful FEM4C build preflight/run_a24_batch self-test preflight removed/' "${a24_batch_test_script_fail_build_preflight_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_build_preflight_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_build_preflight_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses build preflight marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_build_preflight_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_build_preflight_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_build_preflight_marker.log"; then
  echo "FAIL: expected a24_batch_test_build_preflight_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_build_preflight_marker.log" >&2
  exit 1
fi

sed -i 's/run_a24_regression_full self-test requires successful FEM4C build preflight/run_a24_regression_full self-test preflight removed/' "${a24_regression_full_test_script_fail_build_preflight_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_build_preflight_marker}" >"${tmp_dir}/contract_fail_a24_full_test_build_preflight_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses build preflight marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_build_preflight_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_build_preflight_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_build_preflight_marker.log"; then
  echo "FAIL: expected a24_full_test_build_preflight_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_build_preflight_marker.log" >&2
  exit 1
fi

sed -i 's#\/tmp\/fem4c_test_run_a24_batch.lock#\/tmp\/removed_fem4c_test_run_a24_batch.lock#' "${a24_batch_test_script_fail_selftest_lock_dir_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_selftest_lock_dir_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses selftest lock-dir marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_selftest_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_dir_marker.log"; then
  echo "FAIL: expected a24_batch_test_selftest_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

sed -i '0,/acquire_selftest_lock()/s//acquire_selftest_lock_removed()/' "${a24_batch_test_script_fail_selftest_lock_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_selftest_lock_function_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses selftest lock-function marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_selftest_lock_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_function_marker.log"; then
  echo "FAIL: expected a24_batch_test_selftest_lock_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

sed -i 's/test_run_a24_batch already running/test_run_a24_batch lock marker removed/' "${a24_batch_test_script_fail_selftest_lock_busy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_selftest_lock_busy_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_busy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses selftest lock-busy marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_selftest_lock_busy_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_busy_marker.log"; then
  echo "FAIL: expected a24_batch_test_selftest_lock_busy_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

sed -i 's/&& ! kill -0 "${owner_pid}" 2>\/dev\/null/&& ! kill_removed -0 "${owner_pid}" 2>\/dev\/null/' "${a24_batch_test_script_fail_selftest_lock_stale_recovery_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_selftest_lock_stale_recovery_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_stale_recovery_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses selftest lock stale-recovery marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_stale_recovery_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_selftest_lock_stale_recovery_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_stale_recovery_marker.log"; then
  echo "FAIL: expected a24_batch_test_selftest_lock_stale_recovery_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_selftest_lock_stale_recovery_marker.log" >&2
  exit 1
fi

sed -i 's#\/tmp\/fem4c_test_run_a24_regression_full.lock#\/tmp\/removed_fem4c_test_run_a24_regression_full.lock#' "${a24_regression_full_test_script_fail_selftest_lock_dir_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_selftest_lock_dir_marker}" >"${tmp_dir}/contract_fail_a24_full_test_selftest_lock_dir_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses selftest lock-dir marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_selftest_lock_dir_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_dir_marker.log"; then
  echo "FAIL: expected a24_full_test_selftest_lock_dir_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_dir_marker.log" >&2
  exit 1
fi

sed -i '0,/acquire_selftest_lock()/s//acquire_selftest_lock_removed()/' "${a24_regression_full_test_script_fail_selftest_lock_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_selftest_lock_function_marker}" >"${tmp_dir}/contract_fail_a24_full_test_selftest_lock_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses selftest lock-function marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_selftest_lock_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_function_marker.log"; then
  echo "FAIL: expected a24_full_test_selftest_lock_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_function_marker.log" >&2
  exit 1
fi

sed -i 's/test_run_a24_regression_full already running/test_run_a24_regression_full lock marker removed/' "${a24_regression_full_test_script_fail_selftest_lock_busy_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_selftest_lock_busy_marker}" >"${tmp_dir}/contract_fail_a24_full_test_selftest_lock_busy_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses selftest lock-busy marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_selftest_lock_busy_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_busy_marker.log"; then
  echo "FAIL: expected a24_full_test_selftest_lock_busy_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_busy_marker.log" >&2
  exit 1
fi

sed -i 's/&& ! kill -0 "${owner_pid}" 2>\/dev\/null/&& ! kill_removed -0 "${owner_pid}" 2>\/dev\/null/' "${a24_regression_full_test_script_fail_selftest_lock_stale_recovery_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_selftest_lock_stale_recovery_marker}" >"${tmp_dir}/contract_fail_a24_full_test_selftest_lock_stale_recovery_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses selftest lock stale-recovery marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_stale_recovery_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_selftest_lock_stale_recovery_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_stale_recovery_marker.log"; then
  echo "FAIL: expected a24_full_test_selftest_lock_stale_recovery_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_selftest_lock_stale_recovery_marker.log" >&2
  exit 1
fi

sed -i 's/A24_FULL_SUMMARY_OUT="${tmp_dir}\/missing_full_summary_dir\/summary.log"/A24_FULL_SUMMARY_OUT_REMOVED="${tmp_dir}\/missing_full_summary_dir\/summary.log"/' "${a24_regression_full_test_script_fail_summary_out_missing_dir_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_summary_out_missing_dir_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_summary_out_missing_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses summary-out missing-dir case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_summary_out_missing_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_summary_out_missing_dir_case_marker.log"; then
  echo "FAIL: expected a24_full_test_summary_out_missing_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_FULL_SUMMARY_OUT="${tmp_dir}\/summary_dir_path"/A24_FULL_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_dir_path"/' "${a24_regression_full_test_script_fail_summary_out_dir_path_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_summary_out_dir_path_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_summary_out_dir_path_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses summary-out dir-path case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_summary_out_dir_path_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_summary_out_dir_path_case_marker.log"; then
  echo "FAIL: expected a24_full_test_summary_out_dir_path_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_FULL_SUMMARY_OUT="${tmp_dir}\/summary_parent_readonly\/summary.log"/A24_FULL_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_parent_readonly\/summary.log"/' "${a24_regression_full_test_script_fail_summary_out_readonly_parent_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_summary_out_readonly_parent_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_summary_out_readonly_parent_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses summary-out readonly-parent case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_summary_out_readonly_parent_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_summary_out_readonly_parent_case_marker.log"; then
  echo "FAIL: expected a24_full_test_summary_out_readonly_parent_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

sed -i 's/full_summary_write_probe="\/proc\/1\/cmdline"/full_summary_write_probe_removed="\/proc\/1\/cmdline"/' "${a24_regression_full_test_script_fail_summary_out_write_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_summary_out_write_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_summary_out_write_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses summary-out write case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_summary_out_write_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_summary_out_write_case_marker.log"; then
  echo "FAIL: expected a24_full_test_summary_out_write_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_nonexec_bin=/summary_out_nonexec_bin_removed=/' "${a24_regression_full_test_script_fail_nonexec_bin_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_nonexec_bin_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_nonexec_bin_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses nonexec-bin case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_nonexec_bin_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_nonexec_bin_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_nonexec_bin_case_marker.log"; then
  echo "FAIL: expected a24_full_test_nonexec_bin_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_nonexec_bin_case_marker.log" >&2
  exit 1
fi

sed -i 's/export A24_REGRESSION_LOCK_DIR="${tmp_dir}\/a24_regression.lock"/export A24_REGRESSION_LOCK_DIR_REMOVED="${tmp_dir}\/a24_regression.lock"/' "${a24_regression_full_test_script_fail_regression_lock_dir_isolation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_regression_lock_dir_isolation_marker}" >"${tmp_dir}/contract_fail_a24_full_test_regression_lock_dir_isolation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses regression lock-dir isolation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_regression_lock_dir_isolation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_regression_lock_dir_isolation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_regression_lock_dir_isolation_marker.log"; then
  echo "FAIL: expected a24_full_test_regression_lock_dir_isolation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_regression_lock_dir_isolation_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_fallback=/summary_out_log_summary_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary-fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_precedence=/summary_out_log_summary_precedence_removed=/' "${a24_regression_full_test_script_fail_log_summary_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_precedence_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_precedence_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_malformed_key_precedence=/summary_out_log_summary_malformed_key_precedence_removed=/' "${a24_regression_full_test_script_fail_log_summary_malformed_key_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_malformed_key_precedence_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_key_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses malformed-key log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_key_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_malformed_key_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_key_precedence_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_malformed_key_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_key_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_duplicate_key_precedence=/summary_out_log_summary_duplicate_key_precedence_removed=/' "${a24_regression_full_test_script_fail_log_summary_duplicate_key_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_duplicate_key_precedence_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_duplicate_key_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses duplicate-key log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_duplicate_key_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_duplicate_key_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_duplicate_key_precedence_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_duplicate_key_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_duplicate_key_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_malformed_unknown_fallback=/summary_out_log_summary_malformed_unknown_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_unknown_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses malformed+unknown fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_unknown_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_malformed_unknown_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_unknown_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_malformed_unknown_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_malformed_unknown_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_uppercase=/summary_out_log_summary_uppercase_removed=/' "${a24_regression_full_test_script_fail_log_summary_uppercase_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_uppercase_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary uppercase case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_uppercase_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_uppercase_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_uppercase_precedence=/summary_out_log_summary_uppercase_precedence_removed=/' "${a24_regression_full_test_script_fail_log_summary_uppercase_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_uppercase_precedence_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses uppercase log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_uppercase_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_precedence_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_uppercase_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_uppercase_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_crlf_fallback=/summary_out_log_summary_crlf_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_crlf_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_crlf_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_crlf_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary CRLF fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_crlf_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_crlf_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_crlf_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_crlf_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_crlf_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_tab_whitespace=/summary_out_log_summary_tab_whitespace_removed=/' "${a24_regression_full_test_script_fail_log_summary_tab_whitespace_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_tab_whitespace_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_tab_whitespace_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary tab-whitespace case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_tab_whitespace_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_tab_whitespace_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_tab_whitespace_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_tab_whitespace_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_tab_whitespace_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_equals_whitespace_fallback=/summary_out_log_summary_equals_whitespace_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_equals_whitespace_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary equals-whitespace fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_equals_whitespace_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_equals_whitespace_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_equals_whitespace_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_equals_whitespace_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_equals_whitespace_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_value_fallback=/summary_out_log_summary_quoted_value_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_quoted_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_quoted_value_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary quoted-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_quoted_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_quoted_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_single_quote_fallback=/summary_out_log_summary_single_quote_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_single_quote_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_single_quote_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quote_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary single-quote fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quote_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_single_quote_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quote_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_single_quote_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quote_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quote_mixed_fallback=/summary_out_log_summary_quote_mixed_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_quote_mixed_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_quote_mixed_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_quote_mixed_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary quote-mixed fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quote_mixed_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_quote_mixed_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_quote_mixed_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_quote_mixed_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quote_mixed_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_backslash_value_fallback=/summary_out_log_summary_backslash_value_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_backslash_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_backslash_value_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_backslash_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary backslash-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_backslash_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_backslash_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_backslash_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_backslash_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_backslash_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_key_fallback=/summary_out_log_summary_quoted_key_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_quoted_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_quoted_key_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary quoted-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_quoted_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_quoted_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_single_quoted_key_fallback=/summary_out_log_summary_single_quoted_key_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quoted_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary single-quoted-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_single_quoted_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quoted_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_single_quoted_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_single_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_cmd_key_fallback=/summary_out_log_summary_quoted_cmd_key_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary quoted command-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_partial_fallback=/summary_out_log_summary_partial_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_partial_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_partial_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_partial_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary partial-fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_partial_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_partial_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_partial_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_partial_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_partial_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_missing_equals_fallback=/summary_out_log_summary_missing_equals_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_missing_equals_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_missing_equals_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_missing_equals_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary missing-equals fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_missing_equals_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_missing_equals_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_missing_equals_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_missing_equals_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_missing_equals_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_empty_value_fallback=/summary_out_log_summary_empty_value_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_empty_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_empty_value_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary empty-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_empty_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_empty_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_empty_key_fallback=/summary_out_log_summary_empty_key_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_empty_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_empty_key_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary empty-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_empty_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_empty_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_empty_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_extra_equals_fallback=/summary_out_log_summary_extra_equals_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_extra_equals_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_extra_equals_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_extra_equals_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary extra-equals fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_extra_equals_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_extra_equals_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_extra_equals_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_extra_equals_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_extra_equals_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_leading_punct_fallback=/summary_out_log_summary_leading_punct_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_leading_punct_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_leading_punct_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_leading_punct_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary leading-punct fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_leading_punct_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_leading_punct_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_leading_punct_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_leading_punct_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_leading_punct_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_internal_symbol_fallback=/summary_out_log_summary_internal_symbol_fallback_removed=/' "${a24_regression_full_test_script_fail_log_summary_internal_symbol_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_pass}" "${a24_regression_full_test_script_fail_log_summary_internal_symbol_fallback_case_marker}" >"${tmp_dir}/contract_fail_a24_full_test_log_summary_internal_symbol_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full-test script misses log-summary internal-symbol fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_internal_symbol_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_test_log_summary_internal_symbol_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_test_log_summary_internal_symbol_fallback_case_marker.log"; then
  echo "FAIL: expected a24_full_test_log_summary_internal_symbol_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_test_log_summary_internal_symbol_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/full->batch chain case/full->batch chain removed/' "${a24_batch_test_script_fail_full_chain_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_full_chain_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_full_chain_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses full->batch chain marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_full_chain_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_full_chain_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_full_chain_marker.log"; then
  echo "FAIL: expected a24_batch_test_full_chain_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_full_chain_marker.log" >&2
  exit 1
fi

sed -i 's/A24_BATCH_SUMMARY_OUT="${tmp_dir}\/missing_batch_summary_dir\/summary.log"/A24_BATCH_SUMMARY_OUT_REMOVED="${tmp_dir}\/missing_batch_summary_dir\/summary.log"/' "${a24_batch_test_script_fail_summary_out_missing_dir_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_summary_out_missing_dir_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_summary_out_missing_dir_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses summary-out missing-dir case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_summary_out_missing_dir_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_summary_out_missing_dir_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_summary_out_missing_dir_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_missing_dir_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_BATCH_SUMMARY_OUT="${tmp_dir}\/summary_dir_path"/A24_BATCH_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_dir_path"/' "${a24_batch_test_script_fail_summary_out_dir_path_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_summary_out_dir_path_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_summary_out_dir_path_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses summary-out dir-path case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_summary_out_dir_path_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_summary_out_dir_path_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_summary_out_dir_path_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_dir_path_case_marker.log" >&2
  exit 1
fi

sed -i 's/A24_BATCH_SUMMARY_OUT="${tmp_dir}\/summary_parent_readonly\/summary.log"/A24_BATCH_SUMMARY_OUT_REMOVED="${tmp_dir}\/summary_parent_readonly\/summary.log"/' "${a24_batch_test_script_fail_summary_out_readonly_parent_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_summary_out_readonly_parent_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_summary_out_readonly_parent_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses summary-out readonly-parent case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_summary_out_readonly_parent_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_summary_out_readonly_parent_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_summary_out_readonly_parent_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_readonly_parent_case_marker.log" >&2
  exit 1
fi

sed -i 's/batch_summary_write_probe="\/proc\/1\/cmdline"/batch_summary_write_probe_removed="\/proc\/1\/cmdline"/' "${a24_batch_test_script_fail_summary_out_write_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_summary_out_write_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_summary_out_write_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses summary-out write case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_summary_out_write_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_summary_out_write_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_summary_out_write_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_summary_out_write_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_nonexec_bin=/summary_out_nonexec_bin_removed=/' "${a24_batch_test_script_fail_nonexec_bin_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_nonexec_bin_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_nonexec_bin_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses nonexec-bin case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_nonexec_bin_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_nonexec_bin_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_nonexec_bin_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_nonexec_bin_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_nonexec_bin_case_marker.log" >&2
  exit 1
fi

sed -i 's/export A24_REGRESSION_LOCK_DIR="${tmp_dir}\/a24_regression.lock"/export A24_REGRESSION_LOCK_DIR_REMOVED="${tmp_dir}\/a24_regression.lock"/' "${a24_batch_test_script_fail_regression_lock_dir_isolation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_regression_lock_dir_isolation_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_regression_lock_dir_isolation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses regression lock-dir isolation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_regression_lock_dir_isolation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_regression_lock_dir_isolation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_regression_lock_dir_isolation_marker.log"; then
  echo "FAIL: expected a24_batch_test_regression_lock_dir_isolation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_regression_lock_dir_isolation_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_fallback=/summary_out_log_summary_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary-fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_precedence=/summary_out_log_summary_precedence_removed=/' "${a24_batch_test_script_fail_log_summary_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_precedence_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_precedence_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_malformed_key_precedence=/summary_out_log_summary_malformed_key_precedence_removed=/' "${a24_batch_test_script_fail_log_summary_malformed_key_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_malformed_key_precedence_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_key_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses malformed-key log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_key_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_malformed_key_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_key_precedence_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_malformed_key_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_key_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_duplicate_key_precedence=/summary_out_log_summary_duplicate_key_precedence_removed=/' "${a24_batch_test_script_fail_log_summary_duplicate_key_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_duplicate_key_precedence_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_duplicate_key_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses duplicate-key log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_duplicate_key_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_duplicate_key_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_duplicate_key_precedence_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_duplicate_key_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_duplicate_key_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_malformed_unknown_fallback=/summary_out_log_summary_malformed_unknown_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_malformed_unknown_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_unknown_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses malformed+unknown fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_unknown_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_malformed_unknown_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_unknown_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_malformed_unknown_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_malformed_unknown_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_uppercase=/summary_out_log_summary_uppercase_removed=/' "${a24_batch_test_script_fail_log_summary_uppercase_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_uppercase_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary uppercase case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_uppercase_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_uppercase_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_uppercase_precedence=/summary_out_log_summary_uppercase_precedence_removed=/' "${a24_batch_test_script_fail_log_summary_uppercase_precedence_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_uppercase_precedence_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_precedence_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses uppercase log-summary precedence case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_precedence_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_uppercase_precedence_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_precedence_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_uppercase_precedence_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_uppercase_precedence_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_crlf_fallback=/summary_out_log_summary_crlf_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_crlf_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_crlf_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_crlf_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary CRLF fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_crlf_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_crlf_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_crlf_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_crlf_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_crlf_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_tab_whitespace=/summary_out_log_summary_tab_whitespace_removed=/' "${a24_batch_test_script_fail_log_summary_tab_whitespace_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_tab_whitespace_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_tab_whitespace_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary tab-whitespace case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_tab_whitespace_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_tab_whitespace_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_tab_whitespace_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_tab_whitespace_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_tab_whitespace_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_equals_whitespace_fallback=/summary_out_log_summary_equals_whitespace_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_equals_whitespace_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_equals_whitespace_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary equals-whitespace fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_equals_whitespace_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_equals_whitespace_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_equals_whitespace_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_equals_whitespace_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_equals_whitespace_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_value_fallback=/summary_out_log_summary_quoted_value_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_quoted_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_quoted_value_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary quoted-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_quoted_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_quoted_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_single_quote_fallback=/summary_out_log_summary_single_quote_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_single_quote_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_single_quote_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quote_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary single-quote fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quote_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_single_quote_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quote_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_single_quote_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quote_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quote_mixed_fallback=/summary_out_log_summary_quote_mixed_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_quote_mixed_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_quote_mixed_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_quote_mixed_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary quote-mixed fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quote_mixed_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_quote_mixed_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quote_mixed_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_quote_mixed_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quote_mixed_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_backslash_value_fallback=/summary_out_log_summary_backslash_value_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_backslash_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_backslash_value_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_backslash_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary backslash-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_backslash_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_backslash_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_backslash_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_backslash_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_backslash_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_key_fallback=/summary_out_log_summary_quoted_key_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_quoted_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_quoted_key_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary quoted-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_quoted_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_quoted_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_single_quoted_key_fallback=/summary_out_log_summary_single_quoted_key_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_single_quoted_key_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quoted_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary single-quoted-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_single_quoted_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quoted_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_single_quoted_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_single_quoted_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_quoted_cmd_key_fallback=/summary_out_log_summary_quoted_cmd_key_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_quoted_cmd_key_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary quoted command-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_partial_fallback=/summary_out_log_summary_partial_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_partial_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_partial_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_partial_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary partial-fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_partial_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_partial_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_partial_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_partial_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_partial_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_missing_equals_fallback=/summary_out_log_summary_missing_equals_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_missing_equals_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_missing_equals_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_missing_equals_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary missing-equals fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_missing_equals_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_missing_equals_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_missing_equals_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_missing_equals_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_missing_equals_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_empty_value_fallback=/summary_out_log_summary_empty_value_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_empty_value_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_empty_value_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_value_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary empty-value fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_value_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_empty_value_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_value_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_empty_value_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_value_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_empty_key_fallback=/summary_out_log_summary_empty_key_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_empty_key_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_empty_key_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_key_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary empty-key fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_key_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_empty_key_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_key_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_empty_key_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_empty_key_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_extra_equals_fallback=/summary_out_log_summary_extra_equals_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_extra_equals_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_extra_equals_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_extra_equals_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary extra-equals fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_extra_equals_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_extra_equals_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_extra_equals_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_extra_equals_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_extra_equals_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_leading_punct_fallback=/summary_out_log_summary_leading_punct_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_leading_punct_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_leading_punct_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_leading_punct_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary leading-punct fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_leading_punct_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_leading_punct_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_leading_punct_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_leading_punct_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_leading_punct_fallback_case_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out_log_summary_internal_symbol_fallback=/summary_out_log_summary_internal_symbol_fallback_removed=/' "${a24_batch_test_script_fail_log_summary_internal_symbol_fallback_case_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_pass}" "${b8_knob_matrix_script_pass}" "${b8_guard_script_pass}" "${b8_guard_contract_test_script_pass}" "${b8_guard_test_script_pass}" "${a24_regression_test_script_pass}" "${a24_batch_test_script_fail_log_summary_internal_symbol_fallback_case_marker}" "${a24_regression_full_test_script_pass}" >"${tmp_dir}/contract_fail_a24_batch_test_log_summary_internal_symbol_fallback_case_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch-test script misses log-summary internal-symbol fallback case marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_internal_symbol_fallback_case_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_test_log_summary_internal_symbol_fallback_case_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_test_log_summary_internal_symbol_fallback_case_marker.log"; then
  echo "FAIL: expected a24_batch_test_log_summary_internal_symbol_fallback_case_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_test_log_summary_internal_symbol_fallback_case_marker.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${tmp_dir}/missing_run_a24_regression_full.sh" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_missing_a24_full_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full regression script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_full_script.log" >&2
  exit 1
fi

if ! grep -q "a24 full regression script missing" "${tmp_dir}/contract_fail_missing_a24_full_script.log"; then
  echo "FAIL: expected missing a24 full regression script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_full_script.log" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${tmp_dir}/missing_run_a24_batch.sh" >"${tmp_dir}/contract_fail_missing_a24_batch_script.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script path does not exist" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_batch_script.log" >&2
  exit 1
fi

if ! grep -q "a24 batch script missing" "${tmp_dir}/contract_fail_missing_a24_batch_script.log"; then
  echo "FAIL: expected missing a24 batch script error was not found" >&2
  cat "${tmp_dir}/contract_fail_missing_a24_batch_script.log" >&2
  exit 1
fi

sed -i 's/if make -C FEM4C mbd_a24_regression >"\${nested_regression_log}" 2>\&1; then/if make -C FEM4C mbd_a24_regression_missing >"\${nested_regression_log}" 2>\&1; then/' "${a24_regression_full_script_fail_command}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_command}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_regression_command.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full regression script misses mbd_a24_regression command" >&2
  cat "${tmp_dir}/contract_fail_a24_full_regression_command.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_regression_cmd_a24\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_regression_command.log"; then
  echo "FAIL: expected a24_full_regression_cmd_a24 failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_regression_command.log" >&2
  exit 1
fi

sed -i 's/A24 full summary output directory does not exist/full summary output directory validation removed/' "${a24_regression_full_script_fail_summary_out_dir_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_summary_out_dir_validation_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_summary_out_dir_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses summary-out missing-dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_summary_out_dir_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_summary_out_dir_validation_marker.log"; then
  echo "FAIL: expected a24_full_summary_out_dir_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 full summary output path must be a file/full summary output path-type validation removed/' "${a24_regression_full_script_fail_summary_out_type_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_summary_out_type_validation_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_summary_out_type_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses summary-out path-type validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_type_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_summary_out_type_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_summary_out_type_validation_marker.log"; then
  echo "FAIL: expected a24_full_summary_out_type_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_type_validation_marker.log" >&2
  exit 1
fi

sed -i 's/cannot write A24 full summary output/full summary output write validation removed/' "${a24_regression_full_script_fail_summary_out_write_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_summary_out_write_validation_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_summary_out_write_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses summary-out write validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_write_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_summary_out_write_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_summary_out_write_validation_marker.log"; then
  echo "FAIL: expected a24_full_summary_out_write_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_summary_out_write_validation_marker.log" >&2
  exit 1
fi

sed -i 's/retry_on_137="${A24_FULL_RETRY_ON_137:-1}"/retry_on_137_removed="${A24_FULL_RETRY_ON_137:-1}"/' "${a24_regression_full_script_fail_retry_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_retry_knob_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_retry_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses retry-knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_retry_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_retry_knob_marker.log"; then
  echo "FAIL: expected a24_full_retry_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_FULL_RETRY_ON_137 must be 0 or 1/A24_FULL_RETRY_ON_137 validation removed/' "${a24_regression_full_script_fail_retry_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_retry_validation_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_retry_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses retry-validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_retry_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_retry_validation_marker.log"; then
  echo "FAIL: expected a24_full_retry_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_validation_marker.log" >&2
  exit 1
fi

sed -i 's/retry_used=/retry_used_removed=/' "${a24_regression_full_script_fail_retry_used_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_retry_used_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_retry_used_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses retry-used marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_used_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_retry_used_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_retry_used_marker.log"; then
  echo "FAIL: expected a24_full_retry_used_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_retry_used_marker.log" >&2
  exit 1
fi

sed -i 's/local_failed_step="regression_${nested_failed_step}"/local_failed_step_removed="regression_${nested_failed_step}"/' "${a24_regression_full_script_fail_nested_failed_step_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_failed_step_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_failed_step_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested failed-step propagation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_failed_step_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_failed_step_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_failed_step_marker.log"; then
  echo "FAIL: expected a24_full_nested_failed_step_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_failed_step_marker.log" >&2
  exit 1
fi

sed -i 's/local_failed_cmd="${nested_failed_cmd}"/local_failed_cmd_removed="${nested_failed_cmd}"/' "${a24_regression_full_script_fail_nested_failed_cmd_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_failed_cmd_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_failed_cmd_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested failed-cmd propagation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_failed_cmd_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_failed_cmd_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_failed_cmd_marker.log"; then
  echo "FAIL: expected a24_full_nested_failed_cmd_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_failed_cmd_marker.log" >&2
  exit 1
fi

sed -i 's/nested mbd_integrator_checks failed; verify concurrent make clean\/build interference or FEM4C_MBD_BIN path/nested-integrator-hint-removed/' "${a24_regression_full_script_fail_nested_integrator_hint_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_integrator_hint_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_integrator_hint_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested integrator diagnosis hint marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_integrator_hint_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_integrator_hint_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_integrator_hint_marker.log"; then
  echo "FAIL: expected a24_full_nested_integrator_hint_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_integrator_hint_marker.log" >&2
  exit 1
fi

sed -i 's/extract_nested_regression_failure_from_log()/extract_nested_regression_failure_from_log_removed()/' "${a24_regression_full_script_fail_nested_log_fallback_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_fallback_function_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_fallback_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-fallback function marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_fallback_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_function_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_fallback_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_function_marker.log" >&2
  exit 1
fi

sed -i 's/grep -qi "requires executable fem4c binary"/grep -q "requires executable fem4c binary"/' "${a24_regression_full_script_fail_nested_log_fallback_pattern_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_fallback_pattern_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_fallback_pattern_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-fallback pattern marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_pattern_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_fallback_pattern_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_pattern_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_fallback_pattern_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_pattern_marker.log" >&2
  exit 1
fi

sed -i 's/elif extract_nested_regression_failure_from_log "${nested_regression_log}"; then/elif extract_nested_regression_failure_from_log_removed "${nested_regression_log}"; then/' "${a24_regression_full_script_fail_nested_log_fallback_call_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_fallback_call_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_fallback_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-fallback call marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_fallback_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_call_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_fallback_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_fallback_call_marker.log" >&2
  exit 1
fi

sed -i 's/parse_nested_regression_summary_line() {/parse_nested_regression_summary_line_removed() {/' "${a24_regression_full_script_fail_nested_log_summary_parser_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_parser_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_parser_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary parser marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parser_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_parser_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parser_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_parser_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parser_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="${summary_line/summary_line_removed="${summary_line/' "${a24_regression_full_script_fail_nested_log_summary_crlf_trim_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_crlf_trim_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_crlf_trim_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary CRLF trim marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_crlf_trim_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_crlf_trim_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_crlf_trim_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_crlf_trim_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_crlf_trim_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="$(grep -E '\''^A24_REGRESSION_SUMMARY/summary_line_removed="$(grep -E '\''^A24_REGRESSION_SUMMARY/' "${a24_regression_full_script_fail_nested_log_summary_line_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_line_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_line_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary line marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_line_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_line_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_line_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_line_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_line_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="$(grep -Ei '\''^A24_REGRESSION_SUMMARY/summary_line_removed="$(grep -Ei '\''^A24_REGRESSION_SUMMARY/' "${a24_regression_full_script_fail_nested_log_summary_casefold_line_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_casefold_line_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_line_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary casefold line marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_line_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_casefold_line_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_line_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_casefold_line_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_line_marker.log" >&2
  exit 1
fi

sed -i 's/case "${key,,}" in/case_removed "${key,,}" in/' "${a24_regression_full_script_fail_nested_log_summary_casefold_token_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_casefold_token_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_token_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary token-casefold marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_token_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_casefold_token_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_token_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_casefold_token_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_casefold_token_marker.log" >&2
  exit 1
fi

sed -i 's/"${token}" !=/"${token}" !__removed__/' "${a24_regression_full_script_fail_nested_log_summary_token_equals_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_token_equals_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_token_equals_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary token '=' guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_token_equals_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_token_equals_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_token_equals_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_token_equals_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_token_equals_guard_marker.log" >&2
  exit 1
fi

sed -i 's/\[\[ ! "${key}" =~/\[\[ !__removed__ "${key}" =~/' "${a24_regression_full_script_fail_nested_log_summary_key_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_key_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_key_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary key guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_key_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_key_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_key_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_key_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_key_guard_marker.log" >&2
  exit 1
fi

sed -i 's/"${value}" == \*"="\*/"${value}" == __removed_equals_guard__/' "${a24_regression_full_script_fail_nested_log_summary_value_equals_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_value_equals_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_equals_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary value '=' guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_equals_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_value_equals_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_equals_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_value_equals_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_equals_guard_marker.log" >&2
  exit 1
fi

sed -i 's/\*\\\\\* \]\]; then/__removed_quote_guard__ ]]; then/' "${a24_regression_full_script_fail_nested_log_summary_quote_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_quote_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_quote_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary quote-guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_quote_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_quote_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_quote_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_quote_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_quote_guard_marker.log" >&2
  exit 1
fi

sed -i 's#if \[\[ ! "${value}" =~ ^\[a-z0-9_]+$ \]\]; then#if [[ ! "${value}" =~ ^removed_charset$ ]]; then#' "${a24_regression_full_script_fail_nested_log_summary_value_charset_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_value_charset_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_charset_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary value charset guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_charset_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_value_charset_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_charset_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_value_charset_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_value_charset_guard_marker.log" >&2
  exit 1
fi

sed -i 's/Treat partial nested summary as invalid and defer to generic log fallback./nested-log-summary-partial-guard-marker-removed/' "${a24_regression_full_script_fail_nested_log_summary_partial_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_partial_guard_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_partial_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested summary partial guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_partial_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_partial_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_partial_guard_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_partial_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_partial_guard_marker.log" >&2
  exit 1
fi

sed -i 's/Prefer explicit nested summary fields over generic preflight log fallback./nested-log-summary-precedence-marker-removed/' "${a24_regression_full_script_fail_nested_log_summary_precedence_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_precedence_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_precedence_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary precedence marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_precedence_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_precedence_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_precedence_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_precedence_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_precedence_marker.log" >&2
  exit 1
fi

sed -i 's/if parse_nested_regression_summary_line "${summary_line}"; then/if parse_nested_regression_summary_line_removed "${summary_line}"; then/' "${a24_regression_full_script_fail_nested_log_summary_parse_call_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_fail_nested_log_summary_parse_call_marker}" "${a24_batch_script_pass}" >"${tmp_dir}/contract_fail_a24_full_nested_log_summary_parse_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 full script misses nested log-summary parse-call marker" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parse_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_full_nested_log_summary_parse_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parse_call_marker.log"; then
  echo "FAIL: expected a24_full_nested_log_summary_parse_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_full_nested_log_summary_parse_call_marker.log" >&2
  exit 1
fi

sed -i 's/FAIL: a24 batch lock is already held/FAIL: a24 batch lock marker removed/' "${a24_batch_script_fail_lock_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_lock_marker}" >"${tmp_dir}/contract_fail_a24_batch_lock_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses lock-fail marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_lock_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_lock_fail_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_lock_marker.log"; then
  echo "FAIL: expected a24_batch_lock_fail_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_lock_marker.log" >&2
  exit 1
fi

sed -i 's/lock_pid_file="${lock_dir}\/pid"/lock_pid_file_removed="${lock_dir}\/pid"/' "${a24_batch_script_fail_lock_pid_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_lock_pid_marker}" >"${tmp_dir}/contract_fail_a24_batch_lock_pid_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses lock pid marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_lock_pid_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_lock_pid_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_lock_pid_marker.log"; then
  echo "FAIL: expected a24_batch_lock_pid_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_lock_pid_marker.log" >&2
  exit 1
fi

sed -i 's/INFO: recovered stale a24 batch lock/INFO: stale lock marker removed/' "${a24_batch_script_fail_stale_recovery_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_stale_recovery_marker}" >"${tmp_dir}/contract_fail_a24_batch_stale_recovery_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses stale-recovery marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_stale_recovery_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_stale_recovery_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_stale_recovery_marker.log"; then
  echo "FAIL: expected a24_batch_stale_recovery_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_stale_recovery_marker.log" >&2
  exit 1
fi

sed -i 's/summary_out="${A24_BATCH_SUMMARY_OUT:-}"/summary_out_removed="${A24_BATCH_SUMMARY_OUT:-}"/' "${a24_batch_script_fail_summary_out_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_summary_out_marker}" >"${tmp_dir}/contract_fail_a24_batch_summary_out_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses summary-out marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_summary_out_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_summary_out_marker.log"; then
  echo "FAIL: expected a24_batch_summary_out_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_marker.log" >&2
  exit 1
fi

sed -i 's/A24 batch summary output directory does not exist/batch summary output directory validation removed/' "${a24_batch_script_fail_summary_out_dir_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_summary_out_dir_validation_marker}" >"${tmp_dir}/contract_fail_a24_batch_summary_out_dir_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses summary-out missing-dir validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_summary_out_dir_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_summary_out_dir_validation_marker.log"; then
  echo "FAIL: expected a24_batch_summary_out_dir_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_dir_validation_marker.log" >&2
  exit 1
fi

sed -i 's/A24 batch summary output path must be a file/batch summary output path-type validation removed/' "${a24_batch_script_fail_summary_out_type_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_summary_out_type_validation_marker}" >"${tmp_dir}/contract_fail_a24_batch_summary_out_type_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses summary-out path-type validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_type_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_summary_out_type_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_summary_out_type_validation_marker.log"; then
  echo "FAIL: expected a24_batch_summary_out_type_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_type_validation_marker.log" >&2
  exit 1
fi

sed -i 's/cannot write A24 batch summary output/batch summary output write validation removed/' "${a24_batch_script_fail_summary_out_write_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_summary_out_write_validation_marker}" >"${tmp_dir}/contract_fail_a24_batch_summary_out_write_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses summary-out write validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_write_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_summary_out_write_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_summary_out_write_validation_marker.log"; then
  echo "FAIL: expected a24_batch_summary_out_write_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_summary_out_write_validation_marker.log" >&2
  exit 1
fi

sed -i 's/export MAKEFLAGS="-j1"/export MAKEFLAGS="-j2"/' "${a24_batch_script_fail_makeflags_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_makeflags_marker}" >"${tmp_dir}/contract_fail_a24_batch_makeflags_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses serial makeflags marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_makeflags_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_makeflags_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_makeflags_marker.log"; then
  echo "FAIL: expected a24_batch_makeflags_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_makeflags_marker.log" >&2
  exit 1
fi

sed -i 's/overall=\${overall} failed_step=\${failed_step} failed_cmd=\${failed_cmd}/overall=\${overall} failed_step=\${failed_step} failed_cmd_removed=\${failed_cmd}/' "${a24_batch_script_fail_failed_cmd_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_failed_cmd_marker}" >"${tmp_dir}/contract_fail_a24_batch_failed_cmd_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses failed-cmd marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_failed_cmd_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_failed_cmd_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_failed_cmd_marker.log"; then
  echo "FAIL: expected a24_batch_failed_cmd_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_failed_cmd_marker.log" >&2
  exit 1
fi

sed -i 's/retry_on_137="${A24_BATCH_RETRY_ON_137:-1}"/retry_on_137_removed="${A24_BATCH_RETRY_ON_137:-1}"/' "${a24_batch_script_fail_retry_knob_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_retry_knob_marker}" >"${tmp_dir}/contract_fail_a24_batch_retry_knob_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses retry-knob marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_knob_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_retry_knob_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_retry_knob_marker.log"; then
  echo "FAIL: expected a24_batch_retry_knob_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_knob_marker.log" >&2
  exit 1
fi

sed -i 's/A24_BATCH_RETRY_ON_137 must be 0 or 1/A24_BATCH_RETRY_ON_137 validation removed/' "${a24_batch_script_fail_retry_validation_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_retry_validation_marker}" >"${tmp_dir}/contract_fail_a24_batch_retry_validation_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses retry-validation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_validation_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_retry_validation_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_retry_validation_marker.log"; then
  echo "FAIL: expected a24_batch_retry_validation_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_validation_marker.log" >&2
  exit 1
fi

sed -i 's/retry_used=/retry_used_removed=/' "${a24_batch_script_fail_retry_used_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_retry_used_marker}" >"${tmp_dir}/contract_fail_a24_batch_retry_used_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses retry-used marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_used_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_retry_used_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_retry_used_marker.log"; then
  echo "FAIL: expected a24_batch_retry_used_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_retry_used_marker.log" >&2
  exit 1
fi

sed -i 's/local_failed_step="regression_${nested_failed_step}"/local_failed_step_removed="regression_${nested_failed_step}"/' "${a24_batch_script_fail_nested_failed_step_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_failed_step_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_failed_step_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested failed-step propagation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_failed_step_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_failed_step_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_failed_step_marker.log"; then
  echo "FAIL: expected a24_batch_nested_failed_step_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_failed_step_marker.log" >&2
  exit 1
fi

sed -i 's/local_failed_cmd="${nested_failed_cmd}"/local_failed_cmd_removed="${nested_failed_cmd}"/' "${a24_batch_script_fail_nested_failed_cmd_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_failed_cmd_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_failed_cmd_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested failed-cmd propagation marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_failed_cmd_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_failed_cmd_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_failed_cmd_marker.log"; then
  echo "FAIL: expected a24_batch_nested_failed_cmd_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_failed_cmd_marker.log" >&2
  exit 1
fi

sed -i 's/nested mbd_integrator_checks failed; verify concurrent make clean\/build interference or FEM4C_MBD_BIN path/nested-integrator-hint-removed/' "${a24_batch_script_fail_nested_integrator_hint_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_integrator_hint_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_integrator_hint_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested integrator diagnosis hint marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_integrator_hint_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_integrator_hint_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_integrator_hint_marker.log"; then
  echo "FAIL: expected a24_batch_nested_integrator_hint_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_integrator_hint_marker.log" >&2
  exit 1
fi

sed -i 's/extract_nested_regression_failure_from_log()/extract_nested_regression_failure_from_log_removed()/' "${a24_batch_script_fail_nested_log_fallback_function_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_fallback_function_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_function_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-fallback function marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_function_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_fallback_function_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_function_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_fallback_function_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_function_marker.log" >&2
  exit 1
fi

sed -i 's/grep -qi "requires executable fem4c binary"/grep -q "requires executable fem4c binary"/' "${a24_batch_script_fail_nested_log_fallback_pattern_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_fallback_pattern_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_pattern_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-fallback pattern marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_pattern_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_fallback_pattern_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_pattern_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_fallback_pattern_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_pattern_marker.log" >&2
  exit 1
fi

sed -i 's/elif extract_nested_regression_failure_from_log "${nested_regression_log}"; then/elif extract_nested_regression_failure_from_log_removed "${nested_regression_log}"; then/' "${a24_batch_script_fail_nested_log_fallback_call_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_fallback_call_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-fallback call marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_fallback_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_call_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_fallback_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_fallback_call_marker.log" >&2
  exit 1
fi

sed -i 's/parse_nested_regression_summary_line() {/parse_nested_regression_summary_line_removed() {/' "${a24_batch_script_fail_nested_log_summary_parser_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_parser_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parser_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary parser marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parser_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_parser_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parser_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_parser_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parser_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="${summary_line/summary_line_removed="${summary_line/' "${a24_batch_script_fail_nested_log_summary_crlf_trim_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_crlf_trim_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_crlf_trim_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary CRLF trim marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_crlf_trim_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_crlf_trim_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_crlf_trim_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_crlf_trim_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_crlf_trim_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="$(grep -E '\''^A24_REGRESSION_SUMMARY/summary_line_removed="$(grep -E '\''^A24_REGRESSION_SUMMARY/' "${a24_batch_script_fail_nested_log_summary_line_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_line_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_line_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary line marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_line_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_line_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_line_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_line_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_line_marker.log" >&2
  exit 1
fi

sed -i 's/summary_line="$(grep -Ei '\''^A24_REGRESSION_SUMMARY/summary_line_removed="$(grep -Ei '\''^A24_REGRESSION_SUMMARY/' "${a24_batch_script_fail_nested_log_summary_casefold_line_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_casefold_line_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_line_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary casefold line marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_line_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_casefold_line_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_line_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_casefold_line_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_line_marker.log" >&2
  exit 1
fi

sed -i 's/case "${key,,}" in/case_removed "${key,,}" in/' "${a24_batch_script_fail_nested_log_summary_casefold_token_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_casefold_token_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_token_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary token-casefold marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_token_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_casefold_token_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_token_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_casefold_token_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_casefold_token_marker.log" >&2
  exit 1
fi

sed -i 's/"${token}" !=/"${token}" !__removed__/' "${a24_batch_script_fail_nested_log_summary_token_equals_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_token_equals_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_token_equals_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary token '=' guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_token_equals_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_token_equals_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_token_equals_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_token_equals_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_token_equals_guard_marker.log" >&2
  exit 1
fi

sed -i 's/\[\[ ! "${key}" =~/\[\[ !__removed__ "${key}" =~/' "${a24_batch_script_fail_nested_log_summary_key_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_key_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_key_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary key guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_key_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_key_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_key_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_key_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_key_guard_marker.log" >&2
  exit 1
fi

sed -i 's/"${value}" == \*"="\*/"${value}" == __removed_equals_guard__/' "${a24_batch_script_fail_nested_log_summary_value_equals_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_value_equals_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_equals_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary value '=' guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_equals_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_value_equals_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_equals_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_value_equals_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_equals_guard_marker.log" >&2
  exit 1
fi

sed -i 's/\*\\\\\* \]\]; then/__removed_quote_guard__ ]]; then/' "${a24_batch_script_fail_nested_log_summary_quote_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_quote_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_quote_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary quote-guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_quote_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_quote_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_quote_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_quote_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_quote_guard_marker.log" >&2
  exit 1
fi

sed -i 's#if \[\[ ! "${value}" =~ ^\[a-z0-9_]+$ \]\]; then#if [[ ! "${value}" =~ ^removed_charset$ ]]; then#' "${a24_batch_script_fail_nested_log_summary_value_charset_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_value_charset_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_charset_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary value charset guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_charset_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_value_charset_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_charset_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_value_charset_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_value_charset_guard_marker.log" >&2
  exit 1
fi

sed -i 's/Treat partial nested summary as invalid and defer to generic log fallback./nested-log-summary-partial-guard-marker-removed/' "${a24_batch_script_fail_nested_log_summary_partial_guard_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_partial_guard_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_partial_guard_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested summary partial guard marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_partial_guard_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_partial_guard_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_partial_guard_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_partial_guard_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_partial_guard_marker.log" >&2
  exit 1
fi

sed -i 's/Prefer explicit nested summary fields over generic preflight log fallback./nested-log-summary-precedence-marker-removed/' "${a24_batch_script_fail_nested_log_summary_precedence_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_precedence_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_precedence_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary precedence marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_precedence_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_precedence_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_precedence_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_precedence_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_precedence_marker.log" >&2
  exit 1
fi

sed -i 's/if parse_nested_regression_summary_line "${summary_line}"; then/if parse_nested_regression_summary_line_removed "${summary_line}"; then/' "${a24_batch_script_fail_nested_log_summary_parse_call_marker}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_nested_log_summary_parse_call_marker}" >"${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parse_call_marker.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses nested log-summary parse-call marker" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parse_call_marker.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_nested_log_summary_parse_call_marker\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parse_call_marker.log"; then
  echo "FAIL: expected a24_batch_nested_log_summary_parse_call_marker failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_nested_log_summary_parse_call_marker.log" >&2
  exit 1
fi

sed -i 's/if make -C FEM4C mbd_a24_regression >"${nested_regression_log}" 2>\&1; then/if make -C FEM4C mbd_a24_regression_missing >"${nested_regression_log}" 2>\&1; then/' "${a24_batch_script_fail_regression_command}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_regression_command}" >"${tmp_dir}/contract_fail_a24_batch_regression_command.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses mbd_a24_regression command" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_regression_command.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_cmd_a24\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_regression_command.log"; then
  echo "FAIL: expected a24_batch_cmd_a24 failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_regression_command.log" >&2
  exit 1
fi

sed -i 's/if make -C FEM4C mbd_a24_regression_full_test; then/if make -C FEM4C mbd_a24_regression_full_test_missing; then/' "${a24_batch_script_fail_command}"

if bash "FEM4C/scripts/check_ci_contract.sh" "${workflow_copy}" "${makefile_pass}" "${marker_script_pass}" "${mbd_integrator_script_pass}" "${a24_regression_script_pass}" "${b8_regression_script_pass}" "${b8_regression_full_script_pass}" "${a24_regression_full_script_pass}" "${a24_batch_script_fail_command}" >"${tmp_dir}/contract_fail_a24_batch_command.log" 2>&1; then
  echo "FAIL: check_ci_contract should fail when a24 batch script misses mbd_a24_regression_full_test command" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_command.log" >&2
  exit 1
fi

if ! grep -q "CI_CONTRACT_CHECK\\[a24_batch_cmd_a24_full_test\\]=FAIL" "${tmp_dir}/contract_fail_a24_batch_command.log"; then
  echo "FAIL: expected a24_batch_cmd_a24_full_test failure marker was not found" >&2
  cat "${tmp_dir}/contract_fail_a24_batch_command.log" >&2
  exit 1
fi

echo "PASS: check_ci_contract self-test (pass case + expected fail cases)"
