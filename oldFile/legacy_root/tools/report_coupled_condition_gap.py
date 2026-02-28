#!/usr/bin/env python3
"""Generate a Markdown report comparing bound-based and spectral condition numbers."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


@dataclass
class BenchRow:
    eq_count: int
    epsilon: float
    max_condition: float
    max_condition_spectral: float
    max_condition_gap: float
    avg_condition_gap: float
    scenario: str = "default"

    @staticmethod
    def from_dict(row: Dict[str, str]) -> "BenchRow":
        return BenchRow(
            eq_count=int(row["eq_count"]),
            epsilon=float(row["epsilon"]),
            max_condition=float(row["max_condition"]),
            max_condition_spectral=float(row.get("max_condition_spectral", row["max_condition"])),
            max_condition_gap=float(row.get("max_condition_gap", "0.0")),
            avg_condition_gap=float(row.get("avg_condition_gap", "0.0")),
            scenario=row.get("scenario", "default"),
        )

    def ratio(self) -> float:
        if self.max_condition <= 0.0 or not math.isfinite(self.max_condition):
            return math.inf
        return self.max_condition_spectral / self.max_condition


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report differences between bound-based and spectral condition numbers."
    )
    parser.add_argument(
        "--input",
        default="data/coupled_benchmark_metrics.csv",
        help="Input CSV produced by bench_coupled_constraint (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        default="docs/coupled_condition_gap_report.md",
        help="Destination Markdown file (default: %(default)s)",
    )
    return parser.parse_args()


def load_rows(path: Path) -> List[BenchRow]:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        required = {
            "eq_count",
            "epsilon",
            "max_condition",
            "max_condition_spectral",
            "max_condition_gap",
            "avg_condition_gap",
        }
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise RuntimeError(f"CSV missing columns: {', '.join(sorted(missing))}")
        return [BenchRow.from_dict(row) for row in reader]


def aggregate_by_eq(rows: Iterable[BenchRow]) -> Dict[int, Tuple[float, float, float, float]]:
    buckets: Dict[int, List[BenchRow]] = {}
    for row in rows:
        buckets.setdefault(row.eq_count, []).append(row)
    aggregates: Dict[int, Tuple[float, float, float, float]] = {}
    for eq, items in buckets.items():
        gaps = [item.max_condition_gap for item in items]
        gap_means = [item.avg_condition_gap for item in items]
        ratios = [item.ratio() for item in items if math.isfinite(item.ratio())]
        aggregates[eq] = (
            max(gaps) if gaps else 0.0,
            sum(gap_means) / len(gap_means) if gap_means else 0.0,
            max(ratios) if ratios else 0.0,
            sum(ratios) / len(ratios) if ratios else 0.0,
        )
    return aggregates


def _compute_histogram(values: List[float], bins: int = 10) -> List[Tuple[float, float, int]]:
    filtered = [value for value in values if math.isfinite(value) and value >= 0.0]
    if not filtered:
        return []
    minimum = min(filtered)
    maximum = max(filtered)
    if minimum == maximum:
        return [(minimum, maximum, len(filtered))]
    width = (maximum - minimum) / bins if bins > 0 else maximum - minimum
    if width <= 0.0:
        width = max(abs(maximum), 1.0)
    edges = [minimum + idx * width for idx in range(bins)]
    counts = [0 for _ in range(bins)]
    for value in filtered:
        if value == maximum:
            counts[-1] += 1
            continue
        idx = int((value - minimum) / width)
        if idx < 0:
            idx = 0
        elif idx >= bins:
            idx = bins - 1
        counts[idx] += 1
    histogram: List[Tuple[float, float, int]] = []
    for idx, count in enumerate(counts):
        start = edges[idx]
        end = start + width
        histogram.append((start, end, count))
    return histogram


def _format_histogram(values: List[float]) -> List[str]:
    histogram = _compute_histogram(values)
    lines: List[str] = []
    if not histogram:
        lines.append("_No spectral gap data available for histogram._")
        return lines
    total = sum(count for _, _, count in histogram) or 1
    lines.append("| Range (gap) | Count | Share |")
    lines.append("|-------------|------:|------:|")
    for start, end, count in histogram:
        share = (count / total) * 100.0
        lines.append(f"| [{start:.2e}, {end:.2e}) | {count} | {share:5.1f}% |")
    return lines


def render_markdown(rows: List[BenchRow], source: Path) -> str:
    timestamp = datetime.now(timezone.utc).isoformat()
    aggregates = aggregate_by_eq(rows)
    lines: List[str] = []
    lines.append("# Coupled Condition Gap Report")
    lines.append("")
    lines.append(f"> Generated {timestamp} from `{source}`")
    lines.append("")
    if not rows:
        lines.append("_No rows available._")
        return "\n".join(lines) + "\n"

    lines.append("## Per-case Overview")
    lines.append(
        "| eq_count | epsilon | scenario | max κ (bound) | max κ (spectral) | gap | ratio (spectral/bound) |"
    )
    lines.append(
        "|---------:|--------:|:---------|--------------:|------------------:|----:|------------------------:|"
    )
    for row in sorted(rows, key=lambda r: (r.eq_count, r.epsilon)):
        ratio = row.ratio()
        ratio_text = f"{ratio:.3f}" if math.isfinite(ratio) else "inf"
        lines.append(
            f"| {row.eq_count} | {row.epsilon:.1e} | {row.scenario} | {row.max_condition:.3e} | "
            f"{row.max_condition_spectral:.3e} | {row.max_condition_gap:.3e} | {ratio_text} |"
        )

    lines.append("")
    lines.append("## Aggregate by Equation Count")
    lines.append("| eq_count | max gap | mean gap | max ratio | mean ratio |")
    lines.append("|---------:|--------:|---------:|----------:|-----------:|")
    for eq in sorted(aggregates.keys()):
        max_gap, mean_gap, max_ratio, mean_ratio = aggregates[eq]
        lines.append(
            f"| {eq} | {max_gap:.3e} | {mean_gap:.3e} | {max_ratio:.3f} | {mean_ratio:.3f} |"
        )

    lines.append("")
    lines.append("## Spectral Gap Histogram")
    histogram_lines = _format_histogram([row.max_condition_gap for row in rows])
    lines.extend(histogram_lines)

    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    input_path = Path(args.input).expanduser()
    if not input_path.exists():
        raise FileNotFoundError(f"Input CSV not found: {input_path}")
    rows = load_rows(input_path)
    markdown = render_markdown(rows, input_path)
    output_path = Path(args.output).expanduser()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(markdown, encoding="utf-8")
    print(f"Wrote report to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
