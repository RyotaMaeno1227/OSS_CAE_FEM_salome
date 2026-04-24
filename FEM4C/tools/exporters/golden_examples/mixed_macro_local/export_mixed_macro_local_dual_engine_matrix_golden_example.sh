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

if [[ $# -ne 1 ]]; then
    echo "Usage: bash scripts/export_mixed_macro_local_dual_engine_matrix_golden_example.sh <outdir>" >&2
    exit 2
fi

OUT_DIR="$1"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

env FEM4C_MIXED_MACRO_LOCAL_DUAL_ENGINE_MATRIX_V1_OUTDIR="${OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_dual_engine_matrix_v1.sh"

for path in \
    "${OUT_DIR}/dual_engine_matrix_manifest.json" \
    "${OUT_DIR}/dual_engine_matrix_target_summary.json" \
    "${OUT_DIR}/dual_engine_matrix_target_summary.md"; do
    [[ -e "${path}" ]] || { echo "FAIL: missing dual-engine golden artifact ${path}" >&2; exit 1; }
done

SCENARIOS=(
    "A_all_valid_dual_engine"
    "B_partial_invalid_mbd_only"
    "C_partial_invalid_fem_later_only"
    "D_all_invalid_both"
)

for scenario in "${SCENARIOS[@]}"; do
    for path in \
        "${OUT_DIR}/${scenario}/oneway_run/reduced_feedback/mixed_feedback_reduced_summary.json" \
        "${OUT_DIR}/${scenario}/split_replay_run/mixed_split_replay_summary.json"; do
        [[ -e "${path}" ]] || { echo "FAIL: missing dual-engine scenario artifact ${path}" >&2; exit 1; }
    done
done

python3 - "${OUT_DIR}" <<'PY'
import json
import pathlib
import sys

out_dir = pathlib.Path(sys.argv[1]).resolve()
target_summary = json.loads((out_dir / "dual_engine_matrix_target_summary.json").read_text(encoding="utf-8"))

scenario_rows = []
for scenario in target_summary["scenarios"]:
    mbd = scenario["mbd_target"]
    fem = scenario["fem_later_step_target"]
    scenario_rows.append(
        "\n".join(
            [
                f"### {scenario['scenario']}",
                "",
                f"- MBD target fallback_applied: `{str(mbd['fallback_applied']).lower()}`",
                f"- MBD target replay_changed: `{str(mbd['replay_changed']).lower()}`",
                f"- FEM target fallback_applied: `{str(fem['fallback_applied']).lower()}`",
                f"- FEM target replay_changed: `{str(fem['replay_changed']).lower()}`",
            ]
        )
    )

readme_lines = [
    "# mixed_macro_local_dual_engine_matrix_golden_example",
    "",
    "この outdir は dual-engine matrix canonical family を reviewer-facing hardening 用 golden example として固定したものです。",
    "",
    "## global count と target-key count の違い",
    "",
    "- global replay / fallback count は non-target row を含みうる",
    "- canonical target reduced key の outcome は `dual_engine_matrix_target_summary.json` / `.md` を正本として読む",
    "",
    "## reduced key semantics",
    "",
    "- MBD: `(step, pair_id)`",
    "- FEM: `(load_step, pair_id)`",
    "",
    "## fallback rule",
    "",
    "- partial-invalid / all-invalid は current all-valid rule で fallback に落ちる",
    "- invalid or non-ok reduced row では `gamma_n_used = 1.0` / `k_pen_used = k_pen_base` を使う",
    "",
    "## truth",
    "",
    "- static-only な FEM source を mixed route に流している",
    "- first-class feedback は gamma_n のみ",
    "- local solver は proxy / MVP",
    "- joint mixed replay は未実装",
    "- mixed solve は未実装",
    "- live co-sim は未実装",
    "- true lagged mixed time-step co-sim ではない",
    "- same-time mixed co-sim ではない",
    "",
    "## scenario target summary",
    "",
    *scenario_rows,
    "",
]

(out_dir / "README.md").write_text("\n".join(readme_lines), encoding="utf-8")
PY

echo "PASS: export_mixed_macro_local_dual_engine_matrix_golden_example"
echo "out_dir=${OUT_DIR}"
