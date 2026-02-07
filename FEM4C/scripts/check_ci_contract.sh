#!/usr/bin/env bash
set -euo pipefail

workflow="${1:-.github/workflows/ci.yaml}"

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

if [[ ! -f "$workflow" ]]; then
  echo "CI_CONTRACT_CHECK: FAIL (workflow missing: $workflow)" >&2
  exit 1
fi

checks=0
fails=0

check_pattern() {
  local label="$1"
  local pattern="$2"
  checks=$((checks + 1))
  if rg -n --fixed-strings -- "$pattern" "$workflow" > /dev/null; then
    echo "CI_CONTRACT_CHECK[$label]=PASS"
  else
    echo "CI_CONTRACT_CHECK[$label]=FAIL (missing: $pattern)" >&2
    fails=$((fails + 1))
  fi
}

check_pattern "step_name" "- name: Run FEM4C regression entrypoint"
check_pattern "step_id" "id: run_fem4c_tests"
check_pattern "fem4c_test_log" "fem4c_test.log"
check_pattern "failure_gate" "if: steps.run_fem4c_tests.outcome == 'failure'"
check_pattern "test_command" "make -C FEM4C test"
check_pattern "artifact_upload" "- name: Upload test log"

if [[ $fails -ne 0 ]]; then
  echo "CI_CONTRACT_CHECK_SUMMARY=FAIL checks=$checks failed=$fails workflow=$workflow" >&2
  exit 1
fi

echo "CI_CONTRACT_CHECK_SUMMARY=PASS checks=$checks failed=0 workflow=$workflow"
