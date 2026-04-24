#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SRC_BULK="${1:-${ROOT_DIR}/NastranBalkFile/MBD_2link.txt}"
OUT_DIR="${FEM4C_MBD_2LINK_ENFORCEMENT_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mbd_2link_ground_revolute_enforcement}"
PARSER_OUT="${OUT_DIR}/parser_out"
PARSER_LOG="${OUT_DIR}/parser_bridge.log"
BASE_MANIFEST="${PARSER_OUT}/assembly_manifest.json"
PATCH_A="${ROOT_DIR}/examples/mbd_2link_bulk_rigid_ground0_verify_manifest_patch.json"
PATCH_B="${ROOT_DIR}/examples/mbd_2link_bulk_rigid_ground0_revolute_nogravity_manifest_patch.json"
MANIFEST_A="${PARSER_OUT}/ground_only_fall_manifest.json"
MANIFEST_B="${PARSER_OUT}/ground_plus_revolute_nogravity_manifest.json"
DECK_A="${OUT_DIR}/ground_only_fall.dat"
DECK_B="${OUT_DIR}/ground_plus_revolute_nogravity.dat"
OUT_A="${OUT_DIR}/ground_only_fall.out"
OUT_B="${OUT_DIR}/ground_plus_revolute_nogravity.out"
LOG_A="${OUT_DIR}/ground_only_fall.log"
LOG_B="${OUT_DIR}/ground_plus_revolute_nogravity.log"
SUMMARY_A="${OUT_DIR}/ground_only_fall.summary.json"
SUMMARY_B="${OUT_DIR}/ground_plus_revolute_nogravity.summary.json"

A_GROUND_DISP_TOL="${A_GROUND_DISP_TOL:-1.0e-8}"
A_BODY1_DROP_MIN="${A_BODY1_DROP_MIN:-1.0e-4}"
B_GROUND_DISP_TOL="${B_GROUND_DISP_TOL:-1.0e-8}"
B_THETA_SPAN_MIN="${B_THETA_SPAN_MIN:-5.0e-1}"
B_RADIUS_DRIFT_TOL="${B_RADIUS_DRIFT_TOL:-1.0e-5}"
B_REVOLUTE_MISMATCH_TOL="${B_REVOLUTE_MISMATCH_TOL:-1.0e-6}"

if [[ ! -f "${SRC_BULK}" ]]; then
  echo "FAIL: missing bulk file ${SRC_BULK}" >&2
  exit 1
fi

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j >/dev/null

"${ROOT_DIR}/bin/fem4c" "${SRC_BULK}" "${PARSER_OUT}" >"${PARSER_LOG}" 2>&1

for path in "${PARSER_LOG}" "${BASE_MANIFEST}" "${PATCH_A}" "${PATCH_B}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing artifact: ${path}" >&2
    exit 1
  fi
done

python3 - <<'PY' "${BASE_MANIFEST}" "${PATCH_A}" "${MANIFEST_A}"
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
    raise SystemExit(f"FAIL: unexpected parts for {patch_path.name}: {sorted(parts)}")

body_by_part = {body["part_id"]: body for body in base["bodies"]}
if set(body_by_part) != expected_parts:
    raise SystemExit(f"FAIL: unexpected bodies for {patch_path.name}: {sorted(body_by_part)}")

for body in base["bodies"]:
    body.pop("mass_kg_override", None)
    body.pop("iz_com_kg_m2_override", None)
    body["is_ground"] = False

for update in patch["body_updates"]:
    body = body_by_part[update["part_id"]]
    for key, value in update.items():
        if key != "part_id":
            body[key] = value

base["gravity"] = patch["gravity"]
base["forces"] = list(patch.get("forces", []))
base["joints"] = list(patch.get("joints", []))
base["coupled_flex"] = list(patch.get("coupled_flex", []))
patched_manifest_path.write_text(json.dumps(base, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(patched_manifest_path)
PY

python3 - <<'PY' "${BASE_MANIFEST}" "${PATCH_B}" "${MANIFEST_B}"
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
    raise SystemExit(f"FAIL: unexpected parts for {patch_path.name}: {sorted(parts)}")

body_by_part = {body["part_id"]: body for body in base["bodies"]}
if set(body_by_part) != expected_parts:
    raise SystemExit(f"FAIL: unexpected bodies for {patch_path.name}: {sorted(body_by_part)}")

for body in base["bodies"]:
    body.pop("mass_kg_override", None)
    body.pop("iz_com_kg_m2_override", None)
    body["is_ground"] = False

for update in patch["body_updates"]:
    body = body_by_part[update["part_id"]]
    for key, value in update.items():
        if key != "part_id":
            body[key] = value

base["gravity"] = patch["gravity"]
base["forces"] = list(patch.get("forces", []))
base["joints"] = list(patch.get("joints", []))
base["coupled_flex"] = list(patch.get("coupled_flex", []))
patched_manifest_path.write_text(json.dumps(base, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(patched_manifest_path)
PY

python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${MANIFEST_A}" "${DECK_A}" >/dev/null
python3 "${ROOT_DIR}/scripts/build_mbd_master_from_manifest.py" "${MANIFEST_B}" "${DECK_B}" >/dev/null

grep -q '^MBD_BODY_GROUND 0$' "${DECK_A}"
if grep -q '^MBD_REVOLUTE ' "${DECK_A}"; then
  echo "FAIL: ground_only_fall deck must not contain MBD_REVOLUTE" >&2
  exit 1
fi

grep -q '^MBD_BODY_GROUND 0$' "${DECK_B}"
grep -q '^MBD_REVOLUTE ' "${DECK_B}"

"${ROOT_DIR}/bin/fem4c" \
  --mode=mbd \
  --mbd-integrator=newmark_beta \
  --mbd-dt=1.0e-4 \
  --mbd-steps=200 \
  "${DECK_A}" "${OUT_A}" >"${LOG_A}" 2>&1

"${ROOT_DIR}/bin/fem4c" \
  --mode=mbd \
  --mbd-integrator=newmark_beta \
  --mbd-dt=1.0e-4 \
  --mbd-steps=200 \
  "${DECK_B}" "${OUT_B}" >"${LOG_B}" 2>&1

for path in "${OUT_A}.history.csv" "${OUT_B}.history.csv" "${LOG_A}" "${LOG_B}" "${OUT_A}" "${OUT_B}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing runtime artifact: ${path}" >&2
    exit 1
  fi
done

python3 "${ROOT_DIR}/scripts/analyze_mbd_2link_history.py" "${OUT_A}.history.csv" "${SUMMARY_A}"
python3 "${ROOT_DIR}/scripts/analyze_mbd_2link_history.py" "${OUT_B}.history.csv" "${SUMMARY_B}"

python3 - <<'PY' "${SUMMARY_A}" "${SUMMARY_B}" \
    "${A_GROUND_DISP_TOL}" "${A_BODY1_DROP_MIN}" \
    "${B_GROUND_DISP_TOL}" "${B_THETA_SPAN_MIN}" "${B_RADIUS_DRIFT_TOL}" "${B_REVOLUTE_MISMATCH_TOL}"
import json
import sys
from pathlib import Path

summary_a = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
summary_b = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
a_ground_disp_tol = float(sys.argv[3])
a_body1_drop_min = float(sys.argv[4])
b_ground_disp_tol = float(sys.argv[5])
b_theta_span_min = float(sys.argv[6])
b_radius_drift_tol = float(sys.argv[7])
b_revolute_mismatch_tol = float(sys.argv[8])

radius_drift_b = (
    float(summary_b["revolute_body_j_com_radius_max"]) -
    float(summary_b["revolute_body_j_com_radius_min"])
)

checks = [
    (
        float(summary_a["body0_max_displacement_from_initial"]) <= a_ground_disp_tol,
        "A body0 max displacement",
        float(summary_a["body0_max_displacement_from_initial"]),
        a_ground_disp_tol,
    ),
    (
        float(summary_a["body1_final_minus_initial_y"]) <= -a_body1_drop_min,
        "A body1 final minus initial y",
        float(summary_a["body1_final_minus_initial_y"]),
        -a_body1_drop_min,
    ),
    (
        float(summary_b["body0_max_displacement_from_initial"]) <= b_ground_disp_tol,
        "B body0 max displacement",
        float(summary_b["body0_max_displacement_from_initial"]),
        b_ground_disp_tol,
    ),
    (
        float(summary_b["body1_theta_span"]) >= b_theta_span_min,
        "B body1 theta span",
        float(summary_b["body1_theta_span"]),
        b_theta_span_min,
    ),
    (
        radius_drift_b <= b_radius_drift_tol,
        "B body1 COM radius drift",
        radius_drift_b,
        b_radius_drift_tol,
    ),
    (
        float(summary_b["revolute_anchor_mismatch_max"]) <= b_revolute_mismatch_tol,
        "B revolute anchor mismatch max",
        float(summary_b["revolute_anchor_mismatch_max"]),
        b_revolute_mismatch_tol,
    ),
]

for ok, name, value, threshold in checks:
    status = "PASS" if ok else "FAIL"
    print(f"{status}: {name}: value={value:.16e} threshold={threshold:.16e}")

failed = [name for ok, name, _value, _threshold in checks if not ok]
if failed:
    raise SystemExit("FAIL: " + ", ".join(failed))

print("PASS: ground/revolute enforcement")
print(f"B radius drift value={radius_drift_b:.16e}")
PY

grep -q "route=multipart_manifest" "${PARSER_LOG}"
grep -q "Parser export completed successfully." "${PARSER_LOG}"
grep -q "Program completed successfully." "${LOG_A}"
grep -q "Program completed successfully." "${LOG_B}"

echo "PASS: mbd 2link ground/revolute enforcement"
echo "output_dir=${OUT_DIR}"
echo "thresholds=A(body0_disp<=${A_GROUND_DISP_TOL},body1_drop>=${A_BODY1_DROP_MIN}) B(body0_disp<=${B_GROUND_DISP_TOL},theta_span>=${B_THETA_SPAN_MIN},radius_drift<=${B_RADIUS_DRIFT_TOL},revolute_mismatch<=${B_REVOLUTE_MISMATCH_TOL})"
