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
OUT_DIR="${FEM4C_MACRO_PATCH_BRIDGE_OUTDIR:-${TMPDIR:-/tmp}/fem4c_macro_to_patch_bridge}"

bash "${ROOT_DIR}/scripts/run_macro_to_patch_bridge.sh"

python3 - "${OUT_DIR}" <<'PY'
import csv
import json
import math
import os
import pathlib
import sys

out_dir = pathlib.Path(sys.argv[1])
trace_path = out_dir / "circle_collision_explicit.out.contact_trace.csv"
selected_step_path = out_dir / "selected_step.txt"
selected_fn_path = out_dir / "selected_fn.txt"
selected_row_json_path = out_dir / "selected_row.json"
requested_step_text = os.environ.get("FEM4C_MACRO_PATCH_BRIDGE_STEP", "").strip()

if not trace_path.exists():
    raise SystemExit(f"FAIL: missing trace {trace_path}")
if not selected_step_path.exists():
    raise SystemExit(f"FAIL: missing selected step file {selected_step_path}")
if not selected_fn_path.exists():
    raise SystemExit(f"FAIL: missing selected fn file {selected_fn_path}")
if not selected_row_json_path.exists():
    raise SystemExit(f"FAIL: missing selected row json {selected_row_json_path}")

selected_step = int(selected_step_path.read_text().strip())
selected_fn = float(selected_fn_path.read_text().strip())
selected_row_json = json.loads(selected_row_json_path.read_text())
requested_step = int(requested_step_text) if requested_step_text else None

with trace_path.open(newline="") as handle:
    reader = csv.DictReader(handle)
    best_active = None
    selected_row = None
    for row in reader:
        active = int(float(row["active"]))
        if active == 1:
            if best_active is None or float(row["fn"]) > float(best_active["fn"]):
                best_active = row
        if int(float(row["step"])) == selected_step:
            selected_row = row
if best_active is None:
    raise SystemExit(f"FAIL: no active row in {trace_path}")
if selected_row is None:
    raise SystemExit(f"FAIL: selected step {selected_step} not found in {trace_path}")
if requested_step is not None and selected_step != requested_step:
    raise SystemExit(
        f"FAIL: selected step mismatch, requested {requested_step} but run used {selected_step}"
    )
if requested_step is None and selected_step != int(float(best_active["step"])):
    raise SystemExit(
        f"FAIL: default bridge should use max-fn active step {int(float(best_active['step']))}, "
        f"but run used {selected_step}"
    )

metadata_paths = [
    out_dir / "patch_metadata" / f"patch_pair0_body0_step{selected_step:04d}.json",
    out_dir / "patch_metadata" / f"patch_pair0_body1_step{selected_step:04d}.json",
]
summary_paths = [
    out_dir / "patch_receiver" / f"patch_pair0_body0_step{selected_step:04d}_receiver_summary.json",
    out_dir / "patch_receiver" / f"patch_pair0_body1_step{selected_step:04d}_receiver_summary.json",
]

expected_step = int(float(selected_row["step"]))
expected_gap = float(selected_row["gap"])
expected_fn = float(selected_row["fn"])
if not math.isfinite(selected_fn):
    raise SystemExit(f"FAIL: selected fn is not finite: {selected_fn!r}")
if abs(selected_fn - expected_fn) > 1.0e-9:
    raise SystemExit(
        f"FAIL: selected fn mismatch, selected_fn.txt has {selected_fn:.16e} "
        f"but trace row has {expected_fn:.16e}"
    )

required_selected_row_keys = (
    "selection_policy",
    "step",
    "time",
    "gap",
    "fn",
    "pair_id",
    "active",
    "receiver_small_contact_point_local",
    "receiver_large_contact_point_local",
    "metadata_small_path",
    "metadata_large_path",
    "receiver_small_summary_path",
    "receiver_large_summary_path",
)
for key in required_selected_row_keys:
    if key not in selected_row_json:
        raise SystemExit(f"FAIL: missing {key} in {selected_row_json_path}")

expected_policy = "step_override" if requested_step is not None else "max_fn_active"
if selected_row_json["selection_policy"] != expected_policy:
    raise SystemExit(
        f"FAIL: selected row policy mismatch in {selected_row_json_path}: "
        f"{selected_row_json['selection_policy']!r} != {expected_policy!r}"
    )
if int(selected_row_json["step"]) != expected_step:
    raise SystemExit(f"FAIL: selected row step mismatch in {selected_row_json_path}")
if abs(float(selected_row_json["time"]) - float(selected_row["time"])) > 1.0e-12:
    raise SystemExit(f"FAIL: selected row time mismatch in {selected_row_json_path}")
if abs(float(selected_row_json["gap"]) - expected_gap) > 1.0e-12:
    raise SystemExit(f"FAIL: selected row gap mismatch in {selected_row_json_path}")
if abs(float(selected_row_json["fn"]) - expected_fn) > 1.0e-9:
    raise SystemExit(f"FAIL: selected row fn mismatch in {selected_row_json_path}")
if int(selected_row_json["pair_id"]) != int(float(selected_row["pair_id"])):
    raise SystemExit(f"FAIL: selected row pair_id mismatch in {selected_row_json_path}")
if int(selected_row_json["active"]) != int(float(selected_row["active"])):
    raise SystemExit(f"FAIL: selected row active mismatch in {selected_row_json_path}")

expected_receiver_contact_points = (
    [2.0e-2, 5.0e-3],
    [3.2e-2, 8.0e-3],
)
for key, expected in (
    ("receiver_small_contact_point_local", expected_receiver_contact_points[0]),
    ("receiver_large_contact_point_local", expected_receiver_contact_points[1]),
):
    point = selected_row_json[key]
    if len(point) != 2:
        raise SystemExit(f"FAIL: {key} length mismatch in {selected_row_json_path}")
    if any(not math.isfinite(float(v)) for v in point):
        raise SystemExit(f"FAIL: non-finite {key} in {selected_row_json_path}")
    if any(abs(float(v) - expected_i) > 1.0e-12 for v, expected_i in zip(point, expected)):
        raise SystemExit(
            f"FAIL: {key} mismatch in {selected_row_json_path}: "
            f"{point!r} != {expected!r}"
        )

for path in metadata_paths:
    if not path.exists():
        raise SystemExit(f"FAIL: missing metadata {path}")
    data = json.loads(path.read_text())
    for key in ("radius_body", "gap_macro", "fn_macro", "mesh_path", "output_path"):
        if key not in data:
            raise SystemExit(f"FAIL: missing {key} in {path}")
    if int(data["step"]) != expected_step:
        raise SystemExit(f"FAIL: metadata step mismatch in {path}")
    if not math.isfinite(float(data["gap_macro"])) or not math.isfinite(float(data["fn_macro"])):
        raise SystemExit(f"FAIL: non-finite gap/fn in {path}")
    if abs(float(data["gap_macro"]) - expected_gap) > 1.0e-12:
        raise SystemExit(f"FAIL: metadata gap mismatch in {path}")
    if abs(float(data["fn_macro"]) - expected_fn) > 1.0e-9:
        raise SystemExit(f"FAIL: metadata fn mismatch in {path}")

if pathlib.Path(selected_row_json["metadata_small_path"]) != metadata_paths[0]:
    raise SystemExit(f"FAIL: selected row metadata_small_path mismatch in {selected_row_json_path}")
if pathlib.Path(selected_row_json["metadata_large_path"]) != metadata_paths[1]:
    raise SystemExit(f"FAIL: selected row metadata_large_path mismatch in {selected_row_json_path}")

for path in summary_paths:
    if not path.exists():
        raise SystemExit(f"FAIL: missing receiver summary {path}")
    data = json.loads(path.read_text())
    for key in ("displacement_max_norm", "reaction_resultant_local", "deformed_output_path"):
        if key not in data:
            raise SystemExit(f"FAIL: missing {key} in {path}")
    max_disp = float(data["displacement_max_norm"])
    if not math.isfinite(max_disp) or max_disp <= 0.0:
        raise SystemExit(f"FAIL: invalid displacement_max_norm in {path}")
    reaction = data["reaction_resultant_local"]
    if len(reaction) != 3 or any(not math.isfinite(float(v)) for v in reaction):
        raise SystemExit(f"FAIL: invalid reaction_resultant_local in {path}")
    deformed_path = pathlib.Path(data["deformed_output_path"])
    if not deformed_path.exists():
        raise SystemExit(f"FAIL: missing deformed output {deformed_path}")
    if "node_id,x_local,y_local" not in deformed_path.read_text():
        raise SystemExit(f"FAIL: malformed deformed output {deformed_path}")
    print(
        f"PASS summary={path.name} "
        f"max_disp={max_disp:.6e} "
        f"reaction=({float(reaction[0]):.6e},{float(reaction[1]):.6e},{float(reaction[2]):.6e})"
    )

if pathlib.Path(selected_row_json["receiver_small_summary_path"]) != summary_paths[0]:
    raise SystemExit(f"FAIL: selected row receiver_small_summary_path mismatch in {selected_row_json_path}")
if pathlib.Path(selected_row_json["receiver_large_summary_path"]) != summary_paths[1]:
    raise SystemExit(f"FAIL: selected row receiver_large_summary_path mismatch in {selected_row_json_path}")

print(f"PASS bridge step={expected_step} gap={expected_gap:.6e} fn={expected_fn:.6e}")
PY

echo "PASS: macro to patch bridge check"
