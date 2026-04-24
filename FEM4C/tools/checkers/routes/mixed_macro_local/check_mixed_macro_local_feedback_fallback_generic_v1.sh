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
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_FEEDBACK_FALLBACK_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_feedback_fallback_generic_v1}"
MBD_TRACE_OUT_DIR="${OUT_DIR}/mbd_trace"
FEM_TRACE_OUT_DIR="${OUT_DIR}/fem_trace"
MIXED_MANIFEST="${OUT_DIR}/multirow_mixed_manifest.json"
MBD_DECK="${ROOT_DIR}/examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat"
FEM_DECK="${ROOT_DIR}/examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md" \
    "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md" \
    "${ROOT_DIR}/scripts/export_mixed_local_feedback_reduced_generic.py" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_oneway_generic_v1.sh" \
    "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" \
    "${MBD_DECK}" \
    "${FEM_DECK}"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_MBD_GENERIC_CONTACT_TRACE_OUTDIR="${MBD_TRACE_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_penalty_generic_trace_v1.sh"
env FEM4C_FEM_GENERIC_CONTACT_TRACE_OUTDIR="${FEM_TRACE_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_penalty_generic_trace_v1.sh"

python3 - "${MBD_TRACE_OUT_DIR}" "${FEM_TRACE_OUT_DIR}" "${MIXED_MANIFEST}" <<'PY'
import csv
import json
import pathlib
import sys
from collections import defaultdict


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def pick_group(rows, key_fields, active_field):
    groups = defaultdict(list)
    for row in rows:
        if int(float(row[active_field])) != 1:
            continue
        key = tuple(int(float(row[field])) for field in key_fields)
        groups[key].append(row)
    for key in sorted(groups):
        if len(groups[key]) >= 2:
            return key, groups[key][:2]
    fail(f"no active group with at least two rows for key_fields={key_fields!r}")


mbd_out_dir = pathlib.Path(sys.argv[1]).resolve()
fem_out_dir = pathlib.Path(sys.argv[2]).resolve()
manifest_path = pathlib.Path(sys.argv[3]).resolve()

mbd_trace = mbd_out_dir / "generic_trace_active.out.contact_generic_trace.csv"
fem_trace = fem_out_dir / "fem_generic_trace_active.out.fem_contact_generic_trace.csv"

mbd_rows = list(csv.DictReader(mbd_trace.open(newline="", encoding="utf-8")))
fem_rows = list(csv.DictReader(fem_trace.open(newline="", encoding="utf-8")))

mbd_key, mbd_selected = pick_group(mbd_rows, ("step", "pair_id"), "active")
fem_key, fem_selected = pick_group(fem_rows, ("load_step", "pair_id"), "active_flag")

sources = []
for idx, row in enumerate(mbd_selected, start=1):
    sources.append(
        {
            "source_engine": "MBD",
            "trace_csv": str(mbd_trace),
            "row_selector": {
                "step": int(float(row["step"])),
                "pair_id": int(float(row["pair_id"])),
                "slave_vertex_id": int(float(row["slave_vertex_id"])),
                "master_segment_id": int(float(row["master_segment_id"])),
            },
            "provenance": {
                "trace_contract": "mbd_macro_penalty_generic_trace_v1",
                "source_route": "mbd_macro_penalty_generic_trace_v1",
                "selection_reason": f"fallback_group_{mbd_key[0]}_{mbd_key[1]}_row{idx}",
            },
        }
    )
for idx, row in enumerate(fem_selected, start=1):
    sources.append(
        {
            "source_engine": "FEM",
            "trace_csv": str(fem_trace),
            "row_selector": {
                "load_step": int(float(row["load_step"])),
                "pair_id": int(float(row["pair_id"])),
                "slave_node_id": int(float(row["slave_node_id"])),
                "master_segment_id": int(float(row["master_segment_id"])),
            },
            "provenance": {
                "trace_contract": "fem_macro_penalty_generic_trace_v1",
                "source_route": "fem_macro_penalty_generic_trace_v1",
                "selection_reason": f"fallback_group_{fem_key[0]}_{fem_key[1]}_row{idx}",
            },
        }
    )

manifest = {
    "contract_version": "mixed_macro_local_generic_contract_v1",
    "schema_name": "mixed_macro_local_generic_manifest_v1",
    "job_id": "mixed_feedback_fallback_multirow_case",
    "output_policy": {
        "request_contract_version": "local_patch_generic_contract_v1",
        "feedback_to_macro": False,
    },
    "sources": sources,
}
manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
PY

python3 "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" "${MIXED_MANIFEST}"

SCENARIOS=(A_partial_invalid_mbd B_partial_invalid_fem C_all_invalid_mbd D_all_invalid_fem)

for scenario in "${SCENARIOS[@]}"; do
    SCENARIO_DIR="${OUT_DIR}/${scenario}"
    ONEWAY_DIR="${SCENARIO_DIR}/oneway_run"
    SPLIT_DIR="${SCENARIO_DIR}/split_replay_run"

    bash "${ROOT_DIR}/scripts/run_mixed_macro_local_oneway_generic_v1.sh" "${MIXED_MANIFEST}" "${ONEWAY_DIR}"

    python3 - "${ONEWAY_DIR}" "${scenario}" <<'PY'
import json
import pathlib
import sys


oneway_dir = pathlib.Path(sys.argv[1]).resolve()
scenario = sys.argv[2]
provenance = json.loads((oneway_dir / "mixed_bundle" / "provenance_manifest.json").read_text(encoding="utf-8"))
entries = provenance["requests"]

mbd_entries = [entry for entry in entries if entry["source_engine"] == "MBD"]
fem_entries = [entry for entry in entries if entry["source_engine"] == "FEM"]

if len(mbd_entries) < 2 or len(fem_entries) < 2:
    raise SystemExit("FAIL: fallback scenarios require at least two MBD and two FEM rows")

if scenario == "A_partial_invalid_mbd":
    targets = [mbd_entries[0]]
    status_value = "invalid_partial_mbd"
elif scenario == "B_partial_invalid_fem":
    targets = [fem_entries[0]]
    status_value = "invalid_partial_fem"
elif scenario == "C_all_invalid_mbd":
    targets = mbd_entries
    status_value = "invalid_all_mbd"
elif scenario == "D_all_invalid_fem":
    targets = fem_entries
    status_value = "invalid_all_fem"
else:
    raise SystemExit(f"FAIL: unsupported scenario {scenario}")

for entry in targets:
    response_path = oneway_dir / "mixed_bundle" / entry["response_path"]
    payload = json.loads(response_path.read_text(encoding="utf-8"))
    payload["result"]["valid_flag"] = 0
    payload["result"]["status"] = status_value
    response_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

    python3 "${ROOT_DIR}/scripts/export_mixed_local_feedback_reduced_generic.py" "${ONEWAY_DIR}"
    bash "${ROOT_DIR}/scripts/run_mixed_macro_local_replay_split_generic_v1.sh" \
        "${ONEWAY_DIR}" \
        "${MBD_DECK}" \
        "${FEM_DECK}" \
        "${SPLIT_DIR}"

    python3 - "${scenario}" "${ONEWAY_DIR}" "${SPLIT_DIR}" <<'PY'
import csv
import json
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


scenario = sys.argv[1]
oneway_dir = pathlib.Path(sys.argv[2]).resolve()
split_dir = pathlib.Path(sys.argv[3]).resolve()

reduced_manifest = json.loads((oneway_dir / "reduced_feedback" / "mixed_feedback_reduced_manifest.json").read_text(encoding="utf-8"))
split_handoff = json.loads((split_dir / "split_replay_handoff_manifest.json").read_text(encoding="utf-8"))
split_summary = json.loads((split_dir / "mixed_split_replay_summary.json").read_text(encoding="utf-8"))

rows = reduced_manifest.get("rows")
if not isinstance(rows, list) or len(rows) != 2:
    fail("expected two reduced feedback rows")

engine_target = "MBD" if "mbd" in scenario.lower() else "FEM"
target_row = next((row for row in rows if row.get("source_engine") == engine_target), None)
other_row = next((row for row in rows if row.get("source_engine") != engine_target), None)
if target_row is None or other_row is None:
    fail("failed to resolve target and other reduced rows")

source_row_count = int(target_row.get("source_row_count", 0))
valid_source_row_count = int(target_row.get("valid_source_row_count", -1))
invalid_source_row_count = int(target_row.get("invalid_source_row_count", -1))
if source_row_count < 2:
    fail(f"{scenario}: target source_row_count must be >= 2")

if scenario.startswith("A_") or scenario.startswith("B_"):
    if valid_source_row_count != source_row_count - 1:
        fail(f"{scenario}: partial-invalid valid_source_row_count mismatch")
    if invalid_source_row_count != 1:
        fail(f"{scenario}: partial-invalid invalid_source_row_count mismatch")
else:
    if valid_source_row_count != 0:
        fail(f"{scenario}: all-invalid valid_source_row_count must be 0")
    if invalid_source_row_count != source_row_count:
        fail(f"{scenario}: all-invalid invalid_source_row_count mismatch")

if int(target_row["exported_feedback"]["valid_flag"]) != 0:
    fail(f"{scenario}: reduced feedback valid_flag must be 0 for target row")

provenance_link = split_handoff.get("provenance_link")
if not isinstance(provenance_link, dict):
    fail(f"{scenario}: missing provenance_link")
links_key = "mbd_replay_links" if engine_target == "MBD" else "fem_replay_links"
target_links = provenance_link.get(links_key)
if not isinstance(target_links, list) or not target_links:
    fail(f"{scenario}: missing replay links for {engine_target}")

matched_links = []
for link in target_links:
    row_key = link.get("reduced_feedback_row_key")
    if row_key == target_row.get("feedback_row_key"):
        matched_links.append(link)
if not matched_links:
    fail(f"{scenario}: no replay links matched target reduced key")

if not all(bool(link.get("fallback_applied")) for link in matched_links):
    fail(f"{scenario}: target replay links must all indicate fallback_applied")
if not all(int(link.get("source_row_count", 0)) == source_row_count for link in matched_links):
    fail(f"{scenario}: source_row_count mismatch in replay links")
if not all(int(link.get("valid_source_row_count", -1)) == valid_source_row_count for link in matched_links):
    fail(f"{scenario}: valid_source_row_count mismatch in replay links")
if not all(int(link.get("invalid_source_row_count", -1)) == invalid_source_row_count for link in matched_links):
    fail(f"{scenario}: invalid_source_row_count mismatch in replay links")

if engine_target == "MBD":
    replay_use_path = split_dir / "mbd_replay" / "replay_generic.out.contact_generic_replay_use.csv"
    rows_use = list(csv.DictReader(replay_use_path.open(newline="", encoding="utf-8")))
    fallback_rows = [
        row for row in rows_use
        if int(row["pair_id"]) == int(target_row["feedback_row_key"]["pair_id"])
        and int(row["step"]) == int(target_row["feedback_row_key"]["step"]) + 1
    ]
    if not fallback_rows:
        fail(f"{scenario}: missing MBD replay-use rows for target reduced key")
    for row in fallback_rows:
        if abs(float(row["gamma_n_used"]) - 1.0) > 1.0e-12:
            fail(f"{scenario}: MBD fallback gamma_n_used must be 1.0")
        if abs(float(row["k_pen_used"]) - float(row["k_pen_base"])) > 1.0e-9:
            fail(f"{scenario}: MBD fallback k_pen_used must equal k_pen_base")
else:
    replay_use_path = split_dir / "fem_replay" / "replay_generic.out.fem_contact_generic_replay_use.csv"
    rows_use = list(csv.DictReader(replay_use_path.open(newline="", encoding="utf-8")))
    fallback_rows = [
        row for row in rows_use
        if int(row["pair_id"]) == int(target_row["feedback_row_key"]["pair_id"])
        and int(row["load_step"]) == int(target_row["feedback_row_key"]["load_step"])
    ]
    if not fallback_rows:
        fail(f"{scenario}: missing FEM replay-use rows for target reduced key")
    for row in fallback_rows:
        if abs(float(row["gamma_n_used"]) - 1.0) > 1.0e-12:
            fail(f"{scenario}: FEM fallback gamma_n_used must be 1.0")
        if abs(float(row["k_pen_used"]) - float(row["k_pen_base"])) > 1.0e-9:
            fail(f"{scenario}: FEM fallback k_pen_used must equal k_pen_base")

if split_summary.get("overall_status") != "PASS":
    fail(f"{scenario}: split summary overall_status mismatch")
summary_fallback_key = "mbd_fallback_rows" if engine_target == "MBD" else "fem_fallback_rows"
if int(split_summary.get(summary_fallback_key, 0)) <= 0:
    fail(f"{scenario}: split summary {summary_fallback_key} must be > 0")

print(
    f"PASS {scenario} "
    f"engine={engine_target} source_row_count={source_row_count} "
    f"valid_source_row_count={valid_source_row_count} invalid_source_row_count={invalid_source_row_count}"
)
PY
done

grep -q "valid_flag=0" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "baseline / fallback semantics" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "MBD reduced key は \`(step, pair_id)\`" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "FEM reduced key は \`(load_step, pair_id)\`" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"

echo "PASS: mixed_macro_local_feedback_fallback_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
