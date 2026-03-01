#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

case_bad_body="${tmp_dir}/bad_body.dat"
case_unknown="${tmp_dir}/unknown_directive.dat"
case_duplicate_body="${tmp_dir}/duplicate_body.dat"
case_undefined_ref="${tmp_dir}/undefined_ref.dat"
case_non_numeric="${tmp_dir}/non_numeric.dat"
case_invalid_value="${tmp_dir}/invalid_value.dat"
case_distance_range="${tmp_dir}/distance_range.dat"
case_body_range="${tmp_dir}/body_range.dat"
case_revolute_range="${tmp_dir}/revolute_range.dat"
case_incomplete="${tmp_dir}/incomplete_input.dat"

cat >"${case_bad_body}" <<'EOF'
TITLE BAD BODY
MBD_BODY 0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_unknown}" <<'EOF'
TITLE UNKNOWN DIRECTIVE
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_SOMETHING 9 1 2
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_duplicate_body}" <<'EOF'
TITLE DUPLICATE BODY
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 0 1.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_undefined_ref}" <<'EOF'
TITLE UNDEFINED BODY REFERENCE
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 2 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_non_numeric}" <<'EOF'
TITLE NON NUMERIC
MBD_BODY 0 abc 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_invalid_value}" <<'EOF'
TITLE INVALID DISTANCE
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 -1.0
EOF

cat >"${case_distance_range}" <<'EOF'
TITLE DISTANCE RANGE
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 8 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_body_range}" <<'EOF'
TITLE BODY RANGE
MBD_BODY 8 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 1 1 0.0 0.0 0.0 0.0 1.0
EOF

cat >"${case_revolute_range}" <<'EOF'
TITLE REVOLUTE RANGE
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_REVOLUTE 2 0 8 0.0 0.0 0.0 0.0
EOF

cat >"${case_incomplete}" <<'EOF'
TITLE INCOMPLETE INPUT
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
EOF

expect_fail() {
    local input_file="$1"
    local expect_pattern="$2"
    local expect_code="$3"
    local log_file="$4"
    local out_file="$5"
    set +e
    "${FEM4C_DIR}/bin/fem4c" --mode=mbd "${input_file}" "${out_file}" >"${log_file}" 2>&1
    local rc=$?
    set -e
    if [[ ${rc} -eq 0 ]]; then
        echo "FAIL: expected non-zero exit for ${input_file}" >&2
        cat "${log_file}" >&2
        exit 1
    fi
    if ! grep -q "${expect_pattern}" "${log_file}"; then
        echo "FAIL: expected pattern '${expect_pattern}' not found for ${input_file}" >&2
        cat "${log_file}" >&2
        exit 1
    fi
    if ! grep -q "MBD_INPUT_ERROR\\[${expect_code}\\]" "${log_file}"; then
        echo "FAIL: expected diagnostic code '${expect_code}' not found for ${input_file}" >&2
        cat "${log_file}" >&2
        exit 1
    fi
}

expect_fail "${case_bad_body}" "MBD_INPUT_ERROR\\[E_BODY_PARSE\\] Invalid MBD_BODY at line 2" "E_BODY_PARSE" "${tmp_dir}/bad_body.log" "${tmp_dir}/bad_body.out"
expect_fail "${case_unknown}" "MBD_INPUT_ERROR\\[E_UNSUPPORTED_DIRECTIVE\\] Unsupported MBD directive at line 4" "E_UNSUPPORTED_DIRECTIVE" "${tmp_dir}/unknown.log" "${tmp_dir}/unknown.out"
expect_fail "${case_duplicate_body}" "MBD_INPUT_ERROR\\[E_DUP_BODY\\] Duplicate MBD_BODY id 0 at line 3" "E_DUP_BODY" "${tmp_dir}/duplicate.log" "${tmp_dir}/duplicate.out"
expect_fail "${case_undefined_ref}" "MBD_INPUT_ERROR\\[E_UNDEFINED_BODY_REF\\] Undefined MBD_BODY 2 referenced" "E_UNDEFINED_BODY_REF" "${tmp_dir}/undefined.log" "${tmp_dir}/undefined.out"
expect_fail "${case_non_numeric}" "MBD_INPUT_ERROR\\[E_BODY_PARSE\\] Invalid MBD_BODY at line 2" "E_BODY_PARSE" "${tmp_dir}/non_numeric.log" "${tmp_dir}/non_numeric.out"
expect_fail "${case_invalid_value}" "MBD_INPUT_ERROR\\[E_DISTANCE_PARSE\\] Invalid MBD_DISTANCE at line 4" "E_DISTANCE_PARSE" "${tmp_dir}/invalid_value.log" "${tmp_dir}/invalid_value.out"
expect_fail "${case_distance_range}" "MBD_INPUT_ERROR\\[E_DISTANCE_RANGE\\] MBD_DISTANCE at line 4 references body outside supported range" "E_DISTANCE_RANGE" "${tmp_dir}/distance_range.log" "${tmp_dir}/distance_range.out"
expect_fail "${case_body_range}" "MBD_INPUT_ERROR\\[E_BODY_RANGE\\] MBD_BODY index 8 at line 2 exceeds supported range" "E_BODY_RANGE" "${tmp_dir}/body_range.log" "${tmp_dir}/body_range.out"
expect_fail "${case_revolute_range}" "MBD_INPUT_ERROR\\[E_REVOLUTE_RANGE\\] MBD_REVOLUTE at line 4 references body outside supported range" "E_REVOLUTE_RANGE" "${tmp_dir}/revolute_range.log" "${tmp_dir}/revolute_range.out"
expect_fail "${case_incomplete}" "MBD_INPUT_ERROR\\[E_INCOMPLETE_INPUT\\] MBD input is incomplete: no constraints found" "E_INCOMPLETE_INPUT" "${tmp_dir}/incomplete.log" "${tmp_dir}/incomplete.out"

diag_codes_seen="$(grep -h -o 'MBD_INPUT_ERROR\[[A-Z_][A-Z_]*\]' "${tmp_dir}"/*.log | sed -E 's/.*\[([A-Z_]+)\]/\1/' | sort -u | paste -sd, -)"

for expected in E_BODY_PARSE E_UNSUPPORTED_DIRECTIVE E_DUP_BODY E_UNDEFINED_BODY_REF E_DISTANCE_PARSE E_DISTANCE_RANGE E_BODY_RANGE E_REVOLUTE_RANGE E_INCOMPLETE_INPUT; do
    if [[ ",${diag_codes_seen}," != *",${expected},"* ]]; then
        echo "FAIL: expected diagnostic code '${expected}' was not present in negative regression logs" >&2
        exit 1
    fi
done

echo "DIAG_CODES_SEEN=${diag_codes_seen}"

echo "PASS: invalid MBD inputs fail with line-aware diagnostics and stable error codes"
