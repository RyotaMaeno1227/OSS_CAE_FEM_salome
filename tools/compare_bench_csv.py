#!/usr/bin/env python3
"""
Compare bench CSV (bench_pivots.csv) against previous snapshot to detect drift.
Expected columns: case,threads,time_us
Outputs a warning if time_us exceeds threshold * previous.
Optional flag `--fail-on-drift` makes the script exit 1 if drift is detected.
"""
import csv
import sys
from pathlib import Path
from statistics import mean


def load(path: Path):
    rows = []
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                rows.append(
                    (
                        row["case"],
                        int(row["threads"]),
                        float(row["time_us"]),
                    )
                )
            except Exception:
                continue
    return rows


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("current")
    parser.add_argument("--previous", default=None, help="path to previous bench_pivots.csv")
    parser.add_argument("--threshold", type=float, default=1.5, help="drift threshold ratio")
    parser.add_argument("--fail-on-drift", action="store_true", help="exit 1 when drift is detected")
    args = parser.parse_args()

    cur_path = Path(args.current)
    if not cur_path.exists():
        print(f"no bench file: {cur_path}")
        return 0

    cur = load(cur_path)
    print(f"current rows: {len(cur)}")

    if not args.previous:
        print("no previous provided; skip drift check")
        return 0

    prev_path = Path(args.previous)
    if not prev_path.exists():
        print(f"no previous file: {prev_path}; skip drift check")
        return 0

    prev = load(prev_path)
    if not prev:
        print("previous empty; skip drift check")
        return 0

    prev_dict = {(c, t): v for c, t, v in prev}
    warnings = []
    for case, threads, time_us in cur:
        key = (case, threads)
        if key not in prev_dict:
            continue
        base = prev_dict[key]
        if time_us > args.threshold * base:
            warnings.append((case, threads, base, time_us))

    if warnings:
        print(f"WARNING: drift detected (>{args.threshold}x):")
        for w in warnings:
            case, threads, base, now = w
            print(f"  {case} threads={threads}: prev={base:.2f}us now={now:.2f}us")
        return 1 if args.fail_on_drift else 0

    print("no drift detected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
