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
ci_contract_test_script_path="${21:-FEM4C/scripts/test_check_ci_contract.sh}"

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

check_order_in_file() {
  local label="$1"
  local first_pattern="$2"
  local second_pattern="$3"
  local file_path="$4"
  local first_line=""
  local second_line=""
  checks=$((checks + 1))
  first_line="$(rg -n --fixed-strings -- "$first_pattern" "$file_path" | head -n1 | cut -d: -f1 || true)"
  second_line="$(rg -n --fixed-strings -- "$second_pattern" "$file_path" | head -n1 | cut -d: -f1 || true)"
  if [[ -n "$first_line" && -n "$second_line" && "$first_line" =~ ^[0-9]+$ && "$second_line" =~ ^[0-9]+$ ]] && (( first_line < second_line )); then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (order mismatch: '$first_pattern' before '$second_pattern' in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_min_count_in_file() {
  local label="$1"
  local pattern="$2"
  local min_count="$3"
  local file_path="$4"
  local actual_count=0
  checks=$((checks + 1))
  actual_count="$( (rg -n --fixed-strings -- "$pattern" "$file_path" || true) | wc -l | tr -d '[:space:]')"
  if [[ "$actual_count" =~ ^[0-9]+$ ]] && (( actual_count >= min_count )); then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (count mismatch: pattern='$pattern' actual=${actual_count} min=${min_count} in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_exact_count_in_file() {
  local label="$1"
  local pattern="$2"
  local expected_count="$3"
  local file_path="$4"
  local actual_count=0
  checks=$((checks + 1))
  actual_count="$( (rg -n --fixed-strings -- "$pattern" "$file_path" || true) | wc -l | tr -d '[:space:]')"
  if [[ "$actual_count" =~ ^[0-9]+$ ]] && (( actual_count == expected_count )); then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (count mismatch: pattern='$pattern' actual=${actual_count} expected=${expected_count} in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_absence_in_file() {
  local label="$1"
  local pattern="$2"
  local file_path="$3"
  checks=$((checks + 1))
  if rg -n --fixed-strings -- "$pattern" "$file_path" > /dev/null; then
    echo "CI_CONTRACT_CHECK[$label]=FAIL (unexpected pattern: $pattern in $file_path)" >&2
    fails=$((fails + 1))
  else
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  fi
}

check_shell_syntax_in_file() {
  local label="$1"
  local file_path="$2"
  checks=$((checks + 1))
  if bash -n "$file_path" > /dev/null 2>&1; then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (shell syntax error in $file_path)" >&2
    fails=$((fails + 1))
  fi
}

check_lock_wait_max_contract_sync() {
  local file_path="$1"
  local mode="${FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE:-compat}"
  local require_sync="${FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC:-0}"
  local mode_source="default"
  local require_sync_source="default"
  local has_knob=0
  local has_validation=0
  local has_guard=0
  local state="mixed"
  local drift_reason="partial"
  local pending_sync=0
  if [[ -n "${FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE+x}" ]]; then
    mode_source="env"
  fi
  if [[ -n "${FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC+x}" ]]; then
    require_sync_source="env"
  fi
  if rg -n --fixed-strings -- 'lock_wait_sec_max="${FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC_MAX:-30}"' "$file_path" > /dev/null; then
    has_knob=1
  fi
  if rg -n --fixed-strings -- "FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC_MAX must be a non-negative integer" "$file_path" > /dev/null; then
    has_validation=1
  fi
  if rg -n --fixed-strings -- "FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC exceeds max" "$file_path" > /dev/null; then
    has_guard=1
  fi

  if (( has_knob == 1 && has_validation == 1 && has_guard == 1 )); then
    state="all_present"
    drift_reason="none"
  elif (( has_knob == 0 && has_validation == 0 && has_guard == 0 )); then
    state="all_absent"
    drift_reason="none"
  elif (( has_knob + has_validation + has_guard == 1 )); then
    drift_reason="single_marker_only"
  elif (( has_knob + has_validation + has_guard == 2 )); then
    drift_reason="two_markers_only"
  fi
  if [[ "$mode" == "compat" && "$state" == "all_absent" ]]; then
    pending_sync=1
  fi
  echo "CI_CONTRACT_INFO[ci_contract_test_selftest_lock_wait_max_state]=${state} mode=${mode} mode_source=${mode_source} require_sync=${require_sync} require_sync_source=${require_sync_source} knob=${has_knob} validation=${has_validation} guard=${has_guard} drift_reason=${drift_reason} pending_sync=${pending_sync}"

  checks=$((checks + 7))
  case "$mode" in
    compat|strict_present|strict_absent)
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_mode_marker]=PASS"
      ;;
    *)
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_mode_marker]=FAIL (invalid FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=${mode}; expected compat|strict_present|strict_absent)" >&2
      fails=$((fails + 1))
      ;;
  esac
  case "$require_sync" in
    0|1)
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_require_sync_marker]=PASS"
      ;;
    *)
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_require_sync_marker]=FAIL (invalid FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC=${require_sync}; expected 0 or 1)" >&2
      fails=$((fails + 1))
      ;;
  esac
  if (( pending_sync == 1 && require_sync == 1 )); then
    echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_pending_sync_marker]=FAIL (pending sync detected in ${file_path}; set FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=strict_present after re-sync)" >&2
    fails=$((fails + 1))
  else
    echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_pending_sync_marker]=PASS"
  fi
  if (( require_sync == 1 )) && [[ "$mode" != "strict_present" ]]; then
    echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_require_sync_mode_marker]=FAIL (FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC=1 requires FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=strict_present)" >&2
    fails=$((fails + 1))
  else
    echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_require_sync_mode_marker]=PASS"
  fi
  case "$mode" in
    compat)
      # During cross-team edits, accept both "all-present" and "all-absent";
      # only mixed states are treated as contract drift.
      if (( has_knob == has_validation && has_validation == has_guard )); then
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=PASS"
      else
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=FAIL (compat mode mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=FAIL (compat mode mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=FAIL (compat mode mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        fails=$((fails + 3))
      fi
      ;;
    strict_present)
      if (( has_knob == 1 && has_validation == 1 && has_guard == 1 )); then
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=PASS"
      else
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=FAIL (strict_present mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=FAIL (strict_present mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=FAIL (strict_present mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        fails=$((fails + 3))
      fi
      ;;
    strict_absent)
      if (( has_knob == 0 && has_validation == 0 && has_guard == 0 )); then
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=PASS"
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=PASS"
      else
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=FAIL (strict_absent mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=FAIL (strict_absent mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=FAIL (strict_absent mismatch in $file_path: knob=${has_knob} validation=${has_validation} guard=${has_guard})" >&2
        fails=$((fails + 3))
      fi
      ;;
    *)
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_knob_marker]=FAIL (invalid mode state for lock_wait_max contract: ${mode})" >&2
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_validation_marker]=FAIL (invalid mode state for lock_wait_max contract: ${mode})" >&2
      echo "CI_CONTRACT_CHECK[ci_contract_test_selftest_lock_wait_max_guard_marker]=FAIL (invalid mode state for lock_wait_max contract: ${mode})" >&2
      fails=$((fails + 3))
      ;;
  esac
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
check_pattern_in_file "mbd_b8_knob_matrix_smoke_skip_flag" 'B8_KNOB_MATRIX_SKIP_FULL=1 bash $(MBD_B8_KNOB_MATRIX_TEST_SCRIPT)' "$makefile_path"
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
check_pattern_in_file "mbd_integrator_bin_default_marker" 'FEM4C_BIN_DEFAULT="${FEM4C_DIR}/bin/fem4c"' "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_bin_env_override_marker" 'FEM4C_BIN="${FEM4C_MBD_BIN:-${FEM4C_BIN_DEFAULT}}"' "$mbd_integrator_script_path"
check_pattern_in_file "mbd_integrator_bin_preflight_marker" "mbd integrator checker requires executable fem4c binary" "$mbd_integrator_script_path"
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
check_pattern_in_file "a24_regression_summary_out_dir_validation_marker" "A24 regression summary output directory does not exist" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_summary_out_type_validation_marker" "A24 regression summary output path must be a file" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_summary_out_write_validation_marker" "cannot write A24 regression summary output" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_skip_lock_knob_marker" 'skip_lock="${A24_REGRESSION_SKIP_LOCK:-0}"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_skip_lock_validation_marker" "A24_REGRESSION_SKIP_LOCK must be 0 or 1" "$a24_regression_script_path"
check_pattern_in_file "a24_regression_lock_dir_default_marker" 'lock_dir="${A24_REGRESSION_LOCK_DIR:-/tmp/fem4c_a24_regression.lock}"' "$a24_regression_script_path"
check_pattern_in_file "a24_regression_lock_fail_marker" "FAIL: a24 regression lock is already held" "$a24_regression_script_path"
check_regex_in_file "a24_full_regression_cmd_clean" "^(if )?make -C FEM4C clean(; then)?$" "$a24_regression_full_script_path"
check_regex_in_file "a24_full_regression_cmd_build" "^(if )?make -C FEM4C(; then)?$" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_regression_cmd_a24" 'if make -C FEM4C mbd_a24_regression >"${nested_regression_log}" 2>&1; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_dir_marker" 'lock_dir="${A24_FULL_LOCK_DIR:-${A24_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_batch.lock}}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_pid_marker" 'lock_pid_file="${lock_dir}/pid"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_lock_fail_marker" "FAIL: a24 full lock is already held" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_stale_recovery_marker" "INFO: recovered stale a24 full lock" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_marker" "A24_FULL_SUMMARY lock=" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_out_marker" 'summary_out="${A24_FULL_SUMMARY_OUT:-}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_out_dir_validation_marker" "A24 full summary output directory does not exist" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_out_type_validation_marker" "A24 full summary output path must be a file" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_summary_out_write_validation_marker" "cannot write A24 full summary output" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_makeflags_marker" 'export MAKEFLAGS="-j1"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_knob_marker" 'retry_on_137="${A24_FULL_RETRY_ON_137:-1}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_validation_marker" "A24_FULL_RETRY_ON_137 must be 0 or 1" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_retry_used_marker" "retry_used=" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_failed_step_marker" 'local_failed_step="regression_${nested_failed_step}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_failed_cmd_marker" 'local_failed_cmd="${nested_failed_cmd}"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_integrator_hint_marker" "nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_fallback_function_marker" "extract_nested_regression_failure_from_log() {" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_parser_marker" "parse_nested_regression_summary_line() {" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_crlf_trim_marker" "summary_line=\"\${summary_line//\$'\\r'/}\"" "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_line_marker" 'summary_line="$(grep -E '\''^A24_REGRESSION_SUMMARY[[:space:]]'\'' "${summary_log}" | tail -n 1 || true)"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_casefold_line_marker" 'summary_line="$(grep -Ei '\''^A24_REGRESSION_SUMMARY[[:space:]]'\'' "${summary_log}" | tail -n 1 || true)"' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_casefold_token_marker" 'case "${key,,}" in' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_token_equals_guard_marker" 'if [[ "${token}" != *=* ]]; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_key_guard_marker" 'if [[ ! "${key}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_value_equals_guard_marker" 'if [[ "${value}" == *"="* ]]; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_quote_guard_marker" '|| "${value}" == *\\* ]]; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_value_charset_guard_marker" 'if [[ ! "${value}" =~ ^[a-z0-9_]+$ ]]; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_partial_guard_marker" "Treat partial nested summary as invalid and defer to generic log fallback." "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_precedence_marker" "Prefer explicit nested summary fields over generic preflight log fallback." "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_summary_parse_call_marker" 'if parse_nested_regression_summary_line "${summary_line}"; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_fallback_pattern_marker" 'if grep -qi "requires executable fem4c binary" "${summary_log}"; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_full_nested_log_fallback_call_marker" 'elif extract_nested_regression_failure_from_log "${nested_regression_log}"; then' "$a24_regression_full_script_path"
check_pattern_in_file "a24_batch_cmd_a24" 'if make -C FEM4C mbd_a24_regression >"${nested_regression_log}" 2>&1; then' "$a24_batch_script_path"
check_regex_in_file "a24_batch_cmd_a24_test" "^(if )?make -C FEM4C mbd_a24_regression_test(; then)?$" "$a24_batch_script_path"
check_regex_in_file "a24_batch_cmd_a24_full_test" "^(if )?make -C FEM4C mbd_a24_regression_full_test(; then)?$" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_dir_marker" 'lock_dir="${A24_BATCH_LOCK_DIR:-${A24_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_batch.lock}}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_pid_marker" 'lock_pid_file="${lock_dir}/pid"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_lock_fail_marker" "FAIL: a24 batch lock is already held" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_stale_recovery_marker" "INFO: recovered stale a24 batch lock" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_marker" "A24_BATCH_SUMMARY lock=" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_out_marker" 'summary_out="${A24_BATCH_SUMMARY_OUT:-}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_out_dir_validation_marker" "A24 batch summary output directory does not exist" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_out_type_validation_marker" "A24 batch summary output path must be a file" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_summary_out_write_validation_marker" "cannot write A24 batch summary output" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_makeflags_marker" 'export MAKEFLAGS="-j1"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_failed_cmd_marker" 'overall=${overall} failed_step=${failed_step} failed_cmd=${failed_cmd}' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_knob_marker" 'retry_on_137="${A24_BATCH_RETRY_ON_137:-1}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_validation_marker" "A24_BATCH_RETRY_ON_137 must be 0 or 1" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_retry_used_marker" "retry_used=" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_failed_step_marker" 'local_failed_step="regression_${nested_failed_step}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_failed_cmd_marker" 'local_failed_cmd="${nested_failed_cmd}"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_integrator_hint_marker" "nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_fallback_function_marker" "extract_nested_regression_failure_from_log() {" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_parser_marker" "parse_nested_regression_summary_line() {" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_crlf_trim_marker" "summary_line=\"\${summary_line//\$'\\r'/}\"" "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_line_marker" 'summary_line="$(grep -E '\''^A24_REGRESSION_SUMMARY[[:space:]]'\'' "${summary_log}" | tail -n 1 || true)"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_casefold_line_marker" 'summary_line="$(grep -Ei '\''^A24_REGRESSION_SUMMARY[[:space:]]'\'' "${summary_log}" | tail -n 1 || true)"' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_casefold_token_marker" 'case "${key,,}" in' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_token_equals_guard_marker" 'if [[ "${token}" != *=* ]]; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_key_guard_marker" 'if [[ ! "${key}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_value_equals_guard_marker" 'if [[ "${value}" == *"="* ]]; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_quote_guard_marker" '|| "${value}" == *\\* ]]; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_value_charset_guard_marker" 'if [[ ! "${value}" =~ ^[a-z0-9_]+$ ]]; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_partial_guard_marker" "Treat partial nested summary as invalid and defer to generic log fallback." "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_precedence_marker" "Prefer explicit nested summary fields over generic preflight log fallback." "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_summary_parse_call_marker" 'if parse_nested_regression_summary_line "${summary_line}"; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_fallback_pattern_marker" 'if grep -qi "requires executable fem4c binary" "${summary_log}"; then' "$a24_batch_script_path"
check_pattern_in_file "a24_batch_nested_log_fallback_call_marker" 'elif extract_nested_regression_failure_from_log "${nested_regression_log}"; then' "$a24_batch_script_path"
check_pattern_in_file "b8_guard_makeflags_isolation" "env -u MAKEFLAGS -u MFLAGS" "$b8_guard_script_path"
check_pattern_in_file "b8_guard_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_guard_script_path"
check_pattern_in_file "b8_guard_contract_test_b14_target_override" "B8_B14_TARGET=mbd_ci_contract" "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_guard_contract_test_repo_root_passthrough_marker" 'FEM4C_REPO_ROOT="${root_dir}"' "$b8_guard_contract_test_script_path"
check_pattern_in_file "b8_guard_test_makeflags_case_marker" "makeflags_isolation" "$b8_guard_test_script_path"
check_pattern_in_file "b8_regression_knob_validation" "B8_RUN_B14_REGRESSION must be 0 or 1" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_skip_lock_knob_marker" 'b8_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_skip_lock_validation_marker" "B8_REGRESSION_SKIP_LOCK must be 0 or 1" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_scope_knob_marker" 'b8_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_scope_validation_marker" "B8_REGRESSION_LOCK_SCOPE must be repo or global" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_default_global_marker" 'b8_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_source_default_env_marker" 'b8_lock_dir_source="env"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_repo_hash_marker" 'b8_lock_repo_hash="$(printf '\''%s\n'\'' "$root_dir" | cksum | awk '\''{print $1}'\'')"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_default_repo_marker" 'b8_lock_dir="/tmp/fem4c_b8_regression.${b8_lock_repo_hash}.lock"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_source_repo_marker" 'b8_lock_dir_source="scope_repo_default"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_source_global_marker" 'b8_lock_dir_source="scope_global_default"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_pid_marker" 'b8_lock_pid_file="${b8_lock_dir}/pid"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_fail_marker" "FAIL: b8 regression lock is already held" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_stale_recovery_marker" "INFO: recovered stale b8 regression lock" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_trace_marker" 'lock_dir=$b8_lock_dir' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_lock_dir_source_trace_marker" 'lock_dir_source=$b8_lock_dir_source' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_make_cmd_validation" "B8_MAKE_CMD is not executable" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_repo_root_override_marker" 'root_dir="${FEM4C_REPO_ROOT:-}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_repo_root_validate_marker" "FEM4C repo root is invalid" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_target_default" 'b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_target_pass_through" 'B8_B14_TARGET="$b8_b14_target"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_makeflags_isolation" "env -u MAKEFLAGS -u MFLAGS" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_regression_script_path"
check_absence_in_file "b8_regression_no_direct_contract_test_call" "run_make_target mbd_ci_contract_test" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_local_target_isolation" "-u B8_LOCAL_TARGET" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_target_isolation" "-u B8_B14_TARGET" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_b14_knob_isolation" "-u B8_RUN_B14_REGRESSION" "$b8_regression_script_path"
check_pattern_in_file "b8_regression_local_target_pass_through" 'B8_LOCAL_TARGET="$b8_local_target"' "$b8_regression_script_path"
check_pattern_in_file "b8_regression_local_target_summary_trace_marker" 'local_target=$b8_local_target' "$b8_regression_script_path"
check_pattern_in_file "b8_full_regression_knob_validation" "B8_RUN_B14_REGRESSION must be 0 or 1" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_skip_lock_knob_marker" 'b8_regression_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_skip_lock_validation_marker" "B8_REGRESSION_SKIP_LOCK must be 0 or 1" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_scope_knob_marker" 'b8_regression_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_scope_validation_marker" "B8_REGRESSION_LOCK_SCOPE must be repo or global" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_default_global_marker" 'b8_regression_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_source_default_env_marker" 'b8_regression_lock_dir_source="env"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_used_default_marker" "b8_test_retry_used=0" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_reason_default_marker" 'b8_test_retry_reason="none"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_repo_hash_marker" 'b8_lock_repo_hash="$(printf '\''%s\n'\'' "$root_dir" | cksum | awk '\''{print $1}'\'')"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_default_repo_marker" 'b8_regression_lock_dir="/tmp/fem4c_b8_regression.${b8_lock_repo_hash}.lock"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_source_repo_marker" 'b8_regression_lock_dir_source="scope_repo_default"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_source_global_marker" 'b8_regression_lock_dir_source="scope_global_default"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_make_cmd_validation" "B8_MAKE_CMD is not executable" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_repo_root_override_marker" 'root_dir="${FEM4C_REPO_ROOT:-}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_repo_root_validate_marker" "FEM4C repo root is invalid" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_b14_target_default" 'b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_b14_target_pass_through" 'B8_B14_TARGET="$b8_b14_target"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_tmp_copy_dir_isolation" "-u B8_TEST_TMP_COPY_DIR" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_local_target_isolation" "-u B8_LOCAL_TARGET" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_skip_lock_isolation" "-u B8_REGRESSION_SKIP_LOCK" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_scope_isolation" "-u B8_REGRESSION_LOCK_SCOPE" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_isolation" "-u B8_REGRESSION_LOCK_DIR" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_local_target_pass_through" 'B8_LOCAL_TARGET="$b8_local_target"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_local_target_summary_trace_marker" 'local_target=$b8_local_target' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_skip_lock_pass_through" 'B8_REGRESSION_SKIP_LOCK="$b8_regression_skip_lock"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_scope_pass_through" 'B8_REGRESSION_LOCK_SCOPE="$b8_regression_lock_scope"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_pass_through" 'B8_REGRESSION_LOCK_DIR="$b8_regression_lock_dir"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_lock_dir_source_trace_marker" 'b8_lock_dir_source=$b8_regression_lock_dir_source' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_fn_marker" "run_test_with_parser_retry() {" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_used_set_marker" "b8_test_retry_used=1" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_reason_set_marker" 'b8_test_retry_reason="parser_missing"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_parser_missing_marker" "INFO: parser executable missing after test failure; rebuilding via make all and retrying test once" "$b8_regression_full_script_path"
check_regex_in_file "b8_full_regression_test_retry_call_marker" '^run_test_with_parser_retry$' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_reason_consistency_zero_marker" 'if [[ "$b8_test_retry_used" == "0" && "$b8_test_retry_reason" != "none" ]]; then' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_reason_consistency_one_marker" 'if [[ "$b8_test_retry_used" == "1" && "$b8_test_retry_reason" != "parser_missing" ]]; then' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_used_summary_marker" "test_retry_used=\$b8_test_retry_used" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_test_retry_reason_summary_marker" "test_retry_reason=\$b8_test_retry_reason" "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_make_serial_target_marker" '"$b8_make_cmd" -j1 -C FEM4C "$target"' "$b8_regression_full_script_path"
check_pattern_in_file "b8_full_regression_make_serial_b8_marker" '"$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression' "$b8_regression_full_script_path"
check_exact_count_in_file "b8_full_regression_make_serial_target_count_marker" '"$b8_make_cmd" -j1 -C FEM4C "$target"' 2 "$b8_regression_full_script_path"
check_exact_count_in_file "b8_full_regression_make_serial_b8_count_marker" '"$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression' 2 "$b8_regression_full_script_path"
check_pattern_in_file "b8_knob_matrix_skip_full_validation" "B8_KNOB_MATRIX_SKIP_FULL must be 0 or 1" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_skip_full_info_marker" "INFO: skip full regression matrix cases (B8_KNOB_MATRIX_SKIP_FULL=1)" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_local_target_env_marker" 'export B8_LOCAL_TARGET="${B8_LOCAL_TARGET:-mbd_b8_syntax}"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_zero_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_one_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_local_target_summary_trace_case" 'grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}/regression_1.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_repo_default_lock_source_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_repo_default_lock_source_trace_case" 'grep -q "lock_dir_source=scope_repo_default" "${tmp_dir}/regression_repo_default_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_global_default_lock_source_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_global_default_lock_source_trace_case" 'grep -q "lock_dir_source=scope_global_default" "${tmp_dir}/regression_global_default_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_env_lock_source_case" 'mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${regression_env_lock_dir}" B8_MAKE_CMD=make' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_env_lock_source_trace_case" 'grep -q "lock_dir_source=env" "${tmp_dir}/regression_env_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_invalid_knob_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_regression_invalid_make_case" "mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_zero_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_one_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_zero_retry_reason_trace_case" 'grep -Eq "test_retry_reason=(none|parser_missing)" "${tmp_dir}/full_0.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_one_retry_reason_trace_case" 'grep -Eq "test_retry_reason=(none|parser_missing)" "${tmp_dir}/full_1.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_local_target_summary_trace_case" 'grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}/full_1.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_repo_default_lock_source_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_repo_default_lock_source_trace_case" 'grep -q "b8_lock_dir_source=scope_repo_default" "${tmp_dir}/full_repo_default_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_global_default_lock_source_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_global_default_lock_source_trace_case" 'grep -q "b8_lock_dir_source=scope_global_default" "${tmp_dir}/full_global_default_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_env_lock_source_case" 'mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${full_env_lock_dir}" B8_MAKE_CMD=make' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_env_lock_source_trace_case" 'grep -q "b8_lock_dir_source=env" "${tmp_dir}/full_env_lock_scope.log"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_repo_lock_hash_marker" 'repo_lock_hash="$(printf '\''%s\n'\'' "${root_dir}" | cksum | awk '\''{print $1}'\'')"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_repo_default_lock_dir_marker" 'repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_global_default_lock_dir_marker" 'global_default_lock_dir="/tmp/fem4c_b8_regression.lock"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_expected_calls_marker" "full_cleanup_expected_calls=7" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_expected_order_trace_marker" "full_cleanup_expected_order_trace=\"\"" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_expected_order_build_marker" "for ((cleanup_idx=0; cleanup_idx<full_cleanup_expected_calls; cleanup_idx++)); do" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_cleanup_counter_marker" "parser_cleanup_call_count=\$((parser_cleanup_call_count + 1))" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_cleanup_counter_marker" "b8_cleanup_call_count=\$((b8_cleanup_call_count + 1))" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_order_trace_init_marker" "cleanup_call_order_trace=\"\"" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_order_trace_append_fn_marker" "append_cleanup_call_order_trace() {" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_cleanup_order_trace_append_marker" 'append_cleanup_call_order_trace "parser"' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_cleanup_order_trace_append_marker" 'append_cleanup_call_order_trace "b8"' "$b8_knob_matrix_script_path"
check_order_in_file "b8_knob_matrix_full_parser_cleanup_order_trace_before_rm_marker" 'append_cleanup_call_order_trace "parser"' 'rm -rf /tmp/fem4c_parser_compat.lock 2>/dev/null || true' "$b8_knob_matrix_script_path"
check_order_in_file "b8_knob_matrix_full_b8_cleanup_order_trace_before_rm_marker" 'append_cleanup_call_order_trace "b8"' 'rm -rf "${repo_default_lock_dir}" "${global_default_lock_dir}" 2>/dev/null || true' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_lock_cleanup_fn_marker" "cleanup_parser_compat_lock() {" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_lock_cleanup_marker" 'rm -rf /tmp/fem4c_parser_compat.lock 2>/dev/null || true' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_lock_cleanup_call_marker" "  cleanup_parser_compat_lock" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_lock_cleanup_fn_marker" "cleanup_b8_regression_locks() {" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_lock_cleanup_marker" 'rm -rf "${repo_default_lock_dir}" "${global_default_lock_dir}" 2>/dev/null || true' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_lock_cleanup_call_marker" "  cleanup_b8_regression_locks" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_parser_cleanup_count_assert_marker" 'if [[ "${parser_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" ]]; then' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_b8_cleanup_count_assert_marker" 'if [[ "${b8_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" ]]; then' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_order_trace_assert_marker" 'if [[ "${cleanup_call_order_trace}" != "${full_cleanup_expected_order_trace}" ]]; then' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_count_summary_marker" 'INFO: full cleanup call count parser=${parser_cleanup_call_count} b8=${b8_cleanup_call_count} expected=${full_cleanup_expected_calls}' "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_cleanup_order_summary_marker" 'INFO: full cleanup call order trace=${cleanup_call_order_trace} expected=${full_cleanup_expected_order_trace}' "$b8_knob_matrix_script_path"
check_order_in_file "b8_knob_matrix_full_cleanup_call_order_marker" "  cleanup_parser_compat_lock" "  cleanup_b8_regression_locks" "$b8_knob_matrix_script_path"
check_min_count_in_file "b8_knob_matrix_full_parser_lock_cleanup_call_count_marker" "  cleanup_parser_compat_lock" 7 "$b8_knob_matrix_script_path"
check_min_count_in_file "b8_knob_matrix_full_b8_lock_cleanup_call_count_marker" "  cleanup_b8_regression_locks" 7 "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_invalid_knob_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_knob_matrix_full_invalid_make_case" "mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__" "$b8_knob_matrix_script_path"
check_pattern_in_file "b8_guard_local_target_default" 'local_target="${B8_LOCAL_TARGET:-mbd_checks}"' "$b8_guard_script_path"
check_pattern_in_file "a24_regression_test_build_preflight_marker" "run_a24_regression self-test requires successful FEM4C build preflight" "$a24_regression_test_script_path"
check_pattern_in_file "a24_regression_test_summary_out_missing_dir_case_marker" 'A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/missing_summary_dir/summary.log"' "$a24_regression_test_script_path"
check_pattern_in_file "a24_regression_test_summary_out_dir_path_case_marker" 'A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/summary_dir_path"' "$a24_regression_test_script_path"
check_pattern_in_file "a24_regression_test_summary_out_readonly_parent_case_marker" 'A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/summary_parent_readonly/summary.log"' "$a24_regression_test_script_path"
check_pattern_in_file "a24_regression_test_summary_out_write_case_marker" 'summary_out_write_probe="/proc/1/cmdline"' "$a24_regression_test_script_path"
check_pattern_in_file "a24_batch_test_build_preflight_marker" "run_a24_batch self-test requires successful FEM4C build preflight" "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_selftest_lock_dir_marker" 'selftest_lock_dir="/tmp/fem4c_test_run_a24_batch.lock"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_selftest_lock_function_marker" "acquire_selftest_lock()" "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_selftest_lock_busy_marker" "test_run_a24_batch already running" "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_selftest_lock_stale_recovery_marker" 'if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then' "$a24_batch_test_script_path"
check_pattern_in_file "a24_full_test_build_preflight_marker" "run_a24_regression_full self-test requires successful FEM4C build preflight" "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_selftest_lock_dir_marker" 'selftest_lock_dir="/tmp/fem4c_test_run_a24_regression_full.lock"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_selftest_lock_function_marker" "acquire_selftest_lock()" "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_selftest_lock_busy_marker" "test_run_a24_regression_full already running" "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_selftest_lock_stale_recovery_marker" 'if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_regression_lock_dir_isolation_marker" 'export A24_REGRESSION_LOCK_DIR="${tmp_dir}/a24_regression.lock"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_summary_out_missing_dir_case_marker" 'A24_FULL_SUMMARY_OUT="${tmp_dir}/missing_full_summary_dir/summary.log"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_summary_out_dir_path_case_marker" 'A24_FULL_SUMMARY_OUT="${tmp_dir}/summary_dir_path"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_summary_out_readonly_parent_case_marker" 'A24_FULL_SUMMARY_OUT="${tmp_dir}/summary_parent_readonly/summary.log"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_summary_out_write_case_marker" 'full_summary_write_probe="/proc/1/cmdline"' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_nonexec_bin_case_marker" 'summary_out_nonexec_bin=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_fallback_case_marker" 'summary_out_log_summary_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_precedence_case_marker" 'summary_out_log_summary_precedence=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_malformed_key_precedence_case_marker" 'summary_out_log_summary_malformed_key_precedence=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_duplicate_key_precedence_case_marker" 'summary_out_log_summary_duplicate_key_precedence=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_malformed_unknown_fallback_case_marker" 'summary_out_log_summary_malformed_unknown_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_uppercase_case_marker" 'summary_out_log_summary_uppercase=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_uppercase_precedence_case_marker" 'summary_out_log_summary_uppercase_precedence=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_tab_whitespace_case_marker" 'summary_out_log_summary_tab_whitespace=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_equals_whitespace_fallback_case_marker" 'summary_out_log_summary_equals_whitespace_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_quoted_value_fallback_case_marker" 'summary_out_log_summary_quoted_value_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_single_quote_fallback_case_marker" 'summary_out_log_summary_single_quote_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_quote_mixed_fallback_case_marker" 'summary_out_log_summary_quote_mixed_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_backslash_value_fallback_case_marker" 'summary_out_log_summary_backslash_value_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_quoted_key_fallback_case_marker" 'summary_out_log_summary_quoted_key_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_single_quoted_key_fallback_case_marker" 'summary_out_log_summary_single_quoted_key_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_quoted_cmd_key_fallback_case_marker" 'summary_out_log_summary_quoted_cmd_key_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_partial_fallback_case_marker" 'summary_out_log_summary_partial_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_missing_equals_fallback_case_marker" 'summary_out_log_summary_missing_equals_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_empty_value_fallback_case_marker" 'summary_out_log_summary_empty_value_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_empty_key_fallback_case_marker" 'summary_out_log_summary_empty_key_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_extra_equals_fallback_case_marker" 'summary_out_log_summary_extra_equals_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_leading_punct_fallback_case_marker" 'summary_out_log_summary_leading_punct_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_internal_symbol_fallback_case_marker" 'summary_out_log_summary_internal_symbol_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_full_test_log_summary_crlf_fallback_case_marker" 'summary_out_log_summary_crlf_fallback=' "$a24_regression_full_test_script_path"
check_pattern_in_file "a24_batch_test_full_chain_marker" "full->batch chain case" "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_summary_out_missing_dir_case_marker" 'A24_BATCH_SUMMARY_OUT="${tmp_dir}/missing_batch_summary_dir/summary.log"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_summary_out_dir_path_case_marker" 'A24_BATCH_SUMMARY_OUT="${tmp_dir}/summary_dir_path"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_summary_out_readonly_parent_case_marker" 'A24_BATCH_SUMMARY_OUT="${tmp_dir}/summary_parent_readonly/summary.log"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_summary_out_write_case_marker" 'batch_summary_write_probe="/proc/1/cmdline"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_nonexec_bin_case_marker" 'summary_out_nonexec_bin=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_regression_lock_dir_isolation_marker" 'export A24_REGRESSION_LOCK_DIR="${tmp_dir}/a24_regression.lock"' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_fallback_case_marker" 'summary_out_log_summary_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_precedence_case_marker" 'summary_out_log_summary_precedence=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_malformed_key_precedence_case_marker" 'summary_out_log_summary_malformed_key_precedence=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_duplicate_key_precedence_case_marker" 'summary_out_log_summary_duplicate_key_precedence=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_malformed_unknown_fallback_case_marker" 'summary_out_log_summary_malformed_unknown_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_uppercase_case_marker" 'summary_out_log_summary_uppercase=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_uppercase_precedence_case_marker" 'summary_out_log_summary_uppercase_precedence=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_tab_whitespace_case_marker" 'summary_out_log_summary_tab_whitespace=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_equals_whitespace_fallback_case_marker" 'summary_out_log_summary_equals_whitespace_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_quoted_value_fallback_case_marker" 'summary_out_log_summary_quoted_value_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_single_quote_fallback_case_marker" 'summary_out_log_summary_single_quote_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_quote_mixed_fallback_case_marker" 'summary_out_log_summary_quote_mixed_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_backslash_value_fallback_case_marker" 'summary_out_log_summary_backslash_value_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_quoted_key_fallback_case_marker" 'summary_out_log_summary_quoted_key_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_single_quoted_key_fallback_case_marker" 'summary_out_log_summary_single_quoted_key_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_quoted_cmd_key_fallback_case_marker" 'summary_out_log_summary_quoted_cmd_key_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_partial_fallback_case_marker" 'summary_out_log_summary_partial_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_missing_equals_fallback_case_marker" 'summary_out_log_summary_missing_equals_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_empty_value_fallback_case_marker" 'summary_out_log_summary_empty_value_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_empty_key_fallback_case_marker" 'summary_out_log_summary_empty_key_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_extra_equals_fallback_case_marker" 'summary_out_log_summary_extra_equals_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_leading_punct_fallback_case_marker" 'summary_out_log_summary_leading_punct_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_internal_symbol_fallback_case_marker" 'summary_out_log_summary_internal_symbol_fallback=' "$a24_batch_test_script_path"
check_pattern_in_file "a24_batch_test_log_summary_crlf_fallback_case_marker" 'summary_out_log_summary_crlf_fallback=' "$a24_batch_test_script_path"
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
check_pattern_in_file "a24_acceptance_serial_summary_out_dir_validation_marker" "A24 acceptance serial summary output directory does not exist" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_summary_out_type_validation_marker" "A24 acceptance serial summary output path must be a file" "$a24_acceptance_serial_script_path"
check_pattern_in_file "a24_acceptance_serial_summary_out_write_validation_marker" "cannot write A24 acceptance serial summary output" "$a24_acceptance_serial_script_path"
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
check_pattern_in_file "a24_acceptance_serial_test_summary_out_missing_dir_case_marker" 'A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/missing_summary_dir/summary.log"' "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_summary_out_dir_path_case_marker" 'A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_dir_path"' "$a24_acceptance_serial_test_script_path"
check_pattern_in_file "a24_acceptance_serial_test_summary_out_readonly_case_marker" 'A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_readonly.log"' "$a24_acceptance_serial_test_script_path"
check_shell_syntax_in_file "ci_contract_test_selftest_script_syntax_marker" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_scope_marker" 'lock_scope_id="${FEM4C_CI_CONTRACT_TEST_LOCK_SCOPE_ID:-${PPID:-$$}}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_dir_marker" 'selftest_lock_dir="${FEM4C_CI_CONTRACT_TEST_SELFTEST_LOCK_DIR:-/tmp/fem4c_test_check_ci_contract.${lock_scope_id}.lock}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_knob_marker" 'lock_wait_sec="${FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC:-2}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_pair_fragment_builder_marker" "build_lock_pair_fragment() {" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_pair_fragment_busy_call_marker" 'pair_fragment="$(build_lock_pair_fragment "${owner_pid_value:-unknown}" "${wait_sec_value}")"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_message_builder_marker" "build_lock_busy_message() {" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_skip_knob_marker" 'skip_lock_wait_runtime_smoke="${FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE:-0}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_function_marker" "acquire_selftest_lock()" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_function_marker" "run_lock_wait_runtime_smoke() {" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_child_call_marker" "FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE=1" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_child_wait_zero_marker" "FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC=0" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_skip_validation_marker" "FEM4C_CI_CONTRACT_TEST_SKIP_LOCK_WAIT_RUNTIME_SMOKE must be 0 or 1" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_invocation_marker" 'if [[ "${skip_lock_wait_runtime_smoke}" == "0" ]]; then' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_fail_message_marker" "lock wait runtime smoke should fail immediately when lock is held and wait_sec=0" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_contention_message_marker" "lock wait runtime smoke did not emit lock contention message" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_owner_trace_message_marker" "lock wait runtime smoke did not emit owner_pid trace" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_wait_trace_message_marker" "lock wait runtime smoke did not emit lock_wait_sec trace" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_pair_trace_message_marker" "lock wait runtime smoke did not emit owner/wait pair trace" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_pair_expected_marker" 'expected_runtime_pair="$(build_lock_pair_fragment "$$" "0")"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_pair_call_order_marker" 'expected_runtime_pair="$(build_lock_pair_fragment "$$" "0")"' "$ci_contract_test_script_path"
check_absence_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_pair_literal_fallback_absence_marker" 'expected_runtime_pair="$(printf ' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_pair_grep_marker" 'grep -Fq "${expected_runtime_pair}" "${runtime_log}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_trace_message_marker" "lock wait runtime smoke did not emit lock-dir anchored pair trace" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_expected_line_marker" 'expected_runtime_busy_line="$(build_lock_busy_message "${runtime_lock_dir}" "$$" "0")"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_grep_marker" 'grep -Fq "${expected_runtime_busy_line}" "${runtime_log}"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_runtime_smoke_lock_dir_pair_order_grep_marker" 'expected_runtime_busy_line="$(build_lock_busy_message "${runtime_lock_dir}" "$$" "0")"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_marker" "test_check_ci_contract already running" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_template_marker" "FAIL: test_check_ci_contract already running (%s, %s)" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_pair_fragment_arg_marker" '"${lock_dir}" "${pair_fragment}"' "$ci_contract_test_script_path"
check_order_in_file "ci_contract_test_selftest_lock_busy_pair_fragment_call_order_marker" 'pair_fragment="$(build_lock_pair_fragment "${owner_pid_value:-unknown}" "${wait_sec_value}")"' "printf 'FAIL: test_check_ci_contract already running (%s, %s)' \\" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_builder_call_marker" 'echo "$(build_lock_busy_message "${selftest_lock_dir}" "${owner_pid:-unknown}" "${lock_wait_sec}")" >&2' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_owner_marker" "owner_pid=%s" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_wait_marker" "lock_wait_sec=%s" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_busy_owner_wait_pair_marker" "owner_pid=%s, lock_wait_sec=%s" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_validation_marker" "FEM4C_CI_CONTRACT_TEST_LOCK_WAIT_SEC must be a non-negative integer" "$ci_contract_test_script_path"
check_lock_wait_max_contract_sync "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_deadline_marker" 'deadline_epoch=$((now_epoch + lock_wait_sec))' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_deadline_guard_marker" 'if (( now_epoch >= deadline_epoch )); then' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_loop_marker" "while true; do" "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_missing_pid_recovery_marker" 'if [[ -f "${selftest_lock_pid}" ]]' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_selftest_lock_wait_sleep_marker" "sleep 1" "$ci_contract_test_script_path"
check_min_count_in_file "ci_contract_test_selftest_lock_wait_now_epoch_refresh_count_marker" 'now_epoch="$(date +%s)"' 2 "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_cleanup_jobs_marker" 'bg_pids="$(jobs -pr 2>/dev/null || true)"' "$ci_contract_test_script_path"
check_pattern_in_file "ci_contract_test_cleanup_kill_marker" 'kill ${bg_pids} 2>/dev/null || true' "$ci_contract_test_script_path"
check_order_in_file "ci_contract_test_cleanup_call_order_marker" 'bg_pids="$(jobs -pr 2>/dev/null || true)"' 'kill ${bg_pids} 2>/dev/null || true' "$ci_contract_test_script_path"
check_absence_in_file "ci_contract_test_cleanup_no_pkill_marker" 'pkill -P $$' "$ci_contract_test_script_path"
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
check_pattern_in_file "b8_regression_test_b14_target_override_case_marker" "B8_B14_TARGET=mbd_b8_syntax" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_make_call_log_marker" 'B8_MAKE_CALL_LOG="${mock_make_call_log}"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_no_direct_contract_test_marker" "run_b8_regression should not invoke mbd_ci_contract_test directly" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_default_lock_dir_marker" 'default_lock_dir="${tmp_dir}/b8_regression_test.lock"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_export_lock_dir_marker" 'export B8_REGRESSION_LOCK_DIR="${default_lock_dir}"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_default_lock_scope_trace_marker" 'grep -q "lock_scope=repo" "${tmp_dir}/pass.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_env_lock_dir_source_trace_marker" 'grep -q "lock_dir_source=env" "${tmp_dir}/pass.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_invalid_lock_scope_case_marker" "B8_REGRESSION_LOCK_SCOPE=cluster" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_lock_scope_case_marker" "B8_REGRESSION_LOCK_SCOPE=global" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_lock_scope_trace_marker" 'grep -q "lock_scope=global" "${tmp_dir}/global_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_lock_scope_env_source_trace_marker" 'grep -q "lock_dir_source=env" "${tmp_dir}/global_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_repo_default_lock_scope_case_marker" 'repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_repo_default_lock_dir_trace_marker" 'grep -q "lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_repo_default_lock_scope_trace_marker" 'grep -q "lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_default_lock_scope_case_marker" 'global_default_lock_dir="/tmp/fem4c_b8_regression.lock"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_default_lock_dir_trace_marker" 'grep -q "lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_global_default_lock_scope_trace_marker" 'grep -q "lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_invalid_skip_lock_case_marker" "B8_REGRESSION_SKIP_LOCK=2" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_lock_held_case_marker" 'held_lock_dir="${tmp_dir}/lock_held"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_skip_lock_case_marker" "B8_REGRESSION_SKIP_LOCK=1" "$b8_regression_test_script_path"
check_pattern_in_file "b8_regression_test_stale_lock_case_marker" 'stale_lock_dir="${tmp_dir}/stale_lock"' "$b8_regression_test_script_path"
check_pattern_in_file "b8_full_test_repo_root_passthrough_marker" 'FEM4C_REPO_ROOT="${root_dir}"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_b14_case_marker" "B8_RUN_B14_REGRESSION=0" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_b14_target_override_case_marker" "B8_B14_TARGET=mbd_b8_syntax" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_default_lock_dir_marker" 'full_default_lock_dir="${tmp_dir}/b8_regression_full_test.lock"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_export_lock_dir_marker" 'export B8_REGRESSION_LOCK_DIR="${full_default_lock_dir}"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_default_lock_scope_trace_marker" 'grep -q "b8_lock_scope=repo" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_env_lock_dir_source_trace_marker" 'grep -q "b8_lock_dir_source=env" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_default_lock_dir_trace_marker" 'grep -q "b8_lock_dir=${full_default_lock_dir}" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_invalid_lock_scope_case_marker" "B8_REGRESSION_LOCK_SCOPE=cluster" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_invalid_skip_lock_case_marker" "B8_REGRESSION_SKIP_LOCK=2" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_lock_case_marker" "B8_REGRESSION_SKIP_LOCK=1" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_lock_scope_case_marker" "B8_REGRESSION_LOCK_SCOPE=global" "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_lock_scope_env_source_trace_marker" 'grep -q "b8_lock_dir_source=env" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_repo_default_lock_scope_case_marker" 'repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_repo_default_lock_dir_trace_marker" 'grep -q "b8_lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_repo_default_lock_scope_trace_marker" 'grep -q "b8_lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_default_lock_scope_case_marker" 'global_default_lock_dir="/tmp/fem4c_b8_regression.lock"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_default_lock_dir_trace_marker" 'grep -q "b8_lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_default_lock_scope_trace_marker" 'grep -q "b8_lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_lock_dir_case_marker" 'full_skip_lock_dir="${tmp_dir}/full_skip_lock.lock"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_baseline_run_b14_trace_marker" 'grep -q "run_b14_regression=1" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_override_run_b14_trace_marker" 'grep -q "run_b14_regression=1" "${tmp_dir}/override_b14_target.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_b14_target_trace_marker" 'grep -q "b14_target=mbd_ci_contract" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_lock_trace_marker" 'grep -q "b8_skip_lock=1" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_lock_scope_trace_marker" 'grep -q "b8_lock_scope=global" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_lock_dir_trace_marker" 'grep -q "b8_lock_dir=${full_skip_lock_dir}" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_skip_lock_retry_reason_trace_marker" 'grep -q "test_retry_reason=" "${tmp_dir}/skip_b14.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_repo_default_lock_retry_reason_trace_marker" 'grep -q "test_retry_reason=" "${tmp_dir}/repo_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_global_default_lock_retry_reason_trace_marker" 'grep -q "test_retry_reason=" "${tmp_dir}/global_default_lock_scope.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_zero_trace_marker" 'grep -q "test_retry_used=0" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_make_case_marker" 'retry_make_dir="${tmp_dir}/retry_make_dir"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_trace_marker" 'grep -q "parser executable missing after test failure; rebuilding via make all and retrying test once" "${tmp_dir}/retry_parser.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_one_trace_marker" 'grep -q "test_retry_used=1" "${tmp_dir}/retry_parser.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_reason_none_trace_marker" 'grep -q "test_retry_reason=none" "${tmp_dir}/pass.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_reason_parser_missing_trace_marker" 'grep -q "test_retry_reason=parser_missing" "${tmp_dir}/retry_parser.log"' "$b8_regression_full_test_script_path"
check_pattern_in_file "b8_full_test_retry_test_call_count_marker" "rg -n --fixed-strings -- ' test' \"\${retry_call_log}\"" "$b8_regression_full_test_script_path"
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
