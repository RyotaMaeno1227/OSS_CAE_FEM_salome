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
  echo "FAIL: run_a24_regression self-test requires successful FEM4C build preflight" >&2
  cat "${tmp_dir}/build.log" >&2
  exit 1
fi

if ! A24_RUN_CONTRACT_TEST=0 bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_a24_regression should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 .*integrator_attempts=1 .*ci_contract_attempts=1 .*ci_contract_test_attempts=0 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_a24_regression baseline should emit pass summary marker" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

script_copy="${root_dir}/FEM4C/scripts/.tmp_run_a24_regression_fail.sh"
cp "FEM4C/scripts/run_a24_regression.sh" "${script_copy}"

# Break one make target so we can verify fail-fast behavior.
sed -i 's/"mbd_ci_contract"/"mbd_ci_contract_missing"/' "${script_copy}"

if A24_RUN_CONTRACT_TEST=0 bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_a24_regression should fail when command is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "No rule to make target 'mbd_ci_contract_missing'" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-target diagnostic was not found" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 .*integrator_attempts=1 .*ci_contract_attempts=1 .*ci_contract_test_attempts=0 .*overall=fail failed_step=ci_contract failed_cmd=make_mbd_ci_contract_missing$" "${tmp_dir}/fail.log"; then
  echo "FAIL: modified run_a24_regression should emit fail summary marker for ci_contract step" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if A24_RUN_CONTRACT_TEST=2 bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/config_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when A24_RUN_CONTRACT_TEST is out of range" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24_RUN_CONTRACT_TEST must be 0 or 1" "${tmp_dir}/config_fail.log"; then
  echo "FAIL: expected A24_RUN_CONTRACT_TEST validation error was not found" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=2 .*integrator_attempts=0 .*ci_contract_attempts=0 .*ci_contract_test_attempts=0 .*overall=fail failed_step=config failed_cmd=run_contract_test$" "${tmp_dir}/config_fail.log"; then
  echo "FAIL: config-fail path should emit summary marker for run_contract_test validation" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/lock_held"
printf '%s\n' "$$" >"${tmp_dir}/lock_held/pid"
if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_LOCK_DIR="${tmp_dir}/lock_held" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/lock_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when lock directory is already held" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: a24 regression lock is already held" "${tmp_dir}/lock_fail.log"; then
  echo "FAIL: expected lock-held diagnostic was not found" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=held .*integrator_attempts=0 .*ci_contract_attempts=0 .*ci_contract_test_attempts=0 .*overall=fail failed_step=lock failed_cmd=lock$" "${tmp_dir}/lock_fail.log"; then
  echo "FAIL: lock-held path should emit fail summary marker for lock step" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SKIP_LOCK=2 bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/skip_lock_config_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when A24_REGRESSION_SKIP_LOCK is out of range" >&2
  cat "${tmp_dir}/skip_lock_config_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24_REGRESSION_SKIP_LOCK must be 0 or 1" "${tmp_dir}/skip_lock_config_fail.log"; then
  echo "FAIL: expected A24_REGRESSION_SKIP_LOCK validation error was not found" >&2
  cat "${tmp_dir}/skip_lock_config_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=not_used .*integrator_attempts=0 .*ci_contract_attempts=0 .*ci_contract_test_attempts=0 .*overall=fail failed_step=config failed_cmd=skip_lock$" "${tmp_dir}/skip_lock_config_fail.log"; then
  echo "FAIL: skip-lock config-fail path should emit summary marker for skip_lock validation" >&2
  cat "${tmp_dir}/skip_lock_config_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/skip_lock_held"
printf '%s\n' "$$" >"${tmp_dir}/skip_lock_held/pid"
if ! A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SKIP_LOCK=1 A24_REGRESSION_LOCK_DIR="${tmp_dir}/skip_lock_held" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/skip_lock_pass.log" 2>&1; then
  echo "FAIL: run_a24_regression should pass with A24_REGRESSION_SKIP_LOCK=1 even when lock directory exists" >&2
  cat "${tmp_dir}/skip_lock_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=skipped .*integrator_attempts=1 .*ci_contract_attempts=1 .*ci_contract_test_attempts=0 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/skip_lock_pass.log"; then
  echo "FAIL: skip-lock pass path should emit lock=skipped summary marker" >&2
  cat "${tmp_dir}/skip_lock_pass.log" >&2
  exit 1
fi

summary_out_file="${tmp_dir}/summary_out.log"
if ! A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SUMMARY_OUT="${summary_out_file}" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/summary_out_pass.log" 2>&1; then
  echo "FAIL: run_a24_regression should pass when A24_REGRESSION_SUMMARY_OUT points to a writable file" >&2
  cat "${tmp_dir}/summary_out_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 .*overall=pass failed_step=none failed_cmd=none$" "${summary_out_file}"; then
  echo "FAIL: summary_out pass path should persist summary marker to output file" >&2
  cat "${tmp_dir}/summary_out_pass.log" >&2
  cat "${summary_out_file}" >&2 || true
  exit 1
fi

if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/missing_summary_dir/summary.log" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/summary_out_missing_dir_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when summary-out parent directory is missing" >&2
  cat "${tmp_dir}/summary_out_missing_dir_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24 regression summary output directory does not exist" "${tmp_dir}/summary_out_missing_dir_fail.log"; then
  echo "FAIL: expected summary-out missing-dir validation error was not found" >&2
  cat "${tmp_dir}/summary_out_missing_dir_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=not_used .*overall=fail failed_step=config failed_cmd=summary_out_dir$" "${tmp_dir}/summary_out_missing_dir_fail.log"; then
  echo "FAIL: summary-out missing-dir path should emit summary marker with failed_cmd=summary_out_dir" >&2
  cat "${tmp_dir}/summary_out_missing_dir_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/summary_dir_path"
if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/summary_dir_path" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/summary_out_dir_path_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when summary-out path points to a directory" >&2
  cat "${tmp_dir}/summary_out_dir_path_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24 regression summary output path must be a file" "${tmp_dir}/summary_out_dir_path_fail.log"; then
  echo "FAIL: expected summary-out path-type validation error was not found" >&2
  cat "${tmp_dir}/summary_out_dir_path_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=not_used .*overall=fail failed_step=config failed_cmd=summary_out_type$" "${tmp_dir}/summary_out_dir_path_fail.log"; then
  echo "FAIL: summary-out directory path should emit summary marker with failed_cmd=summary_out_type" >&2
  cat "${tmp_dir}/summary_out_dir_path_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/summary_parent_readonly"
chmod 555 "${tmp_dir}/summary_parent_readonly"
if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SUMMARY_OUT="${tmp_dir}/summary_parent_readonly/summary.log" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/summary_out_readonly_parent_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression should fail when summary-out parent directory is not writable" >&2
  cat "${tmp_dir}/summary_out_readonly_parent_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: cannot write A24 regression summary output" "${tmp_dir}/summary_out_readonly_parent_fail.log"; then
  echo "FAIL: expected summary-out readonly-parent validation error was not found" >&2
  cat "${tmp_dir}/summary_out_readonly_parent_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=not_used .*overall=fail failed_step=config failed_cmd=summary_out_write$" "${tmp_dir}/summary_out_readonly_parent_fail.log"; then
  echo "FAIL: summary-out readonly-parent path should emit summary marker with failed_cmd=summary_out_write" >&2
  cat "${tmp_dir}/summary_out_readonly_parent_fail.log" >&2
  exit 1
fi

summary_out_write_probe="/proc/1/cmdline"
if ! bash -c ': >"$1"' _ "${summary_out_write_probe}" >/dev/null 2>&1; then
  if A24_RUN_CONTRACT_TEST=0 A24_REGRESSION_SUMMARY_OUT="${summary_out_write_probe}" bash "FEM4C/scripts/run_a24_regression.sh" >"${tmp_dir}/summary_out_write_fail.log" 2>&1; then
    echo "FAIL: run_a24_regression should fail when summary-out target is not writable" >&2
    cat "${tmp_dir}/summary_out_write_fail.log" >&2
    exit 1
  fi

  if ! grep -q "FAIL: cannot write A24 regression summary output" "${tmp_dir}/summary_out_write_fail.log"; then
    echo "FAIL: expected summary-out write validation error was not found" >&2
    cat "${tmp_dir}/summary_out_write_fail.log" >&2
    exit 1
  fi

  if ! grep -q "^A24_REGRESSION_SUMMARY contract_test=0 lock=not_used .*overall=fail failed_step=config failed_cmd=summary_out_write$" "${tmp_dir}/summary_out_write_fail.log"; then
    echo "FAIL: summary-out write-fail path should emit summary marker with failed_cmd=summary_out_write" >&2
    cat "${tmp_dir}/summary_out_write_fail.log" >&2
    exit 1
  fi
fi

echo "PASS: run_a24_regression self-test (pass + expected fail path + summary-output + makeflags-isolation)"
