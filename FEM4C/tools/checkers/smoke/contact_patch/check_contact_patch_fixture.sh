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
OUT_DIR="${FEM4C_CONTACT_PATCH_FIXTURE_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_patch_fixture}"

run_case() {
    local case_name="$1"
    local deck_path="$2"
    local output_path="${OUT_DIR}/${case_name}.out"
    local log_path="${OUT_DIR}/${case_name}.log"
    local vtk_path="${OUT_DIR}/${case_name}.vtk"
    local f06_path="${OUT_DIR}/${case_name}.f06"

    "${ROOT_DIR}/bin/fem4c" "${deck_path}" "${output_path}" >"${log_path}" 2>&1

    if [[ ! -f "${output_path}" ]]; then
        echo "FAIL: missing output file for ${case_name}: ${output_path}" >&2
        exit 1
    fi
    if [[ ! -f "${vtk_path}" ]]; then
        echo "FAIL: missing VTK file for ${case_name}: ${vtk_path}" >&2
        exit 1
    fi
    if [[ ! -f "${f06_path}" ]]; then
        echo "FAIL: missing F06 file for ${case_name}: ${f06_path}" >&2
        exit 1
    fi

    grep -q "Program completed successfully." "${log_path}"
    grep -q "Nodal Displacements:" "${output_path}"
    grep -q "Reaction Forces:" "${output_path}"

    echo "PASS case=${case_name} output=${output_path}"
}

mkdir -p "${OUT_DIR}"

make -C "${ROOT_DIR}" -j

run_case "patch_small_q4" "${ROOT_DIR}/examples/contact_patch/patch_small_q4.dat"
run_case "patch_large_q4" "${ROOT_DIR}/examples/contact_patch/patch_large_q4.dat"

echo "PASS: contact patch fixtures"
