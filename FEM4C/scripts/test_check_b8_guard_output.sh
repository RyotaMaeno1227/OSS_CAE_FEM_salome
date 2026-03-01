#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

pass_log="${tmp_dir}/pass.log"
cat >"${pass_log}" <<'EOF'
B8_GUARD
contract_result=pass
local_regression_result=pass
b8_make_cmd=make
contract_target=mbd_ci_contract
local_target=mbd_checks
b14_target=mbd_b14_regression
spot_target=mbd_ci_evidence
b14_regression_requested=yes
b14_regression_result=pass
spot_requested=no
spot_result=skipped
spot_run_id=n/a
spot_step_outcome=n/a
spot_artifact_present=n/a
spot_acceptance_result=n/a
spot_failure_reason=n/a
spot_error_type=n/a
spot_retry_after_sec=n/a
spot_scan_runs=20
spot_strict=0
B8_GUARD_SUMMARY=PASS
EOF

if ! bash FEM4C/scripts/check_b8_guard_output.sh "${pass_log}" >"${tmp_dir}/pass.out" 2>&1; then
  echo "FAIL: check_b8_guard_output should pass for valid log" >&2
  cat "${tmp_dir}/pass.out" >&2
  exit 1
fi

fail_log="${tmp_dir}/fail.log"
cp "${pass_log}" "${fail_log}"
sed -i '/^spot_error_type=/d' "${fail_log}"

if bash FEM4C/scripts/check_b8_guard_output.sh "${fail_log}" >"${tmp_dir}/fail.out" 2>&1; then
  echo "FAIL: check_b8_guard_output should fail when required key is missing" >&2
  cat "${tmp_dir}/fail.out" >&2
  exit 1
fi

if ! grep -q "missing key in B8 guard output: spot_error_type" "${tmp_dir}/fail.out"; then
  echo "FAIL: expected missing-key diagnostic was not found" >&2
  cat "${tmp_dir}/fail.out" >&2
  exit 1
fi

echo "PASS: check_b8_guard_output self-test (pass + expected fail path)"
