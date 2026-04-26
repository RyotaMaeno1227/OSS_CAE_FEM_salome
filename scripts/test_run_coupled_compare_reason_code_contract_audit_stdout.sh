#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
RUN_LOG="$(mktemp)"

cleanup() {
    rm -f "${RUN_LOG}"
}
trap cleanup EXIT

cd "${REPO_ROOT}"
bash ./scripts/run_coupled_compare_reason_code_contract_audit.sh \
    >"${RUN_LOG}" 2>&1

cache_log_path="$(awk -F= '/^contract_audit_cache_log=/{value=$2} END{print value}' "${RUN_LOG}")"

grep -q '^PASS: coupled_compare manifest validator rejects undocumented reason codes$' \
    "${RUN_LOG}"
grep -q '^contract_audit_target=coupled_compare_reason_code_contract_checks$' \
    "${RUN_LOG}"
grep -q '^contract_audit_mode=stdout$' \
    "${RUN_LOG}"
grep -q '^contract_audit_log_path=<stdout>$' \
    "${RUN_LOG}"
grep -Fqx "contract_audit_cache_log=${cache_log_path}" \
    "${RUN_LOG}"
grep -q '^contract_audit_result=pass$' \
    "${RUN_LOG}"
[[ -f "${cache_log_path}" ]]

echo "PASS: coupled_compare reason-code audit wrapper stdout mode is stable"
