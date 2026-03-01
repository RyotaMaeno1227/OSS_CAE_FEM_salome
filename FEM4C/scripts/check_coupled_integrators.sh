#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

cd "${FEM4C_DIR}"

run_case() {
    local case_name="$1"
    local integrator_value="$2"
    local expected_integrator="$3"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    FEM4C_COUPLED_INTEGRATOR="${integrator_value}" \
        ./bin/fem4c --mode=coupled "examples/t3_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    # coupled mode is currently stubbed; non-zero is expected.
    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly returned success (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    if ! grep -q "Analysis mode: coupled" "${case_log}"; then
        echo "FAIL: missing coupled mode header (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi
    if ! grep -q "Coupled mode contract snapshot (stub):" "${case_log}"; then
        echo "FAIL: missing coupled contract snapshot (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi
    if ! grep -q "integrator=${expected_integrator}" "${case_log}"; then
        echo "FAIL: unexpected integrator log (${case_name}) expected=${expected_integrator}" >&2
        cat "${case_log}" >&2
        exit 1
    fi
    if ! grep -q "Coupled FEM+MBD mode is not wired yet" "${case_log}"; then
        echo "FAIL: missing coupled stub tail (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi
}

run_case "coupled_integrator_newmark" "newmark_beta" "newmark_beta"
run_case "coupled_integrator_hht" "hht_alpha" "hht_alpha"
run_case "coupled_integrator_invalid_fallback" "invalid_integrator" "newmark_beta"

echo "PASS: coupled integrator switch check (newmark_beta + hht_alpha + invalid fallback)"
