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
OUT_DIR="${FEM4C_MIXED_MACRO_LOCAL_ONEWAY_GENERIC_V1_OUTDIR:-${TMPDIR:-/tmp}/fem4c_mixed_macro_local_oneway_generic_v1}"
MBD_OUT_DIR="${OUT_DIR}/mbd_trace"
FEM_OUT_DIR="${OUT_DIR}/fem_trace"
CONTRACT_OUT_DIR="${OUT_DIR}/contract_route"
RUN_DIR="${OUT_DIR}/oneway_run"
MANIFEST_PATH="${CONTRACT_OUT_DIR}/tiny_mixed_manifest.json"
MIXED_BUNDLE_DIR="${RUN_DIR}/mixed_bundle"
PROVENANCE_PATH="${MIXED_BUNDLE_DIR}/provenance_manifest.json"
REQUEST_SUMMARY_PATH="${MIXED_BUNDLE_DIR}/request_build_summary.json"
RESPONSE_SUMMARY_PATH="${MIXED_BUNDLE_DIR}/response_build_summary.json"
SUMMARY_JSON_PATH="${RUN_DIR}/mixed_oneway_summary.json"
SUMMARY_MD_PATH="${RUN_DIR}/mixed_oneway_summary.md"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

for path in \
    "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md" \
    "${ROOT_DIR}/tools/runners/mixed_bounded/run_mixed_macro_local_oneway_generic_v1.sh" \
    "${ROOT_DIR}/scripts/check_mixed_macro_local_generic_contract_v1.sh"; do
    [[ -f "${path}" ]] || { echo "FAIL: missing required file ${path}" >&2; exit 1; }
done

env FEM4C_MBD_GENERIC_CONTACT_TRACE_OUTDIR="${MBD_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mbd_macro_penalty_generic_trace_v1.sh"
env FEM4C_FEM_GENERIC_CONTACT_TRACE_OUTDIR="${FEM_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_fem_macro_penalty_generic_trace_v1.sh"
env FEM4C_MIXED_MACRO_LOCAL_GENERIC_CONTRACT_V1_OUTDIR="${CONTRACT_OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_generic_contract_v1.sh"

[[ -f "${MANIFEST_PATH}" ]] || { echo "FAIL: missing manifest ${MANIFEST_PATH}" >&2; exit 1; }

bash "${ROOT_DIR}/tools/runners/mixed_bounded/run_mixed_macro_local_oneway_generic_v1.sh" "${MANIFEST_PATH}" "${RUN_DIR}"

[[ -f "${PROVENANCE_PATH}" ]] || { echo "FAIL: missing provenance manifest ${PROVENANCE_PATH}" >&2; exit 1; }
[[ -f "${REQUEST_SUMMARY_PATH}" ]] || { echo "FAIL: missing request build summary ${REQUEST_SUMMARY_PATH}" >&2; exit 1; }
[[ -f "${RESPONSE_SUMMARY_PATH}" ]] || { echo "FAIL: missing response build summary ${RESPONSE_SUMMARY_PATH}" >&2; exit 1; }
[[ -f "${SUMMARY_JSON_PATH}" ]] || { echo "FAIL: missing summary JSON ${SUMMARY_JSON_PATH}" >&2; exit 1; }
[[ -f "${SUMMARY_MD_PATH}" ]] || { echo "FAIL: missing summary MD ${SUMMARY_MD_PATH}" >&2; exit 1; }

python3 - "${ROOT_DIR}" "${RUN_DIR}" "${PROVENANCE_PATH}" "${REQUEST_SUMMARY_PATH}" "${RESPONSE_SUMMARY_PATH}" "${SUMMARY_JSON_PATH}" <<'PY'
import json
import pathlib
import subprocess
import sys


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


root_dir = pathlib.Path(sys.argv[1]).resolve()
run_dir = pathlib.Path(sys.argv[2]).resolve()
provenance_path = pathlib.Path(sys.argv[3]).resolve()
request_summary_path = pathlib.Path(sys.argv[4]).resolve()
response_summary_path = pathlib.Path(sys.argv[5]).resolve()
summary_json_path = pathlib.Path(sys.argv[6]).resolve()

request_paths = sorted((run_dir / "mixed_bundle" / "requests").glob("request_row*.json"))
response_paths = sorted((run_dir / "mixed_bundle" / "responses").glob("row*/response.json"))
if not request_paths:
    fail("no request JSON files generated")
if not response_paths:
    fail("no response JSON files generated")
if len(request_paths) != len(response_paths):
    fail(f"request count {len(request_paths)} != response count {len(response_paths)}")

for response_path in response_paths:
    subprocess.run(
        [sys.executable, str(root_dir / "scripts" / "validate_local_patch_generic_response.py"), str(response_path)],
        check=True,
    )

provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
request_summary = json.loads(request_summary_path.read_text(encoding="utf-8"))
response_summary = json.loads(response_summary_path.read_text(encoding="utf-8"))
summary = json.loads(summary_json_path.read_text(encoding="utf-8"))

if request_summary["request_counts_by_engine"]["MBD"] <= 0:
    fail("missing MBD requests")
if request_summary["request_counts_by_engine"]["FEM"] <= 0:
    fail("missing FEM requests")
if response_summary["total_request_count"] != len(request_paths):
    fail("response summary total_request_count mismatch")
if response_summary["total_response_count"] != len(response_paths):
    fail("response summary total_response_count mismatch")
if summary["pipeline_mode"] != "mixed_oneway_local_execution_mvp":
    fail("summary pipeline_mode mismatch")
if summary["mixed_replay"] is not False:
    fail("summary mixed_replay must be false")
if summary["mixed_solve"] is not False:
    fail("summary mixed_solve must be false")
if summary["live_cosim"] is not False:
    fail("summary live_cosim must be false")
if summary.get("feedback_available_flag") is not False:
    fail("summary feedback_available_flag must be false")
if summary.get("skip_reason") != "route_has_no_feedback":
    fail("summary skip_reason mismatch")
if int(summary.get("feedback_row_count", -1)) != 0:
    fail("summary feedback_row_count must be 0")
if int(summary.get("feedback_apply_count", -1)) != 0:
    fail("summary feedback_apply_count must be 0")
if summary.get("last_available_differs_from_last_applied") is not False:
    fail("summary last_available_differs_from_last_applied must be false")
if summary.get("unapplied_terminal_candidate_flag") is not False:
    fail("summary unapplied_terminal_candidate_flag must be false")
if summary.get("last_available_feedback_ref") is not None:
    fail("summary last_available_feedback_ref must be null")
if summary.get("last_applied_feedback_ref") is not None:
    fail("summary last_applied_feedback_ref must be null")

entries = provenance.get("requests")
if not isinstance(entries, list) or len(entries) != len(request_paths):
    fail("provenance requests length mismatch")
if provenance.get("feedback_available_flag") is not False:
    fail("provenance feedback_available_flag must be false")
if provenance.get("skip_reason") != "route_has_no_feedback":
    fail("provenance skip_reason mismatch")
if int(provenance.get("feedback_row_count", -1)) != 0:
    fail("provenance feedback_row_count must be 0")
if int(provenance.get("feedback_apply_count", -1)) != 0:
    fail("provenance feedback_apply_count must be 0")
if provenance.get("last_available_differs_from_last_applied") is not False:
    fail("provenance last_available_differs_from_last_applied must be false")
if provenance.get("unapplied_terminal_candidate_flag") is not False:
    fail("provenance unapplied_terminal_candidate_flag must be false")
if provenance.get("last_available_feedback_ref") is not None:
    fail("provenance last_available_feedback_ref must be null")
if provenance.get("last_applied_feedback_ref") is not None:
    fail("provenance last_applied_feedback_ref must be null")
engines = set()
for entry in entries:
    if not isinstance(entry, dict):
        fail("provenance entry must be an object")
    engine = entry.get("source_engine")
    if engine not in {"MBD", "FEM"}:
        fail("bad source_engine in provenance")
    engines.add(engine)
    for field in ("source_trace_csv", "request_path", "response_path"):
        value = entry.get(field)
        if not isinstance(value, str) or not value:
            fail(f"missing provenance field {field}")
    if not isinstance(entry.get("source_trace_key"), dict):
        fail("missing source_trace_key")
    request_path = run_dir / "mixed_bundle" / entry["request_path"]
    response_path = run_dir / "mixed_bundle" / entry["response_path"]
    if not request_path.is_file():
        fail(f"missing request target {request_path}")
    if not response_path.is_file():
        fail(f"missing response target {response_path}")

if engines != {"MBD", "FEM"}:
    fail(f"expected both MBD and FEM engines, got {engines!r}")

print(
    "PASS mixed_macro_local_oneway_generic_v1 "
    f"request_count={len(request_paths)} response_count={len(response_paths)} "
    f"engines={sorted(engines)!r}"
)
PY

grep -q "mixed replay ではない" "${SUMMARY_MD_PATH}"
grep -q "mixed solve ではない" "${SUMMARY_MD_PATH}"
grep -q "live co-sim ではない" "${SUMMARY_MD_PATH}"
grep -q "true lagged mixed time-step co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "same-time mixed co-sim ではない" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "first-class feedback は gamma_n のみ" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "FEM source は static-only" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "feedback_available_flag = false" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "skip_reason = route_has_no_feedback" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "feedback_row_count = 0" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "feedback_apply_count = 0" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "last_available_feedback_ref = null" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "post-solve artifact" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "friction solver state" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "source_step" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"
grep -q "dt_comm" "${ROOT_DIR}/docs/mixed_macro_local_oneway_generic_v1.md"

echo "PASS: mixed_macro_local_oneway_generic_v1 dedicated check"
echo "out_dir=${OUT_DIR}"
