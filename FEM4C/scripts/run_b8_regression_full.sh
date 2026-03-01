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
b8_make_timeout_sec="${B8_MAKE_TIMEOUT_SEC:-0}"
b8_run_b14_regression="${B8_RUN_B14_REGRESSION:-1}"
b8_b14_target="${B8_B14_TARGET:-mbd_ci_contract}"
b8_local_target="${B8_LOCAL_TARGET:-}"
b8_regression_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"
b8_regression_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"
b8_regression_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"
b8_regression_lock_dir="${B8_REGRESSION_LOCK_DIR:-}"
b8_regression_lock_dir_source="env"
b8_lock_repo_hash=""
b8_test_retry_used=0
b8_test_retry_reason="none"

if [[ "$b8_run_b14_regression" != "0" && "$b8_run_b14_regression" != "1" ]]; then
  echo "FAIL: B8_RUN_B14_REGRESSION must be 0 or 1 (got: $b8_run_b14_regression)" >&2
  exit 2
fi

if ! [[ "$b8_make_timeout_sec" =~ ^[0-9]+$ ]]; then
  echo "FAIL: B8_MAKE_TIMEOUT_SEC must be a non-negative integer (got: $b8_make_timeout_sec)" >&2
  exit 2
fi

if [[ "$b8_regression_skip_lock" != "0" && "$b8_regression_skip_lock" != "1" ]]; then
  echo "FAIL: B8_REGRESSION_SKIP_LOCK must be 0 or 1 (got: $b8_regression_skip_lock)" >&2
  exit 2
fi

if [[ "$b8_regression_lock_scope" != "repo" && "$b8_regression_lock_scope" != "global" ]]; then
  echo "FAIL: B8_REGRESSION_LOCK_SCOPE must be repo or global (got: $b8_regression_lock_scope)" >&2
  exit 2
fi

if [[ -z "$b8_regression_lock_dir" ]]; then
  if [[ "$b8_regression_lock_scope" == "repo" ]]; then
    b8_lock_repo_hash="$(printf '%s\n' "$root_dir" | cksum | awk '{print $1}')"
    b8_regression_lock_dir="/tmp/fem4c_b8_regression.${b8_lock_repo_hash}.lock"
    b8_regression_lock_dir_source="scope_repo_default"
  else
    b8_regression_lock_dir="$b8_regression_lock_dir_default_global"
    b8_regression_lock_dir_source="scope_global_default"
  fi
fi

if ! command -v "$b8_make_cmd" >/dev/null 2>&1; then
  echo "FAIL: B8_MAKE_CMD is not executable (got: $b8_make_cmd)" >&2
  exit 2
fi

if [[ "$b8_make_timeout_sec" -gt 0 ]] && ! command -v timeout >/dev/null 2>&1; then
  echo "FAIL: timeout command is required when B8_MAKE_TIMEOUT_SEC > 0" >&2
  exit 2
fi

run_make_target() {
  local target="$1"
  local rc=0

  if [[ "$b8_make_timeout_sec" -gt 0 ]]; then
    set +e
    env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR -u B8_LOCAL_TARGET -u B8_REGRESSION_SKIP_LOCK -u B8_REGRESSION_LOCK_SCOPE -u B8_REGRESSION_LOCK_DIR \
      timeout --foreground "$b8_make_timeout_sec" \
      "$b8_make_cmd" -j1 -C FEM4C "$target"
    rc=$?
    set -e
    if [[ "$rc" -eq 124 ]]; then
      echo "FAIL: make target timed out (target=$target timeout_sec=$b8_make_timeout_sec)" >&2
    fi
    return "$rc"
  fi

  env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR -u B8_LOCAL_TARGET -u B8_REGRESSION_SKIP_LOCK -u B8_REGRESSION_LOCK_SCOPE -u B8_REGRESSION_LOCK_DIR \
    "$b8_make_cmd" -j1 -C FEM4C "$target"
}

run_b8_regression_target() {
  local rc=0

  if [[ "$b8_make_timeout_sec" -gt 0 ]]; then
    set +e
    B8_MAKE_CMD="$b8_make_cmd" \
    B8_MAKE_TIMEOUT_SEC="$b8_make_timeout_sec" \
    B8_RUN_B14_REGRESSION="$b8_run_b14_regression" \
    B8_B14_TARGET="$b8_b14_target" \
    B8_REGRESSION_SKIP_LOCK="$b8_regression_skip_lock" \
    B8_REGRESSION_LOCK_SCOPE="$b8_regression_lock_scope" \
    B8_REGRESSION_LOCK_DIR="$b8_regression_lock_dir" \
      env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR -u B8_LOCAL_TARGET \
      B8_LOCAL_TARGET="$b8_local_target" \
      timeout --foreground "$b8_make_timeout_sec" \
      "$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression
    rc=$?
    set -e
    if [[ "$rc" -eq 124 ]]; then
      echo "FAIL: make target timed out (target=mbd_b8_regression timeout_sec=$b8_make_timeout_sec)" >&2
    fi
    return "$rc"
  fi

  B8_MAKE_CMD="$b8_make_cmd" \
  B8_MAKE_TIMEOUT_SEC="$b8_make_timeout_sec" \
  B8_RUN_B14_REGRESSION="$b8_run_b14_regression" \
  B8_B14_TARGET="$b8_b14_target" \
  B8_REGRESSION_SKIP_LOCK="$b8_regression_skip_lock" \
  B8_REGRESSION_LOCK_SCOPE="$b8_regression_lock_scope" \
  B8_REGRESSION_LOCK_DIR="$b8_regression_lock_dir" \
    env -u MAKEFLAGS -u MFLAGS -u B8_TEST_TMP_COPY_DIR -u B8_LOCAL_TARGET \
    B8_LOCAL_TARGET="$b8_local_target" \
    "$b8_make_cmd" -j1 -C FEM4C mbd_b8_regression
}

run_test_with_parser_retry() {
  local rc=0

  set +e
  run_make_target test
  rc=$?
  set -e
  if [[ "$rc" -eq 0 ]]; then
    return 0
  fi

  if [[ "$b8_make_cmd" == "make" || "$b8_make_cmd" == */make ]]; then
    if [[ ! -x "FEM4C/parser/parser" && ! -x "FEM4C/parser/parser.exe" ]]; then
      b8_test_retry_used=1
      b8_test_retry_reason="parser_missing"
      echo "INFO: parser executable missing after test failure; rebuilding via make all and retrying test once"
      run_make_target all
      run_make_target test
      return 0
    fi
  fi

  return "$rc"
}

run_make_target clean
run_make_target all
if [[ "$b8_make_cmd" == "make" || "$b8_make_cmd" == */make ]]; then
  if [[ ! -x "FEM4C/parser/parser" && ! -x "FEM4C/parser/parser.exe" ]]; then
    echo "INFO: parser executable missing before test; rebuilding via make all"
    run_make_target all
  fi
fi
run_test_with_parser_retry
run_b8_regression_target

if [[ "$b8_test_retry_used" == "0" && "$b8_test_retry_reason" != "none" ]]; then
  echo "FAIL: inconsistent retry state (test_retry_used=0 requires test_retry_reason=none, got: $b8_test_retry_reason)" >&2
  exit 2
fi

if [[ "$b8_test_retry_used" == "1" && "$b8_test_retry_reason" != "parser_missing" ]]; then
  echo "FAIL: inconsistent retry state (test_retry_used=1 requires test_retry_reason=parser_missing, got: $b8_test_retry_reason)" >&2
  exit 2
fi

echo "PASS: b8 full regression (clean rebuild + b8 regression; run_b14_regression=$b8_run_b14_regression b14_target=$b8_b14_target local_target=$b8_local_target b8_skip_lock=$b8_regression_skip_lock b8_lock_scope=$b8_regression_lock_scope b8_lock_dir=$b8_regression_lock_dir b8_lock_dir_source=$b8_regression_lock_dir_source make_timeout_sec=$b8_make_timeout_sec test_retry_used=$b8_test_retry_used test_retry_reason=$b8_test_retry_reason)"
