#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
cleanup() {
  pkill -P $$ 2>/dev/null || true
  if [[ -n "${script_copy}" && -f "${script_copy}" ]]; then
    rm -f "${script_copy}"
  fi
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

if ! env -u MAKEFLAGS -u MFLAGS make -C FEM4C >"${tmp_dir}/build.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial self-test requires successful FEM4C build preflight" >&2
  cat "${tmp_dir}/build.log" >&2
  exit 1
fi

if ! A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/baseline_lock" A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_pass.log" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_ACCEPT_SERIAL_SUMMARY .*retry_on_137=1 .*fake_137_step=none .*step_log_dir=none .*full_test=pass .*batch_test=pass .*ci_contract_test=pass .*overall=pass failed_step=none failed_cmd=none failed_rc=0 failed_log=none$" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_a24_acceptance_serial baseline should emit pass summary marker" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "full_test_attempts=1 .*full_test_retry_used=0 .*batch_test_attempts=1 .*batch_test_retry_used=0 .*ci_contract_test_attempts=1 .*ci_contract_test_retry_used=0" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_a24_acceptance_serial baseline should emit attempts/retry-used markers" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if [[ ! -s "${tmp_dir}/summary_pass.log" ]]; then
  echo "FAIL: run_a24_acceptance_serial baseline should write summary output file" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/invalid_knob_lock" A24_ACCEPT_SERIAL_RETRY_ON_137=2 bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/invalid_knob.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when retry knob is invalid" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if ! grep -q "A24_ACCEPT_SERIAL_RETRY_ON_137 must be 0 or 1" "${tmp_dir}/invalid_knob.log"; then
  echo "FAIL: expected invalid retry knob diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/invalid_fake_step_lock" A24_ACCEPT_SERIAL_FAKE_137_STEP=bad_step bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/invalid_fake_step.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when fake rc137 step knob is invalid" >&2
  cat "${tmp_dir}/invalid_fake_step.log" >&2
  exit 1
fi

if ! grep -q "A24_ACCEPT_SERIAL_FAKE_137_STEP must be one of" "${tmp_dir}/invalid_fake_step.log"; then
  echo "FAIL: expected invalid fake-step diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_fake_step.log" >&2
  exit 1
fi

if ! A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/retry_pass_lock" A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_retry_pass.log" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/retry_pass.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should pass when simulated rc=137 is retried" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_ACCEPT_SERIAL_SUMMARY .*retry_on_137=1 .*fake_137_step=batch_test .*step_log_dir=none .*full_test=pass .*batch_test=pass .*ci_contract_test=pass .*overall=pass failed_step=none failed_cmd=none failed_rc=0 failed_log=none$" "${tmp_dir}/retry_pass.log"; then
  echo "FAIL: retry pass case should emit pass summary with fake_137_step=batch_test" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if ! grep -q "batch_test_attempts=2 .*batch_test_retry_used=1" "${tmp_dir}/retry_pass.log"; then
  echo "FAIL: retry pass case should record batch retry usage" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/retry_fail_lock" A24_ACCEPT_SERIAL_RETRY_ON_137=0 A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_retry_fail.log" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/retry_fail.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when simulated rc=137 occurs with retry disabled" >&2
  cat "${tmp_dir}/retry_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_ACCEPT_SERIAL_SUMMARY .*retry_on_137=0 .*fake_137_step=batch_test .*step_log_dir=none .*full_test=pass .*batch_test=fail .*ci_contract_test=skip .*overall=fail failed_step=batch_test failed_cmd=make_mbd_a24_batch_test failed_rc=137 failed_log=none$" "${tmp_dir}/retry_fail.log"; then
  echo "FAIL: retry disabled case should emit rc=137 failure summary for batch step" >&2
  cat "${tmp_dir}/retry_fail.log" >&2
  exit 1
fi

if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/invalid_step_log_dir_lock" A24_ACCEPT_SERIAL_STEP_LOG_DIR="/proc/fem4c_a24_step_logs" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/invalid_step_log_dir.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when step-log dir is not creatable" >&2
  cat "${tmp_dir}/invalid_step_log_dir.log" >&2
  exit 1
fi

if ! grep -q "cannot create A24 acceptance serial step-log dir" "${tmp_dir}/invalid_step_log_dir.log"; then
  echo "FAIL: expected invalid step-log dir diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_step_log_dir.log" >&2
  exit 1
fi

touch "${tmp_dir}/step_log_file"
if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/step_log_file_lock" A24_ACCEPT_SERIAL_STEP_LOG_DIR="${tmp_dir}/step_log_file" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/invalid_step_log_file.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when step-log path is a file" >&2
  cat "${tmp_dir}/invalid_step_log_file.log" >&2
  exit 1
fi

if ! grep -q "A24 acceptance serial step-log dir must be a directory" "${tmp_dir}/invalid_step_log_file.log"; then
  echo "FAIL: expected step-log dir type diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_step_log_file.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/step_logs_readonly"
chmod 555 "${tmp_dir}/step_logs_readonly"
if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/step_log_readonly_lock" A24_ACCEPT_SERIAL_STEP_LOG_DIR="${tmp_dir}/step_logs_readonly" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/invalid_step_log_readonly.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when step-log dir is not writable" >&2
  cat "${tmp_dir}/invalid_step_log_readonly.log" >&2
  exit 1
fi
chmod 755 "${tmp_dir}/step_logs_readonly"

if ! grep -q "A24 acceptance serial step-log dir is not writable" "${tmp_dir}/invalid_step_log_readonly.log"; then
  echo "FAIL: expected step-log dir writable diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_step_log_readonly.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/step_logs"
if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/step_log_fail_lock" A24_ACCEPT_SERIAL_RETRY_ON_137=0 A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test A24_ACCEPT_SERIAL_STEP_LOG_DIR="${tmp_dir}/step_logs" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/step_log_fail.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail in step-log case when retry is disabled" >&2
  cat "${tmp_dir}/step_log_fail.log" >&2
  exit 1
fi

expected_failed_log="${tmp_dir}/step_logs/batch_test.attempt1.log"
if ! grep -F -q "failed_log=${expected_failed_log}" "${tmp_dir}/step_log_fail.log"; then
  echo "FAIL: step-log case should emit failed_log path in summary" >&2
  cat "${tmp_dir}/step_log_fail.log" >&2
  exit 1
fi

if [[ ! -s "${expected_failed_log}" ]]; then
  echo "FAIL: step-log case should create non-empty failed step log file" >&2
  cat "${tmp_dir}/step_log_fail.log" >&2
  exit 1
fi

script_copy="${root_dir}/FEM4C/scripts/.tmp_run_a24_acceptance_serial_fail.sh"
cp "FEM4C/scripts/run_a24_acceptance_serial.sh" "${script_copy}"

sed -i 's/"mbd_a24_batch_test"/"mbd_a24_batch_test_missing"/' "${script_copy}"

if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/fail_lock" A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_fail.log" bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_a24_acceptance_serial should fail when batch target is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "No rule to make target 'mbd_a24_batch_test_missing'" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-target diagnostic was not found for batch step" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_ACCEPT_SERIAL_SUMMARY .*fake_137_step=none .*step_log_dir=none .*full_test=pass .*batch_test=fail .*ci_contract_test=skip .*overall=fail failed_step=batch_test failed_cmd=make_mbd_a24_batch_test_missing failed_rc=2 failed_log=none$" "${tmp_dir}/fail.log"; then
  echo "FAIL: modified run_a24_acceptance_serial should emit fail summary for batch step" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/lock_held"
printf '%s\n' "$$" >"${tmp_dir}/lock_held/pid"
if A24_ACCEPT_SERIAL_LOCK_DIR="${tmp_dir}/lock_held" A24_ACCEPT_SERIAL_SUMMARY_OUT="${tmp_dir}/summary_lock.log" bash "FEM4C/scripts/run_a24_acceptance_serial.sh" >"${tmp_dir}/lock.log" 2>&1; then
  echo "FAIL: run_a24_acceptance_serial should fail when lock is already held" >&2
  cat "${tmp_dir}/lock.log" >&2
  exit 1
fi

if ! grep -q "FAIL: a24 acceptance serial lock is already held" "${tmp_dir}/lock.log"; then
  echo "FAIL: expected lock-held diagnostic was not found" >&2
  cat "${tmp_dir}/lock.log" >&2
  exit 1
fi

if ! grep -q "^A24_ACCEPT_SERIAL_SUMMARY .*fake_137_step=none .*step_log_dir=none .*full_test=skip .*batch_test=skip .*ci_contract_test=skip .*overall=fail failed_step=lock failed_cmd=lock failed_rc=1 failed_log=none$" "${tmp_dir}/lock.log"; then
  echo "FAIL: lock-held path is missing fail summary marker" >&2
  cat "${tmp_dir}/lock.log" >&2
  exit 1
fi

echo "PASS: run_a24_acceptance_serial self-test (pass + retry rc137 pass/fail + step-log contract + invalid knobs + expected fail path + lock-held path + summary-output)"
