#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
resolve_workspace_root() {
    local start_dir="${1:-$(pwd)}"
    local candidate="${start_dir}"
    while [[ "${candidate}" != "/" ]]; do
        if [[ -d "${candidate}/scripts" && -d "${candidate}/docs" && -d "${candidate}/examples" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
        candidate="$(dirname "${candidate}")"
    done
    local git_root=""
    git_root="$(git -C "${start_dir}" rev-parse --show-toplevel 2>/dev/null || true)"
    if [[ -n "${git_root}" && -d "${git_root}/FEM4C/scripts" && -d "${git_root}/FEM4C/docs" && -d "${git_root}/FEM4C/examples" ]]; then
        printf '%s\n' "${git_root}/FEM4C"
        return 0
    fi
    if [[ -n "${git_root}" && -d "${git_root}/scripts" && -d "${git_root}/docs" && -d "${git_root}/examples" ]]; then
        printf '%s\n' "${git_root}"
        return 0
    fi
    echo "FAIL: unable to resolve workspace root" >&2
    return 1
}
ROOT_DIR="$(resolve_workspace_root "${SCRIPT_DIR}")"
OUT_DIR="${FEM4C_INVOLUTE_GEAR_LOCAL_PATCH_OUTDIR:-${TMPDIR:-/tmp}/fem4c_involute_gear_local_patch_oneway}"
LOG_PATH="${OUT_DIR}/check.log"
SUMMARY_JSON="${OUT_DIR}/involute_gear_local_patch_summary.json"
SUMMARY_MD="${OUT_DIR}/involute_gear_local_patch_summary.md"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j >"${OUT_DIR}/make.log" 2>&1

python3 "${ROOT_DIR}/scripts/build_involute_gear_static_case.py" "${OUT_DIR}" >"${LOG_PATH}" 2>&1

for path in "${LOG_PATH}" "${SUMMARY_JSON}" "${SUMMARY_MD}"; do
    if [[ ! -f "${path}" ]]; then
        echo "FAIL: missing artifact: ${path}" >&2
        exit 1
    fi
done

python3 - "${SUMMARY_JSON}" "${SUMMARY_MD}" <<'PY'
import json
import pathlib
import sys

summary_json = pathlib.Path(sys.argv[1])
summary_md = pathlib.Path(sys.argv[2])
summary = json.loads(summary_json.read_text(encoding="utf-8"))

if summary.get("status") != "blocked":
    raise SystemExit(f"FAIL: expected blocked status, got {summary.get('status')}")
if summary.get("expected_fail") is not True:
    raise SystemExit("FAIL: expected_fail should be true for current repo")
if summary.get("truthful_result") != "real involute gear macro contact is not executable in the current repo":
    raise SystemExit("FAIL: truthful_result mismatch")
if summary.get("macro_run_executed") is not False:
    raise SystemExit("FAIL: macro_run_executed should be false")
if summary.get("local_patch_oneway_executed") is not False:
    raise SystemExit("FAIL: local_patch_oneway_executed should be false")
if summary.get("png_generated") is not False:
    raise SystemExit("FAIL: png_generated should be false")

blockers = summary.get("blockers", [])
if len(blockers) < 2:
    raise SystemExit(f"FAIL: expected at least 2 blockers, got {len(blockers)}")
blocker_ids = {item.get("id") for item in blockers}
required = {
    "macro_contact_surface_limited",
    "local_patch_bridge_circle_only",
}
if not required.issubset(blocker_ids):
    raise SystemExit(f"FAIL: blocker ids mismatch: {sorted(blocker_ids)}")

for blocker in blockers:
    if not blocker.get("evidence_file"):
        raise SystemExit(f"FAIL: blocker missing evidence_file: {blocker}")
    if not blocker.get("impact"):
        raise SystemExit(f"FAIL: blocker missing impact: {blocker}")

text = summary_md.read_text(encoding="utf-8")
required_text = [
    "status: blocked",
    "expected_fail: true",
    "gear_pin proxy is still proxy",
]
for token in required_text:
    if token not in text:
        raise SystemExit(f"FAIL: summary markdown missing token {token!r}")

print(
    "PASS involute_gear_local_patch_oneway "
    f"status={summary['status']} blocker_count={len(blockers)} expected_fail=true"
)
PY

echo "PASS: involute gear local patch oneway check"
echo "out_dir=${OUT_DIR}"
