#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
selftest_lock_dir="/tmp/fem4c_test_run_a24_regression_full.lock"
selftest_lock_pid="${selftest_lock_dir}/pid"
script_copy=""
script_summary_copy=""
baseline_lock_dir="${tmp_dir}/baseline_lock"
fail_lock_dir="${tmp_dir}/fail_lock"
missing_summary_lock_dir="${tmp_dir}/missing_summary_lock"
stale_lock_dir="${tmp_dir}/stale_lock"
active_lock_dir="${tmp_dir}/active_lock"
active_lock_dir_for_summary="${tmp_dir}/active_lock_summary"
summary_out_pass="${tmp_dir}/summary_pass.log"
summary_out_fail="${tmp_dir}/summary_fail.log"
summary_out_lock="${tmp_dir}/summary_lock.log"
summary_out_config="${tmp_dir}/summary_config.log"
summary_out_retry="${tmp_dir}/summary_retry.log"
summary_out_retry_build="${tmp_dir}/summary_retry_build.log"
summary_out_retry_regression="${tmp_dir}/summary_retry_regression.log"
summary_out_nested_trace="${tmp_dir}/summary_nested_trace.log"
summary_out_missing_bin="${tmp_dir}/summary_missing_bin.log"
summary_out_log_fallback="${tmp_dir}/summary_log_fallback.log"
summary_out_log_fallback_upper="${tmp_dir}/summary_log_fallback_upper.log"
summary_out_log_summary_fallback="${tmp_dir}/summary_log_summary_fallback.log"
summary_out_log_summary_precedence="${tmp_dir}/summary_log_summary_precedence.log"
summary_out_log_summary_malformed_key_precedence="${tmp_dir}/summary_log_summary_malformed_key_precedence.log"
summary_out_log_summary_duplicate_key_precedence="${tmp_dir}/summary_log_summary_duplicate_key_precedence.log"
summary_out_log_summary_malformed_unknown_fallback="${tmp_dir}/summary_log_summary_malformed_unknown_fallback.log"
summary_out_log_summary_uppercase="${tmp_dir}/summary_log_summary_uppercase.log"
summary_out_log_summary_uppercase_precedence="${tmp_dir}/summary_log_summary_uppercase_precedence.log"
summary_out_log_summary_tab_whitespace="${tmp_dir}/summary_log_summary_tab_whitespace.log"
summary_out_log_summary_equals_whitespace_fallback="${tmp_dir}/summary_log_summary_equals_whitespace_fallback.log"
summary_out_log_summary_quoted_value_fallback="${tmp_dir}/summary_log_summary_quoted_value_fallback.log"
summary_out_log_summary_single_quote_fallback="${tmp_dir}/summary_log_summary_single_quote_fallback.log"
summary_out_log_summary_quote_mixed_fallback="${tmp_dir}/summary_log_summary_quote_mixed_fallback.log"
summary_out_log_summary_backslash_value_fallback="${tmp_dir}/summary_log_summary_backslash_value_fallback.log"
summary_out_log_summary_quoted_key_fallback="${tmp_dir}/summary_log_summary_quoted_key_fallback.log"
summary_out_log_summary_single_quoted_key_fallback="${tmp_dir}/summary_log_summary_single_quoted_key_fallback.log"
summary_out_log_summary_quoted_cmd_key_fallback="${tmp_dir}/summary_log_summary_quoted_cmd_key_fallback.log"
summary_out_log_summary_partial_fallback="${tmp_dir}/summary_log_summary_partial_fallback.log"
summary_out_log_summary_missing_equals_fallback="${tmp_dir}/summary_log_summary_missing_equals_fallback.log"
summary_out_log_summary_empty_value_fallback="${tmp_dir}/summary_log_summary_empty_value_fallback.log"
summary_out_log_summary_empty_key_fallback="${tmp_dir}/summary_log_summary_empty_key_fallback.log"
summary_out_log_summary_extra_equals_fallback="${tmp_dir}/summary_log_summary_extra_equals_fallback.log"
summary_out_log_summary_leading_punct_fallback="${tmp_dir}/summary_log_summary_leading_punct_fallback.log"
summary_out_log_summary_internal_symbol_fallback="${tmp_dir}/summary_log_summary_internal_symbol_fallback.log"
summary_out_log_summary_trailing_punct="${tmp_dir}/summary_log_summary_trailing_punct.log"
summary_out_log_summary_crlf_fallback="${tmp_dir}/summary_log_summary_crlf_fallback.log"
summary_out_nonexec_bin="${tmp_dir}/summary_nonexec_bin.log"
cleanup() {
  # Avoid leaking nested make/bash processes when the self-test aborts early.
  pkill -P $$ 2>/dev/null || true
  if [[ -n "${script_copy}" && -f "${script_copy}" ]]; then
    rm -f "${script_copy}"
  fi
  if [[ -n "${script_summary_copy}" && -f "${script_summary_copy}" ]]; then
    rm -f "${script_summary_copy}"
  fi
  rm -f "${selftest_lock_pid}" 2>/dev/null || true
  rmdir "${selftest_lock_dir}" 2>/dev/null || true
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

acquire_selftest_lock() {
  local owner_pid
  if mkdir "${selftest_lock_dir}" 2>/dev/null; then
    echo "$$" >"${selftest_lock_pid}"
    return 0
  fi

  if [[ -f "${selftest_lock_pid}" ]]; then
    owner_pid="$(cat "${selftest_lock_pid}" 2>/dev/null || true)"
    if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then
      rm -rf "${selftest_lock_dir}" 2>/dev/null || true
      if mkdir "${selftest_lock_dir}" 2>/dev/null; then
        echo "$$" >"${selftest_lock_pid}"
        return 0
      fi
    fi
  fi

  echo "FAIL: test_run_a24_regression_full already running (${selftest_lock_dir})" >&2
  return 1
}

acquire_selftest_lock

# Isolate nested run_a24_regression lock path from external sessions.
export A24_REGRESSION_LOCK_DIR="${tmp_dir}/a24_regression.lock"

if ! env -u MAKEFLAGS -u MFLAGS make -C FEM4C >"${tmp_dir}/build.log" 2>&1; then
  echo "FAIL: run_a24_regression_full self-test requires successful FEM4C build preflight" >&2
  cat "${tmp_dir}/build.log" >&2
  exit 1
fi

if ! A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${baseline_lock_dir}" A24_FULL_SUMMARY_OUT="${summary_out_pass}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/pass.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should pass in baseline state" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=0 .*clean_attempts=1 .*build_attempts=1 .*regression_attempts=1 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/pass.log"; then
  echo "FAIL: baseline run_a24_regression_full output is missing summary pass marker" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if [[ ! -s "${summary_out_pass}" ]]; then
  echo "FAIL: baseline run_a24_regression_full did not write summary output file" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

summary_line="$(rg -n "^A24_FULL_SUMMARY " "${tmp_dir}/pass.log" | tail -n 1 | sed 's/^[0-9]*://')"
if [[ -z "${summary_line}" ]]; then
  echo "FAIL: baseline full-summary line extraction failed" >&2
  cat "${tmp_dir}/pass.log" >&2
  exit 1
fi

if [[ "$(cat "${summary_out_pass}")" != "${summary_line}" ]]; then
  echo "FAIL: full-summary output file does not match baseline summary line" >&2
  cat "${tmp_dir}/pass.log" >&2
  cat "${summary_out_pass}" >&2
  exit 1
fi

script_copy="${root_dir}/FEM4C/scripts/.tmp_run_a24_regression_full_fail.sh"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${script_copy}"

# Break one make target so we can verify fail-fast behavior.
sed -i '0,/if make -C FEM4C mbd_a24_regression >"\${nested_regression_log}" 2>\&1; then/s//if make -C FEM4C mbd_a24_regression_missing >"\${nested_regression_log}" 2>\&1; then/' "${script_copy}"

if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${fail_lock_dir}" A24_FULL_SUMMARY_OUT="${summary_out_fail}" bash "${script_copy}" >"${tmp_dir}/fail.log" 2>&1; then
  echo "FAIL: modified run_a24_regression_full should fail when command is broken" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression failed_cmd=make_mbd_a24_regression$" "${tmp_dir}/fail.log"; then
  echo "FAIL: modified run_a24_regression_full should emit fail summary for regression step" >&2
  cat "${tmp_dir}/fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression failed_cmd=make_mbd_a24_regression$" "${summary_out_fail}"; then
  echo "FAIL: modified run_a24_regression_full should write fail summary to output file" >&2
  cat "${tmp_dir}/fail.log" >&2
  cat "${summary_out_fail}" >&2
  exit 1
fi

fake_make_nested_dir="${tmp_dir}/fake_make_nested"
mkdir -p "${fake_make_nested_dir}"
cat > "${fake_make_nested_dir}/make" <<EOF_MAKE
#!/usr/bin/env bash
set -euo pipefail
if [[ "\$*" == *"mbd_a24_regression"* ]]; then
  summary_line="A24_REGRESSION_SUMMARY contract_test=1 lock=acquired integrator_attempts=1 ci_contract_attempts=0 ci_contract_test_attempts=0 overall=fail failed_step=integrator_checks failed_cmd=make_mbd_integrator_checks"
  echo "\${summary_line}"
  if [[ -n "\${A24_REGRESSION_SUMMARY_OUT:-}" ]]; then
    printf '%s\n' "\${summary_line}" >"\${A24_REGRESSION_SUMMARY_OUT}"
  fi
  exit 2
fi
exit 0
EOF_MAKE
chmod +x "${fake_make_nested_dir}/make"

if PATH="${fake_make_nested_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/nested_trace_lock" A24_FULL_SUMMARY_OUT="${summary_out_nested_trace}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/nested_trace_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested mbd_a24_regression reports integrator failure" >&2
  cat "${tmp_dir}/nested_trace_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/nested_trace_fail.log"; then
  echo "FAIL: nested regression trace should propagate integrator failure into full summary" >&2
  cat "${tmp_dir}/nested_trace_fail.log" >&2
  exit 1
fi

if ! grep -q "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "${tmp_dir}/nested_trace_fail.log"; then
  echo "FAIL: nested regression trace should emit integrator preflight diagnosis hint" >&2
  cat "${tmp_dir}/nested_trace_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_nested_trace}"; then
  echo "FAIL: nested regression trace should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/nested_trace_fail.log" >&2
  cat "${summary_out_nested_trace}" >&2
  exit 1
fi

fake_make_log_fallback_dir="${tmp_dir}/fake_make_log_fallback"
mkdir -p "${fake_make_log_fallback_dir}"
cat > "${fake_make_log_fallback_dir}/make" <<'EOF_MAKE_LOG_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "FAIL: mbd integrator checker requires executable fem4c binary (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_FALLBACK
chmod +x "${fake_make_log_fallback_dir}/make"

if PATH="${fake_make_log_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary is absent and log has integrator preflight error" >&2
  cat "${tmp_dir}/log_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "requires executable fem4c binary" "${tmp_dir}/log_fallback_fail.log"; then
  echo "FAIL: log-fallback path should include nested preflight diagnostic in log output" >&2
  cat "${tmp_dir}/log_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "${tmp_dir}/log_fallback_fail.log"; then
  echo "FAIL: log-fallback path should emit integrator preflight diagnosis hint" >&2
  cat "${tmp_dir}/log_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_fallback_fail.log"; then
  echo "FAIL: log-fallback path should propagate integrator failure into full summary" >&2
  cat "${tmp_dir}/log_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_fallback}"; then
  echo "FAIL: log-fallback path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/log_fallback_fail.log" >&2
  cat "${summary_out_log_fallback}" >&2
  exit 1
fi

fake_make_log_fallback_upper_dir="${tmp_dir}/fake_make_log_fallback_upper"
mkdir -p "${fake_make_log_fallback_upper_dir}"
cat > "${fake_make_log_fallback_upper_dir}/make" <<'EOF_MAKE_LOG_FALLBACK_UPPER'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_FALLBACK_UPPER
chmod +x "${fake_make_log_fallback_upper_dir}/make"

if PATH="${fake_make_log_fallback_upper_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_fallback_upper_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_fallback_upper}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_fallback_upper_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested preflight log is uppercase" >&2
  cat "${tmp_dir}/log_fallback_upper_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_fallback_upper_fail.log"; then
  echo "FAIL: uppercase log-fallback path should propagate integrator failure into full summary" >&2
  cat "${tmp_dir}/log_fallback_upper_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_fallback_upper}"; then
  echo "FAIL: uppercase log-fallback path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/log_fallback_upper_fail.log" >&2
  cat "${summary_out_log_fallback_upper}" >&2
  exit 1
fi

fake_make_log_summary_fallback_dir="${tmp_dir}/fake_make_log_summary_fallback"
mkdir -p "${fake_make_log_summary_fallback_dir}"
cat > "${fake_make_log_summary_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail failed_step=integrator_checks failed_cmd=make_mbd_integrator_checks"
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_FALLBACK
chmod +x "${fake_make_log_summary_fallback_dir}/make"

if PATH="${fake_make_log_summary_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary file is absent but log includes nested summary line" >&2
  cat "${tmp_dir}/log_summary_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_REGRESSION_SUMMARY .*overall=fail failed_step=integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_fallback_fail.log"; then
  echo "FAIL: log-summary-fallback path should include nested regression summary line in log output" >&2
  cat "${tmp_dir}/log_summary_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "${tmp_dir}/log_summary_fallback_fail.log"; then
  echo "FAIL: log-summary-fallback path should emit integrator preflight diagnosis hint" >&2
  cat "${tmp_dir}/log_summary_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_fallback_fail.log"; then
  echo "FAIL: log-summary-fallback path should propagate nested summary failure into full summary" >&2
  cat "${tmp_dir}/log_summary_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_fallback}"; then
  echo "FAIL: log-summary-fallback path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_fallback_fail.log" >&2
  cat "${summary_out_log_summary_fallback}" >&2
  exit 1
fi

fake_make_log_summary_precedence_dir="${tmp_dir}/fake_make_log_summary_precedence"
mkdir -p "${fake_make_log_summary_precedence_dir}"
cat > "${fake_make_log_summary_precedence_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_PRECEDENCE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail failed_step=ci_contract failed_cmd=make_mbd_ci_contract"
  echo "FAIL: mbd integrator checker requires executable fem4c binary (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_PRECEDENCE
chmod +x "${fake_make_log_summary_precedence_dir}/make"

if PATH="${fake_make_log_summary_precedence_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_precedence_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_precedence}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_precedence_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary and generic preflight log are both present" >&2
  cat "${tmp_dir}/log_summary_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${tmp_dir}/log_summary_precedence_fail.log"; then
  echo "FAIL: log-summary precedence path should prefer nested summary over generic preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${summary_out_log_summary_precedence}"; then
  echo "FAIL: log-summary precedence path should persist nested-summary-priority failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_precedence_fail.log" >&2
  cat "${summary_out_log_summary_precedence}" >&2
  exit 1
fi

fake_make_log_summary_malformed_key_precedence_dir="${tmp_dir}/fake_make_log_summary_malformed_key_precedence"
mkdir -p "${fake_make_log_summary_malformed_key_precedence_dir}"
cat > "${fake_make_log_summary_malformed_key_precedence_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_MALFORMED_KEY_PRECEDENCE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=CI_CONTRACT FAILED_CMD=MAKE_MBD_CI_CONTRACT "FAILED_STEP"=INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_MALFORMED_KEY_PRECEDENCE
chmod +x "${fake_make_log_summary_malformed_key_precedence_dir}/make"

if PATH="${fake_make_log_summary_malformed_key_precedence_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_malformed_key_precedence_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_malformed_key_precedence}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_malformed_key_precedence_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary includes malformed key plus valid keys" >&2
  cat "${tmp_dir}/log_summary_malformed_key_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${tmp_dir}/log_summary_malformed_key_precedence_fail.log"; then
  echo "FAIL: malformed-key precedence should keep valid nested summary keys in full summary" >&2
  cat "${tmp_dir}/log_summary_malformed_key_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${summary_out_log_summary_malformed_key_precedence}"; then
  echo "FAIL: malformed-key precedence should persist valid nested-summary keys into full summary output file" >&2
  cat "${tmp_dir}/log_summary_malformed_key_precedence_fail.log" >&2
  cat "${summary_out_log_summary_malformed_key_precedence}" >&2
  exit 1
fi

fake_make_log_summary_duplicate_key_precedence_dir="${tmp_dir}/fake_make_log_summary_duplicate_key_precedence"
mkdir -p "${fake_make_log_summary_duplicate_key_precedence_dir}"
cat > "${fake_make_log_summary_duplicate_key_precedence_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_DUPLICATE_KEY_PRECEDENCE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=CI_CONTRACT FAILED_CMD=MAKE_MBD_CI_CONTRACT FAILED_STEP=INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_DUPLICATE_KEY_PRECEDENCE
chmod +x "${fake_make_log_summary_duplicate_key_precedence_dir}/make"

if PATH="${fake_make_log_summary_duplicate_key_precedence_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_duplicate_key_precedence_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_duplicate_key_precedence}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_duplicate_key_precedence_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary includes duplicate valid keys" >&2
  cat "${tmp_dir}/log_summary_duplicate_key_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${tmp_dir}/log_summary_duplicate_key_precedence_fail.log"; then
  echo "FAIL: duplicate-key precedence should keep first valid nested summary keys in full summary" >&2
  cat "${tmp_dir}/log_summary_duplicate_key_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${summary_out_log_summary_duplicate_key_precedence}"; then
  echo "FAIL: duplicate-key precedence should persist first valid nested summary keys into full summary output file" >&2
  cat "${tmp_dir}/log_summary_duplicate_key_precedence_fail.log" >&2
  cat "${summary_out_log_summary_duplicate_key_precedence}" >&2
  exit 1
fi

fake_make_log_summary_malformed_unknown_fallback_dir="${tmp_dir}/fake_make_log_summary_malformed_unknown_fallback"
mkdir -p "${fake_make_log_summary_malformed_unknown_fallback_dir}"
cat > "${fake_make_log_summary_malformed_unknown_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_MALFORMED_UNKNOWN_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail "FAILED_STEP"=CI_CONTRACT UNKNOWN_KEY=NOPE BROKEN-KEY=INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_MALFORMED_UNKNOWN_FALLBACK
chmod +x "${fake_make_log_summary_malformed_unknown_fallback_dir}/make"

if PATH="${fake_make_log_summary_malformed_unknown_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_malformed_unknown_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_malformed_unknown_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_malformed_unknown_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary mixes malformed/unknown keys without canonical failure keys" >&2
  cat "${tmp_dir}/log_summary_malformed_unknown_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_malformed_unknown_fallback_fail.log"; then
  echo "FAIL: malformed+unknown key mix should canonical-fallback to integrator preflight in full summary" >&2
  cat "${tmp_dir}/log_summary_malformed_unknown_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_malformed_unknown_fallback}"; then
  echo "FAIL: malformed+unknown key mix should persist canonical fallback into full summary output file" >&2
  cat "${tmp_dir}/log_summary_malformed_unknown_fallback_fail.log" >&2
  cat "${summary_out_log_summary_malformed_unknown_fallback}" >&2
  exit 1
fi

fake_make_log_summary_uppercase_dir="${tmp_dir}/fake_make_log_summary_uppercase"
mkdir -p "${fake_make_log_summary_uppercase_dir}"
cat > "${fake_make_log_summary_uppercase_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_UPPERCASE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_UPPERCASE
chmod +x "${fake_make_log_summary_uppercase_dir}/make"

if PATH="${fake_make_log_summary_uppercase_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_uppercase_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_uppercase}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_uppercase_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary line/tokens are uppercase variants" >&2
  cat "${tmp_dir}/log_summary_uppercase_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_uppercase_fail.log"; then
  echo "FAIL: uppercase nested-summary path should normalize failure fields into full summary" >&2
  cat "${tmp_dir}/log_summary_uppercase_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_uppercase}"; then
  echo "FAIL: uppercase nested-summary path should persist normalized failure fields into full summary output file" >&2
  cat "${tmp_dir}/log_summary_uppercase_fail.log" >&2
  cat "${summary_out_log_summary_uppercase}" >&2
  exit 1
fi

fake_make_log_summary_uppercase_precedence_dir="${tmp_dir}/fake_make_log_summary_uppercase_precedence"
mkdir -p "${fake_make_log_summary_uppercase_precedence_dir}"
cat > "${fake_make_log_summary_uppercase_precedence_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_UPPERCASE_PRECEDENCE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=CI_CONTRACT FAILED_CMD=MAKE_MBD_CI_CONTRACT"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_UPPERCASE_PRECEDENCE
chmod +x "${fake_make_log_summary_uppercase_precedence_dir}/make"

if PATH="${fake_make_log_summary_uppercase_precedence_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_uppercase_precedence_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_uppercase_precedence}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_uppercase_precedence_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when uppercase nested summary and generic preflight log are both present" >&2
  cat "${tmp_dir}/log_summary_uppercase_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${tmp_dir}/log_summary_uppercase_precedence_fail.log"; then
  echo "FAIL: uppercase log-summary precedence path should prefer normalized nested summary over generic preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_uppercase_precedence_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_ci_contract failed_cmd=make_mbd_ci_contract$" "${summary_out_log_summary_uppercase_precedence}"; then
  echo "FAIL: uppercase log-summary precedence path should persist nested-summary-priority failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_uppercase_precedence_fail.log" >&2
  cat "${summary_out_log_summary_uppercase_precedence}" >&2
  exit 1
fi

fake_make_log_summary_tab_whitespace_dir="${tmp_dir}/fake_make_log_summary_tab_whitespace"
mkdir -p "${fake_make_log_summary_tab_whitespace_dir}"
cat > "${fake_make_log_summary_tab_whitespace_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_TAB_WHITESPACE'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  printf 'a24_regression_summary\tcontract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail    FAILED_STEP=INTEGRATOR_CHECKS   FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS\n'
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_TAB_WHITESPACE
chmod +x "${fake_make_log_summary_tab_whitespace_dir}/make"

if PATH="${fake_make_log_summary_tab_whitespace_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_tab_whitespace_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_tab_whitespace}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_tab_whitespace_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary uses tab/multiple-space separators" >&2
  cat "${tmp_dir}/log_summary_tab_whitespace_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_tab_whitespace_fail.log"; then
  echo "FAIL: tab-whitespace nested-summary path should normalize failure fields into full summary" >&2
  cat "${tmp_dir}/log_summary_tab_whitespace_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_tab_whitespace}"; then
  echo "FAIL: tab-whitespace nested-summary path should persist normalized failure fields into full summary output file" >&2
  cat "${tmp_dir}/log_summary_tab_whitespace_fail.log" >&2
  cat "${summary_out_log_summary_tab_whitespace}" >&2
  exit 1
fi

fake_make_log_summary_equals_whitespace_fallback_dir="${tmp_dir}/fake_make_log_summary_equals_whitespace_fallback"
mkdir -p "${fake_make_log_summary_equals_whitespace_fallback_dir}"
cat > "${fake_make_log_summary_equals_whitespace_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_EQUALS_WHITESPACE_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP =INTEGRATOR_CHECKS FAILED_CMD =MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_EQUALS_WHITESPACE_FALLBACK
chmod +x "${fake_make_log_summary_equals_whitespace_fallback_dir}/make"

if PATH="${fake_make_log_summary_equals_whitespace_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_equals_whitespace_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_equals_whitespace_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_equals_whitespace_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token uses whitespace around '='" >&2
  cat "${tmp_dir}/log_summary_equals_whitespace_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_equals_whitespace_fallback_fail.log"; then
  echo "FAIL: whitespace-around-equals nested-summary token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_equals_whitespace_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_equals_whitespace_fallback}"; then
  echo "FAIL: whitespace-around-equals nested-summary token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_equals_whitespace_fallback_fail.log" >&2
  cat "${summary_out_log_summary_equals_whitespace_fallback}" >&2
  exit 1
fi

fake_make_log_summary_quoted_value_fallback_dir="${tmp_dir}/fake_make_log_summary_quoted_value_fallback"
mkdir -p "${fake_make_log_summary_quoted_value_fallback_dir}"
cat > "${fake_make_log_summary_quoted_value_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_QUOTED_VALUE_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS FAILED_CMD="MAKE_MBD_INTEGRATOR_CHECKS"'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_QUOTED_VALUE_FALLBACK
chmod +x "${fake_make_log_summary_quoted_value_fallback_dir}/make"

if PATH="${fake_make_log_summary_quoted_value_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_quoted_value_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_quoted_value_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_quoted_value_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token value is quoted" >&2
  cat "${tmp_dir}/log_summary_quoted_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_quoted_value_fallback_fail.log"; then
  echo "FAIL: quoted nested-summary value token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_quoted_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_quoted_value_fallback}"; then
  echo "FAIL: quoted nested-summary value token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_quoted_value_fallback_fail.log" >&2
  cat "${summary_out_log_summary_quoted_value_fallback}" >&2
  exit 1
fi

fake_make_log_summary_single_quote_fallback_dir="${tmp_dir}/fake_make_log_summary_single_quote_fallback"
mkdir -p "${fake_make_log_summary_single_quote_fallback_dir}"
cat > "${fake_make_log_summary_single_quote_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_SINGLE_QUOTE_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS FAILED_CMD='MAKE_MBD_INTEGRATOR_CHECKS'"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_SINGLE_QUOTE_FALLBACK
chmod +x "${fake_make_log_summary_single_quote_fallback_dir}/make"

if PATH="${fake_make_log_summary_single_quote_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_single_quote_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_single_quote_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_single_quote_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token value uses single quotes" >&2
  cat "${tmp_dir}/log_summary_single_quote_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_single_quote_fallback_fail.log"; then
  echo "FAIL: single-quoted nested-summary value token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_single_quote_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_single_quote_fallback}"; then
  echo "FAIL: single-quoted nested-summary value token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_single_quote_fallback_fail.log" >&2
  cat "${summary_out_log_summary_single_quote_fallback}" >&2
  exit 1
fi

fake_make_log_summary_quote_mixed_fallback_dir="${tmp_dir}/fake_make_log_summary_quote_mixed_fallback"
mkdir -p "${fake_make_log_summary_quote_mixed_fallback_dir}"
cat > "${fake_make_log_summary_quote_mixed_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_QUOTE_MIXED_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=\"INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_QUOTE_MIXED_FALLBACK
chmod +x "${fake_make_log_summary_quote_mixed_fallback_dir}/make"

if PATH="${fake_make_log_summary_quote_mixed_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_quote_mixed_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_quote_mixed_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_quote_mixed_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary tokens mix malformed quotes" >&2
  cat "${tmp_dir}/log_summary_quote_mixed_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_quote_mixed_fallback_fail.log"; then
  echo "FAIL: quote-mixed malformed nested-summary token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_quote_mixed_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_quote_mixed_fallback}"; then
  echo "FAIL: quote-mixed malformed nested-summary token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_quote_mixed_fallback_fail.log" >&2
  cat "${summary_out_log_summary_quote_mixed_fallback}" >&2
  exit 1
fi

fake_make_log_summary_backslash_value_fallback_dir="${tmp_dir}/fake_make_log_summary_backslash_value_fallback"
mkdir -p "${fake_make_log_summary_backslash_value_fallback_dir}"
cat > "${fake_make_log_summary_backslash_value_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_BACKSLASH_VALUE_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD\_INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_BACKSLASH_VALUE_FALLBACK
chmod +x "${fake_make_log_summary_backslash_value_fallback_dir}/make"

if PATH="${fake_make_log_summary_backslash_value_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_backslash_value_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_backslash_value_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_backslash_value_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token value includes backslash" >&2
  cat "${tmp_dir}/log_summary_backslash_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_backslash_value_fallback_fail.log"; then
  echo "FAIL: backslash-containing nested-summary value token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_backslash_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_backslash_value_fallback}"; then
  echo "FAIL: backslash-containing nested-summary value token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_backslash_value_fallback_fail.log" >&2
  cat "${summary_out_log_summary_backslash_value_fallback}" >&2
  exit 1
fi

fake_make_log_summary_quoted_key_fallback_dir="${tmp_dir}/fake_make_log_summary_quoted_key_fallback"
mkdir -p "${fake_make_log_summary_quoted_key_fallback_dir}"
cat > "${fake_make_log_summary_quoted_key_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_QUOTED_KEY_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail "FAILED_STEP"=INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_QUOTED_KEY_FALLBACK
chmod +x "${fake_make_log_summary_quoted_key_fallback_dir}/make"

if PATH="${fake_make_log_summary_quoted_key_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_quoted_key_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_quoted_key_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_quoted_key_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token key is quoted" >&2
  cat "${tmp_dir}/log_summary_quoted_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_quoted_key_fallback_fail.log"; then
  echo "FAIL: quoted nested-summary key token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_quoted_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_quoted_key_fallback}"; then
  echo "FAIL: quoted nested-summary key token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_quoted_key_fallback_fail.log" >&2
  cat "${summary_out_log_summary_quoted_key_fallback}" >&2
  exit 1
fi

fake_make_log_summary_single_quoted_key_fallback_dir="${tmp_dir}/fake_make_log_summary_single_quoted_key_fallback"
mkdir -p "${fake_make_log_summary_single_quoted_key_fallback_dir}"
cat > "${fake_make_log_summary_single_quoted_key_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_SINGLE_QUOTED_KEY_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail 'FAILED_STEP'=INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_SINGLE_QUOTED_KEY_FALLBACK
chmod +x "${fake_make_log_summary_single_quoted_key_fallback_dir}/make"

if PATH="${fake_make_log_summary_single_quoted_key_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_single_quoted_key_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_single_quoted_key_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_single_quoted_key_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token key is single-quoted" >&2
  cat "${tmp_dir}/log_summary_single_quoted_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_single_quoted_key_fallback_fail.log"; then
  echo "FAIL: single-quoted nested-summary key token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_single_quoted_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_single_quoted_key_fallback}"; then
  echo "FAIL: single-quoted nested-summary key token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_single_quoted_key_fallback_fail.log" >&2
  cat "${summary_out_log_summary_single_quoted_key_fallback}" >&2
  exit 1
fi

fake_make_log_summary_quoted_cmd_key_fallback_dir="${tmp_dir}/fake_make_log_summary_quoted_cmd_key_fallback"
mkdir -p "${fake_make_log_summary_quoted_cmd_key_fallback_dir}"
cat > "${fake_make_log_summary_quoted_cmd_key_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_QUOTED_CMD_KEY_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo 'a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS "FAILED_CMD"=MAKE_MBD_INTEGRATOR_CHECKS'
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_QUOTED_CMD_KEY_FALLBACK
chmod +x "${fake_make_log_summary_quoted_cmd_key_fallback_dir}/make"

if PATH="${fake_make_log_summary_quoted_cmd_key_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_quoted_cmd_key_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_quoted_cmd_key_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_quoted_cmd_key_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary command key is quoted" >&2
  cat "${tmp_dir}/log_summary_quoted_cmd_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_quoted_cmd_key_fallback_fail.log"; then
  echo "FAIL: quoted nested-summary command-key token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_quoted_cmd_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_quoted_cmd_key_fallback}"; then
  echo "FAIL: quoted nested-summary command-key token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_quoted_cmd_key_fallback_fail.log" >&2
  cat "${summary_out_log_summary_quoted_cmd_key_fallback}" >&2
  exit 1
fi

fake_make_log_summary_partial_fallback_dir="${tmp_dir}/fake_make_log_summary_partial_fallback"
mkdir -p "${fake_make_log_summary_partial_fallback_dir}"
cat > "${fake_make_log_summary_partial_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_PARTIAL_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=CI_CONTRACT"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_PARTIAL_FALLBACK
chmod +x "${fake_make_log_summary_partial_fallback_dir}/make"

if PATH="${fake_make_log_summary_partial_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_partial_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_partial_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_partial_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary is partial and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_partial_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_partial_fallback_fail.log"; then
  echo "FAIL: partial nested-summary path should defer to preflight fallback instead of partial summary fields" >&2
  cat "${tmp_dir}/log_summary_partial_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_partial_fallback}"; then
  echo "FAIL: partial nested-summary path should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_partial_fallback_fail.log" >&2
  cat "${summary_out_log_summary_partial_fallback}" >&2
  exit 1
fi

fake_make_log_summary_missing_equals_fallback_dir="${tmp_dir}/fake_make_log_summary_missing_equals_fallback"
mkdir -p "${fake_make_log_summary_missing_equals_fallback_dir}"
cat > "${fake_make_log_summary_missing_equals_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_MISSING_EQUALS_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_MISSING_EQUALS_FALLBACK
chmod +x "${fake_make_log_summary_missing_equals_fallback_dir}/make"

if PATH="${fake_make_log_summary_missing_equals_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_missing_equals_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_missing_equals_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_missing_equals_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token misses '=' and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_missing_equals_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_missing_equals_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary token without '=' should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_missing_equals_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_missing_equals_fallback}"; then
  echo "FAIL: malformed nested-summary token without '=' should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_missing_equals_fallback_fail.log" >&2
  cat "${summary_out_log_summary_missing_equals_fallback}" >&2
  exit 1
fi

fake_make_log_summary_empty_value_fallback_dir="${tmp_dir}/fake_make_log_summary_empty_value_fallback"
mkdir -p "${fake_make_log_summary_empty_value_fallback_dir}"
cat > "${fake_make_log_summary_empty_value_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_EMPTY_VALUE_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP= FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_EMPTY_VALUE_FALLBACK
chmod +x "${fake_make_log_summary_empty_value_fallback_dir}/make"

if PATH="${fake_make_log_summary_empty_value_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_empty_value_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_empty_value_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_empty_value_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token has empty value and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_empty_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_empty_value_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary empty-value token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_empty_value_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_empty_value_fallback}"; then
  echo "FAIL: malformed nested-summary empty-value token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_empty_value_fallback_fail.log" >&2
  cat "${summary_out_log_summary_empty_value_fallback}" >&2
  exit 1
fi

fake_make_log_summary_empty_key_fallback_dir="${tmp_dir}/fake_make_log_summary_empty_key_fallback"
mkdir -p "${fake_make_log_summary_empty_key_fallback_dir}"
cat > "${fake_make_log_summary_empty_key_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_EMPTY_KEY_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail =INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_EMPTY_KEY_FALLBACK
chmod +x "${fake_make_log_summary_empty_key_fallback_dir}/make"

if PATH="${fake_make_log_summary_empty_key_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_empty_key_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_empty_key_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_empty_key_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token has empty key and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_empty_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_empty_key_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary empty-key token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_empty_key_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_empty_key_fallback}"; then
  echo "FAIL: malformed nested-summary empty-key token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_empty_key_fallback_fail.log" >&2
  cat "${summary_out_log_summary_empty_key_fallback}" >&2
  exit 1
fi

fake_make_log_summary_extra_equals_fallback_dir="${tmp_dir}/fake_make_log_summary_extra_equals_fallback"
mkdir -p "${fake_make_log_summary_extra_equals_fallback_dir}"
cat > "${fake_make_log_summary_extra_equals_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_EXTRA_EQUALS_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR=CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_EXTRA_EQUALS_FALLBACK
chmod +x "${fake_make_log_summary_extra_equals_fallback_dir}/make"

if PATH="${fake_make_log_summary_extra_equals_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_extra_equals_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_extra_equals_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_extra_equals_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token has extra '=' and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_extra_equals_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_extra_equals_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary extra '=' token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_extra_equals_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_extra_equals_fallback}"; then
  echo "FAIL: malformed nested-summary extra '=' token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_extra_equals_fallback_fail.log" >&2
  cat "${summary_out_log_summary_extra_equals_fallback}" >&2
  exit 1
fi

fake_make_log_summary_leading_punct_fallback_dir="${tmp_dir}/fake_make_log_summary_leading_punct_fallback"
mkdir -p "${fake_make_log_summary_leading_punct_fallback_dir}"
cat > "${fake_make_log_summary_leading_punct_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_LEADING_PUNCT_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=.INTEGRATOR_CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_LEADING_PUNCT_FALLBACK
chmod +x "${fake_make_log_summary_leading_punct_fallback_dir}/make"

if PATH="${fake_make_log_summary_leading_punct_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_leading_punct_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_leading_punct_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_leading_punct_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token has leading punctuation and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_leading_punct_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_leading_punct_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary leading-punctuation token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_leading_punct_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_leading_punct_fallback}"; then
  echo "FAIL: malformed nested-summary leading-punctuation token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_leading_punct_fallback_fail.log" >&2
  cat "${summary_out_log_summary_leading_punct_fallback}" >&2
  exit 1
fi

fake_make_log_summary_internal_symbol_fallback_dir="${tmp_dir}/fake_make_log_summary_internal_symbol_fallback"
mkdir -p "${fake_make_log_summary_internal_symbol_fallback_dir}"
cat > "${fake_make_log_summary_internal_symbol_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_INTERNAL_SYMBOL_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR-CHECKS FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS"
  echo "FAIL: MBD INTEGRATOR CHECKER REQUIRES EXECUTABLE FEM4C BINARY (/tmp/fake_missing_fem4c_bin)" >&2
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_INTERNAL_SYMBOL_FALLBACK
chmod +x "${fake_make_log_summary_internal_symbol_fallback_dir}/make"

if PATH="${fake_make_log_summary_internal_symbol_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_internal_symbol_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_internal_symbol_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_internal_symbol_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary token has internal symbol and fallback preflight exists" >&2
  cat "${tmp_dir}/log_summary_internal_symbol_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_internal_symbol_fallback_fail.log"; then
  echo "FAIL: malformed nested-summary internal-symbol token should defer to preflight fallback in full summary" >&2
  cat "${tmp_dir}/log_summary_internal_symbol_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_internal_symbol_fallback}"; then
  echo "FAIL: malformed nested-summary internal-symbol token should persist fallback failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_internal_symbol_fallback_fail.log" >&2
  cat "${summary_out_log_summary_internal_symbol_fallback}" >&2
  exit 1
fi

fake_make_log_summary_trailing_punct_dir="${tmp_dir}/fake_make_log_summary_trailing_punct"
mkdir -p "${fake_make_log_summary_trailing_punct_dir}"
cat > "${fake_make_log_summary_trailing_punct_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_TRAILING_PUNCT'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  echo "a24_regression_summary contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail FAILED_STEP=INTEGRATOR_CHECKS, FAILED_CMD=MAKE_MBD_INTEGRATOR_CHECKS;"
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_TRAILING_PUNCT
chmod +x "${fake_make_log_summary_trailing_punct_dir}/make"

if PATH="${fake_make_log_summary_trailing_punct_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_trailing_punct_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_trailing_punct}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_trailing_punct_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary values include trailing punctuation" >&2
  cat "${tmp_dir}/log_summary_trailing_punct_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_trailing_punct_fail.log"; then
  echo "FAIL: trailing-punctuation nested-summary values should be normalized into full summary" >&2
  cat "${tmp_dir}/log_summary_trailing_punct_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_trailing_punct}"; then
  echo "FAIL: trailing-punctuation nested-summary values should persist normalized failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_trailing_punct_fail.log" >&2
  cat "${summary_out_log_summary_trailing_punct}" >&2
  exit 1
fi

fake_make_log_summary_crlf_fallback_dir="${tmp_dir}/fake_make_log_summary_crlf_fallback"
mkdir -p "${fake_make_log_summary_crlf_fallback_dir}"
cat > "${fake_make_log_summary_crlf_fallback_dir}/make" <<'EOF_MAKE_LOG_SUMMARY_CRLF_FALLBACK'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == *"mbd_a24_regression"* ]]; then
  printf 'A24_REGRESSION_SUMMARY contract_test=0 lock=acquired integrator_attempts=1 ci_contract_attempts=1 ci_contract_test_attempts=0 overall=fail failed_step=integrator_checks failed_cmd=make_mbd_integrator_checks\r\n'
  exit 2
fi
exit 0
EOF_MAKE_LOG_SUMMARY_CRLF_FALLBACK
chmod +x "${fake_make_log_summary_crlf_fallback_dir}/make"

if PATH="${fake_make_log_summary_crlf_fallback_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/log_summary_crlf_fallback_lock" A24_FULL_SUMMARY_OUT="${summary_out_log_summary_crlf_fallback}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/log_summary_crlf_fallback_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested summary log line contains CRLF terminator" >&2
  cat "${tmp_dir}/log_summary_crlf_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/log_summary_crlf_fallback_fail.log"; then
  echo "FAIL: log-summary CRLF fallback path should propagate nested summary failure into full summary" >&2
  cat "${tmp_dir}/log_summary_crlf_fallback_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_log_summary_crlf_fallback}"; then
  echo "FAIL: log-summary CRLF fallback path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/log_summary_crlf_fallback_fail.log" >&2
  cat "${summary_out_log_summary_crlf_fallback}" >&2
  exit 1
fi

if A24_RUN_CONTRACT_TEST=0 FEM4C_MBD_BIN="${tmp_dir}/missing_fem4c_bin" A24_FULL_LOCK_DIR="${tmp_dir}/missing_bin_lock" A24_FULL_SUMMARY_OUT="${summary_out_missing_bin}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/missing_bin_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested mbd_integrator_checks cannot resolve fem4c binary" >&2
  cat "${tmp_dir}/missing_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "requires executable fem4c binary" "${tmp_dir}/missing_bin_fail.log"; then
  echo "FAIL: missing-bin preflight diagnostic from nested mbd_integrator_checks was not found" >&2
  cat "${tmp_dir}/missing_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${tmp_dir}/missing_bin_fail.log"; then
  echo "FAIL: missing-bin path should propagate integrator failure into full summary" >&2
  cat "${tmp_dir}/missing_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "${tmp_dir}/missing_bin_fail.log"; then
  echo "FAIL: missing-bin path should emit integrator preflight diagnosis hint" >&2
  cat "${tmp_dir}/missing_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_missing_bin}"; then
  echo "FAIL: missing-bin path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/missing_bin_fail.log" >&2
  cat "${summary_out_missing_bin}" >&2
  exit 1
fi

nonexec_bin="${tmp_dir}/nonexec_fem4c_bin"
printf '#!/usr/bin/env bash\nexit 0\n' >"${nonexec_bin}"
chmod 644 "${nonexec_bin}"
if A24_RUN_CONTRACT_TEST=0 FEM4C_MBD_BIN="${nonexec_bin}" A24_FULL_LOCK_DIR="${tmp_dir}/nonexec_bin_lock" A24_FULL_SUMMARY_OUT="${summary_out_nonexec_bin}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/nonexec_bin_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when nested mbd_integrator_checks sees non-executable fem4c binary" >&2
  cat "${tmp_dir}/nonexec_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "requires executable fem4c binary" "${tmp_dir}/nonexec_bin_fail.log"; then
  echo "FAIL: non-executable-bin preflight diagnostic from nested mbd_integrator_checks was not found" >&2
  cat "${tmp_dir}/nonexec_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" "${tmp_dir}/nonexec_bin_fail.log"; then
  echo "FAIL: non-executable-bin path should emit integrator preflight diagnosis hint" >&2
  cat "${tmp_dir}/nonexec_bin_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*regression_attempts=1 .*overall=fail failed_step=regression_integrator_checks failed_cmd=make_mbd_integrator_checks$" "${summary_out_nonexec_bin}"; then
  echo "FAIL: non-executable-bin path should persist propagated failure into full summary output file" >&2
  cat "${tmp_dir}/nonexec_bin_fail.log" >&2
  cat "${summary_out_nonexec_bin}" >&2
  exit 1
fi

script_summary_copy="${root_dir}/FEM4C/scripts/.tmp_run_a24_regression_full_no_summary.sh"
cp "FEM4C/scripts/run_a24_regression_full.sh" "${script_summary_copy}"
sed -i 's/A24_FULL_SUMMARY/A24_FULL_SUMMARY_BROKEN/g' "${script_summary_copy}"

mkdir -p "${active_lock_dir_for_summary}"
printf '%s\n' "$$" >"${active_lock_dir_for_summary}/pid"
if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${active_lock_dir_for_summary}" bash "${script_summary_copy}" >"${tmp_dir}/missing_summary.log" 2>&1; then
  echo "FAIL: full-summary-missing variant should fail fast on lock-held path" >&2
  cat "${tmp_dir}/missing_summary.log" >&2
  exit 1
fi

if grep -q "^A24_FULL_SUMMARY " "${tmp_dir}/missing_summary.log"; then
  echo "FAIL: full-summary-missing variant unexpectedly emitted canonical A24_FULL_SUMMARY marker" >&2
  cat "${tmp_dir}/missing_summary.log" >&2
  exit 1
fi

mkdir -p "${active_lock_dir}"
printf '%s\n' "$$" >"${active_lock_dir}/pid"
if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${active_lock_dir}" A24_FULL_SUMMARY_OUT="${summary_out_lock}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/lock_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when lock directory is already held" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: a24 full lock is already held" "${tmp_dir}/lock_fail.log"; then
  echo "FAIL: expected full lock-held diagnostic was not found" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=lock failed_cmd=lock$" "${tmp_dir}/lock_fail.log"; then
  echo "FAIL: lock-held path is missing fail summary for full lock step" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=lock failed_cmd=lock$" "${summary_out_lock}"; then
  echo "FAIL: lock-held path is missing fail summary in full-summary output file" >&2
  cat "${tmp_dir}/lock_fail.log" >&2
  cat "${summary_out_lock}" >&2
  exit 1
fi

fake_make_dir="${tmp_dir}/fake_make_retry"
mkdir -p "${fake_make_dir}"
cat > "${fake_make_dir}/make" <<EOF_MAKE
#!/usr/bin/env bash
set -euo pipefail
state_file="${tmp_dir}/retry137_full.state"
if [[ "\$*" == *" clean"* ]] && [[ ! -f "\${state_file}" ]]; then
  touch "\${state_file}"
  exit 137
fi
exit 0
EOF_MAKE
chmod +x "${fake_make_dir}/make"

if ! PATH="${fake_make_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/retry_lock" A24_FULL_SUMMARY_OUT="${summary_out_retry}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/retry_pass.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should pass when rc=137 is retried once" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if ! grep -q "WARN: clean failed with rc=137; retrying once" "${tmp_dir}/retry_pass.log"; then
  echo "FAIL: full retry path did not emit expected retry warning" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=2 .*build_attempts=1 .*regression_attempts=1 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/retry_pass.log"; then
  echo "FAIL: full retry-pass path is missing retry_used=1 summary marker" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=2 .*build_attempts=1 .*regression_attempts=1 .*overall=pass failed_step=none failed_cmd=none$" "${summary_out_retry}"; then
  echo "FAIL: full retry-pass path did not persist retry_used=1 summary to output file" >&2
  cat "${tmp_dir}/retry_pass.log" >&2
  cat "${summary_out_retry}" >&2
  exit 1
fi

fake_make_retry_build_dir="${tmp_dir}/fake_make_retry_build"
mkdir -p "${fake_make_retry_build_dir}"
cat > "${fake_make_retry_build_dir}/make" <<EOF_MAKE
#!/usr/bin/env bash
set -euo pipefail
state_file="${tmp_dir}/retry137_full_build.state"
if [[ "\$*" == "-C FEM4C" ]] && [[ ! -f "\${state_file}" ]]; then
  touch "\${state_file}"
  exit 137
fi
exit 0
EOF_MAKE
chmod +x "${fake_make_retry_build_dir}/make"

if ! PATH="${fake_make_retry_build_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/retry_build_lock" A24_FULL_SUMMARY_OUT="${summary_out_retry_build}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/retry_build_pass.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should pass when build rc=137 is retried once" >&2
  cat "${tmp_dir}/retry_build_pass.log" >&2
  exit 1
fi

if ! grep -q "WARN: build failed with rc=137; retrying once" "${tmp_dir}/retry_build_pass.log"; then
  echo "FAIL: full build retry path did not emit expected retry warning" >&2
  cat "${tmp_dir}/retry_build_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=1 .*build_attempts=2 .*regression_attempts=1 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/retry_build_pass.log"; then
  echo "FAIL: full build retry-pass path is missing retry_used=1 summary marker" >&2
  cat "${tmp_dir}/retry_build_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=1 .*build_attempts=2 .*regression_attempts=1 .*overall=pass failed_step=none failed_cmd=none$" "${summary_out_retry_build}"; then
  echo "FAIL: full build retry-pass path did not persist retry summary to output file" >&2
  cat "${tmp_dir}/retry_build_pass.log" >&2
  cat "${summary_out_retry_build}" >&2
  exit 1
fi

fake_make_retry_regression_dir="${tmp_dir}/fake_make_retry_regression"
mkdir -p "${fake_make_retry_regression_dir}"
cat > "${fake_make_retry_regression_dir}/make" <<EOF_MAKE
#!/usr/bin/env bash
set -euo pipefail
state_file="${tmp_dir}/retry137_full_regression.state"
if [[ "\$*" == *"mbd_a24_regression"* ]] && [[ ! -f "\${state_file}" ]]; then
  touch "\${state_file}"
  exit 137
fi
exit 0
EOF_MAKE
chmod +x "${fake_make_retry_regression_dir}/make"

if ! PATH="${fake_make_retry_regression_dir}:$PATH" A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/retry_regression_lock" A24_FULL_SUMMARY_OUT="${summary_out_retry_regression}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/retry_regression_pass.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should pass when regression rc=137 is retried once" >&2
  cat "${tmp_dir}/retry_regression_pass.log" >&2
  exit 1
fi

if ! grep -q "WARN: regression failed with rc=137; retrying once" "${tmp_dir}/retry_regression_pass.log"; then
  echo "FAIL: full regression retry path did not emit expected retry warning" >&2
  cat "${tmp_dir}/retry_regression_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=1 .*build_attempts=1 .*regression_attempts=2 .*overall=pass failed_step=none failed_cmd=none$" "${tmp_dir}/retry_regression_pass.log"; then
  echo "FAIL: full regression retry-pass path is missing retry_used=1 summary marker" >&2
  cat "${tmp_dir}/retry_regression_pass.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*retry_on_137=1 retry_used=1 .*clean_attempts=1 .*build_attempts=1 .*regression_attempts=2 .*overall=pass failed_step=none failed_cmd=none$" "${summary_out_retry_regression}"; then
  echo "FAIL: full regression retry-pass path did not persist retry summary to output file" >&2
  cat "${tmp_dir}/retry_regression_pass.log" >&2
  cat "${summary_out_retry_regression}" >&2
  exit 1
fi

if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/config_lock" A24_FULL_RETRY_ON_137=2 A24_FULL_SUMMARY_OUT="${summary_out_config}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/config_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when retry knob is out of allowed range" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24_FULL_RETRY_ON_137 must be 0 or 1" "${tmp_dir}/config_fail.log"; then
  echo "FAIL: expected full retry knob validation error was not found" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=retry_on_137$" "${tmp_dir}/config_fail.log"; then
  echo "FAIL: full config-fail path is missing fail summary for retry knob" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=retry_on_137$" "${summary_out_config}"; then
  echo "FAIL: full config-fail path is missing fail summary in output file" >&2
  cat "${tmp_dir}/config_fail.log" >&2
  cat "${summary_out_config}" >&2
  exit 1
fi

if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/summary_missing_dir_lock" A24_FULL_SUMMARY_OUT="${tmp_dir}/missing_full_summary_dir/summary.log" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/summary_missing_dir_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when full-summary parent directory is missing" >&2
  cat "${tmp_dir}/summary_missing_dir_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24 full summary output directory does not exist" "${tmp_dir}/summary_missing_dir_fail.log"; then
  echo "FAIL: expected full-summary missing-dir validation error was not found" >&2
  cat "${tmp_dir}/summary_missing_dir_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=summary_out_dir$" "${tmp_dir}/summary_missing_dir_fail.log"; then
  echo "FAIL: full-summary missing-dir path is missing fail summary marker" >&2
  cat "${tmp_dir}/summary_missing_dir_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/summary_dir_path"
if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/summary_dir_path_lock" A24_FULL_SUMMARY_OUT="${tmp_dir}/summary_dir_path" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/summary_dir_path_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when full-summary path points to a directory" >&2
  cat "${tmp_dir}/summary_dir_path_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: A24 full summary output path must be a file" "${tmp_dir}/summary_dir_path_fail.log"; then
  echo "FAIL: expected full-summary path-type validation error was not found" >&2
  cat "${tmp_dir}/summary_dir_path_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=summary_out_type$" "${tmp_dir}/summary_dir_path_fail.log"; then
  echo "FAIL: full-summary dir-path path is missing fail summary marker" >&2
  cat "${tmp_dir}/summary_dir_path_fail.log" >&2
  exit 1
fi

mkdir -p "${tmp_dir}/summary_parent_readonly"
chmod 555 "${tmp_dir}/summary_parent_readonly"
if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/summary_readonly_parent_lock" A24_FULL_SUMMARY_OUT="${tmp_dir}/summary_parent_readonly/summary.log" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/summary_readonly_parent_fail.log" 2>&1; then
  echo "FAIL: run_a24_regression_full should fail when full-summary parent directory is not writable" >&2
  cat "${tmp_dir}/summary_readonly_parent_fail.log" >&2
  exit 1
fi

if ! grep -q "FAIL: cannot write A24 full summary output" "${tmp_dir}/summary_readonly_parent_fail.log"; then
  echo "FAIL: expected full-summary readonly-parent validation error was not found" >&2
  cat "${tmp_dir}/summary_readonly_parent_fail.log" >&2
  exit 1
fi

if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=summary_out_write$" "${tmp_dir}/summary_readonly_parent_fail.log"; then
  echo "FAIL: full-summary readonly-parent path is missing fail summary marker" >&2
  cat "${tmp_dir}/summary_readonly_parent_fail.log" >&2
  exit 1
fi

full_summary_write_probe="/proc/1/cmdline"
if ! bash -c ': >"$1"' _ "${full_summary_write_probe}" >/dev/null 2>&1; then
  if A24_RUN_CONTRACT_TEST=0 A24_FULL_LOCK_DIR="${tmp_dir}/summary_write_fail_lock" A24_FULL_SUMMARY_OUT="${full_summary_write_probe}" bash "FEM4C/scripts/run_a24_regression_full.sh" >"${tmp_dir}/summary_write_fail.log" 2>&1; then
    echo "FAIL: run_a24_regression_full should fail when full-summary output is not writable" >&2
    cat "${tmp_dir}/summary_write_fail.log" >&2
    exit 1
  fi

  if ! grep -q "FAIL: cannot write A24 full summary output" "${tmp_dir}/summary_write_fail.log"; then
    echo "FAIL: expected full-summary write validation error was not found" >&2
    cat "${tmp_dir}/summary_write_fail.log" >&2
    exit 1
  fi

  if ! grep -q "^A24_FULL_SUMMARY .*clean_attempts=0 .*build_attempts=0 .*regression_attempts=0 .*overall=fail failed_step=config failed_cmd=summary_out_write$" "${tmp_dir}/summary_write_fail.log"; then
    echo "FAIL: full-summary write-fail path is missing fail summary marker" >&2
    cat "${tmp_dir}/summary_write_fail.log" >&2
    exit 1
  fi
fi

echo "PASS: run_a24_regression_full self-test (pass + summary-output + expected fail path + missing-summary + lock-held path + retry-knob/retry-used validation)"
