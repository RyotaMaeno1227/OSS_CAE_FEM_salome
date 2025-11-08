#!/usr/bin/env python3
"""
Validate and normalise nightly regression CSV outputs.

The nightly workflow uploads `artifacts/nightly/regression_results.csv`.  This
helper ensures the header matches expectations, rewrites the file in a stable
order, and flags any unexpected columns so regressions are easy to spot.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import List, Sequence, Tuple

EXPECTED_COLUMNS = [
    "test_name",
    "mode",
    "status",
    "exit_code",
    "duration_seconds",
    "log_path",
    "log_digest",
    "started_at",
    "finished_at",
]

SUMMARY_COLUMNS = [
    "test_name",
    "baseline_mode",
    "other_mode",
    "baseline_status",
    "other_status",
    "diff_path",
]

VALID_STATUSES = {"passed", "failed", "timeout"}


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Lint nightly regression CSV files and optionally rewrite them in a canonical format."
    )
    parser.add_argument(
        "input",
        nargs="?",
        default="artifacts/nightly/regression_results.csv",
        help="Path to the nightly regression CSV (default: %(default)s).",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Rewrite the CSV in-place with sorted rows and canonical header order.",
    )
    parser.add_argument(
        "--summary",
        help="Optional path to the mode-diff summary CSV for validation.",
    )
    return parser.parse_args(argv)


def load_rows(path: Path) -> Tuple[List[str], List[dict]]:
    with path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        header = reader.fieldnames or []
        rows = [dict(row) for row in reader]
    return header, rows


def ensure_header(header: List[str]) -> List[str]:
    missing = [column for column in EXPECTED_COLUMNS if column not in header]
    extras = [column for column in header if column not in EXPECTED_COLUMNS]
    issues: List[str] = []
    if missing:
        issues.append(f"Missing columns: {', '.join(missing)}")
    if extras:
        issues.append(f"Unexpected columns: {', '.join(extras)}")
    if issues:
        raise RuntimeError("; ".join(issues))
    return EXPECTED_COLUMNS


def sort_rows(rows: List[dict]) -> List[dict]:
    return sorted(rows, key=lambda row: (row.get("test_name", ""), row.get("mode", "")))


def rewrite(path: Path, rows: List[dict], header: List[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=header)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: row.get(key, "") for key in header})


def lint_rows(rows: List[dict]) -> List[str]:
    issues: List[str] = []
    for index, row in enumerate(rows, start=2):  # account for header line
        status = row.get("status", "")
        if status not in VALID_STATUSES:
            issues.append(f"Line {index}: unsupported status '{status}'")
        duration = row.get("duration_seconds", "")
        try:
            float(duration)
        except (TypeError, ValueError):
            issues.append(f"Line {index}: invalid duration '{duration}'")
        exit_code = row.get("exit_code", "")
        if exit_code:
            try:
                int(exit_code)
            except (TypeError, ValueError):
                issues.append(f"Line {index}: invalid exit_code '{exit_code}'")
    return issues


def lint_summary(path: Path, fix: bool) -> List[str]:
    header, rows = load_rows(path)
    issues: List[str] = []
    if header and header != SUMMARY_COLUMNS:
        issues.append(f"{path}: unexpected columns {header}, expected {SUMMARY_COLUMNS}")
    for index, row in enumerate(rows, start=2):
        for key in ("baseline_status", "other_status"):
            status = row.get(key, "")
            if status and status not in VALID_STATUSES:
                issues.append(f"{path}: line {index} invalid {key} '{status}'")
    if fix and header == SUMMARY_COLUMNS:
        rewrite(path, sorted(rows, key=lambda row: (row.get("test_name", ""), row.get("baseline_mode", ""))), SUMMARY_COLUMNS)
    return issues


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    csv_path = Path(args.input).expanduser()
    if not csv_path.exists():
        print(f"Nightly CSV not found: {csv_path}")
        return 1

    header, rows = load_rows(csv_path)
    try:
        canonical_header = ensure_header(header)
    except RuntimeError as exc:
        print(f"Header lint failed: {exc}")
        return 1

    issues = lint_rows(rows)
    if args.summary:
        summary_path = Path(args.summary).expanduser()
        if summary_path.exists():
            issues.extend(lint_summary(summary_path, args.fix))
    if issues:
        for issue in issues:
            print(issue)
    if args.fix:
        rewrite(csv_path, sort_rows(rows), canonical_header)

    return 1 if issues else 0


if __name__ == "__main__":
    raise SystemExit(main())
