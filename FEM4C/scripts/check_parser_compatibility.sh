#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}

lock_dir="${TMPDIR:-/tmp}/fem4c_parser_compat.lock"
if ! mkdir "${lock_dir}" 2>/dev/null; then
    echo "FAIL: parser compatibility check is already running (lock: ${lock_dir})" >&2
    exit 1
fi

cleanup_lock() {
    rmdir "${lock_dir}" 2>/dev/null || true
}

trap 'cleanup; cleanup_lock' EXIT

build_fallback_legacy_pkg() {
    local pkg_dir="$1"
    mkdir -p "${pkg_dir}/mesh" "${pkg_dir}/material" "${pkg_dir}/Boundary Conditions"

    cat >"${pkg_dir}/mesh/mesh.dat" <<'EOF'
Total number of nodes [-]
3
Total number of elements [-]
1
Element type
CTRIA3
nodes
1, 0.0, 0.0, 0.0
2, 1.0, 0.0, 0.0
3, 0.0, 1.0, 0.0
elements
1, 1, 2, 3
EOF

    cat >"${pkg_dir}/material/material.dat" <<'EOF'
Young's modulus [N/mm^2]
2.0e5
Poisson's ratio [-]
0.3
density [kg/mm^3]
7.8e-6
EOF

    cat >"${pkg_dir}/Boundary Conditions/boundary.dat" <<'EOF'
Total number of Boundary Conditions [-]
2
SPC SID=1 G=1 C=12 D=0.0
FORCE SID=2 G=3 F=1000.0 N=(0.0,1.0,0.0)
EOF
}

resolve_old_pkg() {
    if [[ "${FEM4C_PARSER_COMPAT_FORCE_FALLBACK:-0}" == "1" ]]; then
        local forced_fallback="${tmp_dir}/parser_pkg_old_forced_fallback"
        build_fallback_legacy_pkg "${forced_fallback}"
        echo "${forced_fallback}"
        return
    fi

    if [[ -n "${FEM4C_OLD_PARSER_PKG:-}" ]]; then
        if [[ ! -d "${FEM4C_OLD_PARSER_PKG}" ]]; then
            echo "FAIL: FEM4C_OLD_PARSER_PKG not found: ${FEM4C_OLD_PARSER_PKG}" >&2
            exit 1
        fi
        echo "${FEM4C_OLD_PARSER_PKG}"
        return
    fi

    if [[ -d "/tmp/parser_pkg_old" ]]; then
        echo "/tmp/parser_pkg_old"
        return
    fi

    local fallback_pkg="${tmp_dir}/parser_pkg_old_fallback"
    build_fallback_legacy_pkg "${fallback_pkg}"
    echo "${fallback_pkg}"
}

old_log="${tmp_dir}/parser_old.log"
nastran_log="${tmp_dir}/parser_nastran.log"
old_out="${tmp_dir}/parser_old_out.dat"
nastran_out="${tmp_dir}/parser_nastran_out.dat"
OLD_PKG="$(resolve_old_pkg)"

cd "${FEM4C_DIR}"

./bin/fem4c "${OLD_PKG}" "${old_out}" >"${old_log}"
./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 "${nastran_out}" >"${nastran_log}"

grep -q "parser boundary cards: SPC legacy=1 fixed=0, FORCE legacy=1 fixed=0" "${old_log}"
grep -q "Applied 2 boundary conditions" "${old_log}"
grep -q "Total applied force magnitude: 1.000000e+03" "${old_log}"

grep -q "Applied 40 boundary conditions" "${nastran_log}"
grep -q "Total applied force magnitude: 1.000000e+01" "${nastran_log}"

echo "PASS: parser compatibility checks (legacy SPC/FORCE + Nastran parser path)"
echo "PARSER_COMPAT_OLD_PKG=${OLD_PKG}"
