#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

usage() {
  echo "Usage: bash scripts/run_mbd_2link_rigid_from_bulk.sh <bulk-file> <outdir> [manifest-patch.json]" >&2
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
  usage
  exit 1
fi

SRC_BULK="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
OUT_DIR="$2"
PATCH_JSON="${3:-${ROOT_DIR}/examples/mbd_2link_bulk_rigid_ground0_manifest_patch.json}"

PARSER_OUT="${OUT_DIR}/parser_export"
PARSER_LOG="${OUT_DIR}/parser_bridge.log"
BASE_MANIFEST="${PARSER_OUT}/assembly_manifest.json"
MERGED_MANIFEST="${OUT_DIR}/merged_manifest.json"
GEN_DAT="${OUT_DIR}/generated_mbd_2link_rigid.dat"
RUN_SUMMARY="${OUT_DIR}/generated_mbd_2link_rigid.out"
RUN_LOG="${OUT_DIR}/fem4c_run.log"
SUMMARY_JSON="${OUT_DIR}/result_summary.json"
SUMMARY_CSV="${OUT_DIR}/result_summary.csv"
SUMMARY_TXT="${OUT_DIR}/result_summary.txt"

if [[ ! -f "${SRC_BULK}" ]]; then
  echo "FAIL: missing bulk file ${SRC_BULK}" >&2
  exit 1
fi
if [[ ! -f "${PATCH_JSON}" ]]; then
  echo "FAIL: missing manifest patch ${PATCH_JSON}" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"
rm -rf "${PARSER_OUT}"
rm -f "${PARSER_LOG}" "${MERGED_MANIFEST}" "${GEN_DAT}" \
      "${RUN_SUMMARY}" "${RUN_SUMMARY}.history.csv" "${RUN_LOG}" \
      "${SUMMARY_JSON}" "${SUMMARY_CSV}" "${SUMMARY_TXT}"

make -C "${ROOT_DIR}" -j >/dev/null

"${ROOT_DIR}/bin/fem4c" "${SRC_BULK}" "${PARSER_OUT}" >"${PARSER_LOG}" 2>&1

for path in "${PARSER_LOG}" "${BASE_MANIFEST}" "${PATCH_JSON}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing artifact: ${path}" >&2
    exit 1
  fi
done

python3 - <<'PY' "${BASE_MANIFEST}" "${PATCH_JSON}" "${MERGED_MANIFEST}"
import json
from pathlib import Path
import sys

base_manifest_path = Path(sys.argv[1])
patch_path = Path(sys.argv[2])
merged_manifest_path = Path(sys.argv[3])

base = json.loads(base_manifest_path.read_text(encoding="utf-8"))
patch = json.loads(patch_path.read_text(encoding="utf-8"))

expected_parts = set(patch["expected_parts"])
parts = {part["part_id"] for part in base["parts"]}
if parts != expected_parts:
    raise SystemExit(f"FAIL: unexpected parts: {sorted(parts)}")

body_by_part = {body["part_id"]: body for body in base["bodies"]}
if set(body_by_part) != expected_parts:
    raise SystemExit(f"FAIL: unexpected bodies: {sorted(body_by_part)}")

body_ids = {int(body["body_id"]) for body in base["bodies"]}

for body in base["bodies"]:
    body.pop("mass_kg_override", None)
    body.pop("iz_com_kg_m2_override", None)
    body["is_ground"] = False

for update in patch["body_updates"]:
    part_id = update["part_id"]
    if part_id not in body_by_part:
        raise SystemExit(f"FAIL: patch references unknown part_id: {part_id}")
    body = body_by_part[part_id]
    for key, value in update.items():
        if key == "part_id":
            continue
        body[key] = value

ground_ids = [int(body["body_id"]) for body in base["bodies"] if bool(body.get("is_ground", False))]
if len(ground_ids) > 1:
    raise SystemExit(f"FAIL: patch resolves to multiple ground bodies: {ground_ids}")

for joint in patch["joints"]:
    if int(joint["body_i"]) not in body_ids:
        raise SystemExit(f"FAIL: patch joint references unknown body_i: {joint['body_i']}")
    if int(joint["body_j"]) not in body_ids:
        raise SystemExit(f"FAIL: patch joint references unknown body_j: {joint['body_j']}")

for part in base["parts"]:
    for key in ("package_dir", "mesh_path", "material_path", "body_properties_path", "boundary_path"):
        value = str(part[key])
        if value.startswith("/") or value.startswith("parser_export/"):
            continue
        part[key] = f"parser_export/{value}"

base["gravity"] = patch["gravity"]
base["forces"] = list(patch.get("forces", []))
base["joints"] = list(patch["joints"])
base["coupled_flex"] = list(patch.get("coupled_flex", []))

merged_manifest_path.write_text(
    json.dumps(base, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
)
print(merged_manifest_path)
PY

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${MERGED_MANIFEST}" "${GEN_DAT}" >/dev/null

for path in "${MERGED_MANIFEST}" "${GEN_DAT}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing artifact: ${path}" >&2
    exit 1
  fi
done

"${ROOT_DIR}/bin/fem4c" \
  --mode=mbd \
  --mbd-integrator=newmark_beta \
  --mbd-dt=1.0e-4 \
  --mbd-steps=20 \
  "${GEN_DAT}" "${RUN_SUMMARY}" >"${RUN_LOG}" 2>&1

for path in "${RUN_LOG}" "${RUN_SUMMARY}" "${RUN_SUMMARY}.history.csv"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing runtime artifact: ${path}" >&2
    exit 1
  fi
done

python3 - <<'PY' "${MERGED_MANIFEST}" "${RUN_SUMMARY}.history.csv" "${RUN_LOG}" "${SUMMARY_JSON}" "${SUMMARY_CSV}" "${SUMMARY_TXT}" "${SRC_BULK}" "${PATCH_JSON}" "${PARSER_OUT}" "${GEN_DAT}" "${RUN_SUMMARY}"
import csv
import json
import math
from pathlib import Path
import sys

manifest_path = Path(sys.argv[1])
history_path = Path(sys.argv[2])
run_log_path = Path(sys.argv[3])
summary_json_path = Path(sys.argv[4])
summary_csv_path = Path(sys.argv[5])
summary_txt_path = Path(sys.argv[6])
bulk_path = Path(sys.argv[7])
patch_path = Path(sys.argv[8])
parser_out = Path(sys.argv[9])
generated_deck = Path(sys.argv[10])
run_summary = Path(sys.argv[11])

ground_pose_tolerance = 1.0e-6
moved_pose_tolerance = 1.0e-5
revolute_anchor_mismatch_tolerance = 1.0e-4

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
rows = list(csv.DictReader(history_path.open(encoding="utf-8", newline="")))
run_log_text = run_log_path.read_text(encoding="utf-8", errors="replace")

if not rows:
    raise SystemExit(f"FAIL: empty history csv: {history_path}")

body_meta = {int(body["body_id"]): body for body in manifest["bodies"]}
body_rows = {}
step_rows = {}
for row in rows:
    body_id = int(row["body_id"])
    step = int(row["step"])
    body_rows.setdefault(body_id, []).append(row)
    step_rows.setdefault(step, {})[body_id] = row

def _state(row, keys):
    return {key: float(row[key]) for key in keys}

summary = {
    "bulk_file": str(bulk_path),
    "patch_file": str(patch_path),
    "parser_export_root": str(parser_out),
    "merged_manifest": str(manifest_path),
    "generated_mbd_deck": str(generated_deck),
    "run_summary": str(run_summary),
    "run_log": str(run_log_path),
    "history_csv": str(history_path),
    "program_completed_successfully": "Program completed successfully." in run_log_text,
    "tolerances": {
        "ground_pose": ground_pose_tolerance,
        "moved_pose": moved_pose_tolerance,
        "revolute_anchor_mismatch": revolute_anchor_mismatch_tolerance,
    },
    "bodies": [],
    "revolute_joints": [],
}

csv_rows = []

def emit(scope, ident, key, value):
    csv_rows.append({
        "scope": scope,
        "id": "" if ident is None else str(ident),
        "key": key,
        "value": str(value),
    })

for key, value in [
    ("bulk_file", bulk_path),
    ("patch_file", patch_path),
    ("parser_export_root", parser_out),
    ("merged_manifest", manifest_path),
    ("generated_mbd_deck", generated_deck),
    ("run_summary", run_summary),
    ("run_log", run_log_path),
    ("history_csv", history_path),
    ("program_completed_successfully", int(summary["program_completed_successfully"])),
    ("ground_pose_tolerance", ground_pose_tolerance),
    ("moved_pose_tolerance", moved_pose_tolerance),
    ("revolute_anchor_mismatch_tolerance", revolute_anchor_mismatch_tolerance),
]:
    emit("run", None, key, value)

for body_id in sorted(body_rows):
    body = body_meta[body_id]
    body_hist = sorted(body_rows[body_id], key=lambda row: int(row["step"]))
    initial = body_hist[0]
    final = body_hist[-1]
    initial_q = _state(initial, ("x", "y", "theta"))
    final_q = _state(final, ("x", "y", "theta"))
    initial_v = _state(initial, ("vx", "vy", "omega"))
    final_v = _state(final, ("vx", "vy", "omega"))
    dx = final_q["x"] - initial_q["x"]
    dy = final_q["y"] - initial_q["y"]
    dtheta = final_q["theta"] - initial_q["theta"]
    translational_delta_norm = math.hypot(dx, dy)
    angular_delta_abs = abs(dtheta)
    stayed_fixed_like_ground = (
        translational_delta_norm <= ground_pose_tolerance and
        angular_delta_abs <= ground_pose_tolerance
    )
    moved_from_initial = (
        translational_delta_norm >= moved_pose_tolerance or
        angular_delta_abs >= moved_pose_tolerance
    )
    body_summary = {
        "body_id": body_id,
        "part_id": str(body["part_id"]),
        "is_ground": bool(body.get("is_ground", False)),
        "initial_q": initial_q,
        "final_q": final_q,
        "initial_v": initial_v,
        "final_v": final_v,
        "translational_delta_norm": translational_delta_norm,
        "angular_delta_abs": angular_delta_abs,
        "stayed_fixed_like_ground": stayed_fixed_like_ground,
        "moved_from_initial": moved_from_initial,
    }
    summary["bodies"].append(body_summary)

    emit("body", body_id, "part_id", body_summary["part_id"])
    emit("body", body_id, "is_ground", int(body_summary["is_ground"]))
    for state_name, state in [
        ("initial_q", initial_q),
        ("final_q", final_q),
        ("initial_v", initial_v),
        ("final_v", final_v),
    ]:
        for key, value in state.items():
            emit("body", body_id, f"{state_name}_{key}", value)
    emit("body", body_id, "translational_delta_norm", translational_delta_norm)
    emit("body", body_id, "angular_delta_abs", angular_delta_abs)
    emit("body", body_id, "stayed_fixed_like_ground", int(stayed_fixed_like_ground))
    emit("body", body_id, "moved_from_initial", int(moved_from_initial))

max_revolute_anchor_mismatch = 0.0
for joint in sorted(manifest["joints"], key=lambda item: int(item["joint_id"])):
    if joint["type"] != "revolute":
        continue
    body_i = int(joint["body_i"])
    body_j = int(joint["body_j"])
    ai = [float(joint["ai"][0]), float(joint["ai"][1])]
    aj = [float(joint["aj"][0]), float(joint["aj"][1])]
    joint_max = 0.0
    joint_step = None

    for step in sorted(step_rows):
        step_map = step_rows[step]
        if body_i not in step_map or body_j not in step_map:
            continue

        def anchor(row, local):
            x = float(row["x"])
            y = float(row["y"])
            theta = float(row["theta"])
            c = math.cos(theta)
            s = math.sin(theta)
            return (
                x + c * local[0] - s * local[1],
                y + s * local[0] + c * local[1],
            )

        pi = anchor(step_map[body_i], ai)
        pj = anchor(step_map[body_j], aj)
        mismatch = math.hypot(pi[0] - pj[0], pi[1] - pj[1])
        if mismatch > joint_max:
            joint_max = mismatch
            joint_step = step

    joint_summary = {
        "joint_id": int(joint["joint_id"]),
        "body_i": body_i,
        "body_j": body_j,
        "max_anchor_mismatch": joint_max,
        "max_anchor_mismatch_step": joint_step,
    }
    summary["revolute_joints"].append(joint_summary)
    max_revolute_anchor_mismatch = max(max_revolute_anchor_mismatch, joint_max)

    emit("joint", joint_summary["joint_id"], "body_i", body_i)
    emit("joint", joint_summary["joint_id"], "body_j", body_j)
    emit("joint", joint_summary["joint_id"], "max_anchor_mismatch", joint_max)
    emit("joint", joint_summary["joint_id"], "max_anchor_mismatch_step", "" if joint_step is None else joint_step)

summary["max_revolute_anchor_mismatch"] = max_revolute_anchor_mismatch
emit("run", None, "max_revolute_anchor_mismatch", max_revolute_anchor_mismatch)

summary_json_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
with summary_csv_path.open("w", encoding="utf-8", newline="") as fp:
    writer = csv.DictWriter(fp, fieldnames=["scope", "id", "key", "value"])
    writer.writeheader()
    writer.writerows(csv_rows)

lines = [
    "rigid_2link_result_summary",
    f"bulk_file={bulk_path}",
    f"patch_file={patch_path}",
    f"parser_export_root={parser_out}",
    f"merged_manifest={manifest_path}",
    f"generated_mbd_deck={generated_deck}",
    f"run_summary={run_summary}",
    f"run_log={run_log_path}",
    f"history_csv={history_path}",
    f"program_completed_successfully={int(summary['program_completed_successfully'])}",
    f"ground_pose_tolerance={ground_pose_tolerance:.16e}",
    f"moved_pose_tolerance={moved_pose_tolerance:.16e}",
    f"revolute_anchor_mismatch_tolerance={revolute_anchor_mismatch_tolerance:.16e}",
]
for body in summary["bodies"]:
    lines.append(
        "body[{body_id}] part_id={part_id} is_ground={is_ground} "
        "initial_q=({iqx:.16e},{iqy:.16e},{iqt:.16e}) "
        "final_q=({fqx:.16e},{fqy:.16e},{fqt:.16e}) "
        "initial_v=({ivx:.16e},{ivy:.16e},{ivo:.16e}) "
        "final_v=({fvx:.16e},{fvy:.16e},{fvo:.16e}) "
        "translational_delta_norm={td:.16e} angular_delta_abs={ad:.16e} "
        "stayed_fixed_like_ground={fixed} moved_from_initial={moved}".format(
            body_id=body["body_id"],
            part_id=body["part_id"],
            is_ground=int(body["is_ground"]),
            iqx=body["initial_q"]["x"],
            iqy=body["initial_q"]["y"],
            iqt=body["initial_q"]["theta"],
            fqx=body["final_q"]["x"],
            fqy=body["final_q"]["y"],
            fqt=body["final_q"]["theta"],
            ivx=body["initial_v"]["vx"],
            ivy=body["initial_v"]["vy"],
            ivo=body["initial_v"]["omega"],
            fvx=body["final_v"]["vx"],
            fvy=body["final_v"]["vy"],
            fvo=body["final_v"]["omega"],
            td=body["translational_delta_norm"],
            ad=body["angular_delta_abs"],
            fixed=int(body["stayed_fixed_like_ground"]),
            moved=int(body["moved_from_initial"]),
        )
    )
    lines.append(
        "body{body_id}_stayed_fixed_like_ground={fixed}".format(
            body_id=body["body_id"],
            fixed=int(body["stayed_fixed_like_ground"]),
        )
    )
    lines.append(
        "body{body_id}_moved_from_initial={moved}".format(
            body_id=body["body_id"],
            moved=int(body["moved_from_initial"]),
        )
    )
for joint in summary["revolute_joints"]:
    lines.append(
        "joint[{joint_id}] type=revolute body_i={body_i} body_j={body_j} "
        "max_anchor_mismatch={mismatch:.16e} max_anchor_mismatch_step={step}".format(
            joint_id=joint["joint_id"],
            body_i=joint["body_i"],
            body_j=joint["body_j"],
            mismatch=joint["max_anchor_mismatch"],
            step="" if joint["max_anchor_mismatch_step"] is None else joint["max_anchor_mismatch_step"],
        )
    )
lines.append(f"max_revolute_anchor_mismatch={max_revolute_anchor_mismatch:.16e}")
summary_txt_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

grep -q "Program completed successfully." "${RUN_LOG}"

echo "PASS: rigid 2link bulk run"
echo "outdir=${OUT_DIR}"
echo "parser_export=${PARSER_OUT}"
echo "merged_manifest=${MERGED_MANIFEST}"
echo "generated_mbd_deck=${GEN_DAT}"
echo "run_log=${RUN_LOG}"
echo "result_summary_json=${SUMMARY_JSON}"
echo "result_summary_csv=${SUMMARY_CSV}"
echo "result_summary_txt=${SUMMARY_TXT}"
