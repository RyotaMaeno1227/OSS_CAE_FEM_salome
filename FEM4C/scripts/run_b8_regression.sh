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
b8_skip_lock="${B8_REGRESSION_SKIP_LOCK:-0}"
b8_lock_scope="${B8_REGRESSION_LOCK_SCOPE:-repo}"
b8_lock_dir_default_global="/tmp/fem4c_b8_regression.lock"
b8_lock_dir="${B8_REGRESSION_LOCK_DIR:-}"
b8_lock_dir_source="env"
b8_lock_repo_hash=""
b8_lock_pid_file="${b8_lock_dir}/pid"
b8_lock_status="not_used"

if [[ "$b8_run_b14_regression" != "0" && "$b8_run_b14_regression" != "1" ]]; then
  echo "FAIL: B8_RUN_B14_REGRESSION must be 0 or 1 (got: $b8_run_b14_regression)" >&2
  exit 2
fi

if ! [[ "$b8_make_timeout_sec" =~ ^[0-9]+$ ]]; then
  echo "FAIL: B8_MAKE_TIMEOUT_SEC must be a non-negative integer (got: $b8_make_timeout_sec)" >&2
  exit 2
fi

if [[ "$b8_skip_lock" != "0" && "$b8_skip_lock" != "1" ]]; then
  echo "FAIL: B8_REGRESSION_SKIP_LOCK must be 0 or 1 (got: $b8_skip_lock)" >&2
  exit 2
fi

if [[ "$b8_lock_scope" != "repo" && "$b8_lock_scope" != "global" ]]; then
  echo "FAIL: B8_REGRESSION_LOCK_SCOPE must be repo or global (got: $b8_lock_scope)" >&2
  exit 2
fi

if [[ -z "$b8_lock_dir" ]]; then
  if [[ "$b8_lock_scope" == "repo" ]]; then
    b8_lock_repo_hash="$(printf '%s\n' "$root_dir" | cksum | awk '{print $1}')"
    b8_lock_dir="/tmp/fem4c_b8_regression.${b8_lock_repo_hash}.lock"
    b8_lock_dir_source="scope_repo_default"
  else
    b8_lock_dir="$b8_lock_dir_default_global"
    b8_lock_dir_source="scope_global_default"
  fi
fi
b8_lock_pid_file="${b8_lock_dir}/pid"

if ! command -v "$b8_make_cmd" >/dev/null 2>&1; then
  echo "FAIL: B8_MAKE_CMD is not executable (got: $b8_make_cmd)" >&2
  exit 2
fi

if [[ "$b8_make_timeout_sec" -gt 0 ]] && ! command -v timeout >/dev/null 2>&1; then
  echo "FAIL: timeout command is required when B8_MAKE_TIMEOUT_SEC > 0" >&2
  exit 2
fi

cleanup() {
  if [[ "${b8_lock_status}" == "acquired" || "${b8_lock_status}" == "acquired_stale_recovered" ]]; then
    rm -f "${b8_lock_pid_file}" 2>/dev/null || true
    rmdir "${b8_lock_dir}" 2>/dev/null || true
  fi
}

acquire_lock() {
  local owner_pid=""
  if mkdir "${b8_lock_dir}" 2>/dev/null; then
    echo "$$" >"${b8_lock_pid_file}"
    b8_lock_status="acquired"
    return 0
  fi

  if [[ -f "${b8_lock_pid_file}" ]]; then
    owner_pid="$(cat "${b8_lock_pid_file}" 2>/dev/null || true)"
    if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then
      rm -rf "${b8_lock_dir}" 2>/dev/null || true
      if mkdir "${b8_lock_dir}" 2>/dev/null; then
        echo "$$" >"${b8_lock_pid_file}"
        b8_lock_status="acquired_stale_recovered"
        echo "INFO: recovered stale b8 regression lock (${b8_lock_dir})"
        return 0
      fi
    fi
  else
    rm -rf "${b8_lock_dir}" 2>/dev/null || true
    if mkdir "${b8_lock_dir}" 2>/dev/null; then
      echo "$$" >"${b8_lock_pid_file}"
      b8_lock_status="acquired_stale_recovered"
      echo "INFO: recovered stale b8 regression lock without pid (${b8_lock_dir})"
      return 0
    fi
  fi

  return 1
}

run_make_target() {
  local target="$1"
  local rc=0

  if [[ "$b8_make_timeout_sec" -gt 0 ]]; then
    set +e
    env -u MAKEFLAGS -u MFLAGS \
      -u B8_TEST_TMP_COPY_DIR \
      -u B8_LOCAL_TARGET \
      -u B8_B14_TARGET \
      -u B8_RUN_B14_REGRESSION \
      timeout --foreground "$b8_make_timeout_sec" \
      "$b8_make_cmd" -C FEM4C "$target"
    rc=$?
    set -e
    if [[ "$rc" -eq 124 ]]; then
      echo "FAIL: make target timed out (target=$target timeout_sec=$b8_make_timeout_sec)" >&2
    fi
    return "$rc"
  fi

  env -u MAKEFLAGS -u MFLAGS \
    -u B8_TEST_TMP_COPY_DIR \
    -u B8_LOCAL_TARGET \
    -u B8_B14_TARGET \
    -u B8_RUN_B14_REGRESSION \
    "$b8_make_cmd" -C FEM4C "$target"
}

run_guard_contract_target() {
  local rc=0

  if [[ "$b8_make_timeout_sec" -gt 0 ]]; then
    set +e
    env -u MAKEFLAGS -u MFLAGS \
      -u B8_TEST_TMP_COPY_DIR \
      B8_LOCAL_TARGET="$b8_local_target" \
      RUN_B14_REGRESSION="$b8_run_b14_regression" \
      B8_B14_TARGET="$b8_b14_target" \
      timeout --foreground "$b8_make_timeout_sec" \
      "$b8_make_cmd" -C FEM4C mbd_b8_guard_contract
    rc=$?
    set -e
    if [[ "$rc" -eq 124 ]]; then
      echo "FAIL: make target timed out (target=mbd_b8_guard_contract timeout_sec=$b8_make_timeout_sec)" >&2
    fi
    return "$rc"
  fi

  env -u MAKEFLAGS -u MFLAGS \
    -u B8_TEST_TMP_COPY_DIR \
    B8_LOCAL_TARGET="$b8_local_target" \
    RUN_B14_REGRESSION="$b8_run_b14_regression" \
    B8_B14_TARGET="$b8_b14_target" \
    "$b8_make_cmd" -C FEM4C mbd_b8_guard_contract
}

if [[ "${b8_skip_lock}" == "1" ]]; then
  b8_lock_status="skipped"
else
  if ! acquire_lock; then
    echo "FAIL: b8 regression lock is already held (${b8_lock_dir})" >&2
    exit 2
  fi
fi
trap cleanup EXIT

run_make_target mbd_b8_syntax
run_make_target mbd_b8_guard_output_test
run_make_target mbd_ci_contract
# mbd_b8_guard_contract_test already exercises mbd_ci_contract_test via wrapper self-test.
# Keep one execution path to reduce re-entry overlap during nested B-8 regression runs.
run_make_target mbd_b8_guard_test
run_make_target mbd_b8_guard_contract_test
run_guard_contract_target

echo "PASS: b8 regression (contract + self-tests + guard-contract; run_b14_regression=$b8_run_b14_regression b14_target=$b8_b14_target local_target=$b8_local_target lock=$b8_lock_status lock_scope=$b8_lock_scope lock_dir=$b8_lock_dir lock_dir_source=$b8_lock_dir_source make_timeout_sec=$b8_make_timeout_sec)"
