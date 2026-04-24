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
    echo "Usage: bash scripts/export_mixed_macro_local_golden_example.sh <outdir>" >&2
    exit 2
fi

OUT_DIR="$1"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

env FEM4C_MIXED_MACRO_LOCAL_FEEDBACK_AGGREGATION_GENERIC_V1_OUTDIR="${OUT_DIR}" \
    bash "${ROOT_DIR}/scripts/check_mixed_macro_local_feedback_aggregation_generic_v1.sh"

for path in \
    "${OUT_DIR}/multirow_mixed_manifest.json" \
    "${OUT_DIR}/oneway_run/reduced_feedback/mixed_feedback_reduced_manifest.json" \
    "${OUT_DIR}/oneway_run/reduced_feedback/mixed_feedback_reduced_summary.json" \
    "${OUT_DIR}/oneway_run/reduced_feedback/mbd_local_feedback_reduced.csv" \
    "${OUT_DIR}/oneway_run/reduced_feedback/fem_local_feedback_reduced.csv" \
    "${OUT_DIR}/split_replay_run/split_replay_handoff_manifest.json" \
    "${OUT_DIR}/split_replay_run/mixed_split_replay_summary.json" \
    "${OUT_DIR}/split_replay_run/mixed_split_replay_summary.md" \
    "${OUT_DIR}/split_replay_run/mbd_replay/replay_generic.out.contact_generic_replay_use.csv" \
    "${OUT_DIR}/split_replay_run/fem_replay/replay_generic.out.fem_contact_generic_replay_use.csv"; do
    [[ -e "${path}" ]] || { echo "FAIL: missing golden example artifact ${path}" >&2; exit 1; }
done

python3 - "${OUT_DIR}" <<'PY'
import json
import pathlib
import textwrap
import sys

out_dir = pathlib.Path(sys.argv[1]).resolve()
reduced_summary = json.loads((out_dir / "oneway_run" / "reduced_feedback" / "mixed_feedback_reduced_summary.json").read_text(encoding="utf-8"))
split_summary = json.loads((out_dir / "split_replay_run" / "mixed_split_replay_summary.json").read_text(encoding="utf-8"))

readme = textwrap.dedent(
    f"""\
    # mixed_macro_local_golden_example

    この outdir は CT_238e multi-row aggregation case を canonical mixed golden example として固定したものです。

    ## reduced key semantics

    - MBD: `(step, pair_id)`
    - FEM: `(load_step, pair_id)`

    ## aggregate rule

    - gamma_n: arithmetic mean
    - delta_g_eff_m: arithmetic mean
    - fn_ref_n: sum
    - p_max_pa: max
    - valid_flag: all-valid
    - status: current normalized rule

    ## truth

    - first-class feedback は gamma_n のみ
    - local solver は proxy / MVP
    - FEM source は static-only
    - 今回できているのは mixed reduced feedback surface / split replay consume の hardening まで
    - joint mixed replay は未実装
    - mixed solve は未実装
    - live co-sim は未実装
    - true lagged mixed time-step co-sim ではない
    - same-time mixed co-sim ではない

    ## artifact summary

    - reduced total_feedback_rows: `{reduced_summary["total_feedback_rows"]}`
    - reduced mbd_feedback_rows: `{reduced_summary["mbd_feedback_rows"]}`
    - reduced fem_feedback_rows: `{reduced_summary["fem_feedback_rows"]}`
    - split mbd_replay_changed_rows: `{split_summary["mbd_replay_changed_rows"]}`
    - split fem_replay_changed_rows: `{split_summary["fem_replay_changed_rows"]}`
    """
)

(out_dir / "README.md").write_text(readme, encoding="utf-8")
PY

echo "PASS: export_mixed_macro_local_golden_example"
echo "out_dir=${OUT_DIR}"
