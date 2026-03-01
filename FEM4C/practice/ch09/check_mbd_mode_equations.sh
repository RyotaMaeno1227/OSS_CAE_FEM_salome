#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FEM4C_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)

probe_log=$(mktemp)
mode_log=$(mktemp)
mbd_out=$(mktemp)
cleanup() {
  rm -f "$probe_log" "$mode_log" "$mbd_out"
}
trap cleanup EXIT

make -C "$FEM4C_ROOT" mbd_probe >"$probe_log"

probe_equations=$(sed -n 's/.*distance+revolute=\([0-9][0-9]*\).*/\1/p' "$probe_log" | tail -n 1)
if [[ -z "${probe_equations:-}" ]]; then
  echo "FAIL: could not parse probe equation count" >&2
  exit 1
fi

"$FEM4C_ROOT/bin/fem4c" --mode=mbd "$FEM4C_ROOT/examples/t6_cantilever_beam.dat" "$mbd_out" >"$mode_log"

mode_equations=$(sed -n 's/.*constraint_equations:[[:space:]]*\([0-9][0-9]*\).*/\1/p' "$mode_log" | tail -n 1)
if [[ -z "${mode_equations:-}" ]]; then
  echo "FAIL: could not parse --mode=mbd equation count" >&2
  exit 1
fi

if [[ "$probe_equations" != "$mode_equations" ]]; then
  echo "FAIL: equation mismatch (probe=$probe_equations, mode=$mode_equations)" >&2
  exit 1
fi

echo "PASS: equation count aligned (probe=$probe_equations, mode=$mode_equations)"
