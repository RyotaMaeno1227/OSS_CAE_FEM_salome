#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TMP_DIR="$(mktemp -d)"
OUT_DIR="${TMP_DIR}/nested/contracts/stack"

cleanup() {
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
bash ./scripts/run_coupled_compare_reason_code_contract_stack.sh "${OUT_DIR}" \
    >/tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log 2>&1

[[ -f "${OUT_DIR}/fem4c_contract_bundle.log" ]]
[[ -f "${OUT_DIR}/audit_wrapper_modes.log" ]]
[[ -f "${OUT_DIR}/contract_audit_report.log" ]]
grep -q '^contract_stack_components=fem4c_bundle,audit_wrapper_modes,contract_audit_report$' \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log
grep -q "^contract_stack_out_dir=${OUT_DIR}$" \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log
grep -q "^contract_stack_bundle_log=${OUT_DIR}/fem4c_contract_bundle.log$" \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log
grep -q "^contract_stack_audit_modes_log=${OUT_DIR}/audit_wrapper_modes.log$" \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log
grep -q "^contract_stack_contract_report_log=${OUT_DIR}/contract_audit_report.log$" \
    /tmp/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.log
grep -q '^PASS: coupled_compare manifest validator prints known reason-code pattern$' \
    "${OUT_DIR}/fem4c_contract_bundle.log"
grep -q '^PASS: coupled_compare reason-code audit wrapper modes are stable$' \
    "${OUT_DIR}/audit_wrapper_modes.log"
grep -q '^PASS: coupled_compare reason-code contract audit report validator is stable$' \
    "${OUT_DIR}/contract_audit_report.log"

echo "PASS: coupled_compare reason-code contract stack creates nested out_dir"
