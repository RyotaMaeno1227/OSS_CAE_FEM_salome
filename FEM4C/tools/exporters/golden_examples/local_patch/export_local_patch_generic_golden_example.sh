#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
resolve_workspace_root() {
    local start_dir="${1:-$(pwd)}"
    local candidate="${start_dir}"
    while [[ "${candidate}" != "/" ]]; do
        if [[ -d "${candidate}/scripts" && -d "${candidate}/docs" && -d "${candidate}/examples" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
        candidate="$(dirname "${candidate}")"
    done
    local git_root=""
    git_root="$(git -C "${start_dir}" rev-parse --show-toplevel 2>/dev/null || true)"
    if [[ -n "${git_root}" && -d "${git_root}/FEM4C/scripts" && -d "${git_root}/FEM4C/docs" && -d "${git_root}/FEM4C/examples" ]]; then
        printf '%s\n' "${git_root}/FEM4C"
        return 0
    fi
    if [[ -n "${git_root}" && -d "${git_root}/scripts" && -d "${git_root}/docs" && -d "${git_root}/examples" ]]; then
        printf '%s\n' "${git_root}"
        return 0
    fi
    echo "FAIL: unable to resolve workspace root" >&2
    return 1
}
ROOT_DIR="$(resolve_workspace_root "${SCRIPT_DIR}")"

if [[ $# -ne 2 ]]; then
    echo "Usage: bash scripts/export_local_patch_generic_golden_example.sh <penetration|normal_force> <outdir>" >&2
    exit 1
fi

MODE="$1"
OUT_DIR="$2"

case "${MODE}" in
    penetration)
        REQUEST_PATH="${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_penetration.json"
        ;;
    normal_force)
        REQUEST_PATH="${ROOT_DIR}/examples/local_patch_generic_contract_v1/request_force.json"
        ;;
    *)
        echo "FAIL: unsupported mode: ${MODE}" >&2
        echo "supported modes: penetration normal_force" >&2
        exit 1
        ;;
esac

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

cp "${REQUEST_PATH}" "${OUT_DIR}/request.json"

python3 "${ROOT_DIR}/scripts/run_local_patch_generic_contract_v1.py" \
    "${OUT_DIR}/request.json" \
    "${OUT_DIR}"

python3 "${ROOT_DIR}/scripts/plot_local_patch_generic_solver_v1.py" \
    "${OUT_DIR}"

cat >"${OUT_DIR}/README.md" <<EOF
# local_patch_generic_golden_example

- mode: \`${MODE}\`
- solver_mode: \`proxy_flat_plane_structured_grid_v1\`
- truth: proxy / MVP, not exact FEM surface-to-surface contact
- scope: local-only, before any macro bridge

## files

- \`request.json\`
- \`response.json\`
- \`pressure_field_grid.csv\`
- \`grid_definition.json\`
- \`local_patch_generic_summary.json\`
- \`local_patch_generic_summary.md\`
- \`pressure_field_heatmap.png\`
- \`pressure_field_centerline_t1.png\`
- \`reduced_summary.png\`
EOF

echo "PASS: local patch generic golden example export mode=${MODE} out_dir=${OUT_DIR}"
