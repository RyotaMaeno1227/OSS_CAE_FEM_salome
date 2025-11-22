#!/usr/bin/env python3
"""
Check chrono-2d CSV schema for kkt_descriptor_actions_local.csv.
Required columns:
  time, case, method, condition_bound, condition_spectral, min_pivot, max_pivot
Fails with exit 1 if columns are missing or file is unreadable.
"""
import csv
import sys
from pathlib import Path

REQUIRED = [
    "time",
    "case",
    "method",
    "condition_bound",
    "condition_spectral",
    "min_pivot",
    "max_pivot",
]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_chrono2d_csv_schema.py <csv_path>", file=sys.stderr)
        return 1
    path = Path(sys.argv[1])
    if not path.exists():
        print(f"error: file not found: {path}", file=sys.stderr)
        return 1
    try:
        with path.open(newline="") as f:
            reader = csv.reader(f)
            header = next(reader)
    except Exception as exc:  # pragma: no cover
        print(f"error: failed to read {path}: {exc}", file=sys.stderr)
        return 1

    missing = [c for c in REQUIRED if c not in header]
    if missing:
        print(f"error: missing columns: {missing}", file=sys.stderr)
        print(f"header: {header}", file=sys.stderr)
        return 1

    print(f"ok: {path} cols={len(header)} contains {REQUIRED}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
