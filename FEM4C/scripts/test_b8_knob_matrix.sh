#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"
repo_lock_hash="$(printf '%s\n' "${root_dir}" | cksum | awk '{print $1}')"
repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"
global_default_lock_dir="/tmp/fem4c_b8_regression.lock"

b8_knob_matrix_skip_full="${B8_KNOB_MATRIX_SKIP_FULL:-0}"
if [[ "$b8_knob_matrix_skip_full" != "0" && "$b8_knob_matrix_skip_full" != "1" ]]; then
  echo "FAIL: B8_KNOB_MATRIX_SKIP_FULL must be 0 or 1 (got: $b8_knob_matrix_skip_full)" >&2
  exit 2
fi

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

# Pass local target via environment so wrapper scripts consume it consistently.
export B8_LOCAL_TARGET="${B8_LOCAL_TARGET:-mbd_b8_syntax}"
full_cleanup_expected_calls=0
if [[ "$b8_knob_matrix_skip_full" == "0" ]]; then
  full_cleanup_expected_calls=7
fi
full_cleanup_expected_order_trace=""
for ((cleanup_idx=0; cleanup_idx<full_cleanup_expected_calls; cleanup_idx++)); do
  if [[ -z "${full_cleanup_expected_order_trace}" ]]; then
    full_cleanup_expected_order_trace="parser,b8"
  else
    full_cleanup_expected_order_trace="${full_cleanup_expected_order_trace},parser,b8"
  fi
done
parser_cleanup_call_count=0
b8_cleanup_call_count=0
cleanup_call_order_trace=""

append_cleanup_call_order_trace() {
  local cleanup_name="$1"
  if [[ -z "${cleanup_call_order_trace}" ]]; then
    cleanup_call_order_trace="${cleanup_name}"
  else
    cleanup_call_order_trace="${cleanup_call_order_trace},${cleanup_name}"
  fi
}

cleanup_parser_compat_lock() {
  parser_cleanup_call_count=$((parser_cleanup_call_count + 1))
  append_cleanup_call_order_trace "parser"
  rm -rf /tmp/fem4c_parser_compat.lock 2>/dev/null || true
}

cleanup_b8_regression_locks() {
  b8_cleanup_call_count=$((b8_cleanup_call_count + 1))
  append_cleanup_call_order_trace "b8"
  rm -rf "${repo_default_lock_dir}" "${global_default_lock_dir}" 2>/dev/null || true
}

if ! make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make >"${tmp_dir}/regression_0.log" 2>&1; then
  echo "FAIL: b8 regression should pass with B8_RUN_B14_REGRESSION=0" >&2
  cat "${tmp_dir}/regression_0.log" >&2
  exit 1
fi

if ! grep -q "b14_regression_requested=no" "${tmp_dir}/regression_0.log"; then
  echo "FAIL: expected b14_regression_requested=no for B8_RUN_B14_REGRESSION=0" >&2
  cat "${tmp_dir}/regression_0.log" >&2
  exit 1
fi

if ! make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make >"${tmp_dir}/regression_1.log" 2>&1; then
  echo "FAIL: b8 regression should pass with B8_RUN_B14_REGRESSION=1" >&2
  cat "${tmp_dir}/regression_1.log" >&2
  exit 1
fi

if ! grep -q "b14_regression_requested=yes" "${tmp_dir}/regression_1.log"; then
  echo "FAIL: expected b14_regression_requested=yes for B8_RUN_B14_REGRESSION=1" >&2
  cat "${tmp_dir}/regression_1.log" >&2
  exit 1
fi

if ! grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}/regression_1.log"; then
  echo "FAIL: expected local_target=${B8_LOCAL_TARGET} marker in b8 regression summary output" >&2
  cat "${tmp_dir}/regression_1.log" >&2
  exit 1
fi

if ! make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make >"${tmp_dir}/regression_repo_default_lock_scope.log" 2>&1; then
  echo "FAIL: b8 regression should pass with repo default lock scope trace case" >&2
  cat "${tmp_dir}/regression_repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=scope_repo_default" "${tmp_dir}/regression_repo_default_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=scope_repo_default in regression repo default lock scope case" >&2
  cat "${tmp_dir}/regression_repo_default_lock_scope.log" >&2
  exit 1
fi

rm -rf /tmp/fem4c_b8_regression.lock 2>/dev/null || true
if ! make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make >"${tmp_dir}/regression_global_default_lock_scope.log" 2>&1; then
  echo "FAIL: b8 regression should pass with global default lock scope trace case" >&2
  cat "${tmp_dir}/regression_global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=scope_global_default" "${tmp_dir}/regression_global_default_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=scope_global_default in regression global default lock scope case" >&2
  cat "${tmp_dir}/regression_global_default_lock_scope.log" >&2
  exit 1
fi

regression_env_lock_dir="${tmp_dir}/regression_env_lock_scope.lock"
if ! make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${regression_env_lock_dir}" B8_MAKE_CMD=make >"${tmp_dir}/regression_env_lock_scope.log" 2>&1; then
  echo "FAIL: b8 regression should pass with env lock_dir trace case" >&2
  cat "${tmp_dir}/regression_env_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=env" "${tmp_dir}/regression_env_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=env in regression env lock_dir case" >&2
  cat "${tmp_dir}/regression_env_lock_scope.log" >&2
  exit 1
fi

if make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make >"${tmp_dir}/invalid_knob.log" 2>&1; then
  echo "FAIL: b8 regression should fail for invalid B8_RUN_B14_REGRESSION" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if ! grep -q "B8_RUN_B14_REGRESSION must be 0 or 1" "${tmp_dir}/invalid_knob.log"; then
  echo "FAIL: expected invalid B8_RUN_B14_REGRESSION diagnostic" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__ >"${tmp_dir}/invalid_make.log" 2>&1; then
  echo "FAIL: b8 regression should fail for invalid B8_MAKE_CMD" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if ! grep -q "B8_MAKE_CMD is not executable" "${tmp_dir}/invalid_make.log"; then
  echo "FAIL: expected invalid B8_MAKE_CMD diagnostic" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if [[ "$b8_knob_matrix_skip_full" == "0" ]]; then
  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if ! make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=0 B8_MAKE_CMD=make >"${tmp_dir}/full_0.log" 2>&1; then
    echo "FAIL: b8 full regression should pass with B8_RUN_B14_REGRESSION=0" >&2
    cat "${tmp_dir}/full_0.log" >&2
    exit 1
  fi

  if ! grep -q "run_b14_regression=0" "${tmp_dir}/full_0.log"; then
    echo "FAIL: expected run_b14_regression=0 marker in full regression" >&2
    cat "${tmp_dir}/full_0.log" >&2
    exit 1
  fi

  if ! grep -Eq "test_retry_reason=(none|parser_missing)" "${tmp_dir}/full_0.log"; then
    echo "FAIL: expected test_retry_reason=none|parser_missing marker in full regression (run_b14_regression=0)" >&2
    cat "${tmp_dir}/full_0.log" >&2
    exit 1
  fi

  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if ! make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=make >"${tmp_dir}/full_1.log" 2>&1; then
    echo "FAIL: b8 full regression should pass with B8_RUN_B14_REGRESSION=1" >&2
    cat "${tmp_dir}/full_1.log" >&2
    exit 1
  fi

  if ! grep -q "run_b14_regression=1" "${tmp_dir}/full_1.log"; then
    echo "FAIL: expected run_b14_regression=1 marker in full regression" >&2
    cat "${tmp_dir}/full_1.log" >&2
    exit 1
  fi

  if ! grep -Eq "test_retry_reason=(none|parser_missing)" "${tmp_dir}/full_1.log"; then
    echo "FAIL: expected test_retry_reason=none|parser_missing marker in full regression (run_b14_regression=1)" >&2
    cat "${tmp_dir}/full_1.log" >&2
    exit 1
  fi

  if ! grep -Fq "local_target=${B8_LOCAL_TARGET}" "${tmp_dir}/full_1.log"; then
    echo "FAIL: expected local_target=${B8_LOCAL_TARGET} marker in b8 full regression summary output" >&2
    cat "${tmp_dir}/full_1.log" >&2
    exit 1
  fi

  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if ! make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=repo B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make >"${tmp_dir}/full_repo_default_lock_scope.log" 2>&1; then
    echo "FAIL: b8 full regression should pass with repo default lock scope trace case" >&2
    cat "${tmp_dir}/full_repo_default_lock_scope.log" >&2
    exit 1
  fi

  if ! grep -q "b8_lock_dir_source=scope_repo_default" "${tmp_dir}/full_repo_default_lock_scope.log"; then
    echo "FAIL: expected b8_lock_dir_source=scope_repo_default in full repo default lock scope case" >&2
    cat "${tmp_dir}/full_repo_default_lock_scope.log" >&2
    exit 1
  fi

  rm -rf /tmp/fem4c_b8_regression.lock 2>/dev/null || true
  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if ! make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR= B8_MAKE_CMD=make >"${tmp_dir}/full_global_default_lock_scope.log" 2>&1; then
    echo "FAIL: b8 full regression should pass with global default lock scope trace case" >&2
    cat "${tmp_dir}/full_global_default_lock_scope.log" >&2
    exit 1
  fi

  if ! grep -q "b8_lock_dir_source=scope_global_default" "${tmp_dir}/full_global_default_lock_scope.log"; then
    echo "FAIL: expected b8_lock_dir_source=scope_global_default in full global default lock scope case" >&2
    cat "${tmp_dir}/full_global_default_lock_scope.log" >&2
    exit 1
  fi

  full_env_lock_dir="${tmp_dir}/full_env_lock_scope.lock"
  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if ! make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_REGRESSION_LOCK_DIR="${full_env_lock_dir}" B8_MAKE_CMD=make >"${tmp_dir}/full_env_lock_scope.log" 2>&1; then
    echo "FAIL: b8 full regression should pass with env lock_dir trace case" >&2
    cat "${tmp_dir}/full_env_lock_scope.log" >&2
    exit 1
  fi

  if ! grep -q "b8_lock_dir_source=env" "${tmp_dir}/full_env_lock_scope.log"; then
    echo "FAIL: expected b8_lock_dir_source=env in full env lock_dir case" >&2
    cat "${tmp_dir}/full_env_lock_scope.log" >&2
    exit 1
  fi

  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=2 B8_MAKE_CMD=make >"${tmp_dir}/invalid_knob_full.log" 2>&1; then
    echo "FAIL: b8 full regression should fail for invalid B8_RUN_B14_REGRESSION" >&2
    cat "${tmp_dir}/invalid_knob_full.log" >&2
    exit 1
  fi

  if ! grep -q "B8_RUN_B14_REGRESSION must be 0 or 1" "${tmp_dir}/invalid_knob_full.log"; then
    echo "FAIL: expected invalid B8_RUN_B14_REGRESSION diagnostic in full regression" >&2
    cat "${tmp_dir}/invalid_knob_full.log" >&2
    exit 1
  fi

  cleanup_parser_compat_lock
  cleanup_b8_regression_locks
  if make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=1 B8_MAKE_CMD=__missing_make__ >"${tmp_dir}/invalid_make_full.log" 2>&1; then
    echo "FAIL: b8 full regression should fail for invalid B8_MAKE_CMD" >&2
    cat "${tmp_dir}/invalid_make_full.log" >&2
    exit 1
  fi

  if ! grep -q "B8_MAKE_CMD is not executable" "${tmp_dir}/invalid_make_full.log"; then
    echo "FAIL: expected invalid B8_MAKE_CMD diagnostic in full regression" >&2
    cat "${tmp_dir}/invalid_make_full.log" >&2
    exit 1
  fi
else
  echo "INFO: skip full regression matrix cases (B8_KNOB_MATRIX_SKIP_FULL=1)"
fi

if [[ "${parser_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" ]]; then
  echo "FAIL: expected parser cleanup call count=${full_cleanup_expected_calls}, got=${parser_cleanup_call_count}" >&2
  exit 1
fi

if [[ "${b8_cleanup_call_count}" -ne "${full_cleanup_expected_calls}" ]]; then
  echo "FAIL: expected b8 cleanup call count=${full_cleanup_expected_calls}, got=${b8_cleanup_call_count}" >&2
  exit 1
fi

if [[ "${cleanup_call_order_trace}" != "${full_cleanup_expected_order_trace}" ]]; then
  echo "FAIL: expected cleanup call order trace=${full_cleanup_expected_order_trace}, got=${cleanup_call_order_trace}" >&2
  exit 1
fi

echo "INFO: full cleanup call count parser=${parser_cleanup_call_count} b8=${b8_cleanup_call_count} expected=${full_cleanup_expected_calls}"
echo "INFO: full cleanup call order trace=${cleanup_call_order_trace} expected=${full_cleanup_expected_order_trace}"
echo "PASS: b8 knob matrix test (B8_RUN_B14_REGRESSION and B8_MAKE_CMD, skip_full=${b8_knob_matrix_skip_full})"
