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
    echo "Usage: bash scripts/export_mixed_macro_local_fallback_golden_example.sh <outdir>" >&2
    exit 2
fi

OUT_DIR="$1"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

env FEM4C_MIXED_MACRO_LOCAL_FEEDBACK_FALLBACK_GENERIC_V1_OUTDIR="${OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_feedback_fallback_generic_v1.sh"

SCENARIOS=(
    "A_partial_invalid_mbd"
    "B_partial_invalid_fem"
    "C_all_invalid_mbd"
    "D_all_invalid_fem"
)

for scenario in "${SCENARIOS[@]}"; do
    for path in \
        "${OUT_DIR}/${scenario}/oneway_run/reduced_feedback/mixed_feedback_reduced_manifest.json" \
        "${OUT_DIR}/${scenario}/split_replay_run/split_replay_handoff_manifest.json" \
        "${OUT_DIR}/${scenario}/split_replay_run/mixed_split_replay_summary.json" \
        "${OUT_DIR}/${scenario}/split_replay_run/mbd_replay/replay_generic.out.contact_generic_replay_use.csv" \
        "${OUT_DIR}/${scenario}/split_replay_run/fem_replay/replay_generic.out.fem_contact_generic_replay_use.csv"; do
        [[ -e "${path}" ]] || { echo "FAIL: missing fallback golden artifact ${path}" >&2; exit 1; }
    done
done

python3 - "${OUT_DIR}" <<'PY'
import json
import pathlib
import textwrap
import sys

out_dir = pathlib.Path(sys.argv[1]).resolve()
scenarios = [
    "A_partial_invalid_mbd",
    "B_partial_invalid_fem",
    "C_all_invalid_mbd",
    "D_all_invalid_fem",
]

lines = [
    "# mixed_macro_local_fallback_golden_example",
    "",
    "この outdir は CT_239a の invalid/status fallback hardening 4 scenario を canonical mixed fallback golden example として固定したものです。",
    "",
    "## reduced key semantics",
    "",
    "- MBD: `(step, pair_id)`",
    "- FEM: `(load_step, pair_id)`",
    "",
    "## aggregate rule",
    "",
    "- gamma_n: arithmetic mean",
    "- delta_g_eff_m: arithmetic mean",
    "- fn_ref_n: sum",
    "- p_max_pa: max",
    "- valid_flag: all-valid",
    "- status: current normalized rule",
    "",
    "## fallback semantics",
    "",
    "- partial-invalid は reduced row invalid -> replay fallback",
    "- all-invalid は replay fallback",
    "- replay fallback では current route で `gamma_n_used = 1.0` と `k_pen_used = k_pen_base` を確認する",
    "",
    "## truth",
    "",
    "- first-class feedback は gamma_n のみ",
    "- local solver は proxy / MVP",
    "- FEM source は static-only",
    "- joint mixed replay は未実装",
    "- mixed solve は未実装",
    "- live co-sim は未実装",
    "- true lagged mixed time-step co-sim ではない",
    "- same-time mixed co-sim ではない",
    "",
    "## scenario summary",
    "",
]

for scenario in scenarios:
    reduced_manifest = json.loads((out_dir / scenario / "oneway_run" / "reduced_feedback" / "mixed_feedback_reduced_manifest.json").read_text(encoding="utf-8"))
    split_summary = json.loads((out_dir / scenario / "split_replay_run" / "mixed_split_replay_summary.json").read_text(encoding="utf-8"))
    lines.append(f"### {scenario}")
    lines.append("")
    for row in reduced_manifest["rows"]:
        lines.append(
            f"- {row['source_engine']}: source_row_count=`{row.get('source_row_count')}` "
            f"valid_source_row_count=`{row.get('valid_source_row_count')}` "
            f"invalid_source_row_count=`{row.get('invalid_source_row_count')}` "
            f"valid_flag=`{row['exported_feedback']['valid_flag']}` "
            f"status=`{row['exported_feedback']['status']}`"
        )
    lines.append(
        f"- split summary: mbd_fallback_rows=`{split_summary.get('mbd_fallback_rows')}`, "
        f"fem_fallback_rows=`{split_summary.get('fem_fallback_rows')}`, "
        f"mbd_replay_changed_rows=`{split_summary.get('mbd_replay_changed_rows')}`, "
        f"fem_replay_changed_rows=`{split_summary.get('fem_replay_changed_rows')}`"
    )
    lines.append("")

(out_dir / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

echo "PASS: export_mixed_macro_local_fallback_golden_example"
echo "out_dir=${OUT_DIR}"
