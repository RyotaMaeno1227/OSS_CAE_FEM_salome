#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${1:-${TMPDIR:-/tmp}/coupled_compare_reason_code_contract_stack}"
FEM4C_LOG_PATH="${OUT_DIR}/fem4c_contract_bundle.log"
AUDIT_LOG_PATH="${OUT_DIR}/audit_wrapper_modes.log"
REPORT_LOG_PATH="${OUT_DIR}/contract_audit_report.log"

mkdir -p "${OUT_DIR}"

make -C "${REPO_ROOT}/FEM4C" coupled_compare_reason_code_contract_checks \
    >"${FEM4C_LOG_PATH}" 2>&1
bash "${REPO_ROOT}/scripts/test_run_coupled_compare_reason_code_contract_audit_modes.sh" \
    >"${AUDIT_LOG_PATH}" 2>&1
make -C "${REPO_ROOT}/FEM4C" coupled_compare_reason_code_contract_audit_report_test \
    >"${REPORT_LOG_PATH}" 2>&1

cat "${FEM4C_LOG_PATH}"
cat "${AUDIT_LOG_PATH}"
cat "${REPORT_LOG_PATH}"
echo "contract_stack_components=fem4c_bundle,audit_wrapper_modes,contract_audit_report"
echo "contract_stack_out_dir=${OUT_DIR}"
echo "contract_stack_bundle_log=${FEM4C_LOG_PATH}"
echo "contract_stack_audit_modes_log=${AUDIT_LOG_PATH}"
echo "contract_stack_contract_report_log=${REPORT_LOG_PATH}"
echo "contract_stack_result=pass"
