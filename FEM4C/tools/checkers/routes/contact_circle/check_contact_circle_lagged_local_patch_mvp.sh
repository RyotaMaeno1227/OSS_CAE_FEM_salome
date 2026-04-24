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
OUT_DIR="${FEM4C_CONTACT_CIRCLE_LAGGED_LOCAL_PATCH_MVP_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_circle_lagged_local_patch_mvp}"
CASE_NAME="circle_collision_oblique_lagged_reduced"
OFFICIAL_DECK="${ROOT_DIR}/examples/contact/${CASE_NAME}.dat"
SEED_SUMMARY="${OUT_DIR}/${CASE_NAME}.seed.out"
SEED_LOG="${OUT_DIR}/${CASE_NAME}.seed.log"
SEED_HISTORY="${SEED_SUMMARY}.history.csv"
SEED_TRACE="${SEED_SUMMARY}.contact_trace.csv"
GENERATED_LOCAL="${OUT_DIR}/local_contact_circle_patch_mvp.csv"
MVP_ROOT="${OUT_DIR}/local_solver_mvp"
TEMP_DECK="${OUT_DIR}/${CASE_NAME}.patch_mvp.dat"
REPLAY_SUMMARY="${OUT_DIR}/${CASE_NAME}.replay.out"
REPLAY_LOG="${OUT_DIR}/${CASE_NAME}.replay.log"
REPLAY_FEEDBACK_USE="${REPLAY_SUMMARY}.contact_feedback_use.csv"
REPLAY_REDUCED="${REPLAY_SUMMARY}.contact_reduced_data.csv"
EHL_FILE="${ROOT_DIR}/examples/contact/ehl_circle_contract.csv"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j

ASAN_OPTIONS=detect_leaks=0 \
    "${ROOT_DIR}/bin/fem4c" \
    --mode=mbd \
    --mbd-integrator=explicit \
    --mbd-dt=1.0e-3 \
    --mbd-steps=350 \
    "${OFFICIAL_DECK}" \
    "${SEED_SUMMARY}" \
    >"${SEED_LOG}" 2>&1

grep -q "Program completed successfully." "${SEED_LOG}"

python3 "${ROOT_DIR}/scripts/build_circle_local_patch_mvp_from_macro_trace.py" \
    "${SEED_HISTORY}" \
    "${SEED_TRACE}" \
    "${GENERATED_LOCAL}" \
    "${MVP_ROOT}" \
    lagged_reduced

python3 - "${OFFICIAL_DECK}" "${TEMP_DECK}" "${GENERATED_LOCAL}" "${EHL_FILE}" <<'PY'
from pathlib import Path
import sys

official_deck = Path(sys.argv[1])
temp_deck = Path(sys.argv[2])
generated_local = Path(sys.argv[3]).resolve()
ehl_file = Path(sys.argv[4]).resolve()
text = official_deck.read_text(encoding="utf-8")
text = text.replace(
    "MBD_LOCAL_CONTACT_FILE examples/contact/local_contact_circle_contract.csv",
    f"MBD_LOCAL_CONTACT_FILE {generated_local}",
)
text = text.replace(
    "MBD_EHL_FILE examples/contact/ehl_circle_contract.csv",
    f"MBD_EHL_FILE {ehl_file}",
)
temp_deck.write_text(text, encoding="utf-8")
PY

ASAN_OPTIONS=detect_leaks=0 \
    "${ROOT_DIR}/bin/fem4c" \
    --mode=mbd \
    --mbd-integrator=explicit \
    --mbd-dt=1.0e-3 \
    --mbd-steps=350 \
    "${TEMP_DECK}" \
    "${REPLAY_SUMMARY}" \
    >"${REPLAY_LOG}" 2>&1

grep -q "Program completed successfully." "${REPLAY_LOG}"

python3 - "${GENERATED_LOCAL}" "${MVP_ROOT}" "${REPLAY_FEEDBACK_USE}" "${REPLAY_REDUCED}" <<'PY'
import csv
import json
import math
import pathlib
import sys

generated_local = pathlib.Path(sys.argv[1])
mvp_root = pathlib.Path(sys.argv[2])
feedback_use_path = pathlib.Path(sys.argv[3])
reduced_path = pathlib.Path(sys.argv[4])
summary_path = mvp_root / "diagnostics" / "local_patch_mvp_summary.json"

if not generated_local.is_file():
    raise SystemExit(f"FAIL: missing generated local CSV {generated_local}")
if not summary_path.is_file():
    raise SystemExit(f"FAIL: missing summary {summary_path}")
if not feedback_use_path.is_file():
    raise SystemExit(f"FAIL: missing feedback_use CSV {feedback_use_path}")
if not reduced_path.is_file():
    raise SystemExit(f"FAIL: missing reduced data CSV {reduced_path}")

summary = json.loads(summary_path.read_text(encoding="utf-8"))
if summary.get("route") != "circle_macro_trace_local_patch_mvp:lagged_reduced":
    raise SystemExit(f"FAIL: unexpected route {summary.get('route')}")
if int(summary.get("gamma_not_one_rows", 0)) <= 0:
    raise SystemExit("FAIL: no gamma_n variation in summary")
if int(summary.get("fn_positive_rows", 0)) <= 0:
    raise SystemExit("FAIL: no fn_ref>0 rows in summary")

local_rows = list(csv.DictReader(generated_local.open(newline="")))
if not local_rows:
    raise SystemExit("FAIL: generated local CSV is empty")
gamma_shift_rows = 0
fn_positive_rows = 0
for row in local_rows:
    gamma_n = float(row["gamma_n"])
    delta_g_eff = float(row["delta_g_eff"])
    fn_ref = float(row["fn_ref"])
    p_max = float(row["p_max"])
    if not all(math.isfinite(v) for v in (gamma_n, delta_g_eff, fn_ref, p_max)):
        raise SystemExit("FAIL: generated local CSV has non-finite values")
    if fn_ref > 0.0:
        fn_positive_rows += 1
        if abs(gamma_n - 1.0) > 1.0e-12:
            gamma_shift_rows += 1
if gamma_shift_rows <= 0:
    raise SystemExit("FAIL: lagged local CSV has no gamma_n variation")
if fn_positive_rows <= 0:
    raise SystemExit("FAIL: lagged local CSV has no fn_ref>0 row")

feedback_rows = list(csv.DictReader(feedback_use_path.open(newline="")))
if not feedback_rows:
    raise SystemExit("FAIL: feedback_use CSV is empty")
external_rows = [row for row in feedback_rows if row["source_mode"] == "EXTERNAL"]
if not external_rows:
    raise SystemExit("FAIL: replay feedback has no EXTERNAL rows")
for row in external_rows:
    for key in ("mu_base", "mu_used", "gamma_n_used", "k_base", "k_used"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: non-finite feedback_use {key}")

reduced_rows = list(csv.DictReader(reduced_path.open(newline="")))
if not reduced_rows:
    raise SystemExit("FAIL: reduced data CSV is empty")
for row in reduced_rows:
    for key in ("mu_eff", "gamma_n", "delta_g_eff", "fn_ref", "p_max", "h_min"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: non-finite reduced data {key}")
external_reduced = [row for row in reduced_rows if row["source_mode"] == "EXTERNAL"]
if not external_reduced:
    raise SystemExit("FAIL: reduced data has no EXTERNAL rows")

metadata_files = sorted((mvp_root / "patch_metadata").glob("**/*.json"))
receiver_files = sorted((mvp_root / "patch_receiver").glob("**/*receiver_summary.json"))
if not metadata_files:
    raise SystemExit("FAIL: missing patch metadata artifacts")
if not receiver_files:
    raise SystemExit("FAIL: missing patch receiver summary artifacts")

print(
    "PASS contact_circle_lagged_local_patch_mvp "
    f"external_rows={len(external_rows)} gamma_shift_rows={gamma_shift_rows} "
    f"fn_positive_rows={fn_positive_rows} metadata_files={len(metadata_files)} "
    f"receiver_files={len(receiver_files)}"
)
PY

echo "PASS: contact circle lagged local patch MVP check"
echo "out_dir=${OUT_DIR}"
