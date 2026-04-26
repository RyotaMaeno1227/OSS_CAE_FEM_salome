#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"
bash ./scripts/test_run_coupled_compare_reason_code_contract_stack.sh \
    >/tmp/test_run_coupled_compare_reason_code_contract_stack_modes_explicit.log 2>&1
bash ./scripts/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.sh \
    >/tmp/test_run_coupled_compare_reason_code_contract_stack_modes_default.log 2>&1
bash ./scripts/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.sh \
    >/tmp/test_run_coupled_compare_reason_code_contract_stack_modes_nested.log 2>&1

grep -q '^PASS: coupled_compare reason-code contract stack is stable$' \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_modes_explicit.log
grep -q '^PASS: coupled_compare reason-code contract stack default out_dir is stable$' \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_modes_default.log
grep -q '^PASS: coupled_compare reason-code contract stack creates nested out_dir$' \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_modes_nested.log

echo "PASS: coupled_compare reason-code contract stack modes are stable"
