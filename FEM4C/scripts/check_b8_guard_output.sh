#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  FEM4C/scripts/check_b8_guard_output.sh <guard_log_path>
EOF
}

guard_log="${1:-}"
if [[ -z "${guard_log}" ]]; then
  echo "FAIL: guard log path is required" >&2
  usage >&2
  exit 2
fi
if [[ ! -f "${guard_log}" ]]; then
  echo "FAIL: guard log file not found: ${guard_log}" >&2
  exit 2
fi

require_key() {
  local key="$1"
  if ! grep -q "^${key}=" "${guard_log}"; then
    echo "FAIL: missing key in B8 guard output: ${key}" >&2
    cat "${guard_log}" >&2
    exit 1
  fi
}

require_key "contract_result"
require_key "local_regression_result"
require_key "b8_make_cmd"
require_key "contract_target"
require_key "local_target"
require_key "b14_target"
require_key "spot_target"
require_key "b14_regression_requested"
require_key "b14_regression_result"
require_key "spot_requested"
require_key "spot_result"
require_key "spot_run_id"
require_key "spot_step_outcome"
require_key "spot_artifact_present"
require_key "spot_acceptance_result"
require_key "spot_failure_reason"
require_key "spot_error_type"
require_key "spot_retry_after_sec"
require_key "spot_scan_runs"
require_key "spot_strict"

if ! grep -q '^B8_GUARD$' "${guard_log}"; then
  echo "FAIL: missing B8_GUARD header" >&2
  cat "${guard_log}" >&2
  exit 1
fi

if ! grep -q '^B8_GUARD_SUMMARY=' "${guard_log}"; then
  echo "FAIL: missing B8_GUARD_SUMMARY line" >&2
  cat "${guard_log}" >&2
  exit 1
fi

summary="$(sed -n 's/^B8_GUARD_SUMMARY=//p' "${guard_log}" | head -n1)"
if [[ -z "${summary}" ]]; then
  echo "FAIL: B8_GUARD_SUMMARY is empty" >&2
  cat "${guard_log}" >&2
  exit 1
fi

if [[ "${summary}" == "PASS" ]]; then
  if ! grep -q '^contract_result=pass$' "${guard_log}"; then
    echo "FAIL: PASS summary requires contract_result=pass" >&2
    cat "${guard_log}" >&2
    exit 1
  fi
  if ! grep -q '^local_regression_result=pass$' "${guard_log}"; then
    echo "FAIL: PASS summary requires local_regression_result=pass" >&2
    cat "${guard_log}" >&2
    exit 1
  fi
fi

echo "PASS: b8 guard output contract check"
