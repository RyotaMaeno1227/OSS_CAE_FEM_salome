#!/usr/bin/env bash
set -euo pipefail

root_dir="${FEM4C_REPO_ROOT:-}"
if [[ -z "${root_dir}" ]]; then
  root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
fi
if [[ ! -d "${root_dir}/FEM4C/scripts" ]]; then
  echo "FAIL: FEM4C repo root is invalid (got: ${root_dir})" >&2
  exit 2
fi
cd "$root_dir"

b8_make_cmd="${B8_MAKE_CMD:-make}"
b8_run_b14_regression="${B8_RUN_B14_REGRESSION:-1}"
b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"

if [[ "$b8_run_b14_regression" != "0" && "$b8_run_b14_regression" != "1" ]]; then
  echo "FAIL: B8_RUN_B14_REGRESSION must be 0 or 1 (got: $b8_run_b14_regression)" >&2
  exit 2
fi

if ! command -v "$b8_make_cmd" >/dev/null 2>&1; then
  echo "FAIL: B8_MAKE_CMD is not executable (got: $b8_make_cmd)" >&2
  exit 2
fi

run_make_target() {
  env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR "$b8_make_cmd" -C FEM4C "$1"
}

run_make_target clean
run_make_target all
run_make_target test
B8_MAKE_CMD="$b8_make_cmd" \
B8_RUN_B14_REGRESSION="$b8_run_b14_regression" \
B8_B14_TARGET="$b8_b14_target" \
  env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR "$b8_make_cmd" -C FEM4C mbd_b8_regression

echo "PASS: b8 full regression (clean rebuild + b8 regression; run_b14_regression=$b8_run_b14_regression b14_target=$b8_b14_target)"
