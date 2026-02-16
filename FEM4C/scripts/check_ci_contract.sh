#!/usr/bin/env bash
set -euo pipefail

workflow="${1:-.github/workflows/ci.yaml}"
makefile_path="${2:-FEM4C/Makefile}"
marker_script_path="${3:-FEM4C/scripts/check_fem4c_test_log_markers.sh}"
mbd_integrator_script_path="${4:-FEM4C/scripts/check_mbd_integrators.sh}"
a24_regression_script_path="${5:-FEM4C/scripts/run_a24_regression.sh}"
b8_regression_script_path="${6:-FEM4C/scripts/run_b8_regression.sh}"
b8_regression_full_script_path="${7:-FEM4C/scripts/run_b8_regression_full.sh}"
a24_regression_full_script_path="${8:-FEM4C/scripts/run_a24_regression_full.sh}"
a24_batch_script_path="${9:-FEM4C/scripts/run_a24_batch.sh}"
b8_knob_matrix_script_path="${10:-FEM4C/scripts/test_b8_knob_matrix.sh}"
b8_guard_script_path="${11:-FEM4C/scripts/run_b8_guard.sh}"
b8_guard_contract_test_script_path="${12:-FEM4C/scripts/test_run_b8_guard_contract.sh}"
b8_guard_test_script_path="${13:-FEM4C/scripts/test_run_b8_guard.sh}"
a24_regression_test_script_path="${14:-FEM4C/scripts/test_run_a24_regression.sh}"
a24_batch_test_script_path="${15:-FEM4C/scripts/test_run_a24_batch.sh}"
a24_regression_full_test_script_path="${16:-FEM4C/scripts/test_run_a24_regression_full.sh}"
b8_regression_test_script_path="${17:-FEM4C/scripts/test_run_b8_regression.sh}"
b8_regression_full_test_script_path="${18:-FEM4C/scripts/test_run_b8_regression_full.sh}"
a24_acceptance_serial_script_path="${19:-FEM4C/scripts/run_a24_acceptance_serial.sh}"
a24_acceptance_serial_test_script_path="${20:-FEM4C/scripts/test_run_a24_acceptance_serial.sh}"

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

if [[ ! -f "$workflow" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (workflow missing: $workflow)" >&2
  exit 1
fi

checks=0
fails=0

check_pattern_in_file() {
  local label="$1"
  local pattern="$2"
  local file_path="$3"
  checks=$((checks + 1))
  if rg -n --fixed-strings -- "$pattern" "$file_path" > /dev/null; then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (missing: $pattern in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_regex_in_file() {
  local label="$1"
  local regex="$2"
  local file_path="$3"
  checks=$((checks + 1))
  if rg -n --regexp "$regex" "$file_path" > /dev/null; then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (missing regex: $regex in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_target_dependency() {
  local label="$1"
  local target_name="$2"
  local dependency="$3"
  local file_path="$4"
  checks=$((checks + 1))
  if rg -n --regexp "^${target_name}:[^\r\n]*\\b${dependency}\\b" "$file_path" > /dev/null; then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (missing dependency: ${target_name} -> ${dependency} in ${file_path})" >&2
    fails=$((fails + 1))
  fi
}

if [[ ! -f "$makefile_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (Makefile missing: $makefile_path)" >&2
  exit 1
fi
if [[ ! -f "$marker_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (marker script missing: $marker_script_path)" >&2
  exit 1
fi
if [[ ! -f "$mbd_integrator_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (mbd integrator script missing: $mbd_integrator_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_regression_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 regression script missing: $a24_regression_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_regression_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 regression script missing: $b8_regression_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_regression_full_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 full regression script missing: $b8_regression_full_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_regression_full_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 full regression script missing: $a24_regression_full_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_batch_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 batch script missing: $a24_batch_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_knob_matrix_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 knob matrix script missing: $b8_knob_matrix_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_guard_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 guard script missing: $b8_guard_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_guard_contract_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 guard contract test script missing: $b8_guard_contract_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_guard_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 guard test script missing: $b8_guard_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_regression_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 regression test script missing: $a24_regression_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_batch_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 batch test script missing: $a24_batch_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_regression_full_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 full regression test script missing: $a24_regression_full_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_regression_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 regression test script missing: $b8_regression_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$b8_regression_full_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (b8 full regression test script missing: $b8_regression_full_test_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_acceptance_serial_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 acceptance serial script missing: $a24_acceptance_serial_script_path)" >&2
  exit 1
fi
if [[ ! -f "$a24_acceptance_serial_test_script_path" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (a24 acceptance serial test script missing: $a24_acceptance_serial_test_script_path)" >&2
  exit 1
fi

check_pattern_in_file "step_name" "- name: Run FEM4C regression entrypoint" "$workflow"
check_pattern_in_file "step_id" "id: run_fem4c_tests" "$workflow"
check_pattern_in_file "fem4c_test_log" "fem4c_test.log" "$workflow"
check_pattern_in_file "failure_gate" "if: steps.run_fem4c_tests.outcome == 'failure'" "$workflow"
check_pattern_in_file "test_command" "make -C FEM4C test" "$workflow"
check_pattern_in_file "test_log_gate_script_call" "bash FEM4C/scripts/check_fem4c_test_log_markers.sh fem4c_test.log" "$workflow"
check_pattern_in_file "marker_coupled_integrator" "PASS: coupled integrator switch check" "$marker_script_path"
check_pattern_in_file "marker_mbd_integrator" "PASS: mbd integrator switch check" "$marker_script_path"
check_pattern_in_file "marker_mbd_suite" "PASS: all MBD checks completed" "$marker_script_path"
check_pattern_in_file "artifact_upload" "- name: Upload test log" "$workflow"
check_pattern_in_file "integrator_target" "integrator_checks: \$(TARGET)" "$makefile_path"
check_pattern_in_file "integrator_in_test" "\$(MAKE) integrator_checks" "$makefile_path"
check_pattern_in_file "mbd_integrator_target" "mbd_integrator_checks: \$(TARGET)" "$makefile_path"
check_pattern_in_file "mbd_a21_script_var" "MBD_A21_REGRESSION_SCRIPT = scripts/run_a21_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_a21_target" "mbd_a21_regression:" "$makefile_path"
check_pattern_in_file "mbd_a21_test_script_var" "MBD_A21_REGRESSION_TEST_SCRIPT = scripts/test_run_a21_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_a21_test_target" "mbd_a21_regression_test:" "$makefile_path"
check_pattern_in_file "mbd_a24_script_var" "MBD_A24_REGRESSION_SCRIPT = scripts/run_a24_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_target" "mbd_a24_regression:" "$makefile_path"
check_pattern_in_file "mbd_a24_test_script_var" "MBD_A24_REGRESSION_TEST_SCRIPT = scripts/test_run_a24_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_test_target" "mbd_a24_regression_test:" "$makefile_path"
check_pattern_in_file "mbd_a24_full_script_var" "MBD_A24_REGRESSION_FULL_SCRIPT = scripts/run_a24_regression_full.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_full_target" "mbd_a24_regression_full:" "$makefile_path"
check_pattern_in_file "mbd_a24_full_test_script_var" "MBD_A24_REGRESSION_FULL_TEST_SCRIPT = scripts/test_run_a24_regression_full.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_full_test_target" "mbd_a24_regression_full_test:" "$makefile_path"
check_pattern_in_file "mbd_a24_batch_script_var" "MBD_A24_BATCH_SCRIPT = scripts/run_a24_batch.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_batch_target" "mbd_a24_batch:" "$makefile_path"
check_pattern_in_file "mbd_a24_batch_test_script_var" "MBD_A24_BATCH_TEST_SCRIPT = scripts/test_run_a24_batch.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_batch_test_target" "mbd_a24_batch_test:" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_script_var" "MBD_A24_ACCEPTANCE_SERIAL_SCRIPT = scripts/run_a24_acceptance_serial.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_target" "mbd_a24_acceptance_serial:" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_test_script_var" "MBD_A24_ACCEPTANCE_SERIAL_TEST_SCRIPT = scripts/test_run_a24_acceptance_serial.sh" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_test_target" "mbd_a24_acceptance_serial_test:" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_help" "mbd_a24_acceptance_serial - Run A-24 serial acceptance" "$makefile_path"
check_pattern_in_file "mbd_a24_acceptance_serial_test_help" "mbd_a24_acceptance_serial_test - Self-test for A-24 serial acceptance wrapper" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_test_script_var" "B8_GUARD_TEST_SCRIPT = scripts/test_run_b8_guard.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_test_target" "mbd_b8_guard_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_contract_script_var" "B8_GUARD_CONTRACT_SCRIPT = scripts/run_b8_guard_contract.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_contract_target" "mbd_b8_guard_contract:" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_contract_test_script_var" "B8_GUARD_CONTRACT_TEST_SCRIPT = scripts/test_run_b8_guard_contract.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_guard_contract_test_target" "mbd_b8_guard_contract_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_syntax_script_var" "B8_SYNTAX_CHECK_SCRIPT = scripts/check_b8_scripts_syntax.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_syntax_target" "mbd_b8_syntax:" "$makefile_path"
check_pattern_in_file "mbd_b8_output_test_script_var" "B8_OUTPUT_TEST_SCRIPT = scripts/test_check_b8_guard_output.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_output_test_target" "mbd_b8_guard_output_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_script_var" "MBD_B8_REGRESSION_SCRIPT = scripts/run_b8_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_target" "mbd_b8_regression:" "$makefile_path"
check_pattern_in_file "mbd_b8_test_script_var" "MBD_B8_REGRESSION_TEST_SCRIPT = scripts/test_run_b8_regression.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_test_target" "mbd_b8_regression_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_full_script_var" "MBD_B8_REGRESSION_FULL_SCRIPT = scripts/run_b8_regression_full.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_full_target" "mbd_b8_regression_full:" "$makefile_path"
check_pattern_in_file "mbd_b8_full_test_script_var" "MBD_B8_REGRESSION_FULL_TEST_SCRIPT = scripts/test_run_b8_regression_full.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_full_test_target" "mbd_b8_regression_full_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_knob_matrix_test_script_var" "MBD_B8_KNOB_MATRIX_TEST_SCRIPT = scripts/test_b8_knob_matrix.sh" "$makefile_path"
check_pattern_in_file "mbd_b8_knob_matrix_test_target" "mbd_b8_knob_matrix_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_knob_matrix_smoke_test_target" "mbd_b8_knob_matrix_smoke_test:" "$makefile_path"
check_pattern_in_file "mbd_b8_knob_matrix_smoke_skip_flag" "B8_KNOB_MATRIX_SKIP_FULL=1 bash \$(MBD_B8_KNOB_MATRIX_TEST_SCRIPT)" "$makefile_path"
check_pattern_in_file "mbd_b8_make_knob_pass_through" 'B8_MAKE_CMD=$(if $(B8_MAKE_CMD),$(B8_MAKE_CMD),make)' "$makefile_path"
check_pattern_in_file "mbd_b8_b14_knob_pass_through" 'B8_RUN_B14_REGRESSION=$(if $(B8_RUN_B14_REGRESSION),$(B8_RUN_B14_REGRESSION),1)' "$makefile_path"
check_pattern_in_file "mbd_b8_local_target_default" 'B8_LOCAL_TARGET=$(if $(B8_LOCAL_TARGET),$(B8_LOCAL_TARGET),mbd_checks)' "$makefile_path"
check_target_dependency "mbd_checks_dep_integrator" "mbd_checks" "mbd_integrator_checks" "$makefile_path"
check_pattern_in_file "mbd_checks_in_test" "\$(MAKE) mbd_checks" "$makefile_path"
check_pattern_in_file "mbd_integrator_env_time_fallback_case" "run_env_time_fallback_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_env_time_whitespace_case" "run_env_time_whitespace_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_env_time_compact_trace_case" "run_env_time_compact_trace_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_cli_compact_trace_case" "run_cli_compact_trace_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_cli_invalid_dt_case" "run_cli_invalid_dt_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_cli_invalid_steps_case" "run_cli_invalid_steps_case" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_time_fallback_marker" "time_fallback: dt=env_invalid_fallback steps=env_out_of_range_fallback" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_time_whitespace_fallback_marker" "time_fallback: dt=env_invalid_fallback steps=env_invalid_fallback" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_compact_trace_marker" "mbd_step=... (14 steps omitted for compact trace)" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_source_status_default_marker" "require_pattern \"mbd_default\" \"\${case_out}\" \"newmark_beta_source_status,default\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_source_status_cli_marker" "require_pattern \"mbd_cli\" \"\${case_out}\" \"newmark_beta_source_status,cli\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_source_status_env_fallback_marker" "require_pattern \"mbd_env_params_fallback\" \"\${case_out}\" \"hht_alpha_source_status,env_out_of_range_fallback\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_time_source_status_default_marker" "require_pattern \"mbd_default\" \"\${case_out}\" \"dt_source_status,default\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_time_source_status_env_fallback_marker" "require_pattern \"mbd_env_time_fallback\" \"\${case_out}\" \"dt_source_status,env_invalid_fallback\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_time_source_status_cli_marker" "require_pattern \"mbd_cli\" \"\${case_out}\" \"dt_source_status,cli\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_fallback_cli_marker" "integrator_fallback: newmark_beta=cli newmark_gamma=cli hht_alpha=cli" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_marker" "require_pattern \"mbd_cli\" \"\${case_log}\" \"mbd_step=1/3\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_summary_marker" "require_pattern \"mbd_cli\" \"\${case_log}\" \"steps_trace: requested=3 executed=3\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_output_requested_marker" "require_pattern \"mbd_cli\" \"\${case_out}\" \"steps_requested,3\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_output_executed_marker" "require_pattern \"mbd_cli\" \"\${case_out}\" \"steps_executed,3\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_compact_marker" "require_pattern \"mbd_cli_compact_trace\" \"\${case_log}\" \"mbd_step=... (14 steps omitted for compact trace)\"" "$mbd_integrator_script_path"
check_pattern_in_file "mbd_step_trace_cli_compact_summary_marker" "require_pattern \"mbd_cli_compact_trace\" \"\${case_log}\" \"steps_trace: requested=20 executed=20\"" "$mbd_integrator_script_path"
check_pattern_in_file "a24_regression_cmd_integrator_checks" 'run_make "integrator_checks" "mbd_integrator_checks" "integrator_attempts"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_cmd_ci_contract" 'run_make "ci_contract" "mbd_ci_contract" "ci_contract_attempts"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_cmd_ci_contract_test" 'run_make "ci_contract_test" "mbd_ci_contract_test" "ci_contract_test_attempts"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_contract_test_knob_marker" 'run_contract_test="${A24_RUN_CONTRACT_TEST:-1}"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_contract_test_knob_validation_marker" "A24_RUN_CONTRACT_TEST must be 0 or 1" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_contract_test_skip_marker" "INFO: skip mbd_ci_contract_test (A24_RUN_CONTRACT_TEST=0)" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_makeflags_isolation_marker" "env -u MAKEFLAGS -u MFLAGS make -C FEM4C" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_summary_marker" "A24_REGRESSION_SUMMARY contract_test=" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_summary_out_marker" 'summary_out="${A24_REGRESSION_SUMMARY_OUT:-}"' "$a24_regression_script_path"
check_regex_in_file "a24_full_regression_cmd_clean" "^(if )?make -C FEM4C clean(; then)?$" "$a24_regression_full_script_path"
check_regex_in_file "a24_full_regression_cmd_build" "^(if )?make -C FEM4C(; then)?$" "$a24_regression_full_script_path"
check_regex_in_file "a24_full_regression_cmd_a24" "^(if )?make -C FEM4C mbd_a24_regression(; then)?$" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_dir_marker" 'lock_dir="${A24_FULL_LOCK_DIR:-${A24_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_batch.lock}}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_pid_marker" 'lock_pid_file="${lock_dir}/pid"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_fail_marker" "FAIL: a24 full lock is already held" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_stale_recovery_marker" "INFO: recovered stale a24 full lock" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_marker" "A24_FULL_SUMMARY lock=" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_out_marker" 'summary_out="${A24_FULL_SUMMARY_OUT:-}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_makeflags_marker" 'export MAKEFLAGS="-j1"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_knob_marker" 'retry_on_137="${A24_FULL_RETRY_ON_137:-1}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_validation_marker" "A24_FULL_RETRY_ON_137 must be 0 or 1" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_used_marker" "retry_used=" "$a24_regression_full_script_path"
check_regex_in_file "a24_batch_cmd_a24" "^(if )?make -C FEM4C mbd_a24_regression(; then)?$" "$a24_batch_script_path"
check_regex_in_file "a24_batch_cmd_a24_test" "^(if )?make -C FEM4C mbd_a24_regression_test(; then)?$" "$a24_batch_script_path"
check_regex_in_file "a24_batch_cmd_a24_full_test" "^(if )?make -C FEM4C mbd_a24_regression_full_test(; then)?$" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_dir_marker" 'lock_dir="${A24_BATCH_LOCK_DIR:-${A24_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_batch.lock}}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_pid_marker" 'lock_pid_file="${lock_dir}/pid"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_fail_marker" "FAIL: a24 batch lock is already held" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_stale_recovery_marker" "INFO: recovered stale a24 batch lock" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_marker" "A24_BATCH_SUMMARY lock=" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_out_marker" 'summary_out="${A24_BATCH_SUMMARY_OUT:-}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_makeflags_marker" 'export MAKEFLAGS="-j1"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_failed_cmd_marker" "failed_cmd=" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_knob_marker" 'retry_on_137="${A24_BATCH_RETRY_ON_137:-1}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_validation_marker" "A24_BATCH_RETRY_ON_137 must be 0 or 1" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_used_marker" "retry_used=" "$a24_batch_script_path"
check_pattern_in_file "b8_guard_makeflags_isolation" "env -u MAKEFLAGS -u MFLAGS" "$b8_guard_script_path"
check_pattern_in_file "b8_guard_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_guard_script_path"
check_pattern_in_file "b8_guard_contract_test_b14_target_override" "B8_B14_TARGET=mbd_ci_contract" "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_guard_contract_test_repo_root_passthrough_marker" 'FEM4C_REPO_ROOT="${root_dir}"' "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_guard_test_makeflags_case_marker" "makeflags_isolation" "$b8_guard_test_script_path"
check_pattern_in_file "b8_regression_knob_validation" "B8_RUN_B14_REGRESSION must be 0 or 1" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_make_cmd_validation" "B8_MAKE_CMD is not executable" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_repo_root_override_marker" 'root_dir="${FEM4C_REPO_ROOT:-}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_repo_root_validate_marker" "FEM4C repo root is invalid" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_target_default" 'b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_target_pass_through" 'B8_B14_TARGET="$b8_b14_target"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_makeflags_isolation" "env -u MAKEFLAGS -u MFLAGS" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_regression_script_path"
check_pattern_in_file "b8_full_regression_knob_validation" "B8_RUN_B14_REGRESSION must be 0 or 1" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_make_cmd_validation" "B8_MAKE_CMD is not executable" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_repo_root_override_marker" 'root_dir="${FEM4C_REPO_ROOT:-}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_repo_root_validate_marker" "FEM4C repo root is invalid" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_b14_target_default" 'b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_b14_target_pass_through" 'B8_B14_TARGET="$b8_b14_target"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_regression_full_script_path"
check_pattern_in_file "b8_knob_matrix_skip_full_validation" "B8_KNOB_MATRIX_SKIP_FULL must be 0 or 1" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_skip_full_info_marker" "INFO: skip full regression matrix cases (B8_KNOB_MATRIX_SKIP_FULL=1)" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_zero_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_one_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_invalid_knob_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_invalid_make_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_zero_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_one_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_invalid_knob_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_invalid_make_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_guard_local_target_default" 'local_target="${B8_LOCAL_TARGET:-mbd_checks}"' "$b8_guard_script_path"
check_pattern_in_file "a24_regression_test_build_preflight_marker" "run_a24_regression self-test requires successful FEM4C build preflight" "$a24_regression_test_script_path"
check_pattern_in_file "a24_batch_test_build_preflight_marker" "run_a24_batch self-test requires successful FEM4C build preflight" "$a24_batch_test_script_path"
check_pattern_in_file "a24_full_test_build_preflight_marker" "run_a24_regression_full self-test requires successful FEM4C build preflight" "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_batch_test_full_chain_marker" "full->batch chain case" "$a24_batch_test_script_path"
check_pattern_in_file "a24_acceptance_serial_summary_marker" "A24_ACCEPT_SERIAL_SUMMARY lock=" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_lock_marker" "a24 acceptance serial lock is already held" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_retry_knob_marker" 'retry_on_137="${A24_ACCEPT_SERIAL_RETRY_ON_137:-1}"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_retry_validation_marker" "A24_ACCEPT_SERIAL_RETRY_ON_137 must be 0 or 1" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_fake_137_step_knob_marker" 'fake_137_step="${A24_ACCEPT_SERIAL_FAKE_137_STEP:-none}"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_fake_137_step_validation_marker" "A24_ACCEPT_SERIAL_FAKE_137_STEP must be one of" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_step_log_dir_knob_marker" 'step_log_dir="${A24_ACCEPT_SERIAL_STEP_LOG_DIR:-}"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_step_log_dir_validation_marker" "cannot create A24 acceptance serial step-log dir" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_step_log_dir_type_validation_marker" "A24 acceptance serial step-log dir must be a directory" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_step_log_dir_writable_validation_marker" "A24 acceptance serial step-log dir is not writable" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_failed_rc_marker" "failed_rc=" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_failed_log_marker" "failed_log=" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_cmd_full_test" '"mbd_a24_regression_full_test"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_cmd_batch_test" '"mbd_a24_batch_test"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_cmd_ci_contract_test" '"mbd_ci_contract_test"' "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_test_build_preflight_marker" "run_a24_acceptance_serial self-test requires successful FEM4C build preflight" "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_retry_knob_case_marker" "A24_ACCEPT_SERIAL_RETRY_ON_137=2" "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_fake_step_case_marker" "A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test" "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_step_log_dir_case_marker" "A24_ACCEPT_SERIAL_STEP_LOG_DIR=" "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_step_log_file_case_marker" 'A24_ACCEPT_SERIAL_STEP_LOG_DIR="${tmp_dir}/step_log_file"' "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_step_log_readonly_case_marker" 'A24_ACCEPT_SERIAL_STEP_LOG_DIR="${tmp_dir}/step_logs_readonly"' "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "b8_regression_test_temp_copy_dir_knob_marker" 'script_copy_dir="${B8_TEST_TMP_COPY_DIR:-${root_dir}/FEM4C/scripts}"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_temp_copy_dir_knob_marker" 'script_copy_dir="${B8_TEST_TMP_COPY_DIR:-${root_dir}/FEM4C/scripts}"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_guard_contract_test_temp_copy_dir_knob_marker" 'script_copy_dir="${B8_TEST_TMP_COPY_DIR:-${root_dir}/FEM4C/scripts}"' "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_regression_test_temp_copy_dir_validate_marker" "B8_TEST_TMP_COPY_DIR does not exist" "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_temp_copy_dir_validate_marker" "B8_TEST_TMP_COPY_DIR does not exist" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_guard_contract_test_temp_copy_dir_validate_marker" "B8_TEST_TMP_COPY_DIR does not exist" "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_regression_test_temp_copy_dir_writable_marker" "B8_TEST_TMP_COPY_DIR is not writable" "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_temp_copy_dir_writable_marker" "B8_TEST_TMP_COPY_DIR is not writable" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_guard_contract_test_temp_copy_dir_writable_marker" "B8_TEST_TMP_COPY_DIR is not writable" "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_regression_test_repo_root_passthrough_marker" 'FEM4C_REPO_ROOT="${root_dir}"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_repo_root_passthrough_marker" 'FEM4C_REPO_ROOT="${root_dir}"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_regression_test_temp_copy_stamp_marker" 'temp_copy_stamp="$$.${RANDOM}"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_temp_copy_stamp_marker" 'temp_copy_stamp="$$.${RANDOM}"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_guard_contract_test_temp_copy_stamp_marker" 'temp_copy_stamp="$$.${RANDOM}"' "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_regression_test_temp_copy_marker" 'mktemp "${script_copy_dir}/.tmp_run_b8_regression_fail.${temp_copy_stamp}.XXXXXX.sh"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_temp_copy_marker" 'mktemp "${script_copy_dir}/.tmp_run_b8_regression_full_fail.${temp_copy_stamp}.XXXXXX.sh"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_guard_contract_test_temp_copy_marker" 'mktemp "${script_copy_dir}/.tmp_run_b8_guard_contract_fail.${temp_copy_stamp}.XXXXXX.sh"' "$b8_guard_contract_test_script_path"

if [[ $fails -ne 0 ]]; then
  echo "CI_CONTRACT_CHECK_SUMMARY=FAIL checks=$checks failed=$fails workflow=$workflow makefile=$makefile_path marker_script=$marker_script_path mbd_integrator_script=$mbd_integrator_script_path a24_regression_script=$a24_regression_script_path a24_full_regression_script=$a24_regression_full_script_path a24_batch_script=$a24_batch_script_path b8_regression_script=$b8_regression_script_path b8_full_regression_script=$b8_regression_full_script_path b8_knob_matrix_script=$b8_knob_matrix_script_path b8_guard_script=$b8_guard_script_path b8_guard_contract_test_script=$b8_guard_contract_test_script_path b8_guard_test_script=$b8_guard_test_script_path a24_regression_test_script=$a24_regression_test_script_path a24_batch_test_script=$a24_batch_test_script_path a24_full_test_script=$a24_regression_full_test_script_path b8_regression_test_script=$b8_regression_test_script_path b8_full_test_script=$b8_regression_full_test_script_path a24_acceptance_serial_script=$a24_acceptance_serial_script_path a24_acceptance_serial_test_script=$a24_acceptance_serial_test_script_path" >&2
  exit 1
fi

echo "CI_CONTRACT_CHECK_SUMMARY=PASS checks=$checks failed=0 workflow=$workflow makefile=$makefile_path marker_script=$marker_script_path mbd_integrator_script=$mbd_integrator_script_path a24_regression_script=$a24_regression_script_path a24_full_regression_script=$a24_regression_full_script_path a24_batch_script=$a24_batch_script_path b8_regression_script=$b8_regression_script_path b8_full_regression_script=$b8_regression_full_script_path b8_knob_matrix_script=$b8_knob_matrix_script_path b8_guard_script=$b8_guard_script_path b8_guard_contract_test_script=$b8_guard_contract_test_script_path b8_guard_test_script=$b8_guard_test_script_path a24_regression_test_script=$a24_regression_test_script_path a24_batch_test_script=$a24_batch_test_script_path a24_full_test_script=$a24_regression_full_test_script_path b8_regression_test_script=$b8_regression_test_script_path b8_full_test_script=$b8_regression_full_test_script_path a24_acceptance_serial_script=$a24_acceptance_serial_script_path a24_acceptance_serial_test_script=$a24_acceptance_serial_test_script_path"
