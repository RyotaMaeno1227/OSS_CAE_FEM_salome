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
OUT_DIR="${FEM4C_GEAR_PIN_PROXY_LAGGED_REDUCED_OUTDIR:-${TMPDIR:-/tmp}/fem4c_gear_pin_proxy_lagged_reduced}"
CASE_NAME="pin_tooth_flank_proxy_lagged_reduced"
DECK_PATH="examples/gear_pin_proxy/${CASE_NAME}.dat"
LOCAL_CONTACT_PATH="examples/gear_pin_proxy/local_contact_gear_pin_contract.csv"
EHL_PATH="examples/gear_pin_proxy/ehl_gear_pin_contract.csv"
SUMMARY_PATH="${OUT_DIR}/${CASE_NAME}.out"
LOG_PATH="${OUT_DIR}/${CASE_NAME}.log"
TRACE_PATH="${SUMMARY_PATH}.contact_trace.csv"
FEEDBACK_USE_PATH="${SUMMARY_PATH}.contact_feedback_use.csv"
REDUCED_DATA_PATH="${SUMMARY_PATH}.contact_reduced_data.csv"

mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j

(
    cd "${ROOT_DIR}"
    ASAN_OPTIONS=detect_leaks=0 \
        "${ROOT_DIR}/bin/fem4c" \
        --mode=mbd \
        --mbd-integrator=explicit \
        --mbd-dt=1.0e-3 \
        --mbd-steps=180 \
        "${DECK_PATH}" \
        "${SUMMARY_PATH}" \
        >"${LOG_PATH}" 2>&1
)

[[ -f "${LOG_PATH}" ]] || { echo "FAIL: missing log file: ${LOG_PATH}" >&2; exit 1; }
[[ -f "${TRACE_PATH}" ]] || { echo "FAIL: missing contact trace file: ${TRACE_PATH}" >&2; exit 1; }
[[ -f "${FEEDBACK_USE_PATH}" ]] || { echo "FAIL: missing contact feedback-use file: ${FEEDBACK_USE_PATH}" >&2; exit 1; }
[[ -f "${REDUCED_DATA_PATH}" ]] || { echo "FAIL: missing contact reduced-data file: ${REDUCED_DATA_PATH}" >&2; exit 1; }
[[ -f "${ROOT_DIR}/${LOCAL_CONTACT_PATH}" ]] || { echo "FAIL: missing local contact contract file: ${ROOT_DIR}/${LOCAL_CONTACT_PATH}" >&2; exit 1; }
[[ -f "${ROOT_DIR}/${EHL_PATH}" ]] || { echo "FAIL: missing EHL contract file: ${ROOT_DIR}/${EHL_PATH}" >&2; exit 1; }
grep -q "MBD_LOCAL_CONTACT_FILE ${LOCAL_CONTACT_PATH}" "${ROOT_DIR}/${DECK_PATH}"
grep -q "MBD_EHL_FILE ${EHL_PATH}" "${ROOT_DIR}/${DECK_PATH}"
if grep -q "MBD_LOCAL_FEEDBACK_FILE" "${ROOT_DIR}/${DECK_PATH}"; then
    echo "FAIL: gear-pin lagged reduced deck still uses legacy MBD_LOCAL_FEEDBACK_FILE" >&2
    exit 1
fi

grep -q "Program completed successfully." "${LOG_PATH}"

python3 - "${TRACE_PATH}" "${FEEDBACK_USE_PATH}" "${REDUCED_DATA_PATH}" "${ROOT_DIR}/${LOCAL_CONTACT_PATH}" "${ROOT_DIR}/${EHL_PATH}" <<'PY'
import csv
import math
import pathlib
import sys

trace_path = pathlib.Path(sys.argv[1])
feedback_use_path = pathlib.Path(sys.argv[2])
reduced_data_path = pathlib.Path(sys.argv[3])
local_contact_path = pathlib.Path(sys.argv[4])
ehl_path = pathlib.Path(sys.argv[5])

trace_rows = list(csv.DictReader(trace_path.open(newline="")))
if not trace_rows:
    raise SystemExit(f"FAIL: {trace_path} has no data rows")

active_rows = 0
for idx, row in enumerate(trace_rows, start=1):
    for key, value in row.items():
        if key in ("pair_id", "active", "step"):
            continue
        numeric = float(value)
        if not math.isfinite(numeric):
            raise SystemExit(f"FAIL: {trace_path} row {idx} has non-finite {key}")
    if int(round(float(row["active"]))) == 1:
        active_rows += 1

if active_rows == 0:
    raise SystemExit("FAIL: gear-pin lagged reduced case has no active rows")

with local_contact_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    header = reader.fieldnames
    expected_prefix = ["step", "pair_id", "gamma_n", "delta_g_eff", "fn_ref", "p_max", "valid_flag", "status"]
    if header is None or header[:8] != expected_prefix:
        raise SystemExit(f"FAIL: {local_contact_path} header prefix mismatch: {header!r}")

with ehl_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    header = reader.fieldnames
    expected_prefix = ["step", "pair_id", "mu_eff", "h_min", "regime_flag", "valid_flag", "status"]
    if header is None or header[:7] != expected_prefix:
        raise SystemExit(f"FAIL: {ehl_path} header prefix mismatch: {header!r}")

rows = list(csv.DictReader(feedback_use_path.open(newline="")))
if not rows:
    raise SystemExit(f"FAIL: {feedback_use_path} has no data rows")

reduced_rows = list(csv.DictReader(reduced_data_path.open(newline="")))
if not reduced_rows:
    raise SystemExit(f"FAIL: {reduced_data_path} has no data rows")

fallback_row = None
external_rows = []
reduced_fallback_row = None
reduced_external_rows = []
for idx, row in enumerate(rows, start=1):
    for key in ("mu_base", "mu_used", "gamma_n_used", "k_base", "k_used"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: {feedback_use_path} row {idx} has non-finite {key}")
    if int(round(float(row["step"]))) == 0:
        fallback_row = row
    if row["source_mode"] == "EXTERNAL":
        external_rows.append(row)

for idx, row in enumerate(reduced_rows, start=1):
    step = int(round(float(row["step"])))
    record_step = int(round(float(row["record_step"])))
    record_iter = int(round(float(row["record_iter"])))
    status_ok = int(round(float(row["status_ok"])))
    regime_flag = int(round(float(row["regime_flag"])))
    valid_flag = int(round(float(row["valid_flag"])))
    for key in ("time", "mu_eff", "gamma_n", "delta_g_eff", "fn_ref", "p_max", "h_min"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has non-finite {key}")
    if row["fallback_reason"] == "":
        raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has empty fallback_reason")
    if row["status"] == "":
        raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has empty status")
    if record_step < -1 or record_iter < -1:
        raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has invalid record metadata")
    if status_ok not in (0, 1):
        raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has invalid status_ok={status_ok}")
    if regime_flag < 0 or valid_flag < 0:
        raise SystemExit(f"FAIL: {reduced_data_path} row {idx} has invalid flags")
    if step == 0:
        reduced_fallback_row = row
    if row["source_mode"] == "EXTERNAL":
        reduced_external_rows.append(row)

if fallback_row is None:
    raise SystemExit("FAIL: missing step 0 fallback row")
if fallback_row["source_mode"] != "FALLBACK":
    raise SystemExit(f"FAIL: step 0 source_mode={fallback_row['source_mode']!r}")
if fallback_row["fallback_reason"] != "step0_no_prev":
    raise SystemExit(f"FAIL: step 0 fallback_reason={fallback_row['fallback_reason']!r}")
if not external_rows:
    raise SystemExit("FAIL: no EXTERNAL feedback rows found")
if reduced_fallback_row is None:
    raise SystemExit("FAIL: reduced-data CSV is missing step 0 row")
if reduced_fallback_row["source_mode"] != "FALLBACK":
    raise SystemExit(f"FAIL: reduced-data step 0 source_mode={reduced_fallback_row['source_mode']!r}")
if reduced_fallback_row["fallback_reason"] != "step0_no_prev":
    raise SystemExit(
        f"FAIL: reduced-data step 0 fallback_reason={reduced_fallback_row['fallback_reason']!r}"
    )
if int(round(float(reduced_fallback_row["record_step"]))) != -1:
    raise SystemExit(
        f"FAIL: reduced-data step 0 record_step={reduced_fallback_row['record_step']!r}"
    )
if not reduced_external_rows:
    raise SystemExit("FAIL: reduced-data CSV has no EXTERNAL rows")

active_external_rows = []
for row in external_rows:
    step = int(round(float(row["step"])))
    matching_trace = next((tr for tr in trace_rows if int(round(float(tr["step"]))) == step), None)
    if matching_trace is not None and int(round(float(matching_trace["active"]))) == 1:
        active_external_rows.append((row, matching_trace))

if not active_external_rows:
    raise SystemExit("FAIL: no EXTERNAL feedback row aligns with an active contact row")

for row in reduced_external_rows:
    if int(round(float(row["record_step"]))) < 0:
        raise SystemExit("FAIL: reduced-data EXTERNAL row is missing record_step")
    if int(round(float(row["status_ok"]))) != 1:
        raise SystemExit("FAIL: reduced-data EXTERNAL row has status_ok != 1")

row, _ = active_external_rows[0]
mu_base = float(row["mu_base"])
mu_used = float(row["mu_used"])
gamma_used = float(row["gamma_n_used"])
k_base = float(row["k_base"])
k_used = float(row["k_used"])

if abs(mu_used - mu_base) <= 1.0e-12:
    raise SystemExit("FAIL: external mu_used does not differ from mu_base")
if abs(k_used - k_base) <= 1.0e-12:
    raise SystemExit("FAIL: external k_used does not differ from k_base")

print(
    "PASS gear_pin_lagged_reduced "
    f"active_rows={active_rows} external_rows={len(external_rows)} "
    f"mu_base={mu_base:.6e} mu_used={mu_used:.6e} "
    f"gamma_n_used={gamma_used:.6e} "
    f"k_base={k_base:.6e} k_used={k_used:.6e}"
)
PY

echo "PASS: gear pin proxy lagged reduced feedback check"
echo "output_dir=${OUT_DIR}"
