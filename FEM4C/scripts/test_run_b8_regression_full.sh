#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
script_copy_dir=""
full_default_lock_dir="${tmp_dir}/b8_regression_full_test.lock"
repo_lock_hash="$(printf '%s\n' "${root_dir}" | cksum | awk '{print $1}')"
repo_default_lock_dir="/tmp/fem4c_b8_regression.${repo_lock_hash}.lock"
global_default_lock_dir="/tmp/fem4c_b8_regression.lock"
export B8_REGRESSION_LOCK_DIR="${full_default_lock_dir}"
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

if ! B8_LOCAL_TARGET=mbd_ci_contract bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "b14_target=mbd_ci_contract" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression_full should use mbd_ci_contract as default b14 target" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression_full should expose local_target=mbd_ci_contract in summary output" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "run_b14_regression=1" "${tmp_dir}/pass.log"; then
  echo "FAIL: baseline run_b8_regression_full should request B-14 regression (run_b14_regression=1)" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_scope=repo" "${tmp_dir}/pass.log"; then
  echo "FAIL: run_b8_regression_full should default lock scope to repo" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir_source=env" "${tmp_dir}/pass.log"; then
  echo "FAIL: expected b8_lock_dir_source=env in baseline lock-dir override path" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir=${full_default_lock_dir}" "${tmp_dir}/pass.log"; then
  echo "FAIL: expected b8_lock_dir=${full_default_lock_dir} in baseline lock-dir override path" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if grep -q "test_retry_used=0" "${tmp_dir}/pass.log"; then
  if ! grep -q "test_retry_reason=none" "${tmp_dir}/pass.log"; then
    echo "FAIL: baseline full regression reported test_retry_used=0 without test_retry_reason=none" >&2
    cat "${tmp_dir}/pass.log" >&2
    exit 1
  fi
elif grep -q "test_retry_used=1" "${tmp_dir}/pass.log"; then
  if ! grep -q "parser executable missing after test failure; rebuilding via make all and retrying test once" "${tmp_dir}/pass.log"; then
    echo "FAIL: baseline full regression reported test_retry_used=1 without parser-missing retry trace" >&2
    cat "${tmp_dir}/pass.log" >&2
    exit 1
  fi
  if ! grep -q "test_retry_reason=parser_missing" "${tmp_dir}/pass.log"; then
    echo "FAIL: baseline full regression reported test_retry_used=1 without test_retry_reason=parser_missing" >&2
    cat "${tmp_dir}/pass.log" >&2
    exit 1
  fi
else
  echo "FAIL: expected test_retry_used trace in baseline full regression path" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! B8_LOCAL_TARGET=mbd_ci_contract B8_B14_TARGET=mbd_b8_syntax bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/override_b14_target.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should pass when B8_B14_TARGET override is set" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if ! grep -q "b14_target=mbd_b8_syntax" "${tmp_dir}/override_b14_target.log"; then
  echo "FAIL: expected overridden b14 target was not found in run_b8_regression_full output" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if ! grep -q "run_b14_regression=1" "${tmp_dir}/override_b14_target.log"; then
  echo "FAIL: overridden run_b8_regression_full should keep B-14 regression enabled (run_b14_regression=1)" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/override_b14_target.log"; then
  echo "FAIL: run_b8_regression_full should keep local_target=mbd_ci_contract in override-b14 summary output" >&2
  cat "${tmp_dir}/override_b14_target.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_RUN_B14_REGRESSION=2 bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/invalid_knob.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail when B8_RUN_B14_REGRESSION is invalid" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if ! grep -q "B8_RUN_B14_REGRESSION must be 0 or 1" "${tmp_dir}/invalid_knob.log"; then
  echo "FAIL: expected invalid knob diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_knob.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_SKIP_LOCK=2 bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/invalid_skip_lock.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail when B8_REGRESSION_SKIP_LOCK is invalid" >&2
  cat "${tmp_dir}/invalid_skip_lock.log" >&2
  exit 1
fi

if ! grep -q "B8_REGRESSION_SKIP_LOCK must be 0 or 1" "${tmp_dir}/invalid_skip_lock.log"; then
  echo "FAIL: expected invalid skip-lock diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_skip_lock.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_LOCK_SCOPE=cluster bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/invalid_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail when B8_REGRESSION_LOCK_SCOPE is invalid" >&2
  cat "${tmp_dir}/invalid_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "B8_REGRESSION_LOCK_SCOPE must be repo or global" "${tmp_dir}/invalid_lock_scope.log"; then
  echo "FAIL: expected invalid lock-scope diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_lock_scope.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_CMD=__missing_make__ bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/invalid_make.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail when B8_MAKE_CMD is invalid" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if ! grep -q "B8_MAKE_CMD is not executable" "${tmp_dir}/invalid_make.log"; then
  echo "FAIL: expected invalid make command diagnostic was not found" >&2
  cat "${tmp_dir}/invalid_make.log" >&2
  exit 1
fi

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_TIMEOUT_SEC=abc bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/invalid_make_timeout.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail when B8_MAKE_TIMEOUT_SEC is non-numeric" >&2
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

if B8_LOCAL_TARGET=mbd_ci_contract B8_MAKE_CMD="${slow_make}" B8_MAKE_TIMEOUT_SEC=1 B8_RUN_B14_REGRESSION=0 B8_REGRESSION_SKIP_LOCK=1 bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/make_timeout.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should fail-fast when B8_MAKE_TIMEOUT_SEC is exceeded" >&2
  cat "${tmp_dir}/make_timeout.log" >&2
  exit 1
fi

if ! grep -q "make target timed out (target=clean timeout_sec=1)" "${tmp_dir}/make_timeout.log"; then
  echo "FAIL: expected timeout diagnostic was not found in run_b8_regression_full output" >&2
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
case "${args}" in
  *" clean"|*" clean "*|*" all"|*" all "*|*" test"|*" test "*)
    if [[ -n "${B8_REGRESSION_SKIP_LOCK:-}" || -n "${B8_REGRESSION_LOCK_SCOPE:-}" || -n "${B8_REGRESSION_LOCK_DIR:-}" ]]; then
      echo "FAIL: lock knobs should be isolated for clean/all/test (skip=${B8_REGRESSION_SKIP_LOCK:-unset} scope=${B8_REGRESSION_LOCK_SCOPE:-unset} lock_dir=${B8_REGRESSION_LOCK_DIR:-unset})" >&2
      exit 93
    fi
    exit 0
    ;;
  *" mbd_b8_regression"|*" mbd_b8_regression "*)
    if [[ "${B8_B14_TARGET:-}" != "mbd_ci_contract" ]]; then
      echo "FAIL: B8_B14_TARGET not forwarded to mbd_b8_regression (got: ${B8_B14_TARGET:-unset})" >&2
      exit 92
    fi
    if [[ "${B8_REGRESSION_SKIP_LOCK:-}" != "${EXPECT_SKIP_LOCK:-0}" ]]; then
      echo "FAIL: B8_REGRESSION_SKIP_LOCK not forwarded to mbd_b8_regression (got: ${B8_REGRESSION_SKIP_LOCK:-unset}, expected: ${EXPECT_SKIP_LOCK:-0})" >&2
      exit 94
    fi
    if [[ "${B8_REGRESSION_LOCK_SCOPE:-}" != "${EXPECT_LOCK_SCOPE:-repo}" ]]; then
      echo "FAIL: B8_REGRESSION_LOCK_SCOPE not forwarded to mbd_b8_regression (got: ${B8_REGRESSION_LOCK_SCOPE:-unset}, expected: ${EXPECT_LOCK_SCOPE:-repo})" >&2
      exit 96
    fi
    if [[ "${B8_REGRESSION_LOCK_DIR:-}" != "${EXPECT_LOCK_DIR:-/tmp/fem4c_b8_regression.lock}" ]]; then
      echo "FAIL: B8_REGRESSION_LOCK_DIR not forwarded to mbd_b8_regression (got: ${B8_REGRESSION_LOCK_DIR:-unset}, expected: ${EXPECT_LOCK_DIR:-/tmp/fem4c_b8_regression.lock})" >&2
      exit 95
    fi
    exit 0
    ;;
esac

echo "FAIL: unexpected make args: ${args}" >&2
exit 90
EOF
chmod +x "${mock_make}"

if ! MAKEFLAGS="--jobs=9" MFLAGS="--jobs=9" B8_MAKE_CMD="${mock_make}" B8_B14_TARGET=mbd_ci_contract B8_LOCAL_TARGET=mbd_ci_contract EXPECT_SKIP_LOCK=0 EXPECT_LOCK_SCOPE=repo EXPECT_LOCK_DIR="${full_default_lock_dir}" bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/makeflags.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should isolate MAKEFLAGS/MFLAGS and pass B8_B14_TARGET" >&2
  cat "${tmp_dir}/makeflags.log" >&2
  exit 1
fi

full_skip_lock_dir="${tmp_dir}/full_skip_lock.lock"
if ! B8_MAKE_CMD="${mock_make}" B8_RUN_B14_REGRESSION=0 B8_B14_TARGET=mbd_ci_contract B8_LOCAL_TARGET=mbd_ci_contract B8_REGRESSION_SKIP_LOCK=1 B8_REGRESSION_LOCK_SCOPE=global B8_REGRESSION_LOCK_DIR="${full_skip_lock_dir}" EXPECT_SKIP_LOCK=1 EXPECT_LOCK_SCOPE=global EXPECT_LOCK_DIR="${full_skip_lock_dir}" bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/skip_b14.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should pass when B8_RUN_B14_REGRESSION=0 (mock make path)" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "run_b14_regression=0" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected run_b14_regression=0 in B8_RUN_B14_REGRESSION=0 full path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b14_target=mbd_ci_contract" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b14_target trace in B8_RUN_B14_REGRESSION=0 full path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "local_target=mbd_ci_contract" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: run_b8_regression_full should keep local_target=mbd_ci_contract in skip-b14 summary output" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b8_skip_lock=1" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b8_skip_lock=1 trace in full wrapper skip-lock path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_scope=global" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b8_lock_scope=global trace in full wrapper global lock-scope path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir_source=env" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b8_lock_dir_source=env in full wrapper lock-dir override path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir=${full_skip_lock_dir}" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected b8_lock_dir trace in full wrapper skip-lock path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! grep -q "test_retry_reason=" "${tmp_dir}/skip_b14.log"; then
  echo "FAIL: expected test_retry_reason trace in full wrapper skip-lock path" >&2
  cat "${tmp_dir}/skip_b14.log" >&2
  exit 1
fi

if ! env -u B8_REGRESSION_LOCK_DIR \
  B8_MAKE_CMD="${mock_make}" \
  B8_RUN_B14_REGRESSION=0 \
  B8_B14_TARGET=mbd_ci_contract \
  B8_LOCAL_TARGET=mbd_ci_contract \
  B8_REGRESSION_SKIP_LOCK=1 \
  B8_REGRESSION_LOCK_SCOPE=repo \
  EXPECT_SKIP_LOCK=1 \
  EXPECT_LOCK_SCOPE=repo \
  EXPECT_LOCK_DIR="${repo_default_lock_dir}" \
  bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/repo_default_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should pass with repo default lock-dir derivation" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_scope=repo" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected b8_lock_scope=repo in repo default lock-dir path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir=${repo_default_lock_dir}" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected repo-derived b8_lock_dir in full wrapper repo default path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir_source=scope_repo_default" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected b8_lock_dir_source=scope_repo_default in full wrapper repo default path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "test_retry_reason=" "${tmp_dir}/repo_default_lock_scope.log"; then
  echo "FAIL: expected test_retry_reason trace in full wrapper repo default path" >&2
  cat "${tmp_dir}/repo_default_lock_scope.log" >&2
  exit 1
fi

if ! env -u B8_REGRESSION_LOCK_DIR \
  B8_MAKE_CMD="${mock_make}" \
  B8_RUN_B14_REGRESSION=0 \
  B8_B14_TARGET=mbd_ci_contract \
  B8_LOCAL_TARGET=mbd_ci_contract \
  B8_REGRESSION_SKIP_LOCK=1 \
  B8_REGRESSION_LOCK_SCOPE=global \
  EXPECT_SKIP_LOCK=1 \
  EXPECT_LOCK_SCOPE=global \
  EXPECT_LOCK_DIR="${global_default_lock_dir}" \
  bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/global_default_lock_scope.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should pass with global default lock-dir derivation" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_scope=global" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected b8_lock_scope=global in global default lock-dir path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir=${global_default_lock_dir}" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected global b8_lock_dir in full wrapper global default path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "b8_lock_dir_source=scope_global_default" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected b8_lock_dir_source=scope_global_default in full wrapper global default path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

if ! grep -q "test_retry_reason=" "${tmp_dir}/global_default_lock_scope.log"; then
  echo "FAIL: expected test_retry_reason trace in full wrapper global default path" >&2
  cat "${tmp_dir}/global_default_lock_scope.log" >&2
  exit 1
fi

retry_make_script="${tmp_dir}/retry_make_impl.sh"
retry_make_dir="${tmp_dir}/retry_make_dir"
retry_state_file="${tmp_dir}/retry_make_state"
retry_call_log="${tmp_dir}/retry_make_calls.log"
mkdir -p "${retry_make_dir}"
cat >"${retry_make_script}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

args="$*"
state_file="${RETRY_STATE_FILE:?}"
call_log="${RETRY_CALL_LOG:?}"
printf '%s\n' "$args" >> "${call_log}"

case "${args}" in
  *" clean"|*" clean "*|*" all"|*" all "*|*" mbd_b8_regression"|*" mbd_b8_regression "*)
    exit 0
    ;;
  *" test"|*" test "*)
    if [[ ! -f "${state_file}" ]]; then
      : > "${state_file}"
      rm -f FEM4C/parser/parser FEM4C/parser/parser.exe
      echo "FAIL: parser executable missing during test" >&2
      exit 2
    fi
    exit 0
    ;;
esac

echo "FAIL: unexpected make args in retry-make stub: ${args}" >&2
exit 90
EOF
chmod +x "${retry_make_script}"
ln -s "${retry_make_script}" "${retry_make_dir}/make"

if ! RETRY_STATE_FILE="${retry_state_file}" \
  RETRY_CALL_LOG="${retry_call_log}" \
  B8_MAKE_CMD="${retry_make_dir}/make" \
  B8_RUN_B14_REGRESSION=0 \
  B8_REGRESSION_SKIP_LOCK=1 \
  B8_LOCAL_TARGET=mbd_ci_contract \
  bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/retry_parser.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should recover when parser is missing after test failure" >&2
  cat "${tmp_dir}/retry_parser.log" >&2
  exit 1
fi

if ! grep -q "parser executable missing after test failure; rebuilding via make all and retrying test once" "${tmp_dir}/retry_parser.log"; then
  echo "FAIL: expected parser-missing retry trace was not found in run_b8_regression_full output" >&2
  cat "${tmp_dir}/retry_parser.log" >&2
  exit 1
fi

if ! grep -q "test_retry_used=1" "${tmp_dir}/retry_parser.log"; then
  echo "FAIL: expected test_retry_used=1 in parser-missing retry path" >&2
  cat "${tmp_dir}/retry_parser.log" >&2
  exit 1
fi

if ! grep -q "test_retry_reason=parser_missing" "${tmp_dir}/retry_parser.log"; then
  echo "FAIL: expected test_retry_reason=parser_missing in parser-missing retry path" >&2
  cat "${tmp_dir}/retry_parser.log" >&2
  exit 1
fi

if [[ "$(rg -n --fixed-strings -- ' test' "${retry_call_log}" | wc -l | tr -d '[:space:]')" != "2" ]]; then
  echo "FAIL: retry make stub should receive two test calls (initial + retry)" >&2
  cat "${retry_call_log}" >&2
  exit 1
fi

temp_copy_stamp="$$.${RANDOM}"
script_copy="$(mktemp "${script_copy_dir}/.tmp_run_b8_regression_full_fail.${temp_copy_stamp}.XXXXXX.sh")"
cp "FEM4C/scripts/run_b8_regression_full.sh" "${script_copy}"

# Break one make target so we can verify fail-fast behavior.
sed -i 's/mbd_b8_regression/mbd_b8_regression_missing/' "${script_copy}"

if FEM4C_REPO_ROOT="${root_dir}" B8_LOCAL_TARGET=mbd_ci_contract bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_b8_regression_full should fail when command is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "No rule to make target 'mbd_b8_regression_missing'" "${tmp_dir}/fail.log"; then
  echo "FAIL: expected missing-target diagnostic was not found" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

echo "PASS: run_b8_regression_full self-test (pass + expected fail path + makeflags/b14-target forwarding)"
