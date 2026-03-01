#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

run_contract_test="${A24_RUN_CONTRACT_TEST:-1}"
summary_out="${A24_REGRESSION_SUMMARY_OUT:-}"
skip_lock="${A24_REGRESSION_SKIP_LOCK:-0}"
lock_dir="${A24_REGRESSION_LOCK_DIR:-/tmp/fem4c_a24_regression.lock}"
lock_pid_file="${lock_dir}/pid"
integrator_attempts=0
ci_contract_attempts=0
ci_contract_test_attempts=0
failed_step="none"
failed_cmd="none"
overall="pass"
lock_status="not_used"
summary_out_fail_reason="none"

emit_summary() {
    local summary_line
    summary_line="A24_REGRESSION_SUMMARY contract_test=${run_contract_test} lock=${lock_status} integrator_attempts=${integrator_attempts} ci_contract_attempts=${ci_contract_attempts} ci_contract_test_attempts=${ci_contract_test_attempts} overall=${overall} failed_step=${failed_step} failed_cmd=${failed_cmd}"
    echo "${summary_line}"
    if [[ -n "${summary_out}" ]]; then
        if ! printf '%s\n' "${summary_line}" >"${summary_out}"; then
            echo "FAIL: cannot write A24 regression summary output (${summary_out})" >&2
            return 1
        fi
    fi
}

validate_summary_out() {
    local summary_out_dir

    summary_out_fail_reason="none"
    if [[ -z "${summary_out}" ]]; then
        return 0
    fi

    summary_out_dir="$(dirname "${summary_out}")"
    if [[ ! -d "${summary_out_dir}" ]]; then
        summary_out_fail_reason="summary_out_dir"
        echo "FAIL: A24 regression summary output directory does not exist (${summary_out_dir})" >&2
        return 1
    fi
    if [[ -d "${summary_out}" ]]; then
        summary_out_fail_reason="summary_out_type"
        echo "FAIL: A24 regression summary output path must be a file (${summary_out})" >&2
        return 1
    fi
    if [[ -e "${summary_out}" ]]; then
        if [[ ! -w "${summary_out}" ]]; then
            summary_out_fail_reason="summary_out_write"
            echo "FAIL: cannot write A24 regression summary output (${summary_out})" >&2
            return 1
        fi
    else
        if [[ ! -w "${summary_out_dir}" ]]; then
            summary_out_fail_reason="summary_out_write"
            echo "FAIL: cannot write A24 regression summary output (${summary_out})" >&2
            return 1
        fi
    fi

    return 0
}

cleanup() {
    if [[ "${lock_status}" == "acquired" || "${lock_status}" == "acquired_stale_recovered" ]]; then
        rm -f "${lock_pid_file}" 2>/dev/null || true
        rmdir "${lock_dir}" 2>/dev/null || true
    fi
}

acquire_lock() {
    local owner_pid=""
    if mkdir "${lock_dir}" 2>/dev/null; then
        echo "$$" >"${lock_pid_file}"
        lock_status="acquired"
        return 0
    fi

    if [[ -f "${lock_pid_file}" ]]; then
        owner_pid="$(cat "${lock_pid_file}" 2>/dev/null || true)"
        if [[ -n "${owner_pid}" ]] && ! kill -0 "${owner_pid}" 2>/dev/null; then
            rm -rf "${lock_dir}" 2>/dev/null || true
            if mkdir "${lock_dir}" 2>/dev/null; then
                echo "$$" >"${lock_pid_file}"
                lock_status="acquired_stale_recovered"
                echo "INFO: recovered stale a24 regression lock (${lock_dir})"
                return 0
            fi
        fi
    else
        rm -rf "${lock_dir}" 2>/dev/null || true
        if mkdir "${lock_dir}" 2>/dev/null; then
            echo "$$" >"${lock_pid_file}"
            lock_status="acquired_stale_recovered"
            echo "INFO: recovered stale a24 regression lock without pid (${lock_dir})"
            return 0
        fi
    fi

    return 1
}

run_make() {
    local step_name="$1"
    local target_name="$2"
    local attempt_var_name="$3"
    local current_attempt

    current_attempt="${!attempt_var_name}"
    current_attempt=$((current_attempt + 1))
    printf -v "${attempt_var_name}" '%d' "${current_attempt}"

    local cmd_rc=0
    if env -u MAKEFLAGS -u MFLAGS make -C FEM4C "${target_name}"; then
        return 0
    else
        cmd_rc=$?
    fi

    overall="fail"
    failed_step="${step_name}"
    failed_cmd="make_${target_name}"
    emit_summary
    return "${cmd_rc}"
}

if ! [[ "${run_contract_test}" =~ ^[01]$ ]]; then
    echo "FAIL: A24_RUN_CONTRACT_TEST must be 0 or 1 (${run_contract_test})" >&2
    overall="fail"
    failed_step="config"
    failed_cmd="run_contract_test"
    emit_summary
    exit 1
fi
if ! [[ "${skip_lock}" =~ ^[01]$ ]]; then
    echo "FAIL: A24_REGRESSION_SKIP_LOCK must be 0 or 1 (${skip_lock})" >&2
    overall="fail"
    failed_step="config"
    failed_cmd="skip_lock"
    emit_summary
    exit 1
fi

if ! validate_summary_out; then
    overall="fail"
    failed_step="config"
    failed_cmd="${summary_out_fail_reason}"
    # Keep summary emission deterministic even when summary_out itself is invalid.
    summary_out=""
    emit_summary
    exit 1
fi

if [[ "${skip_lock}" -eq 1 ]]; then
    lock_status="skipped"
else
    if ! acquire_lock; then
        echo "FAIL: a24 regression lock is already held (${lock_dir})" >&2
        lock_status="held"
        overall="fail"
        failed_step="lock"
        failed_cmd="lock"
        emit_summary
        exit 1
    fi
fi
trap cleanup EXIT

run_make "integrator_checks" "mbd_integrator_checks" "integrator_attempts"
run_make "ci_contract" "mbd_ci_contract" "ci_contract_attempts"
if [[ "${run_contract_test}" -eq 1 ]]; then
    run_make "ci_contract_test" "mbd_ci_contract_test" "ci_contract_test_attempts"
else
    echo "INFO: skip mbd_ci_contract_test (A24_RUN_CONTRACT_TEST=0)"
fi

emit_summary
echo "PASS: a24 regression (mbd step trace runtime+static contract + self-tests)"
