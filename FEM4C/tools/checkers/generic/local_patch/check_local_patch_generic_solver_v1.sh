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
OUT_ROOT="${FEM4C_LOCAL_PATCH_GENERIC_SOLVER_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_local_patch_generic_solver_v1}"
PEN_DIR="${OUT_ROOT}/penetration"
FORCE_DIR="${OUT_ROOT}/force"

rm -rf "${OUT_ROOT}"
mkdir -p "${OUT_ROOT}"

python3 "${ROOT_DIR}/scripts/run_local_patch_generic_contract_v1.py" \
    "${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_penetration.json" \
    "${PEN_DIR}"
python3 "${ROOT_DIR}/scripts/run_local_patch_generic_contract_v1.py" \
    "${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_force.json" \
    "${FORCE_DIR}"

python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_response.py" "${PEN_DIR}/response.json"
python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_response.py" "${FORCE_DIR}/response.json"

python3 - "${PEN_DIR}" "${FORCE_DIR}" <<'PY'
import csv
import json
import math
import pathlib
import sys

dirs = [pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])]

for out_dir in dirs:
    response_path = out_dir / "response.json"
    pressure_path = out_dir / "pressure_field_grid.csv"
    grid_path = out_dir / "grid_definition.json"
    summary_json_path = out_dir / "local_patch_generic_summary.json"
    summary_md_path = out_dir / "local_patch_generic_summary.md"

    for path in (response_path, pressure_path, grid_path, summary_json_path, summary_md_path):
        if not path.is_file():
            raise SystemExit(f"FAIL: missing artifact {path}")

    response = json.loads(response_path.read_text(encoding="utf-8"))
    summary = json.loads(summary_json_path.read_text(encoding="utf-8"))
    grid = json.loads(grid_path.read_text(encoding="utf-8"))
    summary_md = summary_md_path.read_text(encoding="utf-8")

    if summary["solver_mode"] != "proxy_flat_plane_structured_grid_v1":
        raise SystemExit(f"FAIL: solver_mode mismatch in {summary_json_path}")
    if "proxy" not in summary["status"]:
        raise SystemExit(f"FAIL: status is not truthful proxy status in {summary_json_path}")
    if "not exact FEM surface-to-surface contact" not in summary_md:
        raise SystemExit(f"FAIL: summary markdown missing approximation note in {summary_md_path}")

    rows = list(csv.DictReader(pressure_path.open(newline="")))
    if not rows:
        raise SystemExit(f"FAIL: empty pressure field CSV {pressure_path}")

    total_force = 0.0
    p_max = 0.0
    active_rows = 0
    for idx, row in enumerate(rows, start=1):
        pressure = float(row["pressure_pa"])
        displacement = float(row["displacement_n_m"])
        area = float(row["cell_area_m2"])
        active_flag = int(row["active_flag"])
        values = [pressure, displacement, area]
        if not all(math.isfinite(v) for v in values):
            raise SystemExit(f"FAIL: non-finite pressure row {idx} in {pressure_path}")
        if pressure < -1.0e-12:
            raise SystemExit(f"FAIL: negative pressure row {idx} in {pressure_path}")
        total_force += pressure * area
        p_max = max(p_max, pressure)
        active_rows += active_flag

    if active_rows <= 0:
        raise SystemExit(f"FAIL: no active pressure cells in {pressure_path}")
    if abs(total_force - float(response["result"]["fn_ref_n"])) > max(1.0e-6, 1.0e-6 * max(1.0, total_force)):
        raise SystemExit(
            f"FAIL: force integral mismatch in {pressure_path}: "
            f"integral={total_force:.16e} response={float(response['result']['fn_ref_n']):.16e}"
        )
    if abs(p_max - float(response["result"]["p_max_pa"])) > max(1.0e-9, 1.0e-12 * max(1.0, p_max)):
        raise SystemExit(
            f"FAIL: p_max mismatch in {pressure_path}: "
            f"field={p_max:.16e} response={float(response['result']['p_max_pa']):.16e}"
        )
    if grid["grid_shape"]["nx"] <= 0 or grid["grid_shape"]["ny"] <= 0:
        raise SystemExit(f"FAIL: invalid grid shape in {grid_path}")

print("PASS local_patch_generic_solver_v1 penetration_and_force_ok")
PY

echo "PASS: local patch generic solver v1 check"
echo "out_dir=${OUT_ROOT}"
