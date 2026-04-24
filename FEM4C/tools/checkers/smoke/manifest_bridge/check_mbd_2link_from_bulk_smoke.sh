#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
OUT_DIR="${FEM4C_MBD_2LINK_FROM_BULK_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mbd_2link_from_bulk}"
SRC_BULK="${ROOT_DIR}/NastranBalkFile/MBD_2link.txt"
PARSER_OUT="${OUT_DIR}/parser_out"
PARSER_LOG="${OUT_DIR}/parser_bridge.log"
MANIFEST="${PARSER_OUT}/assembly_manifest.json"
GEN_DAT="${OUT_DIR}/generated_mbd_2link.dat"
RUN_SUMMARY="${OUT_DIR}/generated_mbd_2link.out"
RUN_LOG="${OUT_DIR}/generated_mbd_2link.log"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j >/dev/null

"${ROOT_DIR}/bin/fem4c" "${SRC_BULK}" "${PARSER_OUT}" >"${PARSER_LOG}" 2>&1

for path in "${PARSER_LOG}" "${MANIFEST}"; do
    if [[ ! -f "${path}" ]]; then
        echo "FAIL: missing artifact: ${path}" >&2
        exit 1
    fi
done

python3 - <<'PY' "${MANIFEST}"
import json
import math
import pathlib
import sys

manifest_path = pathlib.Path(sys.argv[1])
data = json.loads(manifest_path.read_text(encoding="utf-8"))

parts = {part["part_id"] for part in data["parts"]}
if parts != {"Thin Shell(1)", "Thin Shell(2)"}:
    raise SystemExit(f"FAIL: unexpected parts: {sorted(parts)}")

bodies = {body["part_id"]: body for body in data["bodies"]}
if set(bodies) != parts:
    raise SystemExit(f"FAIL: unexpected bodies: {sorted(bodies)}")

body0 = bodies["Thin Shell(1)"]
body1 = bodies["Thin Shell(2)"]

body0["mass_kg_override"] = 1.0e9
body0["iz_com_kg_m2_override"] = 1.0e9
body0["initial_pose"] = {"qx": 0.0, "qy": -1.5009788686850845e-02, "qtheta": 0.0}
body0["initial_velocity"] = {"vx": 0.0, "vy": 0.0, "omega": 0.0}

body1["initial_pose"] = {"qx": 0.0, "qy": -4.4999999896668356e-02, "qtheta": 0.0}
body1["initial_velocity"] = {"vx": 5.0e-01, "vy": 0.0, "omega": 3.3333333333333336e+01}

data["gravity"] = {"gx": 0.0, "gy": 0.0}
data["forces"] = []
data["joints"] = [
    {
        "joint_id": 1,
        "type": "revolute",
        "body_i": int(body0["body_id"]),
        "body_j": int(body1["body_id"]),
        "ai": [0.0, -1.4990211313149155e-02],
        "aj": [0.0, 1.5000000103331644e-02]
    }
]
data["coupled_flex"] = []

manifest_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(manifest_path)
PY

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${MANIFEST}" "${GEN_DAT}" >/dev/null

for path in "${GEN_DAT}"; do
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
        echo "FAIL: missing artifact: ${path}" >&2
        exit 1
    fi
done

grep -q "Detected Nastran input: ${SRC_BULK}" "${PARSER_LOG}"
grep -q "route=multipart_manifest" "${PARSER_LOG}"
grep -q "Parser export completed successfully." "${PARSER_LOG}"
grep -q "Program completed successfully." "${RUN_LOG}"

python3 - <<'PY' "${MANIFEST}" "${GEN_DAT}" "${RUN_LOG}"
import json
import pathlib
import sys

manifest_path = pathlib.Path(sys.argv[1])
gen_path = pathlib.Path(sys.argv[2])
run_log = pathlib.Path(sys.argv[3])

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if len(manifest["parts"]) != 2 or len(manifest["bodies"]) != 2:
    raise SystemExit("FAIL: manifest must contain exactly two parts/bodies")

lines = gen_path.read_text(encoding="utf-8").splitlines()
body_lines = [ln for ln in lines if ln.startswith("MBD_BODY_DYN ")]
revolute_lines = [ln for ln in lines if ln.startswith("MBD_REVOLUTE ")]
if len(body_lines) != 2:
    raise SystemExit(f"FAIL: expected 2 MBD_BODY_DYN lines, got {len(body_lines)}")
if len(revolute_lines) != 1:
    raise SystemExit(f"FAIL: expected 1 MBD_REVOLUTE line, got {len(revolute_lines)}")

body0 = body_lines[0].split()
body1 = body_lines[1].split()
if float(body0[2]) < 1.0e8 or float(body0[3]) < 1.0e8:
    raise SystemExit(f"FAIL: body0 override not applied: {body_lines[0]}")
if abs(float(body1[7]) - 5.0e-01) > 1.0e-12:
    raise SystemExit(f"FAIL: body1 vx mismatch: {body_lines[1]}")
if abs(float(body1[9]) - 3.3333333333333336e+01) > 1.0e-12:
    raise SystemExit(f"FAIL: body1 omega mismatch: {body_lines[1]}")

if "Program completed successfully." not in run_log.read_text(encoding="utf-8", errors="replace"):
    raise SystemExit("FAIL: MBD runtime log missing completion marker")
PY

echo "PASS: mbd 2link from bulk smoke"
echo "output_dir=${OUT_DIR}"
