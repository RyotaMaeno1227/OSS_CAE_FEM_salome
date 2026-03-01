#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

# Enforce serial make execution to avoid clean/build/run race in nested wrappers.
export MAKEFLAGS="-j1"

lock_dir="${A24_FULL_LOCK_DIR:-${A24_SERIAL_LOCK_DIR:-/tmp/fem4c_a24_batch.lock}}"
lock_pid_file="${lock_dir}/pid"
summary_out="${A24_FULL_SUMMARY_OUT:-}"
summary_out_fail_reason="none"
retry_on_137="${A24_FULL_RETRY_ON_137:-1}"
tmp_dir="$(mktemp -d)"
nested_regression_summary_out="${tmp_dir}/a24_regression.summary"
nested_regression_log="${tmp_dir}/a24_regression.log"
nested_regression_summary_override_applied="0"
nested_failed_step="none"
nested_failed_cmd="none"
retry_used="0"
clean_status="skip"
build_status="skip"
regression_status="skip"
clean_attempts="0"
build_attempts="0"
regression_attempts="0"
lock_status="not_acquired"

emit_summary() {
    local line="$1"
    echo "${line}"
    if [[ -n "${summary_out}" ]]; then
        if ! printf '%s\n' "${line}" >"${summary_out}"; then
            echo "FAIL: cannot write A24 full summary output (${summary_out})" >&2
            return 1
        fi
    fi
}

print_summary() {
    local overall="$1"
    local failed_step="${2:-none}"
    local failed_cmd="${3:-none}"
    local line
    line="A24_FULL_SUMMARY lock=${lock_status} retry_on_137=${retry_on_137} retry_used=${retry_used} clean=${clean_status} clean_attempts=${clean_attempts} build=${build_status} build_attempts=${build_attempts} regression=${regression_status} regression_attempts=${regression_attempts} overall=${overall} failed_step=${failed_step} failed_cmd=${failed_cmd}"
    emit_summary "${line}"
}

cleanup() {
    rm -rf "${tmp_dir}" 2>/dev/null || true
    if [[ -n "${lock_dir}" && -d "${lock_dir}" ]]; then
        rm -f "${lock_pid_file}" 2>/dev/null || true
        rmdir "${lock_dir}" 2>/dev/null || true
    fi
}

extract_nested_regression_failure() {
    local summary_line
    local summary_path="$1"

    nested_failed_step="none"
    nested_failed_cmd="none"
    if [[ ! -f "${summary_path}" ]]; then
        return 1
    fi

    summary_line="$(grep -E '^A24_REGRESSION_SUMMARY[[:space:]]' "${summary_path}" | tail -n 1 || true)"
    if [[ -z "${summary_line}" ]]; then
        summary_line="$(grep -Ei '^A24_REGRESSION_SUMMARY[[:space:]]' "${summary_path}" | tail -n 1 || true)"
    fi
    parse_nested_regression_summary_line "${summary_line}"
}

parse_nested_regression_summary_line() {
    local summary_line="$1"
    local token
    local key
    local value

    nested_failed_step="none"
    nested_failed_cmd="none"
    if [[ -z "${summary_line}" ]]; then
        return 1
    fi
    summary_line="${summary_line//$'\r'/}"

    for token in ${summary_line}; do
        # Ignore malformed tokens (missing '=') so partial lines fall back safely.
        if [[ "${token}" != *=* ]]; then
            continue
        fi
        key="${token%%=*}"
        value="${token#*=}"
        if [[ -z "${key}" || -z "${value}" ]]; then
            continue
        fi
        # Reject malformed key/value pairs and defer to generic fallback.
        if [[ ! "${key}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
            continue
        fi
        if [[ "${value}" == *"="* ]]; then
            continue
        fi
        if [[ "${value}" == *"'"* || "${value}" == *'"'* || "${value}" == *\\* ]]; then
            continue
        fi
        value="${value,,}"
        while [[ -n "${value}" && "${value}" =~ [^a-z0-9_]+$ ]]; do
            value="${value%?}"
        done
        if [[ -z "${value}" ]]; then
            continue
        fi
        if [[ ! "${value}" =~ ^[a-z0-9_]+$ ]]; then
            continue
        fi
        case "${key,,}" in
            failed_step)
                if [[ "${nested_failed_step}" == "none" ]]; then
                    nested_failed_step="${value}"
                fi
                ;;
            failed_cmd)
                if [[ "${nested_failed_cmd}" == "none" ]]; then
                    nested_failed_cmd="${value}"
                fi
                ;;
        esac
    done

    # Treat partial nested summary as invalid and defer to generic log fallback.
    if [[ "${nested_failed_step}" == "none" || "${nested_failed_cmd}" == "none" ]]; then
        return 1
    fi
    return 0
}

extract_nested_regression_failure_from_log() {
    local summary_log="$1"
    local summary_line

    nested_failed_step="none"
    nested_failed_cmd="none"
    if [[ ! -f "${summary_log}" ]]; then
        return 1
    fi

    summary_line="$(grep -E '^A24_REGRESSION_SUMMARY[[:space:]]' "${summary_log}" | tail -n 1 || true)"
    if [[ -z "${summary_line}" ]]; then
        summary_line="$(grep -Ei '^A24_REGRESSION_SUMMARY[[:space:]]' "${summary_log}" | tail -n 1 || true)"
    fi
    # Prefer explicit nested summary fields over generic preflight log fallback.
    if parse_nested_regression_summary_line "${summary_line}"; then
        return 0
    fi

    if grep -qi "requires executable fem4c binary" "${summary_log}"; then
        nested_failed_step="integrator_checks"
        nested_failed_cmd="make_mbd_integrator_checks"
        return 0
    fi

    return 1
}

emit_nested_regression_hint() {
    if [[ "${nested_failed_step}" == "integrator_checks" && "${nested_failed_cmd}" == "make_mbd_integrator_checks" ]]; then
        echo "INFO: nested mbd_integrator_checks failed; verify concurrent make clean/build interference or FEM4C_MBD_BIN path" >&2
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
        echo "FAIL: A24 full summary output directory does not exist (${summary_out_dir})" >&2
        return 1
    fi
    if [[ -d "${summary_out}" ]]; then
        summary_out_fail_reason="summary_out_type"
        echo "FAIL: A24 full summary output path must be a file (${summary_out})" >&2
        return 1
    fi
    if [[ -e "${summary_out}" ]]; then
        if [[ ! -w "${summary_out}" ]]; then
            summary_out_fail_reason="summary_out_write"
            echo "FAIL: cannot write A24 full summary output (${summary_out})" >&2
            return 1
        fi
    else
        if [[ ! -w "${summary_out_dir}" ]]; then
            summary_out_fail_reason="summary_out_write"
            echo "FAIL: cannot write A24 full summary output (${summary_out})" >&2
            return 1
        fi
    fi

    return 0
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
                echo "INFO: recovered stale a24 full lock (${lock_dir})"
                return 0
            fi
        fi
    else
        rm -rf "${lock_dir}" 2>/dev/null || true
        if mkdir "${lock_dir}" 2>/dev/null; then
            echo "$$" >"${lock_pid_file}"
            lock_status="acquired_stale_recovered"
            echo "INFO: recovered stale a24 full lock without pid (${lock_dir})"
            return 0
        fi
    fi

    return 1
}

if ! validate_summary_out; then
    # Keep summary emission deterministic when summary_out itself is invalid.
    summary_out=""
    if ! print_summary "fail" "config" "${summary_out_fail_reason}"; then
        exit 1
    fi
    exit 1
fi

if ! acquire_lock; then
    echo "FAIL: a24 full lock is already held (${lock_dir})" >&2
    lock_status="held"
    if ! print_summary "fail" "lock" "lock"; then
        exit 1
    fi
    exit 1
fi
trap cleanup EXIT

if ! [[ "${retry_on_137}" =~ ^[01]$ ]]; then
    echo "FAIL: A24_FULL_RETRY_ON_137 must be 0 or 1 (${retry_on_137})" >&2
    if ! print_summary "fail" "config" "retry_on_137"; then
        exit 1
    fi
    exit 1
fi

if make -C FEM4C clean; then
    clean_status="pass"
    clean_attempts="1"
else
    clean_rc=$?
    clean_status="fail"
    clean_attempts="1"
    if [[ "${clean_rc}" -eq 137 && "${retry_on_137}" -eq 1 ]]; then
        retry_used="1"
        echo "WARN: clean failed with rc=137; retrying once"
        if make -C FEM4C clean; then
            clean_status="pass"
            clean_attempts="2"
            echo "INFO: clean succeeded after retry"
        else
            clean_attempts="2"
        fi
    fi
    if [[ "${clean_status}" != "pass" ]]; then
        if ! print_summary "fail" "clean" "make_clean"; then
            exit 1
        fi
        exit 1
    fi
fi

if make -C FEM4C; then
    build_status="pass"
    build_attempts="1"
else
    build_rc=$?
    build_status="fail"
    build_attempts="1"
    if [[ "${build_rc}" -eq 137 && "${retry_on_137}" -eq 1 ]]; then
        retry_used="1"
        echo "WARN: build failed with rc=137; retrying once"
        if make -C FEM4C; then
            build_status="pass"
            build_attempts="2"
            echo "INFO: build succeeded after retry"
        else
            build_attempts="2"
        fi
    fi
    if [[ "${build_status}" != "pass" ]]; then
        if ! print_summary "fail" "build" "make_build"; then
            exit 1
        fi
        exit 1
    fi
fi

if [[ -n "${A24_REGRESSION_SUMMARY_OUT:-}" ]]; then
    nested_regression_summary_out="${A24_REGRESSION_SUMMARY_OUT}"
else
    export A24_REGRESSION_SUMMARY_OUT="${nested_regression_summary_out}"
    nested_regression_summary_override_applied="1"
fi

if make -C FEM4C mbd_a24_regression >"${nested_regression_log}" 2>&1; then
    cat "${nested_regression_log}"
    regression_status="pass"
    regression_attempts="1"
else
    regression_rc=$?
    cat "${nested_regression_log}"
    regression_status="fail"
    regression_attempts="1"
    if [[ "${regression_rc}" -eq 137 && "${retry_on_137}" -eq 1 ]]; then
        retry_used="1"
        echo "WARN: regression failed with rc=137; retrying once"
        if make -C FEM4C mbd_a24_regression >"${nested_regression_log}" 2>&1; then
            cat "${nested_regression_log}"
            regression_status="pass"
            regression_attempts="2"
            echo "INFO: regression succeeded after retry"
        else
            cat "${nested_regression_log}"
            regression_attempts="2"
        fi
    fi
    if [[ "${regression_status}" != "pass" ]]; then
        local_failed_step="regression"
        local_failed_cmd="make_mbd_a24_regression"
        if extract_nested_regression_failure "${nested_regression_summary_out}"; then
            local_failed_step="regression_${nested_failed_step}"
            local_failed_cmd="${nested_failed_cmd}"
            emit_nested_regression_hint
        elif extract_nested_regression_failure_from_log "${nested_regression_log}"; then
            local_failed_step="regression_${nested_failed_step}"
            local_failed_cmd="${nested_failed_cmd}"
            emit_nested_regression_hint
        fi
        if ! print_summary "fail" "${local_failed_step}" "${local_failed_cmd}"; then
            exit 1
        fi
        exit 1
    fi
fi

if [[ "${nested_regression_summary_override_applied}" == "1" ]]; then
    unset A24_REGRESSION_SUMMARY_OUT
fi

if ! print_summary "pass" "none" "none"; then
    exit 1
fi

echo "PASS: a24 full regression (clean rebuild + step-trace runtime/static contract + self-tests)"
