#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TMP_DIR="$(mktemp -d)"
FILE_LOG="${TMP_DIR}/file_mode.log"
STDOUT_LOG="${TMP_DIR}/stdout_mode.log"
NESTED_LOG="${TMP_DIR}/nested_mode.log"

cleanup() {
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
bash ./scripts/test_run_coupled_compare_reason_code_contract_audit.sh \
    >"${FILE_LOG}" 2>&1
bash ./scripts/test_run_coupled_compare_reason_code_contract_audit_stdout.sh \
    >"${STDOUT_LOG}" 2>&1
bash ./scripts/test_run_coupled_compare_reason_code_contract_audit_nested_log_dir.sh \
    >"${NESTED_LOG}" 2>&1

grep -q '^PASS: coupled_compare reason-code audit wrapper is stable$' \
    "${FILE_LOG}"
grep -q '^PASS: coupled_compare reason-code audit wrapper stdout mode is stable$' \
    "${STDOUT_LOG}"
grep -q '^PASS: coupled_compare reason-code audit wrapper creates nested log directories$' \
    "${NESTED_LOG}"

echo "PASS: coupled_compare reason-code audit wrapper modes are stable"
