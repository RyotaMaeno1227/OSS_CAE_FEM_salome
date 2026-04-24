#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"

OUT_DIR="${1:-/tmp/test_build_mbd_master_from_manifest_$(date +%Y%m%d_%H%M%S)}"
PARSER_OUT="${OUT_DIR}/parser_out"
GEN="${OUT_DIR}/generated_master.dat"
RUN_LOG="${OUT_DIR}/runtime.log"
RUN_DAT="${OUT_DIR}/runtime.dat"
SOURCE_BULK="${ROOT_DIR}/NastranBalkFile/MBD_2link.txt"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

"${ROOT_DIR}/parser/parser" "${SOURCE_BULK}" "${PARSER_OUT}" >/dev/null

MANIFEST="${PARSER_OUT}/assembly_manifest.json"
if [[ ! -f "${MANIFEST}" ]]; then
  echo "FAIL: missing manifest ${MANIFEST}" >&2
  exit 1
fi

python3 - <<'PY' "${MANIFEST}"
from pathlib import Path
import json
import sys

manifest_path = Path(sys.argv[1])
data = json.loads(manifest_path.read_text(encoding="utf-8"))
body0 = next((body for body in data["bodies"] if int(body["body_id"]) == 0), None)
if body0 is None:
    raise SystemExit("missing body_id 0 in manifest")
body0["is_ground"] = True
manifest_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
PY

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${MANIFEST}" "${GEN}" >/dev/null

grep -nE '^MBD_BODY_DYN|^MBD_BODY_GROUND|^MBD_GRAVITY' "${GEN}"

python3 - <<'PY' "${MANIFEST}" "${GEN}"
from pathlib import Path
import json
import sys

manifest = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
text = Path(sys.argv[2]).read_text(encoding="utf-8").splitlines()
body_lines = [ln for ln in text if ln.startswith("MBD_BODY_DYN ")]
ground_lines = [ln for ln in text if ln.startswith("MBD_BODY_GROUND ")]

part_ids = {part["part_id"] for part in manifest["parts"]}
if part_ids != {"Thin Shell(1)", "Thin Shell(2)"}:
    raise SystemExit(f"unexpected parts: {sorted(part_ids)}")

if len(body_lines) != len(manifest["bodies"]):
    raise SystemExit(f"expected {len(manifest['bodies'])} MBD_BODY_DYN lines, got {len(body_lines)}")
if ground_lines != ["MBD_BODY_GROUND 0"]:
    raise SystemExit(f"expected exactly ['MBD_BODY_GROUND 0'], got {ground_lines}")

expected = {}
part_map = {part["part_id"]: part for part in manifest["parts"]}
for body in manifest["bodies"]:
    part = part_map[body["part_id"]]
    props = json.loads((Path(sys.argv[1]).parent / part["body_properties_path"]).read_text(encoding="utf-8"))
    expected[int(body["body_id"])] = (float(props["mass_kg"]), float(props["iz_com_kg_m2"]))

for line in body_lines:
    parts = line.split()
    bid = int(parts[1])
    mass = float(parts[2])
    inertia = float(parts[3])
    exp_mass, exp_inertia = expected[bid]
    if abs(mass - exp_mass) > 1.0e-15:
        raise SystemExit(f"body {bid}: expected parser mass {exp_mass}, got {mass}")
    if abs(inertia - exp_inertia) > 1.0e-18:
        raise SystemExit(f"body {bid}: expected parser inertia {exp_inertia}, got {inertia}")

print(f"body_count={len(body_lines)}")
print(f"part_ids={sorted(part_ids)}")
print(f"ground_lines={ground_lines}")
PY

if [[ -x "${ROOT_DIR}/bin/fem4c" ]]; then
  "${ROOT_DIR}/bin/fem4c" --mode=mbd --mbd-integrator=newmark_beta "${GEN}" "${RUN_DAT}" >"${RUN_LOG}" 2>&1
  grep -n 'Program completed successfully\.' "${RUN_LOG}"
fi

echo "PASS: build_mbd_master_from_manifest smoke"
