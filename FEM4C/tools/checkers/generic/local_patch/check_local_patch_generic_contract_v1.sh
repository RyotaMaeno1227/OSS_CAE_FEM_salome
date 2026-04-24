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
DOC_PATH="${ROOT_DIR}/docs/local_patch_generic_contract_v1.md"
EXAMPLE_DIR="${ROOT_DIR}/examples/local_patch_generic_contract_v1"

python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_request.py" \
    "${EXAMPLE_DIR}/request_penetration.json"
python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_request.py" \
    "${EXAMPLE_DIR}/request_force.json"
python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_response.py" \
    "${EXAMPLE_DIR}/response_example.json"
python3 "${ROOT_DIR}/scripts/validate_local_patch_generic_response.py" \
    "${EXAMPLE_DIR}/response_force_example.json"

python3 - "${DOC_PATH}" "${EXAMPLE_DIR}" <<'PY'
import json
import pathlib
import sys

doc_path = pathlib.Path(sys.argv[1])
example_dir = pathlib.Path(sys.argv[2])
doc_text = doc_path.read_text(encoding="utf-8")

required_doc_tokens = [
    "shape 非依存",
    "request_mode",
    "penetration",
    "normal_force",
    "2 planes",
    "patch_size_m",
    "thickness_m",
    "public contract",
    "current circle implementation",
]
for token in required_doc_tokens:
    if token not in doc_text:
        raise SystemExit(f"FAIL: doc is missing token {token!r}")

request_pen = json.loads((example_dir / "request_penetration.json").read_text(encoding="utf-8"))
request_force = json.loads((example_dir / "request_force.json").read_text(encoding="utf-8"))
response_pen = json.loads((example_dir / "response_example.json").read_text(encoding="utf-8"))
response_force = json.loads((example_dir / "response_force_example.json").read_text(encoding="utf-8"))

if request_pen["request_mode"] != "penetration":
    raise SystemExit("FAIL: request_penetration.json mode mismatch")
if request_force["request_mode"] != "normal_force":
    raise SystemExit("FAIL: request_force.json mode mismatch")
if "penetration_m" not in request_pen["loading"]:
    raise SystemExit("FAIL: request_penetration.json missing penetration_m")
if "fn_macro_n" not in request_force["loading"]:
    raise SystemExit("FAIL: request_force.json missing fn_macro_n")
if response_pen["request_mode"] != "penetration":
    raise SystemExit("FAIL: response_example.json mode mismatch")
if response_force["request_mode"] != "normal_force":
    raise SystemExit("FAIL: response_force_example.json mode mismatch")

print("PASS local_patch_generic_contract_v1 doc_and_examples_ok")
PY

echo "PASS: local patch generic contract v1 check"
