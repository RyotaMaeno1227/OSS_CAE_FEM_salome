#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
script_copy_dir=""
default_lock_dir="${tmp_dir}/b8_regression_test.lock"
repo_lock_hash="$(printf '%s\n' "${root_dir}" | cksum | awk '{print $1}')"
repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"
global_default_lock_dir="/tmp/fem4c_b8_regression.lock"
export B8_REGRESSION_LOCK_DIR="${default_lock_dir}"
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

if ! B8_LOCAL_TARGET=mbd_ci_contract bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "b14_target=mbd_ci_contract" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression should use mbd_ci_contract as default b14 target" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression should expose local_target=mbd_ci_contract in summary output" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "lock_scope=repo" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression should default lock scope to repo" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=env" "${tmp_dir}/pass.log"; then
  echo "FAIL: expected lock_dir_source=env in baseline lock-dir override path" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! B8_LOCAL_TARGET=mbd_ci_contract B8_RUN_B14_REGRESSION=0 bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/skip_b14.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass when B8_RUN_B14_REGRESSION=0" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b14_regression_requested=no" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b14_regression_requested=no in B8_RUN_B14_REGRESSION=0 path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: run_b8_regression should keep local_target=mbd_ci_contract in skip-b14 summary output" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! B8_LOCAL_TARGET=mbd_ci_contract B8_B14_TARGET=mbd_b8_syntax bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/override_b14_target.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass when B8_B14_TARGET override is set" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if ! grep -q "b14_target=mbd_b8_syntax" "${tmp_dir}/override_b14_target.log"; then
  echo "FAIL: expected overridden b14 target was not found in run_b8_regression output" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/override_b14_target.log"; then
  echo "FAIL: run_b8_regression should keep local_target=mbd_ci_contract in override-b14 summary output" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_RUN_B14_REGRESSION=2 bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/invalid_knob.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when B8_RUN_B14_REGRESSION is invalid" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if ! grep -q "B8_RUN_B14_REGRESSION must be 0 or 1" "${tmp_dir}/invalid_knob.log"; then
  echo "FAIL: expected invalid knob diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_SKIP_LOCK=2 bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/invalid_skip_lock.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when B8_REGRESSION_SKIP_LOCK is invalid" >&2
  cat "${tmp_dir}/invalid_skip_lock.log" >&2
  exit 1
fi

if ! grep -q "B8_REGRESSION_SKIP_LOCK must be 0 or 1" "${tmp_dir}/invalid_skip_lock.log"; then
  echo "FAIL: expected invalid skip-lock diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_skip_lock.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_LOCK_SCOPE=cluster bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/invalid_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when B8_REGRESSION_LOCK_SCOPE is invalid" >&2
  cat "${tmp_dir}/invalid_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "B8_REGRESSION_LOCK_SCOPE must be repo or global" "${tmp_dir}/invalid_lock_scope.log"; then
  echo "FAIL: expected invalid lock-scope diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_lock_scope.log" >&2
  exit 1
fi

if ! B8_LOCAL_TARGET=mbd_ci_contract B8_RUN_B14_REGRESSION=0 B8_REGRESSION_LOCK_SCOPE=global bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/global_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass when B8_REGRESSION_LOCK_SCOPE=global" >&2
  cat "${tmp_dir}/global_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_scope=global" "${tmp_dir}/global_lock_scope.log"; then
  echo "FAIL: expected lock_scope=global trace in global lock-scope path" >&2
  cat "${tmp_dir}/global_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=env" "${tmp_dir}/global_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=env when lock dir override is set" >&2
  cat "${tmp_dir}/global_lock_scope.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_CMD=__missing_make__ bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/invalid_make.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when B8_MAKE_CMD is invalid" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if ! grep -q "B8_MAKE_CMD is not executable" "${tmp_dir}/invalid_make.log"; then
  echo "FAIL: expected invalid make command diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_TIMEOUT_SEC=abc bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/invalid_make_timeout.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when B8_MAKE_TIMEOUT_SEC is non-numeric" >&2
  cat "${tmp_dir}/invalid_make_timeout.log" >&2
  exit 1
fi

if ! grep -q "B8_MAKE_TIMEOUT_SEC must be a non-negative integer" "${tmp_dir}/invalid_make_timeout.log"; then
  echo "FAIL: expected invalid make-timeout diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_make_timeout.log" >&2
  exit 1
fi

slow_make="${tmp_dir}/slow_make.sh"
cat >"${slow_make}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
sleep 2
exit 0
EOF
chmod +x "${slow_make}"

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_CMD="${slow_make}" B8_MAKE_TIMEOUT_SEC=1 B8_RUN_B14_REGRESSION=0 B8_REGRESSION_SKIP_LOCK=1 bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/make_timeout.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail-fast when B8_MAKE_TIMEOUT_SEC is exceeded" >&2
  cat "${tmp_dir}/make_timeout.log" >&2
  exit 1
fi

if ! grep -q "make target timed out (target=mbd_b8_syntax timeout_sec=1)" "${tmp_dir}/make_timeout.log"; then
  echo "FAIL: expected timeout diagnostic was not found in run_b8_regression output" >&2
  cat "${tmp_dir}/make_timeout.log" >&2
  exit 1
fi

mock_make="${tmp_dir}/mock_make.sh"
cat >"${mock_make}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${MAKEFLAGS:-}" || -n "${MFLAGS:-}" ]]; then
  echo "FAIL: MAKEFLAGS/MFLAGS should be unset for recursive make" >&2
  exit 91
fi

args="$*"
if [[ "$args" == *"mbd_b8_syntax"* ]] || \
   [[ "$args" == *"mbd_b8_guard_output_test"* ]] || \
   [[ "$args" == *"mbd_ci_contract"* ]] || \
   [[ "$args" == *"mbd_ci_contract_test"* ]] || \
   [[ "$args" == *"mbd_b8_guard_test"* ]] || \
   [[ "$args" == *"mbd_b8_guard_contract_test"* ]] || \
   [[ "$args" == *"mbd_b8_guard_contract"* ]]; then
  exit 0
fi

echo "FAIL: unexpected make args: ${args}" >&2
exit 90
EOF
chmod +x "${mock_make}"

if ! MAKEFLAGS="--jobs=9" MFLAGS="--jobs=9" B8_MAKE_CMD="${mock_make}" B8_RUN_B14_REGRESSION=1 bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/makeflags.log" 2>&1; then
  echo "FAIL: run_b8_regression should isolate MAKEFLAGS/MFLAGS for recursive make invocations" >&2
  cat "${tmp_dir}/makeflags.log" >&2
  exit 1
fi

mock_make_call_log="${tmp_dir}/mock_make_calls.log"
mock_make_no_contract_test="${tmp_dir}/mock_make_no_contract_test.sh"
cat >"${mock_make_no_contract_test}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

args="$*"
printf '%s\n' "${args}" >>"${B8_MAKE_CALL_LOG}"
if [[ "$args" == *"mbd_b8_syntax"* ]] || \
   [[ "$args" == *"mbd_b8_guard_output_test"* ]] || \
   [[ "$args" == *"mbd_ci_contract"* ]] || \
   [[ "$args" == *"mbd_b8_guard_test"* ]] || \
   [[ "$args" == *"mbd_b8_guard_contract_test"* ]] || \
   [[ "$args" == *"mbd_b8_guard_contract"* ]]; then
  exit 0
fi

echo "FAIL: unexpected make args: ${args}" >&2
exit 90
EOF
chmod +x "${mock_make_no_contract_test}"

if ! B8_MAKE_CMD="${mock_make_no_contract_test}" B8_MAKE_CALL_LOG="${mock_make_call_log}" B8_RUN_B14_REGRESSION=0 B8_LOCAL_TARGET=mbd_ci_contract B8_B14_TARGET=mbd_b8_syntax bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/contract_trim.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass with mock make when B8_RUN_B14_REGRESSION=0" >&2
  cat "${tmp_dir}/contract_trim.log" >&2
  exit 1
fi

if grep -q "mbd_ci_contract_test" "${mock_make_call_log}"; then
  echo "FAIL: run_b8_regression should not invoke mbd_ci_contract_test directly (guard wrapper path only)" >&2
  cat "${mock_make_call_log}" >&2
  exit 1
fi

if ! env -u B8_REGRESSION_LOCK_DIR \
  B8_MAKE_CMD="${mock_make_no_contract_test}" \
  B8_MAKE_CALL_LOG="${mock_make_call_log}" \
  B8_RUN_B14_REGRESSION=0 \
  B8_REGRESSION_SKIP_LOCK=1 \
  B8_LOCAL_TARGET=mbd_ci_contract \
  B8_B14_TARGET=mbd_b8_syntax \
  bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/repo_default_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass with repo default lock-dir derivation" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_scope=repo" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected lock_scope=repo in repo default lock-dir path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected repo-derived lock_dir in repo default lock-dir path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=scope_repo_default in repo default lock-dir path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! env -u B8_REGRESSION_LOCK_DIR \
  B8_MAKE_CMD="${mock_make_no_contract_test}" \
  B8_MAKE_CALL_LOG="${mock_make_call_log}" \
  B8_RUN_B14_REGRESSION=0 \
  B8_REGRESSION_SKIP_LOCK=1 \
  B8_REGRESSION_LOCK_SCOPE=global \
  B8_LOCAL_TARGET=mbd_ci_contract \
  B8_B14_TARGET=mbd_b8_syntax \
  bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/global_default_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass with global default lock-dir derivation" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_scope=global" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected lock_scope=global in global default lock-dir path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected global lock_dir in global default lock-dir path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected lock_dir_source=scope_global_default in global default lock-dir path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

held_lock_dir="${tmp_dir}/lock_held"
mkdir -p "${held_lock_dir}"
echo "$$" >"${held_lock_dir}/pid"

if B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_LOCK_DIR="${held_lock_dir}" bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/lock_held.log" 2>&1; then
  echo "FAIL: run_b8_regression should fail when lock is already held" >&2
  cat "${tmp_dir}/lock_held.log" >&2
  exit 1
fi

if ! grep -q "b8 regression lock is already held" "${tmp_dir}/lock_held.log"; then
  echo "FAIL: expected lock-held diagnostic was not found" >&2
  cat "${tmp_dir}/lock_held.log" >&2
  exit 1
fi

if ! B8_MAKE_CMD="${mock_make_no_contract_test}" B8_MAKE_CALL_LOG="${mock_make_call_log}" B8_RUN_B14_REGRESSION=0 B8_LOCAL_TARGET=mbd_ci_contract B8_B14_TARGET=mbd_b8_syntax B8_REGRESSION_SKIP_LOCK=1 B8_REGRESSION_LOCK_DIR="${held_lock_dir}" bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/skip_lock.log" 2>&1; then
  echo "FAIL: run_b8_regression should pass when lock is held but B8_REGRESSION_SKIP_LOCK=1" >&2
  cat "${tmp_dir}/skip_lock.log" >&2
  exit 1
fi

if ! grep -q "lock=skipped" "${tmp_dir}/skip_lock.log"; then
  echo "FAIL: expected lock=skipped in skip-lock path" >&2
  cat "${tmp_dir}/skip_lock.log" >&2
  exit 1
fi

stale_lock_dir="${tmp_dir}/stale_lock"
mkdir -p "${stale_lock_dir}"
echo "999999" >"${stale_lock_dir}/pid"

if ! B8_MAKE_CMD="${mock_make_no_contract_test}" B8_MAKE_CALL_LOG="${mock_make_call_log}" B8_RUN_B14_REGRESSION=0 B8_LOCAL_TARGET=mbd_ci_contract B8_B14_TARGET=mbd_b8_syntax B8_REGRESSION_LOCK_DIR="${stale_lock_dir}" bash "FEM4C/scripts/run_b8_regression.sh" >"${tmp_dir}/stale_lock.log" 2>&1; then
  echo "FAIL: run_b8_regression should recover stale lock and continue" >&2
  cat "${tmp_dir}/stale_lock.log" >&2
  exit 1
fi

if ! grep -q "recovered stale b8 regression lock" "${tmp_dir}/stale_lock.log"; then
  echo "FAIL: expected stale-lock recovery message was not found" >&2
  cat "${tmp_dir}/stale_lock.log" >&2
  exit 1
fi

temp_copy_stamp="$$.${RANDOM}"
script_copy="$(mktemp "${script_copy_dir}/.tmp_run_b8_regression_fail.${temp_copy_stamp}.XXXXXX.sh")"
cp "FEM4C/scripts/run_b8_regression.sh" "${script_copy}"

# Break one make target so we can verify fail-fast behavior.
sed -i 's/mbd_b8_guard_test/mbd_b8_guard_test_missing/' "${script_copy}"

if FEM4C_REPO_ROOT="${root_dir}" B8_LOCAL_TARGET=mbd_ci_contract bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_b8_regression should fail when command is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "No rule to make target 'mbd_b8_guard_test_missing'" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-target diagnostic was not found" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

echo "PASS: run_b8_regression self-test (pass + expected fail path)"
