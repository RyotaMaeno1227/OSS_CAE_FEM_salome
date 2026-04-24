#!/usr/bin/env bash
set -euo pipefail

find_workspace_root() {
    local current
    current="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    while [[ "${current}" != "/" ]]; do
        if [[ -d "${current}/scripts" && -d "${current}/tools" ]]; then
            printf '%s\n' "${current}"
            return 0
        fi
        current="$(dirname "${current}")"
    done
    echo "FAIL: could not determine workspace root" >&2
    return 1
}

WORKSPACE_ROOT="$(find_workspace_root)"
exec "${WORKSPACE_ROOT}/tools/checkers/bridges/manifest_bridge/check_mbd_2link_rigid_result_surface.sh" "$@"
