#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
source "${SCRIPT_DIR}/coupled_compare_reason_code_audit_cache.sh"
TARGET_NAME="${CONTRACT_AUDIT_TARGET:-coupled_compare_reason_code_contract_checks}"
LOG_PATH="${1:-}"

cache_dir() {
    local cache_root

    cache_root="${CONTRACT_AUDIT_CACHE_ROOT:-$(coupled_compare_reason_code_default_cache_root coupled_compare_reason_code_contract_audit_cache)}"
    coupled_compare_reason_code_cache_dir "${REPO_ROOT}" "${cache_root}"
}

bundle_log_path() {
    coupled_compare_reason_code_bundle_log_path "$(cache_dir)" "${TARGET_NAME}"
}

ensure_bundle_log() {
    coupled_compare_reason_code_ensure_bundle_log "${REPO_ROOT}" "${TARGET_NAME}" "$1"
}

cached_bundle_log="$(bundle_log_path)"
ensure_bundle_log "${cached_bundle_log}"

if [[ -n "${LOG_PATH}" ]]; then
    AUDIT_MODE="logfile"
    mkdir -p "$(dirname "${LOG_PATH}")"
    cp "${cached_bundle_log}" "${LOG_PATH}"
    cat "${LOG_PATH}"
else
    AUDIT_MODE="stdout"
    cat "${cached_bundle_log}"
    LOG_PATH="<stdout>"
fi

echo "contract_audit_target=${TARGET_NAME}"
echo "contract_audit_mode=${AUDIT_MODE}"
echo "contract_audit_log_path=${LOG_PATH}"
echo "contract_audit_cache_log=${cached_bundle_log}"
echo "contract_audit_result=pass"
