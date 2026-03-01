#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
FEM4C_BIN_DEFAULT="${FEM4C_DIR}/bin/fem4c"
FEM4C_BIN="${FEM4C_MBD_BIN:-${FEM4C_BIN_DEFAULT}}"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

cd "${FEM4C_DIR}"

if [[ ! -x "${FEM4C_BIN}" ]]; then
    echo "FAIL: mbd integrator checker requires executable fem4c binary (${FEM4C_BIN})" >&2
    echo "hint: run 'make -C FEM4C' before mbd_integrator_checks" >&2
    exit 1
fi

require_pattern() {
    local case_name="$1"
    local case_log="$2"
    local pattern="$3"
    if ! grep -q -- "${pattern}" "${case_log}"; then
        echo "FAIL: missing expected log pattern (${case_name}): ${pattern}" >&2
        cat "${case_log}" >&2
        exit 1
    fi
}

run_default_case() {
    local case_log="${tmp_dir}/mbd_default.log"
    local case_out="${tmp_dir}/mbd_default.dat"

    set +e
    env -u FEM4C_MBD_INTEGRATOR \
        -u FEM4C_MBD_NEWMARK_BETA \
        -u FEM4C_MBD_NEWMARK_GAMMA \
        -u FEM4C_MBD_HHT_ALPHA \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd default case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_default" "${case_log}" "Analysis mode: mbd"
    require_pattern "mbd_default" "${case_log}" "MBD integrator source: default"
    require_pattern "mbd_default" "${case_log}" "MBD time source: dt=default steps=default"
    require_pattern "mbd_default" "${case_log}" "integrator: newmark_beta"
    require_pattern "mbd_default" "${case_log}" "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02"
    require_pattern "mbd_default" "${case_log}" "integrator_fallback: newmark_beta=default newmark_gamma=default hht_alpha=default"
    require_pattern "mbd_default" "${case_log}" "time_control: dt=1.000000e-03 steps=1"
    require_pattern "mbd_default" "${case_log}" "time_fallback: dt=default steps=default"
    require_pattern "mbd_default" "${case_log}" "mbd_step=1/1"
    require_pattern "mbd_default" "${case_log}" "steps_trace: requested=1 executed=1"
    require_pattern "mbd_default" "${case_out}" "newmark_beta_source_status,default"
    require_pattern "mbd_default" "${case_out}" "newmark_gamma_source_status,default"
    require_pattern "mbd_default" "${case_out}" "hht_alpha_source_status,default"
    require_pattern "mbd_default" "${case_out}" "dt_source_status,default"
    require_pattern "mbd_default" "${case_out}" "steps_source_status,default"
    require_pattern "mbd_default" "${case_out}" "steps_requested,1"
    require_pattern "mbd_default" "${case_out}" "steps_executed,1"
}

run_env_case() {
    local case_name="$1"
    local integrator_value="$2"
    local expected_integrator="$3"
    local expected_warn="$4"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"

    set +e
    FEM4C_MBD_INTEGRATOR="${integrator_value}" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd mode returned non-zero (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "${case_name}" "${case_log}" "Analysis mode: mbd"
    require_pattern "${case_name}" "${case_log}" "MBD integrator source: env"
    require_pattern "${case_name}" "${case_log}" "MBD time source: dt=default steps=default"
    require_pattern "${case_name}" "${case_log}" "integrator: ${expected_integrator}"
    require_pattern "${case_name}" "${case_log}" "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02"
    require_pattern "${case_name}" "${case_log}" "integrator_fallback: newmark_beta=default newmark_gamma=default hht_alpha=default"

    if [[ "${expected_warn}" == "yes" ]]; then
        require_pattern "${case_name}" "${case_log}" "Warning: invalid FEM4C_MBD_INTEGRATOR='${integrator_value}', fallback to 'newmark_beta'"
    fi
}

run_env_param_case() {
    local case_log="${tmp_dir}/mbd_env_params.log"
    local case_out="${tmp_dir}/mbd_env_params.dat"

    set +e
    FEM4C_MBD_INTEGRATOR="hht_alpha" \
    FEM4C_MBD_NEWMARK_BETA="0.27" \
    FEM4C_MBD_NEWMARK_GAMMA="0.58" \
    FEM4C_MBD_HHT_ALPHA="-0.08" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env param case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_params" "${case_log}" "MBD integrator source: env"
    require_pattern "mbd_env_params" "${case_log}" "MBD parameter source: newmark_beta=env newmark_gamma=env hht_alpha=env"
    require_pattern "mbd_env_params" "${case_log}" "MBD time source: dt=default steps=default"
    require_pattern "mbd_env_params" "${case_log}" "integrator: hht_alpha"
    require_pattern "mbd_env_params" "${case_log}" "integrator_params: newmark_beta=2.700000e-01 newmark_gamma=5.800000e-01 hht_alpha=-8.000000e-02"
    require_pattern "mbd_env_params" "${case_log}" "integrator_fallback: newmark_beta=env newmark_gamma=env hht_alpha=env"
    require_pattern "mbd_env_params" "${case_out}" "newmark_beta_source_status,env"
    require_pattern "mbd_env_params" "${case_out}" "newmark_gamma_source_status,env"
    require_pattern "mbd_env_params" "${case_out}" "hht_alpha_source_status,env"
}

run_env_param_fallback_case() {
    local case_log="${tmp_dir}/mbd_env_params_fallback.log"
    local case_out="${tmp_dir}/mbd_env_params_fallback.dat"

    set +e
    FEM4C_MBD_INTEGRATOR="hht_alpha" \
    FEM4C_MBD_NEWMARK_BETA="9.9" \
    FEM4C_MBD_NEWMARK_GAMMA="-1.0" \
    FEM4C_MBD_HHT_ALPHA="-0.8" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env fallback case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_params_fallback" "${case_log}" "Warning: out-of-range FEM4C_MBD_NEWMARK_BETA='9.9'"
    require_pattern "mbd_env_params_fallback" "${case_log}" "Warning: out-of-range FEM4C_MBD_NEWMARK_GAMMA='-1.0'"
    require_pattern "mbd_env_params_fallback" "${case_log}" "Warning: out-of-range FEM4C_MBD_HHT_ALPHA='-0.8'"
    require_pattern "mbd_env_params_fallback" "${case_log}" "integrator: hht_alpha"
    require_pattern "mbd_env_params_fallback" "${case_log}" "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02"
    require_pattern "mbd_env_params_fallback" "${case_log}" "integrator_fallback: newmark_beta=env_out_of_range_fallback newmark_gamma=env_out_of_range_fallback hht_alpha=env_out_of_range_fallback"
    require_pattern "mbd_env_params_fallback" "${case_log}" "time_control: dt=1.000000e-03 steps=1"
    require_pattern "mbd_env_params_fallback" "${case_out}" "newmark_beta_source_status,env_out_of_range_fallback"
    require_pattern "mbd_env_params_fallback" "${case_out}" "newmark_gamma_source_status,env_out_of_range_fallback"
    require_pattern "mbd_env_params_fallback" "${case_out}" "hht_alpha_source_status,env_out_of_range_fallback"
}

run_env_time_case() {
    local case_log="${tmp_dir}/mbd_env_time.log"
    local case_out="${tmp_dir}/mbd_env_time.dat"

    set +e
    FEM4C_MBD_DT="4.0e-3" \
    FEM4C_MBD_STEPS="5" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env time case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_time" "${case_log}" "MBD time source: dt=env steps=env"
    require_pattern "mbd_env_time" "${case_log}" "time_control: dt=4.000000e-03 steps=5"
    require_pattern "mbd_env_time" "${case_log}" "time_fallback: dt=env steps=env"
    require_pattern "mbd_env_time" "${case_log}" "mbd_step=1/5"
    require_pattern "mbd_env_time" "${case_log}" "mbd_step=5/5"
    require_pattern "mbd_env_time" "${case_log}" "steps_trace: requested=5 executed=5"
    require_pattern "mbd_env_time" "${case_out}" "dt,4.0000000000000001e-03"
    require_pattern "mbd_env_time" "${case_out}" "steps,5"
    require_pattern "mbd_env_time" "${case_out}" "dt_source_status,env"
    require_pattern "mbd_env_time" "${case_out}" "steps_source_status,env"
    require_pattern "mbd_env_time" "${case_out}" "steps_requested,5"
    require_pattern "mbd_env_time" "${case_out}" "steps_executed,5"
}

run_env_time_compact_trace_case() {
    local case_log="${tmp_dir}/mbd_env_time_compact_trace.log"
    local case_out="${tmp_dir}/mbd_env_time_compact_trace.dat"

    set +e
    FEM4C_MBD_DT="1.0e-3" \
    FEM4C_MBD_STEPS="20" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env compact-trace case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_time_compact_trace" "${case_log}" "MBD time source: dt=env steps=env"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "time_control: dt=1.000000e-03 steps=20"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "mbd_step=1/20"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "mbd_step=3/20"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "mbd_step=... (14 steps omitted for compact trace)"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "mbd_step=18/20"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "mbd_step=20/20"
    require_pattern "mbd_env_time_compact_trace" "${case_log}" "steps_trace: requested=20 executed=20"
    require_pattern "mbd_env_time_compact_trace" "${case_out}" "steps_requested,20"
    require_pattern "mbd_env_time_compact_trace" "${case_out}" "steps_executed,20"
}

run_env_time_fallback_case() {
    local case_log="${tmp_dir}/mbd_env_time_fallback.log"
    local case_out="${tmp_dir}/mbd_env_time_fallback.dat"

    set +e
    FEM4C_MBD_DT="not_a_number" \
    FEM4C_MBD_STEPS="1000001" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env time fallback case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_time_fallback" "${case_log}" "MBD time source: dt=env steps=env"
    require_pattern "mbd_env_time_fallback" "${case_log}" "Warning: invalid FEM4C_MBD_DT='not_a_number', fallback to 1.000000e-03"
    require_pattern "mbd_env_time_fallback" "${case_log}" "Warning: out-of-range FEM4C_MBD_STEPS='1000001' (allowed 1..1000000), fallback to 1"
    require_pattern "mbd_env_time_fallback" "${case_log}" "time_control: dt=1.000000e-03 steps=1"
    require_pattern "mbd_env_time_fallback" "${case_log}" "time_fallback: dt=env_invalid_fallback steps=env_out_of_range_fallback"
    require_pattern "mbd_env_time_fallback" "${case_out}" "dt_source_status,env_invalid_fallback"
    require_pattern "mbd_env_time_fallback" "${case_out}" "steps_source_status,env_out_of_range_fallback"
}

run_env_time_whitespace_case() {
    local case_log="${tmp_dir}/mbd_env_time_whitespace.log"
    local case_out="${tmp_dir}/mbd_env_time_whitespace.dat"

    set +e
    FEM4C_MBD_DT=" 4.0e-3" \
    FEM4C_MBD_STEPS=" 5" \
        "${FEM4C_BIN}" --mode=mbd "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd env time whitespace case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_env_time_whitespace" "${case_log}" "Warning: invalid FEM4C_MBD_DT=' 4.0e-3', fallback to 1.000000e-03"
    require_pattern "mbd_env_time_whitespace" "${case_log}" "Warning: invalid FEM4C_MBD_STEPS=' 5', fallback to 1"
    require_pattern "mbd_env_time_whitespace" "${case_log}" "time_control: dt=1.000000e-03 steps=1"
    require_pattern "mbd_env_time_whitespace" "${case_log}" "time_fallback: dt=env_invalid_fallback steps=env_invalid_fallback"
    require_pattern "mbd_env_time_whitespace" "${case_out}" "dt_source_status,env_invalid_fallback"
    require_pattern "mbd_env_time_whitespace" "${case_out}" "steps_source_status,env_invalid_fallback"
}

run_cli_case() {
    local case_log="${tmp_dir}/mbd_cli.log"
    local case_out="${tmp_dir}/mbd_cli.dat"

    set +e
    FEM4C_MBD_INTEGRATOR="newmark_beta" \
    FEM4C_MBD_NEWMARK_BETA="0.23" \
    FEM4C_MBD_NEWMARK_GAMMA="0.51" \
    FEM4C_MBD_HHT_ALPHA="-0.03" \
        "${FEM4C_BIN}" --mode=mbd \
        --mbd-integrator=hht_alpha \
        --mbd-newmark-beta=0.31 \
        --mbd-newmark-gamma=0.62 \
        --mbd-hht-alpha=-0.10 \
        --mbd-dt=2.0e-3 \
        --mbd-steps=3 \
        "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd CLI case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_cli" "${case_log}" "MBD integrator source: cli"
    require_pattern "mbd_cli" "${case_log}" "MBD parameter source: newmark_beta=cli newmark_gamma=cli hht_alpha=cli"
    require_pattern "mbd_cli" "${case_log}" "MBD time source: dt=cli steps=cli"
    require_pattern "mbd_cli" "${case_log}" "integrator: hht_alpha"
    require_pattern "mbd_cli" "${case_log}" "integrator_params: newmark_beta=3.100000e-01 newmark_gamma=6.200000e-01 hht_alpha=-1.000000e-01"
    require_pattern "mbd_cli" "${case_log}" "integrator_fallback: newmark_beta=cli newmark_gamma=cli hht_alpha=cli"
    require_pattern "mbd_cli" "${case_log}" "time_control: dt=2.000000e-03 steps=3"
    require_pattern "mbd_cli" "${case_log}" "time_fallback: dt=cli steps=cli"
    require_pattern "mbd_cli" "${case_log}" "mbd_step=1/3"
    require_pattern "mbd_cli" "${case_log}" "mbd_step=3/3"
    require_pattern "mbd_cli" "${case_log}" "steps_trace: requested=3 executed=3"
    require_pattern "mbd_cli" "${case_out}" "newmark_beta_source_status,cli"
    require_pattern "mbd_cli" "${case_out}" "newmark_gamma_source_status,cli"
    require_pattern "mbd_cli" "${case_out}" "hht_alpha_source_status,cli"
    require_pattern "mbd_cli" "${case_out}" "dt_source_status,cli"
    require_pattern "mbd_cli" "${case_out}" "steps_source_status,cli"
    require_pattern "mbd_cli" "${case_out}" "steps_requested,3"
    require_pattern "mbd_cli" "${case_out}" "steps_executed,3"
}

run_cli_compact_trace_case() {
    local case_log="${tmp_dir}/mbd_cli_compact_trace.log"
    local case_out="${tmp_dir}/mbd_cli_compact_trace.dat"

    set +e
    "${FEM4C_BIN}" --mode=mbd \
        --mbd-dt=1.0e-3 \
        --mbd-steps=20 \
        "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -ne 0 ]]; then
        echo "FAIL: mbd CLI compact-trace case returned non-zero" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_cli_compact_trace" "${case_log}" "MBD time source: dt=cli steps=cli"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "time_control: dt=1.000000e-03 steps=20"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "mbd_step=1/20"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "mbd_step=3/20"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "mbd_step=... (14 steps omitted for compact trace)"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "mbd_step=18/20"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "mbd_step=20/20"
    require_pattern "mbd_cli_compact_trace" "${case_log}" "steps_trace: requested=20 executed=20"
    require_pattern "mbd_cli_compact_trace" "${case_out}" "steps_requested,20"
    require_pattern "mbd_cli_compact_trace" "${case_out}" "steps_executed,20"
    require_pattern "mbd_cli_compact_trace" "${case_out}" "dt_source_status,cli"
    require_pattern "mbd_cli_compact_trace" "${case_out}" "steps_source_status,cli"
}

run_cli_invalid_param_case() {
    local case_log="${tmp_dir}/mbd_cli_invalid_param.log"
    local case_out="${tmp_dir}/mbd_cli_invalid_param.dat"

    set +e
    "${FEM4C_BIN}" --mode=mbd \
        --mbd-integrator=hht_alpha \
        --mbd-hht-alpha=-0.8 \
        "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: mbd CLI invalid-parameter case unexpectedly succeeded" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_cli_invalid_param" "${case_log}" "Invalid value for --mbd-hht-alpha: -0.8 (allowed range: -1/3..0)"
}

run_cli_invalid_dt_case() {
    local case_log="${tmp_dir}/mbd_cli_invalid_dt.log"
    local case_out="${tmp_dir}/mbd_cli_invalid_dt.dat"

    set +e
    "${FEM4C_BIN}" --mode=mbd \
        --mbd-dt=0 \
        "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: mbd CLI invalid dt case unexpectedly succeeded" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_cli_invalid_dt" "${case_log}" "Invalid value for --mbd-dt: 0 (allowed range: 1e-12..1e3)"
}

run_cli_invalid_steps_case() {
    local case_log="${tmp_dir}/mbd_cli_invalid_steps.log"
    local case_out="${tmp_dir}/mbd_cli_invalid_steps.dat"

    set +e
    "${FEM4C_BIN}" --mode=mbd \
        --mbd-steps=0 \
        "examples/t6_cantilever_beam.dat" "${case_out}" >"${case_log}" 2>&1
    local status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: mbd CLI invalid steps case unexpectedly succeeded" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    require_pattern "mbd_cli_invalid_steps" "${case_log}" "Invalid value for --mbd-steps: 0 (allowed range: 1..1000000)"
}

run_default_case
run_env_case "mbd_integrator_newmark" "newmark_beta" "newmark_beta" "no"
run_env_case "mbd_integrator_hht" "hht_alpha" "hht_alpha" "no"
run_env_case "mbd_integrator_invalid_fallback" "invalid_integrator" "newmark_beta" "yes"
run_env_param_case
run_env_param_fallback_case
run_env_time_case
run_env_time_compact_trace_case
run_env_time_fallback_case
run_env_time_whitespace_case
run_cli_case
run_cli_compact_trace_case
run_cli_invalid_param_case
run_cli_invalid_dt_case
run_cli_invalid_steps_case

echo "PASS: mbd integrator switch check (default/env/cli + params/time + boundary/invalid fallback)"
