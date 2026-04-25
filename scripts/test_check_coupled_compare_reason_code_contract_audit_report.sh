#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TMP_DIR="$(mktemp -d)"
REPORT_LOG="${TMP_DIR}/contract_audit_report.log"
BUNDLE_LOG="${TMP_DIR}/focused_bundle.log"
CACHE_LOG="${TMP_DIR}/focused_bundle_cache.log"

cleanup() {
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
cat >"${BUNDLE_LOG}" <<'EOF'
PASS: coupled_compare reason-code consistency check is stable
PASS: coupled_compare manifest validator rejects undocumented reason codes
EOF
cp "${BUNDLE_LOG}" "${CACHE_LOG}"
cat >"${REPORT_LOG}" <<EOF
PASS: coupled_compare reason-code consistency check is stable
PASS: coupled_compare manifest validator rejects undocumented reason codes
contract_audit_target=coupled_compare_reason_code_contract_checks
contract_audit_mode=logfile
contract_audit_log_path=${BUNDLE_LOG}
contract_audit_cache_log=${CACHE_LOG}
contract_audit_result=pass
EOF
python3 ./scripts/check_coupled_compare_reason_code_contract_audit_report.py \
    "${REPORT_LOG}" \
    >/tmp/test_check_coupled_compare_reason_code_contract_audit_report.log 2>&1

grep -q '^PASS: coupled_compare reason-code contract audit report$' \
    /tmp/test_check_coupled_compare_reason_code_contract_audit_report.log

echo "PASS: coupled_compare reason-code contract audit report validator is stable"
