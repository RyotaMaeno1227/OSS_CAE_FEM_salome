#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
WORK_DIR="${1:-/tmp/pma033_routeb_$(date +%Y%m%d_%H%M%S)}"
SRC_BDF="${ROOT_DIR}/NastranBalkFile/2Dmesh.dat"
DECK="${WORK_DIR}/routeb_two_part_from_real_bulk.dat"
PARSER_OUT="${WORK_DIR}/parser_out"
GEN_DAT="${WORK_DIR}/generated_mbd.dat"
RUN_DAT="${WORK_DIR}/generated_mbd_out.dat"
RUN_LOG="${WORK_DIR}/generated_mbd.log"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

python3 - <<'PY' "${SRC_BDF}" "${DECK}"
from pathlib import Path
import sys

src = Path(sys.argv[1])
dst = Path(sys.argv[2])

lines = src.read_text(encoding="utf-8", errors="replace").splitlines()

elem_line = None
for ln in lines:
    s = ln.lstrip()
    if s.startswith("CTRIA3") or s.startswith("CTRIA6"):
        elem_line = ln
        break

if elem_line is None:
    raise SystemExit("no CTRIA3/CTRIA6 line found in source bulk file")

out_lines = list(lines)
out_lines.append("")
out_lines.append("$*  Mesh Collector: Part2")
out_lines.append(elem_line)

dst.write_text("\n".join(out_lines) + "\n", encoding="utf-8")
print(dst)
PY

"${ROOT_DIR}/parser/parser" "${DECK}" "${PARSER_OUT}"

echo "===== parser output ====="
find "${PARSER_OUT}" -maxdepth 5 -type f | sort

python3 - <<'PY' "${PARSER_OUT}/Parts/Part1/material/body_properties.json" "${PARSER_OUT}/Parts/Part2/material/body_properties.json"
import json, sys
for p in sys.argv[1:]:
    data = json.load(open(p, encoding="utf-8"))
    mass = float(data["mass_kg"])
    inertia = float(data["iz_com_kg_m2"])
    print(f"{p}: mass_kg={mass:.16e} iz_com_kg_m2={inertia:.16e}")
    if mass <= 0.0:
        raise SystemExit(f"{p}: mass_kg must be > 0")
    if inertia <= 0.0:
        raise SystemExit(f"{p}: iz_com_kg_m2 must be > 0")
PY

python3 - <<'PY' "${PARSER_OUT}/assembly_manifest.json"
import json, sys
from pathlib import Path

p = Path(sys.argv[1])
data = json.loads(p.read_text(encoding="utf-8"))

body_id_by_part = {}
for body in data["bodies"]:
    body_id_by_part[body["part_id"]] = int(body["body_id"])

if set(body_id_by_part.keys()) != {"Part1", "Part2"}:
    raise SystemExit(f"unexpected body map: {body_id_by_part}")

for body in data["bodies"]:
    if body["part_id"] == "Part1":
        body["initial_pose"] = {"qx": 0.0, "qy": 0.0, "qtheta": 0.0}
        body["initial_velocity"] = {"vx": 0.0, "vy": 0.0, "omega": 0.0}
    elif body["part_id"] == "Part2":
        body["initial_pose"] = {"qx": 0.01, "qy": 0.0, "qtheta": 0.0}
        body["initial_velocity"] = {"vx": 0.0, "vy": 0.0, "omega": 0.0}

data["gravity"] = {"gx": 0.0, "gy": 0.0}
data["forces"] = []
data["joints"] = [
    {
        "joint_id": 1,
        "type": "revolute",
        "body_i": body_id_by_part["Part1"],
        "body_j": body_id_by_part["Part2"],
        "ai": [0.005, 0.0],
        "aj": [-0.005, 0.0]
    }
]

p.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(p)
PY

echo
echo "===== manifest ====="
python3 -m json.tool "${PARSER_OUT}/assembly_manifest.json"

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" \
  "${PARSER_OUT}/assembly_manifest.json" \
  "${GEN_DAT}"

echo
echo "===== generated directives ====="
grep -nE '^MBD_BODY_DYN|^MBD_GRAVITY|^MBD_REVOLUTE' "${GEN_DAT}"

python3 - <<'PY' "${GEN_DAT}"
from pathlib import Path
import sys

lines = Path(sys.argv[1]).read_text(encoding="utf-8").splitlines()
body_lines = [ln for ln in lines if ln.startswith("MBD_BODY_DYN ")]
if len(body_lines) != 2:
    raise SystemExit(f"expected 2 MBD_BODY_DYN lines, got {len(body_lines)}")
print(f"generated_body_count={len(body_lines)}")
PY

"${ROOT_DIR}/bin/fem4c" --mode=mbd --mbd-integrator=newmark_beta \
  "${GEN_DAT}" "${RUN_DAT}" > "${RUN_LOG}" 2>&1

echo
echo "===== runtime log key lines ====="
grep -nE 'Bodies:|Constraints:|Constraint equations:|Program completed successfully\.|body\[0\]:|body\[1\]:' "${RUN_LOG}"

echo
echo "ROUTE_B_PASS=1"
echo "WORK_DIR=${WORK_DIR}"
echo "DECK=${DECK}"
echo "PARSER_OUT=${PARSER_OUT}"
echo "MANIFEST=${PARSER_OUT}/assembly_manifest.json"
echo "GEN_DAT=${GEN_DAT}"
echo "RUN_LOG=${RUN_LOG}"
echo "RUN_DAT=${RUN_DAT}"
