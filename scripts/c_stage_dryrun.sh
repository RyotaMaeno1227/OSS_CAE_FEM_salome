#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/c_stage_dryrun.sh [--log <path>] [--add-target <path>]...

Run a C-team staging dry-run on a temporary Git index and emit a fixed report:
- dryrun_method
- dryrun_targets
- dryrun_changed_targets
- dryrun_cached_list
- forbidden_check
- required_set_check
- dryrun_result

Default targets:
  FEM4C/src/io/input.c
  FEM4C/src/solver/cg_solver.c
  FEM4C/src/elements/t3/t3_element.c
  docs/fem4c_dirty_diff_triage_2026-02-06.md
  docs/fem4c_team_next_queue.md
  docs/team_status.md
  docs/session_continuity_log.md
EOF
}

LOG_PATH=""
declare -a EXTRA_TARGETS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --log requires a path" >&2
                exit 2
            fi
            LOG_PATH="$2"
            shift 2
            ;;
        --add-target)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --add-target requires a path" >&2
                exit 2
            fi
            EXTRA_TARGETS+=("$2")
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

ROOT_DIR="$(git rev-parse --show-toplevel)"
cd "$ROOT_DIR"

declare -a TARGETS=(
    "FEM4C/src/io/input.c"
    "FEM4C/src/solver/cg_solver.c"
    "FEM4C/src/elements/t3/t3_element.c"
    "docs/fem4c_dirty_diff_triage_2026-02-06.md"
    "docs/fem4c_team_next_queue.md"
    "docs/team_status.md"
    "docs/session_continuity_log.md"
)

for p in "${EXTRA_TARGETS[@]}"; do
    TARGETS+=("$p")
done

# De-duplicate targets while preserving order.
declare -A SEEN=()
declare -a UNIQUE_TARGETS=()
for p in "${TARGETS[@]}"; do
    if [[ -z "${SEEN[$p]:-}" ]]; then
        SEEN[$p]=1
        UNIQUE_TARGETS+=("$p")
    fi
done
TARGETS=("${UNIQUE_TARGETS[@]}")

TMP_INDEX="$(mktemp /tmp/c_stage_dryrun.index.XXXXXX)"
trap 'rm -f "$TMP_INDEX"' EXIT

# Start from HEAD for deterministic dry-run output.
GIT_INDEX_FILE="$TMP_INDEX" git read-tree HEAD

declare -a CHANGED_TARGETS=()
for p in "${TARGETS[@]}"; do
    if [[ ! -e "$p" ]]; then
        continue
    fi
    if [[ -n "$(git status --short -- "$p")" ]]; then
        CHANGED_TARGETS+=("$p")
    fi
    GIT_INDEX_FILE="$TMP_INDEX" git add "$p"
done

CACHED_OUTPUT="$(GIT_INDEX_FILE="$TMP_INDEX" git diff --cached --name-status)"

forbidden_check="pass"
if grep -E -q '[[:space:]](chrono-2d/|\.github/)' <<<"$CACHED_OUTPUT"; then
    forbidden_check="fail"
fi

required_set_check="pass"
for p in "${CHANGED_TARGETS[@]}"; do
    if ! grep -E -q "[[:space:]]${p}$" <<<"$CACHED_OUTPUT"; then
        required_set_check="fail"
    fi
done

dryrun_result="pass"
if [[ "$forbidden_check" != "pass" || "$required_set_check" != "pass" ]]; then
    dryrun_result="fail"
fi

report() {
    echo "dryrun_method=GIT_INDEX_FILE"
    echo "dryrun_targets=${TARGETS[*]}"
    echo "dryrun_changed_targets=${CHANGED_TARGETS[*]}"
    echo "dryrun_cached_list<<EOF"
    if [[ -n "$CACHED_OUTPUT" ]]; then
        echo "$CACHED_OUTPUT"
    fi
    echo "EOF"
    echo "forbidden_check=${forbidden_check}"
    echo "required_set_check=${required_set_check}"
    echo "dryrun_result=${dryrun_result}"
}

if [[ -n "$LOG_PATH" ]]; then
    mkdir -p "$(dirname "$LOG_PATH")"
    report | tee "$LOG_PATH"
else
    report
fi

if [[ "$dryrun_result" != "pass" ]]; then
    exit 1
fi
