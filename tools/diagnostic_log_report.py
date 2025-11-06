#!/usr/bin/env python3
"""
Produce a Markdown summary from coupled constraint diagnostic logs.

The coupled endurance tests emit CSV logs with diagnostic fields such as rank,
condition number, and min/max pivot magnitudes.  This CLI extracts the salient
metrics and renders a compact Markdown report suitable for pasting into pull
request comments or issue threads.
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from statistics import mean, median
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


# Column name aliases frequently encountered in diagnostic dumps.
RANK_COLUMNS = ["rank", "diagnostics_rank", "diagnostics.rank"]
CONDITION_COLUMNS = ["condition_number", "condition", "diagnostics_condition"]
MIN_PIVOT_COLUMNS = ["min_pivot", "pivot_min", "diagnostics_min_pivot"]
MAX_PIVOT_COLUMNS = ["max_pivot", "pivot_max", "diagnostics_max_pivot"]
FLAGS_COLUMNS = ["flags", "diagnostics_flags"]
TIME_COLUMNS = ["time", "step_time", "timestamp"]


@dataclass
class DiagnosticMetrics:
    samples: int
    ranks: List[int]
    conditions: List[float]
    min_pivots: List[float]
    max_pivots: List[float]
    flags: List[int]
    times: List[float]


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarise coupled diagnostic CSV logs as Markdown."
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help="Diagnostic CSV files containing rank/condition/pivot columns.",
    )
    parser.add_argument(
        "--condition-threshold",
        type=float,
        default=5.0e8,
        help="Condition numbers above this threshold are highlighted (default: %(default)s).",
    )
    parser.add_argument(
        "--pivot-threshold",
        type=float,
        help="Optional minimum acceptable pivot magnitude; values below trigger warnings.",
    )
    parser.add_argument(
        "--output",
        help="Write the combined Markdown report to the given path instead of stdout.",
    )
    parser.add_argument(
        "--max-anomalies",
        type=int,
        default=10,
        help="Maximum number of anomalous rows to list per file (default: %(default)s).",
    )
    return parser.parse_args(argv)


def _resolve_column(fieldnames: List[str], candidates: List[str]) -> Optional[str]:
    lowered = {name.lower(): name for name in fieldnames}
    for candidate in candidates:
        if candidate in fieldnames:
            return candidate
        if candidate.lower() in lowered:
            return lowered[candidate.lower()]
    return None


def _parse_float(value: str) -> float:
    if value is None or value == "":
        return math.nan
    try:
        return float(value)
    except ValueError:
        return math.nan


def _parse_int(value: str) -> int:
    if value is None or value == "":
        return 0
    try:
        return int(float(value))
    except ValueError:
        return 0


def load_metrics(path: Path) -> DiagnosticMetrics:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames:
            raise RuntimeError(f"{path}: missing header row.")

        rank_col = _resolve_column(reader.fieldnames, RANK_COLUMNS)
        cond_col = _resolve_column(reader.fieldnames, CONDITION_COLUMNS)
        min_pivot_col = _resolve_column(reader.fieldnames, MIN_PIVOT_COLUMNS)
        max_pivot_col = _resolve_column(reader.fieldnames, MAX_PIVOT_COLUMNS)
        flags_col = _resolve_column(reader.fieldnames, FLAGS_COLUMNS)
        time_col = _resolve_column(reader.fieldnames, TIME_COLUMNS)

        if not cond_col:
            raise RuntimeError(f"{path}: could not locate condition column.")
        if not rank_col:
            raise RuntimeError(f"{path}: could not locate rank column.")
        if not min_pivot_col or not max_pivot_col:
            raise RuntimeError(f"{path}: missing pivot columns.")

        ranks: List[int] = []
        conditions: List[float] = []
        min_pivots: List[float] = []
        max_pivots: List[float] = []
        flags: List[int] = []
        times: List[float] = []

        for row in reader:
            ranks.append(_parse_int(row.get(rank_col, "")))
            conditions.append(_parse_float(row.get(cond_col, "")))
            min_pivots.append(_parse_float(row.get(min_pivot_col, "")))
            max_pivots.append(_parse_float(row.get(max_pivot_col, "")))
            if flags_col:
                flags.append(_parse_int(row.get(flags_col, "")))
            if time_col:
                times.append(_parse_float(row.get(time_col, "")))

    return DiagnosticMetrics(
        samples=len(ranks),
        ranks=ranks,
        conditions=conditions,
        min_pivots=min_pivots,
        max_pivots=max_pivots,
        flags=flags,
        times=times,
    )


def _format_float(value: float, precision: str = ".3e") -> str:
    if math.isnan(value):
        return "n/a"
    if abs(value) >= 1e4 or abs(value) <= 1e-3:
        return f"{value:{precision}}"
    return f"{value:.6g}"


def _ratio(count: int, total: int) -> str:
    if total <= 0:
        return "0 (0.00%)"
    percentage = (count / total) * 100.0
    return f"{count} ({percentage:.2f}%)"


def summarise_metrics(
    path: Path,
    metrics: DiagnosticMetrics,
    condition_threshold: float,
    pivot_threshold: Optional[float],
    max_anomalies: int,
) -> str:
    lines: List[str] = []
    lines.append(f"## {path.name}")
    lines.append("")
    lines.append(f"- Samples: {metrics.samples}")
    if metrics.samples == 0:
        lines.append("")
        return "\n".join(lines)

    rank_min = min(metrics.ranks)
    rank_max = max(metrics.ranks)
    rank_zero = sum(1 for rank in metrics.ranks if rank == 0)
    lines.append(f"- Rank range: {rank_min} – {rank_max} ({_ratio(rank_zero, metrics.samples)} zero)")

    cond_max = max(metrics.conditions)
    cond_mean = mean(metrics.conditions)
    cond_median = median(metrics.conditions)
    lines.append(
        f"- Condition: max={_format_float(cond_max)} mean={_format_float(cond_mean)} "
        f"median={_format_float(cond_median)}"
    )

    pivot_min = min(metrics.min_pivots)
    pivot_max = max(metrics.max_pivots)
    lines.append(
        f"- Pivot extrema: min={_format_float(pivot_min, '.3g')} "
        f"max={_format_float(pivot_max, '.3g')}"
    )

    if metrics.flags:
        warning_bits = sum(1 for flag in metrics.flags if flag & 0x2)
        rank_bits = sum(1 for flag in metrics.flags if flag & 0x1)
        lines.append(
            f"- Diagnostics flags: condition warnings {_ratio(warning_bits, metrics.samples)}, "
            f"rank deficiencies {_ratio(rank_bits, metrics.samples)}"
        )

    lines.append("")
    lines.append("| Index | Condition | Rank | Min pivot | Max pivot | Time | Notes |")
    lines.append("| --- | --- | --- | --- | --- | --- | --- |")

    anomalies: List[Tuple[int, float, int, float, float, Optional[float], str]] = []
    for idx, (condition, rank, min_pivot, max_pivot) in enumerate(
        zip(metrics.conditions, metrics.ranks, metrics.min_pivots, metrics.max_pivots)
    ):
        notes: List[str] = []
        if condition_threshold and condition > condition_threshold:
            notes.append(f"condition>{_format_float(condition_threshold)}")
        if rank == 0:
            notes.append("rank=0")
        if pivot_threshold is not None and min_pivot < pivot_threshold:
            notes.append(f"min_pivot<{_format_float(pivot_threshold, '.3g')}")
        if notes:
            time_value = metrics.times[idx] if metrics.times else None
            anomalies.append(
                (
                    idx,
                    condition,
                    rank,
                    min_pivot,
                    max_pivot,
                    time_value,
                    ", ".join(notes),
                )
            )

    if anomalies:
        for entry in anomalies[:max_anomalies]:
            idx, condition, rank, min_pivot, max_pivot, time_value, note = entry
            time_str = _format_float(time_value, ".6g") if time_value is not None else "n/a"
            lines.append(
                f"| {idx} | {_format_float(condition)} | {rank} | "
                f"{_format_float(min_pivot, '.3g')} | {_format_float(max_pivot, '.3g')} | "
                f"{time_str} | {note} |"
            )
        hidden = len(anomalies) - max_anomalies
        if hidden > 0:
            lines.append(f"| … | … | … | … | … | … | (+{hidden} more) |")
    else:
        lines.append("| – | – | – | – | – | – | No anomalies detected |")

    lines.append("")
    return "\n".join(lines)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    report_sections: List[str] = []

    for input_path in args.inputs:
        path = Path(input_path).expanduser()
        metrics = load_metrics(path)
        report_sections.append(
            summarise_metrics(
                path,
                metrics,
                condition_threshold=args.condition_threshold,
                pivot_threshold=args.pivot_threshold,
                max_anomalies=args.max_anomalies,
            )
        )

    report = "\n".join(report_sections)
    if args.output:
        output_path = Path(args.output).expanduser()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(report, encoding="utf-8")
    else:
        print(report)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

