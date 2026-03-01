#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

mock_bin="${tmp_dir}/bin"
mkdir -p "$mock_bin"

cat >"${mock_bin}/make" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

args="$*"

if [[ -n "${MAKEFLAGS:-}" || -n "${MFLAGS:-}" ]]; then
  echo "mock make: MAKEFLAGS/MFLAGS should be unset" >&2
  exit 91
fi

if [[ "$args" == *"mbd_ci_contract"* ]]; then
  exit "${MOCK_CONTRACT_RC:-0}"
fi

if [[ "$args" == *"mbd_b14_regression"* ]]; then
  exit "${MOCK_B14_RC:-0}"
fi

if [[ "$args" == *"mbd_ci_evidence"* ]]; then
  if [[ "${MOCK_SPOT_MODE:-pass}" == "fail" ]]; then
    echo "run_id=mock-spot-fail"
    echo "step_outcome=missing"
    echo "artifact_present=yes"
    echo "acceptance_result=fail"
  else
    echo "run_id=mock-spot-pass"
    echo "step_outcome=success"
    echo "artifact_present=yes"
    echo "acceptance_result=pass"
  fi
  exit "${MOCK_SPOT_RC:-0}"
fi

if [[ "$args" == *" mbd_checks"* || "$args" == *" mbd_checks" ]]; then
  exit "${MOCK_LOCAL_RC:-0}"
fi

echo "mock make: unsupported args: $args" >&2
exit 90
EOF

chmod +x "${mock_bin}/make"

run_guard_case() {
  local case_name="$1"
  local expected_rc="$2"
  shift 2

  local case_log="${tmp_dir}/${case_name}.log"

  set +e
  env B8_MAKE_CMD="${mock_bin}/make" "$@" bash FEM4C/scripts/run_b8_guard.sh >"${case_log}" 2>&1
  local rc=$?
  set -e

  if [[ "${rc}" -ne "${expected_rc}" ]]; then
    echo "FAIL: ${case_name} rc mismatch (expected=${expected_rc}, actual=${rc})" >&2
    cat "${case_log}" >&2
    exit 1
  fi

  echo "${case_log}"
}

case_log="$(run_guard_case b14_pass 0 MOCK_CONTRACT_RC=0 MOCK_LOCAL_RC=0 MOCK_B14_RC=0 RUN_B14_REGRESSION=1)"
grep -q "b8_make_cmd=${mock_bin}/make" "${case_log}"
grep -q "contract_target=mbd_ci_contract" "${case_log}"
grep -q "local_target=mbd_checks" "${case_log}"
grep -q "b14_target=mbd_b14_regression" "${case_log}"
grep -q "spot_target=mbd_ci_evidence" "${case_log}"
grep -q "b14_regression_requested=yes" "${case_log}"
grep -q "b14_regression_result=pass" "${case_log}"
grep -q "B8_GUARD_SUMMARY=PASS" "${case_log}"

case_log="$(run_guard_case b14_fail 2 MOCK_CONTRACT_RC=0 MOCK_LOCAL_RC=0 MOCK_B14_RC=3 RUN_B14_REGRESSION=1)"
grep -q "b14_regression_requested=yes" "${case_log}"
grep -q "b14_regression_result=fail" "${case_log}"
grep -q "B8_GUARD_SUMMARY=FAIL (contract/local/b14 regression failed)" "${case_log}"

case_log="$(run_guard_case spot_non_strict_fail 0 MOCK_CONTRACT_RC=0 MOCK_LOCAL_RC=0 RUN_SPOT=1 SPOT_STRICT=0 MOCK_SPOT_MODE=fail MOCK_SPOT_RC=1)"
grep -q "spot_requested=yes" "${case_log}"
grep -q "spot_result=fail" "${case_log}"
grep -q "spot_acceptance_result=fail" "${case_log}"
grep -q "spot_error_type=acceptance_fail" "${case_log}"
grep -q "B8_GUARD_SUMMARY=PASS" "${case_log}"

case_log="$(run_guard_case spot_strict_fail 2 MOCK_CONTRACT_RC=0 MOCK_LOCAL_RC=0 RUN_SPOT=1 SPOT_STRICT=1 MOCK_SPOT_MODE=fail MOCK_SPOT_RC=1)"
grep -q "spot_requested=yes" "${case_log}"
grep -q "spot_result=fail" "${case_log}"
grep -q "B8_GUARD_SUMMARY=FAIL (spot requested in strict mode, spot failed: rc=1)" "${case_log}"

case_log="$(run_guard_case makeflags_isolation 0 MAKEFLAGS=--jobs=9 MFLAGS=--jobs=9 MOCK_CONTRACT_RC=0 MOCK_LOCAL_RC=0)"
grep -q "B8_GUARD_SUMMARY=PASS" "${case_log}"

echo "PASS: run_b8_guard self-test (b14 chaining + spot strict/non-strict behavior + makeflags isolation)"
