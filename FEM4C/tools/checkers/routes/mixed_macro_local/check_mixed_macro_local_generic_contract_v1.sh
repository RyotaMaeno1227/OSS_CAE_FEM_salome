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
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_GENERIC_CONTRACT_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_generic_contract_v1}"
MBD_OUT_DIR="${OUT_DIR}/mbd_trace"
FEM_OUT_DIR="${OUT_DIR}/fem_trace"
MIXED_OUT_DIR="${OUT_DIR}/mixed_bundle"
MANIFEST_PATH="${OUT_DIR}/tiny_mixed_manifest.json"
PROVENANCE_PATH="${MIXED_OUT_DIR}/provenance_manifest.json"
SUMMARY_PATH="${MIXED_OUT_DIR}/request_build_summary.json"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

[[ -f "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md" ]] || {
    echo "FAIL: missing mixed contract doc" >&2
    exit 1
}
[[ -f "${ROOT_DIR}/examples/mixed_macro_local_generic_contract_v1/README.md" ]] || {
    echo "FAIL: missing mixed contract example README" >&2
    exit 1
}

env FEM4C_MBD_GENERIC_CONTACT_TRACE_OUTDIR="${MBD_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_penalty_generic_trace_v1.sh"
env FEM4C_FEM_GENERIC_CONTACT_TRACE_OUTDIR="${FEM_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_penalty_generic_trace_v1.sh"

python3 - "${MBD_OUT_DIR}" "${FEM_OUT_DIR}" "${MANIFEST_PATH}" <<'PY'
import csv
import json
import pathlib
import sys

mbd_out_dir = pathlib.Path(sys.argv[1]).resolve()
fem_out_dir = pathlib.Path(sys.argv[2]).resolve()
manifest_path = pathlib.Path(sys.argv[3]).resolve()

mbd_trace = mbd_out_dir / "generic_trace_active.out.contact_generic_trace.csv"
fem_trace = fem_out_dir / "fem_generic_trace_active.out.fem_contact_generic_trace.csv"

mbd_rows = list(csv.DictReader(mbd_trace.open(newline="", encoding="utf-8")))
fem_rows = list(csv.DictReader(fem_trace.open(newline="", encoding="utf-8")))

mbd_active = next((row for row in mbd_rows if int(float(row["active"])) == 1), None)
fem_active = next((row for row in fem_rows if int(float(row["active_flag"])) == 1), None)
if mbd_active is None:
    raise SystemExit("FAIL: no active MBD trace row")
if fem_active is None:
    raise SystemExit("FAIL: no active FEM trace row")

manifest = {
    "contract_version": "mixed_macro_local_generic_contract_v1",
    "schema_name": "mixed_macro_local_generic_manifest_v1",
    "job_id": "tiny_mixed_manifest_from_trace_checks",
    "output_policy": {
        "request_contract_version": "local_patch_generic_contract_v1",
        "feedback_to_macro": False,
    },
    "sources": [
        {
            "source_engine": "MBD",
            "trace_csv": str(mbd_trace),
            "row_selector": {
                "step": int(float(mbd_active["step"])),
                "pair_id": int(float(mbd_active["pair_id"])),
                "slave_vertex_id": int(float(mbd_active["slave_vertex_id"])),
                "master_segment_id": int(float(mbd_active["master_segment_id"])),
            },
            "provenance": {
                "trace_contract": "mbd_macro_penalty_generic_trace_v1",
                "source_route": "mbd_macro_penalty_generic_trace_v1",
                "selection_reason": "first_active_row",
            },
        },
        {
            "source_engine": "FEM",
            "trace_csv": str(fem_trace),
            "row_selector": {
                "load_step": int(float(fem_active["load_step"])),
                "pair_id": int(float(fem_active["pair_id"])),
                "slave_node_id": int(float(fem_active["slave_node_id"])),
                "master_segment_id": int(float(fem_active["master_segment_id"])),
            },
            "provenance": {
                "trace_contract": "fem_macro_penalty_generic_trace_v1",
                "source_route": "fem_macro_penalty_generic_trace_v1",
                "selection_reason": "first_active_row",
            },
        },
    ],
}

manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
PY

python3 "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" "${MANIFEST_PATH}"
python3 "${ROOT_DIR}/scripts/build_local_patch_generic_requests_from_mixed_macro_manifest.py" \
    "${MANIFEST_PATH}" \
    "${MIXED_OUT_DIR}"

python3 - "${ROOT_DIR}" "${MIXED_OUT_DIR}" "${PROVENANCE_PATH}" "${SUMMARY_PATH}" <<'PY'
import json
import pathlib
import subprocess
import sys

root_dir = pathlib.Path(sys.argv[1]).resolve()
mixed_out_dir = pathlib.Path(sys.argv[2]).resolve()
provenance_path = pathlib.Path(sys.argv[3]).resolve()
summary_path = pathlib.Path(sys.argv[4]).resolve()

request_dir = mixed_out_dir / "requests"
request_paths = sorted(request_dir.glob("request_row*.json"))
if len(request_paths) < 2:
    raise SystemExit("FAIL: expected at least two mixed requests")

for request_path in request_paths:
    subprocess.run(
        [sys.executable, str(root_dir / "scripts" / "validate_local_patch_generic_request.py"), str(request_path)],
        check=True,
    )

provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
summary = json.loads(summary_path.read_text(encoding="utf-8"))

if provenance["request_counts_by_engine"]["MBD"] <= 0:
    raise SystemExit("FAIL: missing MBD requests in provenance manifest")
if provenance["request_counts_by_engine"]["FEM"] <= 0:
    raise SystemExit("FAIL: missing FEM requests in provenance manifest")
if summary["request_counts_by_engine"]["MBD"] <= 0:
    raise SystemExit("FAIL: missing MBD requests in summary")
if summary["request_counts_by_engine"]["FEM"] <= 0:
    raise SystemExit("FAIL: missing FEM requests in summary")

for request_path in request_paths:
    payload = json.loads(request_path.read_text(encoding="utf-8"))
    mixed_provenance = payload.get("mixed_macro_source_provenance")
    if not isinstance(mixed_provenance, dict):
        raise SystemExit(f"FAIL: missing mixed provenance in {request_path}")
    if mixed_provenance.get("source_engine") not in {"MBD", "FEM"}:
        raise SystemExit(f"FAIL: bad source_engine in {request_path}")
    if not isinstance(mixed_provenance.get("source_trace_key"), dict):
        raise SystemExit(f"FAIL: missing source_trace_key in {request_path}")

print(
    "PASS mixed_macro_local_generic_contract_v1 "
    f"request_count={len(request_paths)} "
    f"mbd_requests={summary['request_counts_by_engine']['MBD']} "
    f"fem_requests={summary['request_counts_by_engine']['FEM']}"
)
PY

grep -q "contract / interface / builder" "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md"
grep -q "mixed replay は未実装" "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md"
grep -q "mixed solve は未実装" "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md"
grep -q "live co-sim は未実装" "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_generic_contract_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/examples/mixed_macro_local_generic_contract_v1/README.md"

echo "PASS: mixed_macro_local_generic_contract_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
