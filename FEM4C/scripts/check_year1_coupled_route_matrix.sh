#!/usr/bin/env bash
set -euo pipefail

# Year1 coupled route runtime surface only.
# This script promotes current live-root route execution truth; it does not prove
# the triangle-only validated FEM scope and it intentionally uses the existing
# coupled example decks, including Q4-backed coupled fixtures where applicable.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${FEM4C_DIR}"

BIN="./bin/fem4c"
ONEWAY_INPUT="examples/coupled_1link_flex_master.dat"
MULTI_INPUT="examples/coupled_2link_flex_master.dat"
INTEGRATOR="newmark_beta"
STEPS="1"

failures=0

require_path() {
    local path="$1"
    local kind="$2"
    if [[ ! -e "${path}" ]]; then
        echo "FAIL: missing ${kind}: ${path}" >&2
        exit 1
    fi
}

check_token() {
    local route="$1"
    local log_path="$2"
    local token="$3"

    if ! grep -Fq "${token}" "${log_path}"; then
        echo "FAIL: route=${route} missing token: ${token}" >&2
        return 1
    fi
    return 0
}

check_output_token() {
    local route="$1"
    local out_path="$2"
    local token="$3"

    if ! grep -Fqx "${token}" "${out_path}"; then
        echo "FAIL: route=${route} missing output token: ${token}" >&2
        return 1
    fi
    return 0
}

check_route_output_contract() {
    local route="$1"
    local out_path="$2"

    case "${route}" in
        oneway_snapshot)
            check_output_token "${route}" "${out_path}" "comparison_role,official_reference" || return 1
            check_output_token "${route}" "${out_path}" "solver_route_class,accepted_snapshot_replay" || return 1
            check_output_token "${route}" "${out_path}" "delay_semantics_status,not_applicable" || return 1
            check_output_token "${route}" "${out_path}" "compare_schema_version,year1_2link_v1" || return 1
            check_output_token "${route}" "${out_path}" "compare_step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,compare_iteration_count,coupling_converged,exchange_lag_steps,sample_hold_active,delayed_snapshot_step" || return 1
            check_output_token "${route}" "${out_path}" "step_flex_counter_columns,step_index,snapshot_iteration_index,body_id,full_reassembly_count,static_solve_count" || return 1
            ;;
        delayed_cosim_v1_5)
            check_output_token "${route}" "${out_path}" "comparison_role,co_simulation" || return 1
            check_output_token "${route}" "${out_path}" "solver_route_class,partitioned_delayed_cosim_sample_hold_2link_body_interface_skeleton" || return 1
            check_output_token "${route}" "${out_path}" "delay_semantics_status,lag1_sample_hold_accepted_previous_step_skeleton" || return 1
            check_output_token "${route}" "${out_path}" "exchange_lag_steps,1" || return 1
            check_output_token "${route}" "${out_path}" "step_columns,step_index,time,constraint_residual_l2,flex_solves,exchange_lag_steps,sample_hold_active,delayed_snapshot_step" || return 1
            check_output_token "${route}" "${out_path}" "compare_schema_version,year1_2link_v1" || return 1
            check_output_token "${route}" "${out_path}" "compare_step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,compare_iteration_count,coupling_converged,exchange_lag_steps,sample_hold_active,delayed_snapshot_step" || return 1
            check_output_token "${route}" "${out_path}" "step_flex_counter_columns,step_index,communication_iteration_index,body_id,full_reassembly_count,static_solve_count" || return 1
            ;;
        fixed_point_strong)
            check_output_token "${route}" "${out_path}" "comparison_role,legacy_experimental" || return 1
            check_output_token "${route}" "${out_path}" "solver_route_class,same_step_fixed_point" || return 1
            check_output_token "${route}" "${out_path}" "delay_semantics_status,not_applicable" || return 1
            check_output_token "${route}" "${out_path}" "coupling_metric,qflex_l2" || return 1
            check_output_token "${route}" "${out_path}" "step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,fixed_point_iterations,coupling_converged" || return 1
            check_output_token "${route}" "${out_path}" "step_flex_counter_columns,step_index,coupling_iteration_index,body_id,full_reassembly_count,static_solve_count" || return 1
            ;;
        monolithic_strong_v1)
            check_output_token "${route}" "${out_path}" "comparison_role,monolithic_strong" || return 1
            check_output_token "${route}" "${out_path}" "solver_route_class,single_coupled_system_2link_body_interface_skeleton" || return 1
            check_output_token "${route}" "${out_path}" "delay_semantics_status,not_applicable" || return 1
            check_output_token "${route}" "${out_path}" "coupling_metric,monolithic_reduced_residual_l2" || return 1
            check_output_token "${route}" "${out_path}" "step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,fixed_point_iterations,coupling_converged" || return 1
            check_output_token "${route}" "${out_path}" "compare_schema_version,year1_2link_v1" || return 1
            check_output_token "${route}" "${out_path}" "compare_step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,compare_iteration_count,coupling_converged,exchange_lag_steps,sample_hold_active,delayed_snapshot_step" || return 1
            check_output_token "${route}" "${out_path}" "physical_status_columns,step_index,physical_residual_l2,constraint_residual_l2" || return 1
            check_output_token "${route}" "${out_path}" "step_status_columns,step_index,coupling_converged,coupling_iterations,coupling_reason" || return 1
            check_output_token "${route}" "${out_path}" "step_flex_counter_columns,step_index,coupling_iteration_index,body_id,full_reassembly_count,static_solve_count" || return 1
            ;;
        *)
            echo "FAIL: route=${route} has no output contract expectation" >&2
            return 1
            ;;
    esac

    return 0
}

run_route_case() {
    local route="$1"
    local input_path="$2"
    shift 2
    local -a required_tokens=("$@")
    local log_path="/tmp/year1_route_matrix_${route}.log"
    local out_path="/tmp/year1_route_matrix_${route}.out"
    local exit_code=0
    local token_failed=0
    local token

    echo "== route ${route} =="
    echo "log=${log_path}"
    echo "out=${out_path}"

    set +e
    stdbuf -o0 -e0 "${BIN}" \
        --mode=coupled \
        --coupled-scheme="${route}" \
        --coupled-integrator="${INTEGRATOR}" \
        --mbd-steps="${STEPS}" \
        "${input_path}" \
        "${out_path}" \
        >"${log_path}" 2>&1
    exit_code=$?
    set -e

    if [[ ${exit_code} -ne 0 ]]; then
        echo "FAIL: route=${route} exit_code=${exit_code}" >&2
        echo "  log=${log_path}" >&2
        failures=$((failures + 1))
        return
    fi

    for token in "${required_tokens[@]}"; do
        if ! check_token "${route}" "${log_path}" "${token}"; then
            token_failed=1
        fi
    done

    if [[ ${token_failed} -ne 0 ]]; then
        echo "FAIL: route=${route} token check failed" >&2
        echo "  log=${log_path}" >&2
        failures=$((failures + 1))
        return
    fi

    if ! check_route_output_contract "${route}" "${out_path}"; then
        echo "FAIL: route=${route} output contract check failed" >&2
        echo "  out=${out_path}" >&2
        failures=$((failures + 1))
        return
    fi

    echo "PASS: route=${route}"
}

require_path "${BIN}" "binary"
require_path "${ONEWAY_INPUT}" "one-way example"
require_path "${MULTI_INPUT}" "multi-body example"

run_route_case \
    "oneway_snapshot" \
    "${ONEWAY_INPUT}" \
    "step_runner=coupled_step_oneway2d_run" \
    "comparison_role=official_reference" \
    "solver_route_class=accepted_snapshot_replay" \
    "delay_semantics_status=not_applicable"

run_route_case \
    "delayed_cosim_v1_5" \
    "${MULTI_INPUT}" \
    "step_runner=coupled_step_delayed_cosim2d_run" \
    "comparison_role=co_simulation" \
    "delay_semantics_status=lag1_sample_hold_accepted_previous_step_skeleton" \
    "exchange_lag_steps=1" \
    "year1_experimental_only=1" \
    "monolithic_strong_v1!=delayed_cosim_v1_5"

run_route_case \
    "fixed_point_strong" \
    "${MULTI_INPUT}" \
    "step_runner=coupled_step_implicit2d_run" \
    "comparison_role=legacy_experimental" \
    "solver_route_class=same_step_fixed_point" \
    "same_step_iteration="

run_route_case \
    "monolithic_strong_v1" \
    "${MULTI_INPUT}" \
    "step_runner=coupled_step_monolithic2d_run" \
    "comparison_role=monolithic_strong" \
    "solver_route_class=single_coupled_system_2link_body_interface_skeleton" \
    "year1_experimental_only=1" \
    "fixed_point_strong!=monolithic_strong_v1" \
    "monolithic_iteration="

if [[ ${failures} -ne 0 ]]; then
    echo "FAIL: Year1 coupled route matrix runtime surface (${failures} route(s) failed)" >&2
    exit 1
fi

echo "PASS: Year1 coupled route matrix runtime surface"
