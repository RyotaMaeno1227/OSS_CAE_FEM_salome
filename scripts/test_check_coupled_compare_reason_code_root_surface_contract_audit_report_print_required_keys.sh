#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"
python3 ./scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py \
    --print-required-keys \
    >/tmp/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_print_required_keys.log 2>&1

grep -q '^root_surface_contract_audit_required_keys=root_surface_contract_audit_target,root_surface_contract_audit_mode,root_surface_contract_audit_log_path,root_surface_contract_audit_cache_log,root_surface_contract_audit_result$' \
    /tmp/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_print_required_keys.log
grep -q '^root_surface_contract_audit_required_pass_lines=PASS: coupled_compare reason-code doc sync check is stable|PASS: coupled_compare reason-code root surface audit modes are stable$' \
    /tmp/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_print_required_keys.log

echo "PASS: coupled_compare reason-code root surface contract audit required keys are printable"
