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

if [[ $# -ne 2 ]]; then
    echo "Usage: bash tools/runners/mixed_bounded/run_mixed_macro_local_oneway_generic_v1.sh <mixed_manifest.json> <outdir>" >&2
    exit 2
fi

MANIFEST_PATH="$1"
OUT_DIR="$2"
MIXED_BUNDLE_DIR="${OUT_DIR}/mixed_bundle"

rm -rf "${OUT_DIR}"
mkdir -p "${MIXED_BUNDLE_DIR}"

python3 "${ROOT_DIR}/scripts/validate_mixed_macro_local_generic_manifest.py" "${MANIFEST_PATH}"
python3 "${ROOT_DIR}/scripts/build_local_patch_generic_requests_from_mixed_macro_manifest.py" \
    "${MANIFEST_PATH}" \
    "${MIXED_BUNDLE_DIR}"

python3 - "${ROOT_DIR}" "${MANIFEST_PATH}" "${OUT_DIR}" "${MIXED_BUNDLE_DIR}" <<'PY'
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def load_json(path: Path) -> dict:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing JSON file: {path}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON {path}: {exc}")
    if not isinstance(data, dict):
        fail(f"{path} top-level JSON must be an object")
    return data


def write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


root_dir = Path(sys.argv[1]).resolve()
manifest_path = Path(sys.argv[2]).resolve()
out_dir = Path(sys.argv[3]).resolve()
mixed_bundle_dir = Path(sys.argv[4]).resolve()

request_dir = mixed_bundle_dir / "requests"
response_root = mixed_bundle_dir / "responses"
response_root.mkdir(parents=True, exist_ok=True)

request_paths = sorted(request_dir.glob("request_row*.json"))
if not request_paths:
    fail(f"no request files under {request_dir}")

request_build_summary = load_json(mixed_bundle_dir / "request_build_summary.json")
provenance_manifest = load_json(mixed_bundle_dir / "provenance_manifest.json")

provenance_entries = provenance_manifest.get("requests")
if not isinstance(provenance_entries, list):
    fail("provenance_manifest.json requests must be an array")
fem_load_steps = sorted(
    {
        int(entry["source_trace_key"]["load_step"])
        for entry in provenance_entries
        if isinstance(entry, dict)
        and entry.get("source_engine") == "FEM"
        and isinstance(entry.get("source_trace_key"), dict)
        and "load_step" in entry["source_trace_key"]
    }
)
later_step_fem_source_rows = sum(
    1
    for entry in provenance_entries
    if isinstance(entry, dict)
    and entry.get("source_engine") == "FEM"
    and isinstance(entry.get("source_trace_key"), dict)
    and int(entry["source_trace_key"].get("load_step", 0)) > 0
)
provenance_by_request = {}
for entry in provenance_entries:
    if not isinstance(entry, dict):
        fail("provenance_manifest entry must be an object")
    request_path = entry.get("request_path")
    if not isinstance(request_path, str):
        fail("provenance_manifest request_path must be a string")
    provenance_by_request[request_path] = entry

status_counts: Counter[str] = Counter()
valid_response_count = 0
invalid_response_count = 0
response_paths: list[Path] = []

for request_path in request_paths:
    rel_request_path = request_path.relative_to(mixed_bundle_dir).as_posix()
    row_tag = request_path.stem.replace("request_", "")
    response_dir = response_root / row_tag
    response_dir.mkdir(parents=True, exist_ok=True)

    subprocess.run(
        [sys.executable, str(root_dir / "scripts" / "validate_local_patch_generic_request.py"), str(request_path)],
        check=True,
    )
    subprocess.run(
        [sys.executable, str(root_dir / "scripts" / "run_local_patch_generic_contract_v1.py"), str(request_path), str(response_dir)],
        check=True,
    )

    response_path = response_dir / "response.json"
    subprocess.run(
        [sys.executable, str(root_dir / "scripts" / "validate_local_patch_generic_response.py"), str(response_path)],
        check=True,
    )
    response_paths.append(response_path)

    response = load_json(response_path)
    result = response.get("result")
    if not isinstance(result, dict):
        fail(f"{response_path} missing result")
    valid_flag = result.get("valid_flag")
    status = result.get("status")
    if not isinstance(valid_flag, int):
        fail(f"{response_path} valid_flag must be int")
    if not isinstance(status, str):
        fail(f"{response_path} status must be string")

    if valid_flag == 1:
        valid_response_count += 1
    else:
        invalid_response_count += 1
    status_counts[status] += 1

    provenance_entry = provenance_by_request.get(rel_request_path)
    if provenance_entry is None:
        fail(f"missing provenance entry for {rel_request_path}")
    provenance_entry["response_path"] = response_path.relative_to(mixed_bundle_dir).as_posix()
    provenance_entry["response_valid_flag"] = valid_flag
    provenance_entry["response_status"] = status

if len(request_paths) != len(response_paths):
    fail(f"request count {len(request_paths)} != response count {len(response_paths)}")

request_counts_by_engine = request_build_summary.get("request_counts_by_engine", {})
mbd_request_count = int(request_counts_by_engine.get("MBD", 0))
fem_request_count = int(request_counts_by_engine.get("FEM", 0))
if mbd_request_count <= 0:
    fail("request_counts_by_engine.MBD must be >= 1")
if fem_request_count <= 0:
    fail("request_counts_by_engine.FEM must be >= 1")

response_build_summary = {
    "route": "mixed_macro_local_oneway_generic_v1",
    "pipeline_mode": "mixed_oneway_local_execution_mvp",
    "mixed_replay": False,
    "mixed_solve": False,
    "live_cosim": False,
    "source_engine_histogram": {
        "MBD": mbd_request_count,
        "FEM": fem_request_count,
    },
    "fem_distinct_load_step_count": len(fem_load_steps),
    "fem_load_steps": fem_load_steps,
    "later_step_fem_source_rows": later_step_fem_source_rows,
    "total_request_count": len(request_paths),
    "total_response_count": len(response_paths),
    "mbd_request_count": mbd_request_count,
    "fem_request_count": fem_request_count,
    "valid_response_count": valid_response_count,
    "invalid_response_count": invalid_response_count,
    "status_histogram": dict(sorted(status_counts.items())),
    "request_dir": request_dir.relative_to(mixed_bundle_dir).as_posix(),
    "response_dir": response_root.relative_to(mixed_bundle_dir).as_posix(),
}
write_json(mixed_bundle_dir / "response_build_summary.json", response_build_summary)

provenance_manifest["route"] = "mixed_macro_local_oneway_generic_v1"
provenance_manifest["pipeline_mode"] = "mixed_oneway_local_execution_mvp"
provenance_manifest["request_count"] = len(request_paths)
provenance_manifest["response_count"] = len(response_paths)
provenance_manifest["valid_response_count"] = valid_response_count
provenance_manifest["invalid_response_count"] = invalid_response_count
provenance_manifest["mixed_replay"] = False
provenance_manifest["mixed_solve"] = False
provenance_manifest["live_cosim"] = False
provenance_manifest["source_engine_histogram"] = {
    "MBD": mbd_request_count,
    "FEM": fem_request_count,
}
provenance_manifest["fem_distinct_load_step_count"] = len(fem_load_steps)
provenance_manifest["fem_load_steps"] = fem_load_steps
provenance_manifest["later_step_fem_source_rows"] = later_step_fem_source_rows
provenance_manifest["feedback_available_flag"] = False
provenance_manifest["skip_reason"] = "route_has_no_feedback"
provenance_manifest["feedback_row_count"] = 0
provenance_manifest["feedback_apply_count"] = 0
provenance_manifest["last_available_differs_from_last_applied"] = False
provenance_manifest["unapplied_terminal_candidate_flag"] = False
provenance_manifest["last_available_feedback_ref"] = None
provenance_manifest["last_applied_feedback_ref"] = None
write_json(mixed_bundle_dir / "provenance_manifest.json", provenance_manifest)

final_summary = {
    "route": "mixed_macro_local_oneway_generic_v1",
    "pipeline_mode": "mixed_oneway_local_execution_mvp",
    "mixed_replay": False,
    "mixed_solve": False,
    "live_cosim": False,
    "source_engine_histogram": {
        "MBD": mbd_request_count,
        "FEM": fem_request_count,
    },
    "fem_distinct_load_step_count": len(fem_load_steps),
    "fem_load_steps": fem_load_steps,
    "later_step_fem_source_rows": later_step_fem_source_rows,
    "manifest_path": str(manifest_path),
    "mixed_bundle_dir": mixed_bundle_dir.relative_to(out_dir).as_posix(),
    "total_request_count": len(request_paths),
    "total_response_count": len(response_paths),
    "mbd_request_count": mbd_request_count,
    "fem_request_count": fem_request_count,
    "valid_response_count": valid_response_count,
    "invalid_response_count": invalid_response_count,
    "status_histogram": dict(sorted(status_counts.items())),
    "feedback_available_flag": False,
    "skip_reason": "route_has_no_feedback",
    "feedback_row_count": 0,
    "feedback_apply_count": 0,
    "last_available_differs_from_last_applied": False,
    "unapplied_terminal_candidate_flag": False,
    "last_available_feedback_ref": None,
    "last_applied_feedback_ref": None,
    "limitations": [
        "mixed one-way local execution MVP",
        "mixed replay not implemented",
        "mixed solve not implemented",
        "live co-sim not implemented",
        "not true lagged mixed time-step co-sim",
        "not same-time mixed co-sim",
        "local solver is proxy/MVP",
        "FEM source is static-only",
        "first-class feedback remains gamma_n only",
        "friction not implemented",
        "damping not implemented",
        "EHL out of scope",
        "monolithic out of scope",
    ],
}
write_json(out_dir / "mixed_oneway_summary.json", final_summary)

summary_md_lines = [
    "# mixed_macro_local_oneway_generic_v1 summary",
    "",
    f"- pipeline_mode: `{final_summary['pipeline_mode']}`",
    f"- total_request_count: `{final_summary['total_request_count']}`",
    f"- total_response_count: `{final_summary['total_response_count']}`",
    f"- mbd_request_count: `{final_summary['mbd_request_count']}`",
    f"- fem_request_count: `{final_summary['fem_request_count']}`",
    f"- fem_distinct_load_step_count: `{final_summary['fem_distinct_load_step_count']}`",
    f"- fem_load_steps: `{final_summary['fem_load_steps']}`",
    f"- later_step_fem_source_rows: `{final_summary['later_step_fem_source_rows']}`",
    f"- valid_response_count: `{final_summary['valid_response_count']}`",
    f"- invalid_response_count: `{final_summary['invalid_response_count']}`",
    f"- feedback_available_flag: `{final_summary['feedback_available_flag']}`",
    f"- skip_reason: `{final_summary['skip_reason']}`",
    f"- feedback_row_count: `{final_summary['feedback_row_count']}`",
    f"- feedback_apply_count: `{final_summary['feedback_apply_count']}`",
    f"- last_available_differs_from_last_applied: `{final_summary['last_available_differs_from_last_applied']}`",
    f"- unapplied_terminal_candidate_flag: `{final_summary['unapplied_terminal_candidate_flag']}`",
    "",
    "## truth",
    "",
    "- mixed one-way local execution MVP only.",
    "- mixed replay ではない.",
    "- mixed solve ではない.",
    "- live co-sim ではない.",
    "- true lagged mixed time-step co-sim ではない.",
    "- same-time mixed co-sim ではない.",
    "- local solver は proxy / MVP.",
    "- first-class feedback は gamma_n のみという整理を維持する.",
    "- FEM source は static-only.",
    "- この route 自体は reduced feedback candidate を生成しないため、top-level feedback_available_flag は false のまま固定する.",
    "- source_step / apply_step / dt_comm / dt_sub / subcycle_count は time-coupled route ではないので top-level summary には持たない.",
]
Path(out_dir / "mixed_oneway_summary.md").write_text("\n".join(summary_md_lines) + "\n", encoding="utf-8")

print(
    "PASS mixed_macro_local_oneway_generic_v1 "
    f"request_count={len(request_paths)} response_count={len(response_paths)} "
    f"mbd_request_count={mbd_request_count} fem_request_count={fem_request_count}"
)
PY

echo "PASS: run_mixed_macro_local_oneway_generic_v1 out_dir=${OUT_DIR}"
