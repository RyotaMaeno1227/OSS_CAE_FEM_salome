#!/usr/bin/env python3
"""Validate latest.summary.json against the coupled endurance schema."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, Sequence

import plot_coupled_constraint_endurance as plot_helper


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate latest.summary.json and emit a Markdown report."
    )
    parser.add_argument(
        "--summary-json",
        default="data/endurance_archive/latest.summary.json",
        help="Path to latest.summary.json (default: %(default)s).",
    )
    parser.add_argument(
        "--output-markdown",
        default="data/endurance_archive/latest.summary.validation.md",
        help="Markdown report destination (default: %(default)s).",
    )
    parser.add_argument(
        "--output-json",
        help="Optional JSON file for structured validation results.",
    )
    parser.add_argument(
        "--fail-on-error",
        action="store_true",
        help="Exit with status 1 when validation fails.",
    )
    return parser.parse_args(argv)


def load_summary(path: Path) -> Dict[str, Any]:
    if not path.exists():
        raise FileNotFoundError(f"Summary JSON not found at {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def build_markdown(status: str, *, path: Path, message: str, summary: Dict[str, Any]) -> str:
    metrics = []
    for key in ("samples", "max_condition", "warn_ratio", "rank_ratio", "duration"):
        value = summary.get(key, "n/a")
        metrics.append(f"| {key} | {value} |")

    lines = [
        "## latest.summary.json schema validation",
        f"- File: `{path}`",
        f"- Status: **{status}**",
        f"- Message: {message}",
        "",
        "| Metric | Value |",
        "| --- | --- |",
        *metrics,
    ]
    return "\n".join(lines) + "\n"


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    summary_path = Path(args.summary_json).expanduser()
    summary = load_summary(summary_path)

    status = "PASS"
    message = "Schema validation succeeded."
    exit_code = 0
    try:
        plot_helper._validate_summary_schema(summary)  # type: ignore[attr-defined]
    except Exception as exc:  # pragma: no cover - defensive
        status = "FAIL"
        message = str(exc)
        if args.fail_on_error:
            exit_code = 1

    markdown = build_markdown(status, path=summary_path, message=message, summary=summary)
    output_md = Path(args.output_markdown).expanduser()
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_md.write_text(markdown, encoding="utf-8")

    if args.output_json:
        payload = {
            "status": status,
            "message": message,
            "summary_path": str(summary_path),
        }
        Path(args.output_json).write_text(json.dumps(payload, indent=2), encoding="utf-8")

    print(markdown)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
