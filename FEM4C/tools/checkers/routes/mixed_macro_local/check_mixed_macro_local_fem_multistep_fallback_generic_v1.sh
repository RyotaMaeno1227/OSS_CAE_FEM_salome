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
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_FEM_MULTISTEP_FALLBACK_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_fem_multistep_fallback_generic_v1}"
FEM_MULTISTEP_OUT_DIR="${OUT_DIR}/fem_multistep_route"
MIXED_LATERSTEP_OUT_DIR="${OUT_DIR}/mixed_laterstep_route"
MBD_TRACE_OUT_DIR="${OUT_DIR}/mbd_trace"
MIXED_MANIFEST="${OUT_DIR}/laterstep_fem_aggregation_manifest.json"
MBD_DECK="${ROOT_DIR}/examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat"
FEM_DECK="${ROOT_DIR}/examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_multistep_native.dat"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md" \
    "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md" \
    "${ROOT_DIR}/scripts/export_mixed_local_feedback_reduced_generic.py" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_oneway_generic_v1.sh" \
    "${ROOT_DIR}/scripts/check_fem_macro_local_multistep_static_generic_v1.sh" \
    "${ROOT_DIR}/scripts/check_mixed_macro_local_fem_multistep_generic_v1.sh" \
    "${MBD_DECK}" \
    "${FEM_DECK}"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_FEM_MACRO_LOCAL_MULTISTEP_STATIC_GENERIC_V1_OUTDIR="${FEM_MULTISTEP_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_local_multistep_static_generic_v1.sh"
env FEM4C_MIXED_MACRO_LOCAL_FEM_MULTISTEP_GENERIC_V1_OUTDIR="${MIXED_LATERSTEP_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_fem_multistep_generic_v1.sh"
env FEM4C_MBD_GENERIC_CONTACT_TRACE_OUTDIR="${MBD_TRACE_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_penalty_generic_trace_v1.sh"

python3 - "${MBD_TRACE_OUT_DIR}" "${FEM_MULTISTEP_OUT_DIR}" "${MIXED_MANIFEST}" <<'PY'
import csv
import json
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


mbd_out_dir = pathlib.Path(sys.argv[1]).resolve()
fem_multistep_out_dir = pathlib.Path(sys.argv[2]).resolve()
manifest_path = pathlib.Path(sys.argv[3]).resolve()

mbd_trace = mbd_out_dir / "generic_trace_active.out.contact_generic_trace.csv"
fem_trace = (
    fem_multistep_out_dir
    / "multistep_run"
    / "iter_run"
    / "baseline"
    / "baseline_generic.out.fem_contact_generic_trace.csv"
)

mbd_rows = list(csv.DictReader(mbd_trace.open(newline="", encoding="utf-8")))
fem_rows = list(csv.DictReader(fem_trace.open(newline="", encoding="utf-8")))

mbd_active = next((row for row in mbd_rows if int(float(row["active"])) == 1), None)
if mbd_active is None:
    fail("no active MBD trace row")

fem_step0 = next(
    (
        row for row in fem_rows
        if int(float(row["active_flag"])) == 1
        and int(float(row["load_step"])) == 0
        and int(float(row["slave_node_id"])) == 5
    ),
    None,
)
later_rows = [
    row for row in fem_rows
    if int(float(row["active_flag"])) == 1
    and int(float(row["load_step"])) > 0
    and int(float(row["pair_id"])) == 20
]
later_rows = sorted(
    later_rows,
    key=lambda row: (
        int(float(row["load_step"])),
        int(float(row["pair_id"])),
        int(float(row["slave_node_id"])),
        int(float(row["master_segment_id"])),
    ),
)
if fem_step0 is None:
    fail("no active FEM load_step=0 row")
if len(later_rows) < 2:
    fail("need at least two later-step FEM rows")

selected_later = later_rows[:2]
later_key = (
    int(float(selected_later[0]["load_step"])),
    int(float(selected_later[0]["pair_id"])),
)
for row in selected_later[1:]:
    if (int(float(row["load_step"])), int(float(row["pair_id"]))) != later_key:
        fail("selected later-step FEM rows must share reduced key")

sources = [
    {
        "source_engine": "MBD",
        "trace_csv": str(mbd_trace),
        "row_selector": {
            "step": int(float(mbd_active["step"])),
            "pair_id": int(float(mbd_active["pair_id"])),
            "slave_vertex_id": int(float(mbd_active["slave_vertex_id"])),
            "master_segment_id": int(float(mbd_active["master_segment_id"])),
        },
        "provenance": {
            "trace_contract": "mbd_macro_penalty_generic_trace_v1",
            "source_route": "mbd_macro_penalty_generic_trace_v1",
            "selection_reason": "first_active_row",
        },
    },
    {
        "source_engine": "FEM",
        "trace_csv": str(fem_trace),
        "row_selector": {
            "load_step": int(float(fem_step0["load_step"])),
            "pair_id": int(float(fem_step0["pair_id"])),
            "slave_node_id": int(float(fem_step0["slave_node_id"])),
            "master_segment_id": int(float(fem_step0["master_segment_id"])),
        },
        "provenance": {
            "trace_contract": "fem_macro_penalty_generic_trace_v1",
            "source_route": "fem_macro_local_multistep_static_generic_v1",
            "selection_reason": "baseline_load_step0_compare_row",
        },
    },
]

for idx, row in enumerate(selected_later, start=1):
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
                "source_route": "fem_macro_local_multistep_static_generic_v1",
                "selection_reason": f"later_step_aggregation_row{idx}",
            },
        }
    )

manifest = {
    "contract_version": "mixed_macro_local_generic_contract_v1",
    "schema_name": "mixed_macro_local_generic_manifest_v1",
    "job_id": "mixed_fem_multistep_later_step_aggregation_fallback_v1",
    "output_policy": {
        "request_contract_version": "local_patch_generic_contract_v1",
        "feedback_to_macro": False,
    },
    "sources": sources,
}
manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
PY

python3 "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" "${MIXED_MANIFEST}"

SCENARIOS=(A_all_valid_later_step_fem B_partial_invalid_later_step_fem C_all_invalid_later_step_fem)

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

later_step_entries = [
    entry for entry in entries
    if entry["source_engine"] == "FEM"
    and int(entry["source_trace_key"].get("load_step", 0)) > 0
]
if len(later_step_entries) < 2:
    raise SystemExit("FAIL: scenario requires at least two later-step FEM entries")

if scenario == "A_all_valid_later_step_fem":
    targets = []
    status_value = None
elif scenario == "B_partial_invalid_later_step_fem":
    targets = [later_step_entries[0]]
    status_value = "invalid_partial_later_step_fem"
elif scenario == "C_all_invalid_later_step_fem":
    targets = later_step_entries
    status_value = "invalid_all_later_step_fem"
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
import math
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


scenario = sys.argv[1]
oneway_dir = pathlib.Path(sys.argv[2]).resolve()
split_dir = pathlib.Path(sys.argv[3]).resolve()

oneway_summary = json.loads((oneway_dir / "mixed_oneway_summary.json").read_text(encoding="utf-8"))
reduced_manifest = json.loads((oneway_dir / "reduced_feedback" / "mixed_feedback_reduced_manifest.json").read_text(encoding="utf-8"))
reduced_summary = json.loads((oneway_dir / "reduced_feedback" / "mixed_feedback_reduced_summary.json").read_text(encoding="utf-8"))
split_handoff = json.loads((split_dir / "split_replay_handoff_manifest.json").read_text(encoding="utf-8"))
split_summary = json.loads((split_dir / "mixed_split_replay_summary.json").read_text(encoding="utf-8"))

if oneway_summary.get("source_engine_histogram", {}).get("MBD", 0) <= 0:
    fail(f"{scenario}: missing MBD source row")
if int(oneway_summary.get("later_step_fem_source_rows", 0)) < 2:
    fail(f"{scenario}: later_step_fem_source_rows must be >= 2")

if int(reduced_summary.get("fem_distinct_load_step_count", 0)) < 2:
    fail(f"{scenario}: fem_distinct_load_step_count must be >= 2")
if sorted(int(v) for v in reduced_summary.get("fem_load_steps", [])) != [0, 1]:
    fail(f"{scenario}: fem_load_steps must be [0, 1]")
if int(reduced_summary.get("later_step_fem_source_rows", 0)) < 2:
    fail(f"{scenario}: later_step_fem_source_rows must be >= 2")
if reduced_summary.get("overall_status") != "PASS":
    fail(f"{scenario}: reduced summary overall_status must be PASS")

manifest_rows = reduced_manifest.get("rows")
if not isinstance(manifest_rows, list):
    fail(f"{scenario}: reduced manifest rows missing")
later_step_row = next(
    (
        row for row in manifest_rows
        if row.get("source_engine") == "FEM"
        and isinstance(row.get("feedback_row_key"), dict)
        and int(row["feedback_row_key"].get("load_step", 0)) > 0
    ),
    None,
)
if later_step_row is None:
    fail(f"{scenario}: missing later-step FEM reduced manifest row")
if int(later_step_row.get("source_row_count", 0)) < 2:
    fail(f"{scenario}: later-step FEM source_row_count must be >= 2")

later_step_feedback_rows = [
    row for row in csv.DictReader((oneway_dir / "reduced_feedback" / "fem_local_feedback_reduced.csv").open(newline="", encoding="utf-8"))
    if int(row["load_step"]) > 0
]
if len(later_step_feedback_rows) != 1:
    fail(f"{scenario}: expected exactly one later-step FEM reduced feedback row")

later_step_replay_use_rows = [
    row for row in csv.DictReader((split_dir / "fem_replay" / "replay_generic.out.fem_contact_generic_replay_use.csv").open(newline="", encoding="utf-8"))
    if int(row["load_step"]) > 0
]
later_step_replay_trace_rows = [
    row for row in csv.DictReader((split_dir / "fem_replay" / "replay_generic.out.fem_contact_generic_trace.csv").open(newline="", encoding="utf-8"))
    if int(row["load_step"]) > 0
]
if not later_step_replay_use_rows:
    fail(f"{scenario}: missing later-step FEM replay-use rows")
if not later_step_replay_trace_rows:
    fail(f"{scenario}: missing later-step FEM replay-trace rows")

baseline_map = {
    (
        int(row["load_step"]),
        int(row["pair_id"]),
        int(row["slave_node_id"]),
        int(row["master_segment_id"]),
    ): row
    for row in csv.DictReader((split_dir / "fem_replay" / "baseline_generic.out.fem_contact_generic_trace.csv").open(newline="", encoding="utf-8"))
}

later_step_changed_rows = 0
for row in later_step_replay_trace_rows:
    key = (
        int(row["load_step"]),
        int(row["pair_id"]),
        int(row["slave_node_id"]),
        int(row["master_segment_id"]),
    )
    baseline_row = baseline_map.get(key)
    if baseline_row is None:
        fail(f"{scenario}: missing baseline row for {key!r}")
    if (
        abs(float(row["fn_n"]) - float(baseline_row["fn_n"])) > 1.0e-9
        or abs(float(row["penetration_m"]) - float(baseline_row["penetration_m"])) > 1.0e-12
    ):
        later_step_changed_rows += 1

provenance_link = split_handoff.get("provenance_link")
if not isinstance(provenance_link, dict):
    fail(f"{scenario}: missing provenance_link")
fem_links = provenance_link.get("fem_replay_links")
if not isinstance(fem_links, list) or not fem_links:
    fail(f"{scenario}: missing fem_replay_links")
target_links = [
    link for link in fem_links
    if isinstance(link, dict)
    and isinstance(link.get("reduced_feedback_row_key"), dict)
    and int(link["reduced_feedback_row_key"].get("load_step", 0)) > 0
]
if not target_links:
    fail(f"{scenario}: missing later-step FEM replay links")

if scenario == "A_all_valid_later_step_fem":
    if int(reduced_summary.get("later_step_fem_valid_source_rows", 0)) < 2:
        fail(f"{scenario}: later_step_fem_valid_source_rows must be >= 2")
    if int(reduced_summary.get("later_step_fem_invalid_source_rows", 0)) != 0:
        fail(f"{scenario}: later_step_fem_invalid_source_rows must be 0")
    if int(reduced_summary.get("later_step_fem_source_rows", 0)) <= int(reduced_summary.get("later_step_fem_feedback_rows", 0)):
        fail(f"{scenario}: source rows must be greater than reduced rows")
    if int(split_summary.get("later_step_fem_replay_changed_rows", 0)) <= 0:
        fail(f"{scenario}: later_step_fem_replay_changed_rows must be > 0")
    if later_step_changed_rows <= 0:
        fail(f"{scenario}: replay trace must differ from baseline")
    if int(split_summary.get("later_step_fem_fallback_rows", 0)) != 0:
        fail(f"{scenario}: later_step_fem_fallback_rows must be 0")
    if int(split_summary.get("later_step_fem_gamma_shift_rows", 0)) <= 0:
        fail(f"{scenario}: later_step_fem_gamma_shift_rows must be > 0")
    if int(split_summary.get("later_step_fem_k_shift_rows", 0)) <= 0:
        fail(f"{scenario}: later_step_fem_k_shift_rows must be > 0")
else:
    expected_invalid = 1 if scenario == "B_partial_invalid_later_step_fem" else 2
    expected_valid = 1 if scenario == "B_partial_invalid_later_step_fem" else 0
    if int(reduced_summary.get("later_step_fem_valid_source_rows", 0)) != expected_valid:
        fail(f"{scenario}: later_step_fem_valid_source_rows mismatch")
    if int(reduced_summary.get("later_step_fem_invalid_source_rows", 0)) != expected_invalid:
        fail(f"{scenario}: later_step_fem_invalid_source_rows mismatch")
    if int(later_step_row["exported_feedback"]["valid_flag"]) != 0:
        fail(f"{scenario}: later-step FEM reduced row must be invalid")
    if int(split_summary.get("later_step_fem_fallback_rows", 0)) <= 0:
        fail(f"{scenario}: later_step_fem_fallback_rows must be > 0")
    for row in later_step_replay_use_rows:
        gamma_n_used = float(row["gamma_n_used"])
        k_pen_base = float(row["k_pen_base"])
        k_pen_used = float(row["k_pen_used"])
        if not math.isfinite(gamma_n_used) or not math.isfinite(k_pen_base) or not math.isfinite(k_pen_used):
            fail(f"{scenario}: non-finite replay-use values")
        if abs(gamma_n_used - 1.0) > 1.0e-12:
            fail(f"{scenario}: fallback gamma_n_used must be 1.0")
        if abs(k_pen_used - k_pen_base) > 1.0e-9:
            fail(f"{scenario}: fallback k_pen_used must equal k_pen_base")

    if not all(bool(link.get("fallback_applied")) for link in target_links):
        fail(f"{scenario}: later-step FEM links must all indicate fallback_applied")

if split_summary.get("overall_status") != "PASS":
    fail(f"{scenario}: split summary overall_status must be PASS")
if split_handoff.get("overall_status", "PASS") not in {"PASS", None}:
    fail(f"{scenario}: unexpected handoff overall status")

print(
    f"PASS {scenario} "
    f"later_step_fem_source_rows={reduced_summary['later_step_fem_source_rows']} "
    f"later_step_fem_valid_source_rows={reduced_summary['later_step_fem_valid_source_rows']} "
    f"later_step_fem_invalid_source_rows={reduced_summary['later_step_fem_invalid_source_rows']} "
    f"later_step_fem_feedback_rows={reduced_summary['later_step_fem_feedback_rows']} "
    f"later_step_fem_fallback_rows={split_summary['later_step_fem_fallback_rows']} "
    f"later_step_fem_replay_changed_rows={split_summary['later_step_fem_replay_changed_rows']}"
)
PY
done

grep -q "later-step FEM aggregation/fallback hardening" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "later-step FEM aggregation/fallback hardening" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "current all-valid rule では partial-invalid later-step FEM reduced row も fallback に落ちる" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "current all-valid rule では partial-invalid later-step FEM reduced row も fallback に落ちる" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "joint mixed replay ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "mixed solve は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "live co-sim は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"

echo "PASS: mixed_macro_local_fem_multistep_fallback_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
