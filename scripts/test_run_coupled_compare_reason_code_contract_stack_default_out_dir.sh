#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
STACK_OUT_DIR="${TMPDIR:-/tmp}/coupled_compare_reason_code_contract_stack"

cleanup() {
    rm -rf "${STACK_OUT_DIR}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
bash ./scripts/run_coupled_compare_reason_code_contract_stack.sh \
    >/tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log 2>&1

bundle_log="$(awk -F= '/^contract_stack_bundle_log=/{print $2}' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log)"
audit_log="$(awk -F= '/^contract_stack_audit_modes_log=/{print $2}' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log)"
report_log="$(awk -F= '/^contract_stack_contract_report_log=/{print $2}' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log)"
stack_out_dir="$(awk -F= '/^contract_stack_out_dir=/{print $2}' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log)"
stack_components="$(awk -F= '/^contract_stack_components=/{print $2}' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log)"

[[ "${stack_components}" == 'fem4c_bundle,audit_wrapper_modes,contract_audit_report' ]]
[[ "${stack_out_dir}" == "${STACK_OUT_DIR}" ]]
[[ "${bundle_log}" == "${STACK_OUT_DIR}/fem4c_contract_bundle.log" ]]
[[ "${audit_log}" == "${STACK_OUT_DIR}/audit_wrapper_modes.log" ]]
[[ "${report_log}" == "${STACK_OUT_DIR}/contract_audit_report.log" ]]
[[ -f "${bundle_log}" ]]
[[ -f "${audit_log}" ]]
[[ -f "${report_log}" ]]
grep -q '^contract_stack_result=pass$' /tmp/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.log
grep -q '^PASS: coupled_compare reason-code contract bundle is stable$' "${bundle_log}" || \
    grep -q '^PASS: coupled_compare manifest validator prints known reason-code pattern$' "${bundle_log}"
grep -q '^PASS: coupled_compare reason-code audit wrapper modes are stable$' "${audit_log}"
grep -q '^PASS: coupled_compare reason-code contract audit report validator is stable$' "${report_log}"

echo "PASS: coupled_compare reason-code contract stack default out_dir is stable"
