#!/usr/bin/env bash
set -euo pipefail

RUN_ID="${1:-local-chrono2d-$(date +%Y%m%d)-01}"

cd "$(dirname "$0")/.."

make test

CSV_PATH="artifacts/kkt_descriptor_actions_local.csv"
if [[ -f "$CSV_PATH" ]]; then
  echo "[chrono-2d] Run ID: $RUN_ID" | tee artifacts/run_id.log
  echo "[chrono-2d] CSV: $CSV_PATH (head)"
  head -n 5 "$CSV_PATH"
else
  echo "CSV not found: $CSV_PATH" >&2
  exit 1
fi

echo "Share using template: Run=$RUN_ID Artifact=$CSV_PATH"
