#!/usr/bin/env python3
"""Export pair-level reduced feedback CSV from CT_235a local_oneway_rows.csv."""

from __future__ import annotations

import csv
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path


REQUIRED_COLUMNS = (
    "step",
    "pair_id",
    "gamma_n",
    "delta_g_eff_m",
    "fn_ref_n",
    "p_max_pa",
    "valid_flag",
    "status",
)

OUTPUT_COLUMNS = (
    "step",
    "pair_id",
    "gamma_n",
    "delta_g_eff_m",
    "fn_ref_n",
    "p_max_pa",
    "valid_flag",
    "status",
)


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def parse_finite_float(text: str, label: str) -> float:
    try:
        value = float(text)
    except ValueError as exc:
        fail(f"{label} must be a float: {exc}")
    if not math.isfinite(value):
        fail(f"{label} must be finite")
    return value


def parse_int(text: str, label: str) -> int:
    try:
        return int(text)
    except ValueError as exc:
        fail(f"{label} must be an integer: {exc}")


def aggregate_status(status_values: list[str], all_valid: bool) -> str:
    lowered = [value.lower() for value in status_values if value]
    if all_valid and all(value == "1" or value == "ok" or value == "valid" or value.startswith("ok_")
                         for value in lowered):
        return "ok"
    unique_status = sorted({value for value in status_values if value})
    if len(unique_status) == 1:
        return unique_status[0]
    if all_valid:
        return "mixed_valid"
    return "mixed_invalid"


def main() -> int:
    if len(sys.argv) != 3:
        print(
            "Usage: python3 scripts/export_local_feedback_reduced_from_local_oneway_generic.py "
            "<local_oneway_rows.csv> <out.csv>",
            file=sys.stderr,
        )
        return 2

    source_csv = Path(sys.argv[1]).resolve()
    output_csv = Path(sys.argv[2]).resolve()

    if not source_csv.is_file():
        fail(f"missing input CSV: {source_csv}")

    with source_csv.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))

    if not rows:
        fail(f"input CSV has no rows: {source_csv}")

    missing = [name for name in REQUIRED_COLUMNS if name not in rows[0]]
    if missing:
        fail(f"input CSV missing required columns: {missing!r}")

    grouped_rows: dict[tuple[int, int], list[dict[str, object]]] = defaultdict(list)
    for row_index, row in enumerate(rows, start=1):
        step = parse_int(row["step"], f"row {row_index} step")
        pair_id = parse_int(row["pair_id"], f"row {row_index} pair_id")
        gamma_n = parse_finite_float(row["gamma_n"], f"row {row_index} gamma_n")
        delta_g_eff_m = parse_finite_float(
            row["delta_g_eff_m"], f"row {row_index} delta_g_eff_m"
        )
        fn_ref_n = parse_finite_float(row["fn_ref_n"], f"row {row_index} fn_ref_n")
        p_max_pa = parse_finite_float(row["p_max_pa"], f"row {row_index} p_max_pa")
        valid_flag = parse_int(row["valid_flag"], f"row {row_index} valid_flag")
        status = row["status"].strip()
        if not status:
            fail(f"row {row_index} status must be non-empty")

        grouped_rows[(step, pair_id)].append(
            {
                "gamma_n": gamma_n,
                "delta_g_eff_m": delta_g_eff_m,
                "fn_ref_n": fn_ref_n,
                "p_max_pa": p_max_pa,
                "valid_flag": valid_flag,
                "status": status,
            }
        )

    output_csv.parent.mkdir(parents=True, exist_ok=True)
    with output_csv.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=OUTPUT_COLUMNS)
        writer.writeheader()

        for step, pair_id in sorted(grouped_rows):
            records = grouped_rows[(step, pair_id)]
            gamma_n = statistics.fmean(
                float(record["gamma_n"]) for record in records
            )
            delta_g_eff_m = statistics.fmean(
                float(record["delta_g_eff_m"]) for record in records
            )
            fn_ref_n = sum(float(record["fn_ref_n"]) for record in records)
            p_max_pa = max(float(record["p_max_pa"]) for record in records)
            all_valid = all(int(record["valid_flag"]) == 1 for record in records)
            status = aggregate_status(
                [str(record["status"]) for record in records],
                all_valid=all_valid,
            )

            writer.writerow(
                {
                    "step": step,
                    "pair_id": pair_id,
                    "gamma_n": f"{gamma_n:.16e}",
                    "delta_g_eff_m": f"{delta_g_eff_m:.16e}",
                    "fn_ref_n": f"{fn_ref_n:.16e}",
                    "p_max_pa": f"{p_max_pa:.16e}",
                    "valid_flag": 1 if all_valid else 0,
                    "status": status,
                }
            )

    print(
        "PASS export_local_feedback_reduced_from_local_oneway_generic "
        f"source_rows={len(rows)} feedback_rows={len(grouped_rows)} "
        f"source_csv={source_csv} out_csv={output_csv}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
