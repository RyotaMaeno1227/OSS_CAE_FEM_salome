#!/usr/bin/env python3
"""
Compare coupled benchmark metrics against a Chrono main baseline.

The script expects two CSV files with the schema produced by
`tools/run_coupled_benchmark.py`. It emits a Markdown report highlighting
differences in timing and condition-number metrics to streamline regression
analysis.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple

Key = Tuple[int, float, str]


@dataclass(frozen=True)
class BenchmarkRow:
    eq_count: int
    epsilon: float
    scenario: str
    metrics: Dict[str, float]


INTERESTING_METRICS = [
    "avg_solve_time_us",
    "max_condition",
    "max_condition_spectral",
    "max_condition_gap",
    "max_pending_steps",
]


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare coupled benchmark CSV files and highlight numeric diffs."
    )
    parser.add_argument(
        "--baseline",
        required=True,
        help="Baseline CSV path (typically chrono-main results).",
    )
    parser.add_argument(
        "--current",
        default="data/coupled_benchmark_metrics.csv",
        help="Current CSV path to compare (default: %(default)s).",
    )
    parser.add_argument(
        "--output",
        help="Optional output Markdown file (default: stdout).",
    )
    parser.add_argument(
        "--csv-output",
        help="Optional CSV file capturing the same deltas.",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.02,
        help="Relative change threshold for reporting (default: %(default)s).",
    )
    return parser.parse_args(argv)


def load_csv(path: Path) -> Dict[Key, BenchmarkRow]:
    rows: Dict[Key, BenchmarkRow] = {}
    with path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for raw in reader:
            eq_count = int(raw["eq_count"])
            epsilon = float(raw["epsilon"])
            scenario = raw.get("scenario", "default")
            metrics = {
                key: float(raw[key]) for key in INTERESTING_METRICS if key in raw and raw[key]
            }
            rows[(eq_count, epsilon, scenario)] = BenchmarkRow(eq_count, epsilon, scenario, metrics)
    return rows


def relative_change(baseline: float, current: float) -> float:
    if baseline == 0.0:
        return float("inf") if current != 0.0 else 0.0
    return (current - baseline) / baseline


def format_key(key: Key) -> str:
    eq_count, epsilon, scenario = key
    return f"eq={eq_count} ε={epsilon:.2e} ({scenario})"


DiffRow = Dict[str, str]


def collect_differences(
    baseline_rows: Dict[Key, BenchmarkRow],
    current_rows: Dict[Key, BenchmarkRow],
    threshold: float,
) -> Tuple[List[str], List[DiffRow]]:
    lines: List[str] = ["# Coupled Benchmark Comparison", ""]
    missing_baseline = sorted(set(current_rows) - set(baseline_rows))
    missing_current = sorted(set(baseline_rows) - set(current_rows))
    if missing_baseline:
        lines.append("## New scenarios (not in baseline)")
        for key in missing_baseline:
            lines.append(f"- {format_key(key)}")
        lines.append("")
    if missing_current:
        lines.append("## Missing scenarios (baseline only)")
        for key in missing_current:
            lines.append(f"- {format_key(key)}")
        lines.append("")

    diff_rows: List[DiffRow] = []
    lines.append("## Metric deltas")
    header = "| Scenario | Metric | Baseline | Current | Δ | Δ% |"
    sep = "|---|---|---:|---:|---:|---:|"
    lines.extend([header, sep])

    for key in sorted(set(baseline_rows) & set(current_rows)):
        baseline = baseline_rows[key]
        current = current_rows[key]
        for metric in INTERESTING_METRICS:
            if metric not in baseline.metrics or metric not in current.metrics:
                continue
            base_value = baseline.metrics[metric]
            cur_value = current.metrics[metric]
            delta = cur_value - base_value
            rel = relative_change(base_value, cur_value)
            if abs(rel) < threshold:
                continue
            rel_percent = float("inf") if rel == float("inf") else rel * 100.0
            rel_text = "inf" if rel_percent == float("inf") else f"{rel_percent:.1f}%"
            lines.append(
                f"| {format_key(key)} | {metric} | {base_value:.3e} | {cur_value:.3e} | "
                f"{delta:.3e} | {rel_text} |"
            )
            diff_rows.append(
                {
                    "scenario": format_key(key),
                    "metric": metric,
                    "baseline": f"{base_value:.8e}",
                    "current": f"{cur_value:.8e}",
                    "delta": f"{delta:.8e}",
                    "delta_percent": "inf" if rel_percent == float("inf") else f"{rel_percent:.3f}",
                }
            )

    if len(lines) == 4:  # no differences appended
        lines.append("| – | – | – | – | – | – |")

    lines.append("")
    return lines, diff_rows


def write_csv(path: Path, rows: List[DiffRow]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["scenario", "metric", "baseline", "current", "delta", "delta_percent"],
        )
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    baseline_path = Path(args.baseline).expanduser()
    current_path = Path(args.current).expanduser()
    baseline_rows = load_csv(baseline_path)
    current_rows = load_csv(current_path)
    markdown_lines, diff_rows = collect_differences(baseline_rows, current_rows, args.threshold)
    report = "\n".join(markdown_lines)

    if args.output:
        output_path = Path(args.output).expanduser()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(report, encoding="utf-8")
    else:
        print(report)
    if args.csv_output:
        write_csv(Path(args.csv_output).expanduser(), diff_rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
