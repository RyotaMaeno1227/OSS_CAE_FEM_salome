#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

run_spot="${RUN_SPOT:-0}"
spot_strict="${SPOT_STRICT:-0}"
run_id="${RUN_ID:-}"
spot_scan_runs="${SPOT_SCAN_RUNS:-20}"
run_b14_regression="${RUN_B14_REGRESSION:-0}"
b8_make_cmd="${B8_MAKE_CMD:-make}"
contract_target="${B8_CONTRACT_TARGET:-mbd_ci_contract}"
local_target="${B8_LOCAL_TARGET:-mbd_checks}"
b14_target="${B8_B14_TARGET:-mbd_b14_regression}"
spot_target="${B8_SPOT_TARGET:-mbd_ci_evidence}"

contract_log="$(mktemp)"
local_log="$(mktemp)"
spot_log="$(mktemp)"
b14_log="$(mktemp)"
cleanup() {
  rm -f "$contract_log" "$local_log" "$spot_log" "$b14_log"
}
trap cleanup EXIT

contract_result="fail"
local_result="fail"
spot_result="skipped"
spot_rc=0
b14_result="skipped"
b14_request="no"

run_make_target() {
  env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR "$b8_make_cmd" -C FEM4C "$1"
}

if run_make_target "$contract_target" >"$contract_log" 2>&1; then
  contract_result="pass"
fi

if run_make_target "$local_target" >"$local_log" 2>&1; then
  local_result="pass"
fi

if [[ "$run_b14_regression" == "1" ]]; then
  b14_request="yes"
  if run_make_target "$b14_target" >"$b14_log" 2>&1; then
    b14_result="pass"
  else
    b14_result="fail"
  fi
fi

spot_run_id="n/a"
spot_step_outcome="n/a"
spot_artifact_present="n/a"
spot_acceptance_result="n/a"
spot_failure_reason="n/a"
spot_error_type="n/a"
spot_retry_after_sec="n/a"
spot_request="no"

if [[ "$run_spot" == "1" ]]; then
  spot_request="yes"
  spot_cmd=(env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR "$b8_make_cmd" -C FEM4C "$spot_target")
  spot_cmd+=("SCAN_RUNS=${spot_scan_runs}")
  if [[ -n "$run_id" ]]; then
    spot_cmd+=("RUN_ID=$run_id")
  fi
  if "${spot_cmd[@]}" >"$spot_log" 2>&1; then
    spot_result="pass"
    spot_rc=0
  else
    spot_rc=$?
    spot_result="fail"
    spot_failure_reason="$(sed -n 's/^ERROR: //p' "$spot_log" | head -n1)"
    spot_failure_reason="${spot_failure_reason:-unknown}"
    spot_error_type="$(awk -F= '/^error_type=/{print $2; exit}' "$spot_log" || true)"
    spot_retry_after_sec="$(awk -F= '/^retry_after_sec=/{print $2; exit}' "$spot_log" || true)"
    spot_error_type="${spot_error_type:-unknown}"
    spot_retry_after_sec="${spot_retry_after_sec:-unknown}"
  fi
  spot_run_id="$(awk -F= '/^run_id=/{print $2; exit}' "$spot_log" || true)"
  spot_step_outcome="$(awk -F= '/^step_outcome=/{print $2; exit}' "$spot_log" || true)"
  spot_artifact_present="$(awk -F= '/^artifact_present=/{print $2; exit}' "$spot_log" || true)"
  spot_acceptance_result="$(awk -F= '/^acceptance_result=/{print $2; exit}' "$spot_log" || true)"
  spot_run_id="${spot_run_id:-unknown}"
  spot_step_outcome="${spot_step_outcome:-unknown}"
  spot_artifact_present="${spot_artifact_present:-unknown}"
  spot_acceptance_result="${spot_acceptance_result:-unknown}"
  if [[ "$spot_result" == "fail" && "$spot_failure_reason" == "unknown" && "$spot_acceptance_result" != "unknown" ]]; then
    spot_failure_reason="acceptance_result=${spot_acceptance_result} step_outcome=${spot_step_outcome} artifact_present=${spot_artifact_present}"
    if [[ "$spot_error_type" == "unknown" ]]; then
      spot_error_type="acceptance_fail"
    fi
  fi
fi

echo "B8_GUARD"
echo "contract_result=$contract_result"
echo "local_regression_result=$local_result"
echo "b8_make_cmd=$b8_make_cmd"
echo "contract_target=$contract_target"
echo "local_target=$local_target"
echo "b14_target=$b14_target"
echo "spot_target=$spot_target"
echo "b14_regression_requested=$b14_request"
echo "b14_regression_result=$b14_result"
echo "spot_requested=$spot_request"
echo "spot_result=$spot_result"
echo "spot_run_id=$spot_run_id"
echo "spot_step_outcome=$spot_step_outcome"
echo "spot_artifact_present=$spot_artifact_present"
echo "spot_acceptance_result=$spot_acceptance_result"
echo "spot_failure_reason=$spot_failure_reason"
echo "spot_error_type=$spot_error_type"
echo "spot_retry_after_sec=$spot_retry_after_sec"
echo "spot_scan_runs=$spot_scan_runs"
echo "spot_strict=$spot_strict"

if [[ "$contract_result" != "pass" || "$local_result" != "pass" || "$b14_result" == "fail" ]]; then
  echo "B8_GUARD_SUMMARY=FAIL (contract/local/b14 regression failed)"
  echo "contract_log_path=$contract_log"
  echo "local_log_path=$local_log"
  if [[ "$b14_request" == "yes" ]]; then
    echo "b14_log_path=$b14_log"
  fi
  if [[ "$spot_request" == "yes" ]]; then
    echo "spot_log_path=$spot_log"
  fi
  exit 2
fi

if [[ "$spot_request" == "yes" && "$spot_strict" == "1" && "$spot_result" != "pass" ]]; then
  echo "B8_GUARD_SUMMARY=FAIL (spot requested in strict mode, spot failed: rc=$spot_rc)"
  echo "spot_log_path=$spot_log"
  exit 2
fi

echo "B8_GUARD_SUMMARY=PASS"
