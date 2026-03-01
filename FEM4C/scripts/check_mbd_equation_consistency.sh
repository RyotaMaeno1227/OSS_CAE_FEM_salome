#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEM4C_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

fem_log="${tmp_dir}/fem_mbd.log"
probe_log="${tmp_dir}/probe.log"
fem_out="${tmp_dir}/fem_mbd_out.dat"

cd "${FEM4C_DIR}"

./bin/fem4c --mode=mbd examples/t6_cantilever_beam.dat "${fem_out}" >"${fem_log}"
./bin/mbd_constraint_probe >"${probe_log}"

fem_eq="$(grep -m1 "constraint_equations:" "${fem_log}" | awk '{print $2}')"
probe_eq="$(grep -m1 "distance+revolute=" "${probe_log}" | sed -E 's/.*distance\+revolute=([0-9]+).*/\1/')"

if [[ -z "${fem_eq}" || -z "${probe_eq}" ]]; then
    echo "FAIL: could not parse equation counts" >&2
    echo "--- fem log ---" >&2
    cat "${fem_log}" >&2
    echo "--- probe log ---" >&2
    cat "${probe_log}" >&2
    exit 1
fi

if [[ "${fem_eq}" != "${probe_eq}" ]]; then
    echo "FAIL: equation count mismatch (runtime=${fem_eq}, probe=${probe_eq})" >&2
    exit 1
fi

echo "PASS: equation count consistency (runtime=${fem_eq}, probe=${probe_eq})"
