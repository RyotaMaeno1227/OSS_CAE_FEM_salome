#!/usr/bin/env python3
"""Update descriptor-e2e Run ID references across docs."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-id", required=True, help="GitHub Actions run ID (numeric).")
    parser.add_argument(
        "--variant",
        choices=["chrono-c", "chrono-main"],
        default="chrono-c",
        help="Target repo variant to update (default: %(default)s).",
    )
    parser.add_argument(
        "--log-path",
        default=None,
        help="Descriptor log markdown path (overrides variant defaults).",
    )
    parser.add_argument(
        "--plan-path",
        default=None,
        help="Migration plan markdown path (overrides variant defaults).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show the replacements without modifying files.",
    )
    return parser.parse_args()


def updated_log_text(path: Path, run_id: str) -> str:
    text = path.read_text(encoding="utf-8")
    pattern = re.compile(r"(最新 Run ID:\s*\*\*)([0-9_]+|\w+)(\*\*)")
    if "最新 Run ID:" in text:
        return pattern.sub(rf"最新 Run ID: **{run_id}**", text, count=1)
    # Fallback: インラインマーカーが無い場合は見出し直後に追記する
    return re.sub(r"(^# .+\n)", rf"\1\n最新 Run ID: **{run_id}**\n", text, count=1)


def updated_plan_text(path: Path, run_id: str) -> str:
    text = path.read_text(encoding="utf-8")
    if "Run ID:" not in text:
        raise RuntimeError(f"'Run ID:' marker missing in {path}")
    return re.sub(r"Run ID:\s*[0-9_a-zA-Z]+", f"Run ID: {run_id}", text, count=1)


def main() -> int:
    args = parse_args()
    if not args.run_id.isdigit():
        raise SystemExit("Run ID must be numeric")
    if args.variant == "chrono-main":
        default_log = Path("docs/logs/kkt_descriptor_poc_e2e_chrono_main.md")
        default_plan = Path("docs/coupled_island_migration_plan.md")
    else:
        default_log = Path("docs/logs/kkt_descriptor_poc_e2e.md")
        default_plan = Path("docs/coupled_island_migration_plan.md")
    log_path = Path(args.log_path) if args.log_path else default_log
    plan_path = Path(args.plan_path) if args.plan_path else default_plan
    new_log = updated_log_text(log_path, args.run_id)
    new_plan = updated_plan_text(plan_path, args.run_id)
    if args.dry_run:
        print(f"[dry-run] Would update {log_path} and {plan_path} to Run ID {args.run_id}")
    else:
        log_path.write_text(new_log, encoding="utf-8")
        plan_path.write_text(new_plan, encoding="utf-8")
        print(f"Updated descriptor run ID to {args.run_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
