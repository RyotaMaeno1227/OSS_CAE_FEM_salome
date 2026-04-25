#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPORT_LOG="$(mktemp)"
CACHE_LOG="$(mktemp)"

cleanup() {
    rm -f "${REPORT_LOG}" "${CACHE_LOG}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
cat >"${CACHE_LOG}" <<'EOF'
PASS: coupled_compare reason-code consistency check is stable
PASS: coupled_compare manifest validator rejects undocumented reason codes
EOF
cat >"${REPORT_LOG}" <<'EOF'
PASS: coupled_compare reason-code consistency check is stable
PASS: coupled_compare manifest validator rejects undocumented reason codes
contract_audit_target=coupled_compare_reason_code_contract_checks
contract_audit_mode=stdout
contract_audit_log_path=<stdout>
contract_audit_cache_log=CACHE_LOG_PLACEHOLDER
contract_audit_result=pass
EOF
sed -i "s|CACHE_LOG_PLACEHOLDER|${CACHE_LOG}|" "${REPORT_LOG}"
python3 ./scripts/check_coupled_compare_reason_code_contract_audit_report.py \
    "${REPORT_LOG}" \
    >/tmp/test_check_coupled_compare_reason_code_contract_audit_report_stdout.log 2>&1

grep -q '^PASS: coupled_compare reason-code contract audit report$' \
    /tmp/test_check_coupled_compare_reason_code_contract_audit_report_stdout.log

echo "PASS: coupled_compare reason-code contract audit report stdout mode is stable"
