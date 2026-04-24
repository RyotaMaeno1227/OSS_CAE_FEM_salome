#!/usr/bin/env bash
set -euo pipefail

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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(resolve_workspace_root "${SCRIPT_DIR}")"
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_REPLAY_SPLIT_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_replay_split_generic_v1}"
CONTRACT_OUT_DIR="${OUT_DIR}/contract_route"
ONEWAY_OUT_DIR="${OUT_DIR}/oneway_route"
FEEDBACK_OUT_DIR="${OUT_DIR}/feedback_route"
RUN_DIR="${OUT_DIR}/split_replay_run"
MANIFEST_PATH="${CONTRACT_OUT_DIR}/tiny_mixed_manifest.json"
MBD_DECK="${ROOT_DIR}/examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat"
FEM_DECK="${ROOT_DIR}/examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat"
HANDOFF_MANIFEST="${RUN_DIR}/split_replay_handoff_manifest.json"
SUMMARY_JSON="${RUN_DIR}/mixed_split_replay_summary.json"
SUMMARY_MD="${RUN_DIR}/mixed_split_replay_summary.md"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md" \
    "${ROOT_DIR}/tools/runners/mixed_bounded/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${ROOT_DIR}/tools/checkers/mixed_bounded/check_mixed_macro_local_feedback_generic_v1.sh" \
    "${MBD_DECK}" \
    "${FEM_DECK}"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_MIXED_MACRO_LOCAL_GENERIC_CONTRACT_V1_OUTDIR="${CONTRACT_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_generic_contract_v1.sh"
env FEM4C_MIXED_MACRO_LOCAL_ONEWAY_GENERIC_V1_OUTDIR="${ONEWAY_OUT_DIR}" \
    bash "${ROOT_DIR}/tools/checkers/mixed_bounded/check_mixed_macro_local_oneway_generic_v1.sh"
env FEM4C_MIXED_MACRO_LOCAL_FEEDBACK_GENERIC_V1_OUTDIR="${FEEDBACK_OUT_DIR}" \
    bash "${ROOT_DIR}/tools/checkers/mixed_bounded/check_mixed_macro_local_feedback_generic_v1.sh"

[[ -f "${MANIFEST_PATH}" ]] || { echo "FAIL: missing mixed manifest ${MANIFEST_PATH}" >&2; exit 1; }

bash "${ROOT_DIR}/tools/runners/mixed_bounded/run_mixed_macro_local_replay_split_generic_v1.sh" \
    "${MANIFEST_PATH}" \
    "${MBD_DECK}" \
    "${FEM_DECK}" \
    "${RUN_DIR}"

[[ -f "${HANDOFF_MANIFEST}" ]] || { echo "FAIL: missing handoff manifest ${HANDOFF_MANIFEST}" >&2; exit 1; }
[[ -f "${SUMMARY_JSON}" ]] || { echo "FAIL: missing summary JSON ${SUMMARY_JSON}" >&2; exit 1; }
[[ -f "${SUMMARY_MD}" ]] || { echo "FAIL: missing summary MD ${SUMMARY_MD}" >&2; exit 1; }

python3 - "${HANDOFF_MANIFEST}" "${SUMMARY_JSON}" "${RUN_DIR}" <<'PY'
import csv
import json
import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


handoff_path = pathlib.Path(sys.argv[1]).resolve()
summary_path = pathlib.Path(sys.argv[2]).resolve()
run_dir = pathlib.Path(sys.argv[3]).resolve()

handoff = json.loads(handoff_path.read_text(encoding="utf-8"))
summary = json.loads(summary_path.read_text(encoding="utf-8"))

if summary.get("pipeline_mode") != "mixed_split_replay_consume_mvp":
    fail("summary pipeline_mode mismatch")
if summary.get("mixed_replay") is not False:
    fail("summary mixed_replay must be false")
if summary.get("mixed_solve") is not False:
    fail("summary mixed_solve must be false")
if summary.get("live_cosim") is not False:
    fail("summary live_cosim must be false")
if summary.get("joint_coupling") is not False:
    fail("summary joint_coupling must be false")
if summary.get("mbd_feedback_source") != "EXTERNAL":
    fail("summary mbd_feedback_source mismatch")
if summary.get("fem_feedback_source") != "EXTERNAL":
    fail("summary fem_feedback_source mismatch")
if summary.get("feedback_available_flag") is not True:
    fail("summary feedback_available_flag must be true")
if summary.get("skip_reason") != "":
    fail("summary skip_reason must be empty when feedback rows exist")
if int(summary.get("mbd_replay_changed_rows", 0)) <= 0:
    fail("summary mbd_replay_changed_rows must be > 0")
if int(summary.get("fem_replay_changed_rows", 0)) <= 0:
    fail("summary fem_replay_changed_rows must be > 0")

engine_histogram = handoff.get("source_engine_histogram")
if not isinstance(engine_histogram, dict):
    fail("handoff source_engine_histogram missing")
if int(engine_histogram.get("MBD", 0)) <= 0:
    fail("handoff missing MBD source rows")
if int(engine_histogram.get("FEM", 0)) <= 0:
    fail("handoff missing FEM source rows")

for field in ("mbd_feedback_csv_path", "fem_feedback_csv_path", "mbd_replay_outdir", "fem_replay_outdir"):
    value = handoff.get(field)
    if not isinstance(value, str) or not value:
        fail(f"handoff missing {field}")
    path = pathlib.Path(value)
    if field.endswith("_outdir"):
        if not path.is_dir():
            fail(f"handoff outdir missing on disk: {path}")
    else:
        if not path.is_file():
            fail(f"handoff csv missing on disk: {path}")

provenance_link = handoff.get("provenance_link")
if not isinstance(provenance_link, dict):
    fail("handoff missing provenance_link")
mixed_feedback_rows = provenance_link.get("mixed_feedback_rows")
if not isinstance(mixed_feedback_rows, list) or not mixed_feedback_rows:
    fail("handoff mixed_feedback_rows missing")
if handoff.get("feedback_available_flag") is not True:
    fail("handoff feedback_available_flag must be true")
if handoff.get("skip_reason") != "":
    fail("handoff skip_reason must be empty")
if int(handoff.get("feedback_row_count", -1)) != len(mixed_feedback_rows):
    fail("handoff feedback_row_count mismatch")
mbd_replay_links = provenance_link.get("mbd_replay_links")
fem_replay_links = provenance_link.get("fem_replay_links")
if not isinstance(mbd_replay_links, list) or not mbd_replay_links:
    fail("handoff missing mbd replay links")
if not isinstance(fem_replay_links, list) or not fem_replay_links:
    fail("handoff missing fem replay links")

mbd_replay_use = pathlib.Path(handoff["mbd_replay_outdir"]) / "replay_generic.out.contact_generic_replay_use.csv"
fem_replay_use = pathlib.Path(handoff["fem_replay_outdir"]) / "replay_generic.out.fem_contact_generic_replay_use.csv"
if not mbd_replay_use.is_file():
    fail("missing MBD replay-use CSV")
if not fem_replay_use.is_file():
    fail("missing FEM replay-use CSV")

mbd_rows = list(csv.DictReader(mbd_replay_use.open(newline="", encoding="utf-8")))
fem_rows = list(csv.DictReader(fem_replay_use.open(newline="", encoding="utf-8")))
if not any(row["feedback_source"] == "EXTERNAL" for row in mbd_rows):
    fail("MBD replay-use has no EXTERNAL rows")
if not any(row["feedback_source"] == "EXTERNAL" for row in fem_rows):
    fail("FEM replay-use has no EXTERNAL rows")
expected_feedback_apply_count = sum(1 for row in mbd_rows if row["feedback_source"] == "EXTERNAL") + sum(
    1 for row in fem_rows if row["feedback_source"] == "EXTERNAL"
)
if int(summary.get("feedback_row_count", -1)) != len(mixed_feedback_rows):
    fail("summary feedback_row_count mismatch")
if int(summary.get("feedback_apply_count", -1)) != expected_feedback_apply_count:
    fail("summary feedback_apply_count mismatch")
if int(handoff.get("feedback_apply_count", -1)) != expected_feedback_apply_count:
    fail("handoff feedback_apply_count mismatch")

mbd_applied_keys = {
    (int(row["step"]) - 1, int(row["pair_id"]))
    for row in mbd_rows
    if row["feedback_source"] == "EXTERNAL"
}
fem_applied_keys = {
    (int(row["load_step"]), int(row["pair_id"]))
    for row in fem_rows
    if row["feedback_source"] == "EXTERNAL"
}
last_available_ref = None
last_applied_ref = None
for manifest_row in mixed_feedback_rows:
    if not isinstance(manifest_row, dict):
        fail("mixed_feedback_rows entry must be an object")
    feedback_row_key = manifest_row.get("feedback_row_key")
    feedback_csv = manifest_row.get("feedback_csv")
    exported_feedback = manifest_row.get("exported_feedback")
    source_engine = manifest_row.get("source_engine")
    if not isinstance(feedback_row_key, dict):
        fail("mixed_feedback_rows entry missing feedback_row_key")
    if not isinstance(feedback_csv, str) or not feedback_csv:
        fail("mixed_feedback_rows entry missing feedback_csv")
    if not isinstance(exported_feedback, dict):
        fail("mixed_feedback_rows entry missing exported_feedback")
    if source_engine not in {"MBD", "FEM"}:
        fail("mixed_feedback_rows entry missing source_engine")
    ref = {
        "source_engine": source_engine,
        "feedback_csv": feedback_csv,
        "feedback_row_key": feedback_row_key,
        "feedback_valid_flag": int(exported_feedback["valid_flag"]),
        "feedback_status": str(exported_feedback["status"]),
    }
    last_available_ref = ref
    if source_engine == "MBD":
        feedback_key = (int(feedback_row_key["step"]), int(feedback_row_key["pair_id"]))
        if feedback_key in mbd_applied_keys:
            last_applied_ref = ref
    else:
        feedback_key = (int(feedback_row_key["load_step"]), int(feedback_row_key["pair_id"]))
        if feedback_key in fem_applied_keys:
            last_applied_ref = ref

if summary.get("last_available_feedback_ref") != last_available_ref:
    fail("summary last_available_feedback_ref mismatch")
if handoff.get("last_available_feedback_ref") != last_available_ref:
    fail("handoff last_available_feedback_ref mismatch")
if summary.get("last_applied_feedback_ref") != last_applied_ref:
    fail("summary last_applied_feedback_ref mismatch")
if handoff.get("last_applied_feedback_ref") != last_applied_ref:
    fail("handoff last_applied_feedback_ref mismatch")
expected_last_available_differs = last_available_ref != last_applied_ref
if summary.get("last_available_differs_from_last_applied") != expected_last_available_differs:
    fail("summary last_available_differs_from_last_applied mismatch")
if handoff.get("last_available_differs_from_last_applied") != expected_last_available_differs:
    fail("handoff last_available_differs_from_last_applied mismatch")
if summary.get("unapplied_terminal_candidate_flag") != expected_last_available_differs:
    fail("summary unapplied_terminal_candidate_flag mismatch")
if handoff.get("unapplied_terminal_candidate_flag") != expected_last_available_differs:
    fail("handoff unapplied_terminal_candidate_flag mismatch")

print(
    "PASS mixed_macro_local_replay_split_generic_v1 "
    f"mbd_replay_rows={len(mbd_rows)} fem_replay_rows={len(fem_rows)}"
)
PY

grep -q "joint mixed replay ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "mixed solve は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "live co-sim は未実装" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "true lagged mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "same-time mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "feedback_available_flag" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "feedback_apply_count" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "last_available_feedback_ref" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "last_applied_feedback_ref" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "post-solve artifact" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "friction solver state" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "source_step" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"
grep -q "dt_comm" "${ROOT_DIR}/docs/mixed_macro_local_replay_split_generic_v1.md"

echo "PASS: mixed_macro_local_replay_split_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
