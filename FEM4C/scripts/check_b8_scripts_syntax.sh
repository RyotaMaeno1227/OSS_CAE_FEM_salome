#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

scripts=(
  "FEM4C/scripts/check_b8_guard_output.sh"
  "FEM4C/scripts/run_b8_guard.sh"
  "FEM4C/scripts/test_run_b8_guard.sh"
  "FEM4C/scripts/run_b8_guard_contract.sh"
  "FEM4C/scripts/test_run_b8_guard_contract.sh"
  "FEM4C/scripts/run_b8_regression.sh"
  "FEM4C/scripts/test_run_b8_regression.sh"
  "FEM4C/scripts/run_b8_regression_full.sh"
  "FEM4C/scripts/test_run_b8_regression_full.sh"
)

for script_path in "${scripts[@]}"; do
  if [[ ! -f "${script_path}" ]]; then
    echo "FAIL: missing B-8 script: ${script_path}" >&2
    exit 1
  fi
  bash -n "${script_path}"
done

echo "PASS: b8 script syntax check"
