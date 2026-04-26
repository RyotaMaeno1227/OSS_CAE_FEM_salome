#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TMP_DIR="$(mktemp -d)"
LOG_PATH="$(mktemp)"
RUN_LOG="${TMP_DIR}/contract_audit_stdout.log"

cleanup() {
    rm -rf "${TMP_DIR}"
    rm -f "${LOG_PATH}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
bash ./scripts/run_coupled_compare_reason_code_contract_audit.sh "${LOG_PATH}" \
    >"${RUN_LOG}" 2>&1

cache_log_path="$(awk -F= '/^contract_audit_cache_log=/{value=$2} END{print value}' "${RUN_LOG}")"

grep -q '^PASS: coupled_compare reason-code consistency check is stable$' \
    "${RUN_LOG}"
grep -q '^PASS: coupled_compare manifest validator rejects undocumented reason codes$' \
    "${RUN_LOG}"
grep -q '^contract_audit_target=coupled_compare_reason_code_contract_checks$' \
    "${RUN_LOG}"
grep -q '^contract_audit_mode=logfile$' \
    "${RUN_LOG}"
grep -Fqx "contract_audit_log_path=${LOG_PATH}" \
    "${RUN_LOG}"
grep -Fqx "contract_audit_cache_log=${cache_log_path}" \
    "${RUN_LOG}"
grep -q '^contract_audit_result=pass$' \
    "${RUN_LOG}"
[[ -f "${LOG_PATH}" ]]
[[ -f "${cache_log_path}" ]]
grep -q '^PASS: coupled_compare reason-code consistency check is stable$' "${LOG_PATH}"
grep -q '^PASS: coupled_compare reason-code consistency check is stable$' "${cache_log_path}"

echo "PASS: coupled_compare reason-code audit wrapper is stable"
