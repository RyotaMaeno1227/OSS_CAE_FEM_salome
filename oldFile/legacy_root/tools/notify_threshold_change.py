#!/usr/bin/env python3
"""Send a webhook notification when benchmark thresholds change."""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict

try:
    import yaml  # type: ignore
except ImportError:
    yaml = None  # type: ignore


def load_config(path: Path) -> Dict[str, Any]:
    if not path.exists():
        raise FileNotFoundError(f"Threshold file not found: {path}")
    if path.suffix.lower() in {".yaml", ".yml"}:
        if yaml is None:
            raise RuntimeError("PyYAML is required to load YAML configuration files.")
        with path.open("r", encoding="utf-8") as handle:
            return yaml.safe_load(handle)
    if path.suffix.lower() == ".json":
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    raise RuntimeError(f"Unsupported threshold extension: {path.suffix}")


def build_message(payload: Dict[str, Any], commit: str | None) -> str:
    warnings = payload.get("warnings", {})
    failures = payload.get("failures", {})
    timestamp = datetime.now(timezone.utc).isoformat()
    lines = [
        f"Coupled benchmark thresholds updated ({timestamp})",
        "Warnings:",
    ]
    for key, value in warnings.items():
        lines.append(f"  - {key}: {value}")
    lines.append("Failures:")
    for key, value in failures.items():
        lines.append(f"  - {key}: {value}")
    if commit:
        lines.append(f"Commit: {commit}")
    return "\n".join(lines)


def send_webhook(url: str, message: str, dry_run: bool = False) -> None:
    if dry_run:
        print("[dry-run] Webhook payload:")
        print(message)
        return
    import urllib.request

    data = json.dumps({"text": message}).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req) as response:
        if response.status >= 300:
            raise RuntimeError(f"Webhook responded with status {response.status}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Notify Slack/Webhook about threshold changes.")
    parser.add_argument(
        "--config",
        default="config/coupled_benchmark_thresholds.yaml",
        help="Path to the threshold configuration file.",
    )
    parser.add_argument(
        "--webhook",
        help="Webhook URL (defaults to COUPLED_THRESHOLD_WEBHOOK env variable).",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print the payload instead of sending it.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    webhook = args.webhook or os.environ.get("COUPLED_THRESHOLD_WEBHOOK")
    if not webhook and not args.dry_run:
        print("Error: webhook URL not provided (use --webhook or set COUPLED_THRESHOLD_WEBHOOK).", file=sys.stderr)
        return 2

    payload = load_config(Path(args.config).expanduser())
    commit = os.environ.get("GITHUB_SHA")
    message = build_message(payload, commit[:7] if commit else None)
    if args.dry_run:
        send_webhook("", message, dry_run=True)
        return 0
    send_webhook(webhook, message)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
