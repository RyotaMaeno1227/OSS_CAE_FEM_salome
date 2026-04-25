#!/usr/bin/env bash
set -euo pipefail

coupled_compare_reason_code_default_cache_root() {
    local suffix="$1"
    printf '%s/%s\n' "${TMPDIR:-/tmp}" "${suffix}"
}

coupled_compare_reason_code_cache_dir() {
    local repo_root="$1"
    local cache_root="$2"
    shift 2
    local -a extra_inputs=("$@")
    local fingerprint
    local -a cache_inputs

    mapfile -d '' cache_inputs < <(
        {
            printf '%s\0' \
                "${repo_root}/FEM4C/Makefile" \
                "${repo_root}/docs/team_runbook.md" \
                "${repo_root}/docs/fem4c_team_next_queue.md"
            find "${repo_root}/scripts" -maxdepth 1 -type f -name '*coupled_compare_reason_code*' -print0
            find "${repo_root}/FEM4C/scripts" -maxdepth 1 -type f -name '*coupled_compare_reason_code*' -print0
            if [[ "${#extra_inputs[@]}" -gt 0 ]]; then
                printf '%s\0' "${extra_inputs[@]}"
            fi
        } | sort -z
    )

    fingerprint="$(sha256sum "${cache_inputs[@]}" | sha256sum | awk '{print $1}')"
    printf '%s/%s\n' "${cache_root}" "${fingerprint}"
}

coupled_compare_reason_code_bundle_log_path() {
    local cache_dir="$1"
    local target_name="$2"

    mkdir -p "${cache_dir}"
    printf '%s/%s.log\n' "${cache_dir}" "${target_name}"
}

coupled_compare_reason_code_ensure_bundle_log() {
    local repo_root="$1"
    local target_name="$2"
    local cached_log="$3"
    local cached_done
    local lock_path
    local lock_fd
    local tmp_log

    cached_done="${cached_log}.done"
    lock_path="${cached_log}.lock"
    exec {lock_fd}>"${lock_path}"
    flock "${lock_fd}"

    if [[ -f "${cached_log}" && -f "${cached_done}" ]]; then
        flock -u "${lock_fd}"
        exec {lock_fd}>&-
        return 0
    fi

    rm -f "${cached_log}" "${cached_done}"
    tmp_log="$(mktemp "${cached_log}.tmp.XXXXXX")"
    if make -C "${repo_root}/FEM4C" "${target_name}" >"${tmp_log}" 2>&1; then
        mv "${tmp_log}" "${cached_log}"
        : >"${cached_done}"
    else
        rm -f "${tmp_log}" "${cached_log}" "${cached_done}"
        flock -u "${lock_fd}"
        exec {lock_fd}>&-
        return 1
    fi

    flock -u "${lock_fd}"
    exec {lock_fd}>&-
}
