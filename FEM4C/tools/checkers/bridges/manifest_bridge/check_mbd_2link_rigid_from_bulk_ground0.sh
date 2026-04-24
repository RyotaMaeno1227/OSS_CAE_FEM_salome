#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SRC_BULK="${1:-${ROOT_DIR}/NastranBalkFile/MBD_2link.txt}"
PATCH_JSON="${ROOT_DIR}/examples/mbd_2link_bulk_rigid_ground0_manifest_patch.json"
OUT_DIR="${FEM4C_MBD_2LINK_RIGID_GROUND0_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mbd_2link_rigid_ground0}"
PARSER_OUT="${OUT_DIR}/parser_out"
PARSER_LOG="${OUT_DIR}/parser_bridge.log"
BASE_MANIFEST="${PARSER_OUT}/assembly_manifest.json"
PATCHED_MANIFEST="${PARSER_OUT}/assembly_manifest_ground0.json"
GEN_DAT="${OUT_DIR}/generated_mbd_2link_rigid_ground0.dat"
RUN_SUMMARY="${OUT_DIR}/generated_mbd_2link_rigid_ground0.out"
RUN_LOG="${OUT_DIR}/generated_mbd_2link_rigid_ground0.log"

if [[ ! -f "${SRC_BULK}" ]]; then
  echo "FAIL: missing bulk file ${SRC_BULK}" >&2
  exit 1
fi

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j >/dev/null

"${ROOT_DIR}/bin/fem4c" "${SRC_BULK}" "${PARSER_OUT}" >"${PARSER_LOG}" 2>&1

for path in "${PARSER_LOG}" "${BASE_MANIFEST}" "${PATCH_JSON}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing artifact: ${path}" >&2
    exit 1
  fi
done

python3 - <<'PY' "${BASE_MANIFEST}" "${PATCH_JSON}" "${PATCHED_MANIFEST}"
import json
from pathlib import Path
import sys

base_manifest_path = Path(sys.argv[1])
patch_path = Path(sys.argv[2])
patched_manifest_path = Path(sys.argv[3])

base = json.loads(base_manifest_path.read_text(encoding="utf-8"))
patch = json.loads(patch_path.read_text(encoding="utf-8"))

expected_parts = set(patch["expected_parts"])
parts = {part["part_id"] for part in base["parts"]}
if parts != expected_parts:
    raise SystemExit(f"FAIL: unexpected parts: {sorted(parts)}")

body_by_part = {body["part_id"]: body for body in base["bodies"]}
if set(body_by_part) != expected_parts:
    raise SystemExit(f"FAIL: unexpected bodies: {sorted(body_by_part)}")

body0 = body_by_part["Thin Shell(1)"]
body1 = body_by_part["Thin Shell(2)"]
if int(body0["body_id"]) != 0 or int(body1["body_id"]) != 1:
    raise SystemExit(
        "FAIL: canonical ground0 route expects Thin Shell(1)->body 0 and Thin Shell(2)->body 1"
    )

for body in base["bodies"]:
    body.pop("mass_kg_override", None)
    body.pop("iz_com_kg_m2_override", None)
    body["is_ground"] = False

for update in patch["body_updates"]:
    pid = update["part_id"]
    if pid not in body_by_part:
        raise SystemExit(f"FAIL: patch references unknown part_id: {pid}")
    body = body_by_part[pid]
    for key, value in update.items():
        if key == "part_id":
            continue
        body[key] = value

for joint in patch["joints"]:
    if int(joint["body_i"]) not in {int(body["body_id"]) for body in base["bodies"]}:
        raise SystemExit(f"FAIL: patch joint references unknown body_i: {joint['body_i']}")
    if int(joint["body_j"]) not in {int(body["body_id"]) for body in base["bodies"]}:
        raise SystemExit(f"FAIL: patch joint references unknown body_j: {joint['body_j']}")

base["gravity"] = patch["gravity"]
base["forces"] = list(patch.get("forces", []))
base["joints"] = list(patch["joints"])
base["coupled_flex"] = list(patch.get("coupled_flex", []))

patched_manifest_path.write_text(
    json.dumps(base, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
)
print(patched_manifest_path)
PY

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${PATCHED_MANIFEST}" "${GEN_DAT}" >/dev/null

for path in "${PATCHED_MANIFEST}" "${GEN_DAT}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing artifact: ${path}" >&2
    exit 1
  fi
done

python3 - <<'PY' "${PATCHED_MANIFEST}" "${GEN_DAT}"
import json
from pathlib import Path
import sys

manifest_path = Path(sys.argv[1])
gen_path = Path(sys.argv[2])

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
part_map = {part["part_id"]: part for part in manifest["parts"]}
expected = {}
for body in manifest["bodies"]:
    props_path = (manifest_path.parent / part_map[body["part_id"]]["body_properties_path"]).resolve()
    props = json.loads(props_path.read_text(encoding="utf-8"))
    expected[int(body["body_id"])] = (float(props["mass_kg"]), float(props["iz_com_kg_m2"]))

lines = gen_path.read_text(encoding="utf-8").splitlines()
body_lines = [ln for ln in lines if ln.startswith("MBD_BODY_DYN ")]
ground_lines = [ln for ln in lines if ln.startswith("MBD_BODY_GROUND ")]
revolute_lines = [ln for ln in lines if ln.startswith("MBD_REVOLUTE ")]

if len(body_lines) != 2:
    raise SystemExit(f"FAIL: expected 2 MBD_BODY_DYN lines, got {len(body_lines)}")
if ground_lines != ["MBD_BODY_GROUND 0"]:
    raise SystemExit(f"FAIL: expected exactly ['MBD_BODY_GROUND 0'], got {ground_lines}")
if len(revolute_lines) != 1:
    raise SystemExit(f"FAIL: expected 1 MBD_REVOLUTE line, got {len(revolute_lines)}")

for line in body_lines:
    fields = line.split()
    body_id = int(fields[1])
    mass = float(fields[2])
    inertia = float(fields[3])
    exp_mass, exp_inertia = expected[body_id]
    if abs(mass - exp_mass) > 1.0e-15:
        raise SystemExit(f"FAIL: body {body_id} mass mismatch: expected {exp_mass}, got {mass}")
    if abs(inertia - exp_inertia) > 1.0e-18:
        raise SystemExit(f"FAIL: body {body_id} inertia mismatch: expected {exp_inertia}, got {inertia}")

print(f"body_lines={len(body_lines)}")
print(f"ground_lines={ground_lines}")
print(f"revolute_lines={len(revolute_lines)}")
PY

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

grep -q "Detected Nastran input: ${SRC_BULK}" "${PARSER_LOG}"
grep -q "route=multipart_manifest" "${PARSER_LOG}"
grep -q "Parser export completed successfully." "${PARSER_LOG}"
grep -q "Program completed successfully." "${RUN_LOG}"

echo "PASS: mbd 2link rigid bulk ground0"
echo "output_dir=${OUT_DIR}"
