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
OUT_DIR="${FEM4C_CONTACT_LAGGED_STIFFNESS_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_circle_lagged_stiffness}"
CASE_NAME="circle_collision_explicit_lagged"
DECK_PATH="${ROOT_DIR}/examples/contact/${CASE_NAME}.dat"
SUMMARY_PATH="${OUT_DIR}/${CASE_NAME}.out"
LOG_PATH="${OUT_DIR}/${CASE_NAME}.log"
TRACE_PATH="${SUMMARY_PATH}.contact_trace.csv"
FEEDBACK_PATH="${SUMMARY_PATH}.contact_feedback.csv"

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

if [[ ! -f "${LOG_PATH}" ]]; then
    echo "FAIL: missing log file: ${LOG_PATH}" >&2
    exit 1
fi
if [[ ! -f "${TRACE_PATH}" ]]; then
    echo "FAIL: missing contact trace file: ${TRACE_PATH}" >&2
    exit 1
fi
if [[ ! -f "${FEEDBACK_PATH}" ]]; then
    echo "FAIL: missing contact feedback file: ${FEEDBACK_PATH}" >&2
    exit 1
fi

grep -q "Program completed successfully." "${LOG_PATH}"

python3 - "${TRACE_PATH}" "${FEEDBACK_PATH}" <<'PY'
import csv
import math
import pathlib
import sys

trace_path = pathlib.Path(sys.argv[1])
feedback_path = pathlib.Path(sys.argv[2])

trace_expected = [
    "step", "time", "pair_id", "active", "gap", "penetration",
    "nx", "ny", "vn", "fn",
    "cp1_x", "cp1_y", "cp2_x", "cp2_y",
    "f1_x", "f1_y", "m1_z", "f2_x", "f2_y", "m2_z",
]
feedback_expected = [
    "step", "time", "pair_id", "active", "gap", "penetration",
    "fn", "kn_base", "kn_used", "kn_out", "coupling_mode",
]

def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    sys.exit(1)

with trace_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    if reader.fieldnames != trace_expected:
        fail(f"{trace_path} header mismatch: {reader.fieldnames!r}")
    trace_rows = 0
    active_rows = 0
    for row in reader:
        trace_rows += 1
        for key in trace_expected:
            try:
                value = float(row[key])
            except ValueError as exc:
                fail(f"{trace_path} column '{key}' row {trace_rows} is not numeric: {row[key]!r} ({exc})")
            if not math.isfinite(value):
                fail(f"{trace_path} column '{key}' row {trace_rows} is not finite: {row[key]!r}")
        if int(round(float(row["active"]))) == 1:
            active_rows += 1
    if trace_rows == 0:
        fail(f"{trace_path} has no data rows")
    if active_rows == 0:
        fail(f"{trace_path} has no active=1 rows")

with feedback_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    if reader.fieldnames != feedback_expected:
        fail(f"{feedback_path} header mismatch: {reader.fieldnames!r}")

    rows = list(reader)
    if not rows:
        fail(f"{feedback_path} has no data rows")

    first = rows[0]
    if int(round(float(first["step"]))) != 0:
        fail("feedback first row is not step 0")
    if first["coupling_mode"] != "LAGGED_STIFFNESS":
        fail(f"unexpected coupling_mode in first row: {first['coupling_mode']!r}")

    try:
        first_kn_base = float(first["kn_base"])
        first_kn_used = float(first["kn_used"])
    except ValueError as exc:
        fail(f"invalid first-row stiffness value: {exc}")
    if not math.isfinite(first_kn_base) or not math.isfinite(first_kn_used):
        fail("first-row stiffness values are not finite")
    if abs(first_kn_used - first_kn_base) > 1.0e-12:
        fail("step 0 does not use base_kn fallback")

    positive_kn_out_rows = 0
    post_step0_rows = 0
    post_step0_lagged_rows = 0
    changed_kn_used_rows = 0

    for idx, row in enumerate(rows, start=1):
        if row["coupling_mode"] != "LAGGED_STIFFNESS":
            fail(f"row {idx} has unexpected coupling_mode={row['coupling_mode']!r}")
        try:
            step = int(round(float(row["step"])))
            fn = float(row["fn"])
            kn_base = float(row["kn_base"])
            kn_used = float(row["kn_used"])
            kn_out = float(row["kn_out"])
        except ValueError as exc:
            fail(f"invalid feedback value at row {idx}: {exc}")
        for key, value in (("fn", fn), ("kn_base", kn_base), ("kn_used", kn_used), ("kn_out", kn_out)):
            if not math.isfinite(value):
                fail(f"{feedback_path} column '{key}' row {idx} is not finite")
        if step > 0:
            post_step0_rows += 1
            if math.isfinite(kn_used):
                post_step0_lagged_rows += 1
        if kn_out > 0.0:
            positive_kn_out_rows += 1
        if abs(kn_used - kn_base) > 1.0e-12:
            changed_kn_used_rows += 1
        if fn < -1.0e-12:
            fail(f"negative fn at row {idx}")

    if post_step0_rows == 0:
        fail("feedback file has no rows after step 0")
    if post_step0_lagged_rows == 0:
        fail("feedback file has no finite kn_used rows after step 0")
    if positive_kn_out_rows == 0:
        fail("feedback file never produced positive kn_out")

print(
    "PASS lagged_stiffness "
    f"trace_rows={trace_rows} active_rows={active_rows} "
    f"feedback_rows={len(rows)} positive_kn_out_rows={positive_kn_out_rows} "
    f"changed_kn_used_rows={changed_kn_used_rows}"
)
PY

echo "PASS: contact circle lagged stiffness check"
echo "output_dir=${OUT_DIR}"
