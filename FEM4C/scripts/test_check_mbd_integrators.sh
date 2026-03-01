#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"
fem4c_dir="${root_dir}/FEM4C"

tmp_dir="$(mktemp -d)"
script_copy=""
cleanup() {
  if [[ -n "${script_copy}" && -f "${script_copy}" ]]; then
    rm -f "${script_copy}"
  fi
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

if ! make -C FEM4C mbd_integrator_checks >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: mbd_integrator_checks should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

script_copy="${fem4c_dir}/scripts/.tmp_check_mbd_integrators_fail.sh"
cp "${fem4c_dir}/scripts/check_mbd_integrators.sh" "${script_copy}"

# Break one expected marker so we can verify the checker fails with diagnostics.
sed -i 's/MBD integrator source: cli/MBD integrator source: broken_marker_for_test/' "${script_copy}"

if bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified checker should fail when expected marker is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "missing expected log pattern" "${tmp_dir}/fail.log"; then
  echo "FAIL: failure output did not include missing-pattern diagnostic" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if FEM4C_MBD_BIN="${tmp_dir}/missing_fem4c_bin" bash "${fem4c_dir}/scripts/check_mbd_integrators.sh" >"${tmp_dir}/missing_bin.log" 2>&1; then
  echo "FAIL: checker should fail when FEM4C_MBD_BIN points to a non-executable path" >&2
  cat "${tmp_dir}/missing_bin.log" >&2
  exit 1
fi

if ! grep -q "requires executable fem4c binary" "${tmp_dir}/missing_bin.log"; then
  echo "FAIL: missing-bin path did not emit preflight diagnostic" >&2
  cat "${tmp_dir}/missing_bin.log" >&2
  exit 1
fi

echo "PASS: check_mbd_integrators self-test (pass + expected fail with diagnostics + missing-bin preflight)"
