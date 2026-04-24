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
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_FEM_MULTISTEP_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_fem_multistep_generic_v1}"
FEM_MULTISTEP_OUT_DIR="${OUT_DIR}/fem_multistep_route"
MBD_TRACE_OUT_DIR="${OUT_DIR}/mbd_trace"
ONEWAY_OUT_DIR="${OUT_DIR}/oneway_run"
SPLIT_OUT_DIR="${OUT_DIR}/split_replay_run"
MANIFEST_PATH="${OUT_DIR}/mixed_manifest_fem_multistep_later_step.json"
PROVENANCE_PATH="${ONEWAY_OUT_DIR}/mixed_bundle/provenance_manifest.json"
ONEWAY_SUMMARY_PATH="${ONEWAY_OUT_DIR}/mixed_oneway_summary.json"
FEEDBACK_MANIFEST_PATH="${ONEWAY_OUT_DIR}/reduced_feedback/mixed_feedback_reduced_manifest.json"
FEEDBACK_SUMMARY_PATH="${ONEWAY_OUT_DIR}/reduced_feedback/mixed_feedback_reduced_summary.json"
FEM_FEEDBACK_CSV="${ONEWAY_OUT_DIR}/reduced_feedback/fem_local_feedback_reduced.csv"
SPLIT_HANDOFF_PATH="${SPLIT_OUT_DIR}/split_replay_handoff_manifest.json"
SPLIT_SUMMARY_PATH="${SPLIT_OUT_DIR}/mixed_split_replay_summary.json"
FEM_REPLAY_USE_CSV="${SPLIT_OUT_DIR}/fem_replay/replay_generic.out.fem_contact_generic_replay_use.csv"
FEM_REPLAY_TRACE_CSV="${SPLIT_OUT_DIR}/fem_replay/replay_generic.out.fem_contact_generic_trace.csv"
FEM_BASELINE_TRACE_CSV="${SPLIT_OUT_DIR}/fem_replay/baseline_generic.out.fem_contact_generic_trace.csv"
MBD_DECK="${ROOT_DIR}/examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat"
FEM_DECK="${ROOT_DIR}/examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_multistep_native.dat"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md" \
    "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_oneway_generic_v1.sh" \
    "${ROOT_DIR}/scripts/export_mixed_local_feedback_reduced_generic.py" \
    "${ROOT_DIR}/scripts/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${ROOT_DIR}/scripts/check_fem_macro_local_multistep_static_generic_v1.sh" \
    "${MBD_DECK}" \
    "${FEM_DECK}"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_FEM_MACRO_LOCAL_MULTISTEP_STATIC_GENERIC_V1_OUTDIR="${FEM_MULTISTEP_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_local_multistep_static_generic_v1.sh"
env FEM4C_MBD_GENERIC_CONTACT_TRACE_OUTDIR="${MBD_TRACE_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_penalty_generic_trace_v1.sh"

python3 - "${MBD_TRACE_OUT_DIR}" "${FEM_MULTISTEP_OUT_DIR}" "${MANIFEST_PATH}" <<'PY'
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
    fail("no active MBD row")

fem_step0 = next(
    (
        row
        for row in fem_rows
        if int(float(row["active_flag"])) == 1
        and int(float(row["load_step"])) == 0
        and int(float(row["slave_node_id"])) == 5
    ),
    None,
)
fem_later = next(
    (
        row
        for row in fem_rows
        if int(float(row["active_flag"])) == 1
        and int(float(row["load_step"])) > 0
        and int(float(row["slave_node_id"])) == 5
    ),
    None,
)
if fem_step0 is None:
    fail("no active FEM load_step=0 row for slave_node_id=5")
if fem_later is None:
    fail("no active FEM later-step row for slave_node_id=5")

manifest = {
    "contract_version": "mixed_macro_local_generic_contract_v1",
    "schema_name": "mixed_macro_local_generic_manifest_v1",
    "job_id": "mixed_fem_multistep_later_step_native_v1",
    "output_policy": {
        "request_contract_version": "local_patch_generic_contract_v1",
        "feedback_to_macro": False,
    },
    "sources": [
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
                "selection_reason": "first_active_load_step0_row",
            },
        },
        {
            "source_engine": "FEM",
            "trace_csv": str(fem_trace),
            "row_selector": {
                "load_step": int(float(fem_later["load_step"])),
                "pair_id": int(float(fem_later["pair_id"])),
                "slave_node_id": int(float(fem_later["slave_node_id"])),
                "master_segment_id": int(float(fem_later["master_segment_id"])),
            },
            "provenance": {
                "trace_contract": "fem_macro_penalty_generic_trace_v1",
                "source_route": "fem_macro_local_multistep_static_generic_v1",
                "selection_reason": "first_active_later_step_row",
            },
        },
    ],
}
manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
PY

python3 "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" "${MANIFEST_PATH}"

bash "${ROOT_DIR}/scripts/run_mixed_macro_local_oneway_generic_v1.sh" "${MANIFEST_PATH}" "${ONEWAY_OUT_DIR}"
python3 "${ROOT_DIR}/scripts/export_mixed_local_feedback_reduced_generic.py" "${ONEWAY_OUT_DIR}"

bash "${ROOT_DIR}/scripts/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${ONEWAY_OUT_DIR}" \
    "${MBD_DECK}" \
    "${FEM_DECK}" \
    "${SPLIT_OUT_DIR}"

for path in \
    "${MANIFEST_PATH}" \
    "${PROVENANCE_PATH}" \
    "${ONEWAY_SUMMARY_PATH}" \
    "${FEEDBACK_MANIFEST_PATH}" \
    "${FEEDBACK_SUMMARY_PATH}" \
    "${FEM_FEEDBACK_CSV}" \
    "${SPLIT_HANDOFF_PATH}" \
    "${SPLIT_SUMMARY_PATH}" \
    "${FEM_REPLAY_USE_CSV}" \
    "${FEM_REPLAY_TRACE_CSV}" \
    "${FEM_BASELINE_TRACE_CSV}"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing expected artifact ${path}" >&2; exit 1; }
done

python3 - "${MANIFEST_PATH}" "${PROVENANCE_PATH}" "${ONEWAY_SUMMARY_PATH}" "${FEEDBACK_MANIFEST_PATH}" "${FEEDBACK_SUMMARY_PATH}" "${FEM_FEEDBACK_CSV}" "${SPLIT_HANDOFF_PATH}" "${SPLIT_SUMMARY_PATH}" "${FEM_REPLAY_USE_CSV}" "${FEM_REPLAY_TRACE_CSV}" "${FEM_BASELINE_TRACE_CSV}" <<'PY'
import csv
import json
import math
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


manifest_path = pathlib.Path(sys.argv[1]).resolve()
provenance_path = pathlib.Path(sys.argv[2]).resolve()
oneway_summary_path = pathlib.Path(sys.argv[3]).resolve()
feedback_manifest_path = pathlib.Path(sys.argv[4]).resolve()
feedback_summary_path = pathlib.Path(sys.argv[5]).resolve()
fem_feedback_csv_path = pathlib.Path(sys.argv[6]).resolve()
split_handoff_path = pathlib.Path(sys.argv[7]).resolve()
split_summary_path = pathlib.Path(sys.argv[8]).resolve()
fem_replay_use_csv_path = pathlib.Path(sys.argv[9]).resolve()
fem_replay_trace_csv_path = pathlib.Path(sys.argv[10]).resolve()
fem_baseline_trace_csv_path = pathlib.Path(sys.argv[11]).resolve()

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
oneway_summary = json.loads(oneway_summary_path.read_text(encoding="utf-8"))
feedback_manifest = json.loads(feedback_manifest_path.read_text(encoding="utf-8"))
feedback_summary = json.loads(feedback_summary_path.read_text(encoding="utf-8"))
split_handoff = json.loads(split_handoff_path.read_text(encoding="utf-8"))
split_summary = json.loads(split_summary_path.read_text(encoding="utf-8"))

sources = manifest.get("sources")
if not isinstance(sources, list) or len(sources) < 3:
    fail("mixed manifest must contain at least 3 sources")
if not any(source.get("source_engine") == "MBD" for source in sources):
    fail("mixed manifest missing MBD source")
fem_manifest_load_steps = sorted(
    {
        int(source["row_selector"]["load_step"])
        for source in sources
        if source.get("source_engine") == "FEM"
        and isinstance(source.get("row_selector"), dict)
    }
)
if fem_manifest_load_steps != [0, 1]:
    fail(f"expected FEM manifest load_steps [0, 1], got {fem_manifest_load_steps!r}")

provenance_entries = provenance.get("requests")
if not isinstance(provenance_entries, list) or len(provenance_entries) < 3:
    fail("provenance manifest missing requests")
engines = {entry.get("source_engine") for entry in provenance_entries if isinstance(entry, dict)}
if engines != {"MBD", "FEM"}:
    fail(f"expected MBD and FEM provenance engines, got {engines!r}")
fem_provenance_load_steps = sorted(
    {
        int(entry["source_trace_key"]["load_step"])
        for entry in provenance_entries
        if isinstance(entry, dict)
        and entry.get("source_engine") == "FEM"
        and isinstance(entry.get("source_trace_key"), dict)
        and "load_step" in entry["source_trace_key"]
    }
)
if fem_provenance_load_steps != [0, 1]:
    fail(f"expected FEM provenance load_steps [0, 1], got {fem_provenance_load_steps!r}")
later_step_provenance = [
    entry
    for entry in provenance_entries
    if isinstance(entry, dict)
    and entry.get("source_engine") == "FEM"
    and isinstance(entry.get("source_trace_key"), dict)
    and int(entry["source_trace_key"].get("load_step", 0)) > 0
]
if not later_step_provenance:
    fail("missing later-step FEM provenance entry")

for entry in later_step_provenance:
    request_path = provenance_path.parent / entry["request_path"]
    request = json.loads(request_path.read_text(encoding="utf-8"))
    if int(request["key"]["step"]) <= 0:
        fail(f"later-step FEM request step not preserved: {request_path}")
    if int(request["macro_trace_metadata"]["load_step"]) <= 0:
        fail(f"later-step FEM request load_step not preserved: {request_path}")
    if int(request["mixed_macro_source_provenance"]["source_trace_key"]["load_step"]) <= 0:
        fail(f"later-step FEM mixed provenance not preserved in request: {request_path}")

if oneway_summary.get("source_engine_histogram", {}).get("MBD", 0) <= 0:
    fail("oneway summary missing MBD source rows")
if oneway_summary.get("source_engine_histogram", {}).get("FEM", 0) < 2:
    fail("oneway summary must contain at least 2 FEM source rows")
if int(oneway_summary.get("fem_distinct_load_step_count", 0)) < 2:
    fail("oneway summary fem_distinct_load_step_count must be >= 2")
if sorted(int(v) for v in oneway_summary.get("fem_load_steps", [])) != [0, 1]:
    fail("oneway summary fem_load_steps must be [0, 1]")
if int(oneway_summary.get("later_step_fem_source_rows", 0)) <= 0:
    fail("oneway summary later_step_fem_source_rows must be > 0")

feedback_rows = list(csv.DictReader(fem_feedback_csv_path.open(newline="", encoding="utf-8")))
later_step_feedback_rows = [row for row in feedback_rows if int(row["load_step"]) > 0]
if not later_step_feedback_rows:
    fail("missing later-step FEM reduced feedback rows")
if int(feedback_summary.get("fem_distinct_load_step_count", 0)) < 2:
    fail("feedback summary fem_distinct_load_step_count must be >= 2")
if sorted(int(v) for v in feedback_summary.get("fem_load_steps", [])) != [0, 1]:
    fail("feedback summary fem_load_steps must be [0, 1]")
if int(feedback_summary.get("later_step_fem_feedback_rows", 0)) <= 0:
    fail("feedback summary later_step_fem_feedback_rows must be > 0")

feedback_manifest_rows = feedback_manifest.get("rows")
if not isinstance(feedback_manifest_rows, list):
    fail("feedback manifest rows missing")
later_step_feedback_manifest_rows = [
    row
    for row in feedback_manifest_rows
    if isinstance(row, dict)
    and row.get("source_engine") == "FEM"
    and isinstance(row.get("feedback_row_key"), dict)
    and int(row["feedback_row_key"].get("load_step", 0)) > 0
]
if not later_step_feedback_manifest_rows:
    fail("feedback manifest missing later-step FEM row")

fem_replay_use_rows = list(csv.DictReader(fem_replay_use_csv_path.open(newline="", encoding="utf-8")))
fem_replay_trace_rows = list(csv.DictReader(fem_replay_trace_csv_path.open(newline="", encoding="utf-8")))
fem_baseline_trace_rows = list(csv.DictReader(fem_baseline_trace_csv_path.open(newline="", encoding="utf-8")))
later_step_replay_use_rows = [row for row in fem_replay_use_rows if int(row["load_step"]) > 0]
later_step_replay_trace_rows = [row for row in fem_replay_trace_rows if int(row["load_step"]) > 0]
if not later_step_replay_use_rows:
    fail("missing later-step FEM replay-use rows")
if not later_step_replay_trace_rows:
    fail("missing later-step FEM replay-trace rows")

later_step_gamma_shift_rows = 0
later_step_k_shift_rows = 0
for row in later_step_replay_use_rows:
    gamma_n_used = float(row["gamma_n_used"])
    k_pen_base = float(row["k_pen_base"])
    k_pen_used = float(row["k_pen_used"])
    if not all(math.isfinite(v) for v in (gamma_n_used, k_pen_base, k_pen_used)):
        fail("later-step FEM replay-use has non-finite values")
    if abs(gamma_n_used - 1.0) > 1.0e-12:
        later_step_gamma_shift_rows += 1
    if abs(k_pen_used - k_pen_base) > 1.0e-9:
        later_step_k_shift_rows += 1
if later_step_gamma_shift_rows <= 0:
    fail("no later-step FEM replay-use row has gamma_n_used != 1.0")
if later_step_k_shift_rows <= 0:
    fail("no later-step FEM replay-use row has k_pen_used != k_pen_base")

baseline_map = {
    (
        int(row["load_step"]),
        int(row["pair_id"]),
        int(row["slave_node_id"]),
        int(row["master_segment_id"]),
    ): row
    for row in fem_baseline_trace_rows
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
        fail(f"missing baseline FEM row for key {key!r}")
    fn_diff = abs(float(row["fn_n"]) - float(baseline_row["fn_n"]))
    penetration_diff = abs(float(row["penetration_m"]) - float(baseline_row["penetration_m"]))
    if fn_diff > 1.0e-9 or penetration_diff > 1.0e-12:
        later_step_changed_rows += 1
if later_step_changed_rows <= 0:
    fail("later-step FEM replay trace does not differ from baseline")

if int(split_summary.get("fem_distinct_load_step_count", 0)) < 2:
    fail("split summary fem_distinct_load_step_count must be >= 2")
if sorted(int(v) for v in split_summary.get("fem_load_steps", [])) != [0, 1]:
    fail("split summary fem_load_steps must be [0, 1]")
if int(split_summary.get("later_step_fem_feedback_rows", 0)) <= 0:
    fail("split summary later_step_fem_feedback_rows must be > 0")
if int(split_summary.get("later_step_fem_replay_use_rows", 0)) <= 0:
    fail("split summary later_step_fem_replay_use_rows must be > 0")
if int(split_summary.get("later_step_fem_replay_trace_rows", 0)) <= 0:
    fail("split summary later_step_fem_replay_trace_rows must be > 0")
if int(split_summary.get("later_step_fem_replay_changed_rows", 0)) <= 0:
    fail("split summary later_step_fem_replay_changed_rows must be > 0")
if split_summary.get("overall_status") != "PASS":
    fail("split summary overall_status must be PASS")

if int(split_handoff.get("fem_distinct_load_step_count", 0)) < 2:
    fail("split handoff fem_distinct_load_step_count must be >= 2")
if sorted(int(v) for v in split_handoff.get("fem_load_steps", [])) != [0, 1]:
    fail("split handoff fem_load_steps must be [0, 1]")
if int(split_handoff.get("later_step_fem_feedback_rows", 0)) <= 0:
    fail("split handoff later_step_fem_feedback_rows must be > 0")
if int(split_handoff.get("later_step_fem_replay_use_rows", 0)) <= 0:
    fail("split handoff later_step_fem_replay_use_rows must be > 0")
if int(split_handoff.get("later_step_fem_replay_trace_rows", 0)) <= 0:
    fail("split handoff later_step_fem_replay_trace_rows must be > 0")
if int(split_handoff.get("later_step_fem_replay_changed_rows", 0)) <= 0:
    fail("split handoff later_step_fem_replay_changed_rows must be > 0")
if int(split_handoff.get("source_engine_histogram", {}).get("MBD", 0)) <= 0:
    fail("split handoff missing MBD histogram")

print(
    "PASS mixed_macro_local_fem_multistep_generic_v1 "
    f"fem_load_steps={split_summary['fem_load_steps']} "
    f"later_step_fem_feedback_rows={split_summary['later_step_fem_feedback_rows']} "
    f"later_step_fem_replay_use_rows={split_summary['later_step_fem_replay_use_rows']} "
    f"later_step_fem_replay_trace_rows={split_summary['later_step_fem_replay_trace_rows']} "
    f"later_step_fem_replay_changed_rows={split_summary['later_step_fem_replay_changed_rows']}"
)
PY

grep -q "later-step FEM aggregation/fallback hardening" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "later-step FEM aggregation/fallback hardening" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "joint mixed replay ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "mixed solve は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "live co-sim は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "true lagged mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "same-time mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/docs/mixed_macro_local_feedback_generic_v1.md"

echo "PASS: mixed_macro_local_fem_multistep_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
