#!/usr/bin/env bash
set -euo pipefail

log_file="${1:-fem4c_test.log}"

if [[ ! -f "${log_file}" ]]; then
  echo "FAIL: missing FEM4C test log: ${log_file}" >&2
  exit 1
fi

require_marker() {
  local marker="$1"
  if ! grep -q -- "${marker}" "${log_file}"; then
    echo "FAIL: missing marker in ${log_file}: ${marker}" >&2
    tail -n 260 "${log_file}" >&2
    exit 1
  fi
}

require_marker "PASS: coupled integrator switch check"
require_marker "PASS: mbd integrator switch check"
require_marker "PASS: all MBD checks completed"

echo "PASS: FEM4C test log markers"
