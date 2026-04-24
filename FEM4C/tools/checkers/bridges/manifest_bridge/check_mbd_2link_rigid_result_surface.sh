#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SRC_BULK="${1:-${ROOT_DIR}/NastranBalkFile/MBD_2link.txt}"
OUT_DIR="${2:-${TMPDIR:-/tmp}/fem4c_mbd_2link_rigid_result_surface}"
PATCH_JSON="${3:-${ROOT_DIR}/examples/mbd_2link_bulk_rigid_ground0_manifest_patch.json}"

RUN_SCRIPT="${ROOT_DIR}/scripts/run_mbd_2link_rigid_from_bulk.sh"
SUMMARY_JSON="${OUT_DIR}/result_summary.json"
SUMMARY_CSV="${OUT_DIR}/result_summary.csv"
SUMMARY_TXT="${OUT_DIR}/result_summary.txt"
RUN_LOG="${OUT_DIR}/fem4c_run.log"

bash "${RUN_SCRIPT}" "${SRC_BULK}" "${OUT_DIR}" "${PATCH_JSON}" >/dev/null

for path in "${SUMMARY_JSON}" "${SUMMARY_CSV}" "${SUMMARY_TXT}" "${RUN_LOG}"; do
  if [[ ! -f "${path}" ]]; then
    echo "FAIL: missing result artifact: ${path}" >&2
    exit 1
  fi
done

grep -q '^rigid_2link_result_summary$' "${SUMMARY_TXT}"
grep -q '^program_completed_successfully=1$' "${SUMMARY_TXT}"
grep -q '^max_revolute_anchor_mismatch=' "${SUMMARY_TXT}"
grep -q 'Program completed successfully\.' "${RUN_LOG}"

python3 - <<'PY' "${SUMMARY_JSON}"
import json
from pathlib import Path
import sys

summary = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))

if not summary.get("program_completed_successfully", False):
    raise SystemExit("FAIL: program_completed_successfully=0")

bodies = {int(body["body_id"]): body for body in summary["bodies"]}
if 0 not in bodies or 1 not in bodies:
    raise SystemExit(f"FAIL: expected bodies 0 and 1, got {sorted(bodies)}")

body0 = bodies[0]
body1 = bodies[1]

if not body0.get("is_ground", False):
    raise SystemExit("FAIL: body0 must be ground in canonical case")
if not body0.get("stayed_fixed_like_ground", False):
    raise SystemExit("FAIL: body0 did not stay fixed-like ground")
if not body1.get("moved_from_initial", False):
    raise SystemExit("FAIL: body1 did not move from initial pose")

tolerances = summary["tolerances"]
max_mismatch = float(summary["max_revolute_anchor_mismatch"])
if max_mismatch > float(tolerances["revolute_anchor_mismatch"]):
    raise SystemExit(
        f"FAIL: max revolute anchor mismatch {max_mismatch} exceeds tolerance {tolerances['revolute_anchor_mismatch']}"
    )

print(f"body0_ground={int(body0['is_ground'])}")
print(f"body0_stayed_fixed_like_ground={int(body0['stayed_fixed_like_ground'])}")
print(f"body1_moved_from_initial={int(body1['moved_from_initial'])}")
print(f"max_revolute_anchor_mismatch={max_mismatch:.16e}")
PY

echo "PASS: mbd 2link rigid result surface"
echo "outdir=${OUT_DIR}"
