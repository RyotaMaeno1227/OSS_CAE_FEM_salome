#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
script_copy_dir=""
cleanup() {
  if [[ -n "${script_copy}" && -f "${script_copy}" ]]; then
    rm -f "${script_copy}"
  fi
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

script_copy_dir="${B8_TEST_TMP_COPY_DIR:-${root_dir}/FEM4C/scripts}"
if [[ ! -d "${script_copy_dir}" ]]; then
  echo "FAIL: B8_TEST_TMP_COPY_DIR does not exist: ${script_copy_dir}" >&2
  exit 1
fi
if [[ ! -w "${script_copy_dir}" ]]; then
  echo "FAIL: B8_TEST_TMP_COPY_DIR is not writable: ${script_copy_dir}" >&2
  exit 1
fi

# Keep this self-test focused on wrapper plumbing; avoid depending on full B-14 heavy path.
if ! RUN_B14_REGRESSION=1 B8_B14_TARGET=mbd_ci_contract B8_LOCAL_TARGET=mbd_ci_contract bash "FEM4C/scripts/run_b8_guard_contract.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_b8_guard_contract should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

temp_copy_stamp="$$.${RANDOM}"
script_copy="$(mktemp "${script_copy_dir}/.tmp_run_b8_guard_contract_fail.${temp_copy_stamp}.XXXXXX.sh")"
cp "FEM4C/scripts/run_b8_guard_contract.sh" "${script_copy}"

# Break output-contract checker path to verify fail-fast behavior.
sed -i 's#FEM4C/scripts/check_b8_guard_output.sh#FEM4C/scripts/check_b8_guard_output_missing.sh#' "${script_copy}"

if FEM4C_REPO_ROOT="${root_dir}" RUN_B14_REGRESSION=1 B8_B14_TARGET=mbd_ci_contract B8_LOCAL_TARGET=mbd_ci_contract bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_b8_guard_contract should fail when checker path is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "check_b8_guard_output_missing.sh" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-checker diagnostic was not found" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

echo "PASS: run_b8_guard_contract self-test (pass + expected fail path)"
