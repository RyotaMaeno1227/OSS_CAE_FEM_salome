#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"
python3 ./scripts/check_coupled_compare_reason_code_contract_audit_report.py \
    --print-required-keys \
    >/tmp/test_check_coupled_compare_reason_code_contract_audit_report_print_required_keys.log 2>&1

grep -q '^contract_audit_required_keys=contract_audit_target,contract_audit_mode,contract_audit_log_path,contract_audit_cache_log,contract_audit_result$' \
    /tmp/test_check_coupled_compare_reason_code_contract_audit_report_print_required_keys.log
grep -q '^contract_audit_required_pass_lines=PASS: coupled_compare reason-code consistency check is stable|PASS: coupled_compare manifest validator rejects undocumented reason codes$' \
    /tmp/test_check_coupled_compare_reason_code_contract_audit_report_print_required_keys.log

echo "PASS: coupled_compare reason-code contract audit required keys are printable"
