#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
cleanup() {
  # Avoid leaking nested make/bash processes when the self-test aborts early.
  pkill -P $$ 2>/dev/null || true
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

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
a24_regression_script_pass="${tmp_dir}/run_a24_regression.pass.sh"
a24_regression_script_fail_command="${tmp_dir}/run_a24_regression.fail_command.sh"
a24_regression_script_fail_contract_knob_marker="${tmp_dir}/run_a24_regression.fail_contract_knob_marker.sh"
a24_regression_script_fail_contract_knob_validation_marker="${tmp_dir}/run_a24_regression.fail_contract_knob_validation_marker.sh"
a24_regression_script_fail_contract_skip_marker="${tmp_dir}/run_a24_regression.fail_contract_skip_marker.sh"
a24_regression_script_fail_makeflags_isolation_marker="${tmp_dir}/run_a24_regression.fail_makeflags_isolation_marker.sh"
a24_regression_script_fail_summary_marker="${tmp_dir}/run_a24_regression.fail_summary_marker.sh"
a24_regression_script_fail_summary_out_marker="${tmp_dir}/run_a24_regression.fail_summary_out_marker.sh"
a24_regression_full_script_pass="${tmp_dir}/run_a24_regression_full.pass.sh"
a24_regression_full_script_fail_command="${tmp_dir}/run_a24_regression_full.fail_command.sh"
a24_regression_full_script_fail_retry_knob_marker="${tmp_dir}/run_a24_regression_full.fail_retry_knob_marker.sh"
a24_regression_full_script_fail_retry_validation_marker="${tmp_dir}/run_a24_regression_full.fail_retry_validation_marker.sh"
a24_regression_full_script_fail_retry_used_marker="${tmp_dir}/run_a24_regression_full.fail_retry_used_marker.sh"
a24_batch_script_pass="${tmp_dir}/run_a24_batch.pass.sh"
a24_batch_script_fail_command="${tmp_dir}/run_a24_batch.fail_command.sh"
a24_batch_script_fail_lock_pid_marker="${tmp_dir}/run_a24_batch.fail_lock_pid_marker.sh"
a24_batch_script_fail_lock_marker="${tmp_dir}/run_a24_batch.fail_lock_marker.sh"
a24_batch_script_fail_stale_recovery_marker="${tmp_dir}/run_a24_batch.fail_stale_recovery_marker.sh"
a24_batch_script_fail_summary_out_marker="${tmp_dir}/run_a24_batch.fail_summary_out_marker.sh"
a24_batch_script_fail_makeflags_marker="${tmp_dir}/run_a24_batch.fail_makeflags_marker.sh"
a24_batch_script_fail_failed_cmd_marker="${tmp_dir}/run_a24_batch.fail_failed_cmd_marker.sh"
a24_batch_script_fail_retry_knob_marker="${tmp_dir}/run_a24_batch.fail_retry_knob_marker.sh"
a24_batch_script_fail_retry_validation_marker="${tmp_dir}/run_a24_batch.fail_retry_validation_marker.sh"
a24_batch_script_fail_retry_used_marker="${tmp_dir}/run_a24_batch.fail_retry_used_marker.sh"
a24_regression_test_script_pass="${tmp_dir}/test_run_a24_regression.pass.sh"
a24_regression_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_regression.fail_build_preflight_marker.sh"
a24_batch_test_script_pass="${tmp_dir}/test_run_a24_batch.pass.sh"
a24_batch_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_batch.fail_build_preflight_marker.sh"
a24_batch_test_script_fail_full_chain_marker="${tmp_dir}/test_run_a24_batch.fail_full_chain_marker.sh"
a24_regression_full_test_script_pass="${tmp_dir}/test_run_a24_regression_full.pass.sh"
a24_regression_full_test_script_fail_build_preflight_marker="${tmp_dir}/test_run_a24_regression_full.fail_build_preflight_marker.sh"
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
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_pass}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_command}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_knob_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_knob_validation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_contract_skip_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_makeflags_isolation_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_marker}"
cp "FEM4C/scripts/run_a24_regression.sh" "${a24_regression_script_fail_summary_out_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_pass}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_command}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_knob_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_validation_marker}"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${a24_regression_full_script_fail_retry_used_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_pass}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_command}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_lock_pid_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_lock_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_stale_recovery_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_summary_out_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_makeflags_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_failed_cmd_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_knob_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_validation_marker}"
cp "FEM4C/scripts/run_a24_batch.sh" "${a24_batch_script_fail_retry_used_marker}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_regression.sh" "${a24_regression_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_build_preflight_marker}"
cp "FEM4C/scripts/test_run_a24_batch.sh" "${a24_batch_test_script_fail_full_chain_marker}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_pass}"
cp "FEM4C/scripts/test_run_a24_regression_full.sh" "${a24_regression_full_test_script_fail_build_preflight_marker}"
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
sed -i 's#tmp_run_b8_regression_fail\.[^"]*\.sh#tmp_run_b8_regression_fail.REMOVED.sh#' "${b8_regression_test_script_fail_temp_copy_marker}"

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

sed -i 's/if make -C FEM4C mbd_a24_regression; then/if make -C FEM4C mbd_a24_regression_missing; then/' "${a24_regression_full_script_fail_command}"

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

sed -i 's/failed_cmd=/failed_cmd_removed=/' "${a24_batch_script_fail_failed_cmd_marker}"

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
