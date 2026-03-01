#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

log_file="${tmp_dir}/coupled_stub.log"
out_file="${tmp_dir}/coupled_stub_out.dat"

cd "${FEM4C_DIR}"

run_case() {
    local case_name="$1"
    local input_file="$2"
    local fem_pattern="$3"
    local integrator_value="$4"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    FEM4C_COUPLED_INTEGRATOR="${integrator_value}" \
        ./bin/fem4c --mode=coupled "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly returned success (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Analysis mode: coupled" "${case_log}"
    grep -q "Coupled mode contract snapshot (stub):" "${case_log}"
    grep -Eq "${fem_pattern}" "${case_log}"
    grep -q "mbd: bodies=2 constraints=2" "${case_log}"
    grep -Eq "bodies_ptr=0x[0-9a-fA-F]+" "${case_log}"
    grep -Eq "constraints_ptr=0x[0-9a-fA-F]+" "${case_log}"
    grep -q "time: dt=1.000000e-03 steps=1 max_iter=10 residual_tol=1.000000e-06" "${case_log}"
    grep -q "integrator=${integrator_value}" "${case_log}"
    grep -q "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02" "${case_log}"
    grep -q "Coupled FEM+MBD mode is not wired yet" "${case_log}"
}

run_case_cli_integrator() {
    local case_name="$1"
    local input_file="$2"
    local fem_pattern="$3"
    local integrator_value="$4"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    ./bin/fem4c --mode=coupled --coupled-integrator="${integrator_value}" \
        "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly returned success (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Analysis mode: coupled" "${case_log}"
    grep -q "Coupled integrator: ${integrator_value}" "${case_log}"
    grep -q "Coupled integrator source: cli" "${case_log}"
    grep -q "Coupled mode contract snapshot (stub):" "${case_log}"
    grep -Eq "${fem_pattern}" "${case_log}"
    grep -q "integrator=${integrator_value}" "${case_log}"
    grep -q "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02" "${case_log}"
    grep -q "Coupled FEM+MBD mode is not wired yet" "${case_log}"
}

run_expected_input_error_case() {
    local case_name="$1"
    local input_file="$2"
    local expected_error_pattern="$3"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    ./bin/fem4c --mode=coupled "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly succeeded for invalid input (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Analysis mode: coupled" "${case_log}"
    grep -q "FEM4C Error \\[5\\]:" "${case_log}"
    grep -Eq "${expected_error_pattern}" "${case_log}"
    if grep -q "Coupled mode contract snapshot (stub):" "${case_log}"; then
        echo "FAIL: invalid input should fail before coupled stub snapshot (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi
}

run_invalid_integrator_fallback_case() {
    local case_name="$1"
    local input_file="$2"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    FEM4C_COUPLED_INTEGRATOR="invalid_integrator" \
        ./bin/fem4c --mode=coupled "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly succeeded for invalid integrator fallback case" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Warning: invalid FEM4C_COUPLED_INTEGRATOR='invalid_integrator'" "${case_log}"
    grep -q "Coupled mode contract snapshot (stub):" "${case_log}"
    grep -q "integrator=newmark_beta" "${case_log}"
    grep -q "Coupled FEM+MBD mode is not wired yet" "${case_log}"
}

run_env_param_fallback_case() {
    local case_name="$1"
    local input_file="$2"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    FEM4C_COUPLED_INTEGRATOR="hht_alpha" \
    FEM4C_NEWMARK_BETA="9.9" \
    FEM4C_NEWMARK_GAMMA="-1.0" \
    FEM4C_HHT_ALPHA="-0.8" \
        ./bin/fem4c --mode=coupled "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly succeeded for env-param fallback case" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Warning: out-of-range FEM4C_NEWMARK_BETA='9.9'" "${case_log}"
    grep -q "Warning: out-of-range FEM4C_NEWMARK_GAMMA='-1.0'" "${case_log}"
    grep -q "Warning: out-of-range FEM4C_HHT_ALPHA='-0.8'" "${case_log}"
    grep -q "integrator=hht_alpha" "${case_log}"
    grep -q "integrator_params: newmark_beta=2.500000e-01 newmark_gamma=5.000000e-01 hht_alpha=-5.000000e-02" "${case_log}"
}

run_cli_precedence_case() {
    local case_name="$1"
    local input_file="$2"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    FEM4C_COUPLED_INTEGRATOR="newmark_beta" \
        ./bin/fem4c --mode=coupled --coupled-integrator=hht_alpha \
        --newmark-beta=0.31 --newmark-gamma=0.62 --hht-alpha=-0.10 \
        "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly succeeded for CLI precedence case" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "Coupled integrator: hht_alpha" "${case_log}"
    grep -q "Coupled integrator source: cli" "${case_log}"
    grep -q "Coupled parameter source: newmark_beta=cli newmark_gamma=cli hht_alpha=cli" "${case_log}"
    grep -q "integrator=hht_alpha" "${case_log}"
    grep -q "integrator_params: newmark_beta=3.100000e-01 newmark_gamma=6.200000e-01 hht_alpha=-1.000000e-01" "${case_log}"
}

run_expected_cli_option_error_case() {
    local case_name="$1"
    local option_args="$2"
    local input_file="$3"
    local expected_error="$4"
    local case_log="${tmp_dir}/${case_name}.log"
    local case_out="${tmp_dir}/${case_name}.dat"
    local status=0

    set +e
    # shellcheck disable=SC2086
    ./bin/fem4c --mode=coupled ${option_args} "${input_file}" "${case_out}" >"${case_log}" 2>&1
    status=$?
    set -e

    if [[ ${status} -eq 0 ]]; then
        echo "FAIL: coupled mode unexpectedly succeeded for CLI option error case (${case_name})" >&2
        cat "${case_log}" >&2
        exit 1
    fi

    grep -q "${expected_error}" "${case_log}"
}

run_case "coupled_stub_base" "examples/t3_cantilever_beam.dat" \
    "fem: nodes=[1-9][0-9]* elements=[1-9][0-9]* materials=[1-9][0-9]*" \
    "newmark_beta"
run_case "coupled_stub_base_hht" "examples/t3_cantilever_beam.dat" \
    "fem: nodes=[1-9][0-9]* elements=[1-9][0-9]* materials=[1-9][0-9]*" \
    "hht_alpha"
run_case_cli_integrator "coupled_stub_base_hht_cli" "examples/t3_cantilever_beam.dat" \
    "fem: nodes=[1-9][0-9]* elements=[1-9][0-9]* materials=[1-9][0-9]*" \
    "hht_alpha"

input_with_mbd="${tmp_dir}/t3_with_mbd.dat"
cp "examples/t3_cantilever_beam.dat" "${input_with_mbd}"
cat >>"${input_with_mbd}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
MBD_REVOLUTE 2 0 1 0.5 0.0 -0.5 0.0
EOF

run_case "coupled_stub_with_mbd" "${input_with_mbd}" \
    "fem: nodes=[1-9][0-9]* elements=[1-9][0-9]* materials=[1-9][0-9]*" \
    "newmark_beta"
run_case "coupled_stub_with_mbd_hht" "${input_with_mbd}" \
    "fem: nodes=[1-9][0-9]* elements=[1-9][0-9]* materials=[1-9][0-9]*" \
    "hht_alpha"

legacy_pkg="${FEM4C_OLD_PARSER_PKG:-/tmp/parser_pkg_old}"
if [[ -d "${legacy_pkg}" ]]; then
    run_case "coupled_stub_legacy_pkg" "${legacy_pkg}" \
        "fem: nodes=3 elements=1 materials=1" \
        "newmark_beta"
    run_case "coupled_stub_legacy_pkg_hht" "${legacy_pkg}" \
        "fem: nodes=3 elements=1 materials=1" \
        "hht_alpha"
fi

run_invalid_integrator_fallback_case "coupled_stub_invalid_integrator_fallback" \
    "examples/t3_cantilever_beam.dat"
run_env_param_fallback_case "coupled_stub_env_param_fallback" \
    "examples/t3_cantilever_beam.dat"
run_cli_precedence_case "coupled_stub_cli_precedence" "examples/t3_cantilever_beam.dat"
run_expected_cli_option_error_case "coupled_stub_cli_hht_alpha_out_of_range" \
    "--coupled-integrator=hht_alpha --hht-alpha=-0.8" \
    "examples/t3_cantilever_beam.dat" \
    "Invalid value for --hht-alpha:"

case_bad_body_parse="${tmp_dir}/t3_with_bad_body_parse.dat"
cp "examples/t3_cantilever_beam.dat" "${case_bad_body_parse}"
cat >>"${case_bad_body_parse}" <<'EOF'
MBD_BODY 0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
EOF
run_expected_input_error_case "coupled_stub_bad_body_parse" "${case_bad_body_parse}" \
    "MBD_INPUT_ERROR\\[E_BODY_PARSE\\] Invalid MBD_BODY at line"

case_body_range="${tmp_dir}/t3_with_body_range.dat"
cp "examples/t3_cantilever_beam.dat" "${case_body_range}"
cat >>"${case_body_range}" <<'EOF'
MBD_BODY 8 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 1 1 0.0 0.0 0.0 0.0 1.0
EOF
run_expected_input_error_case "coupled_stub_body_range" "${case_body_range}" \
    "MBD_INPUT_ERROR\\[E_BODY_RANGE\\] MBD_BODY index 8"

case_undefined_ref="${tmp_dir}/t3_with_undefined_ref.dat"
cp "examples/t3_cantilever_beam.dat" "${case_undefined_ref}"
cat >>"${case_undefined_ref}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 2 0.0 0.0 0.0 0.0 1.0
EOF
run_expected_input_error_case "coupled_stub_undefined_ref" "${case_undefined_ref}" \
    "MBD_INPUT_ERROR\\[E_UNDEFINED_BODY_REF\\] Undefined MBD_BODY 2 referenced"

case_incomplete="${tmp_dir}/t3_with_incomplete_mbd.dat"
cp "examples/t3_cantilever_beam.dat" "${case_incomplete}"
cat >>"${case_incomplete}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
EOF
run_expected_input_error_case "coupled_stub_incomplete" "${case_incomplete}" \
    "MBD_INPUT_ERROR\\[E_INCOMPLETE_INPUT\\] MBD input is incomplete: no constraints found"

case_distance_range="${tmp_dir}/t3_with_distance_range.dat"
cp "examples/t3_cantilever_beam.dat" "${case_distance_range}"
cat >>"${case_distance_range}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 8 0.0 0.0 0.0 0.0 1.0
EOF
run_expected_input_error_case "coupled_stub_distance_range" "${case_distance_range}" \
    "MBD_INPUT_ERROR\\[E_DISTANCE_RANGE\\] MBD_DISTANCE at line .* references body outside supported range"

case_revolute_range="${tmp_dir}/t3_with_revolute_range.dat"
cp "examples/t3_cantilever_beam.dat" "${case_revolute_range}"
cat >>"${case_revolute_range}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_REVOLUTE 2 0 8 0.0 0.0 0.0 0.0
EOF
run_expected_input_error_case "coupled_stub_revolute_range" "${case_revolute_range}" \
    "MBD_INPUT_ERROR\\[E_REVOLUTE_RANGE\\] MBD_REVOLUTE at line .* references body outside supported range"

case_unsupported_directive="${tmp_dir}/t3_with_unsupported_directive.dat"
cp "examples/t3_cantilever_beam.dat" "${case_unsupported_directive}"
cat >>"${case_unsupported_directive}" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_FOO 1 0 1
EOF
run_expected_input_error_case "coupled_stub_unsupported_directive" "${case_unsupported_directive}" \
    "MBD_INPUT_ERROR\\[E_UNSUPPORTED_DIRECTIVE\\] Unsupported MBD directive at line"

echo "PASS: coupled stub contract check (snapshot path + integrator/parameter switch + precedence + invalid-input boundaries)"
