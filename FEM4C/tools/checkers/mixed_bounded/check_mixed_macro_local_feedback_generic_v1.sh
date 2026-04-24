#!/usr/bin/env bash
set -euo pipefail

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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(resolve_workspace_root "${SCRIPT_DIR}")"
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_FEEDBACK_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_feedback_generic_v1}"
CONTRACT_OUT_DIR="${OUT_DIR}/contract_route"
ONEWAY_OUT_DIR="${OUT_DIR}/oneway_route"
REDUCED_DIR="${ONEWAY_OUT_DIR}/oneway_run/reduced_feedback"

MANIFEST_PATH="${REDUCED_DIR}/mixed_feedback_reduced_manifest.json"
SUMMARY_PATH="${REDUCED_DIR}/mixed_feedback_reduced_summary.json"
MBD_CSV="${REDUCED_DIR}/mbd_local_feedback_reduced.csv"
FEM_CSV="${REDUCED_DIR}/fem_local_feedback_reduced.csv"
PROVENANCE_PATH="${ONEWAY_OUT_DIR}/oneway_run/mixed_bundle/provenance_manifest.json"
RESPONSE_SUMMARY_PATH="${ONEWAY_OUT_DIR}/oneway_run/mixed_bundle/response_build_summary.json"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md" \
    "${ROOT_DIR}/tools/exporters/mixed_bounded/export_mixed_local_feedback_reduced_generic.py" \
    "${ROOT_DIR}/scripts/check_mixed_macro_local_generic_contract_v1.sh" \
    "${ROOT_DIR}/tools/checkers/mixed_bounded/check_mixed_macro_local_oneway_generic_v1.sh"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_MIXED_MACRO_LOCAL_GENERIC_CONTRACT_V1_OUTDIR="${CONTRACT_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_generic_contract_v1.sh"
env FEM4C_MIXED_MACRO_LOCAL_ONEWAY_GENERIC_V1_OUTDIR="${ONEWAY_OUT_DIR}" \
    bash "${ROOT_DIR}/tools/checkers/mixed_bounded/check_mixed_macro_local_oneway_generic_v1.sh"

python3 "${ROOT_DIR}/tools/exporters/mixed_bounded/export_mixed_local_feedback_reduced_generic.py" \
    "${ONEWAY_OUT_DIR}/oneway_run"

[[ -f "${MANIFEST_PATH}" ]] || { echo "FAIL: missing manifest ${MANIFEST_PATH}" >&2; exit 1; }
[[ -f "${SUMMARY_PATH}" ]] || { echo "FAIL: missing summary ${SUMMARY_PATH}" >&2; exit 1; }
[[ -f "${MBD_CSV}" ]] || { echo "FAIL: missing MBD reduced feedback CSV ${MBD_CSV}" >&2; exit 1; }
[[ -f "${FEM_CSV}" ]] || { echo "FAIL: missing FEM reduced feedback CSV ${FEM_CSV}" >&2; exit 1; }

python3 - "${MANIFEST_PATH}" "${SUMMARY_PATH}" "${MBD_CSV}" "${FEM_CSV}" "${PROVENANCE_PATH}" "${RESPONSE_SUMMARY_PATH}" <<'PY'
import csv
import json
import math
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


manifest_path = pathlib.Path(sys.argv[1]).resolve()
summary_path = pathlib.Path(sys.argv[2]).resolve()
mbd_csv_path = pathlib.Path(sys.argv[3]).resolve()
fem_csv_path = pathlib.Path(sys.argv[4]).resolve()
provenance_path = pathlib.Path(sys.argv[5]).resolve()
response_summary_path = pathlib.Path(sys.argv[6]).resolve()

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
summary = json.loads(summary_path.read_text(encoding="utf-8"))
provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
response_summary = json.loads(response_summary_path.read_text(encoding="utf-8"))

mbd_rows = list(csv.DictReader(mbd_csv_path.open(newline="", encoding="utf-8")))
fem_rows = list(csv.DictReader(fem_csv_path.open(newline="", encoding="utf-8")))

if len(mbd_rows) <= 0:
    fail("mbd_local_feedback_reduced.csv must contain at least one row")
if len(fem_rows) <= 0:
    fail("fem_local_feedback_reduced.csv must contain at least one row")

for rows, engine in ((mbd_rows, "MBD"), (fem_rows, "FEM")):
    for idx, row in enumerate(rows, start=1):
        gamma_n = float(row["gamma_n"])
        delta_g_eff_m = float(row["delta_g_eff_m"])
        fn_ref_n = float(row["fn_ref_n"])
        p_max_pa = float(row["p_max_pa"])
        if not all(math.isfinite(v) for v in (gamma_n, delta_g_eff_m, fn_ref_n, p_max_pa)):
            fail(f"{engine} reduced feedback has non-finite value at row {idx}")

if summary.get("pipeline_mode") != "mixed_feedback_surface_mvp":
    fail("summary pipeline_mode mismatch")
if summary.get("feedback_mode") != "LAGGED_REDUCED":
    fail("summary feedback_mode mismatch")
if summary.get("mixed_replay") is not False:
    fail("summary mixed_replay must be false")
if summary.get("mixed_solve") is not False:
    fail("summary mixed_solve must be false")
if summary.get("live_cosim") is not False:
    fail("summary live_cosim must be false")
if summary.get("feedback_available_flag") is not True:
    fail("summary feedback_available_flag must be true")
if summary.get("skip_reason") != "":
    fail("summary skip_reason must be empty when feedback rows exist")

if summary.get("mbd_feedback_rows") != len(mbd_rows):
    fail("summary mbd_feedback_rows mismatch")
if summary.get("fem_feedback_rows") != len(fem_rows):
    fail("summary fem_feedback_rows mismatch")
if summary.get("total_feedback_rows") != len(mbd_rows) + len(fem_rows):
    fail("summary total_feedback_rows mismatch")
if summary.get("response_count") != response_summary.get("total_response_count"):
    fail("summary response_count mismatch")
if int(summary.get("feedback_row_count", -1)) != len(mbd_rows) + len(fem_rows):
    fail("summary feedback_row_count mismatch")
if int(summary.get("feedback_apply_count", -1)) != 0:
    fail("summary feedback_apply_count must be 0")
if summary.get("last_available_differs_from_last_applied") is not True:
    fail("summary last_available_differs_from_last_applied must be true")
if summary.get("unapplied_terminal_candidate_flag") is not True:
    fail("summary unapplied_terminal_candidate_flag must be true")

manifest_rows = manifest.get("rows")
if not isinstance(manifest_rows, list) or len(manifest_rows) != len(mbd_rows) + len(fem_rows):
    fail("manifest rows length mismatch")
if manifest.get("feedback_available_flag") is not True:
    fail("manifest feedback_available_flag must be true")
if manifest.get("skip_reason") != "":
    fail("manifest skip_reason must be empty")
if int(manifest.get("feedback_row_count", -1)) != len(manifest_rows):
    fail("manifest feedback_row_count mismatch")
if int(manifest.get("feedback_apply_count", -1)) != 0:
    fail("manifest feedback_apply_count must be 0")
if manifest.get("last_available_differs_from_last_applied") is not True:
    fail("manifest last_available_differs_from_last_applied must be true")
if manifest.get("unapplied_terminal_candidate_flag") is not True:
    fail("manifest unapplied_terminal_candidate_flag must be true")

seen_engines = set()
source_row_link_count = 0
for entry in manifest_rows:
    if not isinstance(entry, dict):
        fail("manifest row entry must be an object")
    engine = entry.get("source_engine")
    if engine not in {"MBD", "FEM"}:
        fail("manifest row bad source_engine")
    seen_engines.add(engine)
    feedback_csv = entry.get("feedback_csv")
    if not isinstance(feedback_csv, str) or not feedback_csv:
        fail("manifest row missing feedback_csv")
    feedback_row_key = entry.get("feedback_row_key")
    if not isinstance(feedback_row_key, dict):
        fail("manifest row missing feedback_row_key")
    source_rows = entry.get("source_rows")
    if not isinstance(source_rows, list) or not source_rows:
        fail("manifest row missing source_rows")
    for source_row in source_rows:
        if not isinstance(source_row, dict):
            fail("manifest source_row must be an object")
        for field in ("source_trace_csv", "request_path", "response_path"):
            value = source_row.get(field)
            if not isinstance(value, str) or not value:
                fail(f"manifest source_row missing {field}")
        if not isinstance(source_row.get("source_trace_key"), dict):
            fail("manifest source_row missing source_trace_key")
        source_row_link_count += 1

if seen_engines != {"MBD", "FEM"}:
    fail(f"expected both engines in manifest, got {seen_engines!r}")

if provenance.get("response_count") != response_summary.get("total_response_count"):
    fail("provenance response_count mismatch")
if source_row_link_count != provenance.get("response_count"):
    fail("manifest source row link count does not match response count")

last_manifest_row = manifest_rows[-1]
expected_last_ref = {
    "source_engine": last_manifest_row["source_engine"],
    "feedback_csv": last_manifest_row["feedback_csv"],
    "feedback_row_key": last_manifest_row["feedback_row_key"],
    "feedback_valid_flag": int(last_manifest_row["exported_feedback"]["valid_flag"]),
    "feedback_status": str(last_manifest_row["exported_feedback"]["status"]),
}
if summary.get("last_available_feedback_ref") != expected_last_ref:
    fail("summary last_available_feedback_ref mismatch")
if manifest.get("last_available_feedback_ref") != expected_last_ref:
    fail("manifest last_available_feedback_ref mismatch")
if summary.get("last_applied_feedback_ref") is not None:
    fail("summary last_applied_feedback_ref must be null")
if manifest.get("last_applied_feedback_ref") is not None:
    fail("manifest last_applied_feedback_ref must be null")

print(
    "PASS mixed_macro_local_feedback_generic_v1 "
    f"mbd_feedback_rows={len(mbd_rows)} "
    f"fem_feedback_rows={len(fem_rows)} "
    f"response_count={response_summary['total_response_count']}"
)
PY

grep -q "mixed replay は未実装" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "mixed solve は未実装" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "live co-sim は未実装" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "true lagged mixed time-step co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "same-time mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "feedback_available_flag" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "feedback_apply_count = 0" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "last_applied_feedback_ref = null" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "post-solve artifact" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "friction solver state" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "source_step" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "dt_comm" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"

env FEM4C_MBD_MACRO_LOCAL_FEEDBACK_GENERIC_V1_OUTDIR="${OUT_DIR}/mbd_feedback_regression" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_local_feedback_generic_v1.sh"
env FEM4C_FEM_MACRO_LOCAL_FEEDBACK_GENERIC_V1_OUTDIR="${OUT_DIR}/fem_feedback_regression" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_local_feedback_generic_v1.sh"

echo "PASS: mixed_macro_local_feedback_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
