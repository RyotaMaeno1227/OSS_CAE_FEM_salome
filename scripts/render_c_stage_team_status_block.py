#!/usr/bin/env python3
"""Render team_status markdown snippet from c_stage_dryrun report."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys


ORDERED_KEYS = (
    "dryrun_method",
    "dryrun_targets",
    "dryrun_changed_targets",
    "forbidden_check",
    "coupled_freeze_file",
    "coupled_freeze_hits",
    "coupled_freeze_check",
    "required_set_check",
    "safe_stage_targets",
    "safe_stage_command",
    "dryrun_result",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render markdown block for team_status from c_stage_dryrun report"
    )
    parser.add_argument("report_path", help="Path to c_stage_dryrun log/report")
    parser.add_argument(
        "--header",
        default="dry-run 生出力（strict-safe 記録）",
        help="Markdown header label",
    )
    parser.add_argument(
        "--output",
        default="",
        help="Optional output markdown path for copy/paste reuse",
    )
    return parser.parse_args()


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read report: {exc}") from exc


def collect_values(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if key in ORDERED_KEYS and key not in values:
            values[key] = value.strip()
    return values


def main() -> int:
    args = parse_args()
    text = read_text(Path(args.report_path))
    values = collect_values(text)
    missing = [key for key in ORDERED_KEYS if key not in values]
    if missing:
        print("RENDER_C_STAGE_TEAM_STATUS_BLOCK")
        print(f"report_path={args.report_path}")
        print("verdict=FAIL")
        print(f"missing_keys={','.join(missing)}")
        return 1

    lines: list[str] = []
    lines.append(f"- {args.header}:")
    for key in ORDERED_KEYS:
        lines.append(f"  - `{key}={values[key]}`")

    output_text = "\n".join(lines)
    print(output_text)

    if args.output:
        output_path = Path(args.output)
        try:
            output_path.write_text(output_text + "\n", encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: failed to write output file: {exc}", file=sys.stderr)
            return 1
        print(f"render_output_path={output_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
