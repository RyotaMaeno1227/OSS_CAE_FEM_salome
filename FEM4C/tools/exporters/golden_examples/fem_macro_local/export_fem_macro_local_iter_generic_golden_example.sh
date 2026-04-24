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
EXAMPLE_DECK="${ROOT_DIR}/examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat"

if [[ $# -ne 1 ]]; then
    echo "Usage: bash scripts/export_fem_macro_local_iter_generic_golden_example.sh <outdir>" >&2
    exit 1
fi

OUT_DIR="$1"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

bash "${ROOT_DIR}/scripts/run_fem_macro_local_iter_generic_v1.sh" "${EXAMPLE_DECK}" "${OUT_DIR}"
python3 "${ROOT_DIR}/scripts/plot_fem_macro_local_iter_generic_v1.py" "${OUT_DIR}"

python3 - "${OUT_DIR}" "${EXAMPLE_DECK}" <<'PY'
import csv
import json
import sys
from pathlib import Path


out_dir = Path(sys.argv[1]).resolve()
deck_path = Path(sys.argv[2]).resolve()
summary = json.loads((out_dir / "iter_summary.json").read_text(encoding="utf-8"))
plot_summary = json.loads((out_dir / "plot_summary.json").read_text(encoding="utf-8"))

gamma_rows = list(csv.DictReader((out_dir / "gamma_evolution.csv").open(newline="", encoding="utf-8")))
gamma_table_lines = [
    "| iter_index | feedback_row_count | raw_gamma_n_mean | used_gamma_n_mean | raw_gamma_n_min | raw_gamma_n_max | used_gamma_n_min | used_gamma_n_max |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
]
for row in gamma_rows:
    gamma_table_lines.append(
        "| {iter_index} | {feedback_row_count} | {raw_gamma_n_mean} | {used_gamma_n_mean} | {raw_gamma_n_min} | {raw_gamma_n_max} | {used_gamma_n_min} | {used_gamma_n_max} |".format(
            **row
        )
    )

rep = plot_summary["representative_row"]
readme_lines = [
    "# fem_macro_local_iter_generic_golden_example",
    "",
    "- canonical_input: `{}`".format(deck_path),
    "- route: `fem_macro_local_iter_generic_v1`",
    "- pipeline_truth: FEM static macro+local iterative coupling MVP",
    "- static-only: `true`",
    "- true lagged time-step co-sim: `false`",
    "- same-time co-sim: `false`",
    "- first-class feedback: `gamma_n only`",
    "- local solver truth: `proxy / MVP`",
    "- exact full-field coupling: `false`",
    "- converged: `{}`".format(str(summary["converged"]).lower()),
    "- actual_iter_count: `{}`".format(summary["actual_iter_count"]),
    "",
    "## review truth",
    "",
    "- `converged=true` は `local_feedback_reduced.csv` の used/relaxed feedback 上の収束です。",
    "- raw local `gamma_n` fixed-point convergence を主張しません。",
    "- raw gamma と used gamma の両方は `gamma_evolution.csv` と `iteration_history.png` で追えます。",
    "- `representative_local_pressure_field.png` は proxy / MVP local pressure field です。exact local solver の pressure field ではありません。",
    "",
    "## representative row",
    "",
    "- load_step: `{}`".format(rep["load_step"]),
    "- pair_id: `{}`".format(rep["pair_id"]),
    "- slave_node_id: `{}`".format(rep["slave_node_id"]),
    "- master_segment_id: `{}`".format(rep["master_segment_id"]),
    "- valid_flag: `{}`".format(rep["valid_flag"]),
    "- status: `{}`".format(rep["status"]),
    "",
    "## files",
    "",
    "- `iteration_history.csv`",
    "- `iter_summary.json`",
    "- `iter_summary.md`",
    "- `iteration_history.png`",
    "- `baseline_vs_final_replay.png`",
    "- `representative_local_pressure_field.png`",
    "- `gamma_evolution.csv`",
    "- `plot_summary.json`",
    "- `README.md`",
    "",
    "## raw gamma and used gamma table",
    "",
    *gamma_table_lines,
    "",
    "## limitations",
    "",
    "- static-only",
    "- true lagged time-step co-sim ではない",
    "- same-time co-sim ではない",
    "- first-class feedback は gamma_n のみ",
    "- local solver は proxy / MVP",
    "- exact full-field coupling ではない",
    "- friction 未実装",
    "- damping 未実装",
    "- EHL 未着手",
    "- monolithic 未着手",
]
(out_dir / "README.md").write_text("\n".join(readme_lines) + "\n", encoding="utf-8")
PY

echo "PASS: fem_macro_local_iter_generic_golden_example out_dir=${OUT_DIR}"
