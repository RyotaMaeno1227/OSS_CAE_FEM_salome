#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/c_stage_dryrun.sh [--log <path>] [--add-target <path>] [--coupled-freeze-file <path>]...

Run a C-team staging dry-run on a temporary Git index and emit a fixed report:
- dryrun_method
- dryrun_targets
- dryrun_changed_targets
- dryrun_cached_list
- forbidden_check
- coupled_freeze_check
- required_set_check
- safe_stage_targets
- safe_stage_command
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
COUPLED_FREEZE_FILE="${COUPLED_FREEZE_FILE:-scripts/c_coupled_freeze_forbidden_paths.txt}"
declare -a COUPLED_FREEZE_FALLBACK_PATTERNS=(
    "FEM4C/src/analysis/runner.c"
    "FEM4C/src/analysis/runner.h"
    "FEM4C/scripts/check_coupled_stub_contract.sh"
    "FEM4C/src/fem4c.c"
)
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
        --coupled-freeze-file)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --coupled-freeze-file requires a path" >&2
                exit 2
            fi
            COUPLED_FREEZE_FILE="$2"
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
declare -a CACHED_PATHS=()
while IFS=$'\t' read -r status path; do
    if [[ -z "$path" ]]; then
        continue
    fi
    seen_cached=0
    for existing in "${CACHED_PATHS[@]}"; do
        if [[ "$existing" == "$path" ]]; then
            seen_cached=1
            break
        fi
    done
    if [[ "$seen_cached" -eq 0 ]]; then
        CACHED_PATHS+=("$path")
    fi
done <<< "$CACHED_OUTPUT"

forbidden_check="pass"
if grep -E -q '[[:space:]](chrono-2d/|oldFile/|\.github/)' <<<"$CACHED_OUTPUT"; then
    forbidden_check="fail"
fi

declare -a COUPLED_FREEZE_PATTERNS=()
if [[ -f "$COUPLED_FREEZE_FILE" ]]; then
    while IFS= read -r line; do
        line="${line#"${line%%[![:space:]]*}"}"
        line="${line%"${line##*[![:space:]]}"}"
        if [[ -z "$line" || "${line:0:1}" == "#" ]]; then
            continue
        fi
        COUPLED_FREEZE_PATTERNS+=("$line")
    done < "$COUPLED_FREEZE_FILE"
fi
if [[ "${#COUPLED_FREEZE_PATTERNS[@]}" -eq 0 ]]; then
    COUPLED_FREEZE_PATTERNS=("${COUPLED_FREEZE_FALLBACK_PATTERNS[@]}")
fi

coupled_freeze_check="pass"
declare -a COUPLED_FREEZE_HITS=()
while IFS=$'\t' read -r status path; do
    if [[ -z "$path" ]]; then
        continue
    fi
    for pattern in "${COUPLED_FREEZE_PATTERNS[@]}"; do
        if [[ "$path" == $pattern ]]; then
            coupled_freeze_check="fail"
            hit_seen=0
            for existing in "${COUPLED_FREEZE_HITS[@]}"; do
                if [[ "$existing" == "$path" ]]; then
                    hit_seen=1
                    break
                fi
            done
            if [[ "$hit_seen" -eq 0 ]]; then
                COUPLED_FREEZE_HITS+=("$path")
            fi
            break
        fi
    done
done <<< "$CACHED_OUTPUT"

required_set_check="pass"
for p in "${CHANGED_TARGETS[@]}"; do
    if ! grep -E -q "[[:space:]]${p}$" <<<"$CACHED_OUTPUT"; then
        required_set_check="fail"
    fi
done

dryrun_result="pass"
if [[ "$forbidden_check" != "pass" || "$coupled_freeze_check" != "pass" || "$required_set_check" != "pass" ]]; then
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
    echo "coupled_freeze_file=${COUPLED_FREEZE_FILE}"
    if [[ "${#COUPLED_FREEZE_HITS[@]}" -gt 0 ]]; then
        echo "coupled_freeze_hits=${COUPLED_FREEZE_HITS[*]}"
    else
        echo "coupled_freeze_hits=-"
    fi
    echo "coupled_freeze_check=${coupled_freeze_check}"
    echo "required_set_check=${required_set_check}"
    if [[ "${#CACHED_PATHS[@]}" -gt 0 ]]; then
        echo "safe_stage_targets=${CACHED_PATHS[*]}"
        safe_stage_command="git add"
        for path in "${CACHED_PATHS[@]}"; do
            safe_stage_command+=" $(printf '%q' "$path")"
        done
        echo "safe_stage_command=${safe_stage_command}"
    else
        echo "safe_stage_targets=-"
        echo "safe_stage_command=git add"
    fi
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
