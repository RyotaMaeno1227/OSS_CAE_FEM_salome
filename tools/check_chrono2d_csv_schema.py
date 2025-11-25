#!/usr/bin/env python3
"""Validate chrono-2d CSV schema and optionally emit a fresh sample.

Usage:
  python tools/check_chrono2d_csv_schema.py --csv chrono-2d/artifacts/kkt_descriptor_actions_local.csv
  python tools/check_chrono2d_csv_schema.py --emit-sample chrono-2d/artifacts/kkt_descriptor_actions_local.csv
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

TEMPLATE = Path("docs/chrono_2d_cases_template.csv")
EXPECTED_HEADER = (
    "time,case,method,vn,vt,mu_s,mu_d,stick,condition_bound,condition_spectral,min_pivot,max_pivot\n"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, help="CSV to validate")
    parser.add_argument("--emit-sample", type=Path, help="Write template sample to this path")
    return parser.parse_args()


def validate(csv_path: Path) -> bool:
    text = csv_path.read_text(encoding="utf-8")
    if not text.startswith(EXPECTED_HEADER):
        sys.stderr.write(f"[chrono-2d] Header mismatch in {csv_path}\n")
        return False
    return True


def emit_sample(dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text(TEMPLATE.read_text(encoding="utf-8"), encoding="utf-8")
    print(f"[chrono-2d] Wrote sample to {dst}")


def main() -> int:
    args = parse_args()
    if args.csv:
        ok = validate(args.csv)
        if ok:
            print(f"[chrono-2d] OK: {args.csv}")
        else:
            return 1
    if args.emit_sample:
        emit_sample(args.emit_sample)
    if not args.csv and not args.emit_sample:
        print("No action. Use --csv to validate or --emit-sample to write template")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
