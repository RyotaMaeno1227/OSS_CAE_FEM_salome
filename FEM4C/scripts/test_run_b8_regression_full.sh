#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
script_copy=""
script_copy_dir=""
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
    exit 0
    ;;
  *" mbd_b8_regression"|*" mbd_b8_regression "*)
    if [[ "${B8_B14_TARGET:-}" != "mbd_ci_contract" ]]; then
      echo "FAIL: B8_B14_TARGET not forwarded to mbd_b8_regression (got: ${B8_B14_TARGET:-unset})" >&2
      exit 92
    fi
    exit 0
    ;;
esac

echo "FAIL: unexpected make args: ${args}" >&2
exit 90
EOF
chmod +x "${mock_make}"

if ! MAKEFLAGS="--jobs=9" MFLAGS="--jobs=9" B8_MAKE_CMD="${mock_make}" B8_B14_TARGET=mbd_ci_contract B8_LOCAL_TARGET=mbd_ci_contract bash "FEM4C/scripts/run_b8_regression_full.sh" >"${tmp_dir}/makeflags.log" 2>&1; then
  echo "FAIL: run_b8_regression_full should isolate MAKEFLAGS/MFLAGS and pass B8_B14_TARGET" >&2
  cat "${tmp_dir}/makeflags.log" >&2
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
