#!/usr/bin/env bash
set -euo pipefail

interval_sec="${1:-60}"
output_path="${2:-/tmp/team_control_tower_snapshot.md}"

if ! [[ "${interval_sec}" =~ ^[0-9]+$ ]]; then
  echo "ERROR: interval_sec must be an integer" >&2
  exit 2
fi

mkdir -p "$(dirname "${output_path}")"

while true; do
  tmp_path="$(mktemp /tmp/team_control_tower_XXXXXX.md)"
  python3 tools/team_timer/team_control_tower.py --write "${tmp_path}" > /dev/null
  mv "${tmp_path}" "${output_path}"
  sleep "${interval_sec}"
done
