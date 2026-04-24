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
OUT_DIR="${FEM4C_CONTACT_LAGGED_REDUCED_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_circle_lagged_reduced}"
CASE_NAME="circle_collision_oblique_lagged_reduced"
DECK_PATH="examples/contact/${CASE_NAME}.dat"
LOCAL_CONTACT_PATH="examples/contact/local_contact_circle_contract.csv"
EHL_PATH="examples/contact/ehl_circle_contract.csv"
SUMMARY_PATH="${OUT_DIR}/${CASE_NAME}.out"
LOG_PATH="${OUT_DIR}/${CASE_NAME}.log"
HISTORY_PATH="${SUMMARY_PATH}.history.csv"
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
        --mbd-steps=350 \
        "${DECK_PATH}" \
        "${SUMMARY_PATH}" \
        >"${LOG_PATH}" 2>&1
)

if [[ ! -f "${LOG_PATH}" ]]; then
    echo "FAIL: missing log file: ${LOG_PATH}" >&2
    exit 1
fi
if [[ ! -f "${HISTORY_PATH}" ]]; then
    echo "FAIL: missing history file: ${HISTORY_PATH}" >&2
    exit 1
fi
if [[ ! -f "${TRACE_PATH}" ]]; then
    echo "FAIL: missing contact trace file: ${TRACE_PATH}" >&2
    exit 1
fi
if [[ ! -f "${FEEDBACK_USE_PATH}" ]]; then
    echo "FAIL: missing contact feedback-use file: ${FEEDBACK_USE_PATH}" >&2
    exit 1
fi
if [[ ! -f "${REDUCED_DATA_PATH}" ]]; then
    echo "FAIL: missing contact reduced-data file: ${REDUCED_DATA_PATH}" >&2
    exit 1
fi
if [[ ! -f "${ROOT_DIR}/${LOCAL_CONTACT_PATH}" ]]; then
    echo "FAIL: missing local contact contract file: ${ROOT_DIR}/${LOCAL_CONTACT_PATH}" >&2
    exit 1
fi
if [[ ! -f "${ROOT_DIR}/${EHL_PATH}" ]]; then
    echo "FAIL: missing EHL contract file: ${ROOT_DIR}/${EHL_PATH}" >&2
    exit 1
fi
grep -q "MBD_LOCAL_CONTACT_FILE ${LOCAL_CONTACT_PATH}" "${ROOT_DIR}/${DECK_PATH}"
grep -q "MBD_EHL_FILE ${EHL_PATH}" "${ROOT_DIR}/${DECK_PATH}"
if grep -q "MBD_LOCAL_FEEDBACK_FILE" "${ROOT_DIR}/${DECK_PATH}"; then
    echo "FAIL: circle lagged reduced deck still uses legacy MBD_LOCAL_FEEDBACK_FILE" >&2
    exit 1
fi

grep -q "Program completed successfully." "${LOG_PATH}"

python3 - "${HISTORY_PATH}" "${TRACE_PATH}" "${FEEDBACK_USE_PATH}" "${REDUCED_DATA_PATH}" "${ROOT_DIR}/${LOCAL_CONTACT_PATH}" "${ROOT_DIR}/${EHL_PATH}" <<'PY'
import csv
import math
import pathlib
import sys

history_path = pathlib.Path(sys.argv[1])
trace_path = pathlib.Path(sys.argv[2])
feedback_use_path = pathlib.Path(sys.argv[3])
reduced_data_path = pathlib.Path(sys.argv[4])
local_contact_path = pathlib.Path(sys.argv[5])
ehl_path = pathlib.Path(sys.argv[6])

history_rows = {}
with history_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    if reader.fieldnames is None:
        raise SystemExit(f"FAIL: {history_path} has no header")
    for idx, row in enumerate(reader, start=1):
        step = int(round(float(row["step"])))
        body_id = int(round(float(row["body_id"])))
        x = float(row["x"])
        y = float(row["y"])
        vx = float(row["vx"])
        vy = float(row["vy"])
        omega = float(row["omega"])
        for value_name, value in (("x", x), ("y", y), ("vx", vx), ("vy", vy), ("omega", omega)):
            if not math.isfinite(value):
                raise SystemExit(f"FAIL: history row {idx} has non-finite {value_name}")
        history_rows[(step, body_id)] = (x, y, vx, vy, omega)

trace_rows = list(csv.DictReader(trace_path.open(newline="")))
if not trace_rows:
    raise SystemExit(f"FAIL: {trace_path} has no data rows")

active_rows = 0
nonzero_ft_rows = 0
max_abs_vt = 0.0
max_abs_ft = 0.0

for idx, row in enumerate(trace_rows, start=1):
    step = int(round(float(row["step"])))
    active = int(round(float(row["active"])))
    nx = float(row["nx"])
    ny = float(row["ny"])
    cp1_x = float(row["cp1_x"])
    cp1_y = float(row["cp1_y"])
    cp2_x = float(row["cp2_x"])
    cp2_y = float(row["cp2_y"])
    f2_x = float(row["f2_x"])
    f2_y = float(row["f2_y"])
    for value_name, value in (
        ("nx", nx), ("ny", ny), ("cp1_x", cp1_x), ("cp1_y", cp1_y),
        ("cp2_x", cp2_x), ("cp2_y", cp2_y), ("f2_x", f2_x), ("f2_y", f2_y),
    ):
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: trace row {idx} has non-finite {value_name}")

    state1 = history_rows.get((step, 0))
    state2 = history_rows.get((step, 1))
    if state1 is None or state2 is None:
        raise SystemExit(f"FAIL: missing history state for trace step {step}")

    x1, y1, vx1, vy1, omega1 = state1
    x2, y2, vx2, vy2, omega2 = state2
    r1x = cp1_x - x1
    r1y = cp1_y - y1
    r2x = cp2_x - x2
    r2y = cp2_y - y2
    vp1x = vx1 - omega1 * r1y
    vp1y = vy1 + omega1 * r1x
    vp2x = vx2 - omega2 * r2y
    vp2y = vy2 + omega2 * r2x
    tx = -ny
    ty = nx
    vt = (vp2x - vp1x) * tx + (vp2y - vp1y) * ty
    ft = f2_x * tx + f2_y * ty

    if not math.isfinite(vt):
        raise SystemExit(f"FAIL: trace row {idx} reconstructed vt is not finite")
    if not math.isfinite(ft):
        raise SystemExit(f"FAIL: trace row {idx} reconstructed ft is not finite")

    max_abs_vt = max(max_abs_vt, abs(vt))
    max_abs_ft = max(max_abs_ft, abs(ft))
    if active == 1:
        active_rows += 1
        if abs(ft) > 1.0e-12:
            nonzero_ft_rows += 1

if active_rows == 0:
    raise SystemExit("FAIL: lagged reduced case has no active rows")
if nonzero_ft_rows == 0:
    raise SystemExit("FAIL: lagged reduced case has no active rows with |ft|>0")

rows = list(csv.DictReader(feedback_use_path.open(newline="")))
if not rows:
    raise SystemExit(f"FAIL: {feedback_use_path} has no data rows")

reduced_rows = list(csv.DictReader(reduced_data_path.open(newline="")))
if not reduced_rows:
    raise SystemExit(f"FAIL: {reduced_data_path} has no data rows")

with local_contact_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    header = reader.fieldnames
    if header is None:
        raise SystemExit(f"FAIL: {local_contact_path} has no header")
    expected_prefix = ["step", "pair_id", "gamma_n", "delta_g_eff", "fn_ref", "p_max", "valid_flag", "status"]
    if header[:8] != expected_prefix:
        raise SystemExit(f"FAIL: {local_contact_path} header prefix mismatch: {header!r}")

with ehl_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    header = reader.fieldnames
    if header is None:
        raise SystemExit(f"FAIL: {ehl_path} has no header")
    expected_prefix = ["step", "pair_id", "mu_eff", "h_min", "regime_flag", "valid_flag", "status"]
    if header[:7] != expected_prefix:
        raise SystemExit(f"FAIL: {ehl_path} header prefix mismatch: {header!r}")

fallback_row = None
external_row_184 = None
external_rows = 0
reduced_fallback_row = None
reduced_external_row_184 = None

for idx, row in enumerate(rows, start=1):
    pair_id = int(round(float(row["pair_id"])))
    step = int(round(float(row["step"])))
    if pair_id != 0:
        continue
    for key in ("mu_base", "mu_used", "gamma_n_used", "k_base", "k_used"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: feedback-use row {idx} column {key} is not finite")
    if step == 0:
        fallback_row = row
    if row["source_mode"] == "EXTERNAL":
        external_rows += 1
    if step == 184:
        external_row_184 = row

for idx, row in enumerate(reduced_rows, start=1):
    pair_id = int(round(float(row["pair_id"])))
    step = int(round(float(row["step"])))
    record_step = int(round(float(row["record_step"])))
    record_iter = int(round(float(row["record_iter"])))
    status_ok = int(round(float(row["status_ok"])))
    regime_flag = int(round(float(row["regime_flag"])))
    valid_flag = int(round(float(row["valid_flag"])))
    for key in ("time", "mu_eff", "gamma_n", "delta_g_eff", "fn_ref", "p_max", "h_min"):
        value = float(row[key])
        if not math.isfinite(value):
            raise SystemExit(f"FAIL: reduced-data row {idx} column {key} is not finite")
    if pair_id != 0:
        continue
    if row["fallback_reason"] == "":
        raise SystemExit(f"FAIL: reduced-data row {idx} has empty fallback_reason")
    if row["status"] == "":
        raise SystemExit(f"FAIL: reduced-data row {idx} has empty status")
    if record_step < -1:
        raise SystemExit(f"FAIL: reduced-data row {idx} has invalid record_step={record_step}")
    if record_iter < -1:
        raise SystemExit(f"FAIL: reduced-data row {idx} has invalid record_iter={record_iter}")
    if status_ok not in (0, 1):
        raise SystemExit(f"FAIL: reduced-data row {idx} has invalid status_ok={status_ok}")
    if regime_flag < 0 or valid_flag < 0:
        raise SystemExit(f"FAIL: reduced-data row {idx} has invalid flags")
    if step == 0:
        reduced_fallback_row = row
    if step == 184:
        reduced_external_row_184 = row

if fallback_row is None:
    raise SystemExit("FAIL: missing step 0 fallback row")
if fallback_row["source_mode"] != "FALLBACK":
    raise SystemExit(f"FAIL: step 0 source_mode={fallback_row['source_mode']!r}")
if fallback_row["fallback_reason"] != "step0_no_prev":
    raise SystemExit(f"FAIL: step 0 fallback_reason={fallback_row['fallback_reason']!r}")
if external_row_184 is None:
    raise SystemExit("FAIL: missing step 184 external row")
if external_row_184["source_mode"] != "EXTERNAL":
    raise SystemExit(f"FAIL: step 184 source_mode={external_row_184['source_mode']!r}")
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
if reduced_external_row_184 is None:
    raise SystemExit("FAIL: reduced-data CSV is missing step 184 row")
if reduced_external_row_184["source_mode"] != "EXTERNAL":
    raise SystemExit(
        f"FAIL: reduced-data step 184 source_mode={reduced_external_row_184['source_mode']!r}"
    )
if int(round(float(reduced_external_row_184["record_step"]))) != 183:
    raise SystemExit(
        f"FAIL: reduced-data step 184 record_step={reduced_external_row_184['record_step']!r}"
    )
if int(round(float(reduced_external_row_184["status_ok"]))) != 1:
    raise SystemExit(
        f"FAIL: reduced-data step 184 status_ok={reduced_external_row_184['status_ok']!r}"
    )

mu_base = float(external_row_184["mu_base"])
mu_used = float(external_row_184["mu_used"])
gamma_n_used = float(external_row_184["gamma_n_used"])
k_base = float(external_row_184["k_base"])
k_used = float(external_row_184["k_used"])

if abs(mu_used - mu_base) <= 1.0e-12:
    raise SystemExit("FAIL: external mu_used does not differ from mu_base")
if abs(k_used - k_base) <= 1.0e-12:
    raise SystemExit("FAIL: external k_used does not differ from k_base")
if abs(mu_used - 0.20) > 1.0e-12:
    raise SystemExit(f"FAIL: step 184 mu_used={mu_used!r}, expected 0.20")
if abs(gamma_n_used - 0.50) > 1.0e-12:
    raise SystemExit(f"FAIL: step 184 gamma_n_used={gamma_n_used!r}, expected 0.50")
if abs(k_used - 2.5e4) > 1.0e-6:
    raise SystemExit(f"FAIL: step 184 k_used={k_used!r}, expected 2.5e4")
if external_rows == 0:
    raise SystemExit("FAIL: no external feedback rows found")

print(
    "PASS lagged_reduced "
    f"active_rows={active_rows} external_rows={external_rows} "
    f"mu_base={mu_base:.6e} mu_used={mu_used:.6e} "
    f"gamma_n_used={gamma_n_used:.6e} "
    f"k_base={k_base:.6e} k_used={k_used:.6e} "
    f"max_abs_vt={max_abs_vt:.6e} max_abs_ft={max_abs_ft:.6e}"
)
PY

echo "PASS: contact circle lagged reduced check"
echo "output_dir=${OUT_DIR}"
