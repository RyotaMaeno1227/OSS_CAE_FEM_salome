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
OUT_ROOT="${FEM4C_LOCAL_PATCH_GENERIC_VISUALIZATION_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_local_patch_generic_visualization_v1}"
PEN_DIR="${OUT_ROOT}/penetration"
FORCE_DIR="${OUT_ROOT}/normal_force"
GOLDEN_PEN_DIR="${OUT_ROOT}/golden_penetration"
GOLDEN_FORCE_DIR="${OUT_ROOT}/golden_normal_force"

rm -rf "${OUT_ROOT}"
mkdir -p "${OUT_ROOT}"

python3 "${ROOT_DIR}/scripts/run_local_patch_generic_contract_v1.py" \
    "${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_penetration.json" \
    "${PEN_DIR}"
python3 "${ROOT_DIR}/scripts/run_local_patch_generic_contract_v1.py" \
    "${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_force.json" \
    "${FORCE_DIR}"

python3 "${ROOT_DIR}/scripts/plot_local_patch_generic_solver_v1.py" "${PEN_DIR}"
python3 "${ROOT_DIR}/scripts/plot_local_patch_generic_solver_v1.py" "${FORCE_DIR}"

bash "${ROOT_DIR}/scripts/export_local_patch_generic_golden_example.sh" penetration "${GOLDEN_PEN_DIR}"
bash "${ROOT_DIR}/scripts/export_local_patch_generic_golden_example.sh" normal_force "${GOLDEN_FORCE_DIR}"

bash "${ROOT_DIR}/scripts/check_local_patch_generic_contract_v1.sh"
bash "${ROOT_DIR}/scripts/check_local_patch_generic_solver_v1.sh"

python3 - "${PEN_DIR}" "${FORCE_DIR}" "${GOLDEN_PEN_DIR}" "${GOLDEN_FORCE_DIR}" <<'PY'
import pathlib
import sys

dirs = [pathlib.Path(arg) for arg in sys.argv[1:]]
required_pngs = {
    "pressure_field_heatmap.png",
    "pressure_field_centerline_t1.png",
    "reduced_summary.png",
}

for out_dir in dirs:
    pngs = {path.name for path in out_dir.glob("*.png")}
    if not required_pngs.issubset(pngs):
        raise SystemExit(f"FAIL: missing PNGs in {out_dir}: expected {sorted(required_pngs)} got {sorted(pngs)}")
    if len(pngs) < 3:
        raise SystemExit(f"FAIL: expected at least 3 PNGs in {out_dir}")
    for required in [
        "response.json",
        "pressure_field_grid.csv",
        "grid_definition.json",
        "local_patch_generic_summary.json",
        "local_patch_generic_summary.md",
    ]:
        if not (out_dir / required).is_file():
            raise SystemExit(f"FAIL: missing artifact {(out_dir / required)}")

for golden_dir in (dirs[2], dirs[3]):
    if not (golden_dir / "README.md").is_file():
        raise SystemExit(f"FAIL: missing README in {golden_dir}")
    text = (golden_dir / "README.md").read_text(encoding="utf-8")
    if "proxy / MVP" not in text:
        raise SystemExit(f"FAIL: README missing proxy truthfulness in {golden_dir}")

print("PASS local_patch_generic_visualization_v1 png_and_golden_ok")
PY

echo "PASS: local patch generic visualization v1 check"
echo "out_dir=${OUT_ROOT}"
