#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

make -C FEM4C mbd_ci_contract
make -C FEM4C mbd_ci_contract_test
make -C FEM4C mbd_integrator_checks_test
make -C FEM4C test

echo "PASS: b14 regression (mbd entry integration + contract + self-tests + local test entry)"
