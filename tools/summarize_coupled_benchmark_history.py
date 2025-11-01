#!/usr/bin/env python3
"""Summarise Coupled benchmark CSV history into Markdown/HTML tables."""

from __future__ import annotations

import argparse
import csv
import statistics
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


@dataclass
class BenchRow:
    eq_count: int
    epsilon: float
    max_condition: float
    avg_condition: float
    avg_solve_time_us: float
    drop_events: int
    drop_index_mask: int
    recovery_events: int
    avg_recovery_steps: float
    max_recovery_steps: int
    unrecovered_drops: int
    max_pending_steps: int

    @staticmethod
    def from_dict(row: Dict[str, str]) -> "BenchRow":
        return BenchRow(
            eq_count=int(row["eq_count"]),
            epsilon=float(row["epsilon"]),
            max_condition=float(row["max_condition"]),
            avg_condition=float(row["avg_condition"]),
            avg_solve_time_us=float(row["avg_solve_time_us"]),
            drop_events=int(row["drop_events"]),
            drop_index_mask=int(row["drop_index_mask"]),
            recovery_events=int(row["recovery_events"]),
            avg_recovery_steps=float(row["avg_recovery_steps"]),
            max_recovery_steps=int(row["max_recovery_steps"]),
            unrecovered_drops=int(row["unrecovered_drops"]),
            max_pending_steps=int(row["max_pending_steps"]),
        )


@dataclass
class RunRecord:
    path: Path
    timestamp: float
    rows: List[BenchRow] = field(default_factory=list)


@dataclass
class EqAggregate:
    eq_count: int
    sample_count: int
    avg_solve_mean: float
    avg_solve_max: float
    max_condition: float
    drop_mean: float
    unrecovered_mean: float


def discover_inputs(inputs: Sequence[str]) -> List[Path]:
    discovered: List[Path] = []
    for raw in inputs or ["data/coupled_benchmark_metrics.csv"]:
        path = Path(raw).expanduser()
        if path.is_dir():
            discovered.extend(sorted(path.glob("*.csv")))
        else:
            if any(char in raw for char in "*?[]"):
                discovered.extend(sorted(Path().glob(raw)))
            else:
                discovered.append(path)
    unique: List[Path] = []
    seen = set()
    for item in discovered:
        if item.exists() and item not in seen:
            unique.append(item)
            seen.add(item)
    return unique


def load_run(path: Path) -> RunRecord:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = [BenchRow.from_dict(row) for row in reader]
    return RunRecord(path=path, timestamp=path.stat().st_mtime, rows=rows)


def aggregate_by_eq(runs: Iterable[RunRecord]) -> List[EqAggregate]:
    bucket: Dict[int, List[BenchRow]] = {}
    for run in runs:
        for row in run.rows:
            bucket.setdefault(row.eq_count, []).append(row)

    aggregates: List[EqAggregate] = []
    for eq_count, rows in sorted(bucket.items()):
        avg_solve_samples = [row.avg_solve_time_us for row in rows]
        drop_samples = [row.drop_events for row in rows]
        unrecovered_samples = [row.unrecovered_drops for row in rows]
        aggregates.append(
            EqAggregate(
                eq_count=eq_count,
                sample_count=len(rows),
                avg_solve_mean=statistics.mean(avg_solve_samples) if avg_solve_samples else 0.0,
                avg_solve_max=max(avg_solve_samples) if avg_solve_samples else 0.0,
                max_condition=max(row.max_condition for row in rows) if rows else 0.0,
                drop_mean=statistics.mean(drop_samples) if drop_samples else 0.0,
                unrecovered_mean=statistics.mean(unrecovered_samples) if unrecovered_samples else 0.0,
            )
        )
    return aggregates


def render_markdown(runs: List[RunRecord], aggregates: List[EqAggregate]) -> str:
    lines: List[str] = []
    lines.append("# Coupled Benchmark Report")
    lines.append("")
    lines.append("## Aggregate by Equation Count")
    lines.append("| eq_count | samples | mean solve [µs] | max solve [µs] | max condition | mean drops | mean unrecovered |")
    lines.append("|---------:|--------:|----------------:|---------------:|--------------:|-----------:|-----------------:|")
    for agg in aggregates:
        lines.append(
            f"| {agg.eq_count} | {agg.sample_count} | {agg.avg_solve_mean:.3f} | {agg.avg_solve_max:.3f} | "
            f"{agg.max_condition:.3e} | {agg.drop_mean:.2f} | {agg.unrecovered_mean:.2f} |"
        )

    if runs:
        latest = max(runs, key=lambda run: run.timestamp)
        lines.append("")
        lines.append("## Latest Run Snapshot")
        lines.append(f"Source: `{latest.path}`")
        lines.append("")
        lines.append("| eq_count | epsilon | avg solve [µs] | max condition | drops | unrecovered |")
        lines.append("|---------:|--------:|----------------:|--------------:|------:|------------:|")
        for row in sorted(latest.rows, key=lambda r: (r.eq_count, r.epsilon)):
            lines.append(
                f"| {row.eq_count} | {row.epsilon:.1e} | {row.avg_solve_time_us:.3f} | {row.max_condition:.3e} | "
                f"{row.drop_events} | {row.unrecovered_drops} |"
            )

    if runs:
        lines.append("")
        lines.append("## Chronology")
        lines.append("| run | timestamp (UTC) | file |")
        lines.append("|----:|-----------------|------|")
        for index, run in enumerate(sorted(runs, key=lambda r: r.timestamp), start=1):
            stamp = datetime.fromtimestamp(run.timestamp, tz=timezone.utc).isoformat()
            lines.append(f"| {index} | {stamp} | `{run.path}` |")

    return "\n".join(lines) + "\n"


def render_html(markdown: str) -> str:
    # Basic conversion: wrap Markdown string in <pre> while retaining tables.
    # For tables, replace Markdown delimiters with <table> elements.
    rows = markdown.strip().splitlines()
    html_lines: List[str] = ["<html>", "<head><meta charset=\"utf-8\"><title>Coupled Benchmark Report</title></head>", "<body>"]
    table_buffer: List[str] = []

    def flush_table() -> None:
        nonlocal table_buffer
        if not table_buffer:
            return
        header = table_buffer[0].strip("| ").split("|")
        html_lines.append("<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">")
        html_lines.append("  <thead><tr>" + "".join(f"<th>{cell.strip()}</th>" for cell in header) + "</tr></thead>")
        html_lines.append("  <tbody>")
        for row in table_buffer[2:]:
            cells = row.strip("| ").split("|")
            html_lines.append("    <tr>" + "".join(f"<td>{cell.strip()}</td>" for cell in cells) + "</tr>")
        html_lines.append("  </tbody></table>")
        table_buffer = []

    for line in rows:
        if line.startswith("|"):
            table_buffer.append(line)
            continue
        flush_table()
        if line.startswith("#"):
            level = len(line) - len(line.lstrip("#"))
            content = line.lstrip("# ")
            html_lines.append(f"<h{level}>{content}</h{level}>")
        elif line:
            html_lines.append(f"<p>{line}</p>")
    flush_table()
    html_lines.append("</body></html>")
    return "\n".join(html_lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarise Coupled benchmark CSV history.")
    parser.add_argument("inputs", nargs="*", help="CSV paths, directories, or glob patterns.")
    parser.add_argument("--output-md", help="Write Markdown report to the specified path.")
    parser.add_argument("--output-html", help="Write HTML report to the specified path.")
    parser.add_argument("--latest", type=int, help="Limit processing to the most recent N runs.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    paths = discover_inputs(args.inputs)
    if not paths:
        print("No benchmark CSV files found.")
        return 1

    runs = [load_run(path) for path in paths]
    runs.sort(key=lambda run: run.timestamp, reverse=True)
    if args.latest is not None and args.latest > 0:
        runs = runs[: args.latest]

    aggregates = aggregate_by_eq(runs)
    markdown = render_markdown(runs, aggregates)

    if args.output_md:
        md_path = Path(args.output_md).expanduser()
        md_path.parent.mkdir(parents=True, exist_ok=True)
        md_path.write_text(markdown, encoding="utf-8")
        print(f"Wrote Markdown report to {md_path}")
    else:
        print(markdown)

    if args.output_html:
        html = render_html(markdown)
        html_path = Path(args.output_html).expanduser()
        html_path.parent.mkdir(parents=True, exist_ok=True)
        html_path.write_text(html, encoding="utf-8")
        print(f"Wrote HTML report to {html_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
