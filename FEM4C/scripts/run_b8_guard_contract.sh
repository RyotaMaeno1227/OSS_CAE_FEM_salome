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

tmp_log="$(mktemp)"
cleanup() {
  rm -f "${tmp_log}"
}
trap cleanup EXIT

set +e
bash FEM4C/scripts/run_b8_guard.sh >"${tmp_log}" 2>&1
guard_rc=$?
set -e

cat "${tmp_log}"

bash FEM4C/scripts/check_b8_guard_output.sh "${tmp_log}"

if [[ ${guard_rc} -ne 0 ]]; then
  exit "${guard_rc}"
fi

echo "PASS: b8 guard contract wrapper"
