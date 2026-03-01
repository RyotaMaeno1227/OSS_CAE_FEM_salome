#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
cleanup() {
  if [[ -n "${script_copy}" && -f "${script_copy}" ]]; then
    rm -f "${script_copy}"
  fi
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

if ! bash "FEM4C/scripts/run_a21_regression.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_a21_regression should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

script_copy="${root_dir}/FEM4C/scripts/.tmp_run_a21_regression_fail.sh"
cp "FEM4C/scripts/run_a21_regression.sh" "${script_copy}"

# Break one make target so we can verify fail-fast behavior.
sed -i 's/make -C FEM4C mbd_ci_contract$/make -C FEM4C mbd_ci_contract_missing/' "${script_copy}"

if bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_a21_regression should fail when command is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "No rule to make target 'mbd_ci_contract_missing'" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-target diagnostic was not found" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

echo "PASS: run_a21_regression self-test (pass + expected fail path)"
