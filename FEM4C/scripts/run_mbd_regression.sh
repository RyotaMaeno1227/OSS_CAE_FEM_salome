#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

builtin_log="${tmp_dir}/mbd_builtin.log"
input_log="${tmp_dir}/mbd_input.log"
builtin_out="${tmp_dir}/mbd_builtin_out.dat"
input_out="${tmp_dir}/mbd_input_out.dat"
input_case="${tmp_dir}/mbd_input_case.dat"

cat >"${input_case}" <<'EOF'
TITLE MBD INPUT CASE
NODES 0
ELEMENTS 0
MATERIALS 0
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
MBD_REVOLUTE 2 0 1 0.5 0.0 -0.5 0.0
EOF

cd "${FEM4C_DIR}"

./bin/fem4c --mode=mbd examples/t6_cantilever_beam.dat "${builtin_out}" >"${builtin_log}"
./bin/fem4c --mode=mbd "${input_case}" "${input_out}" >"${input_log}"

grep -q "mbd_source: builtin_fallback" "${builtin_log}"
grep -q "constraint_equations: 3" "${builtin_log}"
grep -q "residual_l2:" "${builtin_log}"

grep -q "mbd_source: input_case" "${input_log}"
grep -q "constraint_equations: 3" "${input_log}"
grep -q "residual_l2: 0.000000e+00" "${input_log}"

echo "PASS: mbd regression positive path (builtin fallback + input case)"

negative_log="${tmp_dir}/mbd_negative.log"
bash "${SCRIPT_DIR}/check_mbd_invalid_inputs.sh" | tee "${negative_log}"
grep -q '^DIAG_CODES_SEEN=' "${negative_log}"
grep -q 'E_DUP_BODY' "${negative_log}"
grep -q 'E_UNDEFINED_BODY_REF' "${negative_log}"

echo "PASS: mbd regression (positive + negative diagnostics with stable error codes)"
