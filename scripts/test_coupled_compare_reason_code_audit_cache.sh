#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
source "${REPO_ROOT}/scripts/coupled_compare_reason_code_audit_cache.sh"

TMP_ROOT="$(mktemp -d)"
CACHE_ROOT="${TMP_ROOT}/cache"
TMP_REPO="${TMP_ROOT}/repo"

cleanup() {
    rm -rf "${TMP_ROOT}"
}
trap cleanup EXIT

mkdir -p \
    "${TMP_REPO}/docs" \
    "${TMP_REPO}/scripts" \
    "${TMP_REPO}/FEM4C" \
    "${TMP_REPO}/FEM4C/scripts"

cat >"${TMP_REPO}/FEM4C/Makefile" <<'EOF'
demo_target:
	@echo "PASS: coupled_compare audit cache demo target"
EOF

printf 'run demo\n' >"${TMP_REPO}/docs/team_runbook.md"
printf 'queue demo\n' >"${TMP_REPO}/docs/fem4c_team_next_queue.md"
printf '#!/usr/bin/env bash\n' >"${TMP_REPO}/scripts/run_coupled_compare_reason_code_demo.sh"
printf '#!/usr/bin/env bash\n' >"${TMP_REPO}/FEM4C/scripts/check_coupled_compare_reason_code_demo.sh"
printf '#!/usr/bin/env bash\n' >"${TMP_REPO}/FEM4C/scripts/test_make_coupled_compare_reason_code_root_surface_contract_checks.sh"

cache_dir_without_extra="$(
    coupled_compare_reason_code_cache_dir "${TMP_REPO}" "${CACHE_ROOT}"
)"
cache_dir_with_extra="$(
    coupled_compare_reason_code_cache_dir \
        "${TMP_REPO}" \
        "${CACHE_ROOT}" \
        "${TMP_REPO}/FEM4C/scripts/test_make_coupled_compare_reason_code_root_surface_contract_checks.sh"
)"

[[ "${cache_dir_without_extra}" != "${cache_dir_with_extra}" ]]

bundle_log_path="$(
    coupled_compare_reason_code_bundle_log_path "${cache_dir_with_extra}" demo_target
)"

[[ "${bundle_log_path}" == "${cache_dir_with_extra}/demo_target.log" ]]

coupled_compare_reason_code_ensure_bundle_log "${TMP_REPO}" demo_target "${bundle_log_path}"
grep -q '^PASS: coupled_compare audit cache demo target$' "${bundle_log_path}"
[[ -f "${bundle_log_path}.done" ]]

echo 'CACHE_SENTINEL' >>"${bundle_log_path}"
coupled_compare_reason_code_ensure_bundle_log "${TMP_REPO}" demo_target "${bundle_log_path}"
grep -q '^CACHE_SENTINEL$' "${bundle_log_path}"

rm -f "${bundle_log_path}.done"
coupled_compare_reason_code_ensure_bundle_log "${TMP_REPO}" demo_target "${bundle_log_path}"
if grep -q '^CACHE_SENTINEL$' "${bundle_log_path}"; then
    echo "FAIL: cache rebuild preserved stale sentinel" >&2
    exit 1
fi

echo "PASS: coupled_compare audit cache helper is stable"
