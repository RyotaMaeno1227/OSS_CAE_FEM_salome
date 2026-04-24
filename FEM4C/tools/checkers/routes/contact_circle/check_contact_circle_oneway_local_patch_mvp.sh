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
OUT_DIR="${FEM4C_CONTACT_CIRCLE_ONEWAY_LOCAL_PATCH_MVP_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_circle_oneway_local_patch_mvp}"
CASE_NAME="circle_collision_oblique_explicit"
DECK_PATH="${ROOT_DIR}/examples/contact/${CASE_NAME}.dat"
SUMMARY_PATH="${OUT_DIR}/${CASE_NAME}.out"
LOG_PATH="${OUT_DIR}/${CASE_NAME}.log"
HISTORY_PATH="${SUMMARY_PATH}.history.csv"
TRACE_PATH="${SUMMARY_PATH}.contact_trace.csv"
LOCAL_REDUCED_PATH="${OUT_DIR}/circle_oneway_local_patch_mvp.csv"
MVP_ROOT="${OUT_DIR}/local_solver_mvp"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j

ASAN_OPTIONS=detect_leaks=0 \
    "${ROOT_DIR}/bin/fem4c" \
    --mode=mbd \
    --mbd-integrator=explicit \
    --mbd-dt=1.0e-3 \
    --mbd-steps=350 \
    "${DECK_PATH}" \
    "${SUMMARY_PATH}" \
    >"${LOG_PATH}" 2>&1

grep -q "Program completed successfully." "${LOG_PATH}"

python3 "${ROOT_DIR}/scripts/build_circle_local_patch_mvp_from_macro_trace.py" \
    "${HISTORY_PATH}" \
    "${TRACE_PATH}" \
    "${LOCAL_REDUCED_PATH}" \
    "${MVP_ROOT}" \
    oneway_review

python3 - "${LOCAL_REDUCED_PATH}" "${MVP_ROOT}" <<'PY'
import csv
import json
import math
import pathlib
import sys

local_csv = pathlib.Path(sys.argv[1])
mvp_root = pathlib.Path(sys.argv[2])
summary_path = mvp_root / "diagnostics" / "local_patch_mvp_summary.json"
if not local_csv.is_file():
    raise SystemExit(f"FAIL: missing review local CSV {local_csv}")
if not summary_path.is_file():
    raise SystemExit(f"FAIL: missing summary {summary_path}")

rows = list(csv.DictReader(local_csv.open(newline="")))
if not rows:
    raise SystemExit("FAIL: review local CSV is empty")

active_rows = 0
gamma_shift_rows = 0
fn_positive_rows = 0
for row in rows:
    values = [float(row[key]) for key in ("gamma_n", "delta_g_eff", "fn_ref", "p_max")]
    if any(not math.isfinite(v) for v in values):
        raise SystemExit("FAIL: review local CSV has non-finite values")
    if float(row["fn_ref"]) > 0.0:
        active_rows += 1
        fn_positive_rows += 1
        if abs(float(row["gamma_n"]) - 1.0) > 1.0e-12:
            gamma_shift_rows += 1

if gamma_shift_rows <= 0:
    raise SystemExit("FAIL: one-way review artifact has no gamma_n variation")
if fn_positive_rows <= 0:
    raise SystemExit("FAIL: one-way review artifact has no fn_ref > 0 row")

summary = json.loads(summary_path.read_text(encoding="utf-8"))
if summary.get("route") != "circle_macro_trace_local_patch_mvp:oneway_review":
    raise SystemExit(f"FAIL: unexpected route {summary.get('route')}")
if summary.get("output_mode") != "lagged":
    raise SystemExit("FAIL: unexpected output_mode")
if int(summary.get("gamma_not_one_rows", 0)) <= 0:
    raise SystemExit("FAIL: summary gamma_not_one_rows missing")
if int(summary.get("fn_positive_rows", 0)) <= 0:
    raise SystemExit("FAIL: summary fn_positive_rows missing")

metadata_files = sorted((mvp_root / "patch_metadata").glob("**/*.json"))
receiver_files = sorted((mvp_root / "patch_receiver").glob("**/*receiver_summary.json"))
if not metadata_files:
    raise SystemExit("FAIL: missing patch metadata artifacts")
if not receiver_files:
    raise SystemExit("FAIL: missing patch receiver summary artifacts")

print(
    "PASS contact_circle_oneway_local_patch_mvp "
    f"active_rows={active_rows} gamma_shift_rows={gamma_shift_rows} "
    f"fn_positive_rows={fn_positive_rows} metadata_files={len(metadata_files)} "
    f"receiver_files={len(receiver_files)}"
)
PY

echo "PASS: contact circle one-way local patch MVP check"
echo "out_dir=${OUT_DIR}"
