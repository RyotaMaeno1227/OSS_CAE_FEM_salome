#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TMP_DIR="$(mktemp -d)"
REPORT_LOG="${TMP_DIR}/root_surface_contract_audit_report.log"
BUNDLE_LOG="${TMP_DIR}/focused_bundle.log"
CACHE_LOG="${TMP_DIR}/focused_bundle_cache.log"

cleanup() {
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
cat >"${BUNDLE_LOG}" <<'EOF'
PASS: coupled_compare reason-code doc sync check is stable
PASS: coupled_compare reason-code root surface audit modes are stable
EOF
cp "${BUNDLE_LOG}" "${CACHE_LOG}"
cat >"${REPORT_LOG}" <<EOF
PASS: coupled_compare reason-code doc sync check is stable
PASS: coupled_compare reason-code root surface audit modes are stable
root_surface_contract_audit_target=coupled_compare_reason_code_root_surface_contract_checks
root_surface_contract_audit_mode=logfile
root_surface_contract_audit_log_path=${BUNDLE_LOG}
root_surface_contract_audit_cache_log=${CACHE_LOG}
root_surface_contract_audit_result=pass
EOF
python3 ./scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py \
    "${REPORT_LOG}" \
    >/tmp/test_check_coupled_compare_reason_code_root_surface_contract_audit_report.log 2>&1

grep -q '^PASS: coupled_compare reason-code root surface contract audit report$' \
    /tmp/test_check_coupled_compare_reason_code_root_surface_contract_audit_report.log

echo "PASS: coupled_compare reason-code root surface contract audit report validator is stable"
