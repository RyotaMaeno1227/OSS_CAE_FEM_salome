#!/usr/bin/env bash
set -euo pipefail

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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(resolve_workspace_root "${SCRIPT_DIR}")"
OUT_DIR="${FEM4C_CONTACT_PATCH_RECEIVER_OUTDIR:-${TMPDIR:-/tmp}/fem4c_contact_patch_receiver_smoke}"

bash "${ROOT_DIR}/scripts/run_contact_patch_receiver_smoke.sh"

python3 - "${OUT_DIR}" <<'PY'
import json
import math
import pathlib
import sys

out_dir = pathlib.Path(sys.argv[1])
summary_paths = [
    out_dir / "patch_pair0_body0_step0180_receiver_summary.json",
    out_dir / "patch_pair0_body1_step0180_receiver_summary.json",
]

for summary_path in summary_paths:
    if not summary_path.exists():
        raise SystemExit(f"FAIL: missing summary {summary_path}")
    data = json.loads(summary_path.read_text())

    for key in (
        "fn_macro",
        "applied_force_world",
        "applied_force_local",
        "load_node_ids",
        "displacement_max_norm",
        "reaction_resultant_local",
        "deformed_output_path",
    ):
        if key not in data:
            raise SystemExit(f"FAIL: missing key {key} in {summary_path}")

    max_disp = float(data["displacement_max_norm"])
    if not math.isfinite(max_disp) or max_disp <= 0.0:
        raise SystemExit(f"FAIL: invalid displacement_max_norm in {summary_path}: {max_disp!r}")

    reaction = data["reaction_resultant_local"]
    if len(reaction) != 3:
        raise SystemExit(f"FAIL: reaction_resultant_local length mismatch in {summary_path}")
    if any(not math.isfinite(float(value)) for value in reaction):
        raise SystemExit(f"FAIL: non-finite reaction in {summary_path}")

    applied_force_world = [float(value) for value in data["applied_force_world"]]
    if len(applied_force_world) != 2:
        raise SystemExit(f"FAIL: applied_force_world length mismatch in {summary_path}")
    if any(not math.isfinite(value) for value in applied_force_world):
        raise SystemExit(f"FAIL: non-finite applied_force_world in {summary_path}")

    fn_macro = float(data["fn_macro"])
    applied_force_norm = math.hypot(applied_force_world[0], applied_force_world[1])
    if not math.isfinite(fn_macro):
        raise SystemExit(f"FAIL: invalid fn_macro in {summary_path}: {fn_macro!r}")
    if abs(applied_force_norm - fn_macro) > max(1.0e-9, 1.0e-9 * max(1.0, abs(fn_macro))):
        raise SystemExit(
            f"FAIL: applied force norm mismatch in {summary_path}: "
            f"|F|={applied_force_norm!r} fn_macro={fn_macro!r}"
        )

    load_node_ids = data["load_node_ids"]
    if len(load_node_ids) != 4:
        raise SystemExit(f"FAIL: load_node_ids length mismatch in {summary_path}")

    deformed_path = pathlib.Path(data["deformed_output_path"])
    if not deformed_path.exists():
        raise SystemExit(f"FAIL: missing deformed output {deformed_path}")

    content = deformed_path.read_text()
    if "node_id,x_local,y_local" not in content:
        raise SystemExit(f"FAIL: malformed deformed output {deformed_path}")

    print(
        f"PASS summary={summary_path.name} "
        f"max_disp={max_disp:.6e} "
        f"|F|={applied_force_norm:.6e} "
        f"reaction=({float(reaction[0]):.6e},{float(reaction[1]):.6e},{float(reaction[2]):.6e})"
    )
PY

INVALID_PROBE_SRC="${OUT_DIR}/contact_patch_receiver_invalid_probe.c"
INVALID_PROBE_BIN="${OUT_DIR}/contact_patch_receiver_invalid_probe"
INVALID_PROBE_LOG="${OUT_DIR}/contact_patch_receiver_invalid_probe.log"

cat >"${INVALID_PROBE_SRC}" <<'EOF'
#include "src/coupled/contact_patch2d.h"
#include "src/coupled/contact_patch_load2d.h"
#include "src/common/error.h"

#include <stdio.h>

int main(void)
{
    contact_patch2d_t patch;
    contact_patch_load2d_result_t result;
    fem_error_t err;

    contact_patch2d_zero(&patch);
    contact_patch_load2d_result_zero(&result);

    patch.body_id = 0;
    patch.pair_id = 0;
    patch.step = 180;
    patch.time = 1.8e-1;
    patch.contact_point_world[0] = 2.0e-2;
    patch.contact_point_world[1] = 5.0e-3;
    patch.normal_world[0] = 2.0;
    patch.normal_world[1] = 0.0;
    patch.tangent_world[0] = 0.0;
    patch.tangent_world[1] = 1.0;
    patch.radius_body = 5.0e-2;
    patch.gap_macro = 0.0;
    patch.fn_macro = 1.0e3;
    patch.thickness = 1.0;
    patch.patch_size = 2.0e-2;

    snprintf(patch.mesh_path,
             sizeof(patch.mesh_path),
             "%s",
             "/home/rmaen/highperformanceFEM/FEM4C/examples/contact_patch/patch_small_q4.dat");
    snprintf(patch.output_path,
             sizeof(patch.output_path),
             "%s",
             "/tmp/fem4c_contact_patch_receiver_smoke/invalid_receiver_summary.json");

    err = contact_patch_load2d_run_fixture_static(&patch, &result);
    if (err == FEM_SUCCESS) {
        fprintf(stderr, "unexpected success for invalid frame probe\n");
        return 1;
    }

    fprintf(stderr, "expected failure err=%d message=%s\n", err, error_get_message());
    return 0;
}
EOF

gcc -Wall -Wextra -O3 -std=c99 -mcmodel=large \
    -I. -Isrc/common -Isrc/elements -Isrc/solver -Isrc/io -Isrc/mesh -Isrc/material -Isrc/analysis -Isrc/mbd -Isrc/coupled \
    "${INVALID_PROBE_SRC}" \
    $(find build -name '*.o' ! -path 'build/fem4c.o' | sort) \
    -lm -o "${INVALID_PROBE_BIN}"

"${INVALID_PROBE_BIN}" >"${INVALID_PROBE_LOG}" 2>&1
grep -q "normal_world must be unit length" "${INVALID_PROBE_LOG}"

echo "PASS: contact patch receiver smoke check"
