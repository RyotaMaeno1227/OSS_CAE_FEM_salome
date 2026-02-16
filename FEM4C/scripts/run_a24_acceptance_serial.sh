#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

lock_dir="${A24_ACCEPT_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_accept_serial.lock}"
lock_pid_file="${lock_dir}/pid"
summary_out="${A24_ACCEPT_SERIAL_SUMMARY_OUT:-}"
retry_on_137="${A24_ACCEPT_SERIAL_RETRY_ON_137:-1}"
fake_137_step="${A24_ACCEPT_SERIAL_FAKE_137_STEP:-none}"
step_log_dir="${A24_ACCEPT_SERIAL_STEP_LOG_DIR:-}"
full_test_status="skip"
batch_test_status="skip"
ci_contract_test_status="skip"
full_test_attempts="0"
batch_test_attempts="0"
ci_contract_test_attempts="0"
full_test_retry_used="0"
batch_test_retry_used="0"
ci_contract_test_retry_used="0"
lock_status="not_acquired"

if [[ "${retry_on_137}" != "0" && "${retry_on_137}" != "1" ]]; then
    echo "FAIL: A24_ACCEPT_SERIAL_RETRY_ON_137 must be 0 or 1 (got: ${retry_on_137})" >&2
    exit 1
fi
if [[ "${fake_137_step}" != "none" && "${fake_137_step}" != "full_test" && "${fake_137_step}" != "batch_test" && "${fake_137_step}" != "ci_contract_test" ]]; then
    echo "FAIL: A24_ACCEPT_SERIAL_FAKE_137_STEP must be one of none/full_test/batch_test/ci_contract_test (got: ${fake_137_step})" >&2
    exit 1
fi
if [[ -n "${step_log_dir}" ]]; then
    if [[ -e "${step_log_dir}" && ! -d "${step_log_dir}" ]]; then
        echo "FAIL: A24 acceptance serial step-log dir must be a directory (${step_log_dir})" >&2
        exit 1
    fi
    if ! mkdir -p "${step_log_dir}" 2>/dev/null; then
        echo "FAIL: cannot create A24 acceptance serial step-log dir (${step_log_dir})" >&2
        exit 1
    fi
    if [[ ! -w "${step_log_dir}" ]]; then
        echo "FAIL: A24 acceptance serial step-log dir is not writable (${step_log_dir})" >&2
        exit 1
    fi
fi

emit_summary() {
    local line="$1"
    echo "${line}"
    if [[ -n "${summary_out}" ]]; then
        if ! printf '%s\n' "${line}" >"${summary_out}"; then
            echo "FAIL: cannot write A24 acceptance serial summary output (${summary_out})" >&2
            return 1
        fi
    fi
}

print_summary() {
    local overall="$1"
    local failed_step="${2:-none}"
    local failed_cmd="${3:-none}"
    local failed_rc="${4:-0}"
    local failed_log="${5:-none}"
    local line
    line="A24_ACCEPT_SERIAL_SUMMARY lock=${lock_status} retry_on_137=${retry_on_137} fake_137_step=${fake_137_step} step_log_dir=${step_log_dir:-none} full_test=${full_test_status} full_test_attempts=${full_test_attempts} full_test_retry_used=${full_test_retry_used} batch_test=${batch_test_status} batch_test_attempts=${batch_test_attempts} batch_test_retry_used=${batch_test_retry_used} ci_contract_test=${ci_contract_test_status} ci_contract_test_attempts=${ci_contract_test_attempts} ci_contract_test_retry_used=${ci_contract_test_retry_used} overall=${overall} failed_step=${failed_step} failed_cmd=${failed_cmd} failed_rc=${failed_rc} failed_log=${failed_log}"
    emit_summary "${line}"
}

cleanup() {
    if [[ -n "${lock_dir}" && -d "${lock_dir}" ]]; then
        rm -f "${lock_pid_file}" 2>/dev/null || true
        rmdir "${lock_dir}" 2>/dev/null || true
    fi
}

acquire_lock() {
    local owner_pid
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
                echo "INFO: recovered stale a24 acceptance lock (${lock_dir})"
                return 0
            fi
        fi
    else
        rm -rf "${lock_dir}" 2>/dev/null || true
        if mkdir "${lock_dir}" 2>/dev/null; then
            echo "$$" >"${lock_pid_file}"
            lock_status="acquired_stale_recovered"
            echo "INFO: recovered stale a24 acceptance lock without pid (${lock_dir})"
            return 0
        fi
    fi

    return 1
}

run_step() {
    local step_name="$1"
    local target_name="$2"
    local status_var_name="$3"
    local attempt_var_name="$4"
    local retry_var_name="$5"
    local current_attempt
    local step_rc=0
    local step_log_file=""

    current_attempt="${!attempt_var_name}"
    current_attempt=$((current_attempt + 1))
    printf -v "${attempt_var_name}" '%d' "${current_attempt}"
    if [[ -n "${step_log_dir}" ]]; then
        step_log_file="${step_log_dir%/}/${step_name}.attempt${current_attempt}.log"
    fi

    if [[ "${fake_137_step}" == "${step_name}" && "${current_attempt}" -eq 1 ]]; then
        step_rc=137
        if [[ -n "${step_log_file}" ]]; then
            printf 'INFO: simulate rc=137 for %s on first attempt (A24_ACCEPT_SERIAL_FAKE_137_STEP=%s)\n' "${step_name}" "${fake_137_step}" >"${step_log_file}"
        fi
        echo "INFO: simulate rc=137 for ${step_name} on first attempt (A24_ACCEPT_SERIAL_FAKE_137_STEP=${fake_137_step})"
    elif [[ -n "${step_log_file}" ]]; then
        if env -u MAKEFLAGS -u MFLAGS make -C FEM4C "${target_name}" >"${step_log_file}" 2>&1; then
            printf -v "${status_var_name}" '%s' "pass"
            return 0
        else
            step_rc=$?
            cat "${step_log_file}" >&2
        fi
    elif env -u MAKEFLAGS -u MFLAGS make -C FEM4C "${target_name}"; then
        printf -v "${status_var_name}" '%s' "pass"
        return 0
    else
        step_rc=$?
    fi

    if [[ "${retry_on_137}" == "1" && "${step_rc}" -eq 137 ]]; then
        printf -v "${retry_var_name}" '%s' "1"
        echo "WARN: ${step_name} failed with rc=137; retrying once"
        current_attempt=$((current_attempt + 1))
        printf -v "${attempt_var_name}" '%d' "${current_attempt}"
        if [[ -n "${step_log_dir}" ]]; then
            step_log_file="${step_log_dir%/}/${step_name}.attempt${current_attempt}.log"
        fi
        if [[ -n "${step_log_file}" ]]; then
            if env -u MAKEFLAGS -u MFLAGS make -C FEM4C "${target_name}" >"${step_log_file}" 2>&1; then
                printf -v "${status_var_name}" '%s' "pass"
                return 0
            else
                step_rc=$?
                cat "${step_log_file}" >&2
            fi
        elif env -u MAKEFLAGS -u MFLAGS make -C FEM4C "${target_name}"; then
            printf -v "${status_var_name}" '%s' "pass"
            return 0
        else
            step_rc=$?
        fi
    fi

    printf -v "${status_var_name}" '%s' "fail"
    if ! print_summary "fail" "${step_name}" "make_${target_name}" "${step_rc}" "${step_log_file:-none}"; then
        return 1
    fi
    return "${step_rc}"
}

if ! acquire_lock; then
    echo "FAIL: a24 acceptance serial lock is already held (${lock_dir})" >&2
    lock_status="held"
    if ! print_summary "fail" "lock" "lock" "1" "none"; then
        exit 1
    fi
    exit 1
fi
trap cleanup EXIT

run_step "full_test" "mbd_a24_regression_full_test" "full_test_status" "full_test_attempts" "full_test_retry_used"
run_step "batch_test" "mbd_a24_batch_test" "batch_test_status" "batch_test_attempts" "batch_test_retry_used"
run_step "ci_contract_test" "mbd_ci_contract_test" "ci_contract_test_status" "ci_contract_test_attempts" "ci_contract_test_retry_used"

if ! print_summary "pass" "none" "none" "0" "none"; then
    exit 1
fi

echo "PASS: a24 acceptance serial (full_test + batch_test + ci_contract_test)"
